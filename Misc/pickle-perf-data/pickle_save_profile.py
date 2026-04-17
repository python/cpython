"""
Profile where pure-Python pickle._Pickler.save() spends its time,
broken down by call count.
"""
import cProfile
import io
import pickle
import pstats

PyPickler = pickle._Pickler

def dump_it(obj):
    buf = io.BytesIO()
    p = PyPickler(buf, protocol=5)
    p.dump(obj)

WORKLOADS = {
    "list_of_dicts":   [{"id": i, "name": f"n{i}", "v": i*2} for i in range(500)],
    "deep_list":       [[i]*10 for i in range(500)],
    "list_of_ints":    list(range(10_000)),
    "dict_str_int":    {f"k{i}": i for i in range(5_000)},
}

for name, obj in WORKLOADS.items():
    print(f"\n===== {name} =====")
    pr = cProfile.Profile()
    pr.enable()
    for _ in range(50):
        dump_it(obj)
    pr.disable()
    st = pstats.Stats(pr).sort_stats("cumulative")
    st.print_stats(12)
