# Sample script for use by test_gdb.test_jit

import _testinternalcapi
import operator


WARMUP_ITERATIONS = _testinternalcapi.TIER2_THRESHOLD + 10


def jit_bt_hot(depth, warming_up_caller=False):
    if depth == 0:
        if not warming_up_caller:
            id(42)
        return

    for iteration in range(WARMUP_ITERATIONS):
        operator.call(
            jit_bt_hot,
            depth - 1,
            warming_up_caller or iteration + 1 != WARMUP_ITERATIONS,
        )


# Warm the shared shim once without hitting builtin_id so the real run uses
# the steady-state shim path when GDB breaks inside id(42).
jit_bt_hot(1, warming_up_caller=True)
jit_bt_hot(1)
