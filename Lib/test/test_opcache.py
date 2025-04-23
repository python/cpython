import copy
import pickle
import dis
import threading
import types
import unittest
from test.support import (threading_helper, check_impl_detail,
                          requires_specialization, requires_specialization_ft,
                          cpython_only, requires_jit_disabled, reset_code)
from test.support.import_helper import import_module

# Skip this module on other interpreters, it is cpython specific:
if check_impl_detail(cpython=False):
    raise unittest.SkipTest('implementation detail specific to cpython')

_testinternalcapi = import_module("_testinternalcapi")


class TestBase(unittest.TestCase):
    def assert_specialized(self, f, opname):
        instructions = dis.get_instructions(f, adaptive=True)
        opnames = {instruction.opname for instruction in instructions}
        self.assertIn(opname, opnames)

    def assert_no_opcode(self, f, opname):
        instructions = dis.get_instructions(f, adaptive=True)
        opnames = {instruction.opname for instruction in instructions}
        self.assertNotIn(opname, opnames)


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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD - 1):
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Metaclass.attribute = attribute

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        del Metaclass.attribute

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
            self.assertFalse(f())

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            mro = None

        def f():
            return Class.mro

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertIsNone(f())

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            __base__ = None

        def f():
            return Class.__base__

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertIs(f(), object)

    def test_type_descriptor_shadows_attribute_getset(self):
        class Class:
            __name__ = "Spam"

        def f():
            return Class.__name__

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertEqual(f(), "Class")

    def test_metaclass_getattribute(self):
        class Metaclass(type):
            def __getattribute__(self, name):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Class.__class__ = NewMetaclass

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Descriptor.__get__ = __get__
        Descriptor.__set__ = __set__

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Metaclass.attribute = attribute

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        del Metaclass.attribute

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
            self.assertFalse(f())

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            def mro():
                return ["Spam", "eggs"]

        def f():
            return Class.mro()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertEqual(f(), ["Spam", "eggs"])

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            def __base__():
                return "Spam"

        def f():
            return Class.__base__()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertTrue(f())

        Class.__class__ = NewMetaclass

        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
            self.assertFalse(f())


class InitTakesArg:
    def __init__(self, arg):
        self.arg = arg


class TestCallCache(TestBase):
    def test_too_many_defaults_0(self):
        def f():
            pass

        f.__defaults__ = (None,)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            f()

    def test_too_many_defaults_1(self):
        def f(x):
            pass

        f.__defaults__ = (None, None)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            f(None)
            f()

    def test_too_many_defaults_2(self):
        def f(x, y):
            pass

        f.__defaults__ = (None, None, None)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            f(None, None)
            f(None)
            f()

    @requires_jit_disabled
    @requires_specialization_ft
    def test_assign_init_code(self):
        class MyClass:
            def __init__(self):
                pass

        def instantiate():
            return MyClass()

        # Trigger specialization
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            instantiate()
        self.assert_specialized(instantiate, "CALL_ALLOC_AND_ENTER_INIT")

        def count_args(self, *args):
            self.num_args = len(args)

        # Set MyClass.__init__.__code__ to a code object that uses different
        # args
        MyClass.__init__.__code__ = count_args.__code__
        instantiate()

    @requires_jit_disabled
    @requires_specialization_ft
    def test_push_init_frame_fails(self):
        def instantiate():
            return InitTakesArg()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            with self.assertRaises(TypeError):
                instantiate()
        self.assert_specialized(instantiate, "CALL_ALLOC_AND_ENTER_INIT")

        with self.assertRaises(TypeError):
            instantiate()


def make_deferred_ref_count_obj():
    """Create an object that uses deferred reference counting.

    Only objects that use deferred refence counting may be stored in inline
    caches in free-threaded builds. This constructs a new class named Foo,
    which uses deferred reference counting.
    """
    return type("Foo", (object,), {})


@threading_helper.requires_working_threading()
class TestRacesDoNotCrash(TestBase):
    # Careful with these. Bigger numbers have a higher chance of catching bugs,
    # but you can also burn through a *ton* of type/dict/function versions:
    ITEMS = 1000
    LOOPS = 4
    WRITERS = 2

    @requires_jit_disabled
    def assert_races_do_not_crash(
        self, opname, get_items, read, write, *, check_items=False
    ):
        # This might need a few dozen loops in some cases:
        for _ in range(self.LOOPS):
            items = get_items()
            # Reset:
            if check_items:
                for item in items:
                    reset_code(item)
            else:
                reset_code(read)
            # Specialize:
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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

    @requires_specialization_ft
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

        opname = "BINARY_OP_SUBSCR_GETITEM"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    @requires_specialization_ft
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

        opname = "BINARY_OP_SUBSCR_LIST_INT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    @requires_specialization
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

    @requires_specialization
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

    @requires_specialization_ft
    def test_load_attr_class(self):
        def get_items():
            class C:
                a = make_deferred_ref_count_obj()

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
                item.a = make_deferred_ref_count_obj()

        opname = "LOAD_ATTR_CLASS"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    @requires_specialization_ft
    def test_load_attr_class_with_metaclass_check(self):
        def get_items():
            class Meta(type):
                pass

            class C(metaclass=Meta):
                a = make_deferred_ref_count_obj()

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
                item.a = make_deferred_ref_count_obj()

        opname = "LOAD_ATTR_CLASS_WITH_METACLASS_CHECK"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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

    @requires_specialization_ft
    def test_load_attr_slot(self):
        def get_items():
            class C:
                __slots__ = ["a", "b"]

            items = []
            for i in range(self.ITEMS):
                item = C()
                item.a = i
                item.b = i + self.ITEMS
                items.append(item)
            return items

        def read(items):
            for item in items:
                item.a
                item.b

        def write(items):
            for item in items:
                item.a = 100
                item.b = 200

        opname = "LOAD_ATTR_SLOT"
        self.assert_races_do_not_crash(opname, get_items, read, write)

    @requires_specialization_ft
    def test_load_attr_with_hint(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                item.a = None
                # Resize into a combined unicode dict:
                for i in range(_testinternalcapi.SHARED_KEYS_MAX_SIZE - 1):
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

    @requires_specialization_ft
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

    @requires_specialization
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

    @requires_specialization
    def test_store_attr_with_hint(self):
        def get_items():
            class C:
                pass

            items = []
            for _ in range(self.ITEMS):
                item = C()
                # Resize into a combined unicode dict:
                for i in range(_testinternalcapi.SHARED_KEYS_MAX_SIZE - 1):
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

    @requires_specialization_ft
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

    @requires_specialization_ft
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            c.a
        self.assertIs(c.__dict__, d)

    def test_dict_dematerialization_copy(self):
        c = C()
        c.a = 1
        c.b = 2
        c2 = copy.copy(c)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
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
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            c.a
        self.assertIs(
            _testinternalcapi.get_object_dict_values(c),
            None
        )
        self.assertEqual(
            c.__dict__,
            {'a':1, 'b':2}
        )

    def test_125868(self):

        def make_special_dict():
            """Create a dictionary an object with a this table:
            index | key | value
            ----- | --- | -----
              0   | 'b' | 'value'
              1   | 'b' | NULL
            """
            class A:
                pass
            a = A()
            a.a = 1
            a.b = 2
            d = a.__dict__.copy()
            del d['a']
            del d['b']
            d['b'] = "value"
            return d

        class NoInlineAorB:
            pass
        for i in range(ord('c'), ord('z')):
            setattr(NoInlineAorB(), chr(i), i)

        c = NoInlineAorB()
        c.a = 0
        c.b = 1
        self.assertFalse(_testinternalcapi.has_inline_values(c))

        def f(o, n):
            for i in range(n):
                o.b = i
        # Prime f to store to dict slot 1
        f(c, _testinternalcapi.SPECIALIZATION_THRESHOLD)

        test_obj = NoInlineAorB()
        test_obj.__dict__ = make_special_dict()
        self.assertEqual(test_obj.b, "value")

        #This should set x.b = 0
        f(test_obj, 1)
        self.assertEqual(test_obj.b, 0)


class TestSpecializer(TestBase):

    @cpython_only
    @requires_specialization_ft
    def test_binary_op(self):
        def binary_op_add_int():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 1, 2
                c = a + b
                self.assertEqual(c, 3)

        binary_op_add_int()
        self.assert_specialized(binary_op_add_int, "BINARY_OP_ADD_INT")
        self.assert_no_opcode(binary_op_add_int, "BINARY_OP")

        def binary_op_add_unicode():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = "foo", "bar"
                c = a + b
                self.assertEqual(c, "foobar")

        binary_op_add_unicode()
        self.assert_specialized(binary_op_add_unicode, "BINARY_OP_ADD_UNICODE")
        self.assert_no_opcode(binary_op_add_unicode, "BINARY_OP")

        def binary_op_add_extend():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 6, 3.0
                c = a + b
                self.assertEqual(c, 9.0)
                c = b + a
                self.assertEqual(c, 9.0)
                c = a - b
                self.assertEqual(c, 3.0)
                c = b - a
                self.assertEqual(c, -3.0)
                c = a * b
                self.assertEqual(c, 18.0)
                c = b * a
                self.assertEqual(c, 18.0)
                c = a / b
                self.assertEqual(c, 2.0)
                c = b / a
                self.assertEqual(c, 0.5)

        binary_op_add_extend()
        self.assert_specialized(binary_op_add_extend, "BINARY_OP_EXTEND")
        self.assert_no_opcode(binary_op_add_extend, "BINARY_OP")

        def binary_op_zero_division():
            def compactlong_lhs(arg):
                42 / arg
            def float_lhs(arg):
                42.0 / arg

            with self.assertRaises(ZeroDivisionError):
                compactlong_lhs(0)
            with self.assertRaises(ZeroDivisionError):
                compactlong_lhs(0.0)
            with self.assertRaises(ZeroDivisionError):
                float_lhs(0.0)
            with self.assertRaises(ZeroDivisionError):
                float_lhs(0)

            self.assert_no_opcode(compactlong_lhs, "BINARY_OP_EXTEND")
            self.assert_no_opcode(float_lhs, "BINARY_OP_EXTEND")

        binary_op_zero_division()

        def binary_op_nan():
            def compactlong_lhs(arg):
                return (
                    42 + arg,
                    42 - arg,
                    42 * arg,
                    42 / arg,
                )
            def compactlong_rhs(arg):
                return (
                    arg + 42,
                    arg - 42,
                    arg * 2,
                    arg / 42,
                )
            nan = float('nan')
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                self.assertEqual(compactlong_lhs(1.0), (43.0, 41.0, 42.0, 42.0))
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                self.assertTrue(all(filter(lambda x: x is nan, compactlong_lhs(nan))))
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                self.assertEqual(compactlong_rhs(42.0), (84.0, 0.0, 84.0, 1.0))
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                self.assertTrue(all(filter(lambda x: x is nan, compactlong_rhs(nan))))

            self.assert_no_opcode(compactlong_lhs, "BINARY_OP_EXTEND")
            self.assert_no_opcode(compactlong_rhs, "BINARY_OP_EXTEND")

        binary_op_nan()

        def binary_op_bitwise_extend():
            for _ in range(100):
                a, b = 2, 7
                x = a | b
                self.assertEqual(x, 7)
                y = a & b
                self.assertEqual(y, 2)
                z = a ^ b
                self.assertEqual(z, 5)
                a, b = 3, 9
                a |= b
                self.assertEqual(a, 11)
                a, b = 11, 9
                a &= b
                self.assertEqual(a, 9)
                a, b = 3, 9
                a ^= b
                self.assertEqual(a, 10)

        binary_op_bitwise_extend()
        self.assert_specialized(binary_op_bitwise_extend, "BINARY_OP_EXTEND")
        self.assert_no_opcode(binary_op_bitwise_extend, "BINARY_OP")

    @cpython_only
    @requires_specialization_ft
    def test_load_super_attr(self):
        """Ensure that LOAD_SUPER_ATTR is specialized as expected."""

        class A:
            def __init__(self):
                meth = super().__init__
                super().__init__()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            A()

        self.assert_specialized(A.__init__, "LOAD_SUPER_ATTR_ATTR")
        self.assert_specialized(A.__init__, "LOAD_SUPER_ATTR_METHOD")
        self.assert_no_opcode(A.__init__, "LOAD_SUPER_ATTR")

        # Temporarily replace super() with something else.
        real_super = super

        def fake_super():
            def init(self):
                pass

            return init

        # Force unspecialize
        globals()['super'] = fake_super
        try:
            # Should be unspecialized after enough calls.
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                A()
        finally:
            globals()['super'] = real_super

        # Ensure the specialized instructions are not present
        self.assert_no_opcode(A.__init__, "LOAD_SUPER_ATTR_ATTR")
        self.assert_no_opcode(A.__init__, "LOAD_SUPER_ATTR_METHOD")

    @cpython_only
    @requires_specialization_ft
    def test_contain_op(self):
        def contains_op_dict():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 1, {1: 2, 2: 5}
                self.assertTrue(a in b)
                self.assertFalse(3 in b)

        contains_op_dict()
        self.assert_specialized(contains_op_dict, "CONTAINS_OP_DICT")
        self.assert_no_opcode(contains_op_dict, "CONTAINS_OP")

        def contains_op_set():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 1, {1, 2}
                self.assertTrue(a in b)
                self.assertFalse(3 in b)

        contains_op_set()
        self.assert_specialized(contains_op_set, "CONTAINS_OP_SET")
        self.assert_no_opcode(contains_op_set, "CONTAINS_OP")

    @cpython_only
    @requires_specialization_ft
    def test_send_with(self):
        def run_async(coro):
            while True:
                try:
                    coro.send(None)
                except StopIteration:
                    break

        class CM:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *exc):
                pass

        async def send_with():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                async with CM():
                    x = 1

        run_async(send_with())
        # Note there are still unspecialized "SEND" opcodes in the
        # cleanup paths of the 'with' statement.
        self.assert_specialized(send_with, "SEND_GEN")

    @cpython_only
    @requires_specialization_ft
    def test_send_yield_from(self):
        def g():
            yield None

        def send_yield_from():
            yield from g()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            list(send_yield_from())

        self.assert_specialized(send_yield_from, "SEND_GEN")
        self.assert_no_opcode(send_yield_from, "SEND")

    @cpython_only
    @requires_specialization_ft
    def test_store_attr_slot(self):
        class C:
            __slots__ = ['x']

        def set_slot(n):
            c = C()
            for i in range(n):
                c.x = i

        set_slot(_testinternalcapi.SPECIALIZATION_THRESHOLD)

        self.assert_specialized(set_slot, "STORE_ATTR_SLOT")
        self.assert_no_opcode(set_slot, "STORE_ATTR")

        # Adding a property for 'x' should unspecialize it.
        C.x = property(lambda self: None, lambda self, x: None)
        set_slot(_testinternalcapi.SPECIALIZATION_COOLDOWN)
        self.assert_no_opcode(set_slot, "STORE_ATTR_SLOT")

    @cpython_only
    @requires_specialization_ft
    def test_store_attr_instance_value(self):
        class C:
            pass

        @reset_code
        def set_value(n):
            c = C()
            for i in range(n):
                c.x = i

        set_value(_testinternalcapi.SPECIALIZATION_THRESHOLD)

        self.assert_specialized(set_value, "STORE_ATTR_INSTANCE_VALUE")
        self.assert_no_opcode(set_value, "STORE_ATTR")

        # Adding a property for 'x' should unspecialize it.
        C.x = property(lambda self: None, lambda self, x: None)
        set_value(_testinternalcapi.SPECIALIZATION_COOLDOWN)
        self.assert_no_opcode(set_value, "STORE_ATTR_INSTANCE_VALUE")

    @cpython_only
    @requires_specialization_ft
    def test_store_attr_with_hint(self):
        class C:
            pass

        c = C()
        for i in range(_testinternalcapi.SHARED_KEYS_MAX_SIZE - 1):
            setattr(c, f"_{i}", None)

        @reset_code
        def set_value(n):
            for i in range(n):
                c.x = i

        set_value(_testinternalcapi.SPECIALIZATION_THRESHOLD)

        self.assert_specialized(set_value, "STORE_ATTR_WITH_HINT")
        self.assert_no_opcode(set_value, "STORE_ATTR")

        # Adding a property for 'x' should unspecialize it.
        C.x = property(lambda self: None, lambda self, x: None)
        set_value(_testinternalcapi.SPECIALIZATION_COOLDOWN)
        self.assert_no_opcode(set_value, "STORE_ATTR_WITH_HINT")

    @cpython_only
    @requires_specialization_ft
    def test_to_bool(self):
        def to_bool_bool():
            true_cnt, false_cnt = 0, 0
            elems = [e % 2 == 0 for e in range(_testinternalcapi.SPECIALIZATION_THRESHOLD)]
            for e in elems:
                if e:
                    true_cnt += 1
                else:
                    false_cnt += 1
            d, m = divmod(_testinternalcapi.SPECIALIZATION_THRESHOLD, 2)
            self.assertEqual(true_cnt, d + m)
            self.assertEqual(false_cnt, d)

        to_bool_bool()
        self.assert_specialized(to_bool_bool, "TO_BOOL_BOOL")
        self.assert_no_opcode(to_bool_bool, "TO_BOOL")

        def to_bool_int():
            count = 0
            for i in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                if i:
                    count += 1
                else:
                    count -= 1
            self.assertEqual(count, _testinternalcapi.SPECIALIZATION_THRESHOLD - 2)

        to_bool_int()
        self.assert_specialized(to_bool_int, "TO_BOOL_INT")
        self.assert_no_opcode(to_bool_int, "TO_BOOL")

        def to_bool_list():
            count = 0
            elems = list(range(_testinternalcapi.SPECIALIZATION_THRESHOLD))
            while elems:
                count += elems.pop()
            self.assertEqual(elems, [])
            self.assertEqual(count, sum(range(_testinternalcapi.SPECIALIZATION_THRESHOLD)))

        to_bool_list()
        self.assert_specialized(to_bool_list, "TO_BOOL_LIST")
        self.assert_no_opcode(to_bool_list, "TO_BOOL")

        def to_bool_none():
            count = 0
            elems = [None] * _testinternalcapi.SPECIALIZATION_THRESHOLD
            for e in elems:
                if not e:
                    count += 1
            self.assertEqual(count, _testinternalcapi.SPECIALIZATION_THRESHOLD)

        to_bool_none()
        self.assert_specialized(to_bool_none, "TO_BOOL_NONE")
        self.assert_no_opcode(to_bool_none, "TO_BOOL")

        def to_bool_str():
            count = 0
            elems = [""] + ["foo"] * (_testinternalcapi.SPECIALIZATION_THRESHOLD - 1)
            for e in elems:
                if e:
                    count += 1
            self.assertEqual(count, _testinternalcapi.SPECIALIZATION_THRESHOLD - 1)

        to_bool_str()
        self.assert_specialized(to_bool_str, "TO_BOOL_STR")
        self.assert_no_opcode(to_bool_str, "TO_BOOL")

    @cpython_only
    @requires_specialization_ft
    def test_unpack_sequence(self):
        def unpack_sequence_two_tuple():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                t = 1, 2
                a, b = t
                self.assertEqual(a, 1)
                self.assertEqual(b, 2)

        unpack_sequence_two_tuple()
        self.assert_specialized(unpack_sequence_two_tuple,
                                "UNPACK_SEQUENCE_TWO_TUPLE")
        self.assert_no_opcode(unpack_sequence_two_tuple, "UNPACK_SEQUENCE")

        def unpack_sequence_tuple():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b, c, d = 1, 2, 3, 4
                self.assertEqual(a, 1)
                self.assertEqual(b, 2)
                self.assertEqual(c, 3)
                self.assertEqual(d, 4)

        unpack_sequence_tuple()
        self.assert_specialized(unpack_sequence_tuple, "UNPACK_SEQUENCE_TUPLE")
        self.assert_no_opcode(unpack_sequence_tuple, "UNPACK_SEQUENCE")

        def unpack_sequence_list():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = [1, 2]
                self.assertEqual(a, 1)
                self.assertEqual(b, 2)

        unpack_sequence_list()
        self.assert_specialized(unpack_sequence_list, "UNPACK_SEQUENCE_LIST")
        self.assert_no_opcode(unpack_sequence_list, "UNPACK_SEQUENCE")

    @cpython_only
    @requires_specialization_ft
    def test_binary_subscr(self):
        def binary_subscr_list_int():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a = [1, 2, 3]
                for idx, expected in enumerate(a):
                    self.assertEqual(a[idx], expected)

        binary_subscr_list_int()
        self.assert_specialized(binary_subscr_list_int,
                                "BINARY_OP_SUBSCR_LIST_INT")
        self.assert_no_opcode(binary_subscr_list_int, "BINARY_OP")

        def binary_subscr_tuple_int():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a = (1, 2, 3)
                for idx, expected in enumerate(a):
                    self.assertEqual(a[idx], expected)

        binary_subscr_tuple_int()
        self.assert_specialized(binary_subscr_tuple_int,
                                "BINARY_OP_SUBSCR_TUPLE_INT")
        self.assert_no_opcode(binary_subscr_tuple_int, "BINARY_OP")

        def binary_subscr_dict():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a = {1: 2, 2: 3}
                self.assertEqual(a[1], 2)
                self.assertEqual(a[2], 3)

        binary_subscr_dict()
        self.assert_specialized(binary_subscr_dict, "BINARY_OP_SUBSCR_DICT")
        self.assert_no_opcode(binary_subscr_dict, "BINARY_OP")

        def binary_subscr_str_int():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a = "foobar"
                for idx, expected in enumerate(a):
                    self.assertEqual(a[idx], expected)

        binary_subscr_str_int()
        self.assert_specialized(binary_subscr_str_int, "BINARY_OP_SUBSCR_STR_INT")
        self.assert_no_opcode(binary_subscr_str_int, "BINARY_OP")

        def binary_subscr_getitems():
            class C:
                def __init__(self, val):
                    self.val = val
                def __getitem__(self, item):
                    return self.val

            items = [C(i) for i in range(_testinternalcapi.SPECIALIZATION_THRESHOLD)]
            for i in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                self.assertEqual(items[i][i], i)

        binary_subscr_getitems()
        self.assert_specialized(binary_subscr_getitems, "BINARY_OP_SUBSCR_GETITEM")
        self.assert_no_opcode(binary_subscr_getitems, "BINARY_OP")

    @cpython_only
    @requires_specialization_ft
    def test_compare_op(self):
        def compare_op_int():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 1, 2
                c = a == b
                self.assertFalse(c)

        compare_op_int()
        self.assert_specialized(compare_op_int, "COMPARE_OP_INT")
        self.assert_no_opcode(compare_op_int, "COMPARE_OP")

        def compare_op_float():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = 1.0, 2.0
                c = a == b
                self.assertFalse(c)

        compare_op_float()
        self.assert_specialized(compare_op_float, "COMPARE_OP_FLOAT")
        self.assert_no_opcode(compare_op_float, "COMPARE_OP")

        def compare_op_str():
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                a, b = "spam", "ham"
                c = a == b
                self.assertFalse(c)

        compare_op_str()
        self.assert_specialized(compare_op_str, "COMPARE_OP_STR")
        self.assert_no_opcode(compare_op_str, "COMPARE_OP")

    @cpython_only
    @requires_specialization_ft
    def test_load_const(self):
        def load_const():
            def unused(): pass
            # Currently, the empty tuple is immortal, and the otherwise
            # unused nested function's code object is mortal. This test will
            # have to use different values if either of that changes.
            return ()

        load_const()
        self.assert_specialized(load_const, "LOAD_CONST_IMMORTAL")
        self.assert_specialized(load_const, "LOAD_CONST_MORTAL")
        self.assert_no_opcode(load_const, "LOAD_CONST")

    @cpython_only
    @requires_specialization_ft
    def test_for_iter(self):
        L = list(range(10))
        def for_iter_list():
            for i in L:
                self.assertIn(i, L)

        for_iter_list()
        self.assert_specialized(for_iter_list, "FOR_ITER_LIST")
        self.assert_no_opcode(for_iter_list, "FOR_ITER")

        t = tuple(range(10))
        def for_iter_tuple():
            for i in t:
                self.assertIn(i, t)

        for_iter_tuple()
        self.assert_specialized(for_iter_tuple, "FOR_ITER_TUPLE")
        self.assert_no_opcode(for_iter_tuple, "FOR_ITER")

        r = range(10)
        def for_iter_range():
            for i in r:
                self.assertIn(i, r)

        for_iter_range()
        self.assert_specialized(for_iter_range, "FOR_ITER_RANGE")
        self.assert_no_opcode(for_iter_range, "FOR_ITER")

        def for_iter_generator():
            for i in (i for i in range(10)):
                i + 1

        for_iter_generator()
        self.assert_specialized(for_iter_generator, "FOR_ITER_GEN")
        self.assert_no_opcode(for_iter_generator, "FOR_ITER")


if __name__ == "__main__":
    unittest.main()
