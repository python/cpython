"""Test suite for the profile module."""

import profile

# In order to have reproducible time, we simulate a timer in the global
# variable 'ticks', which represents simulated time in milliseconds.
# (We can't use a helper function increment the timer since it would be
# included in the profile and would appear to consume all the time.)
ticks = 0

def test_main():
    global ticks
    ticks = 0
    prof = profile.Profile(timer)
    prof.runctx("testfunc()", globals(), globals())
    prof.print_stats()

def timer():
    return ticks*0.001

def testfunc():
    # 1 call
    # 1000 ticks total: 400 ticks local, 600 ticks in subfunctions
    global ticks
    ticks += 199
    helper()                            # 300
    helper()                            # 300
    ticks += 201

def helper():
    # 2 calls
    # 300 ticks total: 40 ticks local, 260 ticks in subfunctions
    global ticks
    ticks += 1
    helper1()                           # 30
    ticks += 3
    helper1()                           # 30
    ticks += 6
    helper2()                           # 50
    ticks += 5
    helper2()                           # 50
    ticks += 4
    helper2()                           # 50
    ticks += 7
    helper2()                           # 50
    ticks += 14

def helper1():
    # 4 calls
    # 30 ticks total: 29 ticks local, 1 tick in subfunctions
    global ticks
    ticks += 10
    hasattr(C(), "foo")
    ticks += 19

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
    for i in range(2):
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
