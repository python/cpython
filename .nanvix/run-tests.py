#!/usr/bin/env python3
# run-tests.py - Host-side batched test runner for nanvixd
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Splits test modules into batches and runs each batch in its own
# nanvixd invocation.  Each batch gets a fresh VM process, which
# resets the 32MB heap and avoids OOM from cumulative fragmentation.
#
# Batches run sequentially.  All batches are run regardless of
# failures; the overall exit code reflects whether any batch failed.
#
# Supports both direct mode (single-process / multi-process) and
# standalone mode (ramfs).  In standalone mode, the guest command line
# uses nanvixd's semicolon-delimited format to set PYTHONHOME and
# PYTHONDONTWRITEBYTECODE inside the guest.
#
# Usage:
#     cd <sysroot> && python3 <path>/run-tests.py <module> [<module> ...]
#
# Environment:
#     NANVIX_TEST_BATCH_SIZE - modules per nanvixd invocation (default: 4)
#     NANVIXD_EXTRA_ARGS     - extra flags passed to nanvixd (optional)
#     NANVIX_STANDALONE      - set to "1" to enable standalone mode
#     REGRTEST_TIMEOUT       - per-test timeout in seconds (default: 120)

import os
import shlex
import shutil
import subprocess
import sys
import tempfile

BATCH_SIZE = int(os.environ.get("NANVIX_TEST_BATCH_SIZE", "4"))
STANDALONE = os.environ.get("NANVIX_STANDALONE", "") == "1"
NANVIXD = "./bin/nanvixd.exe" if sys.platform == "win32" else "./bin/nanvixd.elf"
# Must match config.PYTHON_VERSION / config.python_binary().
PYTHON_BIN = os.environ.get("NANVIX_PYTHON_BIN", "./bin/python3.12")
# Must match _sysconfigdata_{ABIFLAGS}_{MACHDEP}_{MULTIARCH} from configure.
SYSCONFIGDATA_NAME = os.environ.get(
    "NANVIX_SYSCONFIGDATA_NAME", "_sysconfigdata__nanvix_"
)
REGRTEST_TIMEOUT = os.environ.get("REGRTEST_TIMEOUT", "120")


def run_batch(
    batch_num: int,
    batch: list[str],
    nanvixd_extra: list[str],
) -> tuple[int, int, list[str]]:
    """Run a single batch. Returns (batch_num, returncode, modules)."""

    # Each batch gets its own temp directory so parallel nanvixd instances
    # don't collide in /tmp (the guest shares the host filesystem in direct
    # mode and the VM's entropy source may produce identical random seeds).
    batch_tmpdir = tempfile.mkdtemp(prefix=f"nanvix_batch{batch_num}_")

    try:
        if STANDALONE:
            # Standalone: all python args packed into one semicolon-delimited
            # string.  nanvixd splits on spaces for argv and on semicolons
            # for environment variables.
            modules_str = " ".join(batch)
            regrtest_args = (
                f"-B -m test --timeout={REGRTEST_TIMEOUT} {modules_str}"
            )
            python_arg = (
                f"{regrtest_args};PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1"
                f" TMPDIR=/tmp"
                f" NANVIX_STANDALONE=1"
                f" _PYTHON_SYSCONFIGDATA_NAME={SYSCONFIGDATA_NAME}"
            )

            cmd = [
                NANVIXD,
                *nanvixd_extra,
                "--",
                PYTHON_BIN,
                python_arg,
            ]
        else:
            # Direct mode: separate argv elements, invoke run-regrtest.py
            # in the guest.  --tmpdir must come before the module list.
            cmd = [
                NANVIXD,
                *nanvixd_extra,
                "--",
                PYTHON_BIN,
                "./run-regrtest.py",
                "--tmpdir",
                batch_tmpdir,
                *batch,
            ]

        try:
            rc = subprocess.run(
                cmd, stdin=subprocess.DEVNULL, timeout=600
            ).returncode
        except subprocess.TimeoutExpired:
            print(f"  TIMEOUT: batch {batch_num} exceeded 600s")
            rc = 124  # match GNU timeout exit code
        return batch_num, rc, batch
    finally:
        shutil.rmtree(batch_tmpdir, ignore_errors=True)


def main() -> int:
    modules = sys.argv[1:]
    if not modules:
        print("run-tests.py: no modules specified", file=sys.stderr)
        return 1

    nanvixd_extra = shlex.split(
        os.environ.get("NANVIXD_EXTRA_ARGS", ""),
        posix=(sys.platform != "win32"),
    )

    batches = []
    for i in range(0, len(modules), BATCH_SIZE):
        batches.append(modules[i : i + BATCH_SIZE])

    total_batches = len(batches)
    mode_label = "standalone" if STANDALONE else "direct"
    print(
        f"  Running {len(modules)} modules in {total_batches} batches "
        f"({BATCH_SIZE}/batch, sequential, {mode_label} mode)"
    )
    for i, batch in enumerate(batches, 1):
        print(f"    Batch {i}: {' '.join(batch)}")

    failed: list[str] = []
    failed_batches = 0

    for i, batch in enumerate(batches, 1):
        batch_num, rc, batch = run_batch(i, batch, nanvixd_extra)
        if rc != 0:
            print(f"  FAIL: batch {batch_num} exited with status {rc}")
            failed.extend(batch)
            failed_batches += 1
        else:
            print(f"  OK: batch {batch_num}")

    total = len(modules)
    if failed:
        print(f"  FAILED: {failed_batches} batch(es) ({len(failed)}/{total} modules affected):")
        print(f"    {' '.join(failed)}")
        return 1

    print(f"  PASS: all {total} modules passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
