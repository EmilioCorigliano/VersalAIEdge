#!/usr/bin/env python3
"""Read temperature and scan OCM/TCM memory for pattern flips via XSDB.

Run this script from the Vitis Python shell:

    vitis -i
    exec(open('test_laser_xsdb.py').read())

The script plots temperature in real time and prints any OCM/TCM flips to the
terminal while correcting flipped words back to the expected pattern.
"""

import argparse
import datetime
import sys
import time
import os

import matplotlib
# Prefer an interactive GUI backend when available so plots appear in Vitis
for _backend in ("Qt5Agg", "QtAgg", "TkAgg", "GTK3Agg"):
    try:
        matplotlib.use(_backend, force=True)
        break
    except Exception:
        pass
import matplotlib.pyplot as plt
import xsdb

DEFAULT_TEMPERATURE_ADDRESS = 0x80010
DEFAULT_PATTERN = 0x55555555
DEFAULT_OCM_BASE = 0xfffc0000
DEFAULT_OCM_SIZE = 0x00040000
DEFAULT_ATCM_BASE = 0xFFE00000
DEFAULT_ATCM_SIZE = 0x00010000
DEFAULT_BTCM_BASE = 0xFFE20000
DEFAULT_BTCM_SIZE = 0x00010000
DEFAULT_CHUNK_WORDS = 64
DEFAULT_TARGET_FILTER = 'name =~ "Versal*"'


def open_xsdb(target_filter):
    session = xsdb.start_debug_session()
    session.connect()

    # Inspect the actual names first
    session.jtag_targets()
    session.targets()

    # Select the Versal device, adapting the pattern to the name printed above
    versal = session.targets(
        "--set",
        filter="name =~ *Versal*"
    )

    # Show the current target list and selected target
    session.targets(3)
    return session, versal


def read_temperature(target, address):
    # Prefer to read 32-bit then extract the correct byte (fallbacks included)
    temp_byte = target.mrd("-v", address=0x80010, size=1, word_size=4)

    return temp_byte


def read_words(target, address, size):
    if size <= 0:
        return []
    values = target.mrd("-v", address=address, size=size, word_size=4)
    if isinstance(values, int):
        return [values]
    return list(values)


def write_word(target, address, value):
    target.mwr(address=address, word_size=4, words=[value])


def scan_region(target, region_name, base, size, pattern, chunk_words):
    mismatches = []
    end_addr = base + size
    for addr in range(base, end_addr, chunk_words * 4):
        count = min(chunk_words, (end_addr - addr) // 4)
        if count <= 0:
            break
        values = read_words(target, addr, count)
        for offset, actual in enumerate(values):
            expected = pattern
            if actual != expected:
                mismatch_addr = addr + offset * 4
                mismatches.append({
                    'address': mismatch_addr,
                    'actual': actual,
                    'expected': expected,
                    'mask': actual ^ expected,
                    'region': region_name,
                })
    return mismatches


def correct_flips(target, mismatches, pattern):
    for mismatch in mismatches:
        write_word(target, mismatch['address'], pattern)


def format_mismatch(mismatch):
    return (
        f"[{mismatch['region']}] Flip at 0x{mismatch['address']:08X}: "
        f"actual=0x{mismatch['actual']:08X}, expected=0x{mismatch['expected']:08X}, "
        f"mask=0x{mismatch['mask']:08X}"
    )


def update_plot(ax, line, times, temps):
    line.set_data(times, temps)
    ax.relim()
    ax.autoscale_view()
    if times:
        ax.set_xlim(0, max(10.0, times[-1] + 1.0))
    if temps:
        min_y = min(temps)
        max_y = max(temps)
        if min_y == max_y:
            ax.set_ylim(min_y - 1.0, max_y + 1.0)
        else:
            ax.set_ylim(min_y - 1.0, max_y + 1.0)


def run_monitor(args):
    session, target = open_xsdb(args.target_filter)

    start_time = time.time()
    times = []
    temps = []

    try:
        while True:
            temp_value = None
            try:
                temp_value = read_temperature(session, args.temp_addr)
                elapsed = time.time() - start_time
                times.append(elapsed)
                temps.append(temp_value)
            except Exception as exc:
                print(f'[{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Temperature read failed: {exc}', file=sys.stderr)

            mismatches = []
            mismatches.extend(scan_region(session, 'OCM', args.ocm_base, args.ocm_size, args.pattern, args.chunk_words))
            mismatches.extend(scan_region(session, 'ATCM', args.atcm_base, args.atcm_size, args.pattern, args.chunk_words))
            mismatches.extend(scan_region(session, 'BTCM', args.btcm_base, args.btcm_size, args.pattern, args.chunk_words))
            
            timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            if mismatches:
                print(f'[{timestamp}] {temp_value/1000.0:.2f}°C Detected {len(mismatches)} flipped word(s):')
                for mismatch in mismatches:
                    print('  ' + format_mismatch(mismatch))
                os.system("paplay /usr/share/sounds/freedesktop/stereo/complete.oga")
                correct_flips(session, mismatches, args.pattern)
            else:
                print(f'[{timestamp}] {temp_value/1000.0:.2f}°C No flips detected.')

            if args.log_file:
                with open(args.log_file, 'a') as logf:
                    logf.write(f'[{timestamp}] temp=')
                    logf.write(f'0x{temp_value:08X}' if temp_value is not None else 'N/A')
                    logf.write(f' total_flips={len(mismatches)}\n')
                    for mismatch in mismatches:
                        logf.write(format_mismatch(mismatch) + '\n')

    except KeyboardInterrupt:
        print('Stopping monitor.')
    finally:
        xsdb.dispose()


def main():
    parser = argparse.ArgumentParser(description='Monitor Versal temperature and scan OCM/TCM for pattern flips using XSDB API.')
    parser.add_argument('--target-filter', default=DEFAULT_TARGET_FILTER, help='XSDB target filter expression')
    parser.add_argument('--temp-addr', type=lambda value: int(value, 0), default=DEFAULT_TEMPERATURE_ADDRESS, help='Temperature register address')
    parser.add_argument('--pattern', type=lambda value: int(value, 0), default=DEFAULT_PATTERN, help='Expected 32-bit memory pattern')
    parser.add_argument('--ocm-base', type=lambda value: int(value, 0), default=DEFAULT_OCM_BASE, help='OCM start address')
    parser.add_argument('--ocm-size', type=lambda value: int(value, 0), default=DEFAULT_OCM_SIZE, help='OCM size in bytes')
    parser.add_argument('--atcm-base', type=lambda value: int(value, 0), default=DEFAULT_ATCM_BASE, help='ATCM start address')
    parser.add_argument('--atcm-size', type=lambda value: int(value, 0), default=DEFAULT_ATCM_SIZE, help='ATCM size in bytes')
    parser.add_argument('--btcm-base', type=lambda value: int(value, 0), default=DEFAULT_BTCM_BASE, help='BTCM start address')
    parser.add_argument('--btcm-size', type=lambda value: int(value, 0), default=DEFAULT_BTCM_SIZE, help='BTCM size in bytes')
    parser.add_argument('--chunk-words', type=int, default=DEFAULT_CHUNK_WORDS, help='Number of 32-bit words to read per mrd chunk')
    parser.add_argument('--log-file', default=None, help='Optional log file path')
    args = parser.parse_args()

    if args.ocm_size % 4 != 0 or args.atcm_size % 4 != 0 or args.btcm_size % 4 != 0:
        parser.error('OCM, ATCM, and BTCM sizes must be a multiple of 4 bytes')

    run_monitor(args)


if __name__ == '__main__':
    main()
