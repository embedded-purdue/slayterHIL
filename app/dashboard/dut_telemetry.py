"""Read the DUT (flight controller) console over USB serial and parse its telemetry.

The DUT prints one telemetry line about every 0.5 s (flight_controller/app/src/main.c):

    [<time>] P: 1.23 R:-0.50 Y: 90.00 | sp P: 0.00 R: 0.00 | alt:1000mm sp:1000 | rx:3 | mix FL:120 FR:118 BL:121 BR:119

Altitude gets "(off)" appended after the setpoint when LiDAR data isn't valid.
Every other line (boot messages, errors) is passed through as plain text.

Run standalone to check the parser against a real board:
    python dut_telemetry.py COM5          # Windows
    python dut_telemetry.py /dev/ttyUSB0  # Linux / Pi
    python dut_telemetry.py fake          # replay sample lines, no hardware
"""

import asyncio
import re
import sys
import threading
import time

_NUM = r'([ -])(\d+)\.(\d{2})'  # matches fx2() output: sign char (space or '-'), whole, 2-digit frac

TELEMETRY_RE = re.compile(
    r'\[(?P<dut_time>[^\]]*)\]\s*'
    rf'P:{_NUM}\s+R:{_NUM}\s+Y:{_NUM}\s*\|\s*'
    rf'sp P:{_NUM}\s+R:{_NUM}\s*\|\s*'
    r'alt:(?P<alt>\d+)mm sp:(?P<alt_sp>-?\d+)(?P<alt_off>\(off\))?\s*\|\s*'
    r'rx:(?P<rx>\d+)\s*\|\s*'
    r'mix FL:(?P<fl>-?\d+) FR:(?P<fr>-?\d+) BL:(?P<bl>-?\d+) BR:(?P<br>-?\d+)'
)

FAKE_LINES = [
    '*** Booting Zephyr OS ***',
    'BNO consumer thread started',
    '[00:00:01.000] P: 0.00 R: 0.00 Y: 0.00 | sp P: 0.00 R: 0.00 | alt:0mm sp:0(off) | rx:0 | mix FL:0 FR:0 BL:0 BR:0',
    '[00:00:01.500] P: 2.50 R:-1.25 Y: 10.00 | sp P: 0.00 R: 0.00 | alt:950mm sp:1000 | rx:1 | mix FL:130 FR:110 BL:128 BR:112',
    '[00:00:02.000] P: 1.10 R:-0.40 Y: 10.20 | sp P: 0.00 R: 0.00 | alt:990mm sp:1000 | rx:1 | mix FL:122 FR:118 BL:121 BR:119',
    'Failed to get LiDAR data: -5',
]


def _fx(m, i):
    """Rebuild a float from the (sign, whole, frac) groups starting at group index i."""
    sign, whole, frac = m.group(i), m.group(i + 1), m.group(i + 2)
    value = int(whole) + int(frac) / 100.0
    return -value if sign == '-' else value


def parse_line(line):
    """Return a telemetry dict for a telemetry line, or None for any other line."""
    m = TELEMETRY_RE.search(line)
    if not m:
        return None
    # Numeric groups: 1 is dut_time (named), then 5 fx2 values x 3 groups = groups 2..16.
    return {
        'dut_time': m.group('dut_time'),
        'pitch_deg': _fx(m, 2),
        'roll_deg': _fx(m, 5),
        'yaw_deg': _fx(m, 8),
        'pitch_sp_deg': _fx(m, 11),
        'roll_sp_deg': _fx(m, 14),
        'altitude_mm': int(m.group('alt')),
        'altitude_sp_mm': int(m.group('alt_sp')),
        'altitude_valid': m.group('alt_off') is None,
        'rc_rx_count': int(m.group('rx')),
        'motors': {
            'FL': int(m.group('fl')),
            'FR': int(m.group('fr')),
            'BL': int(m.group('bl')),
            'BR': int(m.group('br')),
        },
    }


def _reader_thread(port, baud, loop, queue, stop):
    """Blocking serial reads run here so they never stall the aiohttp event loop."""
    def push(item):
        loop.call_soon_threadsafe(queue.put_nowait, item)

    if port == 'fake':
        while not stop.is_set():
            for line in FAKE_LINES:
                if stop.is_set():
                    return
                push(('line', line))
                time.sleep(0.5)
        return

    import serial  # pyserial; imported here so 'fake' mode works without it

    while not stop.is_set():
        try:
            with serial.Serial(port, baud, timeout=1) as ser:
                push(('status', f'connected to {port} @ {baud}'))
                while not stop.is_set():
                    raw = ser.readline()
                    if raw:
                        push(('line', raw.decode('utf-8', errors='replace').rstrip('\r\n')))
        except Exception as e:  # unplugged, wrong port, busy (another serial monitor open), ...
            push(('status', f'serial error on {port}: {e} (retrying in 2 s)'))
            stop.wait(2)


async def read_dut(port, baud=115200):
    """Async generator of ('telemetry', dict) / ('line', str) / ('status', str) events.

    Reconnects on its own if the board is unplugged or the port is busy.
    """
    loop = asyncio.get_running_loop()
    queue = asyncio.Queue()
    stop = threading.Event()
    t = threading.Thread(target=_reader_thread, args=(port, baud, loop, queue, stop), daemon=True)
    t.start()
    try:
        while True:
            kind, payload = await queue.get()
            if kind == 'line':
                parsed = parse_line(payload)
                if parsed is not None:
                    yield 'telemetry', parsed
                    continue
            yield kind, payload
    finally:
        stop.set()


async def _main(port):
    async for kind, payload in read_dut(port):
        print(f'{kind:9} {payload}')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    try:
        asyncio.run(_main(sys.argv[1]))
    except KeyboardInterrupt:
        pass
