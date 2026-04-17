"""
Benchmark the pure-Python pickle._Pickler and pickle._Unpickler paths.

These are the fallbacks when the C accelerator isn't available (embedded
builds, alternative implementations, stdlib regression testing). They also
mirror the semantics that anyone subclassing pickle.Pickler in Python
directly will hit.
"""
import io
import json
import pickle
import statistics
import timeit

# Force pure-Python path by using _Pickler / _Unpickler directly
PyPickler = pickle._Pickler
PyUnpickler = pickle._Unpickler

SAMPLES = {
    "list_of_ints_10k":     list(range(10_000)),
    "list_of_strs_1k":      [f"item_{i}" for i in range(1_000)],
    "dict_str_int_5k":      {f"key_{i}": i for i in range(5_000)},
    "nested_list_of_dicts": [{"id": i, "name": f"n{i}", "val": i*2} for i in range(500)],
    "deep_list":            [[i] * 10 for i in range(500)],
}

def bench_dump(obj, number):
    buf = io.BytesIO()
    def run():
        buf.seek(0); buf.truncate(0)
        p = PyPickler(buf, protocol=5)
        p.dump(obj)
    return timeit.repeat(run, number=number, repeat=9)

def bench_load(data, number):
    def run():
        u = PyUnpickler(io.BytesIO(data))
        u.load()
    return timeit.repeat(run, number=number, repeat=9)

CONFIG = {
    "list_of_ints_10k":     {"dump": 200, "load": 200},
    "list_of_strs_1k":      {"dump": 500, "load": 500},
    "dict_str_int_5k":      {"dump": 200, "load": 200},
    "nested_list_of_dicts": {"dump": 1000, "load": 1000},
    "deep_list":            {"dump": 1000, "load": 1000},
}

results = {}
for name, obj in SAMPLES.items():
    # Pre-serialize with the C pickler (it's fine, we only benchmark the pure-Python path)
    data = pickle.dumps(obj, protocol=5)
    n = CONFIG[name]
    drun = bench_dump(obj,  n["dump"])
    lrun = bench_load(data, n["load"])
    results[name] = {
        "dump_number": n["dump"],
        "load_number": n["load"],
        "dump_runs": drun,
        "load_runs": lrun,
        "dump_median": statistics.median(drun),
        "load_median": statistics.median(lrun),
        "dump_min": min(drun),
        "load_min":  min(lrun),
    }
    print(f"{name:25s} dump_med={results[name]['dump_median']:.5f}s  load_med={results[name]['load_median']:.5f}s", flush=True)

print(json.dumps(results, indent=2, sort_keys=True))
