import inspect
import time
import types
import unittest

from unittest.mock import (
    call, _Call, create_autospec, MagicMock,
    Mock, ANY, _CallList, patch, PropertyMock, _callable
)

from datetime import datetime
from functools import partial

class SomeClass(object):
    def one(self, a, b): pass
    def two(self): pass
    def three(self, a=None): pass



class AnyTest(unittest.TestCase):

    def test_any(self):
        self.assertEqual(ANY, object())

        mock = Mock()
        mock(ANY)
        mock.assert_called_with(ANY)

        mock = Mock()
        mock(foo=ANY)
        mock.assert_called_with(foo=ANY)

    def test_repr(self):
        self.assertEqual(repr(ANY), '<ANY>')
        self.assertEqual(str(ANY), '<ANY>')


    def test_any_and_datetime(self):
        mock = Mock()
        mock(datetime.now(), foo=datetime.now())

        mock.assert_called_with(ANY, foo=ANY)


    def test_any_mock_calls_comparison_order(self):
        mock = Mock()
        class Foo(object):
            def __eq__(self, other): pass
            def __ne__(self, other): pass

        for d in datetime.now(), Foo():
            mock.reset_mock()

            mock(d, foo=d, bar=d)
            mock.method(d, zinga=d, alpha=d)
            mock().method(a1=d, z99=d)

            expected = [
                call(ANY, foo=ANY, bar=ANY),
                call.method(ANY, zinga=ANY, alpha=ANY),
                call(), call().method(a1=ANY, z99=ANY)
            ]
            self.assertEqual(expected, mock.mock_calls)
            self.assertEqual(mock.mock_calls, expected)

    def test_any_no_spec(self):
        # This is a regression test for bpo-37555
        class Foo:
            def __eq__(self, other): pass

        mock = Mock()
        mock(Foo(), 1)
        mock.assert_has_calls([call(ANY, 1)])
        mock.assert_called_with(ANY, 1)
        mock.assert_any_call(ANY, 1)

    def test_any_and_spec_set(self):
        # This is a regression test for bpo-37555
        class Foo:
            def __eq__(self, other): pass

        mock = Mock(spec=Foo)

        mock(Foo(), 1)
        mock.assert_has_calls([call(ANY, 1)])
        mock.assert_called_with(ANY, 1)
        mock.assert_any_call(ANY, 1)

class CallTest(unittest.TestCase):

    def test_call_with_call(self):
        kall = _Call()
        self.assertEqual(kall, _Call())
        self.assertEqual(kall, _Call(('',)))
        self.assertEqual(kall, _Call(((),)))
        self.assertEqual(kall, _Call(({},)))
        self.assertEqual(kall, _Call(('', ())))
        self.assertEqual(kall, _Call(('', {})))
        self.assertEqual(kall, _Call(('', (), {})))
        self.assertEqual(kall, _Call(('foo',)))
        self.assertEqual(kall, _Call(('bar', ())))
        self.assertEqual(kall, _Call(('baz', {})))
        self.assertEqual(kall, _Call(('spam', (), {})))

        kall = _Call(((1, 2, 3),))
        self.assertEqual(kall, _Call(((1, 2, 3),)))
        self.assertEqual(kall, _Call(('', (1, 2, 3))))
        self.assertEqual(kall, _Call(((1, 2, 3), {})))
        self.assertEqual(kall, _Call(('', (1, 2, 3), {})))

        kall = _Call(((1, 2, 4),))
        self.assertNotEqual(kall, _Call(('', (1, 2, 3))))
        self.assertNotEqual(kall, _Call(('', (1, 2, 3), {})))

        kall = _Call(('foo', (1, 2, 4),))
        self.assertNotEqual(kall, _Call(('', (1, 2, 4))))
        self.assertNotEqual(kall, _Call(('', (1, 2, 4), {})))
        self.assertNotEqual(kall, _Call(('bar', (1, 2, 4))))
        self.assertNotEqual(kall, _Call(('bar', (1, 2, 4), {})))

        kall = _Call(({'a': 3},))
        self.assertEqual(kall, _Call(('', (), {'a': 3})))
        self.assertEqual(kall, _Call(('', {'a': 3})))
        self.assertEqual(kall, _Call(((), {'a': 3})))
        self.assertEqual(kall, _Call(({'a': 3},)))


    def test_empty__Call(self):
        args = _Call()

        self.assertEqual(args, ())
        self.assertEqual(args, ('foo',))
        self.assertEqual(args, ((),))
        self.assertEqual(args, ('foo', ()))
        self.assertEqual(args, ('foo',(), {}))
        self.assertEqual(args, ('foo', {}))
        self.assertEqual(args, ({},))


    def test_named_empty_call(self):
        args = _Call(('foo', (), {}))

        self.assertEqual(args, ('foo',))
        self.assertEqual(args, ('foo', ()))
        self.assertEqual(args, ('foo',(), {}))
        self.assertEqual(args, ('foo', {}))

        self.assertNotEqual(args, ((),))
        self.assertNotEqual(args, ())
        self.assertNotEqual(args, ({},))
        self.assertNotEqual(args, ('bar',))
        self.assertNotEqual(args, ('bar', ()))
        self.assertNotEqual(args, ('bar', {}))


    def test_call_with_args(self):
        args = _Call(((1, 2, 3), {}))

        self.assertEqual(args, ((1, 2, 3),))
        self.assertEqual(args, ('foo', (1, 2, 3)))
        self.assertEqual(args, ('foo', (1, 2, 3), {}))
        self.assertEqual(args, ((1, 2, 3), {}))
        self.assertEqual(args.args, (1, 2, 3))
        self.assertEqual(args.kwargs, {})


    def test_named_call_with_args(self):
        args = _Call(('foo', (1, 2, 3), {}))

        self.assertEqual(args, ('foo', (1, 2, 3)))
        self.assertEqual(args, ('foo', (1, 2, 3), {}))
        self.assertEqual(args.args, (1, 2, 3))
        self.assertEqual(args.kwargs, {})

        self.assertNotEqual(args, ((1, 2, 3),))
        self.assertNotEqual(args, ((1, 2, 3), {}))


    def test_call_with_kwargs(self):
        args = _Call(((), dict(a=3, b=4)))

        self.assertEqual(args, (dict(a=3, b=4),))
        self.assertEqual(args, ('foo', dict(a=3, b=4)))
        self.assertEqual(args, ('foo', (), dict(a=3, b=4)))
        self.assertEqual(args, ((), dict(a=3, b=4)))
        self.assertEqual(args.args, ())
        self.assertEqual(args.kwargs, dict(a=3, b=4))


    def test_named_call_with_kwargs(self):
        args = _Call(('foo', (), dict(a=3, b=4)))

        self.assertEqual(args, ('foo', dict(a=3, b=4)))
        self.assertEqual(args, ('foo', (), dict(a=3, b=4)))
        self.assertEqual(args.args, ())
        self.assertEqual(args.kwargs, dict(a=3, b=4))

        self.assertNotEqual(args, (dict(a=3, b=4),))
        self.assertNotEqual(args, ((), dict(a=3, b=4)))


    def test_call_with_args_call_empty_name(self):
        args = _Call(((1, 2, 3), {}))

        self.assertEqual(args, call(1, 2, 3))
        self.assertEqual(call(1, 2, 3), args)
        self.assertIn(call(1, 2, 3), [args])


    def test_call_ne(self):
        self.assertNotEqual(_Call(((1, 2, 3),)), call(1, 2))
        self.assertFalse(_Call(((1, 2, 3),)) != call(1, 2, 3))
        self.assertTrue(_Call(((1, 2), {})) != call(1, 2, 3))


    def test_call_non_tuples(self):
        kall = _Call(((1, 2, 3),))
        for value in 1, None, self, int:
            self.assertNotEqual(kall, value)
            self.assertFalse(kall == value)


    def test_repr(self):
        self.assertEqual(repr(_Call()), 'call()')
        self.assertEqual(repr(_Call(('foo',))), 'call.foo()')

        self.assertEqual(repr(_Call(((1, 2, 3), {'a': 'b'}))),
                         "call(1, 2, 3, a='b')")
        self.assertEqual(repr(_Call(('bar', (1, 2, 3), {'a': 'b'}))),
                         "call.bar(1, 2, 3, a='b')")

        self.assertEqual(repr(call), 'call')
        self.assertEqual(str(call), 'call')

        self.assertEqual(repr(call()), 'call()')
        self.assertEqual(repr(call(1)), 'call(1)')
        self.assertEqual(repr(call(zz='thing')), "call(zz='thing')")

        self.assertEqual(repr(call().foo), 'call().foo')
        self.assertEqual(repr(call(1).foo.bar(a=3).bing),
                         'call().foo.bar().bing')
        self.assertEqual(
            repr(call().foo(1, 2, a=3)),
            "call().foo(1, 2, a=3)"
        )
        self.assertEqual(repr(call()()), "call()()")
        self.assertEqual(repr(call(1)(2)), "call()(2)")
        self.assertEqual(
            repr(call()().bar().baz.beep(1)),
            "call()().bar().baz.beep(1)"
        )


    def test_call(self):
        self.assertEqual(call(), ('', (), {}))
        self.assertEqual(call('foo', 'bar', one=3, two=4),
                         ('', ('foo', 'bar'), {'one': 3, 'two': 4}))

        mock = Mock()
        mock(1, 2, 3)
        mock(a=3, b=6)
        self.assertEqual(mock.call_args_list,
                         [call(1, 2, 3), call(a=3, b=6)])

    def test_attribute_call(self):
        self.assertEqual(call.foo(1), ('foo', (1,), {}))
        self.assertEqual(call.bar.baz(fish='eggs'),
                         ('bar.baz', (), {'fish': 'eggs'}))

        mock = Mock()
        mock.foo(1, 2 ,3)
        mock.bar.baz(a=3, b=6)
        self.assertEqual(mock.method_calls,
                         [call.foo(1, 2, 3), call.bar.baz(a=3, b=6)])


    def test_extended_call(self):
        result = call(1).foo(2).bar(3, a=4)
        self.assertEqual(result, ('().foo().bar', (3,), dict(a=4)))

        mock = MagicMock()
        mock(1, 2, a=3, b=4)
        self.assertEqual(mock.call_args, call(1, 2, a=3, b=4))
        self.assertNotEqual(mock.call_args, call(1, 2, 3))

        self.assertEqual(mock.call_args_list, [call(1, 2, a=3, b=4)])
        self.assertEqual(mock.mock_calls, [call(1, 2, a=3, b=4)])

        mock = MagicMock()
        mock.foo(1).bar()().baz.beep(a=6)

        last_call = call.foo(1).bar()().baz.beep(a=6)
        self.assertEqual(mock.mock_calls[-1], last_call)
        self.assertEqual(mock.mock_calls, last_call.call_list())


    def test_extended_not_equal(self):
        a = call(x=1).foo
        b = call(x=2).foo
        self.assertEqual(a, a)
        self.assertEqual(b, b)
        self.assertNotEqual(a, b)


    def test_nested_calls_not_equal(self):
        a = call(x=1).foo().bar
        b = call(x=2).foo().bar
        self.assertEqual(a, a)
        self.assertEqual(b, b)
        self.assertNotEqual(a, b)


    def test_call_list(self):
        mock = MagicMock()
        mock(1)
        self.assertEqual(call(1).call_list(), mock.mock_calls)

        mock = MagicMock()
        mock(1).method(2)
        self.assertEqual(call(1).method(2).call_list(),
                         mock.mock_calls)

        mock = MagicMock()
        mock(1).method(2)(3)
        self.assertEqual(call(1).method(2)(3).call_list(),
                         mock.mock_calls)

        mock = MagicMock()
        int(mock(1).method(2)(3).foo.bar.baz(4)(5))
        kall = call(1).method(2)(3).foo.bar.baz(4)(5).__int__()
        self.assertEqual(kall.call_list(), mock.mock_calls)


    def test_call_any(self):
        self.assertEqual(call, ANY)

        m = MagicMock()
        int(m)
        self.assertEqual(m.mock_calls, [ANY])
        self.assertEqual([ANY], m.mock_calls)


    def test_two_args_call(self):
        args = _Call(((1, 2), {'a': 3}), two=True)
        self.assertEqual(len(args), 2)
        self.assertEqual(args[0], (1, 2))
        self.assertEqual(args[1], {'a': 3})

        other_args = _Call(((1, 2), {'a': 3}))
        self.assertEqual(args, other_args)

    def test_call_with_name(self):
        self.assertEqual(_Call((), 'foo')[0], 'foo')
        self.assertEqual(_Call((('bar', 'barz'),),)[0], '')
        self.assertEqual(_Call((('bar', 'barz'), {'hello': 'world'}),)[0], '')

    def test_dunder_call(self):
        m = MagicMock()
        m().foo()['bar']()
        self.assertEqual(
            m.mock_calls,
            [call(), call().foo(), call().foo().__getitem__('bar'), call().foo().__getitem__()()]
        )
        m = MagicMock()
        m().foo()['bar'] = 1
        self.assertEqual(
            m.mock_calls,
            [call(), call().foo(), call().foo().__setitem__('bar', 1)]
        )
        m = MagicMock()
        iter(m().foo())
        self.assertEqual(
            m.mock_calls,
            [call(), call().foo(), call().foo().__iter__()]
        )


class SpecSignatureTest(unittest.TestCase):

    def _check_someclass_mock(self, mock):
        self.assertRaises(AttributeError, getattr, mock, 'foo')
        mock.one(1, 2)
        mock.one.assert_called_with(1, 2)
        self.assertRaises(AssertionError,
                          mock.one.assert_called_with, 3, 4)
        self.assertRaises(TypeError, mock.one, 1)

        mock.two()
        mock.two.assert_called_with()
        self.assertRaises(AssertionError,
                          mock.two.assert_called_with, 3)
        self.assertRaises(TypeError, mock.two, 1)

        mock.three()
        mock.three.assert_called_with()
        self.assertRaises(AssertionError,
                          mock.three.assert_called_with, 3)
        self.assertRaises(TypeError, mock.three, 3, 2)

        mock.three(1)
        mock.three.assert_called_with(1)

        mock.three(a=1)
        mock.three.assert_called_with(a=1)


    def test_basic(self):
        mock = create_autospec(SomeClass)
        self._check_someclass_mock(mock)
        mock = create_autospec(SomeClass())
        self._check_someclass_mock(mock)


    def test_create_autospec_return_value(self):
        def f(): pass
        mock = create_autospec(f, return_value='foo')
        self.assertEqual(mock(), 'foo')

        class Foo(object):
            pass

        mock = create_autospec(Foo, return_value='foo')
        self.assertEqual(mock(), 'foo')


    def test_autospec_reset_mock(self):
        m = create_autospec(int)
        int(m)
        m.reset_mock()
        self.assertEqual(m.__int__.call_count, 0)


    def test_mocking_unbound_methods(self):
        class Foo(object):
            def foo(self, foo): pass
        p = patch.object(Foo, 'foo')
        mock_foo = p.start()
        Foo().foo(1)

        mock_foo.assert_called_with(1)


    def test_create_autospec_keyword_arguments(self):
        class Foo(object):
            a = 3
        m = create_autospec(Foo, a='3')
        self.assertEqual(m.a, '3')


    def test_create_autospec_keyword_only_arguments(self):
        def foo(a, *, b=None): pass

        m = create_autospec(foo)
        m(1)
        m.assert_called_with(1)
        self.assertRaises(TypeError, m, 1, 2)

        m(2, b=3)
        m.assert_called_with(2, b=3)


    def test_function_as_instance_attribute(self):
        obj = SomeClass()
        def f(a): pass
        obj.f = f

        mock = create_autospec(obj)
        mock.f('bing')
        mock.f.assert_called_with('bing')


    def test_spec_as_list(self):
        # because spec as a list of strings in the mock constructor means
        # something very different we treat a list instance as the type.
        mock = create_autospec([])
        mock.append('foo')
        mock.append.assert_called_with('foo')

        self.assertRaises(AttributeError, getattr, mock, 'foo')

        class Foo(object):
            foo = []

        mock = create_autospec(Foo)
        mock.foo.append(3)
        mock.foo.append.assert_called_with(3)
        self.assertRaises(AttributeError, getattr, mock.foo, 'foo')


    def test_attributes(self):
        class Sub(SomeClass):
            attr = SomeClass()

        sub_mock = create_autospec(Sub)

        for mock in (sub_mock, sub_mock.attr):
            self._check_someclass_mock(mock)


    def test_spec_has_descriptor_returning_function(self):

        class CrazyDescriptor(object):

            def __get__(self, obj, type_):
                if obj is None:
                    return lambda x: None

        class MyClass(object):

            some_attr = CrazyDescriptor()

        mock = create_autospec(MyClass)
        mock.some_attr(1)
        with self.assertRaises(TypeError):
            mock.some_attr()
        with self.assertRaises(TypeError):
            mock.some_attr(1, 2)


    def test_spec_has_function_not_in_bases(self):

        class CrazyClass(object):

            def __dir__(self):
                return super(CrazyClass, self).__dir__()+['crazy']

            def __getattr__(self, item):
                if item == 'crazy':
                    return lambda x: x
                raise AttributeError(item)

        inst = CrazyClass()
        with self.assertRaises(AttributeError):
            inst.other
        self.assertEqual(inst.crazy(42), 42)

        mock = create_autospec(inst)
        mock.crazy(42)
        with self.assertRaises(TypeError):
            mock.crazy()
        with self.assertRaises(TypeError):
            mock.crazy(1, 2)


    def test_builtin_functions_types(self):
        # we could replace builtin functions / methods with a function
        # with *args / **kwargs signature. Using the builtin method type
        # as a spec seems to work fairly well though.
        class BuiltinSubclass(list):
            def bar(self, arg): pass
            sorted = sorted
            attr = {}

        mock = create_autospec(BuiltinSubclass)
        mock.append(3)
        mock.append.assert_called_with(3)
        self.assertRaises(AttributeError, getattr, mock.append, 'foo')

        mock.bar('foo')
        mock.bar.assert_called_with('foo')
        self.assertRaises(TypeError, mock.bar, 'foo', 'bar')
        self.assertRaises(AttributeError, getattr, mock.bar, 'foo')

        mock.sorted([1, 2])
        mock.sorted.assert_called_with([1, 2])
        self.assertRaises(AttributeError, getattr, mock.sorted, 'foo')

        mock.attr.pop(3)
        mock.attr.pop.assert_called_with(3)
        self.assertRaises(AttributeError, getattr, mock.attr, 'foo')


    def test_method_calls(self):
        class Sub(SomeClass):
            attr = SomeClass()

        mock = create_autospec(Sub)
        mock.one(1, 2)
        mock.two()
        mock.three(3)

        expected = [call.one(1, 2), call.two(), call.three(3)]
        self.assertEqual(mock.method_calls, expected)

        mock.attr.one(1, 2)
        mock.attr.two()
        mock.attr.three(3)

        expected.extend(
            [call.attr.one(1, 2), call.attr.two(), call.attr.three(3)]
        )
        self.assertEqual(mock.method_calls, expected)


    def test_magic_methods(self):
        class BuiltinSubclass(list):
            attr = {}

        mock = create_autospec(BuiltinSubclass)
        self.assertEqual(list(mock), [])
        self.assertRaises(TypeError, int, mock)
        self.assertRaises(TypeError, int, mock.attr)
        self.assertEqual(list(mock), [])

        self.assertIsInstance(mock['foo'], MagicMock)
        self.assertIsInstance(mock.attr['foo'], MagicMock)


    def test_spec_set(self):
        class Sub(SomeClass):
            attr = SomeClass()

        for spec in (Sub, Sub()):
            mock = create_autospec(spec, spec_set=True)
            self._check_someclass_mock(mock)

            self.assertRaises(AttributeError, setattr, mock, 'foo', 'bar')
            self.assertRaises(AttributeError, setattr, mock.attr, 'foo', 'bar')


    def test_descriptors(self):
        class Foo(object):
            @classmethod
            def f(cls, a, b): pass
            @staticmethod
            def g(a, b): pass

        class Bar(Foo): pass

        class Baz(SomeClass, Bar): pass

        for spec in (Foo, Foo(), Bar, Bar(), Baz, Baz()):
            mock = create_autospec(spec)
            mock.f(1, 2)
            mock.f.assert_called_once_with(1, 2)

            mock.g(3, 4)
            mock.g.assert_called_once_with(3, 4)


    def test_recursive(self):
        class A(object):
            def a(self): pass
            foo = 'foo bar baz'
            bar = foo

        A.B = A
        mock = create_autospec(A)

        mock()
        self.assertFalse(mock.B.called)

        mock.a()
        mock.B.a()
        self.assertEqual(mock.method_calls, [call.a(), call.B.a()])

        self.assertIs(A.foo, A.bar)
        self.assertIsNot(mock.foo, mock.bar)
        mock.foo.lower()
        self.assertRaises(AssertionError, mock.bar.lower.assert_called_with)


    def test_spec_inheritance_for_classes(self):
        class Foo(object):
            def a(self, x): pass
            class Bar(object):
                def f(self, y): pass

        class_mock = create_autospec(Foo)

        self.assertIsNot(class_mock, class_mock())

        for this_mock in class_mock, class_mock():
            this_mock.a(x=5)
            this_mock.a.assert_called_with(x=5)
            this_mock.a.assert_called_with(5)
            self.assertRaises(TypeError, this_mock.a, 'foo', 'bar')
            self.assertRaises(AttributeError, getattr, this_mock, 'b')

        instance_mock = create_autospec(Foo())
        instance_mock.a(5)
        instance_mock.a.assert_called_with(5)
        instance_mock.a.assert_called_with(x=5)
        self.assertRaises(TypeError, instance_mock.a, 'foo', 'bar')
        self.assertRaises(AttributeError, getattr, instance_mock, 'b')

        # The return value isn't isn't callable
        self.assertRaises(TypeError, instance_mock)

        instance_mock.Bar.f(6)
        instance_mock.Bar.f.assert_called_with(6)
        instance_mock.Bar.f.assert_called_with(y=6)
        self.assertRaises(AttributeError, getattr, instance_mock.Bar, 'g')

        instance_mock.Bar().f(6)
        instance_mock.Bar().f.assert_called_with(6)
        instance_mock.Bar().f.assert_called_with(y=6)
        self.assertRaises(AttributeError, getattr, instance_mock.Bar(), 'g')


    def test_inherit(self):
        class Foo(object):
            a = 3

        Foo.Foo = Foo

        # class
        mock = create_autospec(Foo)
        instance = mock()
        self.assertRaises(AttributeError, getattr, instance, 'b')

        attr_instance = mock.Foo()
        self.assertRaises(AttributeError, getattr, attr_instance, 'b')

        # instance
        mock = create_autospec(Foo())
        self.assertRaises(AttributeError, getattr, mock, 'b')
        self.assertRaises(TypeError, mock)

        # attribute instance
        call_result = mock.Foo()
        self.assertRaises(AttributeError, getattr, call_result, 'b')


    def test_builtins(self):
        # used to fail with infinite recursion
        create_autospec(1)

        create_autospec(int)
        create_autospec('foo')
        create_autospec(str)
        create_autospec({})
        create_autospec(dict)
        create_autospec([])
        create_autospec(list)
        create_autospec(set())
        create_autospec(set)
        create_autospec(1.0)
        create_autospec(float)
        create_autospec(1j)
        create_autospec(complex)
        create_autospec(False)
        create_autospec(True)


    def test_function(self):
        def f(a, b): pass

        mock = create_autospec(f)
        self.assertRaises(TypeError, mock)
        mock(1, 2)
        mock.assert_called_with(1, 2)
        mock.assert_called_with(1, b=2)
        mock.assert_called_with(a=1, b=2)

        f.f = f
        mock = create_autospec(f)
        self.assertRaises(TypeError, mock.f)
        mock.f(3, 4)
        mock.f.assert_called_with(3, 4)
        mock.f.assert_called_with(a=3, b=4)


    def test_skip_attributeerrors(self):
        class Raiser(object):
            def __get__(self, obj, type=None):
                if obj is None:
                    raise AttributeError('Can only be accessed via an instance')

        class RaiserClass(object):
            raiser = Raiser()

            @staticmethod
            def existing(a, b):
                return a + b

        self.assertEqual(RaiserClass.existing(1, 2), 3)
        s = create_autospec(RaiserClass)
        self.assertRaises(TypeError, lambda x: s.existing(1, 2, 3))
        self.assertEqual(s.existing(1, 2), s.existing.return_value)
        self.assertRaises(AttributeError, lambda: s.nonexisting)

        # check we can fetch the raiser attribute and it has no spec
        obj = s.raiser
        obj.foo, obj.bar


    def test_signature_class(self):
        class Foo(object):
            def __init__(self, a, b=3): pass

        mock = create_autospec(Foo)

        self.assertRaises(TypeError, mock)
        mock(1)
        mock.assert_called_once_with(1)
        mock.assert_called_once_with(a=1)
        self.assertRaises(AssertionError, mock.assert_called_once_with, 2)

        mock(4, 5)
        mock.assert_called_with(4, 5)
        mock.assert_called_with(a=4, b=5)
        self.assertRaises(AssertionError, mock.assert_called_with, a=5, b=4)


    def test_class_with_no_init(self):
        # this used to raise an exception
        # due to trying to get a signature from object.__init__
        class Foo(object):
            pass
        create_autospec(Foo)


    def test_signature_callable(self):
        class Callable(object):
            def __init__(self, x, y): pass
            def __call__(self, a): pass

        mock = create_autospec(Callable)
        mock(1, 2)
        mock.assert_called_once_with(1, 2)
        mock.assert_called_once_with(x=1, y=2)
        self.assertRaises(TypeError, mock, 'a')

        instance = mock(1, 2)
        self.assertRaises(TypeError, instance)
        instance(a='a')
        instance.assert_called_once_with('a')
        instance.assert_called_once_with(a='a')
        instance('a')
        instance.assert_called_with('a')
        instance.assert_called_with(a='a')

        mock = create_autospec(Callable(1, 2))
        mock(a='a')
        mock.assert_called_once_with(a='a')
        self.assertRaises(TypeError, mock)
        mock('a')
        mock.assert_called_with('a')


    def test_signature_noncallable(self):
        class NonCallable(object):
            def __init__(self):
                pass

        mock = create_autospec(NonCallable)
        instance = mock()
        mock.assert_called_once_with()
        self.assertRaises(TypeError, mock, 'a')
        self.assertRaises(TypeError, instance)
        self.assertRaises(TypeError, instance, 'a')

        mock = create_autospec(NonCallable())
        self.assertRaises(TypeError, mock)
        self.assertRaises(TypeError, mock, 'a')


    def test_create_autospec_none(self):
        class Foo(object):
            bar = None

        mock = create_autospec(Foo)
        none = mock.bar
        self.assertNotIsInstance(none, type(None))

        none.foo()
        none.foo.assert_called_once_with()


    def test_autospec_functions_with_self_in_odd_place(self):
        class Foo(object):
            def f(a, self): pass

        a = create_autospec(Foo)
        a.f(10)
        a.f.assert_called_with(10)
        a.f.assert_called_with(self=10)
        a.f(self=10)
        a.f.assert_called_with(10)
        a.f.assert_called_with(self=10)


    def test_autospec_data_descriptor(self):
        class Descriptor(object):
            def __init__(self, value):
                self.value = value

            def __get__(self, obj, cls=None):
                return self

            def __set__(self, obj, value): pass

        class MyProperty(property):
            pass

        class Foo(object):
            __slots__ = ['slot']

            @property
            def prop(self): pass

            @MyProperty
            def subprop(self): pass

            desc = Descriptor(42)

        foo = create_autospec(Foo)

        def check_data_descriptor(mock_attr):
            # Data descriptors don't have a spec.
            self.assertIsInstance(mock_attr, MagicMock)
            mock_attr(1, 2, 3)
            mock_attr.abc(4, 5, 6)
            mock_attr.assert_called_once_with(1, 2, 3)
            mock_attr.abc.assert_called_once_with(4, 5, 6)

        # property
        check_data_descriptor(foo.prop)
        # property subclass
        check_data_descriptor(foo.subprop)
        # class __slot__
        check_data_descriptor(foo.slot)
        # plain data descriptor
        check_data_descriptor(foo.desc)


    def test_autospec_on_bound_builtin_function(self):
        meth = types.MethodType(time.ctime, time.time())
        self.assertIsInstance(meth(), str)
        mocked = create_autospec(meth)

        # no signature, so no spec to check against
        mocked()
        mocked.assert_called_once_with()
        mocked.reset_mock()
        mocked(4, 5, 6)
        mocked.assert_called_once_with(4, 5, 6)


    def test_autospec_getattr_partial_function(self):
        # bpo-32153 : getattr returning partial functions without
        # __name__ should not create AttributeError in create_autospec
        class Foo:

            def __getattr__(self, attribute):
                return partial(lambda name: name, attribute)

        proxy = Foo()
        autospec = create_autospec(proxy)
        self.assertFalse(hasattr(autospec, '__name__'))


    def test_autospec_signature_staticmethod(self):
        class Foo:
            @staticmethod
            def static_method(a, b=10, *, c): pass

        mock = create_autospec(Foo.__dict__['static_method'])
        self.assertEqual(inspect.signature(Foo.static_method), inspect.signature(mock))


    def test_autospec_signature_classmethod(self):
        class Foo:
            @classmethod
            def class_method(cls, a, b=10, *, c): pass

        mock = create_autospec(Foo.__dict__['class_method'])
        self.assertEqual(inspect.signature(Foo.class_method), inspect.signature(mock))


    def test_spec_inspect_signature(self):

        def myfunc(x, y): pass

        mock = create_autospec(myfunc)
        mock(1, 2)
        mock(x=1, y=2)

        self.assertEqual(inspect.signature(mock), inspect.signature(myfunc))
        self.assertEqual(mock.mock_calls, [call(1, 2), call(x=1, y=2)])
        self.assertRaises(TypeError, mock, 1)


    def test_spec_inspect_signature_annotations(self):

        def foo(a: int, b: int=10, *, c:int) -> int:
            return a + b + c

        self.assertEqual(foo(1, 2 , c=3), 6)
        mock = create_autospec(foo)
        mock(1, 2, c=3)
        mock(1, c=3)

        self.assertEqual(inspect.signature(mock), inspect.signature(foo))
        self.assertEqual(mock.mock_calls, [call(1, 2, c=3), call(1, c=3)])
        self.assertRaises(TypeError, mock, 1)
        self.assertRaises(TypeError, mock, 1, 2, 3, c=4)


    def test_spec_function_no_name(self):
        func = lambda: 'nope'
        mock = create_autospec(func)
        self.assertEqual(mock.__name__, 'funcopy')


    def test_spec_function_assert_has_calls(self):
        def f(a): pass
        mock = create_autospec(f)
        mock(1)
        mock.assert_has_calls([call(1)])
        with self.assertRaises(AssertionError):
            mock.assert_has_calls([call(2)])


    def test_spec_function_assert_any_call(self):
        def f(a): pass
        mock = create_autospec(f)
        mock(1)
        mock.assert_any_call(1)
        with self.assertRaises(AssertionError):
            mock.assert_any_call(2)


    def test_spec_function_reset_mock(self):
        def f(a): pass
        rv = Mock()
        mock = create_autospec(f, return_value=rv)
        mock(1)(2)
        self.assertEqual(mock.mock_calls, [call(1)])
        self.assertEqual(rv.mock_calls, [call(2)])
        mock.reset_mock()
        self.assertEqual(mock.mock_calls, [])
        self.assertEqual(rv.mock_calls, [])


class TestCallList(unittest.TestCase):

    def test_args_list_contains_call_list(self):
        mock = Mock()
        self.assertIsInstance(mock.call_args_list, _CallList)

        mock(1, 2)
        mock(a=3)
        mock(3, 4)
        mock(b=6)

        for kall in call(1, 2), call(a=3), call(3, 4), call(b=6):
            self.assertIn(kall, mock.call_args_list)

        calls = [call(a=3), call(3, 4)]
        self.assertIn(calls, mock.call_args_list)
        calls = [call(1, 2), call(a=3)]
        self.assertIn(calls, mock.call_args_list)
        calls = [call(3, 4), call(b=6)]
        self.assertIn(calls, mock.call_args_list)
        calls = [call(3, 4)]
        self.assertIn(calls, mock.call_args_list)

        self.assertNotIn(call('fish'), mock.call_args_list)
        self.assertNotIn([call('fish')], mock.call_args_list)


    def test_call_list_str(self):
        mock = Mock()
        mock(1, 2)
        mock.foo(a=3)
        mock.foo.bar().baz('fish', cat='dog')

        expected = (
            "[call(1, 2),\n"
            " call.foo(a=3),\n"
            " call.foo.bar(),\n"
            " call.foo.bar().baz('fish', cat='dog')]"
        )
        self.assertEqual(str(mock.mock_calls), expected)


    def test_propertymock(self):
        p = patch('%s.SomeClass.one' % __name__, new_callable=PropertyMock)
        mock = p.start()
        try:
            SomeClass.one
            mock.assert_called_once_with()

            s = SomeClass()
            s.one
            mock.assert_called_with()
            self.assertEqual(mock.mock_calls, [call(), call()])

            s.one = 3
            self.assertEqual(mock.mock_calls, [call(), call(), call(3)])
        finally:
            p.stop()


    def test_propertymock_bare(self):
        m = MagicMock()
        p = PropertyMock()
        type(m).foo = p

        returned = m.foo
        p.assert_called_once_with()
        self.assertIsInstance(returned, MagicMock)
        self.assertNotIsInstance(returned, PropertyMock)


    def test_propertymock_returnvalue(self):
        m = MagicMock()
        p = PropertyMock(return_value=42)
        type(m).foo = p

        returned = m.foo
        p.assert_called_once_with()
        self.assertEqual(returned, 42)
        self.assertNotIsInstance(returned, PropertyMock)


    def test_propertymock_side_effect(self):
        m = MagicMock()
        p = PropertyMock(side_effect=ValueError)
        type(m).foo = p

        with self.assertRaises(ValueError):
            m.foo
        p.assert_called_once_with()


    def test_propertymock_attach(self):
        m = Mock()
        p = PropertyMock()
        type(m).foo = p
        m.attach_mock(p, 'foo')
        self.assertEqual(m.mock_calls, [])


class TestCallablePredicate(unittest.TestCase):

    def test_type(self):
        for obj in [str, bytes, int, list, tuple, SomeClass]:
            self.assertTrue(_callable(obj))

    def test_call_magic_method(self):
        class Callable:
            def __call__(self): pass
        instance = Callable()
        self.assertTrue(_callable(instance))

    def test_staticmethod(self):
        class WithStaticMethod:
            @staticmethod
            def staticfunc(): pass
        self.assertTrue(_callable(WithStaticMethod.staticfunc))

    def test_non_callable_staticmethod(self):
        class BadStaticMethod:
            not_callable = staticmethod(None)
        self.assertFalse(_callable(BadStaticMethod.not_callable))

    def test_classmethod(self):
        class WithClassMethod:
            @classmethod
            def classfunc(cls): pass
        self.assertTrue(_callable(WithClassMethod.classfunc))

    def test_non_callable_classmethod(self):
        class BadClassMethod:
            not_callable = classmethod(None)
        self.assertFalse(_callable(BadClassMethod.not_callable))


if __name__ == '__main__':
    unittest.main()
