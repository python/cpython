from test_support import verbose
import locale

oldlocale = locale.setlocale(locale.LC_NUMERIC)

try:
    locale.setlocale(locale.LC_NUMERIC, "en_US")
except locale.Error:
    raise ImportError, "test locale en_US not supported"

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
