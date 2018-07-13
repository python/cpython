"Test calltip, coverage 60%"

from idlelib import calltip
import unittest
import textwrap
import types

default_tip = calltip._default_callable_argspec


# Test Class TC is used in multiple get_argspec test methods
class TC():
    'doc'
    tip = "(ai=None, *b)"
    def __init__(self, ai=None, *b): 'doc'
    __init__.tip = "(self, ai=None, *b)"
    def t1(self): 'doc'
    t1.tip = "(self)"
    def t2(self, ai, b=None): 'doc'
    t2.tip = "(self, ai, b=None)"
    def t3(self, ai, *args): 'doc'
    t3.tip = "(self, ai, *args)"
    def t4(self, *args): 'doc'
    t4.tip = "(self, *args)"
    def t5(self, ai, b=None, *args, **kw): 'doc'
    t5.tip = "(self, ai, b=None, *args, **kw)"
    def t6(no, self): 'doc'
    t6.tip = "(no, self)"
    def __call__(self, ci): 'doc'
    __call__.tip = "(self, ci)"
    # attaching .tip to wrapped methods does not work
    @classmethod
    def cm(cls, a): 'doc'
    @staticmethod
    def sm(b): 'doc'


tc = TC()
signature = calltip.get_argspec  # 2.7 and 3.x use different functions


class Get_signatureTest(unittest.TestCase):
    # The signature function must return a string, even if blank.
    # Test a variety of objects to be sure that none cause it to raise
    # (quite aside from getting as correct an answer as possible).
    # The tests of builtins may break if inspect or the docstrings change,
    # but a red buildbot is better than a user crash (as has happened).
    # For a simple mismatch, change the expected output to the actual.

    def test_builtins(self):

        # Python class that inherits builtin methods
        class List(list): "List() doc"

        # Simulate builtin with no docstring for default tip test
        class SB:  __call__ = None

        def gtest(obj, out):
            self.assertEqual(signature(obj), out)

        if List.__doc__ is not None:
            gtest(List, '(iterable=(), /)' + calltip._argument_positional
                  + '\n' + List.__doc__)
        gtest(list.__new__,
              '(*args, **kwargs)\n'
              'Create and return a new object.  '
              'See help(type) for accurate signature.')
        gtest(list.__init__,
              '(self, /, *args, **kwargs)'
              + calltip._argument_positional + '\n' +
              'Initialize self.  See help(type(self)) for accurate signature.')
        append_doc = (calltip._argument_positional
                      + "\nAppend object to the end of the list.")
        gtest(list.append, '(self, object, /)' + append_doc)
        gtest(List.append, '(self, object, /)' + append_doc)
        gtest([].append, '(object, /)' + append_doc)

        gtest(types.MethodType, "method(function, instance)")
        gtest(SB(), default_tip)
        import re
        p = re.compile('')
        gtest(re.sub, '''\
(pattern, repl, string, count=0, flags=0)
Return the string obtained by replacing the leftmost
non-overlapping occurrences of the pattern in string by the
replacement repl.  repl can be either a string or a callable;
if a string, backslash escapes in it are processed.  If it is
a callable, it's passed the Match object and must return''')
        gtest(p.sub, '''\
(repl, string, count=0)
Return the string obtained by replacing the leftmost \
non-overlapping occurrences o...''')

    def test_signature_wrap(self):
        if textwrap.TextWrapper.__doc__ is not None:
            self.assertEqual(signature(textwrap.TextWrapper), '''\
(width=70, initial_indent='', subsequent_indent='', expand_tabs=True,
    replace_whitespace=True, fix_sentence_endings=False, break_long_words=True,
    drop_whitespace=True, break_on_hyphens=True, tabsize=8, *, max_lines=None,
    placeholder=' [...]')''')

    def test_docline_truncation(self):
        def f(): pass
        f.__doc__ = 'a'*300
        self.assertEqual(signature(f), '()\n' + 'a' * (calltip._MAX_COLS-3) + '...')

    def test_multiline_docstring(self):
        # Test fewer lines than max.
        self.assertEqual(signature(range),
                "range(stop) -> range object\n"
                "range(start, stop[, step]) -> range object")

        # Test max lines
        self.assertEqual(signature(bytes), '''\
bytes(iterable_of_ints) -> bytes
bytes(string, encoding[, errors]) -> bytes
bytes(bytes_or_buffer) -> immutable copy of bytes_or_buffer
bytes(int) -> bytes object of size given by the parameter initialized with null bytes
bytes() -> empty bytes object''')

        # Test more than max lines
        def f(): pass
        f.__doc__ = 'a\n' * 15
        self.assertEqual(signature(f), '()' + '\na' * calltip._MAX_LINES)

    def test_functions(self):
        def t1(): 'doc'
        t1.tip = "()"
        def t2(a, b=None): 'doc'
        t2.tip = "(a, b=None)"
        def t3(a, *args): 'doc'
        t3.tip = "(a, *args)"
        def t4(*args): 'doc'
        t4.tip = "(*args)"
        def t5(a, b=None, *args, **kw): 'doc'
        t5.tip = "(a, b=None, *args, **kw)"

        doc = '\ndoc' if t1.__doc__ is not None else ''
        for func in (t1, t2, t3, t4, t5, TC):
            self.assertEqual(signature(func), func.tip + doc)

    def test_methods(self):
        doc = '\ndoc' if TC.__doc__ is not None else ''
        for meth in (TC.t1, TC.t2, TC.t3, TC.t4, TC.t5, TC.t6, TC.__call__):
            self.assertEqual(signature(meth), meth.tip + doc)
        self.assertEqual(signature(TC.cm), "(a)" + doc)
        self.assertEqual(signature(TC.sm), "(b)" + doc)

    def test_bound_methods(self):
        # test that first parameter is correctly removed from argspec
        doc = '\ndoc' if TC.__doc__ is not None else ''
        for meth, mtip  in ((tc.t1, "()"), (tc.t4, "(*args)"),
                            (tc.t6, "(self)"), (tc.__call__, '(ci)'),
                            (tc, '(ci)'), (TC.cm, "(a)"),):
            self.assertEqual(signature(meth), mtip + doc)

    def test_starred_parameter(self):
        # test that starred first parameter is *not* removed from argspec
        class C:
            def m1(*args): pass
        c = C()
        for meth, mtip  in ((C.m1, '(*args)'), (c.m1, "(*args)"),):
            self.assertEqual(signature(meth), mtip)

    def test_invalid_method_signature(self):
        class C:
            def m2(**kwargs): pass
        class Test:
            def __call__(*, a): pass

        mtip = calltip._invalid_method
        self.assertEqual(signature(C().m2), mtip)
        self.assertEqual(signature(Test()), mtip)

    def test_non_ascii_name(self):
        # test that re works to delete a first parameter name that
        # includes non-ascii chars, such as various forms of A.
        uni = "(A\u0391\u0410\u05d0\u0627\u0905\u1e00\u3042, a)"
        assert calltip._first_param.sub('', uni) == '(a)'

    def test_no_docstring(self):
        def nd(s):
            pass
        TC.nd = nd
        self.assertEqual(signature(nd), "(s)")
        self.assertEqual(signature(TC.nd), "(s)")
        self.assertEqual(signature(tc.nd), "()")

    def test_attribute_exception(self):
        class NoCall:
            def __getattr__(self, name):
                raise BaseException
        class CallA(NoCall):
            def __call__(oui, a, b, c):
                pass
        class CallB(NoCall):
            def __call__(self, ci):
                pass

        for meth, mtip  in ((NoCall, default_tip), (CallA, default_tip),
                            (NoCall(), ''), (CallA(), '(a, b, c)'),
                            (CallB(), '(ci)')):
            self.assertEqual(signature(meth), mtip)

    def test_non_callables(self):
        for obj in (0, 0.0, '0', b'0', [], {}):
            self.assertEqual(signature(obj), '')


class Get_entityTest(unittest.TestCase):
    def test_bad_entity(self):
        self.assertIsNone(calltip.get_entity('1/0'))
    def test_good_entity(self):
        self.assertIs(calltip.get_entity('int'), int)


if __name__ == '__main__':
    unittest.main(verbosity=2)
