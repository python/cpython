from test_support import verbose, TestSkipped
import locale
import sys

if sys.platform == 'darwin':
    raise TestSkipped("Locale support on MacOSX is minimal and cannot be tested")
oldlocale = locale.setlocale(locale.LC_NUMERIC)

tloc = "en_US"
if sys.platform[:3] == "win":
    tloc = "en"

try:
    locale.setlocale(locale.LC_NUMERIC, tloc)
except locale.Error:
    raise ImportError, "test locale %s not supported" % tloc

def testformat(formatstr, value, grouping = 0, output=None):
    if verbose:
        if output:
            print "%s %% %s =? %s ..." %\
                (repr(formatstr), repr(value), repr(output)),
        else:
            print "%s %% %s works? ..." % (repr(formatstr), repr(value)),
    result = locale.format(formatstr, value, grouping = grouping)
    if output and result != output:
        if verbose:
            print 'no'
        print "%s %% %s == %s != %s" %\
              (repr(formatstr), repr(value), repr(result), repr(output))
    else:
        if verbose:
            print "yes"

try:
    testformat("%f", 1024, grouping=1, output='1,024.000000')
    testformat("%f", 102, grouping=1, output='102.000000')
    testformat("%f", -42, grouping=1, output='-42.000000')
    testformat("%+f", -42, grouping=1, output='-42.000000')
    testformat("%20.f", -42, grouping=1, output='                 -42')
    testformat("%+10.f", -4200, grouping=1, output='    -4,200')
    testformat("%-10.f", 4200, grouping=1, output='4,200     ')
finally:
    locale.setlocale(locale.LC_NUMERIC, oldlocale)
