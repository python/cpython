#!/usr/bin/env python2.5
""" Test suite for the fixer modules """
# Author: Collin Winter

# Testing imports
try:
    from tests import support
except ImportError:
    import support

# Python imports
import unittest
from os.path import dirname, pathsep

# Local imports
from .. import pygram
from .. import pytree
from .. import refactor

class Options:
    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)

        self.verbose = False

class FixerTestCase(support.TestCase):
    def setUp(self):
        options = Options(fix=[self.fixer], print_function=False)
        self.refactor = refactor.RefactoringTool(options)
        self.fixer_log = []
        self.filename = "<string>"

        for order in (self.refactor.pre_order, self.refactor.post_order):
            for fixer in order:
                fixer.log = self.fixer_log

    def _check(self, before, after):
        before = support.reformat(before)
        after = support.reformat(after)
        tree = self.refactor.refactor_string(before, self.filename)
        self.failUnlessEqual(after, str(tree))
        return tree

    def check(self, before, after, ignore_warnings=False):
        tree = self._check(before, after)
        self.failUnless(tree.was_changed)
        if not ignore_warnings:
            self.failUnlessEqual(self.fixer_log, [])

    def warns(self, before, after, message, unchanged=False):
        tree = self._check(before, after)
        self.failUnless(message in "".join(self.fixer_log))
        if not unchanged:
            self.failUnless(tree.was_changed)

    def warns_unchanged(self, before, message):
        self.warns(before, before, message, unchanged=True)

    def unchanged(self, before, ignore_warnings=False):
        self._check(before, before)
        if not ignore_warnings:
            self.failUnlessEqual(self.fixer_log, [])

    def assert_runs_after(self, *names):
        fix = [self.fixer]
        fix.extend(names)
        options = Options(fix=fix, print_function=False)
        r = refactor.RefactoringTool(options)
        (pre, post) = r.get_fixers()
        n = "fix_" + self.fixer
        if post and post[-1].__class__.__module__.endswith(n):
            # We're the last fixer to run
            return
        if pre and pre[-1].__class__.__module__.endswith(n) and not post:
            # We're the last in pre and post is empty
            return
        self.fail("Fixer run order (%s) is incorrect; %s should be last."\
               %(", ".join([x.__class__.__module__ for x in (pre+post)]), n))

class Test_ne(FixerTestCase):
    fixer = "ne"

    def test_basic(self):
        b = """if x <> y:
            pass"""

        a = """if x != y:
            pass"""
        self.check(b, a)

    def test_no_spaces(self):
        b = """if x<>y:
            pass"""

        a = """if x!=y:
            pass"""
        self.check(b, a)

    def test_chained(self):
        b = """if x<>y<>z:
            pass"""

        a = """if x!=y!=z:
            pass"""
        self.check(b, a)

class Test_has_key(FixerTestCase):
    fixer = "has_key"

    def test_1(self):
        b = """x = d.has_key("x") or d.has_key("y")"""
        a = """x = "x" in d or "y" in d"""
        self.check(b, a)

    def test_2(self):
        b = """x = a.b.c.d.has_key("x") ** 3"""
        a = """x = ("x" in a.b.c.d) ** 3"""
        self.check(b, a)

    def test_3(self):
        b = """x = a.b.has_key(1 + 2).__repr__()"""
        a = """x = (1 + 2 in a.b).__repr__()"""
        self.check(b, a)

    def test_4(self):
        b = """x = a.b.has_key(1 + 2).__repr__() ** -3 ** 4"""
        a = """x = (1 + 2 in a.b).__repr__() ** -3 ** 4"""
        self.check(b, a)

    def test_5(self):
        b = """x = a.has_key(f or g)"""
        a = """x = (f or g) in a"""
        self.check(b, a)

    def test_6(self):
        b = """x = a + b.has_key(c)"""
        a = """x = a + (c in b)"""
        self.check(b, a)

    def test_7(self):
        b = """x = a.has_key(lambda: 12)"""
        a = """x = (lambda: 12) in a"""
        self.check(b, a)

    def test_8(self):
        b = """x = a.has_key(a for a in b)"""
        a = """x = (a for a in b) in a"""
        self.check(b, a)

    def test_9(self):
        b = """if not a.has_key(b): pass"""
        a = """if b not in a: pass"""
        self.check(b, a)

    def test_10(self):
        b = """if not a.has_key(b).__repr__(): pass"""
        a = """if not (b in a).__repr__(): pass"""
        self.check(b, a)

    def test_11(self):
        b = """if not a.has_key(b) ** 2: pass"""
        a = """if not (b in a) ** 2: pass"""
        self.check(b, a)

class Test_apply(FixerTestCase):
    fixer = "apply"

    def test_1(self):
        b = """x = apply(f, g + h)"""
        a = """x = f(*g + h)"""
        self.check(b, a)

    def test_2(self):
        b = """y = apply(f, g, h)"""
        a = """y = f(*g, **h)"""
        self.check(b, a)

    def test_3(self):
        b = """z = apply(fs[0], g or h, h or g)"""
        a = """z = fs[0](*g or h, **h or g)"""
        self.check(b, a)

    def test_4(self):
        b = """apply(f, (x, y) + t)"""
        a = """f(*(x, y) + t)"""
        self.check(b, a)

    def test_5(self):
        b = """apply(f, args,)"""
        a = """f(*args)"""
        self.check(b, a)

    def test_6(self):
        b = """apply(f, args, kwds,)"""
        a = """f(*args, **kwds)"""
        self.check(b, a)

    # Test that complex functions are parenthesized

    def test_complex_1(self):
        b = """x = apply(f+g, args)"""
        a = """x = (f+g)(*args)"""
        self.check(b, a)

    def test_complex_2(self):
        b = """x = apply(f*g, args)"""
        a = """x = (f*g)(*args)"""
        self.check(b, a)

    def test_complex_3(self):
        b = """x = apply(f**g, args)"""
        a = """x = (f**g)(*args)"""
        self.check(b, a)

    # But dotted names etc. not

    def test_dotted_name(self):
        b = """x = apply(f.g, args)"""
        a = """x = f.g(*args)"""
        self.check(b, a)

    def test_subscript(self):
        b = """x = apply(f[x], args)"""
        a = """x = f[x](*args)"""
        self.check(b, a)

    def test_call(self):
        b = """x = apply(f(), args)"""
        a = """x = f()(*args)"""
        self.check(b, a)

    # Extreme case
    def test_extreme(self):
        b = """x = apply(a.b.c.d.e.f, args, kwds)"""
        a = """x = a.b.c.d.e.f(*args, **kwds)"""
        self.check(b, a)

    # XXX Comments in weird places still get lost
    def test_weird_comments(self):
        b = """apply(   # foo
          f, # bar
          args)"""
        a = """f(*args)"""
        self.check(b, a)

    # These should *not* be touched

    def test_unchanged_1(self):
        s = """apply()"""
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """apply(f)"""
        self.unchanged(s)

    def test_unchanged_3(self):
        s = """apply(f,)"""
        self.unchanged(s)

    def test_unchanged_4(self):
        s = """apply(f, args, kwds, extras)"""
        self.unchanged(s)

    def test_unchanged_5(self):
        s = """apply(f, *args, **kwds)"""
        self.unchanged(s)

    def test_unchanged_6(self):
        s = """apply(f, *args)"""
        self.unchanged(s)

    def test_unchanged_7(self):
        s = """apply(func=f, args=args, kwds=kwds)"""
        self.unchanged(s)

    def test_unchanged_8(self):
        s = """apply(f, args=args, kwds=kwds)"""
        self.unchanged(s)

    def test_unchanged_9(self):
        s = """apply(f, args, kwds=kwds)"""
        self.unchanged(s)

    def test_space_1(self):
        a = """apply(  f,  args,   kwds)"""
        b = """f(*args, **kwds)"""
        self.check(a, b)

    def test_space_2(self):
        a = """apply(  f  ,args,kwds   )"""
        b = """f(*args, **kwds)"""
        self.check(a, b)

class Test_intern(FixerTestCase):
    fixer = "intern"

    def test_prefix_preservation(self):
        b = """x =   intern(  a  )"""
        a = """x =   sys.intern(  a  )"""
        self.check(b, a)

        b = """y = intern("b" # test
              )"""
        a = """y = sys.intern("b" # test
              )"""
        self.check(b, a)

        b = """z = intern(a+b+c.d,   )"""
        a = """z = sys.intern(a+b+c.d,   )"""
        self.check(b, a)

    def test(self):
        b = """x = intern(a)"""
        a = """x = sys.intern(a)"""
        self.check(b, a)

        b = """z = intern(a+b+c.d,)"""
        a = """z = sys.intern(a+b+c.d,)"""
        self.check(b, a)

        b = """intern("y%s" % 5).replace("y", "")"""
        a = """sys.intern("y%s" % 5).replace("y", "")"""
        self.check(b, a)

    # These should not be refactored

    def test_unchanged(self):
        s = """intern(a=1)"""
        self.unchanged(s)

        s = """intern(f, g)"""
        self.unchanged(s)

        s = """intern(*h)"""
        self.unchanged(s)

        s = """intern(**i)"""
        self.unchanged(s)

        s = """intern()"""
        self.unchanged(s)

class Test_print(FixerTestCase):
    fixer = "print"

    def test_prefix_preservation(self):
        b = """print 1,   1+1,   1+1+1"""
        a = """print(1,   1+1,   1+1+1)"""
        self.check(b, a)

    def test_idempotency(self):
        s = """print()"""
        self.unchanged(s)

        s = """print('')"""
        self.unchanged(s)

    def test_idempotency_print_as_function(self):
        print_stmt = pygram.python_grammar.keywords.pop("print")
        try:
            s = """print(1, 1+1, 1+1+1)"""
            self.unchanged(s)

            s = """print()"""
            self.unchanged(s)

            s = """print('')"""
            self.unchanged(s)
        finally:
            pygram.python_grammar.keywords["print"] = print_stmt

    def test_1(self):
        b = """print 1, 1+1, 1+1+1"""
        a = """print(1, 1+1, 1+1+1)"""
        self.check(b, a)

    def test_2(self):
        b = """print 1, 2"""
        a = """print(1, 2)"""
        self.check(b, a)

    def test_3(self):
        b = """print"""
        a = """print()"""
        self.check(b, a)

    def test_tuple(self):
        b = """print (a, b, c)"""
        a = """print((a, b, c))"""
        self.check(b, a)

    # trailing commas

    def test_trailing_comma_1(self):
        b = """print 1, 2, 3,"""
        a = """print(1, 2, 3, end=' ')"""
        self.check(b, a)

    def test_trailing_comma_2(self):
        b = """print 1, 2,"""
        a = """print(1, 2, end=' ')"""
        self.check(b, a)

    def test_trailing_comma_3(self):
        b = """print 1,"""
        a = """print(1, end=' ')"""
        self.check(b, a)

    # >> stuff

    def test_vargs_without_trailing_comma(self):
        b = """print >>sys.stderr, 1, 2, 3"""
        a = """print(1, 2, 3, file=sys.stderr)"""
        self.check(b, a)

    def test_with_trailing_comma(self):
        b = """print >>sys.stderr, 1, 2,"""
        a = """print(1, 2, end=' ', file=sys.stderr)"""
        self.check(b, a)

    def test_no_trailing_comma(self):
        b = """print >>sys.stderr, 1+1"""
        a = """print(1+1, file=sys.stderr)"""
        self.check(b, a)

    def test_spaces_before_file(self):
        b = """print >>  sys.stderr"""
        a = """print(file=sys.stderr)"""
        self.check(b, a)

    # With from __future__ import print_function
    def test_with_future_print_function(self):
        # XXX: These tests won't actually do anything until the parser
        #      is fixed so it won't crash when it sees print(x=y).
        #      When #2412 is fixed, the try/except block can be taken
        #      out and the tests can be run like normal.
        # MvL: disable entirely for now, so that it doesn't print to stdout
        return
        try:
            s = "from __future__ import print_function\n"\
                "print('Hai!', end=' ')"
            self.unchanged(s)

            b = "print 'Hello, world!'"
            a = "print('Hello, world!')"
            self.check(b, a)

            s = "from __future__ import *\n"\
                "print('Hai!', end=' ')"
            self.unchanged(s)
        except:
            return
        else:
            self.assertFalse(True, "#2421 has been fixed -- printing tests "\
                                   "need to be updated!")

class Test_exec(FixerTestCase):
    fixer = "exec"

    def test_prefix_preservation(self):
        b = """  exec code in ns1,   ns2"""
        a = """  exec(code, ns1,   ns2)"""
        self.check(b, a)

    def test_basic(self):
        b = """exec code"""
        a = """exec(code)"""
        self.check(b, a)

    def test_with_globals(self):
        b = """exec code in ns"""
        a = """exec(code, ns)"""
        self.check(b, a)

    def test_with_globals_locals(self):
        b = """exec code in ns1, ns2"""
        a = """exec(code, ns1, ns2)"""
        self.check(b, a)

    def test_complex_1(self):
        b = """exec (a.b()) in ns"""
        a = """exec((a.b()), ns)"""
        self.check(b, a)

    def test_complex_2(self):
        b = """exec a.b() + c in ns"""
        a = """exec(a.b() + c, ns)"""
        self.check(b, a)

    # These should not be touched

    def test_unchanged_1(self):
        s = """exec(code)"""
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """exec (code)"""
        self.unchanged(s)

    def test_unchanged_3(self):
        s = """exec(code, ns)"""
        self.unchanged(s)

    def test_unchanged_4(self):
        s = """exec(code, ns1, ns2)"""
        self.unchanged(s)

class Test_repr(FixerTestCase):
    fixer = "repr"

    def test_prefix_preservation(self):
        b = """x =   `1 + 2`"""
        a = """x =   repr(1 + 2)"""
        self.check(b, a)

    def test_simple_1(self):
        b = """x = `1 + 2`"""
        a = """x = repr(1 + 2)"""
        self.check(b, a)

    def test_simple_2(self):
        b = """y = `x`"""
        a = """y = repr(x)"""
        self.check(b, a)

    def test_complex(self):
        b = """z = `y`.__repr__()"""
        a = """z = repr(y).__repr__()"""
        self.check(b, a)

    def test_tuple(self):
        b = """x = `1, 2, 3`"""
        a = """x = repr((1, 2, 3))"""
        self.check(b, a)

    def test_nested(self):
        b = """x = `1 + `2``"""
        a = """x = repr(1 + repr(2))"""
        self.check(b, a)

    def test_nested_tuples(self):
        b = """x = `1, 2 + `3, 4``"""
        a = """x = repr((1, 2 + repr((3, 4))))"""
        self.check(b, a)

class Test_except(FixerTestCase):
    fixer = "except"

    def test_prefix_preservation(self):
        b = """
            try:
                pass
            except (RuntimeError, ImportError),    e:
                pass"""
        a = """
            try:
                pass
            except (RuntimeError, ImportError) as    e:
                pass"""
        self.check(b, a)

    def test_simple(self):
        b = """
            try:
                pass
            except Foo, e:
                pass"""
        a = """
            try:
                pass
            except Foo as e:
                pass"""
        self.check(b, a)

    def test_simple_no_space_before_target(self):
        b = """
            try:
                pass
            except Foo,e:
                pass"""
        a = """
            try:
                pass
            except Foo as e:
                pass"""
        self.check(b, a)

    def test_tuple_unpack(self):
        b = """
            def foo():
                try:
                    pass
                except Exception, (f, e):
                    pass
                except ImportError, e:
                    pass"""

        a = """
            def foo():
                try:
                    pass
                except Exception as xxx_todo_changeme:
                    (f, e) = xxx_todo_changeme.args
                    pass
                except ImportError as e:
                    pass"""
        self.check(b, a)

    def test_multi_class(self):
        b = """
            try:
                pass
            except (RuntimeError, ImportError), e:
                pass"""

        a = """
            try:
                pass
            except (RuntimeError, ImportError) as e:
                pass"""
        self.check(b, a)

    def test_list_unpack(self):
        b = """
            try:
                pass
            except Exception, [a, b]:
                pass"""

        a = """
            try:
                pass
            except Exception as xxx_todo_changeme:
                [a, b] = xxx_todo_changeme.args
                pass"""
        self.check(b, a)

    def test_weird_target_1(self):
        b = """
            try:
                pass
            except Exception, d[5]:
                pass"""

        a = """
            try:
                pass
            except Exception as xxx_todo_changeme:
                d[5] = xxx_todo_changeme
                pass"""
        self.check(b, a)

    def test_weird_target_2(self):
        b = """
            try:
                pass
            except Exception, a.foo:
                pass"""

        a = """
            try:
                pass
            except Exception as xxx_todo_changeme:
                a.foo = xxx_todo_changeme
                pass"""
        self.check(b, a)

    def test_weird_target_3(self):
        b = """
            try:
                pass
            except Exception, a().foo:
                pass"""

        a = """
            try:
                pass
            except Exception as xxx_todo_changeme:
                a().foo = xxx_todo_changeme
                pass"""
        self.check(b, a)

    # These should not be touched:

    def test_unchanged_1(self):
        s = """
            try:
                pass
            except:
                pass"""
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """
            try:
                pass
            except Exception:
                pass"""
        self.unchanged(s)

    def test_unchanged_3(self):
        s = """
            try:
                pass
            except (Exception, SystemExit):
                pass"""
        self.unchanged(s)

class Test_raise(FixerTestCase):
    fixer = "raise"

    def test_basic(self):
        b = """raise Exception, 5"""
        a = """raise Exception(5)"""
        self.check(b, a)

    def test_prefix_preservation(self):
        b = """raise Exception,5"""
        a = """raise Exception(5)"""
        self.check(b, a)

        b = """raise   Exception,    5"""
        a = """raise   Exception(5)"""
        self.check(b, a)

    def test_with_comments(self):
        b = """raise Exception, 5 # foo"""
        a = """raise Exception(5) # foo"""
        self.check(b, a)

        b = """raise E, (5, 6) % (a, b) # foo"""
        a = """raise E((5, 6) % (a, b)) # foo"""
        self.check(b, a)

        b = """def foo():
                    raise Exception, 5, 6 # foo"""
        a = """def foo():
                    raise Exception(5).with_traceback(6) # foo"""
        self.check(b, a)

    def test_tuple_value(self):
        b = """raise Exception, (5, 6, 7)"""
        a = """raise Exception(5, 6, 7)"""
        self.check(b, a)

    def test_tuple_detection(self):
        b = """raise E, (5, 6) % (a, b)"""
        a = """raise E((5, 6) % (a, b))"""
        self.check(b, a)

    def test_tuple_exc_1(self):
        b = """raise (((E1, E2), E3), E4), V"""
        a = """raise E1(V)"""
        self.check(b, a)

    def test_tuple_exc_2(self):
        b = """raise (E1, (E2, E3), E4), V"""
        a = """raise E1(V)"""
        self.check(b, a)

    # These should produce a warning

    def test_string_exc(self):
        s = """raise 'foo'"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    def test_string_exc_val(self):
        s = """raise "foo", 5"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    def test_string_exc_val_tb(self):
        s = """raise "foo", 5, 6"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    # These should result in traceback-assignment

    def test_tb_1(self):
        b = """def foo():
                    raise Exception, 5, 6"""
        a = """def foo():
                    raise Exception(5).with_traceback(6)"""
        self.check(b, a)

    def test_tb_2(self):
        b = """def foo():
                    a = 5
                    raise Exception, 5, 6
                    b = 6"""
        a = """def foo():
                    a = 5
                    raise Exception(5).with_traceback(6)
                    b = 6"""
        self.check(b, a)

    def test_tb_3(self):
        b = """def foo():
                    raise Exception,5,6"""
        a = """def foo():
                    raise Exception(5).with_traceback(6)"""
        self.check(b, a)

    def test_tb_4(self):
        b = """def foo():
                    a = 5
                    raise Exception,5,6
                    b = 6"""
        a = """def foo():
                    a = 5
                    raise Exception(5).with_traceback(6)
                    b = 6"""
        self.check(b, a)

    def test_tb_5(self):
        b = """def foo():
                    raise Exception, (5, 6, 7), 6"""
        a = """def foo():
                    raise Exception(5, 6, 7).with_traceback(6)"""
        self.check(b, a)

    def test_tb_6(self):
        b = """def foo():
                    a = 5
                    raise Exception, (5, 6, 7), 6
                    b = 6"""
        a = """def foo():
                    a = 5
                    raise Exception(5, 6, 7).with_traceback(6)
                    b = 6"""
        self.check(b, a)

class Test_throw(FixerTestCase):
    fixer = "throw"

    def test_1(self):
        b = """g.throw(Exception, 5)"""
        a = """g.throw(Exception(5))"""
        self.check(b, a)

    def test_2(self):
        b = """g.throw(Exception,5)"""
        a = """g.throw(Exception(5))"""
        self.check(b, a)

    def test_3(self):
        b = """g.throw(Exception, (5, 6, 7))"""
        a = """g.throw(Exception(5, 6, 7))"""
        self.check(b, a)

    def test_4(self):
        b = """5 + g.throw(Exception, 5)"""
        a = """5 + g.throw(Exception(5))"""
        self.check(b, a)

    # These should produce warnings

    def test_warn_1(self):
        s = """g.throw("foo")"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    def test_warn_2(self):
        s = """g.throw("foo", 5)"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    def test_warn_3(self):
        s = """g.throw("foo", 5, 6)"""
        self.warns_unchanged(s, "Python 3 does not support string exceptions")

    # These should not be touched

    def test_untouched_1(self):
        s = """g.throw(Exception)"""
        self.unchanged(s)

    def test_untouched_2(self):
        s = """g.throw(Exception(5, 6))"""
        self.unchanged(s)

    def test_untouched_3(self):
        s = """5 + g.throw(Exception(5, 6))"""
        self.unchanged(s)

    # These should result in traceback-assignment

    def test_tb_1(self):
        b = """def foo():
                    g.throw(Exception, 5, 6)"""
        a = """def foo():
                    g.throw(Exception(5).with_traceback(6))"""
        self.check(b, a)

    def test_tb_2(self):
        b = """def foo():
                    a = 5
                    g.throw(Exception, 5, 6)
                    b = 6"""
        a = """def foo():
                    a = 5
                    g.throw(Exception(5).with_traceback(6))
                    b = 6"""
        self.check(b, a)

    def test_tb_3(self):
        b = """def foo():
                    g.throw(Exception,5,6)"""
        a = """def foo():
                    g.throw(Exception(5).with_traceback(6))"""
        self.check(b, a)

    def test_tb_4(self):
        b = """def foo():
                    a = 5
                    g.throw(Exception,5,6)
                    b = 6"""
        a = """def foo():
                    a = 5
                    g.throw(Exception(5).with_traceback(6))
                    b = 6"""
        self.check(b, a)

    def test_tb_5(self):
        b = """def foo():
                    g.throw(Exception, (5, 6, 7), 6)"""
        a = """def foo():
                    g.throw(Exception(5, 6, 7).with_traceback(6))"""
        self.check(b, a)

    def test_tb_6(self):
        b = """def foo():
                    a = 5
                    g.throw(Exception, (5, 6, 7), 6)
                    b = 6"""
        a = """def foo():
                    a = 5
                    g.throw(Exception(5, 6, 7).with_traceback(6))
                    b = 6"""
        self.check(b, a)

    def test_tb_7(self):
        b = """def foo():
                    a + g.throw(Exception, 5, 6)"""
        a = """def foo():
                    a + g.throw(Exception(5).with_traceback(6))"""
        self.check(b, a)

    def test_tb_8(self):
        b = """def foo():
                    a = 5
                    a + g.throw(Exception, 5, 6)
                    b = 6"""
        a = """def foo():
                    a = 5
                    a + g.throw(Exception(5).with_traceback(6))
                    b = 6"""
        self.check(b, a)

class Test_long(FixerTestCase):
    fixer = "long"

    def test_1(self):
        b = """x = long(x)"""
        a = """x = int(x)"""
        self.check(b, a)

    def test_2(self):
        b = """y = isinstance(x, long)"""
        a = """y = isinstance(x, int)"""
        self.check(b, a)

    def test_3(self):
        b = """z = type(x) in (int, long)"""
        a = """z = type(x) in (int, int)"""
        self.check(b, a)

    def test_4(self):
        b = """a = 12L"""
        a = """a = 12"""
        self.check(b, a)

    def test_5(self):
        b = """b = 0x12l"""
        a = """b = 0x12"""
        self.check(b, a)

    def test_unchanged_1(self):
        s = """a = 12"""
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """b = 0x12"""
        self.unchanged(s)

    def test_unchanged_3(self):
        s = """c = 3.14"""
        self.unchanged(s)

    def test_prefix_preservation(self):
        b = """x =   long(  x  )"""
        a = """x =   int(  x  )"""
        self.check(b, a)

class Test_dict(FixerTestCase):
    fixer = "dict"

    def test_prefix_preservation(self):
        b = "if   d. keys  (  )  : pass"
        a = "if   list(d. keys  (  ))  : pass"
        self.check(b, a)

        b = "if   d. items  (  )  : pass"
        a = "if   list(d. items  (  ))  : pass"
        self.check(b, a)

        b = "if   d. iterkeys  ( )  : pass"
        a = "if   iter(d. keys  ( ))  : pass"
        self.check(b, a)

        b = "[i for i in    d.  iterkeys(  )  ]"
        a = "[i for i in    d.  keys(  )  ]"
        self.check(b, a)

    def test_trailing_comment(self):
        b = "d.keys() # foo"
        a = "list(d.keys()) # foo"
        self.check(b, a)

        b = "d.items()  # foo"
        a = "list(d.items())  # foo"
        self.check(b, a)

        b = "d.iterkeys()  # foo"
        a = "iter(d.keys())  # foo"
        self.check(b, a)

        b = """[i for i in d.iterkeys() # foo
               ]"""
        a = """[i for i in d.keys() # foo
               ]"""
        self.check(b, a)

    def test_unchanged(self):
        wrappers = ["set", "sorted", "any", "all", "tuple", "sum"]
        for wrapper in wrappers:
            s = "s = %s(d.keys())" % wrapper
            self.unchanged(s)

            s = "s = %s(d.values())" % wrapper
            self.unchanged(s)

            s = "s = %s(d.items())" % wrapper
            self.unchanged(s)

    def test_01(self):
        b = "d.keys()"
        a = "list(d.keys())"
        self.check(b, a)

        b = "a[0].foo().keys()"
        a = "list(a[0].foo().keys())"
        self.check(b, a)

    def test_02(self):
        b = "d.items()"
        a = "list(d.items())"
        self.check(b, a)

    def test_03(self):
        b = "d.values()"
        a = "list(d.values())"
        self.check(b, a)

    def test_04(self):
        b = "d.iterkeys()"
        a = "iter(d.keys())"
        self.check(b, a)

    def test_05(self):
        b = "d.iteritems()"
        a = "iter(d.items())"
        self.check(b, a)

    def test_06(self):
        b = "d.itervalues()"
        a = "iter(d.values())"
        self.check(b, a)

    def test_07(self):
        s = "list(d.keys())"
        self.unchanged(s)

    def test_08(self):
        s = "sorted(d.keys())"
        self.unchanged(s)

    def test_09(self):
        b = "iter(d.keys())"
        a = "iter(list(d.keys()))"
        self.check(b, a)

    def test_10(self):
        b = "foo(d.keys())"
        a = "foo(list(d.keys()))"
        self.check(b, a)

    def test_11(self):
        b = "for i in d.keys(): print i"
        a = "for i in list(d.keys()): print i"
        self.check(b, a)

    def test_12(self):
        b = "for i in d.iterkeys(): print i"
        a = "for i in d.keys(): print i"
        self.check(b, a)

    def test_13(self):
        b = "[i for i in d.keys()]"
        a = "[i for i in list(d.keys())]"
        self.check(b, a)

    def test_14(self):
        b = "[i for i in d.iterkeys()]"
        a = "[i for i in d.keys()]"
        self.check(b, a)

    def test_15(self):
        b = "(i for i in d.keys())"
        a = "(i for i in list(d.keys()))"
        self.check(b, a)

    def test_16(self):
        b = "(i for i in d.iterkeys())"
        a = "(i for i in d.keys())"
        self.check(b, a)

    def test_17(self):
        b = "iter(d.iterkeys())"
        a = "iter(d.keys())"
        self.check(b, a)

    def test_18(self):
        b = "list(d.iterkeys())"
        a = "list(d.keys())"
        self.check(b, a)

    def test_19(self):
        b = "sorted(d.iterkeys())"
        a = "sorted(d.keys())"
        self.check(b, a)

    def test_20(self):
        b = "foo(d.iterkeys())"
        a = "foo(iter(d.keys()))"
        self.check(b, a)

    def test_21(self):
        b = "print h.iterkeys().next()"
        a = "print iter(h.keys()).next()"
        self.check(b, a)

    def test_22(self):
        b = "print h.keys()[0]"
        a = "print list(h.keys())[0]"
        self.check(b, a)

    def test_23(self):
        b = "print list(h.iterkeys().next())"
        a = "print list(iter(h.keys()).next())"
        self.check(b, a)

    def test_24(self):
        b = "for x in h.keys()[0]: print x"
        a = "for x in list(h.keys())[0]: print x"
        self.check(b, a)

class Test_xrange(FixerTestCase):
    fixer = "xrange"

    def test_prefix_preservation(self):
        b = """x =    xrange(  10  )"""
        a = """x =    range(  10  )"""
        self.check(b, a)

        b = """x = xrange(  1  ,  10   )"""
        a = """x = range(  1  ,  10   )"""
        self.check(b, a)

        b = """x = xrange(  0  ,  10 ,  2 )"""
        a = """x = range(  0  ,  10 ,  2 )"""
        self.check(b, a)

    def test_1(self):
        b = """x = xrange(10)"""
        a = """x = range(10)"""
        self.check(b, a)

    def test_2(self):
        b = """x = xrange(1, 10)"""
        a = """x = range(1, 10)"""
        self.check(b, a)

    def test_3(self):
        b = """x = xrange(0, 10, 2)"""
        a = """x = range(0, 10, 2)"""
        self.check(b, a)

    def test_4(self):
        b = """for i in xrange(10):\n    j=i"""
        a = """for i in range(10):\n    j=i"""
        self.check(b, a)

class Test_raw_input(FixerTestCase):
    fixer = "raw_input"

    def test_prefix_preservation(self):
        b = """x =    raw_input(   )"""
        a = """x =    input(   )"""
        self.check(b, a)

        b = """x = raw_input(   ''   )"""
        a = """x = input(   ''   )"""
        self.check(b, a)

    def test_1(self):
        b = """x = raw_input()"""
        a = """x = input()"""
        self.check(b, a)

    def test_2(self):
        b = """x = raw_input('')"""
        a = """x = input('')"""
        self.check(b, a)

    def test_3(self):
        b = """x = raw_input('prompt')"""
        a = """x = input('prompt')"""
        self.check(b, a)

    def test_4(self):
        b = """x = raw_input(foo(a) + 6)"""
        a = """x = input(foo(a) + 6)"""
        self.check(b, a)

class Test_funcattrs(FixerTestCase):
    fixer = "funcattrs"

    attrs = ["closure", "doc", "name", "defaults", "code", "globals", "dict"]

    def test(self):
        for attr in self.attrs:
            b = "a.func_%s" % attr
            a = "a.__%s__" % attr
            self.check(b, a)

            b = "self.foo.func_%s.foo_bar" % attr
            a = "self.foo.__%s__.foo_bar" % attr
            self.check(b, a)

    def test_unchanged(self):
        for attr in self.attrs:
            s = "foo(func_%s + 5)" % attr
            self.unchanged(s)

            s = "f(foo.__%s__)" % attr
            self.unchanged(s)

            s = "f(foo.__%s__.foo)" % attr
            self.unchanged(s)

class Test_xreadlines(FixerTestCase):
    fixer = "xreadlines"

    def test_call(self):
        b = "for x in f.xreadlines(): pass"
        a = "for x in f: pass"
        self.check(b, a)

        b = "for x in foo().xreadlines(): pass"
        a = "for x in foo(): pass"
        self.check(b, a)

        b = "for x in (5 + foo()).xreadlines(): pass"
        a = "for x in (5 + foo()): pass"
        self.check(b, a)

    def test_attr_ref(self):
        b = "foo(f.xreadlines + 5)"
        a = "foo(f.__iter__ + 5)"
        self.check(b, a)

        b = "foo(f().xreadlines + 5)"
        a = "foo(f().__iter__ + 5)"
        self.check(b, a)

        b = "foo((5 + f()).xreadlines + 5)"
        a = "foo((5 + f()).__iter__ + 5)"
        self.check(b, a)

    def test_unchanged(self):
        s = "for x in f.xreadlines(5): pass"
        self.unchanged(s)

        s = "for x in f.xreadlines(k=5): pass"
        self.unchanged(s)

        s = "for x in f.xreadlines(*k, **v): pass"
        self.unchanged(s)

        s = "foo(xreadlines)"
        self.unchanged(s)

class Test_imports(FixerTestCase):
    fixer = "imports"

    modules = {"StringIO":  ("io", ["StringIO"]),
               "cStringIO": ("io", ["StringIO"]),
               "__builtin__" : ("builtins", ["open", "Exception",
                   "__debug__", "str"]),
              }

    def test_import_module(self):
        for old, (new, members) in self.modules.items():
            b = "import %s" % old
            a = "import %s" % new
            self.check(b, a)

            b = "import foo, %s, bar" % old
            a = "import foo, %s, bar" % new
            self.check(b, a)

    def test_import_from(self):
        for old, (new, members) in self.modules.items():
            for member in members:
                b = "from %s import %s" % (old, member)
                a = "from %s import %s" % (new, member)
                self.check(b, a)

                s = "from foo import %s" % member
                self.unchanged(s)

    def test_import_module_as(self):
        for old, (new, members) in self.modules.items():
            b = "import %s as foo_bar" % old
            a = "import %s as foo_bar" % new
            self.check(b, a)

            b = "import %s as foo_bar" % old
            a = "import %s as foo_bar" % new
            self.check(b, a)

    def test_import_from_as(self):
        for old, (new, members) in self.modules.items():
            for member in members:
                b = "from %s import %s as foo_bar" % (old, member)
                a = "from %s import %s as foo_bar" % (new, member)
                self.check(b, a)

    def test_star(self):
        for old in self.modules:
            s = "from %s import *" % old
            self.warns_unchanged(s, "Cannot handle star imports")

    def test_import_module_usage(self):
        for old, (new, members) in self.modules.items():
            for member in members:
                b = """
                    import %s
                    foo(%s, %s.%s)
                    """ % (old, old, old, member)
                a = """
                    import %s
                    foo(%s, %s.%s)
                    """ % (new, new, new, member)
                self.check(b, a)

    def test_from_import_usage(self):
        for old, (new, members) in self.modules.items():
            for member in members:
                b = """
                    from %s import %s
                    foo(%s, %s())
                    """ % (old, member, member, member)
                a = """
                    from %s import %s
                    foo(%s, %s())
                    """ % (new, member, member, member)
                self.check(b, a)

class Test_input(FixerTestCase):
    fixer = "input"

    def test_prefix_preservation(self):
        b = """x =   input(   )"""
        a = """x =   eval(input(   ))"""
        self.check(b, a)

        b = """x = input(   ''   )"""
        a = """x = eval(input(   ''   ))"""
        self.check(b, a)

    def test_trailing_comment(self):
        b = """x = input()  #  foo"""
        a = """x = eval(input())  #  foo"""
        self.check(b, a)

    def test_idempotency(self):
        s = """x = eval(input())"""
        self.unchanged(s)

        s = """x = eval(input(''))"""
        self.unchanged(s)

        s = """x = eval(input(foo(5) + 9))"""
        self.unchanged(s)

    def test_1(self):
        b = """x = input()"""
        a = """x = eval(input())"""
        self.check(b, a)

    def test_2(self):
        b = """x = input('')"""
        a = """x = eval(input(''))"""
        self.check(b, a)

    def test_3(self):
        b = """x = input('prompt')"""
        a = """x = eval(input('prompt'))"""
        self.check(b, a)

    def test_4(self):
        b = """x = input(foo(5) + 9)"""
        a = """x = eval(input(foo(5) + 9))"""
        self.check(b, a)

class Test_tuple_params(FixerTestCase):
    fixer = "tuple_params"

    def test_unchanged_1(self):
        s = """def foo(): pass"""
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """def foo(a, b, c): pass"""
        self.unchanged(s)

    def test_unchanged_3(self):
        s = """def foo(a=3, b=4, c=5): pass"""
        self.unchanged(s)

    def test_1(self):
        b = """
            def foo(((a, b), c)):
                x = 5"""

        a = """
            def foo(xxx_todo_changeme):
                ((a, b), c) = xxx_todo_changeme
                x = 5"""
        self.check(b, a)

    def test_2(self):
        b = """
            def foo(((a, b), c), d):
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, d):
                ((a, b), c) = xxx_todo_changeme
                x = 5"""
        self.check(b, a)

    def test_3(self):
        b = """
            def foo(((a, b), c), d) -> e:
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, d) -> e:
                ((a, b), c) = xxx_todo_changeme
                x = 5"""
        self.check(b, a)

    def test_semicolon(self):
        b = """
            def foo(((a, b), c)): x = 5; y = 7"""

        a = """
            def foo(xxx_todo_changeme): ((a, b), c) = xxx_todo_changeme; x = 5; y = 7"""
        self.check(b, a)

    def test_keywords(self):
        b = """
            def foo(((a, b), c), d, e=5) -> z:
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, d, e=5) -> z:
                ((a, b), c) = xxx_todo_changeme
                x = 5"""
        self.check(b, a)

    def test_varargs(self):
        b = """
            def foo(((a, b), c), d, *vargs, **kwargs) -> z:
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, d, *vargs, **kwargs) -> z:
                ((a, b), c) = xxx_todo_changeme
                x = 5"""
        self.check(b, a)

    def test_multi_1(self):
        b = """
            def foo(((a, b), c), (d, e, f)) -> z:
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, xxx_todo_changeme1) -> z:
                ((a, b), c) = xxx_todo_changeme
                (d, e, f) = xxx_todo_changeme1
                x = 5"""
        self.check(b, a)

    def test_multi_2(self):
        b = """
            def foo(x, ((a, b), c), d, (e, f, g), y) -> z:
                x = 5"""

        a = """
            def foo(x, xxx_todo_changeme, d, xxx_todo_changeme1, y) -> z:
                ((a, b), c) = xxx_todo_changeme
                (e, f, g) = xxx_todo_changeme1
                x = 5"""
        self.check(b, a)

    def test_docstring(self):
        b = """
            def foo(((a, b), c), (d, e, f)) -> z:
                "foo foo foo foo"
                x = 5"""

        a = """
            def foo(xxx_todo_changeme, xxx_todo_changeme1) -> z:
                "foo foo foo foo"
                ((a, b), c) = xxx_todo_changeme
                (d, e, f) = xxx_todo_changeme1
                x = 5"""
        self.check(b, a)

    def test_lambda_no_change(self):
        s = """lambda x: x + 5"""
        self.unchanged(s)

    def test_lambda_parens_single_arg(self):
        b = """lambda (x): x + 5"""
        a = """lambda x: x + 5"""
        self.check(b, a)

        b = """lambda(x): x + 5"""
        a = """lambda x: x + 5"""
        self.check(b, a)

        b = """lambda ((((x)))): x + 5"""
        a = """lambda x: x + 5"""
        self.check(b, a)

        b = """lambda((((x)))): x + 5"""
        a = """lambda x: x + 5"""
        self.check(b, a)

    def test_lambda_simple(self):
        b = """lambda (x, y): x + f(y)"""
        a = """lambda x_y: x_y[0] + f(x_y[1])"""
        self.check(b, a)

        b = """lambda(x, y): x + f(y)"""
        a = """lambda x_y: x_y[0] + f(x_y[1])"""
        self.check(b, a)

        b = """lambda (((x, y))): x + f(y)"""
        a = """lambda x_y: x_y[0] + f(x_y[1])"""
        self.check(b, a)

        b = """lambda(((x, y))): x + f(y)"""
        a = """lambda x_y: x_y[0] + f(x_y[1])"""
        self.check(b, a)

    def test_lambda_one_tuple(self):
        b = """lambda (x,): x + f(x)"""
        a = """lambda x1: x1[0] + f(x1[0])"""
        self.check(b, a)

        b = """lambda (((x,))): x + f(x)"""
        a = """lambda x1: x1[0] + f(x1[0])"""
        self.check(b, a)

    def test_lambda_simple_multi_use(self):
        b = """lambda (x, y): x + x + f(x) + x"""
        a = """lambda x_y: x_y[0] + x_y[0] + f(x_y[0]) + x_y[0]"""
        self.check(b, a)

    def test_lambda_simple_reverse(self):
        b = """lambda (x, y): y + x"""
        a = """lambda x_y: x_y[1] + x_y[0]"""
        self.check(b, a)

    def test_lambda_nested(self):
        b = """lambda (x, (y, z)): x + y + z"""
        a = """lambda x_y_z: x_y_z[0] + x_y_z[1][0] + x_y_z[1][1]"""
        self.check(b, a)

        b = """lambda (((x, (y, z)))): x + y + z"""
        a = """lambda x_y_z: x_y_z[0] + x_y_z[1][0] + x_y_z[1][1]"""
        self.check(b, a)

    def test_lambda_nested_multi_use(self):
        b = """lambda (x, (y, z)): x + y + f(y)"""
        a = """lambda x_y_z: x_y_z[0] + x_y_z[1][0] + f(x_y_z[1][0])"""
        self.check(b, a)

class Test_methodattrs(FixerTestCase):
    fixer = "methodattrs"

    attrs = ["func", "self", "class"]

    def test(self):
        for attr in self.attrs:
            b = "a.im_%s" % attr
            if attr == "class":
                a = "a.__self__.__class__"
            else:
                a = "a.__%s__" % attr
            self.check(b, a)

            b = "self.foo.im_%s.foo_bar" % attr
            if attr == "class":
                a = "self.foo.__self__.__class__.foo_bar"
            else:
                a = "self.foo.__%s__.foo_bar" % attr
            self.check(b, a)

    def test_unchanged(self):
        for attr in self.attrs:
            s = "foo(im_%s + 5)" % attr
            self.unchanged(s)

            s = "f(foo.__%s__)" % attr
            self.unchanged(s)

            s = "f(foo.__%s__.foo)" % attr
            self.unchanged(s)

class Test_next(FixerTestCase):
    fixer = "next"

    def test_1(self):
        b = """it.next()"""
        a = """next(it)"""
        self.check(b, a)

    def test_2(self):
        b = """a.b.c.d.next()"""
        a = """next(a.b.c.d)"""
        self.check(b, a)

    def test_3(self):
        b = """(a + b).next()"""
        a = """next((a + b))"""
        self.check(b, a)

    def test_4(self):
        b = """a().next()"""
        a = """next(a())"""
        self.check(b, a)

    def test_5(self):
        b = """a().next() + b"""
        a = """next(a()) + b"""
        self.check(b, a)

    def test_6(self):
        b = """c(      a().next() + b)"""
        a = """c(      next(a()) + b)"""
        self.check(b, a)

    def test_prefix_preservation_1(self):
        b = """
            for a in b:
                foo(a)
                a.next()
            """
        a = """
            for a in b:
                foo(a)
                next(a)
            """
        self.check(b, a)

    def test_prefix_preservation_2(self):
        b = """
            for a in b:
                foo(a) # abc
                # def
                a.next()
            """
        a = """
            for a in b:
                foo(a) # abc
                # def
                next(a)
            """
        self.check(b, a)

    def test_prefix_preservation_3(self):
        b = """
            next = 5
            for a in b:
                foo(a)
                a.next()
            """
        a = """
            next = 5
            for a in b:
                foo(a)
                a.__next__()
            """
        self.check(b, a, ignore_warnings=True)

    def test_prefix_preservation_4(self):
        b = """
            next = 5
            for a in b:
                foo(a) # abc
                # def
                a.next()
            """
        a = """
            next = 5
            for a in b:
                foo(a) # abc
                # def
                a.__next__()
            """
        self.check(b, a, ignore_warnings=True)

    def test_prefix_preservation_5(self):
        b = """
            next = 5
            for a in b:
                foo(foo(a), # abc
                    a.next())
            """
        a = """
            next = 5
            for a in b:
                foo(foo(a), # abc
                    a.__next__())
            """
        self.check(b, a, ignore_warnings=True)

    def test_prefix_preservation_6(self):
        b = """
            for a in b:
                foo(foo(a), # abc
                    a.next())
            """
        a = """
            for a in b:
                foo(foo(a), # abc
                    next(a))
            """
        self.check(b, a)

    def test_method_1(self):
        b = """
            class A:
                def next(self):
                    pass
            """
        a = """
            class A:
                def __next__(self):
                    pass
            """
        self.check(b, a)

    def test_method_2(self):
        b = """
            class A(object):
                def next(self):
                    pass
            """
        a = """
            class A(object):
                def __next__(self):
                    pass
            """
        self.check(b, a)

    def test_method_3(self):
        b = """
            class A:
                def next(x):
                    pass
            """
        a = """
            class A:
                def __next__(x):
                    pass
            """
        self.check(b, a)

    def test_method_4(self):
        b = """
            class A:
                def __init__(self, foo):
                    self.foo = foo

                def next(self):
                    pass

                def __iter__(self):
                    return self
            """
        a = """
            class A:
                def __init__(self, foo):
                    self.foo = foo

                def __next__(self):
                    pass

                def __iter__(self):
                    return self
            """
        self.check(b, a)

    def test_method_unchanged(self):
        s = """
            class A:
                def next(self, a, b):
                    pass
            """
        self.unchanged(s)

    def test_shadowing_assign_simple(self):
        s = """
            next = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_assign_tuple_1(self):
        s = """
            (next, a) = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_assign_tuple_2(self):
        s = """
            (a, (b, (next, c)), a) = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_assign_list_1(self):
        s = """
            [next, a] = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_assign_list_2(self):
        s = """
            [a, [b, [next, c]], a] = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_builtin_assign(self):
        s = """
            def foo():
                __builtin__.next = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_builtin_assign_in_tuple(self):
        s = """
            def foo():
                (a, __builtin__.next) = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_builtin_assign_in_list(self):
        s = """
            def foo():
                [a, __builtin__.next] = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_assign_to_next(self):
        s = """
            def foo():
                A.next = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.unchanged(s)

    def test_assign_to_next_in_tuple(self):
        s = """
            def foo():
                (a, A.next) = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.unchanged(s)

    def test_assign_to_next_in_list(self):
        s = """
            def foo():
                [a, A.next] = foo

            class A:
                def next(self, a, b):
                    pass
            """
        self.unchanged(s)

    def test_shadowing_import_1(self):
        s = """
            import foo.bar as next

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_2(self):
        s = """
            import bar, bar.foo as next

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_3(self):
        s = """
            import bar, bar.foo as next, baz

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_from_1(self):
        s = """
            from x import next

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_from_2(self):
        s = """
            from x.a import next

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_from_3(self):
        s = """
            from x import a, next, b

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_import_from_4(self):
        s = """
            from x.a import a, next, b

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_funcdef_1(self):
        s = """
            def next(a):
                pass

            class A:
                def next(self, a, b):
                    pass
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_funcdef_2(self):
        b = """
            def next(a):
                pass

            class A:
                def next(self):
                    pass

            it.next()
            """
        a = """
            def next(a):
                pass

            class A:
                def __next__(self):
                    pass

            it.__next__()
            """
        self.warns(b, a, "Calls to builtin next() possibly shadowed")

    def test_shadowing_global_1(self):
        s = """
            def f():
                global next
                next = 5
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_global_2(self):
        s = """
            def f():
                global a, next, b
                next = 5
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_for_simple(self):
        s = """
            for next in it():
                pass

            b = 5
            c = 6
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_for_tuple_1(self):
        s = """
            for next, b in it():
                pass

            b = 5
            c = 6
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_shadowing_for_tuple_2(self):
        s = """
            for a, (next, c), b in it():
                pass

            b = 5
            c = 6
            """
        self.warns_unchanged(s, "Calls to builtin next() possibly shadowed")

    def test_noncall_access_1(self):
        b = """gnext = g.next"""
        a = """gnext = g.__next__"""
        self.check(b, a)

    def test_noncall_access_2(self):
        b = """f(g.next + 5)"""
        a = """f(g.__next__ + 5)"""
        self.check(b, a)

    def test_noncall_access_3(self):
        b = """f(g().next + 5)"""
        a = """f(g().__next__ + 5)"""
        self.check(b, a)

class Test_nonzero(FixerTestCase):
    fixer = "nonzero"

    def test_1(self):
        b = """
            class A:
                def __nonzero__(self):
                    pass
            """
        a = """
            class A:
                def __bool__(self):
                    pass
            """
        self.check(b, a)

    def test_2(self):
        b = """
            class A(object):
                def __nonzero__(self):
                    pass
            """
        a = """
            class A(object):
                def __bool__(self):
                    pass
            """
        self.check(b, a)

    def test_unchanged_1(self):
        s = """
            class A(object):
                def __bool__(self):
                    pass
            """
        self.unchanged(s)

    def test_unchanged_2(self):
        s = """
            class A(object):
                def __nonzero__(self, a):
                    pass
            """
        self.unchanged(s)

    def test_unchanged_func(self):
        s = """
            def __nonzero__(self):
                pass
            """
        self.unchanged(s)

class Test_numliterals(FixerTestCase):
    fixer = "numliterals"

    def test_octal_1(self):
        b = """0755"""
        a = """0o755"""
        self.check(b, a)

    def test_long_int_1(self):
        b = """a = 12L"""
        a = """a = 12"""
        self.check(b, a)

    def test_long_int_2(self):
        b = """a = 12l"""
        a = """a = 12"""
        self.check(b, a)

    def test_long_hex(self):
        b = """b = 0x12l"""
        a = """b = 0x12"""
        self.check(b, a)

    def test_unchanged_int(self):
        s = """5"""
        self.unchanged(s)

    def test_unchanged_float(self):
        s = """5.0"""
        self.unchanged(s)

    def test_unchanged_octal(self):
        s = """0o755"""
        self.unchanged(s)

    def test_unchanged_hex(self):
        s = """0xABC"""
        self.unchanged(s)

    def test_unchanged_exp(self):
        s = """5.0e10"""
        self.unchanged(s)

    def test_unchanged_complex_int(self):
        s = """5 + 4j"""
        self.unchanged(s)

    def test_unchanged_complex_float(self):
        s = """5.4 + 4.9j"""
        self.unchanged(s)

    def test_unchanged_complex_bare(self):
        s = """4j"""
        self.unchanged(s)
        s = """4.4j"""
        self.unchanged(s)

class Test_renames(FixerTestCase):
    fixer = "renames"

    modules = {"sys":  ("maxint", "maxsize"),
              }

    def test_import_from(self):
        for mod, (old, new) in self.modules.items():
            b = "from %s import %s" % (mod, old)
            a = "from %s import %s" % (mod, new)
            self.check(b, a)

            s = "from foo import %s" % old
            self.unchanged(s)

    def test_import_from_as(self):
        for mod, (old, new) in self.modules.items():
            b = "from %s import %s as foo_bar" % (mod, old)
            a = "from %s import %s as foo_bar" % (mod, new)
            self.check(b, a)

    def test_import_module_usage(self):
        for mod, (old, new) in self.modules.items():
            b = """
                import %s
                foo(%s, %s.%s)
                """ % (mod, mod, mod, old)
            a = """
                import %s
                foo(%s, %s.%s)
                """ % (mod, mod, mod, new)
            self.check(b, a)

    def XXX_test_from_import_usage(self):
        # not implemented yet
        for mod, (old, new) in self.modules.items():
            b = """
                from %s import %s
                foo(%s, %s)
                """ % (mod, old, mod, old)
            a = """
                from %s import %s
                foo(%s, %s)
                """ % (mod, new, mod, new)
            self.check(b, a)

class Test_unicode(FixerTestCase):
    fixer = "unicode"

    def test_unicode_call(self):
        b = """unicode(x, y, z)"""
        a = """str(x, y, z)"""
        self.check(b, a)

    def test_unicode_literal_1(self):
        b = '''u"x"'''
        a = '''"x"'''
        self.check(b, a)

    def test_unicode_literal_2(self):
        b = """ur'x'"""
        a = """r'x'"""
        self.check(b, a)

    def test_unicode_literal_3(self):
        b = """UR'''x'''"""
        a = """R'''x'''"""
        self.check(b, a)

class Test_callable(FixerTestCase):
    fixer = "callable"

    def test_prefix_preservation(self):
        b = """callable(    x)"""
        a = """hasattr(    x, '__call__')"""
        self.check(b, a)

        b = """if     callable(x): pass"""
        a = """if     hasattr(x, '__call__'): pass"""
        self.check(b, a)

    def test_callable_call(self):
        b = """callable(x)"""
        a = """hasattr(x, '__call__')"""
        self.check(b, a)

    def test_callable_should_not_change(self):
        a = """callable(*x)"""
        self.unchanged(a)

        a = """callable(x, y)"""
        self.unchanged(a)

        a = """callable(x, kw=y)"""
        self.unchanged(a)

        a = """callable()"""
        self.unchanged(a)

class Test_filter(FixerTestCase):
    fixer = "filter"

    def test_prefix_preservation(self):
        b = """x =   filter(    foo,     'abc'   )"""
        a = """x =   list(filter(    foo,     'abc'   ))"""
        self.check(b, a)

        b = """x =   filter(  None , 'abc'  )"""
        a = """x =   [_f for _f in 'abc' if _f]"""
        self.check(b, a)

    def test_filter_basic(self):
        b = """x = filter(None, 'abc')"""
        a = """x = [_f for _f in 'abc' if _f]"""
        self.check(b, a)

        b = """x = len(filter(f, 'abc'))"""
        a = """x = len(list(filter(f, 'abc')))"""
        self.check(b, a)

        b = """x = filter(lambda x: x%2 == 0, range(10))"""
        a = """x = [x for x in range(10) if x%2 == 0]"""
        self.check(b, a)

        # Note the parens around x
        b = """x = filter(lambda (x): x%2 == 0, range(10))"""
        a = """x = [x for x in range(10) if x%2 == 0]"""
        self.check(b, a)

        # XXX This (rare) case is not supported
##         b = """x = filter(f, 'abc')[0]"""
##         a = """x = list(filter(f, 'abc'))[0]"""
##         self.check(b, a)

    def test_filter_nochange(self):
        a = """b.join(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """(a + foo(5)).join(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """iter(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """list(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """list(filter(f, 'abc'))[0]"""
        self.unchanged(a)
        a = """set(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """set(filter(f, 'abc')).pop()"""
        self.unchanged(a)
        a = """tuple(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """any(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """all(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """sum(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """sorted(filter(f, 'abc'))"""
        self.unchanged(a)
        a = """sorted(filter(f, 'abc'), key=blah)"""
        self.unchanged(a)
        a = """sorted(filter(f, 'abc'), key=blah)[0]"""
        self.unchanged(a)
        a = """for i in filter(f, 'abc'): pass"""
        self.unchanged(a)
        a = """[x for x in filter(f, 'abc')]"""
        self.unchanged(a)
        a = """(x for x in filter(f, 'abc'))"""
        self.unchanged(a)

    def test_future_builtins(self):
        a = "from future_builtins import spam, filter; filter(f, 'ham')"
        self.unchanged(a)

        b = """from future_builtins import spam; x = filter(f, 'abc')"""
        a = """from future_builtins import spam; x = list(filter(f, 'abc'))"""
        self.check(b, a)

        a = "from future_builtins import *; filter(f, 'ham')"
        self.unchanged(a)

class Test_map(FixerTestCase):
    fixer = "map"

    def check(self, b, a):
        self.unchanged("from future_builtins import map; " + b, a)
        FixerTestCase.check(self, b, a)

    def test_prefix_preservation(self):
        b = """x =    map(   f,    'abc'   )"""
        a = """x =    list(map(   f,    'abc'   ))"""
        self.check(b, a)

    def test_trailing_comment(self):
        b = """x = map(f, 'abc')   #   foo"""
        a = """x = list(map(f, 'abc'))   #   foo"""
        self.check(b, a)

    def test_map_basic(self):
        b = """x = map(f, 'abc')"""
        a = """x = list(map(f, 'abc'))"""
        self.check(b, a)

        b = """x = len(map(f, 'abc', 'def'))"""
        a = """x = len(list(map(f, 'abc', 'def')))"""
        self.check(b, a)

        b = """x = map(None, 'abc')"""
        a = """x = list('abc')"""
        self.check(b, a)

        b = """x = map(None, 'abc', 'def')"""
        a = """x = list(map(None, 'abc', 'def'))"""
        self.check(b, a)

        b = """x = map(lambda x: x+1, range(4))"""
        a = """x = [x+1 for x in range(4)]"""
        self.check(b, a)

        # Note the parens around x
        b = """x = map(lambda (x): x+1, range(4))"""
        a = """x = [x+1 for x in range(4)]"""
        self.check(b, a)

        b = """
            foo()
            # foo
            map(f, x)
            """
        a = """
            foo()
            # foo
            list(map(f, x))
            """
        self.warns(b, a, "You should use a for loop here")

        # XXX This (rare) case is not supported
##         b = """x = map(f, 'abc')[0]"""
##         a = """x = list(map(f, 'abc'))[0]"""
##         self.check(b, a)

    def test_map_nochange(self):
        a = """b.join(map(f, 'abc'))"""
        self.unchanged(a)
        a = """(a + foo(5)).join(map(f, 'abc'))"""
        self.unchanged(a)
        a = """iter(map(f, 'abc'))"""
        self.unchanged(a)
        a = """list(map(f, 'abc'))"""
        self.unchanged(a)
        a = """list(map(f, 'abc'))[0]"""
        self.unchanged(a)
        a = """set(map(f, 'abc'))"""
        self.unchanged(a)
        a = """set(map(f, 'abc')).pop()"""
        self.unchanged(a)
        a = """tuple(map(f, 'abc'))"""
        self.unchanged(a)
        a = """any(map(f, 'abc'))"""
        self.unchanged(a)
        a = """all(map(f, 'abc'))"""
        self.unchanged(a)
        a = """sum(map(f, 'abc'))"""
        self.unchanged(a)
        a = """sorted(map(f, 'abc'))"""
        self.unchanged(a)
        a = """sorted(map(f, 'abc'), key=blah)"""
        self.unchanged(a)
        a = """sorted(map(f, 'abc'), key=blah)[0]"""
        self.unchanged(a)
        a = """for i in map(f, 'abc'): pass"""
        self.unchanged(a)
        a = """[x for x in map(f, 'abc')]"""
        self.unchanged(a)
        a = """(x for x in map(f, 'abc'))"""
        self.unchanged(a)

    def test_future_builtins(self):
        a = "from future_builtins import spam, map, eggs; map(f, 'ham')"
        self.unchanged(a)

        b = """from future_builtins import spam, eggs; x = map(f, 'abc')"""
        a = """from future_builtins import spam, eggs; x = list(map(f, 'abc'))"""
        self.check(b, a)

        a = "from future_builtins import *; map(f, 'ham')"
        self.unchanged(a)

class Test_zip(FixerTestCase):
    fixer = "zip"

    def check(self, b, a):
        self.unchanged("from future_builtins import zip; " + b, a)
        FixerTestCase.check(self, b, a)

    def test_zip_basic(self):
        b = """x = zip(a, b, c)"""
        a = """x = list(zip(a, b, c))"""
        self.check(b, a)

        b = """x = len(zip(a, b))"""
        a = """x = len(list(zip(a, b)))"""
        self.check(b, a)

    def test_zip_nochange(self):
        a = """b.join(zip(a, b))"""
        self.unchanged(a)
        a = """(a + foo(5)).join(zip(a, b))"""
        self.unchanged(a)
        a = """iter(zip(a, b))"""
        self.unchanged(a)
        a = """list(zip(a, b))"""
        self.unchanged(a)
        a = """list(zip(a, b))[0]"""
        self.unchanged(a)
        a = """set(zip(a, b))"""
        self.unchanged(a)
        a = """set(zip(a, b)).pop()"""
        self.unchanged(a)
        a = """tuple(zip(a, b))"""
        self.unchanged(a)
        a = """any(zip(a, b))"""
        self.unchanged(a)
        a = """all(zip(a, b))"""
        self.unchanged(a)
        a = """sum(zip(a, b))"""
        self.unchanged(a)
        a = """sorted(zip(a, b))"""
        self.unchanged(a)
        a = """sorted(zip(a, b), key=blah)"""
        self.unchanged(a)
        a = """sorted(zip(a, b), key=blah)[0]"""
        self.unchanged(a)
        a = """for i in zip(a, b): pass"""
        self.unchanged(a)
        a = """[x for x in zip(a, b)]"""
        self.unchanged(a)
        a = """(x for x in zip(a, b))"""
        self.unchanged(a)

    def test_future_builtins(self):
        a = "from future_builtins import spam, zip, eggs; zip(a, b)"
        self.unchanged(a)

        b = """from future_builtins import spam, eggs; x = zip(a, b)"""
        a = """from future_builtins import spam, eggs; x = list(zip(a, b))"""
        self.check(b, a)

        a = "from future_builtins import *; zip(a, b)"
        self.unchanged(a)

class Test_standarderror(FixerTestCase):
    fixer = "standarderror"

    def test(self):
        b = """x =    StandardError()"""
        a = """x =    Exception()"""
        self.check(b, a)

        b = """x = StandardError(a, b, c)"""
        a = """x = Exception(a, b, c)"""
        self.check(b, a)

        b = """f(2 + StandardError(a, b, c))"""
        a = """f(2 + Exception(a, b, c))"""
        self.check(b, a)

class Test_types(FixerTestCase):
    fixer = "types"

    def test_basic_types_convert(self):
        b = """types.StringType"""
        a = """bytes"""
        self.check(b, a)

        b = """types.DictType"""
        a = """dict"""
        self.check(b, a)

        b = """types . IntType"""
        a = """int"""
        self.check(b, a)

        b = """types.ListType"""
        a = """list"""
        self.check(b, a)

        b = """types.LongType"""
        a = """int"""
        self.check(b, a)

        b = """types.NoneType"""
        a = """type(None)"""
        self.check(b, a)

class Test_idioms(FixerTestCase):
    fixer = "idioms"

    def test_while(self):
        b = """while 1: foo()"""
        a = """while True: foo()"""
        self.check(b, a)

        b = """while   1: foo()"""
        a = """while   True: foo()"""
        self.check(b, a)

        b = """
            while 1:
                foo()
            """
        a = """
            while True:
                foo()
            """
        self.check(b, a)

    def test_while_unchanged(self):
        s = """while 11: foo()"""
        self.unchanged(s)

        s = """while 0: foo()"""
        self.unchanged(s)

        s = """while foo(): foo()"""
        self.unchanged(s)

        s = """while []: foo()"""
        self.unchanged(s)

    def test_eq_simple(self):
        b = """type(x) == T"""
        a = """isinstance(x, T)"""
        self.check(b, a)

        b = """if   type(x) == T: pass"""
        a = """if   isinstance(x, T): pass"""
        self.check(b, a)

    def test_eq_reverse(self):
        b = """T == type(x)"""
        a = """isinstance(x, T)"""
        self.check(b, a)

        b = """if   T == type(x): pass"""
        a = """if   isinstance(x, T): pass"""
        self.check(b, a)

    def test_eq_expression(self):
        b = """type(x+y) == d.get('T')"""
        a = """isinstance(x+y, d.get('T'))"""
        self.check(b, a)

        b = """type(   x  +  y) == d.get('T')"""
        a = """isinstance(x  +  y, d.get('T'))"""
        self.check(b, a)

    def test_is_simple(self):
        b = """type(x) is T"""
        a = """isinstance(x, T)"""
        self.check(b, a)

        b = """if   type(x) is T: pass"""
        a = """if   isinstance(x, T): pass"""
        self.check(b, a)

    def test_is_reverse(self):
        b = """T is type(x)"""
        a = """isinstance(x, T)"""
        self.check(b, a)

        b = """if   T is type(x): pass"""
        a = """if   isinstance(x, T): pass"""
        self.check(b, a)

    def test_is_expression(self):
        b = """type(x+y) is d.get('T')"""
        a = """isinstance(x+y, d.get('T'))"""
        self.check(b, a)

        b = """type(   x  +  y) is d.get('T')"""
        a = """isinstance(x  +  y, d.get('T'))"""
        self.check(b, a)

    def test_is_not_simple(self):
        b = """type(x) is not T"""
        a = """not isinstance(x, T)"""
        self.check(b, a)

        b = """if   type(x) is not T: pass"""
        a = """if   not isinstance(x, T): pass"""
        self.check(b, a)

    def test_is_not_reverse(self):
        b = """T is not type(x)"""
        a = """not isinstance(x, T)"""
        self.check(b, a)

        b = """if   T is not type(x): pass"""
        a = """if   not isinstance(x, T): pass"""
        self.check(b, a)

    def test_is_not_expression(self):
        b = """type(x+y) is not d.get('T')"""
        a = """not isinstance(x+y, d.get('T'))"""
        self.check(b, a)

        b = """type(   x  +  y) is not d.get('T')"""
        a = """not isinstance(x  +  y, d.get('T'))"""
        self.check(b, a)

    def test_ne_simple(self):
        b = """type(x) != T"""
        a = """not isinstance(x, T)"""
        self.check(b, a)

        b = """if   type(x) != T: pass"""
        a = """if   not isinstance(x, T): pass"""
        self.check(b, a)

    def test_ne_reverse(self):
        b = """T != type(x)"""
        a = """not isinstance(x, T)"""
        self.check(b, a)

        b = """if   T != type(x): pass"""
        a = """if   not isinstance(x, T): pass"""
        self.check(b, a)

    def test_ne_expression(self):
        b = """type(x+y) != d.get('T')"""
        a = """not isinstance(x+y, d.get('T'))"""
        self.check(b, a)

        b = """type(   x  +  y) != d.get('T')"""
        a = """not isinstance(x  +  y, d.get('T'))"""
        self.check(b, a)

    def test_type_unchanged(self):
        a = """type(x).__name__"""
        self.unchanged(a)

    def test_sort_list_call(self):
        b = """
            v = list(t)
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(t)
            foo(v)
            """
        self.check(b, a)

        b = """
            v = list(foo(b) + d)
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(foo(b) + d)
            foo(v)
            """
        self.check(b, a)

        b = """
            while x:
                v = list(t)
                v.sort()
                foo(v)
            """
        a = """
            while x:
                v = sorted(t)
                foo(v)
            """
        self.check(b, a)

        b = """
            v = list(t)
            # foo
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(t)
            # foo
            foo(v)
            """
        self.check(b, a)

        b = r"""
            v = list(   t)
            v.sort()
            foo(v)
            """
        a = r"""
            v = sorted(   t)
            foo(v)
            """
        self.check(b, a)

    def test_sort_simple_expr(self):
        b = """
            v = t
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(t)
            foo(v)
            """
        self.check(b, a)

        b = """
            v = foo(b)
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(foo(b))
            foo(v)
            """
        self.check(b, a)

        b = """
            v = b.keys()
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(b.keys())
            foo(v)
            """
        self.check(b, a)

        b = """
            v = foo(b) + d
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(foo(b) + d)
            foo(v)
            """
        self.check(b, a)

        b = """
            while x:
                v = t
                v.sort()
                foo(v)
            """
        a = """
            while x:
                v = sorted(t)
                foo(v)
            """
        self.check(b, a)

        b = """
            v = t
            # foo
            v.sort()
            foo(v)
            """
        a = """
            v = sorted(t)
            # foo
            foo(v)
            """
        self.check(b, a)

        b = r"""
            v =   t
            v.sort()
            foo(v)
            """
        a = r"""
            v =   sorted(t)
            foo(v)
            """
        self.check(b, a)

    def test_sort_unchanged(self):
        s = """
            v = list(t)
            w.sort()
            foo(w)
            """
        self.unchanged(s)

        s = """
            v = list(t)
            v.sort(u)
            foo(v)
            """
        self.unchanged(s)

class Test_basestring(FixerTestCase):
    fixer = "basestring"

    def test_basestring(self):
        b = """isinstance(x, basestring)"""
        a = """isinstance(x, str)"""
        self.check(b, a)

class Test_buffer(FixerTestCase):
    fixer = "buffer"

    def test_buffer(self):
        b = """x = buffer(y)"""
        a = """x = memoryview(y)"""
        self.check(b, a)

class Test_future(FixerTestCase):
    fixer = "future"

    def test_future(self):
        b = """from __future__ import braces"""
        a = """"""
        self.check(b, a)

        b = """# comment\nfrom __future__ import braces"""
        a = """# comment\n"""
        self.check(b, a)

        b = """from __future__ import braces\n# comment"""
        a = """\n# comment"""
        self.check(b, a)

    def test_run_order(self):
        self.assert_runs_after('print')

class Test_itertools(FixerTestCase):
    fixer = "itertools"

    def checkall(self, before, after):
        # Because we need to check with and without the itertools prefix
        # and on each of the three functions, these loops make it all
        # much easier
        for i in ('itertools.', ''):
            for f in ('map', 'filter', 'zip'):
                b = before %(i+'i'+f)
                a = after %(f)
                self.check(b, a)

    def test_0(self):
        # A simple example -- test_1 covers exactly the same thing,
        # but it's not quite as clear.
        b = "itertools.izip(a, b)"
        a = "zip(a, b)"
        self.check(b, a)

    def test_1(self):
        b = """%s(f, a)"""
        a = """%s(f, a)"""
        self.checkall(b, a)

    def test_2(self):
        b = """itertools.ifilterfalse(a, b)"""
        a = """itertools.filterfalse(a, b)"""
        self.check(b, a)

    def test_4(self):
        b = """ifilterfalse(a, b)"""
        a = """filterfalse(a, b)"""
        self.check(b, a)

    def test_space_1(self):
        b = """    %s(f, a)"""
        a = """    %s(f, a)"""
        self.checkall(b, a)

    def test_space_2(self):
        b = """    itertools.ifilterfalse(a, b)"""
        a = """    itertools.filterfalse(a, b)"""
        self.check(b, a)

    def test_run_order(self):
        self.assert_runs_after('map', 'zip', 'filter')

class Test_itertools_imports(FixerTestCase):
    fixer = 'itertools_imports'

    def test_reduced(self):
        b = "from itertools import imap, izip, foo"
        a = "from itertools import foo"
        self.check(b, a)

        b = "from itertools import bar, imap, izip, foo"
        a = "from itertools import bar, foo"
        self.check(b, a)

    def test_comments(self):
        b = "#foo\nfrom itertools import imap, izip"
        a = "#foo\n"
        self.check(b, a)

    def test_none(self):
        b = "from itertools import imap, izip"
        a = ""
        self.check(b, a)

        b = "from itertools import izip"
        a = ""
        self.check(b, a)

    def test_import_as(self):
        b = "from itertools import izip, bar as bang, imap"
        a = "from itertools import bar as bang"
        self.check(b, a)

        s = "from itertools import bar as bang"
        self.unchanged(s)

    def test_ifilter(self):
        b = "from itertools import ifilterfalse"
        a = "from itertools import filterfalse"
        self.check(b, a)

        b = "from itertools import imap, ifilterfalse, foo"
        a = "from itertools import filterfalse, foo"
        self.check(b, a)

        b = "from itertools import bar, ifilterfalse, foo"
        a = "from itertools import bar, filterfalse, foo"
        self.check(b, a)


    def test_unchanged(self):
        s = "from itertools import foo"
        self.unchanged(s)

class Test_import(FixerTestCase):
    fixer = "import"

    def setUp(self):
        FixerTestCase.setUp(self)
        # Need to replace fix_import's isfile and isdir method
        # so we can check that it's doing the right thing
        self.files_checked = []
        self.always_exists = True
        def fake_exists(name):
            self.files_checked.append(name)
            return self.always_exists

        from ..fixes import fix_import
        fix_import.exists = fake_exists

    def check_both(self, b, a):
        self.always_exists = True
        FixerTestCase.check(self, b, a)
        self.always_exists = False
        FixerTestCase.unchanged(self, b)

    def test_files_checked(self):
        def p(path):
            # Takes a unix path and returns a path with correct separators
            return pathsep.join(path.split("/"))

        self.always_exists = False
        expected_extensions = ('.py', pathsep, '.pyc', '.so', '.sl', '.pyd')
        names_to_test = (p("/spam/eggs.py"), "ni.py", p("../../shrubbery.py"))

        for name in names_to_test:
            self.files_checked = []
            self.filename = name
            self.unchanged("import jam")

            if dirname(name): name = dirname(name) + '/jam'
            else:             name = 'jam'
            expected_checks = set(name + ext for ext in expected_extensions)

            self.failUnlessEqual(set(self.files_checked), expected_checks)

    def test_from(self):
        b = "from foo import bar, baz"
        a = "from .foo import bar, baz"
        self.check_both(b, a)

        b = "from foo import bar"
        a = "from .foo import bar"
        self.check_both(b, a)

    def test_dotted_from(self):
        b = "from green.eggs import ham"
        a = "from .green.eggs import ham"
        self.check_both(b, a)

    def test_from_as(self):
        b = "from green.eggs import ham as spam"
        a = "from .green.eggs import ham as spam"
        self.check_both(b, a)

    def test_import(self):
        b = "import foo"
        a = "from . import foo"
        self.check_both(b, a)

        b = "import foo, bar"
        a = "from . import foo, bar"
        self.check_both(b, a)

    def test_dotted_import(self):
        b = "import foo.bar"
        a = "from . import foo.bar"
        self.check_both(b, a)

    def test_dotted_import_as(self):
        b = "import foo.bar as bang"
        a = "from . import foo.bar as bang"
        self.check_both(b, a)


if __name__ == "__main__":
    import __main__
    support.run_all_tests(__main__)
