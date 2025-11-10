import unittest
from test.support import (requires_docstrings, TestFailed)
import inspect
from . import (C, P, load_tests_for_base_classes,
               requires_cdecimal, skip_if_extra_functionality,
               setUpModule, tearDownModule)


@skip_if_extra_functionality
@requires_cdecimal
class CheckAttributes(unittest.TestCase):
    def test_module_attributes(self):
        # Architecture dependent context limits
        self.assertEqual(C.MAX_PREC, P.MAX_PREC)
        self.assertEqual(C.MAX_EMAX, P.MAX_EMAX)
        self.assertEqual(C.MIN_EMIN, P.MIN_EMIN)
        self.assertEqual(C.MIN_ETINY, P.MIN_ETINY)
        self.assertEqual(C.IEEE_CONTEXT_MAX_BITS, P.IEEE_CONTEXT_MAX_BITS)

        self.assertTrue(C.HAVE_THREADS is True or C.HAVE_THREADS is False)
        self.assertTrue(P.HAVE_THREADS is True or P.HAVE_THREADS is False)

        self.assertEqual(C.SPEC_VERSION, P.SPEC_VERSION)

        self.assertLessEqual(set(dir(C)), set(dir(P)))
        self.assertEqual([n for n in dir(C) if n[:2] != '__'], sorted(P.__all__))

    def test_context_attributes(self):
        x = [s for s in dir(C.Context()) if '__' in s or not s.startswith('_')]
        y = [s for s in dir(P.Context()) if '__' in s or not s.startswith('_')]
        self.assertEqual(set(x) - set(y), set())

    def test_decimal_attributes(self):
        x = [s for s in dir(C.Decimal(9)) if '__' in s or not s.startswith('_')]
        y = [s for s in dir(C.Decimal(9)) if '__' in s or not s.startswith('_')]
        self.assertEqual(set(x) - set(y), set())


@requires_docstrings
@requires_cdecimal
class SignatureTest(unittest.TestCase):
    """Function signatures"""

    def test_inspect_module(self):
        for attr in dir(P):
            if attr.startswith('_'):
                continue
            p_func = getattr(P, attr)
            c_func = getattr(C, attr)
            if (attr == 'Decimal' or attr == 'Context' or
                inspect.isfunction(p_func)):
                p_sig = inspect.signature(p_func)
                c_sig = inspect.signature(c_func)

                # parameter names:
                c_names = list(c_sig.parameters.keys())
                p_names = [x for x in p_sig.parameters.keys() if not
                           x.startswith('_')]

                self.assertEqual(c_names, p_names,
                                 msg="parameter name mismatch in %s" % p_func)

                c_kind = [x.kind for x in c_sig.parameters.values()]
                p_kind = [x[1].kind for x in p_sig.parameters.items() if not
                          x[0].startswith('_')]

                # parameters:
                if attr != 'setcontext':
                    self.assertEqual(c_kind, p_kind,
                                     msg="parameter kind mismatch in %s" % p_func)

    def test_inspect_types(self):
        POS = inspect._ParameterKind.POSITIONAL_ONLY
        POS_KWD = inspect._ParameterKind.POSITIONAL_OR_KEYWORD

        # Type heuristic (type annotations would help!):
        pdict = {C: {'other': C.Decimal(1),
                     'third': C.Decimal(1),
                     'x': C.Decimal(1),
                     'y': C.Decimal(1),
                     'z': C.Decimal(1),
                     'a': C.Decimal(1),
                     'b': C.Decimal(1),
                     'c': C.Decimal(1),
                     'exp': C.Decimal(1),
                     'modulo': C.Decimal(1),
                     'num': "1",
                     'f': 1.0,
                     'rounding': C.ROUND_HALF_UP,
                     'context': C.getcontext()},
                 P: {'other': P.Decimal(1),
                     'third': P.Decimal(1),
                     'a': P.Decimal(1),
                     'b': P.Decimal(1),
                     'c': P.Decimal(1),
                     'exp': P.Decimal(1),
                     'modulo': P.Decimal(1),
                     'num': "1",
                     'f': 1.0,
                     'rounding': P.ROUND_HALF_UP,
                     'context': P.getcontext()}}

        def mkargs(module, sig):
            args = []
            kwargs = {}
            for name, param in sig.parameters.items():
                if name == 'self': continue
                if param.kind == POS:
                    args.append(pdict[module][name])
                elif param.kind == POS_KWD:
                    kwargs[name] = pdict[module][name]
                else:
                    raise TestFailed("unexpected parameter kind")
            return args, kwargs

        def tr(s):
            """The C Context docstrings use 'x' in order to prevent confusion
               with the article 'a' in the descriptions."""
            if s == 'x': return 'a'
            if s == 'y': return 'b'
            if s == 'z': return 'c'
            return s

        def doit(ty):
            p_type = getattr(P, ty)
            c_type = getattr(C, ty)
            for attr in dir(p_type):
                if attr.startswith('_'):
                    continue
                p_func = getattr(p_type, attr)
                c_func = getattr(c_type, attr)
                if inspect.isfunction(p_func):
                    p_sig = inspect.signature(p_func)
                    c_sig = inspect.signature(c_func)

                    # parameter names:
                    p_names = list(p_sig.parameters.keys())
                    c_names = [tr(x) for x in c_sig.parameters.keys()]

                    self.assertEqual(c_names, p_names,
                                     msg="parameter name mismatch in %s" % p_func)

                    p_kind = [x.kind for x in p_sig.parameters.values()]
                    c_kind = [x.kind for x in c_sig.parameters.values()]

                    # 'self' parameter:
                    self.assertIs(p_kind[0], POS_KWD)
                    self.assertIs(c_kind[0], POS)

                    # remaining parameters:
                    if ty == 'Decimal':
                        self.assertEqual(c_kind[1:], p_kind[1:],
                                         msg="parameter kind mismatch in %s" % p_func)
                    else: # Context methods are positional only in the C version.
                        self.assertEqual(len(c_kind), len(p_kind),
                                         msg="parameter kind mismatch in %s" % p_func)

                    # Run the function:
                    args, kwds = mkargs(C, c_sig)
                    try:
                        getattr(c_type(9), attr)(*args, **kwds)
                    except Exception:
                        raise TestFailed("invalid signature for %s: %s %s" % (c_func, args, kwds))

                    args, kwds = mkargs(P, p_sig)
                    try:
                        getattr(p_type(9), attr)(*args, **kwds)
                    except Exception:
                        raise TestFailed("invalid signature for %s: %s %s" % (p_func, args, kwds))

        doit('Decimal')
        doit('Context')


class TestModule:
    def test_deprecated__version__(self):
        with self.assertWarnsRegex(
            DeprecationWarning,
            "'__version__' is deprecated and slated for removal in Python 3.20",
        ) as cm:
            getattr(self.decimal, "__version__")
        self.assertEqual(cm.filename, __file__)


def load_tests(loader, tests, pattern):
    return load_tests_for_base_classes(loader, tests, [TestModule])


if __name__ == '__main__':
    unittest.main()
