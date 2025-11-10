import unittest
import os
import sys
import random
from . import (C, P, Signals, OrderedSignals,
               setUpModule, tearDownModule)

TESTDATADIR = 'data'
if __name__ == '__main__':
    file = sys.argv[0]
else:
    file = __file__
testdir = os.path.dirname(file) or os.curdir
directory = testdir + os.sep + TESTDATADIR + os.sep

skip_expected = not os.path.isdir(directory)

# Make sure it actually raises errors when not expected and caught in flags
# Slower, since it runs some things several times.
EXTENDEDERRORTEST = False


class IBMTestCases:
    """Class which tests the Decimal class against the IBM test cases."""

    def setUp(self):
        self.context = self.decimal.Context()
        self.readcontext = self.decimal.Context()
        self.ignore_list = ['#']

        # List of individual .decTest test ids that correspond to tests that
        # we're skipping for one reason or another.
        self.skipped_test_ids = set([
            # Skip implementation-specific scaleb tests.
            'scbx164',
            'scbx165',

            # For some operations (currently exp, ln, log10, power), the decNumber
            # reference implementation imposes additional restrictions on the context
            # and operands.  These restrictions are not part of the specification;
            # however, the effect of these restrictions does show up in some of the
            # testcases.  We skip testcases that violate these restrictions, since
            # Decimal behaves differently from decNumber for these testcases so these
            # testcases would otherwise fail.
            'expx901',
            'expx902',
            'expx903',
            'expx905',
            'lnx901',
            'lnx902',
            'lnx903',
            'lnx905',
            'logx901',
            'logx902',
            'logx903',
            'logx905',
            'powx1183',
            'powx1184',
            'powx4001',
            'powx4002',
            'powx4003',
            'powx4005',
            'powx4008',
            'powx4010',
            'powx4012',
            'powx4014',
            ])

        if self.decimal == C:
            # status has additional Subnormal, Underflow
            self.skipped_test_ids.add('pwsx803')
            self.skipped_test_ids.add('pwsx805')
            # Correct rounding (skipped for decNumber, too)
            self.skipped_test_ids.add('powx4302')
            self.skipped_test_ids.add('powx4303')
            self.skipped_test_ids.add('powx4342')
            self.skipped_test_ids.add('powx4343')
            # http://bugs.python.org/issue7049
            self.skipped_test_ids.add('pwmx325')
            self.skipped_test_ids.add('pwmx326')

        # Map test directives to setter functions.
        self.ChangeDict = {'precision' : self.change_precision,
                           'rounding' : self.change_rounding_method,
                           'maxexponent' : self.change_max_exponent,
                           'minexponent' : self.change_min_exponent,
                           'clamp' : self.change_clamp}

        # Name adapter to be able to change the Decimal and Context
        # interface without changing the test files from Cowlishaw.
        self.NameAdapter = {'and':'logical_and',
                            'apply':'_apply',
                            'class':'number_class',
                            'comparesig':'compare_signal',
                            'comparetotal':'compare_total',
                            'comparetotmag':'compare_total_mag',
                            'copy':'copy_decimal',
                            'copyabs':'copy_abs',
                            'copynegate':'copy_negate',
                            'copysign':'copy_sign',
                            'divideint':'divide_int',
                            'invert':'logical_invert',
                            'iscanonical':'is_canonical',
                            'isfinite':'is_finite',
                            'isinfinite':'is_infinite',
                            'isnan':'is_nan',
                            'isnormal':'is_normal',
                            'isqnan':'is_qnan',
                            'issigned':'is_signed',
                            'issnan':'is_snan',
                            'issubnormal':'is_subnormal',
                            'iszero':'is_zero',
                            'maxmag':'max_mag',
                            'minmag':'min_mag',
                            'nextminus':'next_minus',
                            'nextplus':'next_plus',
                            'nexttoward':'next_toward',
                            'or':'logical_or',
                            'reduce':'normalize',
                            'remaindernear':'remainder_near',
                            'samequantum':'same_quantum',
                            'squareroot':'sqrt',
                            'toeng':'to_eng_string',
                            'tointegral':'to_integral_value',
                            'tointegralx':'to_integral_exact',
                            'tosci':'to_sci_string',
                            'xor':'logical_xor'}

        # Map test-case names to roundings.
        self.RoundingDict = {'ceiling' :P.ROUND_CEILING,
                             'down' :P.ROUND_DOWN,
                             'floor' :P.ROUND_FLOOR,
                             'half_down' :P.ROUND_HALF_DOWN,
                             'half_even' :P.ROUND_HALF_EVEN,
                             'half_up' :P.ROUND_HALF_UP,
                             'up' :P.ROUND_UP,
                             '05up' :P.ROUND_05UP}

        # Map the test cases' error names to the actual errors.
        self.ErrorNames = {'clamped' : self.decimal.Clamped,
                           'conversion_syntax' : self.decimal.InvalidOperation,
                           'division_by_zero' : self.decimal.DivisionByZero,
                           'division_impossible' : self.decimal.InvalidOperation,
                           'division_undefined' : self.decimal.InvalidOperation,
                           'inexact' : self.decimal.Inexact,
                           'invalid_context' : self.decimal.InvalidOperation,
                           'invalid_operation' : self.decimal.InvalidOperation,
                           'overflow' : self.decimal.Overflow,
                           'rounded' : self.decimal.Rounded,
                           'subnormal' : self.decimal.Subnormal,
                           'underflow' : self.decimal.Underflow}

        # The following functions return True/False rather than a
        # Decimal instance.
        self.LogicalFunctions = ('is_canonical',
                                 'is_finite',
                                 'is_infinite',
                                 'is_nan',
                                 'is_normal',
                                 'is_qnan',
                                 'is_signed',
                                 'is_snan',
                                 'is_subnormal',
                                 'is_zero',
                                 'same_quantum')

    def read_unlimited(self, v, context):
        """Work around the limitations of the 32-bit _decimal version. The
           guaranteed maximum values for prec, Emax etc. are 425000000,
           but higher values usually work, except for rare corner cases.
           In particular, all of the IBM tests pass with maximum values
           of 1070000000."""
        if self.decimal == C and self.decimal.MAX_EMAX == 425000000:
            self.readcontext._unsafe_setprec(1070000000)
            self.readcontext._unsafe_setemax(1070000000)
            self.readcontext._unsafe_setemin(-1070000000)
            return self.readcontext.create_decimal(v)
        else:
            return self.decimal.Decimal(v, context)

    def eval_file(self, file):
        global skip_expected
        if skip_expected:
            raise unittest.SkipTest
        with open(file, encoding="utf-8") as f:
            for line in f:
                line = line.replace('\r\n', '').replace('\n', '')
                #print line
                try:
                    t = self.eval_line(line)
                except self.decimal.DecimalException as exception:
                    #Exception raised where there shouldn't have been one.
                    self.fail('Exception "'+exception.__class__.__name__ + '" raised on line '+line)


    def eval_line(self, s):
        if s.find(' -> ') >= 0 and s[:2] != '--' and not s.startswith('  --'):
            s = (s.split('->')[0] + '->' +
                 s.split('->')[1].split('--')[0]).strip()
        else:
            s = s.split('--')[0].strip()

        for ignore in self.ignore_list:
            if s.find(ignore) >= 0:
                #print s.split()[0], 'NotImplemented--', ignore
                return
        if not s:
            return
        elif ':' in s:
            return self.eval_directive(s)
        else:
            return self.eval_equation(s)

    def eval_directive(self, s):
        funct, value = (x.strip().lower() for x in s.split(':'))
        if funct == 'rounding':
            value = self.RoundingDict[value]
        else:
            try:
                value = int(value)
            except ValueError:
                pass

        funct = self.ChangeDict.get(funct, (lambda *args: None))
        funct(value)

    def eval_equation(self, s):
        if not TEST_ALL and random.random() < 0.90:
            return

        self.context.clear_flags()

        try:
            Sides = s.split('->')
            L = Sides[0].strip().split()
            id = L[0]
            if DEBUG:
                print("Test ", id, end=" ")
            funct = L[1].lower()
            valstemp = L[2:]
            L = Sides[1].strip().split()
            ans = L[0]
            exceptions = L[1:]
        except (TypeError, AttributeError, IndexError):
            raise self.decimal.InvalidOperation
        def FixQuotes(val):
            val = val.replace("''", 'SingleQuote').replace('""', 'DoubleQuote')
            val = val.replace("'", '').replace('"', '')
            val = val.replace('SingleQuote', "'").replace('DoubleQuote', '"')
            return val

        if id in self.skipped_test_ids:
            return

        fname = self.NameAdapter.get(funct, funct)
        if fname == 'rescale':
            return
        funct = getattr(self.context, fname)
        vals = []
        conglomerate = ''
        quote = 0
        theirexceptions = [self.ErrorNames[x.lower()] for x in exceptions]

        for exception in Signals[self.decimal]:
            self.context.traps[exception] = 1 #Catch these bugs...
        for exception in theirexceptions:
            self.context.traps[exception] = 0
        for i, val in enumerate(valstemp):
            if val.count("'") % 2 == 1:
                quote = 1 - quote
            if quote:
                conglomerate = conglomerate + ' ' + val
                continue
            else:
                val = conglomerate + val
                conglomerate = ''
            v = FixQuotes(val)
            if fname in ('to_sci_string', 'to_eng_string'):
                if EXTENDEDERRORTEST:
                    for error in theirexceptions:
                        self.context.traps[error] = 1
                        try:
                            funct(self.context.create_decimal(v))
                        except error:
                            pass
                        except Signals[self.decimal] as e:
                            self.fail("Raised %s in %s when %s disabled" % \
                                      (e, s, error))
                        else:
                            self.fail("Did not raise %s in %s" % (error, s))
                        self.context.traps[error] = 0
                v = self.context.create_decimal(v)
            else:
                v = self.read_unlimited(v, self.context)
            vals.append(v)

        ans = FixQuotes(ans)

        if EXTENDEDERRORTEST and fname not in ('to_sci_string', 'to_eng_string'):
            for error in theirexceptions:
                self.context.traps[error] = 1
                try:
                    funct(*vals)
                except error:
                    pass
                except Signals[self.decimal] as e:
                    self.fail("Raised %s in %s when %s disabled" % \
                              (e, s, error))
                else:
                    self.fail("Did not raise %s in %s" % (error, s))
                self.context.traps[error] = 0

            # as above, but add traps cumulatively, to check precedence
            ordered_errors = [e for e in OrderedSignals[self.decimal] if e in theirexceptions]
            for error in ordered_errors:
                self.context.traps[error] = 1
                try:
                    funct(*vals)
                except error:
                    pass
                except Signals[self.decimal] as e:
                    self.fail("Raised %s in %s; expected %s" %
                              (type(e), s, error))
                else:
                    self.fail("Did not raise %s in %s" % (error, s))
            # reset traps
            for error in ordered_errors:
                self.context.traps[error] = 0


        if DEBUG:
            print("--", self.context)
        try:
            result = str(funct(*vals))
            if fname in self.LogicalFunctions:
                result = str(int(eval(result))) # 'True', 'False' -> '1', '0'
        except Signals[self.decimal] as error:
            self.fail("Raised %s in %s" % (error, s))
        except: #Catch any error long enough to state the test case.
            print("ERROR:", s)
            raise

        myexceptions = self.getexceptions()

        myexceptions.sort(key=repr)
        theirexceptions.sort(key=repr)

        self.assertEqual(result, ans,
                         'Incorrect answer for ' + s + ' -- got ' + result)

        self.assertEqual(myexceptions, theirexceptions,
              'Incorrect flags set in ' + s + ' -- got ' + str(myexceptions))

    def getexceptions(self):
        return [e for e in Signals[self.decimal] if self.context.flags[e]]

    def change_precision(self, prec):
        if self.decimal == C and self.decimal.MAX_PREC == 425000000:
            self.context._unsafe_setprec(prec)
        else:
            self.context.prec = prec

    def change_rounding_method(self, rounding):
        self.context.rounding = rounding

    def change_min_exponent(self, exp):
        if self.decimal == C and self.decimal.MAX_PREC == 425000000:
            self.context._unsafe_setemin(exp)
        else:
            self.context.Emin = exp

    def change_max_exponent(self, exp):
        if self.decimal == C and self.decimal.MAX_PREC == 425000000:
            self.context._unsafe_setemax(exp)
        else:
            self.context.Emax = exp

    def change_clamp(self, clamp):
        self.context.clamp = clamp


def load_tests(loader, tests, pattern):
    # Dynamically build custom test definition for each file in the test
    # directory and add the definitions to the DecimalTest class.  This
    # procedure insures that new files do not get skipped.
    for filename in os.listdir(directory):
        if '.decTest' not in filename or filename.startswith("."):
            continue
        head, tail = filename.split('.')
        if TODO_TESTS is not None and head not in TODO_TESTS:
            continue
        tester = lambda self, f=filename: self.eval_file(directory + f)
        setattr(IBMTestCases, 'test_' + head, tester)
        del filename, head, tail, tester

    for prefix, mod in ('C', C), ('Py', P):
        if not mod:
            continue
        test_class = type(prefix + 'IBMTestCases',
                          (IBMTestCases, unittest.TestCase),
                          {'decimal': mod})
        tests.addTest(loader.loadTestsFromTestCase(test_class))

    return tests


TEST_ALL = True
TODO_TESTS = None
DEBUG = False


if __name__ == '__main__':
    unittest.main()
