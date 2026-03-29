# HIL Nucleo — Architecture Analysis

## Old vs New: Defect Inventory

Before analysing the new design, here is a precise accounting of every defect
found in the old codebase and why each one was fatal.

| # | File | Defect | Severity |
|---|------|--------|----------|
| 1 | `imu_emulator.c` | `k_mutex_lock(K_FOREVER)` called inside `imu_read_cb`, which fires in ISR context. Zephyr will kernel-panic immediately. | **Fatal** |
| 2 | `imu_emulator.c` | `read_requested` and `read_processed` both mapped to the same `imu_read_cb`. `imu_read_cb` increments `current_imu_read_reg` on every call. `read_requested` fires *before* the first byte is clocked out, so the register pointer is already +1 by the time the DUT receives the first byte. All data is offset by one register. | **Fatal** |
| 3 | `sensor_emulation.h` | `typedef struct DeviceUpdatePacket { uint32 timestamp = 1; ... }` — this is proto3 syntax embedded in a C header. It will not compile. | **Fatal** |
| 4 | `scheduler.c` | Timer ISR calls `k_msgq_put(&sensor_update_q, K_NO_WAIT)` without purging first. `sensor_update_q` has 10 slots; with a fast Pi it fills immediately. Subsequent puts silently fail, so the DUT is served data that is potentially 10 packets stale. | **Critical** |
| 5 | `scheduler.c` | `if (k_timer_status_get(&injection_timer) == 0)` is used to decide whether to start the timer. `k_timer_status_get()` returns the *expiry count since last called*, not a running/stopped boolean. If the timer fired once and is still running, the count is 1 — the condition is false and the timer is NOT re-armed. The scheduler never triggers again after the first packet. | **Critical** |
| 6 | `sensor_emulation.c` | `LOG_INF("Sensor hello world")` inside the `while(1)` polling loop. Even at LEVEL_INF this floods the log backend and destroys scheduling latency. | **High** |
| 7 | `sensor_emulation.c` | `k_poll_event` `state` field is compared against `K_POLL_TYPE_MSGQ_DATA_AVAILABLE` (the *type* enum) instead of `K_POLL_STATE_MSGQ_DATA_AVAILABLE` (the *state* enum). These have different integer values. The branch is never taken. | **High** |
| 8 | Old scheduler | No SPI double-buffering. SPI is re-armed only after the full protobuf decode + queue push cycle. During this dead-band any transmission from the Pi is silently lost. | **High** |
| 9 | `lidar_emulator.c` | `transaction_distance_cache` is refreshed on `REG_FULL_DELAY_LOW` (0x10), which is the SECOND byte in the sequence. The HIGH byte (0x0F) is served from the live atomic — meaning a background update can still corrupt the MSB before the snapshot is taken. The snapshot must happen on the HIGH byte. | **Medium** |
| 10 | `sensor_emulation.h` | `_Static_assert(sizeof(imu_data_t) == 18, "...size of 26 bytes")` — the assert value (18) and the message string ("26 bytes") disagree. The correct size is 18. | **Low** |

---

## New Architecture: Strengths

### S1 — ISR Path Is Completely Lock-Free
Neither `imu_read_requested_cb`, `imu_read_processed_cb`, `lidar_read_requested_cb`,
nor any other I2C callback touches a mutex, semaphore, or blocking call.
The old architecture's `k_mutex_lock(K_FOREVER)` from ISR was the single most
dangerous defect; that class of bug is now architecturally impossible.

### S2 — Overwrite Semantics Prevent Queue Bloat
`sensor_update_q` has exactly 1 slot.  The timer ISR's `irq_lock/purge/put`
sequence means the slot always holds the *most recent* causally-valid packet,
not the oldest.  The Pi can transmit at 10× the DUT's poll rate without any
risk of queue saturation.

### S3 — The Arrow of Time Is Enforced in Hardware
`injection_timer_isr` fires only when `k_timer` (backed by the STM32 hardware
timer) reaches the packet's `timestamp_us`.  Until then, the DUT reads the
previously-injected state, not a future one.  This is a genuine temporal
guarantee, not a software approximation.

### S4 — SPI Double-Buffer Closes the Re-Arm Gap
The second `spi_read_async` is called immediately after the buffer swap, before
protobuf decode begins.  The re-arm gap shrinks from ~decode time (≥10 µs) to
~pointer swap time (~5 CPU cycles, ~30 ns at 168 MHz).

### S5 — Ping-Pong Eliminates IMU Struct Tearing
Without the ping-pong, an 18-byte `memcpy` from the sensor thread and a byte-by-
byte ISR read overlap with no protection.  The old code used a mutex in the ISR
to solve this, which caused kernel panics.  The ping-pong gives the same
guarantee with zero ISR overhead.

### S6 — LiDAR Snapshot Is Now on the Correct Register
The old code snapshotted the 16-bit distance when the DUT read the LOW byte
(0x10), meaning the HIGH byte (0x0F) was served live.  An update between the
two reads would corrupt the MSB.  The new code snapshots on the HIGH byte —
the first byte in the read sequence — so both bytes are always coherent.

### S7 — `k_timer_status_get` Removed
The ambiguous `k_timer_status_get() == 0` guard is replaced by a `timer_armed`
atomic flag with a proper `atomic_cas` (compare-and-swap) semantic.
The CAS guarantees that only one call site starts the timer, even under
concurrent access from the scheduler thread and the timer ISR.

---

## New Architecture: Weaknesses and Edge Cases

---

### W1 — `k_timer` Resolution vs Microsecond Timestamps  [**HIGH RISK**]

**Problem:**  `k_timer_start(K_USEC(delay_us))` requests microsecond-accuracy
scheduling, but Zephyr's `k_timer` is driven by the system tick.

If `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000` (the default for many STM32
configurations), the minimum timer resolution is **1 ms**.  A request for
`K_USEC(300)` gets rounded to 1 ms — a 3× overshoot.

**Impact:**  The "Arrow of Time" guarantee is degraded from microsecond
precision to ±0.5 tick.  For a 1 kHz tick, you are serving data up to 0.5 ms
later than its timestamp demands.  Whether this matters depends on your
simulation step size and the flight controller's control loop frequency.

**Fix options (choose one):**

| Option | Effort | Precision |
|--------|--------|-----------|
| `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000000` | Low (Kconfig only) | 1 µs (if hardware timer supports it) |
| Use a hardware timer directly (`TIMER6`, `TIMER7`) via Zephyr's `counter` driver API | Medium | Sub-µs |
| Use `k_busy_wait()` for sub-tick residual after `k_timer` fires | Medium | ~1 µs at the cost of CPU burn |

The `counter` API approach is recommended.  Configure `injection_timer_isr` as
the `counter_alarm_callback_t`, set the alarm in cycles directly from
`k_cycle_get_32()`, and bypass `k_timer` entirely.

---

### W2 — Ping-Pong Double-Write Race Condition  [**MEDIUM RISK**]

**Problem:**  The ping-pong buffer is safe against a *single* writer update
during an ISR transaction.  It is NOT safe against two writer updates.

Step-by-step failure scenario:

```
[initial state: active_idx = 0, ISR is mid-transaction reading buf[0]]

  Sensor thread wakes (500 µs sleep elapsed):
    inactive = 1
    memcpy to buf[1]
    atomic_set(active_idx, 1)     → now active=1, inactive=0

  Sensor thread wakes again (another 500 µs):
    inactive = 0                   ← buf[0], which the ISR is reading!
    memcpy to buf[0]               ← WRITES INTO THE BUFFER THE ISR IS IN
    atomic_set(active_idx, 0)
```

At 400 kHz I2C and 18 bytes, a transaction takes ≈405 µs.  The sensor thread
sleeps 500 µs.  These two timers are close enough that this race can occur,
especially under scheduling jitter.

**Impact:**  The DUT reads an 18-byte struct where some bytes are from state N
and some are from state N+2.  For a flight controller processing gyro + euler +
linear accel, this is an inconsistent physics snapshot.

**Fix options:**

**Option A — Triple Buffer (recommended for this use case)**

Add a third `imu_data_t` buffer.  Maintain three indices: `reader`, `writer`,
`pending`.  The writer always writes to `writer`.  When done, it swaps `writer`
and `pending` (atomic).  When the ISR starts a transaction, it swaps `reader`
and `pending` (atomic).  The reader buffer is never the writer buffer.

This is the canonical lock-free triple buffer pattern.

**Option B — Seqlock**

```c
static volatile uint32_t imu_seqnum = 0;

// Writer:
imu_seqnum++;          // odd = writing in progress
__DMB();
memcpy(&imu_buf[inactive], ...);
__DMB();
atomic_set(&active_idx, inactive);
imu_seqnum++;          // even = complete

// ISR read_requested_cb:
imu_snap_seqnum = imu_seqnum;
imu_snap_buf_idx = atomic_get(&active_idx);

// ISR stop_cb — detect torn read:
if (imu_seqnum != imu_snap_seqnum || (imu_seqnum & 1)) {
    // transaction was torn — serve zeros or re-read
}
```

The seqlock is lighter than a triple buffer but requires detecting the tear
*after* the transaction, at which point recovery means serving zeros for that
sample.

**Option C — Accept it (for initial bring-up)**

An occasional 18-byte tear between two consecutive physics states is unlikely
to cause a flight controller to crash.  Mark this as a known limitation,
instrument it with a counter, and revisit if the DUT exhibits instability.

---

### W3 — `irq_lock` Latency on I2C  [**MEDIUM RISK**]

**Problem:**  `irq_lock()` on Cortex-M sets `PRIMASK=1`, masking ALL
configurable interrupts, including the I2C target ISR.  During the
`purge + put` sequence, any I2C read request from the DUT is deferred.

**Duration:**  On a 168 MHz STM32F4, `k_msgq_purge` + `k_msgq_put` on a
1-slot queue takes approximately 50–150 CPU cycles = 0.3–0.9 µs.

**Impact:**  At 400 kHz I2C, one bit period is 2.5 µs.  A 0.9 µs delay does
NOT violate the I2C timing spec (the hold time on the clock stretching feature
would cover this on most controllers).  However, if your I2C bus runs at 1 MHz
(Fast-Mode Plus), this margin becomes tighter.

**Fix:**  Replace `irq_lock/irq_unlock` with a Zephyr spinlock
(`k_spinlock_key_t`) if your I2C ISR has a fixed NVIC priority above the timer
ISR.  A spinlock masks only at or below the spinlock's configured priority
level — this is achievable via `CONFIG_ZERO_LATENCY_IRQS` for the I2C ISR.

Alternatively, since only the timer ISR writes to `sensor_update_q` and the
sensor thread only reads (peek), the `irq_lock` is defensively necessary only
if you ever plan to add a second ISR writing to that queue.  Removing it is
safe in the current single-writer design — but document this assumption.

---

### W4 — Clock Epoch Mismatch  [**HIGH RISK — Operational**]

**Problem:**  The timer ISR computes:

```c
int32_t delay_us = (int32_t)(packet.timestamp_us - now_us());
```

`now_us()` uses `k_cyc_to_us_near32(k_cycle_get_32())`, which counts from
STM32 power-on.  `packet.timestamp_us` comes from the Pi.  If the Pi uses
`time.monotonic()` from its own boot epoch, these two clocks are completely
unrelated and `delay_us` is garbage.

**Consequence:**  Every packet will be served either immediately (if Pi ts > STM32 ts)
or never (if Pi ts < STM32 ts, delay wraps to a huge uint32, timer won't fire
for ~4295 seconds).

**Fix:**  Implement a startup clock synchronisation handshake over SPI before
entering the main streaming loop.  One simple approach:

```
STM32 sends:  { magic=0xC0FFEE, stm32_now_us=k_cyc_to_us_near32(...) }
Pi receives, records delta = (pi_now_us - stm32_now_us) as pi_epoch_offset
Pi sends all subsequent timestamp_us as:  (pi_simulation_time_us - pi_epoch_offset)
```

This makes `packet.timestamp_us` directly comparable to the STM32's clock.

---

### W5 — SPI Framing: No Message Boundary Protocol  [**HIGH RISK**]

**Problem:**  The SPI is configured for raw 8-bit streaming with a fixed-size
256-byte buffer.  Protobuf has no built-in message boundaries.  The STM32 does
not know where one message ends and the next begins.

**Consequences:**

- If the Pi sends a protobuf message smaller than 256 bytes, the STM32 will
  try to decode 256 bytes of garbage-padded data.  `pb_decode` will attempt to
  parse the padding bytes as protobuf fields.
- If the Pi sends a message larger than 256 bytes, it is silently truncated.
- If the DMA fires at an arbitrary boundary (mid-message), the decode will fail.

**Fix:**  Prepend a 2-byte little-endian length field to every frame:

```
[LEN_LOW][LEN_HIGH][protobuf payload ... ]
```

In the scheduler thread, after `k_sem_take`, read `decode_buf[0:1]` to get the
actual payload length, then call `pb_decode` on exactly that many bytes.  Set
`SPI_BUF_SIZE = MAX_PROTO_PAYLOAD + 2`.

Also ensure the Pi CS assertion spans exactly one framed message so the
STM32's DMA RXNE interrupt aligns to message boundaries.

---

### W6 — `uint32_t` Timestamp Wraparound at 71 Minutes  [**LOW RISK**]

**Problem:**  `k_cyc_to_us_near32` returns a `uint32_t`.  On a 168 MHz
STM32F4, `k_cycle_get_32()` wraps every 25.5 seconds.  After conversion to
µs via division, the µs value also wraps.  At 1 µs/tick it wraps at
~4295 seconds ≈ 71 minutes.

**Why it's "LOW RISK":**  The delay calculation uses signed subtraction:
```c
int32_t delay_us = (int32_t)(packet.timestamp_us - now_us());
```
Signed integer wraparound arithmetic is well-defined in C for two's-complement
machines (all Cortex-M targets).  If `packet.timestamp_us` wraps past 0 and
`now_us()` hasn't wrapped yet, the subtraction produces a correct negative
number, which is clamped to 1.  The timer fires immediately — the packet is
served slightly early rather than being withheld indefinitely.

**However:**  This relies on the Pi also using a `uint32_t` microsecond clock
that wraps at the same epoch.  If the Pi uses a 64-bit µs timestamp and only
sends the low 32 bits, the wrap arithmetic holds.  Document this as a
constraint on the Pi-side protocol.

---

### W7 — `k_timer` Expiry Context and `k_msgq` Interactions  [**LOW RISK**]

**Zephyr detail:**  By default, `k_timer` expiry functions execute in the
context of the system clock ISR (the `SysTick` handler on Cortex-M).  This
means:

- `k_msgq_get(K_NO_WAIT)` — **allowed from ISR** ✓
- `k_msgq_purge()` — **allowed from ISR** ✓
- `k_msgq_put(K_NO_WAIT)` — **allowed from ISR** ✓
- `k_msgq_peek()` — **allowed from ISR** ✓
- `k_timer_start()` from within a timer callback — **allowed** ✓

All operations used in `injection_timer_isr` are on the Zephyr-approved ISR
API list.  The timer ISR is safe as written.

**Watch for:**  If you enable `CONFIG_TIMESLICING`, the kernel timer thread may
process timer expiries in a thread context rather than the SysTick ISR.  In
this mode the API constraints relax (mutexes become safe), but the timing
guarantees loosen.  Keep `CONFIG_TIMESLICING=n` for tight HIL timing.

---

### W8 — NVIC Priority Configuration  [**HIGH RISK — Integration**]

The new architecture implicitly requires this NVIC priority ordering
(lower number = higher priority on Cortex-M):

```
Priority 0 (highest)
  ├── I2C target ISR     ← must be highest; DUT timing is hard real-time
  ├── SPI DMA ISR        ← must preempt any thread; re-arm gap is critical
  ├── SysTick (k_timer)  ← injection_timer_isr lives here
  └── (other peripherals)
Priority N (lowest)
  ├── Zephyr kernel scheduler interrupt
  ├── scheduler_thread   (priority 5)
  └── sensor_emulation_thread (priority 7)
```

**The danger:**  If `I2C_TARGET_IRQ_PRIORITY` is set equal to or lower than
`SYSTICK_PRIORITY` in your STM32 pinmux/DTS, the `irq_lock` inside
`injection_timer_isr` will mask the I2C ISR for 0.3–0.9 µs — which is
acceptable — but if the I2C ISR has *lower* priority than SysTick, then a
`k_timer` that fires during an ongoing I2C transaction will preempt the I2C
ISR itself, causing the I2C peripheral to see a mid-stream clock stretch or
stall.

**Fix:**  In your STM32 DTS overlay, explicitly set:

```dts
&i2c1 {
    interrupts = <31 1>;   /* IRQ 31, priority 1 */
};
&dma1 {
    interrupts = <11 2>, <12 2>, ...;  /* SPI DMA, priority 2 */
};
/* SysTick is fixed at priority 0 by Zephyr via CONFIG_CORTEX_M_SYSTICK */
```

Or use `CONFIG_ZERO_LATENCY_IRQS` for the I2C IRQ so it is exempt from
`irq_lock` masking entirely.

---

### W9 — Sensor Thread `k_sleep` Duration Coupling  [**LOW RISK**]

The sensor thread's `k_sleep(K_USEC(500))` determines how quickly a new
timer-injected packet propagates to the hardware emulators.  This creates a
coupling between the sleep value and your simulation step rate:

- If your physics step is 1 ms and you sleep 500 µs, you update the emulators
  ~2× per step.  Fine — idempotent.
- If your physics step is 100 µs (10 kHz simulation), a 500 µs sleep means
  you miss 4 out of 5 updates.  The DUT reads data that is up to 500 µs stale
  relative to what the timer ISR last injected.

**Fix:**  Set `k_sleep(K_USEC(X))` to ≤ half your minimum physics step period.
Better: replace the sleep with `k_msgq_get` blocking *with a timeout* equal to
your minimum step period.  Change the architecture to *pop* (not peek) from a
queue, and have the timer ISR use a `k_msgq_put` directly as the event trigger.
This eliminates the sleep-based polling entirely but requires reconsidering the
purge semantics.

---

### W10 — nanopb Stack Depth  [**LOW RISK**]

`pb_decode` with complex nested messages can use 200–600 bytes of stack
depending on field count and callback usage.  The scheduler thread is allocated
8 KB of stack — sufficient.  However, if you add `PB_DECODE_NOINIT` fields or
callbacks, verify with `CONFIG_THREAD_ANALYZER=y` or Zephyr's built-in stack
sentinel (`CONFIG_STACK_SENTINEL=y`).

---

## Recommendations Summary

| Priority | Action |
|----------|--------|
| **P0** | Add a SPI framing layer (2-byte length prefix). Without this, pb_decode is unreliable. |
| **P0** | Implement the clock-epoch synchronisation handshake at startup. Without this, temporal gating does nothing. |
| **P1** | Replace `k_timer` with the Zephyr `counter` driver (hardware timer) for sub-100 µs injection accuracy. |
| **P1** | Explicitly set NVIC priorities in DTS: I2C target < SPI DMA < SysTick numerically. |
| **P2** | Migrate the IMU ping-pong to a triple buffer to eliminate the double-write race (W2). |
| **P2** | Set `k_sleep` ≤ half the minimum physics step period, or switch to event-driven blocking. |
| **P3** | Enable `CONFIG_THREAD_ANALYZER` and `CONFIG_STACK_SENTINEL` during bring-up to catch stack overflows early. |
| **P3** | Add `LOG_WRN` counters for scheduler_q evictions and pb_decode failures; expose via Zephyr shell for live diagnostics. |

---

## What the New Architecture Cannot Fix Alone

1. **DUT timing slack.**  The ESP32's I2C master firmware has its own polling
   interval.  If it polls IMU at 100 Hz but the physics engine runs at 500 Hz,
   you will always serve 5-packet-old data at the polling moment.  The only fix
   is for the DUT to poll faster, or for the STM32 to assert a GPIO "data ready"
   interrupt to the ESP32 when a new packet is injected.

2. **Pi-side jitter.**  If the Linux orchestrator has scheduling jitter (which it
   does — Linux is not an RTOS), the `timestamp_us` field may arrive out-of-order
   or with variable inter-packet spacing.  The staging ring buffer's FIFO ordering
   assumption breaks if packets arrive out of timestamp order.  Add a sort/reorder
   buffer on the Pi side, or discard out-of-order packets on the STM32 side by
   comparing the incoming timestamp to the last-injected timestamp.

3. **SPI CS alignment.**  If the Raspberry Pi's `spidev` driver deasserts CS
   between DMA bursts, and the STM32 interprets each CS assertion as a new
   frame, you must ensure the framing layer boundaries coincide with CS
   boundaries.  A misaligned CS pulse will cause `pb_decode` to fail on the
   next frame.
