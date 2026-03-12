# Sample script for use by test_gdb.test_jit

import operator
import sys


def jit_bt_hot(depth, warming_up_caller=False):
    if warming_up_caller:
        return
    if depth == 0:
        id(42)
        return

    warming_up = True
    while warming_up:
        warming_up = sys._jit.is_enabled() & (not sys._jit.is_active())
        operator.call(jit_bt_hot, depth - 1, warming_up)


jit_bt_hot(10)
