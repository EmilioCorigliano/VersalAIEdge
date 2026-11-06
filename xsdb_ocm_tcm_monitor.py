#!/usr/bin/env python3
"""XSDB monitor for Versal OCM/TCM pattern scanning and temperature logging."""

import argparse
import datetime
import os
import select
import subprocess
import sys
import time
import re

DEFAULT_TEMPERATURE_ADDRESS = 0x80010
DEFAULT_PATTERN = 0x55555555
DEFAULT_OCM_BASE = 0xFFE00000
DEFAULT_OCM_SIZE = 0x00010000
DEFAULT_TCM_BASE = 0xFFE90000
DEFAULT_TCM_SIZE = 0x00010000
DEFAULT_CHUNK_WORDS = 32
DEFAULT_POLL_INTERVAL = 5.0
DEFAULT_XSDB = "xsdb"
DEFAULT_TARGET_FILTER = 'name =~ "Versal*"'

PROMPT_RE = re.compile(r'^xsdb%\s*$')
HEX_RE = re.compile(r'^[0-9A-Fa-f]+$')


class XsdbCli:
    def __init__(self, command=DEFAULT_XSDB, target_filter=DEFAULT_TARGET_FILTER, verbose=False):
        self.command = command
        self.target_filter = target_filter
        self.verbose = verbose
        self.proc = None

    def open(self):
        self.proc = subprocess.Popen(
            [self.command],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True,
        )
        self._drain_prompt()
        self._run_command('connect', quiet=True)
        self._run_command(f'targets -set -filter {{{self.target_filter}}}', quiet=True)

    def close(self):
        if self.proc and self.proc.stdin:
            try:
                self._run_command('quit', quiet=True)
            except Exception:
                pass
            self.proc.stdin.close()
        if self.proc:
            self.proc.wait(timeout=2)
            self.proc = None

    def _drain_prompt(self):
        if not self.proc or not self.proc.stdout:
            return
        self._read_until_prompt(timeout=2.0)

    def _read_until_prompt(self, timeout=5.0):
        if not self.proc or not self.proc.stdout:
            return []

        output_lines = []
        end_time = time.monotonic() + timeout
        while time.monotonic() < end_time:
            rlist, _, _ = select.select([self.proc.stdout], [], [], 0.2)
            if self.proc.stdout in rlist:
                line = self.proc.stdout.readline()
                if line == '':
                    break
                stripped = line.rstrip('\r\n')
                output_lines.append(stripped)
                if PROMPT_RE.match(stripped):
                    break
            else:
                continue
        return output_lines

    def _run_command(self, cmd, quiet=False, timeout=5.0):
        if not self.proc or not self.proc.stdin:
            raise RuntimeError('XSDB process is not running')
        if self.verbose and not quiet:
            print(f'[XSDB] CMD: {cmd}')
        self.proc.stdin.write(cmd + '\n')
        self.proc.stdin.flush()
        lines = self._read_until_prompt(timeout=timeout)
        if self.verbose and not quiet:
            print('\n'.join(lines))
        return lines

    def read_memory(self, addr, count):
        cmd = f'mrd 0x{addr:08X} {count}'
        lines = self._run_command(cmd)
        return parse_mrd_output(lines)

    def write_memory(self, addr, value, count=1):
        cmd = f'mwr 0x{addr:08X} 0x{value:08X} {count}'
        self._run_command(cmd)

    def read_register(self, addr):
        lines = self._run_command(f'mrd 0x{addr:08X} 1')
        parsed = parse_mrd_output(lines)
        return parsed[0][1] if parsed else None


def parse_mrd_output(lines):
    parsed = []
    for line in lines:
        text = line.strip()
        if not text or text.startswith('xsdb%'):
            continue
        tokens = text.split()
        if not tokens:
            continue
        first = tokens[0].rstrip(':')
        if not first.startswith('0x'):
            continue
        try:
            addr = int(first, 16)
        except ValueError:
            continue
        for token in tokens[1:]:
            tok = token.rstrip(':')
            if tok.startswith('0x'):
                tok = tok[2:]
            if not HEX_RE.fullmatch(tok):
                continue
            value = int(tok, 16)
            parsed.append((addr, value))
            addr += 4
    return parsed


def scan_region(xsdb, name, base, size, pattern, chunk_words):
    mismatches = []
    if size == 0:
        return mismatches
    end_addr = base + size
    for chunk_start in range(base, end_addr, chunk_words * 4):
        count = min(chunk_words, (end_addr - chunk_start) // 4)
        values = xsdb.read_memory(chunk_start, count)
        for addr, actual in values:
            if actual != pattern:
                mismatch = {
                    'address': addr,
                    'actual': actual,
                    'expected': pattern,
                    'mask': actual ^ pattern,
                }
                mismatches.append(mismatch)
    return mismatches


def correct_flips(xsdb, mismatches, pattern):
    for mismatch in mismatches:
        addr = mismatch['address']
        xsdb.write_memory(addr, pattern, 1)


def human_hex(value, width=8):
    return f'0x{value:0{width}X}'


def parse_address_size(text):
    try:
        base_str, size_str = text.split(':', 1)
        base = int(base_str, 0)
        size = int(size_str, 0)
        return base, size
    except ValueError:
        raise argparse.ArgumentTypeError(
            'region must be BASE:SIZE with hex or decimal values, e.g. 0xFFE00000:0x10000'
        )


def format_mismatch(region_name, mismatch):
    return (
        f'[{region_name}] Flip at 0x{mismatch["address"]:08X}: '
        f'actual={human_hex(mismatch["actual"])}, '
        f'expected={human_hex(mismatch["expected"])}, '
        f'mask={human_hex(mismatch["mask"])})'
    )


def run_monitor(args):
    xsdb = XsdbCli(command=args.xsdb, target_filter=args.target_filter, verbose=args.verbose)
    xsdb.open()

    try:
        iteration = 1
        while True:
            ts = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            temp_value = xsdb.read_register(args.temp_addr)
            if temp_value is None:
                print(f'[{ts}] Temperature read failed at 0x{args.temp_addr:08X}')
            else:
                print(f'[{ts}] Temperature register 0x{args.temp_addr:08X} = {human_hex(temp_value)} ({temp_value})')

            total_mismatches = 0
            all_mismatches = []
            for region_name, (base, size) in (
                ('OCM', (args.ocm_base, args.ocm_size)),
                ('TCM', (args.tcm_base, args.tcm_size)),
            ):
                mismatches = scan_region(xsdb, region_name, base, size, args.pattern, args.chunk_words)
                if mismatches:
                    print(f'[{ts}] {region_name} mismatches: {len(mismatches)}')
                    for mismatch in mismatches:
                        print('  ' + format_mismatch(region_name, mismatch))
                    correct_flips(xsdb, mismatches, args.pattern)
                    print(f'[{ts}] {region_name} corrected {len(mismatches)} words back to {human_hex(args.pattern)}')
                else:
                    print(f'[{ts}] {region_name} OK')
                total_mismatches += len(mismatches)
                all_mismatches.extend(mismatches)

            if total_mismatches == 0:
                print(f'[{ts}] iteration {iteration} complete, no bit flips detected.')
            else:
                print(f'[{ts}] iteration {iteration} complete, total flips corrected: {total_mismatches}')

            if args.log_file:
                with open(args.log_file, 'a') as logf:
                    logf.write(f'[{ts}] iteration {iteration} temp=0x{temp_value:08X} total_flips={total_mismatches}\n')
                    for region_name, (base, size) in (
                        ('OCM', (args.ocm_base, args.ocm_size)),
                        ('TCM', (args.tcm_base, args.tcm_size)),
                    ):
                        region_mismatches = [m for m in all_mismatches if base <= m['address'] < base + size]
                        for mismatch in region_mismatches:
                            logf.write(format_mismatch(region_name, mismatch) + '\n')

            iteration += 1
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print('Stop requested, shutting down XSDB session.')
    finally:
        xsdb.close()


def main():
    parser = argparse.ArgumentParser(description='Monitor Versal temperature and scan OCM/TCM for 0x55555555 flips.')
    parser.add_argument('--xsdb', default=DEFAULT_XSDB, help='XSDB command path')
    parser.add_argument('--target-filter', default=DEFAULT_TARGET_FILTER, help='XSDB target filter expression')
    parser.add_argument('--temp-addr', type=lambda v: int(v, 0), default=DEFAULT_TEMPERATURE_ADDRESS,
                        help='Temperature register address')
    parser.add_argument('--pattern', type=lambda v: int(v, 0), default=DEFAULT_PATTERN,
                        help='Expected 32-bit pattern value')
    parser.add_argument('--ocm-base', type=lambda v: int(v, 0), default=DEFAULT_OCM_BASE,
                        help='OCM start address')
    parser.add_argument('--ocm-size', type=lambda v: int(v, 0), default=DEFAULT_OCM_SIZE,
                        help='OCM region size in bytes')
    parser.add_argument('--tcm-base', type=lambda v: int(v, 0), default=DEFAULT_TCM_BASE,
                        help='TCM start address')
    parser.add_argument('--tcm-size', type=lambda v: int(v, 0), default=DEFAULT_TCM_SIZE,
                        help='TCM region size in bytes')
    parser.add_argument('--chunk-words', type=int, default=DEFAULT_CHUNK_WORDS,
                        help='Words per XSDB mrd chunk')
    parser.add_argument('--interval', type=float, default=DEFAULT_POLL_INTERVAL,
                        help='Seconds between scans')
    parser.add_argument('--log-file', default=None, help='Optional log file path')
    parser.add_argument('--verbose', action='store_true', help='Show XSDB command output')
    args = parser.parse_args()

    if args.ocm_size % 4 != 0 or args.tcm_size % 4 != 0:
        parser.error('OCM and TCM sizes must be a multiple of 4 bytes')

    return run_monitor(args)


if __name__ == '__main__':
    sys.exit(main())
