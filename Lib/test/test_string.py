from test_support import verbose, TestSkipped
import string_tests
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
    if value != output:
        if verbose:
            print 'no'
        print f, `input`, `output`, `value`
    else:
        if verbose:
            print 'yes'

string_tests.run_module_tests(test)
string_tests.run_method_tests(test)

string.whitespace
string.lowercase
string.uppercase

