# Test various flavors of legal and illegal future statements

from test_support import unload
import re

rx = re.compile('\((\S+).py, line (\d+)')

def check_error_location(msg):
    mo = rx.search(msg)
    print "SyntaxError %s %s" % mo.group(1, 2)

# The first two tests should work

unload('test_future1')
import test_future1

unload('test_future2')
import test_future2

# The remaining tests should fail
try:
    import test_future3
except SyntaxError, msg:
    check_error_location(str(msg))

try:
    import test_future4
except SyntaxError, msg:
    check_error_location(str(msg))

try:
    import test_future5
except SyntaxError, msg:
    check_error_location(str(msg))

try:
    import test_future6
except SyntaxError, msg:
    check_error_location(str(msg))

try:
    import test_future7
except SyntaxError, msg:
    check_error_location(str(msg))
