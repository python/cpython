from test_support import TestFailed

# A test for SF bug 422177:  manifest float constants varied way too much in
# precision depending on whether Python was loading a module for the first
# time, or reloading it from a precompiled .pyc.  The "expected" failure
# mode is that when test_import imports this after all .pyc files have been
# erased, it passes, but when test_import imports this from
# double_const.pyc, it fails.  This indicates a woeful loss of precision in
# the marshal format for doubles.  It's also possible that repr() doesn't
# produce enough digits to get reasonable precision for this box.

PI    = 3.14159265358979324
TWOPI = 6.28318530717958648

PI_str    = "3.14159265358979324"
TWOPI_str = "6.28318530717958648"

# Verify that the double x is within a few bits of eval(x_str).
def check_ok(x, x_str):
    assert x > 0.0
    x2 = eval(x_str)
    assert x2 > 0.0
    diff = abs(x - x2)
    # If diff is no larger than 3 ULP (wrt x2), then diff/8 is no larger
    # than 0.375 ULP, so adding diff/8 to x2 should have no effect.
    if x2 + (diff / 8.) != x2:
        raise TestFailed("Manifest const %s lost too much precision " % x_str)

check_ok(PI, PI_str)
check_ok(TWOPI, TWOPI_str)
