from test.test_support import verbose, TestSkipped
from test import string_tests
import string, sys

# XXX: kludge... short circuit if strings don't have methods
try:
    ''.join
except AttributeError:
    raise TestSkipped

def test(name, input, output, *args):
    if verbose:
        print 'string.%s%s =? %s... ' % (name, (input,) + args, output),
    try:
        # Prefer string methods over string module functions
        try:
            f = getattr(input, name)
            value = apply(f, args)
        except AttributeError:
            f = getattr(string, name)
            value = apply(f, (input,) + args)
    except:
        value = sys.exc_type
        f = name
    if value == output:
        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if value is input:
            class ssub(str):
                def __repr__(self):
                    return 'ssub(%r)' % str.__repr__(self)
            input = ssub(input)
            try:
                f = getattr(input, name)
                value = apply(f, args)
            except AttributeError:
                f = getattr(string, name)
                value = apply(f, (input,) + args)
            if value is input:
                if verbose:
                    print 'no'
                print '*',f, `input`, `output`, `value`
                return
    if value != output:
        if verbose:
            print 'no'
        print f, `input`, `output`, `value`
    else:
        if verbose:
            print 'yes'

string_tests.run_module_tests(test)
string_tests.run_method_tests(test)
string_tests.run_contains_tests(test)
string_tests.run_inplace_tests(str)

string.whitespace
string.lowercase
string.uppercase

# Float formatting
for prec in range(100):
    formatstring = '%%.%if' % prec
    value = 0.01
    for x in range(60):
        value = value * 3.141592655 / 3.0 * 10.0
        #print 'Overflow check for x=%i and prec=%i:' % \
        #      (x, prec),
        try:
            result = formatstring % value
        except OverflowError:
            # The formatfloat() code in stringobject.c and
            # unicodeobject.c uses a 120 byte buffer and switches from
            # 'f' formatting to 'g' at precision 50, so we expect
            # OverflowErrors for the ranges x < 50 and prec >= 67.
            if x >= 50 or \
               prec < 67:
                print '*** unexpected OverflowError for x=%i and prec=%i' % (x, prec)
            else:
                #print 'OverflowError'
                pass
        else:
            #print result
            pass
    
