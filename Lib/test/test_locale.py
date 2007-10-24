from test.test_support import verbose, TestSkipped
import locale
import sys

if sys.platform == 'darwin':
    raise TestSkipped(
            "Locale support on MacOSX is minimal and cannot be tested")
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
    raise ImportError(
            "test locale not supported (tried %s)" % (', '.join(tlocs)))

def testformat(formatstr, value, grouping = 0, output=None, func=locale.format):
    if verbose:
        if output:
            print("%s %% %s =? %s ..." %
                  (repr(formatstr), repr(value), repr(output)), end=' ')
        else:
            print("%s %% %s works? ..." % (repr(formatstr), repr(value)),
                  end=' ')
    result = func(formatstr, value, grouping = grouping)
    if output and result != output:
        if verbose:
            print('no')
        print("%s %% %s == %s != %s" %
              (repr(formatstr), repr(value), repr(result), repr(output)))
    else:
        if verbose:
            print("yes")

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
