from test.test_support import verbose, TestSkipped
import locale
import sys

if sys.platform == 'darwin':
    raise TestSkipped("Locale support on MacOSX is minimal and cannot be tested")
oldlocale = locale.setlocale(locale.LC_NUMERIC)

if sys.platform.startswith("win"):
    tloc = "en"
elif sys.platform.startswith("freebsd"):
    tloc = "en_US.US-ASCII"
else:
    tloc = "en_US"

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
    # Invoke getpreferredencoding to make sure it does not cause exceptions,
    locale.getpreferredencoding()
finally:
    locale.setlocale(locale.LC_NUMERIC, oldlocale)


# Test BSD Rune locale's bug for isctype functions.
def teststrop(s, method, output):
    if verbose:
        print "%s.%s() =? %s ..." % (repr(s), method, repr(output)),
    result = getattr(s, method)()
    if result != output:
        if verbose:
            print "no"
        print "%s.%s() == %s != %s" % (repr(s), method, repr(result),
                                       repr(output))
    elif verbose:
        print "yes"

try:
    oldlocale = locale.setlocale(locale.LC_CTYPE)
    locale.setlocale(locale.LC_CTYPE, 'en_US.UTF-8')
except locale.Error:
    pass
else:
    try:
        teststrop('\x20', 'isspace', True)
        teststrop('\xa0', 'isspace', False)
        teststrop('\xa1', 'isspace', False)
        teststrop('\xc0', 'isalpha', False)
        teststrop('\xc0', 'isalnum', False)
        teststrop('\xc0', 'isupper', False)
        teststrop('\xc0', 'islower', False)
        teststrop('\xec\xa0\xbc', 'split', ['\xec\xa0\xbc'])
        teststrop('\xed\x95\xa0', 'strip', '\xed\x95\xa0')
        teststrop('\xcc\x85', 'lower', '\xcc\x85')
        teststrop('\xed\x95\xa0', 'upper', '\xed\x95\xa0')
    finally:
        locale.setlocale(locale.LC_CTYPE, oldlocale)
