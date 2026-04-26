/**
 * @file imu_emulator.c
 * @brief BNO055 IMU emulator — lock-free triple buffer (W2 fix).
 *
 * Why triple instead of double?
 * ──────────────────────────────
 * The ping-pong (double) buffer was safe against a SINGLE writer update
 * during an I2C transaction.  ANALYSIS.md §W2 identified that a second
 * writer update before the transaction completes is possible: at 400 kHz
 * I2C, an 18-byte read takes ~405 µs, and the sensor thread sleeps ~500 µs.
 * These are close enough that a second wake-up during the transaction occurs
 * under normal scheduling jitter.
 *
 * When the writer updates twice, the second update targets the buffer the
 * ISR is actively reading (because the first flip made it "inactive", and
 * the second flip made it "active" again — straight back to the reader's
 * buffer).  Result: the DUT reads a struct where some bytes are from physics
 * state N and some are from state N+2.  For a flight controller this is an
 * inconsistent snapshot.
 *
 * The triple buffer eliminates this class of bug entirely.  The three buffer
 * roles (writer / pending / reader) form a closed set that rotates via atomic
 * exchanges.  The writer ALWAYS writes to writer_idx, and writer_idx is never
 * equal to reader_idx.  No matter how many updates occur, the ISR's buffer is
 * never touched by the writer.
 *
 * Triple buffer state machine
 * ───────────────────────────
 *   Initial state:
 *     writer_idx  = 1   (thread-local, no atomic needed)
 *     pending_idx = 2   (atomic — shared exchange point)
 *     reader_idx  = 0   (ISR-local, no atomic needed)
 *     Multiset: {0,1,2}  ← always maintained
 *
 *   After writer update:
 *     memcpy(imu_buf[1], new_data)
 *     __DMB()
 *     writer_idx = atomic_set(&pending_idx, 1)   → writer_idx = 2
 *     Multiset: writer=2, pending=1, reader=0  ✓
 *
 *   After ISR snaps (read_requested_cb):
 *     reader_idx = atomic_set(&pending_idx, 0)   → reader_idx = 1
 *     Multiset: writer=2, pending=0, reader=1  ✓
 *     ISR reads imu_buf[1]  ← freshest complete data
 *
 *   Second writer update (during ISR transaction reading buf[1]):
 *     memcpy(imu_buf[2], newer_data)
 *     __DMB()
 *     writer_idx = atomic_set(&pending_idx, 2)   → writer_idx = 0
 *     Multiset: writer=0, pending=2, reader=1  ✓
 *     buf[1] is NOT touched  ← ISR is safe
 *
 * atomic_set() in Zephyr
 * ──────────────────────
 * Zephyr's atomic_set(target, value) sets *target = value and returns the
 * PREVIOUS value of *target.  On Cortex-M this compiles to a single
 * LDREX/STREX pair — a genuine atomic exchange.  Both the writer and the ISR
 * use this to swap their local index with pending_idx in one instruction.
 * Being ISR-safe and single-instruction, no irq_lock is required.
 */

#include "threads/imu_emulator.h"
#include "threads/sensor_emulation.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(imu_emulator, LOG_LEVEL_INF);

/* ══════════════════════════════════════════════════════════════════════════
 * Triple buffer storage
 * ══════════════════════════════════════════════════════════════════════════ */

/** Three complete IMU register images. */
static imu_data_t imu_buf[3];

/*
 * writer_idx  — index of the buffer the sensor thread is writing to.
 *               Only ever accessed from sensor_emulation_thread context.
 *               No atomic protection needed (single writer, no ISR access).
 *
 * pending_idx — index of the most recently completed write.
 *               Shared between the sensor thread (atomic_set after write)
 *               and the I2C ISR (atomic_set at read_requested_cb).
 *               Must be atomic_t.
 *
 * reader_idx  — index of the buffer the I2C ISR is reading during a
 *               transaction.  Only ever accessed from I2C ISR context.
 *               No atomic protection needed (single reader, no thread access).
 *
 * Invariant: { writer_idx, pending_idx, reader_idx } is always a permutation
 * of { 0, 1, 2 }.  The atomic_set exchange preserves this invariant because
 * it is a bijective value swap — no values are created or destroyed.
 */
static int      writer_idx              = 1;
static atomic_t pending_idx             = ATOMIC_INIT(2);
static int      reader_idx              = 0;

/* ══════════════════════════════════════════════════════════════════════════
 * I2C transaction state  (ISR-local — single-core, no protection needed)
 * ══════════════════════════════════════════════════════════════════════════ */

/** BNO055 register pointer; set by write_received_cb, advanced per byte. */
static uint8_t imu_current_reg = 0x00;

/* ══════════════════════════════════════════════════════════════════════════
 * Register byte servant
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Uses GCC/Clang case-range extension (case A ... B), standard in practice
 * for Zephyr/embedded targets.  Replace with explicit case labels if needed.
 */
static uint8_t serve_imu_byte(int buf_idx, uint8_t reg)
{
    const imu_data_t *d = &imu_buf[buf_idx];
    const uint8_t    *raw;
    uint8_t           offset;

    switch (reg) {

    /* Gyroscope data: regs 0x14–0x19 */
    case IMU_REG_GYRO_X_LSB ... IMU_REG_GYRO_Z_MSB:
        offset = reg - IMU_REG_GYRO_X_LSB;
        raw    = (const uint8_t *)&d->gyro;
        return raw[offset];

    /* Euler angles: regs 0x1A–0x1F */
    case IMU_REG_EUL_X_LSB ... IMU_REG_EUL_Z_MSB:
        offset = reg - IMU_REG_EUL_X_LSB;
        raw    = (const uint8_t *)&d->euler_angles;
        return raw[offset];

    /* Linear acceleration: regs 0x28–0x2D */
    case IMU_REG_LIA_X_LSB ... IMU_REG_LIA_Z_MSB:
        offset = reg - IMU_REG_LIA_X_LSB;
        raw    = (const uint8_t *)&d->linear_acceleration;
        return raw[offset];

    default:
        return 0xFF;  /* unmapped register — I2C convention */
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public write API  (sensor emulation thread context only)
 * ══════════════════════════════════════════════════════════════════════════ */

void imu_emulator_update_data(const imu_data_t *new_data)
{
    /* ── Step 1: Write into the writer's exclusive buffer ─────────────── */
    /*
     * writer_idx is thread-local.  No other context touches it, so no
     * synchronisation is needed for this write.
     */
    memcpy(&imu_buf[writer_idx], new_data, sizeof(imu_data_t));

    /* ── Step 2: Data Memory Barrier ─────────────────────────────────── */
    /*
     * On Cortex-M4 (ARMv7-M) the memory model allows the CPU's store buffer
     * to reorder stores.  __DMB() ensures:
     *   a) The compiler does not move atomic_set above the memcpy.
     *   b) All stores from the memcpy have drained from the store buffer and
     *      are globally visible before the atomic_set executes.
     *
     * Without this, the ISR could observe an updated pending_idx while the
     * new buffer contents are still in the store buffer (i.e., not yet
     * visible to the ISR's load instructions).
     */
    __DMB();

    /* ── Step 3: Publish — atomically swap writer_idx into pending_idx ── */
    /*
     * atomic_set returns the PREVIOUS value of pending_idx.
     * After this line:
     *   pending_idx = writer_idx  (the fresh buffer is now "pending")
     *   writer_idx  = old pending (the stale buffer; safe to overwrite next)
     *
     * The multiset { writer_idx, pending_idx, reader_idx } remains a
     * permutation of {0,1,2} because we're performing a value swap.
     */
    writer_idx = (int)atomic_set(&pending_idx, (atomic_val_t)writer_idx);
}

/* ══════════════════════════════════════════════════════════════════════════
 * I2C Callbacks  (ISR context — absolutely no blocking)
 * ══════════════════════════════════════════════════════════════════════════ */

static int imu_write_requested_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    /* Master is beginning a write phase.  The next received byte will be
     * the register address.  Nothing to do here. */
    return 0;
}

static int imu_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    ARG_UNUSED(config);
    /*
     * Master wrote the register address it intends to read from.
     * Latch it for the subsequent read callbacks.
     */
    imu_current_reg = val;
    return 0;
}

static int imu_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);

    /* ── SNAPSHOT POINT — atomic buffer acquisition ─────────────────── */
    /*
     * Atomically exchange reader_idx with pending_idx.
     *
     * After this single-instruction exchange:
     *   reader_idx  = old pending_idx  (freshest complete buffer)
     *   pending_idx = old reader_idx   (stale; writer may claim it next)
     *
     * From this point until imu_stop_cb, ALL bytes are served from
     * imu_buf[reader_idx].  Even if the sensor thread calls
     * imu_emulator_update_data() multiple times before this transaction
     * ends, it will always target writer_idx, which is guaranteed to be
     * different from reader_idx by the triple-buffer invariant.
     *
     * atomic_set is a single LDREX/STREX — ISR-safe, no irq_lock needed.
     */
    reader_idx = (int)atomic_set(&pending_idx, (atomic_val_t)reader_idx);

    *val = serve_imu_byte(reader_idx, imu_current_reg);
    imu_current_reg++;
    return 0;
}

static int imu_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    /*
     * Master ACKed the previous byte and is clocking in the next.
     * Continue serving from the SAME snapshotted buffer (reader_idx).
     * Do NOT call atomic_set again — reader_idx is already fixed for
     * this transaction's lifetime.
     */
    *val = serve_imu_byte(reader_idx, imu_current_reg);
    imu_current_reg++;
    return 0;
}

static int imu_stop_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    /*
     * Transaction complete.  Reset the register pointer for the next
     * transaction.
     *
     * NOTE: reader_idx is intentionally NOT reset here.  It persists as
     * part of the triple-buffer invariant.  Resetting it to 0 would break
     * the { writer_idx, pending_idx, reader_idx } = { 0,1,2 } invariant
     * by potentially making two indices equal.  reader_idx is only changed
     * by the atomic_set exchange in imu_read_requested_cb.
     */
    imu_current_reg = 0x00;
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * I2C Target registration
 * ══════════════════════════════════════════════════════════════════════════ */

static struct i2c_target_callbacks imu_callbacks = {
    .write_requested = imu_write_requested_cb,
    .write_received  = imu_write_received_cb,
    .read_requested  = imu_read_requested_cb,
    .read_processed  = imu_read_processed_cb,
    .stop            = imu_stop_cb,
};

static struct i2c_target_config imu_config = {
    .address   = IMU_ADDRESS,
    .callbacks = &imu_callbacks,
};

/* ══════════════════════════════════════════════════════════════════════════
 * Public initialisation
 * ══════════════════════════════════════════════════════════════════════════ */

void imu_emulator_init(const struct device *i2c_dev)
{
    /*
     * Zero-fill all three buffers so the DUT reads plausible zeros before
     * the first physics packet arrives.
     */
    memset(imu_buf, 0, sizeof(imu_buf));

    /*
     * Establish initial triple-buffer ownership:
     *   imu_buf[0] → reader  (ISR will read zeros on first transaction)
     *   imu_buf[1] → writer  (sensor thread writes here first)
     *   imu_buf[2] → pending (zeros; first writer update will replace this)
     *
     * Multiset: {0, 1, 2} ✓
     */
    writer_idx  = 1;
    atomic_set(&pending_idx, 2);
    reader_idx  = 0;

    imu_current_reg = 0x00;

    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("IMU I2C device not ready");
        return;
    }
    if (i2c_target_register(i2c_dev, &imu_config) < 0) {
        LOG_ERR("Failed to register IMU I2C target at 0x%02X", IMU_ADDRESS);
        return;
    }
    LOG_INF("IMU I2C target registered at 0x%02X (triple-buffer)", IMU_ADDRESS);
}
