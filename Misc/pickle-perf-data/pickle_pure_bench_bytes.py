"""
Bytes/bytearray-heavy workload to evaluate F6 (extend save() fast path
to bytes).
"""
import io
import json
import pickle
import statistics
import timeit

PyPickler = pickle._Pickler
PyUnpickler = pickle._Unpickler

SAMPLES = {
    "list_of_short_bytes_5k":    [bytes([i & 0xff]) * 4 for i in range(5000)],
    "list_of_medium_bytes_1k":   [bytes(range(32)) for _ in range(1000)],
    "list_of_bytearrays_1k":     [bytearray(range(16)) for _ in range(1000)],
    "dict_bytes_to_int_2k":      {bytes([i & 0xff, (i>>8) & 0xff]): i for i in range(2000)},
}

CONFIG = {k: {"dump": 500, "load": 500} for k in SAMPLES}

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

results = {}
for name, obj in SAMPLES.items():
    data = pickle.dumps(obj, protocol=5)
    n = CONFIG[name]
    drun = bench_dump(obj, n["dump"])
    lrun = bench_load(data, n["load"])
    results[name] = {
        "dump_runs": drun,
        "load_runs": lrun,
        "dump_median": statistics.median(drun),
        "load_median": statistics.median(lrun),
        "dump_min": min(drun),
        "load_min":  min(lrun),
    }
    print(f"{name:30s} dump_med={results[name]['dump_median']:.5f}s  load_med={results[name]['load_median']:.5f}s", flush=True)

print(json.dumps(results, indent=2, sort_keys=True))
