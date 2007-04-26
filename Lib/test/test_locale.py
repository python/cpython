from test.test_support import verbose, TestSkipped
import locale
import sys

if sys.platform == 'darwin':
    raise TestSkipped("Locale support on MacOSX is minimal and cannot be tested")
oldlocale = locale.setlocale(locale.LC_NUMERIC)

if sys.platform.startswith("win"):
    tlocs = ("En", "English")
else:
    tlocs = ("en_US.UTF-8", "en_US.US-ASCII", "en_US")

for tloc in tlocs:
    try:
        locale.setlocale(locale.LC_NUMERIC, tloc)
        break
    except locale.Error:
        continue
else:
    raise ImportError, "test locale not supported (tried %s)"%(', '.join(tlocs))

def testformat(formatstr, value, grouping = 0, output=None, func=locale.format):
    if verbose:
        if output:
            print "%s %% %s =? %s ..." %\
                (repr(formatstr), repr(value), repr(output)),
        else:
            print "%s %% %s works? ..." % (repr(formatstr), repr(value)),
    result = func(formatstr, value, grouping = grouping)
    if output and result != output:
        if verbose:
            print 'no'
        print "%s %% %s == %s != %s" %\
              (repr(formatstr), repr(value), repr(result), repr(output))
    else:
        if verbose:
            print "yes"

try:
    # On Solaris 10, the thousands_sep is the empty string
    sep = locale.localeconv()['thousands_sep']
    testformat("%f", 1024, grouping=1, output='1%s024.000000' % sep)
    testformat("%f", 102, grouping=1, output='102.000000')
    testformat("%f", -42, grouping=1, output='-42.000000')
    testformat("%+f", -42, grouping=1, output='-42.000000')
    testformat("%20.f", -42, grouping=1, output='                 -42')
    testformat("%+10.f", -4200, grouping=1, output='    -4%s200' % sep)
    testformat("%-10.f", 4200, grouping=1, output='4%s200     ' % sep)
    # Invoke getpreferredencoding to make sure it does not cause exceptions,
    locale.getpreferredencoding()

    # === Test format() with more complex formatting strings
    # test if grouping is independent from other characters in formatting string
    testformat("One million is %i", 1000000, grouping=1,
               output='One million is 1%s000%s000' % (sep, sep),
               func=locale.format_string)
    testformat("One  million is %i", 1000000, grouping=1,
               output='One  million is 1%s000%s000' % (sep, sep),
               func=locale.format_string)
    # test dots in formatting string
    testformat(".%f.", 1000.0, output='.1000.000000.', func=locale.format_string)
    # test floats
    testformat("--> %10.2f", 1000.0, grouping=1, output='-->   1%s000.00' % sep,
               func=locale.format_string)
    # test asterisk formats
    testformat("%10.*f", (2, 1000.0), grouping=0, output='   1000.00',
               func=locale.format_string)
    testformat("%*.*f", (10, 2, 1000.0), grouping=1, output='  1%s000.00' % sep,
               func=locale.format_string)
    # test more-in-one
    testformat("int %i float %.2f str %s", (1000, 1000.0, 'str'), grouping=1,
               output='int 1%s000 float 1%s000.00 str str' % (sep, sep),
               func=locale.format_string)

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
    if sys.platform == 'sunos5':
        # On Solaris, in en_US.UTF-8, \xa0 is a space
        raise locale.Error
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
