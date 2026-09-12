"""Compare external inspection modes against a reference mode ("Oracle").

This script reports the following validation metrics:

- impossible: Number of stacks matching a known impossible pattern. The
  classifiers are not exhaustive.

- raw_tvd: Total variation distance between this mode's stack distribution
  and the reference's distribution. Stacks classified as impossible are
  excluded.

- tvd_excess: raw_tvd minus tvd_floor.

- tvd_floor: IID heuristic for the TVD expected from finite sampling of both
  distributions. This is not a lower bound.
"""

import argparse
import contextlib
import math
import os
import random
import subprocess
import statistics
import sys
import tempfile
import time
from collections import Counter

import _remote_debugging

from snippets import CASES, _get_lineno


TRANSIENT_ERRORS = (OSError, RuntimeError, UnicodeDecodeError)

MODES = {
    "live-cache": (False, True),
    "live-nocache": (False, False),
    "blocking-cache": (True, True),
    "blocking-nocache": (True, False),
}


def collapse_cache(mode):
    blocking, _ = MODES[mode]
    return "blocking-nocache" if blocking else "live-nocache"


def tvd(left, right):
    lt, rt = sum(left.values()), sum(right.values())
    if not lt or not rt:
        return None
    return 0.5 * sum(
        abs(left[k] / lt - right[k] / rt) for k in set(left) | set(right)
    )


def tvd_floor(reference_obs, n_live):
    n_ref = sum(reference_obs.values())
    if not n_ref or not n_live:
        return None
    spread = sum(
        math.sqrt(p * (1 - p))
        for p in (c / n_ref for c in reference_obs.values())
    )
    return (
        0.5
        * math.sqrt(2 / math.pi)
        * math.sqrt(1 / n_live + 1 / n_ref)
        * spread
    )


def print_run_info(args, cases):
    print(sys.version.replace("\n", " "))
    print(
        f"cases={','.join(cases)} runs={args.runs} "
        f"duration={args.duration} "
        f"rate_khz={args.rate_khz} warmup={args.warmup} "
        f"poisson_sampling={args.poisson_sampling}"
    )


def terminate_process(proc):
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


@contextlib.contextmanager
def target_process(code, warmup):
    with tempfile.NamedTemporaryFile("w", suffix=".py", delete=False) as tmp:
        tmp.write(code)
        tmp.flush()
        tmp_name = tmp.name
    proc = None
    try:
        proc = subprocess.Popen(
            [sys.executable, tmp_name],
            stdout=subprocess.DEVNULL,
        )
        time.sleep(warmup)
        if proc.poll() is not None:
            raise RuntimeError(
                f"target exited unexpectedly with code {proc.returncode}"
            )
        yield proc
    finally:
        with contextlib.suppress(Exception):
            if proc is not None:
                terminate_process(proc)
        with contextlib.suppress(OSError):
            os.unlink(tmp_name)


def get_trace(unwinder, blocking, op="get_stack_trace"):
    call = getattr(unwinder, op)
    if not blocking:
        return call()
    unwinder.pause_threads()
    try:
        return call()
    finally:
        unwinder.resume_threads()


def iter_units(raw, op):
    if op == "get_stack_trace":
        for interp in raw:
            for thread in interp.threads:
                yield (
                    thread.thread_id,
                    None,
                    thread.status,
                    thread.frame_info,
                )
    else:
        for awaited_info in raw:
            for task in awaited_info.awaited_by:
                frames = [
                    frame
                    for coro in task.coroutine_stack
                    for frame in coro.call_stack
                ]
                if frames:
                    yield (task.task_id, task.task_name, None, frames)


def run_mode(case, mode_name, args):
    code, classify, *rest = case
    op = rest[0] if rest else "get_stack_trace"
    classify_units = op != "get_stack_trace"
    blocking, cache_frames = MODES[mode_name]
    result = {
        "attempts": 0,
        "samples": 0,
        "stacks": 0,
        "errors": 0,
        "observations": Counter(),
        "impossible": 0,
        "work_time": 0.0,
    }
    with target_process(code, args.warmup) as proc:
        unwinder = _remote_debugging.RemoteUnwinder(
            proc.pid,
            all_threads=True,
            cache_frames=cache_frames,
        )
        rate_hz = args.rate_khz * 1000
        period = 1.0 / rate_hz if rate_hz else 0
        next_sample = time.perf_counter()
        deadline = next_sample + args.duration

        while time.perf_counter() < deadline:
            if period:
                if args.poisson_sampling:
                    next_sample += random.expovariate(rate_hz)
                if next_sample >= deadline:
                    break
                now = time.perf_counter()
                if next_sample > now:
                    time.sleep(next_sample - now)
                if not args.poisson_sampling:
                    next_sample += period

            if time.perf_counter() >= deadline:
                break

            result["attempts"] += 1
            work_start = time.perf_counter()
            try:
                trace = get_trace(unwinder, blocking, op)
            except TRANSIENT_ERRORS:
                trace = None
                result["errors"] += 1
            result["work_time"] += time.perf_counter() - work_start

            if not trace:
                continue

            result["samples"] += 1
            for unit in iter_units(trace, op):
                frames = unit[3]
                impossible = classify is not None and (
                    classify(unit) if classify_units else classify(frames)
                )
                result["stacks"] += 1
                if impossible:
                    result["impossible"] += 1
                else:
                    result["observations"][
                        ";".join(
                            f"{frame.funcname}:{_get_lineno(frame)}"
                            for frame in frames
                        )
                    ] += 1
    return result


def result_metrics(result, reference_obs, is_reference, op):
    skip_tvd = is_reference or op != "get_stack_trace"
    n_live = sum(result["observations"].values())
    raw_tvd = None if skip_tvd else tvd(result["observations"], reference_obs)
    # IID heuristic, not a calibrated noise bound.
    floor = None if skip_tvd else tvd_floor(reference_obs, n_live)
    return {
        "samples": result["samples"],
        "stacks": result["stacks"],
        "empty": result["attempts"] - result["samples"] - result["errors"],
        "impossible": result["impossible"],
        "error_percent": (
            100.0 * result["errors"] / result["attempts"]
            if result["attempts"]
            else 0.0
        ),
        "impossible_percent": (
            100.0 * result["impossible"] / result["stacks"]
            if result["stacks"]
            else 0.0
        ),
        "avg_us": (
            1e6 * result["work_time"] / result["attempts"]
            if result["attempts"]
            else 0.0
        ),
        "raw_tvd": raw_tvd,
        "tvd_floor": floor,
        "tvd_excess": (
            None if (raw_tvd is None or floor is None) else raw_tvd - floor
        ),
    }


def fmt_stat(values, precision):
    vals = [value for value in values if value is not None]
    if not vals:
        return "n/a"
    mean = statistics.mean(vals)
    if len(vals) > 1:
        return f"{mean:.{precision}f}±{statistics.stdev(vals):.{precision}f}"
    return f"{mean:.{precision}f}"


def fmt_floor(values):
    vals = [value for value in values if value is not None]
    return "n/a" if not vals else f"{statistics.median(vals):.3f}"


def print_results(
    case_name, run_results, modes, reference_mode, op, has_classify
):
    is_sync = op == "get_stack_trace"
    ref_keys = len(run_results[0][reference_mode]["observations"])
    rows = {}
    for mode in modes:
        metrics = [
            result_metrics(
                results[mode],
                results[reference_mode]["observations"],
                mode == reference_mode,
                op,
            )
            for results in run_results
        ]
        rows[mode] = {
            "mode": mode,
            "samples": sum(item["samples"] for item in metrics),
            "stacks": sum(item["stacks"] for item in metrics),
            "empty": sum(item["empty"] for item in metrics),
            "impossible": sum(item["impossible"] for item in metrics),
            "errors": fmt_stat([item["error_percent"] for item in metrics], 2),
            "us": fmt_stat([item["avg_us"] for item in metrics], 2),
            "impossible_pct": fmt_stat(
                [item["impossible_percent"] for item in metrics], 2
            ),
            "raw_tvd": "ref"
            if mode == reference_mode
            else fmt_stat([item["raw_tvd"] for item in metrics], 3),
            "tvd_excess": "ref"
            if mode == reference_mode
            else fmt_stat([item["tvd_excess"] for item in metrics], 3),
            "tvd_floor": "-"
            if mode == reference_mode
            else fmt_floor([item["tvd_floor"] for item in metrics]),
        }

    show_stacks = any(row["stacks"] != row["samples"] for row in rows.values())
    ordered = [reference_mode] + [m for m in modes if m != reference_mode]

    columns = [("mode", "<18", "mode"), ("samples", ">9", "samples")]
    if show_stacks:
        columns.append(("stacks", ">9", "stacks"))
    if not is_sync:
        columns.append(("empty", ">9", "empty"))
    columns += [("µs", ">12", "us"), ("errors%", ">12", "errors")]
    if has_classify:
        columns += [
            ("impossible", ">10", "impossible"),
            ("impossible%", ">12", "impossible_pct"),
        ]
    if is_sync:
        columns += [
            ("raw_tvd", ">12", "raw_tvd"),
            ("tvd_excess", ">12", "tvd_excess"),
            ("tvd_floor", ">10", "tvd_floor"),
        ]

    print(f"\n{case_name} ({op}) ref_keys={ref_keys}")
    print(" ".join(f"{label:{spec}}" for label, spec, _ in columns))
    for mode in ordered:
        row = rows[mode]
        print(" ".join(f"{row[key]:{spec}}" for _, spec, key in columns))


def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--snippet",
        action="append",
        help="snippet name or Python file; may be passed more than once",
    )
    parser.add_argument(
        "--mode",
        choices=sorted(MODES),
        action="append",
        help="mode to run; may be passed more than once; omit to run all modes",
    )
    parser.add_argument(
        "--reference-mode",
        choices=sorted(MODES),
        default="blocking-nocache",
        help="mode used as the distribution reference",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=3.0,
        help="seconds to sample each mode",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=1,
        help="number of independent runs per case",
    )
    parser.add_argument(
        "--rate-khz",
        type=float,
        default=100.0,
        help="target sampling rate in kHz; 0 samples as fast as possible",
    )
    parser.add_argument(
        "--warmup",
        type=float,
        default=0.7,
        help="seconds to let the target run before sampling",
    )
    parser.add_argument(
        "--poisson-sampling",
        action="store_true",
        help=(
            "sample with exponential inter-arrival times instead of a fixed "
            "period"
        ),
    )
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs must be greater than zero")
    return args


def main():
    args = parse_args()
    cases = list(args.snippet) if args.snippet else sorted(CASES)
    modes = list(args.mode) if args.mode else sorted(MODES)
    if args.reference_mode not in modes:
        modes.append(args.reference_mode)

    print_run_info(args, cases)

    for name in cases:
        if name in CASES:
            case = CASES[name]
        else:
            with open(name, encoding="utf-8") as file:
                case = (file.read(), None)
        op = case[2] if len(case) > 2 else "get_stack_trace"
        case_ref = args.reference_mode
        case_modes = list(modes)
        if op != "get_stack_trace":
            case_ref = collapse_cache(case_ref)
            case_modes = list(
                dict.fromkeys(collapse_cache(m) for m in case_modes)
            )
        if case_ref not in case_modes:
            case_modes.append(case_ref)
        run_results = [
            {mode: run_mode(case, mode, args) for mode in case_modes}
            for _ in range(args.runs)
        ]
        print_results(
            name, run_results, case_modes, case_ref, op, case[1] is not None
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
