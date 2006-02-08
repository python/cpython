"""Test suite for the cProfile module."""

import cProfile, pstats, sys

# In order to have reproducible time, we simulate a timer in the global
# variable 'ticks', which represents simulated time in milliseconds.
# (We can't use a helper function increment the timer since it would be
# included in the profile and would appear to consume all the time.)
ticks = 0

# IMPORTANT: this is an output test.  *ALL* NUMBERS in the expected
# output are relevant.  If you change the formatting of pstats,
# please don't just regenerate output/test_cProfile without checking
# very carefully that not a single number has changed.

def test_main():
    global ticks
    ticks = 42000
    prof = cProfile.Profile(timer, 0.001)
    prof.runctx("testfunc()", globals(), locals())
    assert ticks == 43000, ticks
    st = pstats.Stats(prof)
    st.strip_dirs().sort_stats('stdname').print_stats()
    st.print_callees()
    st.print_callers()

def timer():
    return ticks

def testfunc():
    # 1 call
    # 1000 ticks total: 270 ticks local, 730 ticks in subfunctions
    global ticks
    ticks += 99
    helper()                            # 300
    helper()                            # 300
    ticks += 171
    factorial(14)                       # 130

def factorial(n):
    # 23 calls total
    # 170 ticks total, 150 ticks local
    # 3 primitive calls, 130, 20 and 20 ticks total
    # including 116, 17, 17 ticks local
    global ticks
    if n > 0:
        ticks += n
        return mul(n, factorial(n-1))
    else:
        ticks += 11
        return 1

def mul(a, b):
    # 20 calls
    # 1 tick, local
    global ticks
    ticks += 1
    return a * b

def helper():
    # 2 calls
    # 300 ticks total: 20 ticks local, 260 ticks in subfunctions
    global ticks
    ticks += 1
    helper1()                           # 30
    ticks += 2
    helper1()                           # 30
    ticks += 6
    helper2()                           # 50
    ticks += 3
    helper2()                           # 50
    ticks += 2
    helper2()                           # 50
    ticks += 5
    helper2_indirect()                  # 70
    ticks += 1

def helper1():
    # 4 calls
    # 30 ticks total: 29 ticks local, 1 tick in subfunctions
    global ticks
    ticks += 10
    hasattr(C(), "foo")                 # 1
    ticks += 19
    lst = []
    lst.append(42)                      # 0
    sys.exc_info()                      # 0

def helper2_indirect():
    helper2()                           # 50
    factorial(3)                        # 20

def helper2():
    # 8 calls
    # 50 ticks local: 39 ticks local, 11 ticks in subfunctions
    global ticks
    ticks += 11
    hasattr(C(), "bar")                 # 1
    ticks += 13
    subhelper()                         # 10
    ticks += 15

def subhelper():
    # 8 calls
    # 10 ticks total: 8 ticks local, 2 ticks in subfunctions
    global ticks
    ticks += 2
    for i in range(2):                  # 0
        try:
            C().foo                     # 1 x 2
        except AttributeError:
            ticks += 3                  # 3 x 2

class C:
    def __getattr__(self, name):
        # 28 calls
        # 1 tick, local
        global ticks
        ticks += 1
        raise AttributeError

if __name__ == "__main__":
    test_main()
