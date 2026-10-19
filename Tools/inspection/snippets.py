"""Scripts and classifiers for external inspection validation.

Classifiers detect only recognized impossible patterns. They do not validate
entire stacks. False does not imply that the stack as a whole is valid.
"""
import os


def _get_lineno(frame, default=None):
    if frame is None:
        return default
    loc = getattr(frame, "location", None)
    return getattr(loc, "lineno", default) if loc is not None else default


CODE = '''\
import time
import os
import sys
import math

def slow_fibonacci(n):
    """Intentionally slow recursive fibonacci - should show up prominently in profiler"""
    if n <= 1:
        return n
    return slow_fibonacci(n-1) + slow_fibonacci(n-2)

def medium_computation():
    """Medium complexity function"""
    result = 0
    for i in range(1000):
        result += math.sqrt(i) * math.sin(i)
    return result

def fast_loop():
    """Fast simple loop"""
    total = 0
    for i in range(100):
        total += i
    return total

def string_operations():
    """String manipulation that should be visible in profiler"""
    text = "hello world " * 100
    words = text.split()
    return " ".join(reversed(words))

def nested_calls():
    """Nested function calls to test call stack depth"""
    def level1():
        def level2():
            def level3():
                return medium_computation()
            return level3()
        return level2()
    return level1()

def main_loop():
    """Main computation loop with different execution paths"""
    iteration = 0

    while True:
        iteration += 1

        # Different execution paths with different frequencies
        if iteration % 50 == 0:
            # Expensive operation - should show high per-call time
            result = slow_fibonacci(20)

        elif iteration % 10 == 0:
            # Medium operation
            result = nested_calls()

        elif iteration % 5 == 0:
            # String operations
            result = string_operations()

        else:
            # Fast operation - most common
            result = fast_loop()

        # Small delay to make sampling more interesting
        time.sleep(0.001)

if __name__ == "__main__":
    main_loop()
'''

DEEP_STATIC_CODE = """\
import time
def factorial(n):
    if n <= 1:
        time.sleep(10000)
        return 1
    return n * factorial(n-1)

factorial(900)
"""

CODE_WITH_TONS_OF_THREADS = '''\
import time
import threading
import random
import math

def cpu_intensive_work():
    """Do some CPU intensive calculations"""
    result = 0
    for _ in range(10000):
        result += math.sin(random.random()) * math.cos(random.random())
    return result

def io_intensive_work():
    """Simulate IO intensive work with sleeps"""
    time.sleep(0.1)

def mixed_workload():
    """Mix of CPU and IO work"""
    while True:
        if random.random() < 0.3:
            cpu_intensive_work()
        else:
            io_intensive_work()

def create_threads(n):
    """Create n threads doing mixed workloads"""
    threads = []
    for _ in range(n):
        t = threading.Thread(target=mixed_workload, daemon=True)
        t.start()
        threads.append(t)
    return threads

# Start with 5 threads
active_threads = create_threads(5)

# Main thread manages threads and does work
while True:
    # Randomly add threads up to the limit
    if random.random() < 0.1:  # 10% chance each iteration
        if random.random() < 0.5 and len(active_threads) < 100:
            new_count = min(
                random.randint(1, 5),
                100 - len(active_threads),
            )
            new_threads = create_threads(new_count)
            active_threads.extend(new_threads)

    cpu_intensive_work()
    time.sleep(0.05)
'''

ASYNC_CODE = '''\
import asyncio
import contextlib
import math

def compute_slice(seed):
    result = 0.0
    for i in range(2000):
        result += math.sin(seed + i) * math.sqrt(i + 1)
    return result

async def leaf_task(seed):
    total = 0.0
    while True:
        total += compute_slice(seed)
        await asyncio.sleep(0)

async def parent_task(seed):
    child = asyncio.create_task(leaf_task(seed + 1000), name=f"leaf-{seed}")
    try:
        while True:
            compute_slice(seed)
            await asyncio.sleep(0.001)
    finally:
        child.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            await child

async def main():
    tasks = [
        asyncio.create_task(parent_task(i), name=f"parent-{i}")
        for i in range(8)
    ]
    await asyncio.gather(*tasks)

if __name__ == "__main__":
    asyncio.run(main())
'''


FLAT_ALTERNATING_CODE = """\
def leaf_a(): return sum(range(50))
def leaf_b(): return sum(range(50))
def hot_a(): return leaf_a()
def hot_b(): return leaf_b()
while True:
    hot_a(); hot_b()
"""


def _expected_lines(code):
    expected = {}
    for number, line in enumerate(code.splitlines(), 1):
        stripped = line.strip()
        if stripped.startswith("def "):
            expected[stripped[4:].split("(")[0].strip()] = number
    return expected


FLAT_ALTERNATING_LINES = _expected_lines(FLAT_ALTERNATING_CODE)


def classify_flat(frames):
    present = {}
    for frame in frames:
        if frame.funcname in FLAT_ALTERNATING_LINES:
            present[frame.funcname] = _get_lineno(frame, -1)
    if not present:
        return False
    for name, lineno in present.items():
        if lineno != FLAT_ALTERNATING_LINES[name]:
            return True
    hot_a, hot_b = "hot_a" in present, "hot_b" in present
    leaf_a, leaf_b = "leaf_a" in present, "leaf_b" in present
    any_a = leaf_a or hot_a
    any_b = leaf_b or hot_b
    return (any_a and any_b) or (leaf_a and not hot_a) or (leaf_b and not hot_b)


NESTED_ALTERNATING_CODE = """\
def burn_a():
    total = 0
    for i in range(20000):
        total += i
    return total

def burn_b():
    total = 0
    for i in range(20000):
        total += i
    return total

def a_leaf():
    return burn_a()

def b_leaf():
    return burn_b()

def a_parent():
    return a_leaf()

def b_parent():
    return b_leaf()

while True:
    a_parent()
    b_parent()
"""


NESTED_ALTERNATING_BRANCHES = {
    "a": ["a_parent", "a_leaf", "burn_a"],
    "b": ["b_parent", "b_leaf", "burn_b"],
}


def classify_nested(frames):
    frame_names = [frame.funcname for frame in frames]
    names = set(frame_names)
    present = [
        family
        for family, chain in NESTED_ALTERNATING_BRANCHES.items()
        if names.intersection(chain)
    ]
    if len(present) > 1:
        return True
    if not present:
        return False
    chain = NESTED_ALTERNATING_BRANCHES[present[0]]
    active = [name for name in chain if name in names]
    depth = chain.index(active[-1])
    if len(active) != depth + 1:
        return True
    indices = [frame_names.index(name) for name in reversed(active)]
    return indices != sorted(indices)


SHARED_LEAF_CODE = """\
def shared_leaf(long_run):
    total = 0
    if long_run:
        for i in range(50000):
            total += i
    else:
        for i in range(200):
            total += i
    return total

def a_wrapper():
    return shared_leaf(False)

def b_wrapper():
    return shared_leaf(True)

while True:
    a_wrapper()
    b_wrapper()
"""


def _branch_lines(code, marker):
    for number, line in enumerate(code.splitlines(), 1):
        if marker in line:
            return {number, number + 1}
    return set()


SHARED_LEAF_LONG_LINES = _branch_lines(SHARED_LEAF_CODE, "range(50000)")
SHARED_LEAF_SHORT_LINES = _branch_lines(SHARED_LEAF_CODE, "range(200)")


def classify_shared(frames):
    frame_names = [frame.funcname for frame in frames]
    names = set(frame_names)
    if "a_wrapper" in names and "b_wrapper" in names:
        return True
    if "shared_leaf" not in names:
        return False
    index = frame_names.index("shared_leaf")
    parent = frame_names[index + 1] if index + 1 < len(frame_names) else None
    if parent not in ("a_wrapper", "b_wrapper"):
        return True
    lineno = _get_lineno(frames[index], -1)
    if lineno in SHARED_LEAF_LONG_LINES:
        return parent != "b_wrapper"
    if lineno in SHARED_LEAF_SHORT_LINES:
        return parent != "a_wrapper"
    return False


GEN_ALTERNATING_CODE = """\
def agen(n):
    total = 0
    for i in range(n):
        total += i
        yield i

def bgen(n):
    total = 0
    for i in range(n):
        total += i
        yield i

def drv_a():
    for _ in agen(60):
        pass

def drv_b():
    for _ in bgen(60):
        pass

while True:
    drv_a()
    drv_b()
"""


def classify_gen(frames):
    names = {frame.funcname for frame in frames}
    return ("agen" in names and "drv_b" in names) or (
        "bgen" in names and "drv_a" in names
    )


DEEP_RECURSION_CODE = """\
def leaf():
    total = 0
    for i in range(40):
        total += i

def a(n):
    return a(n - 1) if n else leaf()

def b(n):
    return b(n - 1) if n else leaf()

while True:
    a(300)
    b(300)
"""


def classify_recursion(frames):
    names = {frame.funcname for frame in frames}
    return "a" in names and "b" in names


ASYNC_RUNNING_TASK_CODE = """\
import asyncio


def leaf_hot(n):
    return sum(range(n))


def leaf_rare(n):
    return sum(range(n))


async def run_hot():
    while True:
        leaf_hot(50000)
        await asyncio.sleep(0)


async def run_rare(k):
    while True:
        leaf_rare(500)
        await asyncio.sleep(0)


async def main():
    tasks = [asyncio.create_task(run_hot(), name="hot")]
    for k in range(8):
        tasks.append(asyncio.create_task(run_rare(k), name=f"rare{k}"))
    await asyncio.gather(*tasks)


asyncio.run(main())
"""


def _name_tag(label):
    label = (label or "").lower()
    return "hot" if "hot" in label else "rare" if "rare" in label else None


def _frame_tag(frames):
    fns = {frame.funcname for frame in frames}
    hot = bool(fns & {"run_hot", "leaf_hot"})
    rare = bool(fns & {"run_rare", "leaf_rare"})
    return (
        "mixed"
        if (hot and rare)
        else "hot"
        if hot
        else "rare"
        if rare
        else None
    )


def classify_async_running_task(unit):
    name = _name_tag(unit[1])
    frame = _frame_tag(unit[3])
    return name is not None and frame is not None and frame != name


CODE_OBJECT_REUSE_CODE = """\
SRC_A = "def func_a(n):\\n total=0\\n for i in range(n): total+=i*i\\n return total\\n"
SRC_B = "def func_b(n):\\n total=0\\n for i in range(n): total+=i*i\\n return total\\n"
WORK = 60000


def build_a():
    ns = {}
    code = compile(SRC_A, "A_file.py", "exec")
    exec(code, ns)
    return ns["func_a"], code


def build_b():
    ns = {}
    code = compile(SRC_B, "B_file.py", "exec")
    exec(code, ns)
    return ns["func_b"], code


while True:
    fa, ca = build_a()
    fa(WORK)   # call_a
    del fa, ca
    fb, cb = build_b()
    fb(WORK)   # call_b
    del fb, cb
"""


def _marker_line(code, marker):
    for number, line in enumerate(code.splitlines(), 1):
        if marker in line:
            return number
    return None


CALL_A_LINE = _marker_line(CODE_OBJECT_REUSE_CODE, "# call_a")
CALL_B_LINE = _marker_line(CODE_OBJECT_REUSE_CODE, "# call_b")


def classify_code_object_reuse(frames):
    real = [f for f in frames if f.funcname != "<GC>"]
    leaf = next((f for f in real if f.funcname in ("func_a", "func_b")), None)
    if leaf is None:
        return False
    base = os.path.basename(leaf.filename)
    fn = leaf.funcname
    if (fn == "func_a" and base == "B_file.py") or (
        fn == "func_b" and base == "A_file.py"
    ):
        return True
    index = real.index(leaf)
    caller = real[index + 1] if index + 1 < len(real) else None
    line = _get_lineno(caller)
    if line == CALL_A_LINE and fn == "func_b":
        return True
    if line == CALL_B_LINE and fn == "func_a":
        return True
    return False


OVERSIZED_CHUNK_CODE = """\
NLOCALS = 1800

def make(name, tag, hotbody):
    params = ", ".join(f"x{i}=0" for i in range(NLOCALS))
    src = (
        f"def hot_{tag}():\\n{hotbody}\\n"
        f"def {name}({params}):\\n    return hot_{tag}()\\n"
    )
    exec(compile(src, f"{tag}.py", "exec"), globals())

make("big_a", "a", "    s=0\\n    for i in range(2000):\\n        s+=i*3\\n    return s")
make("big_b", "b", "    s=1\\n    for i in range(2000):\\n        s^=(i<<1)\\n    return s")

while True:
    big_a()
    big_b()
"""


OVERSIZED_A_FUNCS = {"big_a", "hot_a"}
OVERSIZED_B_FUNCS = {"big_b", "hot_b"}


def classify_oversized_chunk(frames):
    saw_a = saw_b = False
    for frame in frames:
        base = os.path.basename(frame.filename)
        fn = frame.funcname
        if base == "a.py":
            if fn in OVERSIZED_A_FUNCS:
                saw_a = True
            elif fn in OVERSIZED_B_FUNCS:
                return True
        elif base == "b.py":
            if fn in OVERSIZED_B_FUNCS:
                saw_b = True
            elif fn in OVERSIZED_A_FUNCS:
                return True
    return saw_a and saw_b


CODE_EXAMPLES = {
    "basic": {
        "code": CODE,
        "description": "Mixed workload with fibonacci, computations, and string operations",
    },
    "deep_static": {
        "code": DEEP_STATIC_CODE,
        "description": "Deep recursive call stack with 900+ frames (factorial)",
    },
    "threads": {
        "code": CODE_WITH_TONS_OF_THREADS,
        "description": "Tons of threads doing mixed CPU/IO work",
    },
    "asyncio": {
        "code": ASYNC_CODE,
        "description": "Asyncio tasks with active and awaited coroutine chains",
    },
}

CASES = {
    "basic": (CODE, None),
    "deep_static": (DEEP_STATIC_CODE, None),
    "threads": (CODE_WITH_TONS_OF_THREADS, None),
    "asyncio": (ASYNC_CODE, None, "get_async_stack_trace"),
    "flat_alternating": (FLAT_ALTERNATING_CODE, classify_flat),
    "nested_alternating": (NESTED_ALTERNATING_CODE, classify_nested),
    "shared_leaf": (SHARED_LEAF_CODE, classify_shared),
    "gen_alternating": (GEN_ALTERNATING_CODE, classify_gen),
    "deep_recursion": (DEEP_RECURSION_CODE, classify_recursion),
    "async_running_task": (
        ASYNC_RUNNING_TASK_CODE,
        classify_async_running_task,
        "get_async_stack_trace",
    ),
    "code_object_reuse": (CODE_OBJECT_REUSE_CODE, classify_code_object_reuse),
    "oversized_chunk": (OVERSIZED_CHUNK_CODE, classify_oversized_chunk),
}
