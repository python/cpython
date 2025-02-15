"""
Input for test_profile.py and test_cprofile.py.

IMPORTANT: This stuff is touchy. If you modify anything above the
test class you'll have to regenerate the stats by running the two
test files.

*ALL* NUMBERS in the expected output are relevant.  If you change
the formatting of pstats, please don't just regenerate the expected
output without checking very carefully that not a single number has
changed.
"""

import sys

# In order to have reproducible time, we simulate a timer in the global
# variable 'TICKS', which represents simulated time in milliseconds.
# (We can't use a helper function increment the timer since it would be
# included in the profile and would appear to consume all the time.)
TICKS = 42000

def timer():
    return TICKS

def testfunc():
    # 1 call
    # 1000 ticks total: 270 ticks local, 730 ticks in subfunctions
    global TICKS
    TICKS += 99
    helper()                            # 300
    helper()                            # 300
    TICKS += 171
    factorial(14)                       # 130

def factorial(n):
    # 23 calls total
    # 170 ticks total, 150 ticks local
    # 3 primitive calls, 130, 20 and 20 ticks total
    # including 116, 17, 17 ticks local
    global TICKS
    if n > 0:
        TICKS += n
        return mul(n, factorial(n-1))
    else:
        TICKS += 11
        return 1

def mul(a, b):
    # 20 calls
    # 1 tick, local
    global TICKS
    TICKS += 1
    return a * b

def helper():
    # 2 calls
    # 300 ticks total: 20 ticks local, 260 ticks in subfunctions
    global TICKS
    TICKS += 1
    helper1()                           # 30
    TICKS += 2
    helper1()                           # 30
    TICKS += 6
    helper2()                           # 50
    TICKS += 3
    helper2()                           # 50
    TICKS += 2
    helper2()                           # 50
    TICKS += 5
    helper2_indirect()                  # 70
    TICKS += 1

def helper1():
    # 4 calls
    # 30 ticks total: 29 ticks local, 1 tick in subfunctions
    global TICKS
    TICKS += 10
    hasattr(C(), "foo")                 # 1
    TICKS += 19
    lst = []
    lst.append(42)                      # 0
    sys.exception()                     # 0

def helper2_indirect():
    helper2()                           # 50
    factorial(3)                        # 20

def helper2():
    # 8 calls
    # 50 ticks local: 39 ticks local, 11 ticks in subfunctions
    global TICKS
    TICKS += 11
    hasattr(C(), "bar")                 # 1
    TICKS += 13
    subhelper()                         # 10
    TICKS += 15

def subhelper():
    # 8 calls
    # 10 ticks total: 8 ticks local, 2 ticks in subfunctions
    global TICKS
    TICKS += 2
    for i in range(2):                  # 0
        try:
            C().foo                     # 1 x 2
        except AttributeError:
            TICKS += 3                  # 3 x 2

class C:
    def __getattr__(self, name):
        # 28 calls
        # 1 tick, local
        global TICKS
        TICKS += 1
        raise AttributeError
