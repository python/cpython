#!/usr/bin/env python3
"""Race instance attribute stores against concurrent __dict__ replacement.

Expected on fixed or GIL builds: the script runs to completion.
Symptom on affected free-threaded debug/ASAN builds: intermittent crash,
abort, or sanitizer use-after-free report in dict/object attribute storage.
"""

import argparse
import faulthandler
import os
import sys
import sysconfig
import threading
import time
from collections import deque


class Target:
    pass


def gil_state():
    is_gil_enabled = getattr(sys, "_is_gil_enabled", None)
    if is_gil_enabled is not None:
        try:
            return "enabled" if is_gil_enabled() else "disabled"
        except Exception:
            pass
    if sysconfig.get_config_var("Py_GIL_DISABLED"):
        return "unknown (free-threaded build)"
    return "unknown (regular build)"


def positive_int(text):
    value = int(text, 0)
    if value <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return value


def positive_float(text):
    value = float(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return value


PROOF_PREFIX = "__stale_write_probe_"


def attr_writer(obj, names, stop, started, errors, delete_stride):
    index = 0
    started.wait()
    while not stop.is_set():
        name = f"{PROOF_PREFIX}{threading.get_ident()}_{index}"
        try:
            # Vulnerable path: _PyObject_StoreInstanceAttribute() borrows the
            # managed dict, drops the object lock, then stores through it.
            setattr(obj, name, index)
            if delete_stride and index % delete_stride == 0:
                try:
                    delattr(obj, names[(index // 3) % len(names)])
                except AttributeError:
                    # A concurrent __dict__ replacement may have removed it.
                    pass
        except BaseException as exc:
            errors.append((threading.current_thread().name, repr(exc)))
            stop.set()
            return

        index += 1
        if index & 0x3F == 0:
            time.sleep(0)


def dict_replacer(
    obj,
    names,
    stop,
    started,
    errors,
    dict_size,
    retired,
    retired_lock,
    keep_retired,
):
    index = 0
    started.wait()
    while not stop.is_set():
        try:
            old_dict = obj.__dict__
            # Swaps the managed dict pointer and drops the old dict reference.
            obj.__dict__ = {
                names[(index + offset) % len(names)]: (index, offset)
                for offset in range(dict_size)
            }
            # Keep old dicts alive after detaching them from obj. If writer
            # threads mutate one after this post-detach snapshot, setattr()
            # stored through a dict pointer that was no longer obj.__dict__.
            old_keys = frozenset(old_dict)
            with retired_lock:
                retired.append((old_dict, old_keys))
                while len(retired) > keep_retired:
                    retired.popleft()
        except BaseException as exc:
            errors.append((threading.current_thread().name, repr(exc)))
            stop.set()
            return

        index += 1
        if index & 0xF == 0:
            time.sleep(0)


def stale_write_monitor(retired, retired_lock, stop, started, errors):
    started.wait()
    while not stop.is_set():
        with retired_lock:
            snapshots = tuple(retired)
        for old_dict, old_keys in snapshots:
            current_keys = set(old_dict)
            added = current_keys - old_keys
            stale_probe_keys = [key for key in added if key.startswith(PROOF_PREFIX)]
            if stale_probe_keys:
                errors.append(
                    (
                        "monitor",
                        "confirmed stale write into detached __dict__: "
                        f"{stale_probe_keys[0]!r}",
                    )
                )
                stop.set()
                return
        time.sleep(0)


def parse_args():
    cpu_count = os.cpu_count() or 2
    parser = argparse.ArgumentParser(
        description="Stress setattr(obj, name, value) racing obj.__dict__ = {}."
    )
    parser.add_argument("--seconds", type=positive_float, default=30.0)
    parser.add_argument("--writers", type=positive_int, default=max(2, (cpu_count * 3) // 2))
    parser.add_argument(
        "--replacers", type=positive_int, default=max(2, cpu_count)
    )
    parser.add_argument("--names", type=positive_int, default=1024)
    parser.add_argument("--dict-size", type=positive_int, default=1)
    parser.add_argument("--delete-stride", type=positive_int, default=1)
    parser.add_argument("--keep-retired", type=positive_int, default=2048)
    return parser.parse_args()


def main():
    faulthandler.enable()
    args = parse_args()

    obj = Target()
    names = [f"a{i}" for i in range(args.names)]
    obj.__dict__ = {name: None for name in names[: args.dict_size]}

    stop = threading.Event()
    started = threading.Barrier(args.writers + args.replacers + 2)
    errors = []
    retired = deque()
    retired_lock = threading.Lock()
    threads = []

    print("finding: 108 instance setattr / __dict__ replacement race", flush=True)
    print(
        "expected signal: stale writer key appears in a detached old __dict__, "
        "or native crash/abort/sanitizer report",
        flush=True,
    )
    print(
        f"parameters: seconds={args.seconds:g}; GIL={gil_state()}; "
        f"writers={args.writers}; replacers={args.replacers}; "
        f"dict_size={args.dict_size}; keep_retired={args.keep_retired}; "
        f"delete_stride={args.delete_stride}",
        flush=True,
    )
    print("-" * 72, flush=True)

    for index in range(args.writers):
        threads.append(
            threading.Thread(
                target=attr_writer,
                name=f"writer-{index}",
                args=(
                    obj,
                    names,
                    stop,
                    started,
                    errors,
                    args.delete_stride,
                ),
            )
        )

    for index in range(args.replacers):
        threads.append(
            threading.Thread(
                target=dict_replacer,
                name=f"replacer-{index}",
                args=(
                    obj,
                    names,
                    stop,
                    started,
                    errors,
                    args.dict_size,
                    retired,
                    retired_lock,
                    args.keep_retired,
                ),
            )
        )

    threads.append(
        threading.Thread(
            target=stale_write_monitor,
            name="stale-write-monitor",
            args=(retired, retired_lock, stop, started, errors),
        )
    )

    for thread in threads:
        thread.start()

    started.wait()
    deadline = time.monotonic() + args.seconds
    try:
        while time.monotonic() < deadline and not stop.is_set():
            time.sleep(0.05)
    finally:
        stop.set()
        for thread in threads:
            thread.join()

    if errors:
        for name, error in errors:
            print(f"{name} raised {error}", file=sys.stderr)
        return 1

    print("finished without a visible crash or sanitizer abort", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
