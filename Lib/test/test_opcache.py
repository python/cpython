import copy
import pickle
import dis
import threading
import types
import unittest
from test.support import threading_helper, check_impl_detail, requires_specialization
from test.support.import_helper import import_module

# Skip this module on other interpreters, it is cpython specific:
if check_impl_detail(cpython=False):
    raise unittest.SkipTest('implementation detail specific to cpython')

_testinternalcapi = import_module("_testinternalcapi")


def disabling_optimizer(func):
    def wrapper(*args, **kwargs):
        if not hasattr(_testinternalcapi, "get_optimizer"):
            return func(*args, **kwargs)
        old_opt = _testinternalcapi.get_optimizer()
        _testinternalcapi.set_optimizer(None)
        try:
            return func(*args, **kwargs)
        finally:
            _testinternalcapi.set_optimizer(old_opt)

    return wrapper


class TestBase(unittest.TestCase):
    def assert_specialized(self, f, opname):
        instructions = dis.get_instructions(f, adaptive=True)
        opnames = {instruction.opname for instruction in instructions}
        self.assertIn(opname, opnames)


class TestLoadSuperAttrCache(unittest.TestCase):
    def test_descriptor_not_double_executed_on_spec_fail(self):
        calls = []
        class Descriptor:
            def __get__(self, instance, owner):
                calls.append((instance, owner))
                return lambda: 1

        class C:
            d = Descriptor()

        class D(C):
            def f(self):
                return super().d()

        d = D()

        self.assertEqual(d.f(), 1)  # warmup
        calls.clear()
        self.assertEqual(d.f(), 1)  # try to specialize
        self.assertEqual(calls, [(d, D)])


class TestLoadAttrCache(unittest.TestCase):
    def test_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class C:
            def __init__(self):
                self.x = 1
            x = Descriptor()

        def f(o):
            return o.x

        o = C()
        for i in range(1025):
            assert f(o) == 1

        Descriptor.__get__ = lambda self, instance, value: 2
        Descriptor.__set__ = lambda *args: None

        self.assertEqual(f(o), 2)

    def test_metaclass_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class Metaclass(type):
            attribute = Descriptor()

        class Class(metaclass=Metaclass):
            attribute = True

        def __get__(self, instance, owner):
            return False

        def __set__(self, instance, value):
            return None

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(1025):
            self.assertFalse(f())

    def test_metaclass_descriptor_shadows_class_attribute(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_set_descriptor_after_optimization(self):
        class Metaclass(type):
            pass

        class Class(metaclass=Metaclass):
            attribute = True

        @property
        def attribute(self):
            return False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        Metaclass.attribute = attribute

        for _ in range(1025):
            self.assertFalse(f())

    def test_metaclass_del_descriptor_after_optimization(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        del Metaclass.attribute

        for _ in range(1025):
            self.assertFalse(f())

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            mro = None

        def f():
            return Class.mro

        for _ in range(1025):
            self.assertIsNone(f())

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            __base__ = None

        def f():
            return Class.__base__

        for _ in range(1025):
            self.assertIs(f(), object)

    def test_type_descriptor_shadows_attribute_getset(self):
        class Class:
            __name__ = "Spam"

        def f():
            return Class.__name__

        for _ in range(1025):
            self.assertEqual(f(), "Class")

    def test_metaclass_getattribute(self):
        class Metaclass(type):
            def __getattribute__(self, name):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_swap(self):
        class OldMetaclass(type):
            @property
            def attribute(self):
                return True

        class NewMetaclass(type):
            @property
            def attribute(self):
                return False

        class Class(metaclass=OldMetaclass):
            pass

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        Class.__class__ = NewMetaclass

        for _ in range(1025):
            self.assertFalse(f())

    def test_load_shadowing_slot_should_raise_type_error(self):
        class Class:
            __slots__ = ("slot",)

        class Sneaky:
            __slots__ = ("shadowed",)
            shadowing = Class.slot

        def f(o):
            o.shadowing

        o = Sneaky()
        o.shadowed = 42

        for _ in range(1025):
            with self.assertRaises(TypeError):
                f(o)

    def test_store_shadowing_slot_should_raise_type_error(self):
        class Class:
            __slots__ = ("slot",)

        class Sneaky:
            __slots__ = ("shadowed",)
            shadowing = Class.slot

        def f(o):
            o.shadowing = 42

        o = Sneaky()

        for _ in range(1025):
            with self.assertRaises(TypeError):
                f(o)

    def test_load_borrowed_slot_should_not_crash(self):
        class Class:
            __slots__ = ("slot",)

        class Sneaky:
            borrowed = Class.slot

        def f(o):
            o.borrowed

        o = Sneaky()

        for _ in range(1025):
            with self.assertRaises(TypeError):
                f(o)

    def test_store_borrowed_slot_should_not_crash(self):
        class Class:
            __slots__ = ("slot",)

        class Sneaky:
            borrowed = Class.slot

        def f(o):
            o.borrowed = 42

        o = Sneaky()

        for _ in range(1025):
            with self.assertRaises(TypeError):
                f(o)


class TestLoadMethodCache(unittest.TestCase):
    def test_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class Class:
            attribute = Descriptor()

        def __get__(self, instance, owner):
            return lambda: False

        def __set__(self, instance, value):
            return None

        def attribute():
            return True

        instance = Class()
        instance.attribute = attribute

        def f():
            return instance.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(1025):
            self.assertFalse(f())

    def test_metaclass_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class Metaclass(type):
            attribute = Descriptor()

        class Class(metaclass=Metaclass):
            def attribute():
                return True

        def __get__(self, instance, owner):
            return lambda: False

        def __set__(self, instance, value):
            return None

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(1025):
            self.assertFalse(f())

    def test_metaclass_descriptor_shadows_class_attribute(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return lambda: True

        class Class(metaclass=Metaclass):
            def attribute():
                return False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_set_descriptor_after_optimization(self):
        class Metaclass(type):
            pass

        class Class(metaclass=Metaclass):
            def attribute():
                return True

        @property
        def attribute(self):
            return lambda: False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Metaclass.attribute = attribute

        for _ in range(1025):
            self.assertFalse(f())

    def test_metaclass_del_descriptor_after_optimization(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return lambda: True

        class Class(metaclass=Metaclass):
            def attribute():
                return False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        del Metaclass.attribute

        for _ in range(1025):
            self.assertFalse(f())

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            def mro():
                return ["Spam", "eggs"]

        def f():
            return Class.mro()

        for _ in range(1025):
            self.assertEqual(f(), ["Spam", "eggs"])

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            def __base__():
                return "Spam"

        def f():
            return Class.__base__()

        for _ in range(1025):
            self.assertNotEqual(f(), "Spam")

    def test_metaclass_getattribute(self):
        class Metaclass(type):
            def __getattribute__(self, name):
                return lambda: True

        class Class(metaclass=Metaclass):
            def attribute():
                return False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_swap(self):
        class OldMetaclass(type):
            @property
            def attribute(self):
                return lambda: True

        class NewMetaclass(type):
            @property
            def attribute(self):
                return lambda: False

        class Class(metaclass=OldMetaclass):
            pass

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Class.__class__ = NewMetaclass

        for _ in range(1025):
            self.assertFalse(f())


class TestCallCache(TestBase):
    def test_too_many_defaults_0(self):
        def f():
            pass

        f.__defaults__ = (None,)
        for _ in range(1025):
            f()

    def test_too_many_defaults_1(self):
        def f(x):
            pass

        f.__defaults__ = (None, None)
        for _ in range(1025):
            f(None)
            f()

    def test_too_many_defaults_2(self):
        def f(x, y):
            pass

        f.__defaults__ = (None, None, None)
        for _ in range(1025):
            f(None, None)
            f(None)
            f()

    @disabling_optimizer
    @requires_specialization
    def test_assign_init_code(self):
        class MyClass:
            def __init__(self):
                pass

        def instantiate():
            return MyClass()

        # Trigger specialization
        for _ in range(1025):
            instantiate()
        self.assert_specialized(instantiate, "CALL_ALLOC_AND_ENTER_INIT")

        def count_args(self, *args):
            self.num_args = len(args)

        # Set MyClass.__init__.__code__ to a code object that is incompatible
        # (uses varargs) with the current specialization
        MyClass.__init__.__code__ = count_args.__code__
        instantiate()


@threading_helper.requires_working_threading()
@requires_specialization
class TestRacesDoNotCrash(TestBase):
    # Careful with these. Bigger numbers have a higher chance of catching bugs,
    # but you can also burn through a *ton* of type/dict/function versions:
    ITEMS = 1000
    LOOPS = 4
    WARMUPS = 2
    WRITERS = 2

    @disabling_optimizer
    def assert_races_do_not_crash(
        self, opname, get_items, read, write, *, check_items=False
    ):
        # This might need a few dozen loops in some cases:
        for _ in range(self.LOOPS):
            items = get_items()
            # Reset:
            if check_items:
                for item in items:
                    item.__code__ = item.__code__.replace()
            else:
                read.__code__ = read.__code__.replace()
            # Specialize:
            for _ in range(self.WARMUPS):
                read(items)
            if check_items:
                for item in items:
                    self.assert_specialized(item, opname)
            else:
                self.assert_specialized(read, opname)
            # Create writers:
            writers = []
            for _ in range(self.WRITERS):
                writer = threading.Thread(target=write, args=[items])
                writers.append(writer)
            # Run:
            for writer in writers:
                writer.start()
            read(items)  # BOOM!
            for writer in writers:
                writer.join()

    def test_binary_subscr_getitem(self):
        def get_items():
            class C:
                __getitem__ = lambda self, item: None

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item[None]
                except TypeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.__getitem__
                except AttributeError:
                    pass
                type(item).__getitem__ = lambda self, item: None

        opname = "BINARY_SUBSCR_GETITEM"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_binary_subscr_list_int(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = [None]
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item[0]
                except IndexError:
                    pass

        def write(items):
            for item in items:
                item.clear()
                item.append(None)

        opname = "BINARY_SUBSCR_LIST_INT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_for_iter_gen(self):
        def get_items():
            def g():
                yield
                yield

            items = []
            for _ in range(self.ITEMS):
                item = g()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    for _ in item:
                        break
                except ValueError:
                    pass

        def write(items):
            for item in items:
                try:
                    for _ in item:
                        break
                except ValueError:
                    pass

        opname = "FOR_ITER_GEN"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_for_iter_list(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = [None]
                items.append(item)
            return items

        def read(items):
            for item in items:
                for item in item:
                    break

        def write(items):
            for item in items:
                item.clear()
                item.append(None)

        opname = "FOR_ITER_LIST"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_class(self):
        def get_items():
            class C:
                a = object()

            items = []
            for _ in range(self.ITEMS):
                item = C
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.a
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.a
                except AttributeError:
                    pass
                item.a = object()

        opname = "LOAD_ATTR_CLASS"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_getattribute_overridden(self):
        def get_items():
            class C:
                __getattribute__ = lambda self, name: None

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.a
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.__getattribute__
                except AttributeError:
                    pass
                type(item).__getattribute__ = lambda self, name: None

        opname = "LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_instance_value(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                item.a = None
                items.append(item)
            return items

        def read(items):
            for item in items:
                item.a

        def write(items):
            for item in items:
                item.__dict__[None] = None

        opname = "LOAD_ATTR_INSTANCE_VALUE"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_method_lazy_dict(self):
        def get_items():
            class C(Exception):
                m = lambda self: None

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.m()
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.m
                except AttributeError:
                    pass
                type(item).m = lambda self: None

        opname = "LOAD_ATTR_METHOD_LAZY_DICT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_method_no_dict(self):
        def get_items():
            class C:
                __slots__ = ()
                m = lambda self: None

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.m()
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.m
                except AttributeError:
                    pass
                type(item).m = lambda self: None

        opname = "LOAD_ATTR_METHOD_NO_DICT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_method_with_values(self):
        def get_items():
            class C:
                m = lambda self: None

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.m()
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del item.m
                except AttributeError:
                    pass
                type(item).m = lambda self: None

        opname = "LOAD_ATTR_METHOD_WITH_VALUES"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_module(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = types.ModuleType("<item>")
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.__name__
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                d = item.__dict__.copy()
                item.__dict__.clear()
                item.__dict__.update(d)

        opname = "LOAD_ATTR_MODULE"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_property(self):
        def get_items():
            class C:
                a = property(lambda self: None)

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item.a
                except AttributeError:
                    pass

        def write(items):
            for item in items:
                try:
                    del type(item).a
                except AttributeError:
                    pass
                type(item).a = property(lambda self: None)

        opname = "LOAD_ATTR_PROPERTY"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_attr_with_hint(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                item.a = None
                # Resize into a combined unicode dict:
                for i in range(29):
                    setattr(item, f"_{i}", None)
                items.append(item)
            return items

        def read(items):
            for item in items:
                item.a

        def write(items):
            for item in items:
                item.__dict__[None] = None

        opname = "LOAD_ATTR_WITH_HINT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_load_global_module(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = eval("lambda: x", {"x": None})
                items.append(item)
            return items

        def read(items):
            for item in items:
                item()

        def write(items):
            for item in items:
                item.__globals__[None] = None

        opname = "LOAD_GLOBAL_MODULE"
        self.assert_races_do_not_crash(
            opname, get_items, read, write, check_items=True
        )

    def test_store_attr_instance_value(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                items.append(item)
            return items

        def read(items):
            for item in items:
                item.a = None

        def write(items):
            for item in items:
                item.__dict__[None] = None

        opname = "STORE_ATTR_INSTANCE_VALUE"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_store_attr_with_hint(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                # Resize into a combined unicode dict:
                for i in range(29):
                    setattr(item, f"_{i}", None)
                items.append(item)
            return items

        def read(items):
            for item in items:
                item.a = None

        def write(items):
            for item in items:
                item.__dict__[None] = None

        opname = "STORE_ATTR_WITH_HINT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_store_subscr_list_int(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = [None]
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    item[0] = None
                except IndexError:
                    pass

        def write(items):
            for item in items:
                item.clear()
                item.append(None)

        opname = "STORE_SUBSCR_LIST_INT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    def test_unpack_sequence_list(self):
        def get_items():
            items = []
            for _ in range(self.ITEMS):
                item = [None]
                items.append(item)
            return items

        def read(items):
            for item in items:
                try:
                    [_] = item
                except ValueError:
                    pass

        def write(items):
            for item in items:
                item.clear()
                item.append(None)

        opname = "UNPACK_SEQUENCE_LIST"
        self.assert_races_do_not_crash(opname, get_items, read, write)

class C:
    pass

@requires_specialization
class TestInstanceDict(unittest.TestCase):

    def setUp(self):
        c = C()
        c.a, c.b, c.c = 0,0,0

    def test_values_on_instance(self):
        c = C()
        c.a = 1
        C().b = 2
        c.c = 3
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c),
            (1, '<NULL>', 3)
        )

    def test_dict_materialization(self):
        c = C()
        c.a = 1
        c.b = 2
        c.__dict__
        self.assertEqual(c.__dict__, {"a":1, "b": 2})

    def test_dict_dematerialization(self):
        c = C()
        c.a = 1
        c.b = 2
        c.__dict__
        for _ in range(100):
            c.a
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c),
            (1, 2, '<NULL>')
        )

    def test_dict_dematerialization_multiple_refs(self):
        c = C()
        c.a = 1
        c.b = 2
        d = c.__dict__
        for _ in range(100):
            c.a
        self.assertIs(c.__dict__, d)

    def test_dict_dematerialization_copy(self):
        c = C()
        c.a = 1
        c.b = 2
        c2 = copy.copy(c)
        for _ in range(100):
            c.a
            c2.a
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c),
            (1, 2, '<NULL>')
        )
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c2),
            (1, 2, '<NULL>')
        )
        c3 = copy.deepcopy(c)
        for _ in range(100):
            c.a
            c3.a
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c),
            (1, 2, '<NULL>')
        )
        #NOTE -- c3.__dict__ does not de-materialize

    def test_dict_dematerialization_pickle(self):
        c = C()
        c.a = 1
        c.b = 2
        c2 = pickle.loads(pickle.dumps(c))
        for _ in range(100):
            c.a
            c2.a
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c),
            (1, 2, '<NULL>')
        )
        self.assertEqual(
            _testinternalcapi.get_object_dict_values(c2),
            (1, 2, '<NULL>')
        )

    def test_dict_dematerialization_subclass(self):
        class D(dict): pass
        c = C()
        c.a = 1
        c.b = 2
        c.__dict__ = D(c.__dict__)
        for _ in range(100):
            c.a
        self.assertIs(
            _testinternalcapi.get_object_dict_values(c),
            None
        )
        self.assertEqual(
            c.__dict__,
            {'a':1, 'b':2}
        )


if __name__ == "__main__":
    unittest.main()
