import enum
import doctest
import inspect
import os
import pydoc
import sys
import unittest
import threading
from collections import OrderedDict
from enum import Enum, IntEnum, StrEnum, EnumType, Flag, IntFlag, unique, auto
from enum import STRICT, CONFORM, EJECT, KEEP, _simple_enum, _test_simple_enum
from enum import verify, UNIQUE, CONTINUOUS, NAMED_FLAGS
from io import StringIO
from pickle import dumps, loads, PicklingError, HIGHEST_PROTOCOL
from test import support
from test.support import ALWAYS_EQ
from test.support import threading_helper
from datetime import timedelta

python_version = sys.version_info[:2]

def load_tests(loader, tests, ignore):
    tests.addTests(doctest.DocTestSuite(enum))
    if os.path.exists('Doc/library/enum.rst'):
        tests.addTests(doctest.DocFileSuite(
                '../../Doc/library/enum.rst',
                optionflags=doctest.ELLIPSIS|doctest.NORMALIZE_WHITESPACE,
                ))
    return tests

MODULE = ('test.test_enum', '__main__')[__name__=='__main__']
SHORT_MODULE = MODULE.split('.')[-1]

# for pickle tests
try:
    class Stooges(Enum):
        LARRY = 1
        CURLY = 2
        MOE = 3
except Exception as exc:
    Stooges = exc

try:
    class IntStooges(int, Enum):
        LARRY = 1
        CURLY = 2
        MOE = 3
except Exception as exc:
    IntStooges = exc

try:
    class FloatStooges(float, Enum):
        LARRY = 1.39
        CURLY = 2.72
        MOE = 3.142596
except Exception as exc:
    FloatStooges = exc

try:
    class FlagStooges(Flag):
        LARRY = 1
        CURLY = 2
        MOE = 3
except Exception as exc:
    FlagStooges = exc

# for pickle test and subclass tests
class Name(StrEnum):
    BDFL = 'Guido van Rossum'
    FLUFL = 'Barry Warsaw'

try:
    Question = Enum('Question', 'who what when where why', module=__name__)
except Exception as exc:
    Question = exc

try:
    Answer = Enum('Answer', 'him this then there because')
except Exception as exc:
    Answer = exc

try:
    Theory = Enum('Theory', 'rule law supposition', qualname='spanish_inquisition')
except Exception as exc:
    Theory = exc

# for doctests
try:
    class Fruit(Enum):
        TOMATO = 1
        BANANA = 2
        CHERRY = 3
except Exception:
    pass

def test_pickle_dump_load(assertion, source, target=None):
    if target is None:
        target = source
    for protocol in range(HIGHEST_PROTOCOL + 1):
        assertion(loads(dumps(source, protocol=protocol)), target)

def test_pickle_exception(assertion, exception, obj):
    for protocol in range(HIGHEST_PROTOCOL + 1):
        with assertion(exception):
            dumps(obj, protocol=protocol)

class TestHelpers(unittest.TestCase):
    # _is_descriptor, _is_sunder, _is_dunder

    def test_is_descriptor(self):
        class foo:
            pass
        for attr in ('__get__','__set__','__delete__'):
            obj = foo()
            self.assertFalse(enum._is_descriptor(obj))
            setattr(obj, attr, 1)
            self.assertTrue(enum._is_descriptor(obj))

    def test_is_sunder(self):
        for s in ('_a_', '_aa_'):
            self.assertTrue(enum._is_sunder(s))

        for s in ('a', 'a_', '_a', '__a', 'a__', '__a__', '_a__', '__a_', '_',
                '__', '___', '____', '_____',):
            self.assertFalse(enum._is_sunder(s))

    def test_is_dunder(self):
        for s in ('__a__', '__aa__'):
            self.assertTrue(enum._is_dunder(s))
        for s in ('a', 'a_', '_a', '__a', 'a__', '_a_', '_a__', '__a_', '_',
                '__', '___', '____', '_____',):
            self.assertFalse(enum._is_dunder(s))

# for subclassing tests

class classproperty:

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        self.fget = fget
        self.fset = fset
        self.fdel = fdel
        if doc is None and fget is not None:
            doc = fget.__doc__
        self.__doc__ = doc

    def __get__(self, instance, ownerclass):
        return self.fget(ownerclass)

# for global repr tests

@enum.global_enum
class HeadlightsK(IntFlag, boundary=enum.KEEP):
    OFF_K = 0
    LOW_BEAM_K = auto()
    HIGH_BEAM_K = auto()
    FOG_K = auto()


@enum.global_enum
class HeadlightsC(IntFlag, boundary=enum.CONFORM):
    OFF_C = 0
    LOW_BEAM_C = auto()
    HIGH_BEAM_C = auto()
    FOG_C = auto()


# tests

class TestEnum(unittest.TestCase):

    def setUp(self):
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = 3
            WINTER = 4
        self.Season = Season

        class Konstants(float, Enum):
            E = 2.7182818
            PI = 3.1415926
            TAU = 2 * PI
        self.Konstants = Konstants

        class Grades(IntEnum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 0
        self.Grades = Grades

        class Directional(str, Enum):
            EAST = 'east'
            WEST = 'west'
            NORTH = 'north'
            SOUTH = 'south'
        self.Directional = Directional

        from datetime import date
        class Holiday(date, Enum):
            NEW_YEAR = 2013, 1, 1
            IDES_OF_MARCH = 2013, 3, 15
        self.Holiday = Holiday

    def test_dir_on_class(self):
        Season = self.Season
        self.assertEqual(
            set(dir(Season)),
            set(['__class__', '__doc__', '__members__', '__module__',
                'SPRING', 'SUMMER', 'AUTUMN', 'WINTER']),
            )

    def test_dir_on_item(self):
        Season = self.Season
        self.assertEqual(
            set(dir(Season.WINTER)),
            set(['__class__', '__doc__', '__module__', 'name', 'value']),
            )

    def test_dir_with_added_behavior(self):
        class Test(Enum):
            this = 'that'
            these = 'those'
            def wowser(self):
                return ("Wowser! I'm %s!" % self.name)
        self.assertEqual(
                set(dir(Test)),
                set(['__class__', '__doc__', '__members__', '__module__', 'this', 'these']),
                )
        self.assertEqual(
                set(dir(Test.this)),
                set(['__class__', '__doc__', '__module__', 'name', 'value', 'wowser']),
                )

    def test_dir_on_sub_with_behavior_on_super(self):
        # see issue22506
        class SuperEnum(Enum):
            def invisible(self):
                return "did you see me?"
        class SubEnum(SuperEnum):
            sample = 5
        self.assertEqual(
                set(dir(SubEnum.sample)),
                set(['__class__', '__doc__', '__module__', 'name', 'value', 'invisible']),
                )

    def test_dir_on_sub_with_behavior_including_instance_dict_on_super(self):
        # see issue40084
        class SuperEnum(IntEnum):
            def __new__(cls, value, description=""):
                obj = int.__new__(cls, value)
                obj._value_ = value
                obj.description = description
                return obj
        class SubEnum(SuperEnum):
            sample = 5
        self.assertTrue({'description'} <= set(dir(SubEnum.sample)))

    def test_enum_in_enum_out(self):
        Season = self.Season
        self.assertIs(Season(Season.WINTER), Season.WINTER)

    def test_enum_value(self):
        Season = self.Season
        self.assertEqual(Season.SPRING.value, 1)

    def test_intenum_value(self):
        self.assertEqual(IntStooges.CURLY.value, 2)

    def test_enum(self):
        Season = self.Season
        lst = list(Season)
        self.assertEqual(len(lst), len(Season))
        self.assertEqual(len(Season), 4, Season)
        self.assertEqual(
            [Season.SPRING, Season.SUMMER, Season.AUTUMN, Season.WINTER], lst)

        for i, season in enumerate('SPRING SUMMER AUTUMN WINTER'.split(), 1):
            e = Season(i)
            self.assertEqual(e, getattr(Season, season))
            self.assertEqual(e.value, i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, season)
            self.assertIn(e, Season)
            self.assertIs(type(e), Season)
            self.assertIsInstance(e, Season)
            self.assertEqual(str(e), season)
            self.assertEqual(repr(e), 'Season.{0}'.format(season))

    def test_value_name(self):
        Season = self.Season
        self.assertEqual(Season.SPRING.name, 'SPRING')
        self.assertEqual(Season.SPRING.value, 1)
        with self.assertRaises(AttributeError):
            Season.SPRING.name = 'invierno'
        with self.assertRaises(AttributeError):
            Season.SPRING.value = 2

    def test_changing_member(self):
        Season = self.Season
        with self.assertRaises(AttributeError):
            Season.WINTER = 'really cold'

    def test_attribute_deletion(self):
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = 3
            WINTER = 4

            def spam(cls):
                pass

        self.assertTrue(hasattr(Season, 'spam'))
        del Season.spam
        self.assertFalse(hasattr(Season, 'spam'))

        with self.assertRaises(AttributeError):
            del Season.SPRING
        with self.assertRaises(AttributeError):
            del Season.DRY
        with self.assertRaises(AttributeError):
            del Season.SPRING.name

    def test_bool_of_class(self):
        class Empty(Enum):
            pass
        self.assertTrue(bool(Empty))

    def test_bool_of_member(self):
        class Count(Enum):
            zero = 0
            one = 1
            two = 2
        for member in Count:
            self.assertTrue(bool(member))

    def test_invalid_names(self):
        with self.assertRaises(ValueError):
            class Wrong(Enum):
                mro = 9
        with self.assertRaises(ValueError):
            class Wrong(Enum):
                _create_= 11
        with self.assertRaises(ValueError):
            class Wrong(Enum):
                _get_mixins_ = 9
        with self.assertRaises(ValueError):
            class Wrong(Enum):
                _find_new_ = 1
        with self.assertRaises(ValueError):
            class Wrong(Enum):
                _any_name_ = 9

    def test_bool(self):
        # plain Enum members are always True
        class Logic(Enum):
            true = True
            false = False
        self.assertTrue(Logic.true)
        self.assertTrue(Logic.false)
        # unless overridden
        class RealLogic(Enum):
            true = True
            false = False
            def __bool__(self):
                return bool(self._value_)
        self.assertTrue(RealLogic.true)
        self.assertFalse(RealLogic.false)
        # mixed Enums depend on mixed-in type
        class IntLogic(int, Enum):
            true = 1
            false = 0
        self.assertTrue(IntLogic.true)
        self.assertFalse(IntLogic.false)

    @unittest.skipIf(
            python_version >= (3, 12),
            '__contains__ now returns True/False for all inputs',
            )
    def test_contains_er(self):
        Season = self.Season
        self.assertIn(Season.AUTUMN, Season)
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                3 in Season
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                'AUTUMN' in Season
        val = Season(3)
        self.assertIn(val, Season)
        #
        class OtherEnum(Enum):
            one = 1; two = 2
        self.assertNotIn(OtherEnum.two, Season)

    @unittest.skipIf(
            python_version < (3, 12),
            '__contains__ only works with enum memmbers before 3.12',
            )
    def test_contains_tf(self):
        Season = self.Season
        self.assertIn(Season.AUTUMN, Season)
        self.assertTrue(3 in Season)
        self.assertFalse('AUTUMN' in Season)
        val = Season(3)
        self.assertIn(val, Season)
        #
        class OtherEnum(Enum):
            one = 1; two = 2
        self.assertNotIn(OtherEnum.two, Season)

    def test_comparisons(self):
        Season = self.Season
        with self.assertRaises(TypeError):
            Season.SPRING < Season.WINTER
        with self.assertRaises(TypeError):
            Season.SPRING > 4

        self.assertNotEqual(Season.SPRING, 1)

        class Part(Enum):
            SPRING = 1
            CLIP = 2
            BARREL = 3

        self.assertNotEqual(Season.SPRING, Part.SPRING)
        with self.assertRaises(TypeError):
            Season.SPRING < Part.CLIP

    def test_enum_duplicates(self):
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = FALL = 3
            WINTER = 4
            ANOTHER_SPRING = 1
        lst = list(Season)
        self.assertEqual(
            lst,
            [Season.SPRING, Season.SUMMER,
             Season.AUTUMN, Season.WINTER,
            ])
        self.assertIs(Season.FALL, Season.AUTUMN)
        self.assertEqual(Season.FALL.value, 3)
        self.assertEqual(Season.AUTUMN.value, 3)
        self.assertIs(Season(3), Season.AUTUMN)
        self.assertIs(Season(1), Season.SPRING)
        self.assertEqual(Season.FALL.name, 'AUTUMN')
        self.assertEqual(
                [k for k,v in Season.__members__.items() if v.name != k],
                ['FALL', 'ANOTHER_SPRING'],
                )

    def test_duplicate_name(self):
        with self.assertRaises(TypeError):
            class Color(Enum):
                red = 1
                green = 2
                blue = 3
                red = 4

        with self.assertRaises(TypeError):
            class Color(Enum):
                red = 1
                green = 2
                blue = 3
                def red(self):
                    return 'red'

        with self.assertRaises(TypeError):
            class Color(Enum):
                @property
                def red(self):
                    return 'redder'
                red = 1
                green = 2
                blue = 3

    def test_reserved__sunder_(self):
        with self.assertRaisesRegex(
                ValueError,
                '_sunder_ names, such as ._bad_., are reserved',
            ):
            class Bad(Enum):
                _bad_ = 1

    def test_enum_with_value_name(self):
        class Huh(Enum):
            name = 1
            value = 2
        self.assertEqual(
            list(Huh),
            [Huh.name, Huh.value],
            )
        self.assertIs(type(Huh.name), Huh)
        self.assertEqual(Huh.name.name, 'name')
        self.assertEqual(Huh.name.value, 1)

    def test_format_enum(self):
        Season = self.Season
        self.assertEqual('{}'.format(Season.SPRING),
                         '{}'.format(str(Season.SPRING)))
        self.assertEqual( '{:}'.format(Season.SPRING),
                          '{:}'.format(str(Season.SPRING)))
        self.assertEqual('{:20}'.format(Season.SPRING),
                         '{:20}'.format(str(Season.SPRING)))
        self.assertEqual('{:^20}'.format(Season.SPRING),
                         '{:^20}'.format(str(Season.SPRING)))
        self.assertEqual('{:>20}'.format(Season.SPRING),
                         '{:>20}'.format(str(Season.SPRING)))
        self.assertEqual('{:<20}'.format(Season.SPRING),
                         '{:<20}'.format(str(Season.SPRING)))

    def test_str_override_enum(self):
        class EnumWithStrOverrides(Enum):
            one = auto()
            two = auto()

            def __str__(self):
                return 'Str!'
        self.assertEqual(str(EnumWithStrOverrides.one), 'Str!')
        self.assertEqual('{}'.format(EnumWithStrOverrides.one), 'Str!')

    def test_format_override_enum(self):
        class EnumWithFormatOverride(Enum):
            one = 1.0
            two = 2.0
            def __format__(self, spec):
                return 'Format!!'
        self.assertEqual(str(EnumWithFormatOverride.one), 'one')
        self.assertEqual('{}'.format(EnumWithFormatOverride.one), 'Format!!')

    def test_str_and_format_override_enum(self):
        class EnumWithStrFormatOverrides(Enum):
            one = auto()
            two = auto()
            def __str__(self):
                return 'Str!'
            def __format__(self, spec):
                return 'Format!'
        self.assertEqual(str(EnumWithStrFormatOverrides.one), 'Str!')
        self.assertEqual('{}'.format(EnumWithStrFormatOverrides.one), 'Format!')

    def test_str_override_mixin(self):
        class MixinEnumWithStrOverride(float, Enum):
            one = 1.0
            two = 2.0
            def __str__(self):
                return 'Overridden!'
        self.assertEqual(str(MixinEnumWithStrOverride.one), 'Overridden!')
        self.assertEqual('{}'.format(MixinEnumWithStrOverride.one), 'Overridden!')

    def test_str_and_format_override_mixin(self):
        class MixinWithStrFormatOverrides(float, Enum):
            one = 1.0
            two = 2.0
            def __str__(self):
                return 'Str!'
            def __format__(self, spec):
                return 'Format!'
        self.assertEqual(str(MixinWithStrFormatOverrides.one), 'Str!')
        self.assertEqual('{}'.format(MixinWithStrFormatOverrides.one), 'Format!')

    def test_format_override_mixin(self):
        class TestFloat(float, Enum):
            one = 1.0
            two = 2.0
            def __format__(self, spec):
                return 'TestFloat success!'
        self.assertEqual(str(TestFloat.one), 'one')
        self.assertEqual('{}'.format(TestFloat.one), 'TestFloat success!')

    @unittest.skipIf(
            python_version < (3, 12),
            'mixin-format is still using member.value',
            )
    def test_mixin_format_warning(self):
        class Grades(int, Enum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 0
        self.assertEqual(f'{self.Grades.B}', 'B')

    @unittest.skipIf(
            python_version >= (3, 12),
            'mixin-format now uses member instead of member.value',
            )
    def test_mixin_format_warning(self):
        class Grades(int, Enum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 0
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(f'{Grades.B}', '4')

    def assertFormatIsValue(self, spec, member):
        if python_version < (3, 12) and (not spec or spec in ('{}','{:}')):
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(spec.format(member), spec.format(member.value))
        else:
            self.assertEqual(spec.format(member), spec.format(member.value))

    def test_format_enum_date(self):
        Holiday = self.Holiday
        self.assertFormatIsValue('{}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:20}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:^20}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:>20}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:<20}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:%Y %m}', Holiday.IDES_OF_MARCH)
        self.assertFormatIsValue('{:%Y %m %M:00}', Holiday.IDES_OF_MARCH)

    def test_format_enum_float(self):
        Konstants = self.Konstants
        self.assertFormatIsValue('{}', Konstants.TAU)
        self.assertFormatIsValue('{:}', Konstants.TAU)
        self.assertFormatIsValue('{:20}', Konstants.TAU)
        self.assertFormatIsValue('{:^20}', Konstants.TAU)
        self.assertFormatIsValue('{:>20}', Konstants.TAU)
        self.assertFormatIsValue('{:<20}', Konstants.TAU)
        self.assertFormatIsValue('{:n}', Konstants.TAU)
        self.assertFormatIsValue('{:5.2}', Konstants.TAU)
        self.assertFormatIsValue('{:f}', Konstants.TAU)

    def test_format_enum_int(self):
        class Grades(int, Enum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 0
        self.assertFormatIsValue('{}', Grades.C)
        self.assertFormatIsValue('{:}', Grades.C)
        self.assertFormatIsValue('{:20}', Grades.C)
        self.assertFormatIsValue('{:^20}', Grades.C)
        self.assertFormatIsValue('{:>20}', Grades.C)
        self.assertFormatIsValue('{:<20}', Grades.C)
        self.assertFormatIsValue('{:+}', Grades.C)
        self.assertFormatIsValue('{:08X}', Grades.C)
        self.assertFormatIsValue('{:b}', Grades.C)

    def test_format_enum_str(self):
        Directional = self.Directional
        self.assertFormatIsValue('{}', Directional.WEST)
        self.assertFormatIsValue('{:}', Directional.WEST)
        self.assertFormatIsValue('{:20}', Directional.WEST)
        self.assertFormatIsValue('{:^20}', Directional.WEST)
        self.assertFormatIsValue('{:>20}', Directional.WEST)
        self.assertFormatIsValue('{:<20}', Directional.WEST)

    def test_object_str_override(self):
        class Colors(Enum):
            RED, GREEN, BLUE = 1, 2, 3
            def __repr__(self):
                return "test.%s" % (self._name_, )
            __str__ = object.__str__
        self.assertEqual(str(Colors.RED), 'test.RED')

    def test_enum_str_override(self):
        class MyStrEnum(Enum):
            def __str__(self):
                return 'MyStr'
        class MyMethodEnum(Enum):
            def hello(self):
                return 'Hello!  My name is %s' % self.name
        class Test1Enum(MyMethodEnum, int, MyStrEnum):
            One = 1
            Two = 2
        self.assertTrue(Test1Enum._member_type_ is int)
        self.assertEqual(str(Test1Enum.One), 'MyStr')
        self.assertEqual(format(Test1Enum.One, ''), 'MyStr')
        #
        class Test2Enum(MyStrEnum, MyMethodEnum):
            One = 1
            Two = 2
        self.assertEqual(str(Test2Enum.One), 'MyStr')
        self.assertEqual(format(Test1Enum.One, ''), 'MyStr')

    def test_inherited_data_type(self):
        class HexInt(int):
            def __repr__(self):
                return hex(self)
        class MyEnum(HexInt, enum.Enum):
            A = 1
            B = 2
            C = 3
            def __repr__(self):
                return '<%s.%s: %r>' % (self.__class__.__name__, self._name_, self._value_)
        self.assertEqual(repr(MyEnum.A), '<MyEnum.A: 0x1>')
        #
        class SillyInt(HexInt):
            __qualname__ = 'SillyInt'
            pass
        class MyOtherEnum(SillyInt, enum.Enum):
            __qualname__ = 'MyOtherEnum'
            D = 4
            E = 5
            F = 6
        self.assertIs(MyOtherEnum._member_type_, SillyInt)
        globals()['SillyInt'] = SillyInt
        globals()['MyOtherEnum'] = MyOtherEnum
        test_pickle_dump_load(self.assertIs, MyOtherEnum.E)
        test_pickle_dump_load(self.assertIs, MyOtherEnum)
        #
        # This did not work in 3.9, but does now with pickling by name
        class UnBrokenInt(int):
            __qualname__ = 'UnBrokenInt'
            def __new__(cls, value):
                return int.__new__(cls, value)
        class MyUnBrokenEnum(UnBrokenInt, Enum):
            __qualname__ = 'MyUnBrokenEnum'
            G = 7
            H = 8
            I = 9
        self.assertIs(MyUnBrokenEnum._member_type_, UnBrokenInt)
        self.assertIs(MyUnBrokenEnum(7), MyUnBrokenEnum.G)
        globals()['UnBrokenInt'] = UnBrokenInt
        globals()['MyUnBrokenEnum'] = MyUnBrokenEnum
        test_pickle_dump_load(self.assertIs, MyUnBrokenEnum.I)
        test_pickle_dump_load(self.assertIs, MyUnBrokenEnum)

    def test_too_many_data_types(self):
        with self.assertRaisesRegex(TypeError, 'too many data types'):
            class Huh(str, int, Enum):
                One = 1

        class MyStr(str):
            def hello(self):
                return 'hello, %s' % self
        class MyInt(int):
            def repr(self):
                return hex(self)
        with self.assertRaisesRegex(TypeError, 'too many data types'):
            class Huh(MyStr, MyInt, Enum):
                One = 1

    def test_hash(self):
        Season = self.Season
        dates = {}
        dates[Season.WINTER] = '1225'
        dates[Season.SPRING] = '0315'
        dates[Season.SUMMER] = '0704'
        dates[Season.AUTUMN] = '1031'
        self.assertEqual(dates[Season.AUTUMN], '1031')

    def test_intenum_from_scratch(self):
        class phy(int, Enum):
            pi = 3
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_intenum_inherited(self):
        class IntEnum(int, Enum):
            pass
        class phy(IntEnum):
            pi = 3
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_floatenum_from_scratch(self):
        class phy(float, Enum):
            pi = 3.1415926
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_floatenum_inherited(self):
        class FloatEnum(float, Enum):
            pass
        class phy(FloatEnum):
            pi = 3.1415926
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_strenum_from_scratch(self):
        class phy(str, Enum):
            pi = 'Pi'
            tau = 'Tau'
        self.assertTrue(phy.pi < phy.tau)

    def test_strenum_inherited_methods(self):
        class phy(StrEnum):
            pi = 'Pi'
            tau = 'Tau'
        self.assertTrue(phy.pi < phy.tau)
        self.assertEqual(phy.pi.upper(), 'PI')
        self.assertEqual(phy.tau.count('a'), 1)

    def test_intenum(self):
        class WeekDay(IntEnum):
            SUNDAY = 1
            MONDAY = 2
            TUESDAY = 3
            WEDNESDAY = 4
            THURSDAY = 5
            FRIDAY = 6
            SATURDAY = 7

        self.assertEqual(['a', 'b', 'c'][WeekDay.MONDAY], 'c')
        self.assertEqual([i for i in range(WeekDay.TUESDAY)], [0, 1, 2])

        lst = list(WeekDay)
        self.assertEqual(len(lst), len(WeekDay))
        self.assertEqual(len(WeekDay), 7)
        target = 'SUNDAY MONDAY TUESDAY WEDNESDAY THURSDAY FRIDAY SATURDAY'
        target = target.split()
        for i, weekday in enumerate(target, 1):
            e = WeekDay(i)
            self.assertEqual(e, i)
            self.assertEqual(int(e), i)
            self.assertEqual(e.name, weekday)
            self.assertIn(e, WeekDay)
            self.assertEqual(lst.index(e)+1, i)
            self.assertTrue(0 < e < 8)
            self.assertIs(type(e), WeekDay)
            self.assertIsInstance(e, int)
            self.assertIsInstance(e, Enum)

    def test_intenum_duplicates(self):
        class WeekDay(IntEnum):
            SUNDAY = 1
            MONDAY = 2
            TUESDAY = TEUSDAY = 3
            WEDNESDAY = 4
            THURSDAY = 5
            FRIDAY = 6
            SATURDAY = 7
        self.assertIs(WeekDay.TEUSDAY, WeekDay.TUESDAY)
        self.assertEqual(WeekDay(3).name, 'TUESDAY')
        self.assertEqual([k for k,v in WeekDay.__members__.items()
                if v.name != k], ['TEUSDAY', ])

    def test_intenum_from_bytes(self):
        self.assertIs(IntStooges.from_bytes(b'\x00\x03', 'big'), IntStooges.MOE)
        with self.assertRaises(ValueError):
            IntStooges.from_bytes(b'\x00\x05', 'big')

    def test_floatenum_fromhex(self):
        h = float.hex(FloatStooges.MOE.value)
        self.assertIs(FloatStooges.fromhex(h), FloatStooges.MOE)
        h = float.hex(FloatStooges.MOE.value + 0.01)
        with self.assertRaises(ValueError):
            FloatStooges.fromhex(h)

    def test_pickle_enum(self):
        if isinstance(Stooges, Exception):
            raise Stooges
        test_pickle_dump_load(self.assertIs, Stooges.CURLY)
        test_pickle_dump_load(self.assertIs, Stooges)

    def test_pickle_int(self):
        if isinstance(IntStooges, Exception):
            raise IntStooges
        test_pickle_dump_load(self.assertIs, IntStooges.CURLY)
        test_pickle_dump_load(self.assertIs, IntStooges)

    def test_pickle_float(self):
        if isinstance(FloatStooges, Exception):
            raise FloatStooges
        test_pickle_dump_load(self.assertIs, FloatStooges.CURLY)
        test_pickle_dump_load(self.assertIs, FloatStooges)

    def test_pickle_enum_function(self):
        if isinstance(Answer, Exception):
            raise Answer
        test_pickle_dump_load(self.assertIs, Answer.him)
        test_pickle_dump_load(self.assertIs, Answer)

    def test_pickle_enum_function_with_module(self):
        if isinstance(Question, Exception):
            raise Question
        test_pickle_dump_load(self.assertIs, Question.who)
        test_pickle_dump_load(self.assertIs, Question)

    def test_enum_function_with_qualname(self):
        if isinstance(Theory, Exception):
            raise Theory
        self.assertEqual(Theory.__qualname__, 'spanish_inquisition')

    def test_class_nested_enum_and_pickle_protocol_four(self):
        # would normally just have this directly in the class namespace
        class NestedEnum(Enum):
            twigs = 'common'
            shiny = 'rare'

        self.__class__.NestedEnum = NestedEnum
        self.NestedEnum.__qualname__ = '%s.NestedEnum' % self.__class__.__name__
        test_pickle_dump_load(self.assertIs, self.NestedEnum.twigs)

    def test_pickle_by_name(self):
        class ReplaceGlobalInt(IntEnum):
            ONE = 1
            TWO = 2
        ReplaceGlobalInt.__reduce_ex__ = enum._reduce_ex_by_global_name
        for proto in range(HIGHEST_PROTOCOL):
            self.assertEqual(ReplaceGlobalInt.TWO.__reduce_ex__(proto), 'TWO')

    def test_exploding_pickle(self):
        BadPickle = Enum(
                'BadPickle', 'dill sweet bread-n-butter', module=__name__)
        globals()['BadPickle'] = BadPickle
        # now break BadPickle to test exception raising
        enum._make_class_unpicklable(BadPickle)
        test_pickle_exception(self.assertRaises, TypeError, BadPickle.dill)
        test_pickle_exception(self.assertRaises, PicklingError, BadPickle)

    def test_string_enum(self):
        class SkillLevel(str, Enum):
            master = 'what is the sound of one hand clapping?'
            journeyman = 'why did the chicken cross the road?'
            apprentice = 'knock, knock!'
        self.assertEqual(SkillLevel.apprentice, 'knock, knock!')

    def test_getattr_getitem(self):
        class Period(Enum):
            morning = 1
            noon = 2
            evening = 3
            night = 4
        self.assertIs(Period(2), Period.noon)
        self.assertIs(getattr(Period, 'night'), Period.night)
        self.assertIs(Period['morning'], Period.morning)

    def test_getattr_dunder(self):
        Season = self.Season
        self.assertTrue(getattr(Season, '__eq__'))

    def test_iteration_order(self):
        class Season(Enum):
            SUMMER = 2
            WINTER = 4
            AUTUMN = 3
            SPRING = 1
        self.assertEqual(
                list(Season),
                [Season.SUMMER, Season.WINTER, Season.AUTUMN, Season.SPRING],
                )

    def test_reversed_iteration_order(self):
        self.assertEqual(
                list(reversed(self.Season)),
                [self.Season.WINTER, self.Season.AUTUMN, self.Season.SUMMER,
                 self.Season.SPRING]
                )

    def test_programmatic_function_string(self):
        SummerMonth = Enum('SummerMonth', 'june july august')
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_string_with_start(self):
        SummerMonth = Enum('SummerMonth', 'june july august', start=10)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 10):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_string_list(self):
        SummerMonth = Enum('SummerMonth', ['june', 'july', 'august'])
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_string_list_with_start(self):
        SummerMonth = Enum('SummerMonth', ['june', 'july', 'august'], start=20)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 20):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_iterable(self):
        SummerMonth = Enum(
                'SummerMonth',
                (('june', 1), ('july', 2), ('august', 3))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_from_dict(self):
        SummerMonth = Enum(
                'SummerMonth',
                OrderedDict((('june', 1), ('july', 2), ('august', 3)))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_type(self):
        SummerMonth = Enum('SummerMonth', 'june july august', type=int)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_type_with_start(self):
        SummerMonth = Enum('SummerMonth', 'june july august', type=int, start=30)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 30):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_type_from_subclass(self):
        SummerMonth = IntEnum('SummerMonth', 'june july august')
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 1):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_programmatic_function_type_from_subclass_with_start(self):
        SummerMonth = IntEnum('SummerMonth', 'june july august', start=40)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 40):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertIn(e, SummerMonth)
            self.assertIs(type(e), SummerMonth)

    def test_subclassing(self):
        if isinstance(Name, Exception):
            raise Name
        self.assertEqual(Name.BDFL, 'Guido van Rossum')
        self.assertTrue(Name.BDFL, Name('Guido van Rossum'))
        self.assertIs(Name.BDFL, getattr(Name, 'BDFL'))
        test_pickle_dump_load(self.assertIs, Name.BDFL)

    def test_extending(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
        with self.assertRaises(TypeError):
            class MoreColor(Color):
                cyan = 4
                magenta = 5
                yellow = 6
        with self.assertRaisesRegex(TypeError, "EvenMoreColor: cannot extend enumeration 'Color'"):
            class EvenMoreColor(Color, IntEnum):
                chartruese = 7

    def test_exclude_methods(self):
        class whatever(Enum):
            this = 'that'
            these = 'those'
            def really(self):
                return 'no, not %s' % self.value
        self.assertIsNot(type(whatever.really), whatever)
        self.assertEqual(whatever.this.really(), 'no, not that')

    def test_wrong_inheritance_order(self):
        with self.assertRaises(TypeError):
            class Wrong(Enum, str):
                NotHere = 'error before this point'

    def test_intenum_transitivity(self):
        class number(IntEnum):
            one = 1
            two = 2
            three = 3
        class numero(IntEnum):
            uno = 1
            dos = 2
            tres = 3
        self.assertEqual(number.one, numero.uno)
        self.assertEqual(number.two, numero.dos)
        self.assertEqual(number.three, numero.tres)

    def test_wrong_enum_in_call(self):
        class Monochrome(Enum):
            black = 0
            white = 1
        class Gender(Enum):
            male = 0
            female = 1
        self.assertRaises(ValueError, Monochrome, Gender.male)

    def test_wrong_enum_in_mixed_call(self):
        class Monochrome(IntEnum):
            black = 0
            white = 1
        class Gender(Enum):
            male = 0
            female = 1
        self.assertRaises(ValueError, Monochrome, Gender.male)

    def test_mixed_enum_in_call_1(self):
        class Monochrome(IntEnum):
            black = 0
            white = 1
        class Gender(IntEnum):
            male = 0
            female = 1
        self.assertIs(Monochrome(Gender.female), Monochrome.white)

    def test_mixed_enum_in_call_2(self):
        class Monochrome(Enum):
            black = 0
            white = 1
        class Gender(IntEnum):
            male = 0
            female = 1
        self.assertIs(Monochrome(Gender.male), Monochrome.black)

    def test_flufl_enum(self):
        class Fluflnum(Enum):
            def __int__(self):
                return int(self.value)
        class MailManOptions(Fluflnum):
            option1 = 1
            option2 = 2
            option3 = 3
        self.assertEqual(int(MailManOptions.option1), 1)

    def test_introspection(self):
        class Number(IntEnum):
            one = 100
            two = 200
        self.assertIs(Number.one._member_type_, int)
        self.assertIs(Number._member_type_, int)
        class String(str, Enum):
            yarn = 'soft'
            rope = 'rough'
            wire = 'hard'
        self.assertIs(String.yarn._member_type_, str)
        self.assertIs(String._member_type_, str)
        class Plain(Enum):
            vanilla = 'white'
            one = 1
        self.assertIs(Plain.vanilla._member_type_, object)
        self.assertIs(Plain._member_type_, object)

    def test_no_such_enum_member(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
        with self.assertRaises(ValueError):
            Color(4)
        with self.assertRaises(KeyError):
            Color['chartreuse']

    def test_new_repr(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
            def __repr__(self):
                return "don't you just love shades of %s?" % self.name
        self.assertEqual(
                repr(Color.blue),
                "don't you just love shades of blue?",
                )

    def test_inherited_repr(self):
        class MyEnum(Enum):
            def __repr__(self):
                return "My name is %s." % self.name
        class MyIntEnum(int, MyEnum):
            this = 1
            that = 2
            theother = 3
        self.assertEqual(repr(MyIntEnum.that), "My name is that.")

    def test_multiple_mixin_mro(self):
        class auto_enum(type(Enum)):
            def __new__(metacls, cls, bases, classdict):
                temp = type(classdict)()
                temp._cls_name = cls
                names = set(classdict._member_names)
                i = 0
                for k in classdict._member_names:
                    v = classdict[k]
                    if v is Ellipsis:
                        v = i
                    else:
                        i = v
                    i += 1
                    temp[k] = v
                for k, v in classdict.items():
                    if k not in names:
                        temp[k] = v
                return super(auto_enum, metacls).__new__(
                        metacls, cls, bases, temp)

        class AutoNumberedEnum(Enum, metaclass=auto_enum):
            pass

        class AutoIntEnum(IntEnum, metaclass=auto_enum):
            pass

        class TestAutoNumber(AutoNumberedEnum):
            a = ...
            b = 3
            c = ...

        class TestAutoInt(AutoIntEnum):
            a = ...
            b = 3
            c = ...

    def test_subclasses_with_getnewargs(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __getnewargs__(self):
                return self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp,
                        )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)


        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_subclasses_with_getnewargs_ex(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __getnewargs_ex__(self):
                return self._args, {}
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp,
                        )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)


        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_subclasses_with_reduce(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __reduce__(self):
                return self.__class__, self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp,
                        )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)


        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_subclasses_with_reduce_ex(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __reduce_ex__(self, proto):
                return self.__class__, self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp,
                        )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)

        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_subclasses_without_direct_pickle_support(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'
            x = ('the-x', 1)
            y = ('the-y', 2)

        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_subclasses_with_direct_pickle_support(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'
            def __new__(cls, *args):
                _args = args
                name, *args = args
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "{}({!r}, {})".format(
                        type(self).__name__,
                        self.__name__,
                        int.__repr__(self),
                        )
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '({0} + {1})'.format(self.__name__, other.__name__),
                        temp,
                        )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'
            x = ('the-x', 1)
            y = ('the-y', 2)
            def __reduce_ex__(self, proto):
                return getattr, (self.__class__, self._name_)

        self.assertIs(NEI.__new__, Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertIs, NEI.y)
        test_pickle_dump_load(self.assertIs, NEI)

    def test_tuple_subclass(self):
        class SomeTuple(tuple, Enum):
            __qualname__ = 'SomeTuple'      # needed for pickle protocol 4
            first = (1, 'for the money')
            second = (2, 'for the show')
            third = (3, 'for the music')
        self.assertIs(type(SomeTuple.first), SomeTuple)
        self.assertIsInstance(SomeTuple.second, tuple)
        self.assertEqual(SomeTuple.third, (3, 'for the music'))
        globals()['SomeTuple'] = SomeTuple
        test_pickle_dump_load(self.assertIs, SomeTuple.first)

    def test_duplicate_values_give_unique_enum_items(self):
        class AutoNumber(Enum):
            first = ()
            second = ()
            third = ()
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
            def __int__(self):
                return int(self._value_)
        self.assertEqual(
                list(AutoNumber),
                [AutoNumber.first, AutoNumber.second, AutoNumber.third],
                )
        self.assertEqual(int(AutoNumber.second), 2)
        self.assertEqual(AutoNumber.third.value, 3)
        self.assertIs(AutoNumber(1), AutoNumber.first)

    def test_inherited_new_from_enhanced_enum(self):
        class AutoNumber(Enum):
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
            def __int__(self):
                return int(self._value_)
        class Color(AutoNumber):
            red = ()
            green = ()
            blue = ()
        self.assertEqual(list(Color), [Color.red, Color.green, Color.blue])
        self.assertEqual(list(map(int, Color)), [1, 2, 3])

    def test_inherited_new_from_mixed_enum(self):
        class AutoNumber(IntEnum):
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = int.__new__(cls, value)
                obj._value_ = value
                return obj
        class Color(AutoNumber):
            red = ()
            green = ()
            blue = ()
        self.assertEqual(list(Color), [Color.red, Color.green, Color.blue])
        self.assertEqual(list(map(int, Color)), [1, 2, 3])

    def test_equality(self):
        class OrdinaryEnum(Enum):
            a = 1
        self.assertEqual(ALWAYS_EQ, OrdinaryEnum.a)
        self.assertEqual(OrdinaryEnum.a, ALWAYS_EQ)

    def test_ordered_mixin(self):
        class OrderedEnum(Enum):
            def __ge__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ >= other._value_
                return NotImplemented
            def __gt__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ > other._value_
                return NotImplemented
            def __le__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ <= other._value_
                return NotImplemented
            def __lt__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ < other._value_
                return NotImplemented
        class Grade(OrderedEnum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 1
        self.assertGreater(Grade.A, Grade.B)
        self.assertLessEqual(Grade.F, Grade.C)
        self.assertLess(Grade.D, Grade.A)
        self.assertGreaterEqual(Grade.B, Grade.B)
        self.assertEqual(Grade.B, Grade.B)
        self.assertNotEqual(Grade.C, Grade.D)

    def test_extending2(self):
        class Shade(Enum):
            def shade(self):
                print(self.name)
        class Color(Shade):
            red = 1
            green = 2
            blue = 3
        with self.assertRaises(TypeError):
            class MoreColor(Color):
                cyan = 4
                magenta = 5
                yellow = 6

    def test_extending3(self):
        class Shade(Enum):
            def shade(self):
                return self.name
        class Color(Shade):
            def hex(self):
                return '%s hexlified!' % self.value
        class MoreColor(Color):
            cyan = 4
            magenta = 5
            yellow = 6
        self.assertEqual(MoreColor.magenta.hex(), '5 hexlified!')

    def test_subclass_duplicate_name(self):
        class Base(Enum):
            def test(self):
                pass
        class Test(Base):
            test = 1
        self.assertIs(type(Test.test), Test)

    def test_subclass_duplicate_name_dynamic(self):
        from types import DynamicClassAttribute
        class Base(Enum):
            @DynamicClassAttribute
            def test(self):
                return 'dynamic'
        class Test(Base):
            test = 1
        self.assertEqual(Test.test.test, 'dynamic')
        class Base2(Enum):
            @enum.property
            def flash(self):
                return 'flashy dynamic'
        class Test(Base2):
            flash = 1
        self.assertEqual(Test.flash.flash, 'flashy dynamic')

    def test_no_duplicates(self):
        class UniqueEnum(Enum):
            def __init__(self, *args):
                cls = self.__class__
                if any(self.value == e.value for e in cls):
                    a = self.name
                    e = cls(self.value).name
                    raise ValueError(
                            "aliases not allowed in UniqueEnum:  %r --> %r"
                            % (a, e)
                            )
        class Color(UniqueEnum):
            red = 1
            green = 2
            blue = 3
        with self.assertRaises(ValueError):
            class Color(UniqueEnum):
                red = 1
                green = 2
                blue = 3
                grene = 2

    def test_init(self):
        class Planet(Enum):
            MERCURY = (3.303e+23, 2.4397e6)
            VENUS   = (4.869e+24, 6.0518e6)
            EARTH   = (5.976e+24, 6.37814e6)
            MARS    = (6.421e+23, 3.3972e6)
            JUPITER = (1.9e+27,   7.1492e7)
            SATURN  = (5.688e+26, 6.0268e7)
            URANUS  = (8.686e+25, 2.5559e7)
            NEPTUNE = (1.024e+26, 2.4746e7)
            def __init__(self, mass, radius):
                self.mass = mass       # in kilograms
                self.radius = radius   # in meters
            @property
            def surface_gravity(self):
                # universal gravitational constant  (m3 kg-1 s-2)
                G = 6.67300E-11
                return G * self.mass / (self.radius * self.radius)
        self.assertEqual(round(Planet.EARTH.surface_gravity, 2), 9.80)
        self.assertEqual(Planet.EARTH.value, (5.976e+24, 6.37814e6))

    def test_ignore(self):
        class Period(timedelta, Enum):
            '''
            different lengths of time
            '''
            def __new__(cls, value, period):
                obj = timedelta.__new__(cls, value)
                obj._value_ = value
                obj.period = period
                return obj
            _ignore_ = 'Period i'
            Period = vars()
            for i in range(13):
                Period['month_%d' % i] = i*30, 'month'
            for i in range(53):
                Period['week_%d' % i] = i*7, 'week'
            for i in range(32):
                Period['day_%d' % i] = i, 'day'
            OneDay = day_1
            OneWeek = week_1
            OneMonth = month_1
        self.assertFalse(hasattr(Period, '_ignore_'))
        self.assertFalse(hasattr(Period, 'Period'))
        self.assertFalse(hasattr(Period, 'i'))
        self.assertTrue(isinstance(Period.day_1, timedelta))
        self.assertTrue(Period.month_1 is Period.day_30)
        self.assertTrue(Period.week_4 is Period.day_28)

    def test_nonhash_value(self):
        class AutoNumberInAList(Enum):
            def __new__(cls):
                value = [len(cls.__members__) + 1]
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
        class ColorInAList(AutoNumberInAList):
            red = ()
            green = ()
            blue = ()
        self.assertEqual(list(ColorInAList), [ColorInAList.red, ColorInAList.green, ColorInAList.blue])
        for enum, value in zip(ColorInAList, range(3)):
            value += 1
            self.assertEqual(enum.value, [value])
            self.assertIs(ColorInAList([value]), enum)

    def test_conflicting_types_resolved_in_new(self):
        class LabelledIntEnum(int, Enum):
            def __new__(cls, *args):
                value, label = args
                obj = int.__new__(cls, value)
                obj.label = label
                obj._value_ = value
                return obj

        class LabelledList(LabelledIntEnum):
            unprocessed = (1, "Unprocessed")
            payment_complete = (2, "Payment Complete")

        self.assertEqual(list(LabelledList), [LabelledList.unprocessed, LabelledList.payment_complete])
        self.assertEqual(LabelledList.unprocessed, 1)
        self.assertEqual(LabelledList(1), LabelledList.unprocessed)

    def test_auto_number(self):
        class Color(Enum):
            red = auto()
            blue = auto()
            green = auto()

        self.assertEqual(list(Color), [Color.red, Color.blue, Color.green])
        self.assertEqual(Color.red.value, 1)
        self.assertEqual(Color.blue.value, 2)
        self.assertEqual(Color.green.value, 3)

    def test_auto_name(self):
        class Color(Enum):
            def _generate_next_value_(name, start, count, last):
                return name
            red = auto()
            blue = auto()
            green = auto()

        self.assertEqual(list(Color), [Color.red, Color.blue, Color.green])
        self.assertEqual(Color.red.value, 'red')
        self.assertEqual(Color.blue.value, 'blue')
        self.assertEqual(Color.green.value, 'green')

    def test_auto_name_inherit(self):
        class AutoNameEnum(Enum):
            def _generate_next_value_(name, start, count, last):
                return name
        class Color(AutoNameEnum):
            red = auto()
            blue = auto()
            green = auto()

        self.assertEqual(list(Color), [Color.red, Color.blue, Color.green])
        self.assertEqual(Color.red.value, 'red')
        self.assertEqual(Color.blue.value, 'blue')
        self.assertEqual(Color.green.value, 'green')

    def test_auto_garbage(self):
        class Color(Enum):
            red = 'red'
            blue = auto()
        self.assertEqual(Color.blue.value, 1)

    def test_auto_garbage_corrected(self):
        class Color(Enum):
            red = 'red'
            blue = 2
            green = auto()

        self.assertEqual(list(Color), [Color.red, Color.blue, Color.green])
        self.assertEqual(Color.red.value, 'red')
        self.assertEqual(Color.blue.value, 2)
        self.assertEqual(Color.green.value, 3)

    def test_auto_order(self):
        with self.assertRaises(TypeError):
            class Color(Enum):
                red = auto()
                green = auto()
                blue = auto()
                def _generate_next_value_(name, start, count, last):
                    return name

    def test_auto_order_wierd(self):
        weird_auto = auto()
        weird_auto.value = 'pathological case'
        class Color(Enum):
            red = weird_auto
            def _generate_next_value_(name, start, count, last):
                return name
            blue = auto()
        self.assertEqual(list(Color), [Color.red, Color.blue])
        self.assertEqual(Color.red.value, 'pathological case')
        self.assertEqual(Color.blue.value, 'blue')

    def test_duplicate_auto(self):
        class Dupes(Enum):
            first = primero = auto()
            second = auto()
            third = auto()
        self.assertEqual([Dupes.first, Dupes.second, Dupes.third], list(Dupes))

    def test_default_missing(self):
        class Color(Enum):
            RED = 1
            GREEN = 2
            BLUE = 3
        try:
            Color(7)
        except ValueError as exc:
            self.assertTrue(exc.__context__ is None)
        else:
            raise Exception('Exception not raised.')

    def test_missing(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
            @classmethod
            def _missing_(cls, item):
                if item == 'three':
                    return cls.blue
                elif item == 'bad return':
                    # trigger internal error
                    return 5
                elif item == 'error out':
                    raise ZeroDivisionError
                else:
                    # trigger not found
                    return None
        self.assertIs(Color('three'), Color.blue)
        try:
            Color(7)
        except ValueError as exc:
            self.assertTrue(exc.__context__ is None)
        else:
            raise Exception('Exception not raised.')
        try:
            Color('bad return')
        except TypeError as exc:
            self.assertTrue(isinstance(exc.__context__, ValueError))
        else:
            raise Exception('Exception not raised.')
        try:
            Color('error out')
        except ZeroDivisionError as exc:
            self.assertTrue(isinstance(exc.__context__, ValueError))
        else:
            raise Exception('Exception not raised.')

    def test_missing_exceptions_reset(self):
        import weakref
        #
        class TestEnum(enum.Enum):
            VAL1 = 'val1'
            VAL2 = 'val2'
        #
        class Class1:
            def __init__(self):
                # Gracefully handle an exception of our own making
                try:
                    raise ValueError()
                except ValueError:
                    pass
        #
        class Class2:
            def __init__(self):
                # Gracefully handle an exception of Enum's making
                try:
                    TestEnum('invalid_value')
                except ValueError:
                    pass
        # No strong refs here so these are free to die.
        class_1_ref = weakref.ref(Class1())
        class_2_ref = weakref.ref(Class2())
        #
        # The exception raised by Enum creates a reference loop and thus
        # Class2 instances will stick around until the next gargage collection
        # cycle, unlike Class1.
        self.assertIs(class_1_ref(), None)
        self.assertIs(class_2_ref(), None)

    def test_multiple_mixin(self):
        class MaxMixin:
            @classproperty
            def MAX(cls):
                max = len(cls)
                cls.MAX = max
                return max
        class StrMixin:
            def __str__(self):
                return self._name_.lower()
        class SomeEnum(Enum):
            def behavior(self):
                return 'booyah'
        class AnotherEnum(Enum):
            def behavior(self):
                return 'nuhuh!'
            def social(self):
                return "what's up?"
        class Color(MaxMixin, Enum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 3)
        self.assertEqual(Color.MAX, 3)
        self.assertEqual(str(Color.BLUE), 'BLUE')
        class Color(MaxMixin, StrMixin, Enum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 3)
        self.assertEqual(Color.MAX, 3)
        self.assertEqual(str(Color.BLUE), 'blue')
        class Color(StrMixin, MaxMixin, Enum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 3)
        self.assertEqual(Color.MAX, 3)
        self.assertEqual(str(Color.BLUE), 'blue')
        class CoolColor(StrMixin, SomeEnum, Enum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(CoolColor.RED.value, 1)
        self.assertEqual(CoolColor.GREEN.value, 2)
        self.assertEqual(CoolColor.BLUE.value, 3)
        self.assertEqual(str(CoolColor.BLUE), 'blue')
        self.assertEqual(CoolColor.RED.behavior(), 'booyah')
        class CoolerColor(StrMixin, AnotherEnum, Enum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(CoolerColor.RED.value, 1)
        self.assertEqual(CoolerColor.GREEN.value, 2)
        self.assertEqual(CoolerColor.BLUE.value, 3)
        self.assertEqual(str(CoolerColor.BLUE), 'blue')
        self.assertEqual(CoolerColor.RED.behavior(), 'nuhuh!')
        self.assertEqual(CoolerColor.RED.social(), "what's up?")
        class CoolestColor(StrMixin, SomeEnum, AnotherEnum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(CoolestColor.RED.value, 1)
        self.assertEqual(CoolestColor.GREEN.value, 2)
        self.assertEqual(CoolestColor.BLUE.value, 3)
        self.assertEqual(str(CoolestColor.BLUE), 'blue')
        self.assertEqual(CoolestColor.RED.behavior(), 'booyah')
        self.assertEqual(CoolestColor.RED.social(), "what's up?")
        class ConfusedColor(StrMixin, AnotherEnum, SomeEnum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(ConfusedColor.RED.value, 1)
        self.assertEqual(ConfusedColor.GREEN.value, 2)
        self.assertEqual(ConfusedColor.BLUE.value, 3)
        self.assertEqual(str(ConfusedColor.BLUE), 'blue')
        self.assertEqual(ConfusedColor.RED.behavior(), 'nuhuh!')
        self.assertEqual(ConfusedColor.RED.social(), "what's up?")
        class ReformedColor(StrMixin, IntEnum, SomeEnum, AnotherEnum):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(ReformedColor.RED.value, 1)
        self.assertEqual(ReformedColor.GREEN.value, 2)
        self.assertEqual(ReformedColor.BLUE.value, 3)
        self.assertEqual(str(ReformedColor.BLUE), 'blue')
        self.assertEqual(ReformedColor.RED.behavior(), 'booyah')
        self.assertEqual(ConfusedColor.RED.social(), "what's up?")
        self.assertTrue(issubclass(ReformedColor, int))

    def test_multiple_inherited_mixin(self):
        @unique
        class Decision1(StrEnum):
            REVERT = "REVERT"
            REVERT_ALL = "REVERT_ALL"
            RETRY = "RETRY"
        class MyEnum(StrEnum):
            pass
        @unique
        class Decision2(MyEnum):
            REVERT = "REVERT"
            REVERT_ALL = "REVERT_ALL"
            RETRY = "RETRY"

    def test_multiple_mixin_inherited(self):
        class MyInt(int):
            def __new__(cls, value):
                return super().__new__(cls, value)

        class HexMixin:
            def __repr__(self):
                return hex(self)

        class MyIntEnum(HexMixin, MyInt, enum.Enum):
            pass

        class Foo(MyIntEnum):
            TEST = 1
        self.assertTrue(isinstance(Foo.TEST, MyInt))
        self.assertEqual(repr(Foo.TEST), "0x1")

        class Fee(MyIntEnum):
            TEST = 1
            def __new__(cls, value):
                value += 1
                member = int.__new__(cls, value)
                member._value_ = value
                return member
        self.assertEqual(Fee.TEST, 2)

    def test_miltuple_mixin_with_common_data_type(self):
        class CaseInsensitiveStrEnum(str, Enum):
            @classmethod
            def _missing_(cls, value):
                for member in cls._member_map_.values():
                    if member._value_.lower() == value.lower():
                        return member
                return super()._missing_(value)
        #
        class LenientStrEnum(str, Enum):
            def __init__(self, *args):
                self._valid = True
            @classmethod
            def _missing_(cls, value):
                unknown = cls._member_type_.__new__(cls, value)
                unknown._valid = False
                unknown._name_ = value.upper()
                unknown._value_ = value
                cls._member_map_[value] = unknown
                return unknown
            @property
            def valid(self):
                return self._valid
        #
        class JobStatus(CaseInsensitiveStrEnum, LenientStrEnum):
            ACTIVE = "active"
            PENDING = "pending"
            TERMINATED = "terminated"
        #
        JS = JobStatus
        self.assertEqual(list(JobStatus), [JS.ACTIVE, JS.PENDING, JS.TERMINATED])
        self.assertEqual(JS.ACTIVE, 'active')
        self.assertEqual(JS.ACTIVE.value, 'active')
        self.assertIs(JS('Active'), JS.ACTIVE)
        self.assertTrue(JS.ACTIVE.valid)
        missing = JS('missing')
        self.assertEqual(list(JobStatus), [JS.ACTIVE, JS.PENDING, JS.TERMINATED])
        self.assertEqual(JS.ACTIVE, 'active')
        self.assertEqual(JS.ACTIVE.value, 'active')
        self.assertIs(JS('Active'), JS.ACTIVE)
        self.assertTrue(JS.ACTIVE.valid)
        self.assertTrue(isinstance(missing, JS))
        self.assertFalse(missing.valid)

    def test_empty_globals(self):
        # bpo-35717: sys._getframe(2).f_globals['__name__'] fails with KeyError
        # when using compile and exec because f_globals is empty
        code = "from enum import Enum; Enum('Animal', 'ANT BEE CAT DOG')"
        code = compile(code, "<string>", "exec")
        global_ns = {}
        local_ls = {}
        exec(code, global_ns, local_ls)

    def test_strenum(self):
        class GoodStrEnum(StrEnum):
            one = '1'
            two = '2'
            three = b'3', 'ascii'
            four = b'4', 'latin1', 'strict'
        self.assertEqual(GoodStrEnum.one, '1')
        self.assertEqual(str(GoodStrEnum.one), '1')
        self.assertEqual('{}'.format(GoodStrEnum.one), '1')
        self.assertEqual(GoodStrEnum.one, str(GoodStrEnum.one))
        self.assertEqual(GoodStrEnum.one, '{}'.format(GoodStrEnum.one))
        self.assertEqual(repr(GoodStrEnum.one), 'GoodStrEnum.one')
        #
        class DumbMixin:
            def __str__(self):
                return "don't do this"
        class DumbStrEnum(DumbMixin, StrEnum):
            five = '5'
            six = '6'
            seven = '7'
        self.assertEqual(DumbStrEnum.seven, '7')
        self.assertEqual(str(DumbStrEnum.seven), "don't do this")
        #
        class EnumMixin(Enum):
            def hello(self):
                print('hello from %s' % (self, ))
        class HelloEnum(EnumMixin, StrEnum):
            eight = '8'
        self.assertEqual(HelloEnum.eight, '8')
        self.assertEqual(HelloEnum.eight, str(HelloEnum.eight))
        #
        class GoodbyeMixin:
            def goodbye(self):
                print('%s wishes you a fond farewell')
        class GoodbyeEnum(GoodbyeMixin, EnumMixin, StrEnum):
            nine = '9'
        self.assertEqual(GoodbyeEnum.nine, '9')
        self.assertEqual(GoodbyeEnum.nine, str(GoodbyeEnum.nine))
        #
        with self.assertRaisesRegex(TypeError, '1 is not a string'):
            class FirstFailedStrEnum(StrEnum):
                one = 1
                two = '2'
        with self.assertRaisesRegex(TypeError, "2 is not a string"):
            class SecondFailedStrEnum(StrEnum):
                one = '1'
                two = 2,
                three = '3'
        with self.assertRaisesRegex(TypeError, '2 is not a string'):
            class ThirdFailedStrEnum(StrEnum):
                one = '1'
                two = 2
        with self.assertRaisesRegex(TypeError, 'encoding must be a string, not %r' % (sys.getdefaultencoding, )):
            class ThirdFailedStrEnum(StrEnum):
                one = '1'
                two = b'2', sys.getdefaultencoding
        with self.assertRaisesRegex(TypeError, 'errors must be a string, not 9'):
            class ThirdFailedStrEnum(StrEnum):
                one = '1'
                two = b'2', 'ascii', 9

    @unittest.skipIf(
            python_version >= (3, 12),
            'mixin-format now uses member instead of member.value',
            )
    def test_custom_strenum_with_warning(self):
        class CustomStrEnum(str, Enum):
            pass
        class OkayEnum(CustomStrEnum):
            one = '1'
            two = '2'
            three = b'3', 'ascii'
            four = b'4', 'latin1', 'strict'
        self.assertEqual(OkayEnum.one, '1')
        self.assertEqual(str(OkayEnum.one), 'one')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual('{}'.format(OkayEnum.one), '1')
            self.assertEqual(OkayEnum.one, '{}'.format(OkayEnum.one))
        self.assertEqual(repr(OkayEnum.one), 'OkayEnum.one')
        #
        class DumbMixin:
            def __str__(self):
                return "don't do this"
        class DumbStrEnum(DumbMixin, CustomStrEnum):
            five = '5'
            six = '6'
            seven = '7'
        self.assertEqual(DumbStrEnum.seven, '7')
        self.assertEqual(str(DumbStrEnum.seven), "don't do this")
        #
        class EnumMixin(Enum):
            def hello(self):
                print('hello from %s' % (self, ))
        class HelloEnum(EnumMixin, CustomStrEnum):
            eight = '8'
        self.assertEqual(HelloEnum.eight, '8')
        self.assertEqual(str(HelloEnum.eight), 'eight')
        #
        class GoodbyeMixin:
            def goodbye(self):
                print('%s wishes you a fond farewell')
        class GoodbyeEnum(GoodbyeMixin, EnumMixin, CustomStrEnum):
            nine = '9'
        self.assertEqual(GoodbyeEnum.nine, '9')
        self.assertEqual(str(GoodbyeEnum.nine), 'nine')
        #
        class FirstFailedStrEnum(CustomStrEnum):
            one = 1   # this will become '1'
            two = '2'
        class SecondFailedStrEnum(CustomStrEnum):
            one = '1'
            two = 2,  # this will become '2'
            three = '3'
        class ThirdFailedStrEnum(CustomStrEnum):
            one = '1'
            two = 2  # this will become '2'
        with self.assertRaisesRegex(TypeError, '.encoding. must be str, not '):
            class ThirdFailedStrEnum(CustomStrEnum):
                one = '1'
                two = b'2', sys.getdefaultencoding
        with self.assertRaisesRegex(TypeError, '.errors. must be str, not '):
            class ThirdFailedStrEnum(CustomStrEnum):
                one = '1'
                two = b'2', 'ascii', 9

    @unittest.skipIf(
            python_version < (3, 12),
            'mixin-format currently uses member.value',
            )
    def test_custom_strenum(self):
        class CustomStrEnum(str, Enum):
            pass
        class OkayEnum(CustomStrEnum):
            one = '1'
            two = '2'
            three = b'3', 'ascii'
            four = b'4', 'latin1', 'strict'
        self.assertEqual(OkayEnum.one, '1')
        self.assertEqual(str(OkayEnum.one), 'one')
        self.assertEqual('{}'.format(OkayEnum.one), 'one')
        self.assertEqual(repr(OkayEnum.one), 'OkayEnum.one')
        #
        class DumbMixin:
            def __str__(self):
                return "don't do this"
        class DumbStrEnum(DumbMixin, CustomStrEnum):
            five = '5'
            six = '6'
            seven = '7'
        self.assertEqual(DumbStrEnum.seven, '7')
        self.assertEqual(str(DumbStrEnum.seven), "don't do this")
        #
        class EnumMixin(Enum):
            def hello(self):
                print('hello from %s' % (self, ))
        class HelloEnum(EnumMixin, CustomStrEnum):
            eight = '8'
        self.assertEqual(HelloEnum.eight, '8')
        self.assertEqual(str(HelloEnum.eight), 'eight')
        #
        class GoodbyeMixin:
            def goodbye(self):
                print('%s wishes you a fond farewell')
        class GoodbyeEnum(GoodbyeMixin, EnumMixin, CustomStrEnum):
            nine = '9'
        self.assertEqual(GoodbyeEnum.nine, '9')
        self.assertEqual(str(GoodbyeEnum.nine), 'nine')
        #
        class FirstFailedStrEnum(CustomStrEnum):
            one = 1   # this will become '1'
            two = '2'
        class SecondFailedStrEnum(CustomStrEnum):
            one = '1'
            two = 2,  # this will become '2'
            three = '3'
        class ThirdFailedStrEnum(CustomStrEnum):
            one = '1'
            two = 2  # this will become '2'
        with self.assertRaisesRegex(TypeError, '.encoding. must be str, not '):
            class ThirdFailedStrEnum(CustomStrEnum):
                one = '1'
                two = b'2', sys.getdefaultencoding
        with self.assertRaisesRegex(TypeError, '.errors. must be str, not '):
            class ThirdFailedStrEnum(CustomStrEnum):
                one = '1'
                two = b'2', 'ascii', 9

    def test_missing_value_error(self):
        with self.assertRaisesRegex(TypeError, "_value_ not set in __new__"):
            class Combined(str, Enum):
                #
                def __new__(cls, value, sequence):
                    enum = str.__new__(cls, value)
                    if '(' in value:
                        fis_name, segment = value.split('(', 1)
                        segment = segment.strip(' )')
                    else:
                        fis_name = value
                        segment = None
                    enum.fis_name = fis_name
                    enum.segment = segment
                    enum.sequence = sequence
                    return enum
                #
                def __repr__(self):
                    return "<%s.%s>" % (self.__class__.__name__, self._name_)
                #
                key_type      = 'An$(1,2)', 0
                company_id    = 'An$(3,2)', 1
                code          = 'An$(5,1)', 2
                description   = 'Bn$',      3

    @unittest.skipUnless(
            python_version == (3, 9),
            'private variables are now normal attributes',
            )
    def test_warning_for_private_variables(self):
        with self.assertWarns(DeprecationWarning):
            class Private(Enum):
                __corporal = 'Radar'
        self.assertEqual(Private._Private__corporal.value, 'Radar')
        try:
            with self.assertWarns(DeprecationWarning):
                class Private(Enum):
                    __major_ = 'Hoolihan'
        except ValueError:
            pass

    def test_private_variable_is_normal_attribute(self):
        class Private(Enum):
            __corporal = 'Radar'
            __major_ = 'Hoolihan'
        self.assertEqual(Private._Private__corporal, 'Radar')
        self.assertEqual(Private._Private__major_, 'Hoolihan')

    @unittest.skipUnless(
            python_version < (3, 12),
            'member-member access now raises an exception',
            )
    def test_warning_for_member_from_member_access(self):
        with self.assertWarns(DeprecationWarning):
            class Di(Enum):
                YES = 1
                NO = 0
            nope = Di.YES.NO
        self.assertIs(Di.NO, nope)

    @unittest.skipUnless(
            python_version >= (3, 12),
            'member-member access currently issues a warning',
            )
    def test_exception_for_member_from_member_access(self):
        with self.assertRaisesRegex(AttributeError, "Di: no instance attribute .NO."):
            class Di(Enum):
                YES = 1
                NO = 0
            nope = Di.YES.NO

    def test_strenum_auto(self):
        class Strings(StrEnum):
            ONE = auto()
            TWO = auto()
        self.assertEqual([Strings.ONE, Strings.TWO], ['one', 'two'])


    def test_dynamic_members_with_static_methods(self):
        #
        foo_defines = {'FOO_CAT': 'aloof', 'BAR_DOG': 'friendly', 'FOO_HORSE': 'big'}
        class Foo(Enum):
            vars().update({
                    k: v
                    for k, v in foo_defines.items()
                    if k.startswith('FOO_')
                    })
            def upper(self):
                return self.value.upper()
        self.assertEqual(list(Foo), [Foo.FOO_CAT, Foo.FOO_HORSE])
        self.assertEqual(Foo.FOO_CAT.value, 'aloof')
        self.assertEqual(Foo.FOO_HORSE.upper(), 'BIG')
        #
        with self.assertRaisesRegex(TypeError, "'FOO_CAT' already defined as: 'aloof'"):
            class FooBar(Enum):
                vars().update({
                        k: v
                        for k, v in foo_defines.items()
                        if k.startswith('FOO_')
                        },
                        **{'FOO_CAT': 'small'},
                        )
                def upper(self):
                    return self.value.upper()


class TestOrder(unittest.TestCase):

    def test_same_members(self):
        class Color(Enum):
            _order_ = 'red green blue'
            red = 1
            green = 2
            blue = 3

    def test_same_members_with_aliases(self):
        class Color(Enum):
            _order_ = 'red green blue'
            red = 1
            green = 2
            blue = 3
            verde = green

    def test_same_members_wrong_order(self):
        with self.assertRaisesRegex(TypeError, 'member order does not match _order_'):
            class Color(Enum):
                _order_ = 'red green blue'
                red = 1
                blue = 3
                green = 2

    def test_order_has_extra_members(self):
        with self.assertRaisesRegex(TypeError, 'member order does not match _order_'):
            class Color(Enum):
                _order_ = 'red green blue purple'
                red = 1
                green = 2
                blue = 3

    def test_order_has_extra_members_with_aliases(self):
        with self.assertRaisesRegex(TypeError, 'member order does not match _order_'):
            class Color(Enum):
                _order_ = 'red green blue purple'
                red = 1
                green = 2
                blue = 3
                verde = green

    def test_enum_has_extra_members(self):
        with self.assertRaisesRegex(TypeError, 'member order does not match _order_'):
            class Color(Enum):
                _order_ = 'red green blue'
                red = 1
                green = 2
                blue = 3
                purple = 4

    def test_enum_has_extra_members_with_aliases(self):
        with self.assertRaisesRegex(TypeError, 'member order does not match _order_'):
            class Color(Enum):
                _order_ = 'red green blue'
                red = 1
                green = 2
                blue = 3
                purple = 4
                verde = green


class TestFlag(unittest.TestCase):
    """Tests of the Flags."""

    class Perm(Flag):
        R, W, X = 4, 2, 1

    class Open(Flag):
        RO = 0
        WO = 1
        RW = 2
        AC = 3
        CE = 1<<19

    class Color(Flag):
        BLACK = 0
        RED = 1
        ROJO = 1
        GREEN = 2
        BLUE = 4
        PURPLE = RED|BLUE
        WHITE = RED|GREEN|BLUE
        BLANCO = RED|GREEN|BLUE

    def test_str(self):
        Perm = self.Perm
        self.assertEqual(str(Perm.R), 'R')
        self.assertEqual(str(Perm.W), 'W')
        self.assertEqual(str(Perm.X), 'X')
        self.assertEqual(str(Perm.R | Perm.W), 'R|W')
        self.assertEqual(str(Perm.R | Perm.W | Perm.X), 'R|W|X')
        self.assertEqual(str(Perm(0)), 'Perm(0)')
        self.assertEqual(str(~Perm.R), 'W|X')
        self.assertEqual(str(~Perm.W), 'R|X')
        self.assertEqual(str(~Perm.X), 'R|W')
        self.assertEqual(str(~(Perm.R | Perm.W)), 'X')
        self.assertEqual(str(~(Perm.R | Perm.W | Perm.X)), 'Perm(0)')
        self.assertEqual(str(Perm(~0)), 'R|W|X')

        Open = self.Open
        self.assertEqual(str(Open.RO), 'RO')
        self.assertEqual(str(Open.WO), 'WO')
        self.assertEqual(str(Open.AC), 'AC')
        self.assertEqual(str(Open.RO | Open.CE), 'CE')
        self.assertEqual(str(Open.WO | Open.CE), 'WO|CE')
        self.assertEqual(str(~Open.RO), 'WO|RW|CE')
        self.assertEqual(str(~Open.WO), 'RW|CE')
        self.assertEqual(str(~Open.AC), 'CE')
        self.assertEqual(str(~(Open.RO | Open.CE)), 'AC')
        self.assertEqual(str(~(Open.WO | Open.CE)), 'RW')

    def test_repr(self):
        Perm = self.Perm
        self.assertEqual(repr(Perm.R), 'Perm.R')
        self.assertEqual(repr(Perm.W), 'Perm.W')
        self.assertEqual(repr(Perm.X), 'Perm.X')
        self.assertEqual(repr(Perm.R | Perm.W), 'Perm.R|Perm.W')
        self.assertEqual(repr(Perm.R | Perm.W | Perm.X), 'Perm.R|Perm.W|Perm.X')
        self.assertEqual(repr(Perm(0)), '0x0')
        self.assertEqual(repr(~Perm.R), 'Perm.W|Perm.X')
        self.assertEqual(repr(~Perm.W), 'Perm.R|Perm.X')
        self.assertEqual(repr(~Perm.X), 'Perm.R|Perm.W')
        self.assertEqual(repr(~(Perm.R | Perm.W)), 'Perm.X')
        self.assertEqual(repr(~(Perm.R | Perm.W | Perm.X)), '0x0')
        self.assertEqual(repr(Perm(~0)), 'Perm.R|Perm.W|Perm.X')

        Open = self.Open
        self.assertEqual(repr(Open.RO), 'Open.RO')
        self.assertEqual(repr(Open.WO), 'Open.WO')
        self.assertEqual(repr(Open.AC), 'Open.AC')
        self.assertEqual(repr(Open.RO | Open.CE), 'Open.CE')
        self.assertEqual(repr(Open.WO | Open.CE), 'Open.WO|Open.CE')
        self.assertEqual(repr(~Open.RO), 'Open.WO|Open.RW|Open.CE')
        self.assertEqual(repr(~Open.WO), 'Open.RW|Open.CE')
        self.assertEqual(repr(~Open.AC), 'Open.CE')
        self.assertEqual(repr(~(Open.RO | Open.CE)), 'Open.AC')
        self.assertEqual(repr(~(Open.WO | Open.CE)), 'Open.RW')

    def test_format(self):
        Perm = self.Perm
        self.assertEqual(format(Perm.R, ''), 'R')
        self.assertEqual(format(Perm.R | Perm.X, ''), 'R|X')

    def test_or(self):
        Perm = self.Perm
        for i in Perm:
            for j in Perm:
                self.assertEqual((i | j), Perm(i.value | j.value))
                self.assertEqual((i | j).value, i.value | j.value)
                self.assertIs(type(i | j), Perm)
        for i in Perm:
            self.assertIs(i | i, i)
        Open = self.Open
        self.assertIs(Open.RO | Open.CE, Open.CE)

    def test_and(self):
        Perm = self.Perm
        RW = Perm.R | Perm.W
        RX = Perm.R | Perm.X
        WX = Perm.W | Perm.X
        RWX = Perm.R | Perm.W | Perm.X
        values = list(Perm) + [RW, RX, WX, RWX, Perm(0)]
        for i in values:
            for j in values:
                self.assertEqual((i & j).value, i.value & j.value)
                self.assertIs(type(i & j), Perm)
        for i in Perm:
            self.assertIs(i & i, i)
            self.assertIs(i & RWX, i)
            self.assertIs(RWX & i, i)
        Open = self.Open
        self.assertIs(Open.RO & Open.CE, Open.RO)

    def test_xor(self):
        Perm = self.Perm
        for i in Perm:
            for j in Perm:
                self.assertEqual((i ^ j).value, i.value ^ j.value)
                self.assertIs(type(i ^ j), Perm)
        for i in Perm:
            self.assertIs(i ^ Perm(0), i)
            self.assertIs(Perm(0) ^ i, i)
        Open = self.Open
        self.assertIs(Open.RO ^ Open.CE, Open.CE)
        self.assertIs(Open.CE ^ Open.CE, Open.RO)

    def test_invert(self):
        Perm = self.Perm
        RW = Perm.R | Perm.W
        RX = Perm.R | Perm.X
        WX = Perm.W | Perm.X
        RWX = Perm.R | Perm.W | Perm.X
        values = list(Perm) + [RW, RX, WX, RWX, Perm(0)]
        for i in values:
            self.assertIs(type(~i), Perm)
            self.assertEqual(~~i, i)
        for i in Perm:
            self.assertIs(~~i, i)
        Open = self.Open
        self.assertIs(Open.WO & ~Open.WO, Open.RO)
        self.assertIs((Open.WO|Open.CE) & ~Open.WO, Open.CE)

    def test_bool(self):
        Perm = self.Perm
        for f in Perm:
            self.assertTrue(f)
        Open = self.Open
        for f in Open:
            self.assertEqual(bool(f.value), bool(f))

    def test_boundary(self):
        self.assertIs(enum.Flag._boundary_, STRICT)
        class Iron(Flag, boundary=STRICT):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Iron._boundary_, STRICT)
        #
        class Water(Flag, boundary=CONFORM):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Water._boundary_, CONFORM)
        #
        class Space(Flag, boundary=EJECT):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Space._boundary_, EJECT)
        #
        class Bizarre(Flag, boundary=KEEP):
            b = 3
            c = 4
            d = 6
        #
        self.assertRaisesRegex(ValueError, 'invalid value: 7', Iron, 7)
        #
        self.assertIs(Water(7), Water.ONE|Water.TWO)
        self.assertIs(Water(~9), Water.TWO)
        #
        self.assertEqual(Space(7), 7)
        self.assertTrue(type(Space(7)) is int)
        #
        self.assertEqual(list(Bizarre), [Bizarre.c])
        self.assertIs(Bizarre(3), Bizarre.b)
        self.assertIs(Bizarre(6), Bizarre.d)

    def test_iter(self):
        Color = self.Color
        Open = self.Open
        self.assertEqual(list(Color), [Color.RED, Color.GREEN, Color.BLUE])
        self.assertEqual(list(Open), [Open.WO, Open.RW, Open.CE])

    def test_programatic_function_string(self):
        Perm = Flag('Perm', 'R W X')
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_string_with_start(self):
        Perm = Flag('Perm', 'R W X', start=8)
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 8<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_string_list(self):
        Perm = Flag('Perm', ['R', 'W', 'X'])
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_iterable(self):
        Perm = Flag('Perm', (('R', 2), ('W', 8), ('X', 32)))
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<(2*i+1)
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_from_dict(self):
        Perm = Flag('Perm', OrderedDict((('R', 2), ('W', 8), ('X', 32))))
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<(2*i+1)
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_pickle(self):
        if isinstance(FlagStooges, Exception):
            raise FlagStooges
        test_pickle_dump_load(self.assertIs, FlagStooges.CURLY|FlagStooges.MOE)
        test_pickle_dump_load(self.assertIs, FlagStooges)

    @unittest.skipIf(
            python_version >= (3, 12),
            '__contains__ now returns True/False for all inputs',
            )
    def test_contains_er(self):
        Open = self.Open
        Color = self.Color
        self.assertFalse(Color.BLACK in Open)
        self.assertFalse(Open.RO in Color)
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                'BLACK' in Color
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                'RO' in Open
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                1 in Color
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                1 in Open

    @unittest.skipIf(
            python_version < (3, 12),
            '__contains__ only works with enum memmbers before 3.12',
            )
    def test_contains_tf(self):
        Open = self.Open
        Color = self.Color
        self.assertFalse(Color.BLACK in Open)
        self.assertFalse(Open.RO in Color)
        self.assertFalse('BLACK' in Color)
        self.assertFalse('RO' in Open)
        self.assertTrue(1 in Color)
        self.assertTrue(1 in Open)

    def test_member_contains(self):
        Perm = self.Perm
        R, W, X = Perm
        RW = R | W
        RX = R | X
        WX = W | X
        RWX = R | W | X
        self.assertTrue(R in RW)
        self.assertTrue(R in RX)
        self.assertTrue(R in RWX)
        self.assertTrue(W in RW)
        self.assertTrue(W in WX)
        self.assertTrue(W in RWX)
        self.assertTrue(X in RX)
        self.assertTrue(X in WX)
        self.assertTrue(X in RWX)
        self.assertFalse(R in WX)
        self.assertFalse(W in RX)
        self.assertFalse(X in RW)

    def test_member_iter(self):
        Color = self.Color
        self.assertEqual(list(Color.BLACK), [])
        self.assertEqual(list(Color.PURPLE), [Color.RED, Color.BLUE])
        self.assertEqual(list(Color.BLUE), [Color.BLUE])
        self.assertEqual(list(Color.GREEN), [Color.GREEN])
        self.assertEqual(list(Color.WHITE), [Color.RED, Color.GREEN, Color.BLUE])
        self.assertEqual(list(Color.WHITE), [Color.RED, Color.GREEN, Color.BLUE])

    def test_member_length(self):
        self.assertEqual(self.Color.__len__(self.Color.BLACK), 0)
        self.assertEqual(self.Color.__len__(self.Color.GREEN), 1)
        self.assertEqual(self.Color.__len__(self.Color.PURPLE), 2)
        self.assertEqual(self.Color.__len__(self.Color.BLANCO), 3)

    def test_number_reset_and_order_cleanup(self):
        class Confused(Flag):
            _order_ = 'ONE TWO FOUR DOS EIGHT SIXTEEN'
            ONE = auto()
            TWO = auto()
            FOUR = auto()
            DOS = 2
            EIGHT = auto()
            SIXTEEN = auto()
        self.assertEqual(
                list(Confused),
                [Confused.ONE, Confused.TWO, Confused.FOUR, Confused.EIGHT, Confused.SIXTEEN])
        self.assertIs(Confused.TWO, Confused.DOS)
        self.assertEqual(Confused.DOS._value_, 2)
        self.assertEqual(Confused.EIGHT._value_, 8)
        self.assertEqual(Confused.SIXTEEN._value_, 16)

    def test_aliases(self):
        Color = self.Color
        self.assertEqual(Color(1).name, 'RED')
        self.assertEqual(Color['ROJO'].name, 'RED')
        self.assertEqual(Color(7).name, 'WHITE')
        self.assertEqual(Color['BLANCO'].name, 'WHITE')
        self.assertIs(Color.BLANCO, Color.WHITE)
        Open = self.Open
        self.assertIs(Open['AC'], Open.AC)

    def test_auto_number(self):
        class Color(Flag):
            red = auto()
            blue = auto()
            green = auto()

        self.assertEqual(list(Color), [Color.red, Color.blue, Color.green])
        self.assertEqual(Color.red.value, 1)
        self.assertEqual(Color.blue.value, 2)
        self.assertEqual(Color.green.value, 4)

    def test_auto_number_garbage(self):
        with self.assertRaisesRegex(TypeError, 'Invalid Flag value: .not an int.'):
            class Color(Flag):
                red = 'not an int'
                blue = auto()

    def test_duplicate_auto(self):
        class Dupes(Enum):
            first = primero = auto()
            second = auto()
            third = auto()
        self.assertEqual([Dupes.first, Dupes.second, Dupes.third], list(Dupes))

    def test_multiple_mixin(self):
        class AllMixin:
            @classproperty
            def ALL(cls):
                members = list(cls)
                all_value = None
                if members:
                    all_value = members[0]
                    for member in members[1:]:
                        all_value |= member
                cls.ALL = all_value
                return all_value
        class StrMixin:
            def __str__(self):
                return self._name_.lower()
        class Color(AllMixin, Flag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'BLUE')
        class Color(AllMixin, StrMixin, Flag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'blue')
        class Color(StrMixin, AllMixin, Flag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'blue')

    @threading_helper.reap_threads
    def test_unique_composite(self):
        # override __eq__ to be identity only
        class TestFlag(Flag):
            one = auto()
            two = auto()
            three = auto()
            four = auto()
            five = auto()
            six = auto()
            seven = auto()
            eight = auto()
            def __eq__(self, other):
                return self is other
            def __hash__(self):
                return hash(self._value_)
        # have multiple threads competing to complete the composite members
        seen = set()
        failed = False
        def cycle_enum():
            nonlocal failed
            try:
                for i in range(256):
                    seen.add(TestFlag(i))
            except Exception:
                failed = True
        threads = [
                threading.Thread(target=cycle_enum)
                for _ in range(8)
                ]
        with threading_helper.start_threads(threads):
            pass
        # check that only 248 members were created
        self.assertFalse(
                failed,
                'at least one thread failed while creating composite members')
        self.assertEqual(256, len(seen), 'too many composite members created')

    def test_init_subclass(self):
        class MyEnum(Flag):
            def __init_subclass__(cls, **kwds):
                super().__init_subclass__(**kwds)
                self.assertFalse(cls.__dict__.get('_test', False))
                cls._test1 = 'MyEnum'
        #
        class TheirEnum(MyEnum):
            def __init_subclass__(cls, **kwds):
                super(TheirEnum, cls).__init_subclass__(**kwds)
                cls._test2 = 'TheirEnum'
        class WhoseEnum(TheirEnum):
            def __init_subclass__(cls, **kwds):
                pass
        class NoEnum(WhoseEnum):
            ONE = 1
        self.assertEqual(TheirEnum.__dict__['_test1'], 'MyEnum')
        self.assertEqual(WhoseEnum.__dict__['_test1'], 'MyEnum')
        self.assertEqual(WhoseEnum.__dict__['_test2'], 'TheirEnum')
        self.assertFalse(NoEnum.__dict__.get('_test1', False))
        self.assertFalse(NoEnum.__dict__.get('_test2', False))
        #
        class OurEnum(MyEnum):
            def __init_subclass__(cls, **kwds):
                cls._test2 = 'OurEnum'
        class WhereEnum(OurEnum):
            def __init_subclass__(cls, **kwds):
                pass
        class NeverEnum(WhereEnum):
            ONE = 1
        self.assertEqual(OurEnum.__dict__['_test1'], 'MyEnum')
        self.assertFalse(WhereEnum.__dict__.get('_test1', False))
        self.assertEqual(WhereEnum.__dict__['_test2'], 'OurEnum')
        self.assertFalse(NeverEnum.__dict__.get('_test1', False))
        self.assertFalse(NeverEnum.__dict__.get('_test2', False))


class TestIntFlag(unittest.TestCase):
    """Tests of the IntFlags."""

    class Perm(IntFlag):
        R = 1 << 2
        W = 1 << 1
        X = 1 << 0

    class Open(IntFlag):
        RO = 0
        WO = 1
        RW = 2
        AC = 3
        CE = 1<<19

    class Color(IntFlag):
        BLACK = 0
        RED = 1
        ROJO = 1
        GREEN = 2
        BLUE = 4
        PURPLE = RED|BLUE
        WHITE = RED|GREEN|BLUE
        BLANCO = RED|GREEN|BLUE

    class Skip(IntFlag):
        FIRST = 1
        SECOND = 2
        EIGHTH = 8

    def test_type(self):
        Perm = self.Perm
        self.assertTrue(Perm._member_type_ is int)
        Open = self.Open
        for f in Perm:
            self.assertTrue(isinstance(f, Perm))
            self.assertEqual(f, f.value)
        self.assertTrue(isinstance(Perm.W | Perm.X, Perm))
        self.assertEqual(Perm.W | Perm.X, 3)
        for f in Open:
            self.assertTrue(isinstance(f, Open))
            self.assertEqual(f, f.value)
        self.assertTrue(isinstance(Open.WO | Open.RW, Open))
        self.assertEqual(Open.WO | Open.RW, 3)


    def test_str(self):
        Perm = self.Perm
        self.assertEqual(str(Perm.R), 'R')
        self.assertEqual(str(Perm.W), 'W')
        self.assertEqual(str(Perm.X), 'X')
        self.assertEqual(str(Perm.R | Perm.W), 'R|W')
        self.assertEqual(str(Perm.R | Perm.W | Perm.X), 'R|W|X')
        self.assertEqual(str(Perm.R | 8), '12')
        self.assertEqual(str(Perm(0)), 'Perm(0)')
        self.assertEqual(str(Perm(8)), '8')
        self.assertEqual(str(~Perm.R), 'W|X')
        self.assertEqual(str(~Perm.W), 'R|X')
        self.assertEqual(str(~Perm.X), 'R|W')
        self.assertEqual(str(~(Perm.R | Perm.W)), 'X')
        self.assertEqual(str(~(Perm.R | Perm.W | Perm.X)), 'Perm(0)')
        self.assertEqual(str(~(Perm.R | 8)), '-13')
        self.assertEqual(str(Perm(~0)), 'R|W|X')
        self.assertEqual(str(Perm(~8)), '-9')

        Open = self.Open
        self.assertEqual(str(Open.RO), 'RO')
        self.assertEqual(str(Open.WO), 'WO')
        self.assertEqual(str(Open.AC), 'AC')
        self.assertEqual(str(Open.RO | Open.CE), 'CE')
        self.assertEqual(str(Open.WO | Open.CE), 'WO|CE')
        self.assertEqual(str(Open(4)), '4')
        self.assertEqual(str(~Open.RO), 'WO|RW|CE')
        self.assertEqual(str(~Open.WO), 'RW|CE')
        self.assertEqual(str(~Open.AC), 'CE')
        self.assertEqual(str(~(Open.RO | Open.CE)), 'AC')
        self.assertEqual(str(~(Open.WO | Open.CE)), 'RW')
        self.assertEqual(str(Open(~4)), '-5')

    def test_repr(self):
        Perm = self.Perm
        self.assertEqual(repr(Perm.R), 'Perm.R')
        self.assertEqual(repr(Perm.W), 'Perm.W')
        self.assertEqual(repr(Perm.X), 'Perm.X')
        self.assertEqual(repr(Perm.R | Perm.W), 'Perm.R|Perm.W')
        self.assertEqual(repr(Perm.R | Perm.W | Perm.X), 'Perm.R|Perm.W|Perm.X')
        self.assertEqual(repr(Perm.R | 8), '12')
        self.assertEqual(repr(Perm(0)), '0x0')
        self.assertEqual(repr(Perm(8)), '8')
        self.assertEqual(repr(~Perm.R), 'Perm.W|Perm.X')
        self.assertEqual(repr(~Perm.W), 'Perm.R|Perm.X')
        self.assertEqual(repr(~Perm.X), 'Perm.R|Perm.W')
        self.assertEqual(repr(~(Perm.R | Perm.W)), 'Perm.X')
        self.assertEqual(repr(~(Perm.R | Perm.W | Perm.X)), '0x0')
        self.assertEqual(repr(~(Perm.R | 8)), '-13')
        self.assertEqual(repr(Perm(~0)), 'Perm.R|Perm.W|Perm.X')
        self.assertEqual(repr(Perm(~8)), '-9')

        Open = self.Open
        self.assertEqual(repr(Open.RO), 'Open.RO')
        self.assertEqual(repr(Open.WO), 'Open.WO')
        self.assertEqual(repr(Open.AC), 'Open.AC')
        self.assertEqual(repr(Open.RO | Open.CE), 'Open.CE')
        self.assertEqual(repr(Open.WO | Open.CE), 'Open.WO|Open.CE')
        self.assertEqual(repr(Open(4)), '4')
        self.assertEqual(repr(~Open.RO), 'Open.WO|Open.RW|Open.CE')
        self.assertEqual(repr(~Open.WO), 'Open.RW|Open.CE')
        self.assertEqual(repr(~Open.AC), 'Open.CE')
        self.assertEqual(repr(~(Open.RO | Open.CE)), 'Open.AC')
        self.assertEqual(repr(~(Open.WO | Open.CE)), 'Open.RW')
        self.assertEqual(repr(Open(~4)), '-5')

    def test_global_repr_keep(self):
        self.assertEqual(
                repr(HeadlightsK(0)),
                '%s.OFF_K' % SHORT_MODULE,
                )
        self.assertEqual(
                repr(HeadlightsK(2**0 + 2**2 + 2**3)),
                '%(m)s.LOW_BEAM_K|%(m)s.FOG_K|0x8' % {'m': SHORT_MODULE},
                )
        self.assertEqual(
                repr(HeadlightsK(2**3)),
                '%(m)s.HeadlightsK(0x8)' % {'m': SHORT_MODULE},
                )

    def test_global_repr_conform1(self):
        self.assertEqual(
                repr(HeadlightsC(0)),
                '%s.OFF_C' % SHORT_MODULE,
                )
        self.assertEqual(
                repr(HeadlightsC(2**0 + 2**2 + 2**3)),
                '%(m)s.LOW_BEAM_C|%(m)s.FOG_C' % {'m': SHORT_MODULE},
                )
        self.assertEqual(
                repr(HeadlightsC(2**3)),
                '%(m)s.OFF_C' % {'m': SHORT_MODULE},
                )

    def test_format(self):
        Perm = self.Perm
        self.assertEqual(format(Perm.R, ''), '4')
        self.assertEqual(format(Perm.R | Perm.X, ''), '5')
        #
        class NewPerm(IntFlag):
            R = 1 << 2
            W = 1 << 1
            X = 1 << 0
            def __str__(self):
                return self._name_
        self.assertEqual(format(NewPerm.R, ''), 'R')
        self.assertEqual(format(NewPerm.R | Perm.X, ''), 'R|X')

    def test_or(self):
        Perm = self.Perm
        for i in Perm:
            for j in Perm:
                self.assertEqual(i | j, i.value | j.value)
                self.assertEqual((i | j).value, i.value | j.value)
                self.assertIs(type(i | j), Perm)
            for j in range(8):
                self.assertEqual(i | j, i.value | j)
                self.assertEqual((i | j).value, i.value | j)
                self.assertIs(type(i | j), Perm)
                self.assertEqual(j | i, j | i.value)
                self.assertEqual((j | i).value, j | i.value)
                self.assertIs(type(j | i), Perm)
        for i in Perm:
            self.assertIs(i | i, i)
            self.assertIs(i | 0, i)
            self.assertIs(0 | i, i)
        Open = self.Open
        self.assertIs(Open.RO | Open.CE, Open.CE)

    def test_and(self):
        Perm = self.Perm
        RW = Perm.R | Perm.W
        RX = Perm.R | Perm.X
        WX = Perm.W | Perm.X
        RWX = Perm.R | Perm.W | Perm.X
        values = list(Perm) + [RW, RX, WX, RWX, Perm(0)]
        for i in values:
            for j in values:
                self.assertEqual(i & j, i.value & j.value, 'i is %r, j is %r' % (i, j))
                self.assertEqual((i & j).value, i.value & j.value, 'i is %r, j is %r' % (i, j))
                self.assertIs(type(i & j), Perm, 'i is %r, j is %r' % (i, j))
            for j in range(8):
                self.assertEqual(i & j, i.value & j)
                self.assertEqual((i & j).value, i.value & j)
                self.assertIs(type(i & j), Perm)
                self.assertEqual(j & i, j & i.value)
                self.assertEqual((j & i).value, j & i.value)
                self.assertIs(type(j & i), Perm)
        for i in Perm:
            self.assertIs(i & i, i)
            self.assertIs(i & 7, i)
            self.assertIs(7 & i, i)
        Open = self.Open
        self.assertIs(Open.RO & Open.CE, Open.RO)

    def test_xor(self):
        Perm = self.Perm
        for i in Perm:
            for j in Perm:
                self.assertEqual(i ^ j, i.value ^ j.value)
                self.assertEqual((i ^ j).value, i.value ^ j.value)
                self.assertIs(type(i ^ j), Perm)
            for j in range(8):
                self.assertEqual(i ^ j, i.value ^ j)
                self.assertEqual((i ^ j).value, i.value ^ j)
                self.assertIs(type(i ^ j), Perm)
                self.assertEqual(j ^ i, j ^ i.value)
                self.assertEqual((j ^ i).value, j ^ i.value)
                self.assertIs(type(j ^ i), Perm)
        for i in Perm:
            self.assertIs(i ^ 0, i)
            self.assertIs(0 ^ i, i)
        Open = self.Open
        self.assertIs(Open.RO ^ Open.CE, Open.CE)
        self.assertIs(Open.CE ^ Open.CE, Open.RO)

    def test_invert(self):
        Perm = self.Perm
        RW = Perm.R | Perm.W
        RX = Perm.R | Perm.X
        WX = Perm.W | Perm.X
        RWX = Perm.R | Perm.W | Perm.X
        values = list(Perm) + [RW, RX, WX, RWX, Perm(0)]
        for i in values:
            self.assertEqual(~i, (~i).value)
            self.assertIs(type(~i), Perm)
            self.assertEqual(~~i, i)
        for i in Perm:
            self.assertIs(~~i, i)
        Open = self.Open
        self.assertIs(Open.WO & ~Open.WO, Open.RO)
        self.assertIs((Open.WO|Open.CE) & ~Open.WO, Open.CE)

    def test_boundary(self):
        self.assertIs(enum.IntFlag._boundary_, EJECT)
        class Iron(IntFlag, boundary=STRICT):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Iron._boundary_, STRICT)
        #
        class Water(IntFlag, boundary=CONFORM):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Water._boundary_, CONFORM)
        #
        class Space(IntFlag, boundary=EJECT):
            ONE = 1
            TWO = 2
            EIGHT = 8
        self.assertIs(Space._boundary_, EJECT)
        #
        #
        class Bizarre(IntFlag, boundary=KEEP):
            b = 3
            c = 4
            d = 6
        #
        self.assertRaisesRegex(ValueError, 'invalid value: 5', Iron, 5)
        #
        self.assertIs(Water(7), Water.ONE|Water.TWO)
        self.assertIs(Water(~9), Water.TWO)
        #
        self.assertEqual(Space(7), 7)
        self.assertTrue(type(Space(7)) is int)
        #
        self.assertEqual(list(Bizarre), [Bizarre.c])
        self.assertIs(Bizarre(3), Bizarre.b)
        self.assertIs(Bizarre(6), Bizarre.d)

    def test_iter(self):
        Color = self.Color
        Open = self.Open
        self.assertEqual(list(Color), [Color.RED, Color.GREEN, Color.BLUE])
        self.assertEqual(list(Open), [Open.WO, Open.RW, Open.CE])

    def test_programatic_function_string(self):
        Perm = IntFlag('Perm', 'R W X')
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e, v)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_string_with_start(self):
        Perm = IntFlag('Perm', 'R W X', start=8)
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 8<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e, v)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_string_list(self):
        Perm = IntFlag('Perm', ['R', 'W', 'X'])
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<i
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e, v)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_iterable(self):
        Perm = IntFlag('Perm', (('R', 2), ('W', 8), ('X', 32)))
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<(2*i+1)
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e, v)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)

    def test_programatic_function_from_dict(self):
        Perm = IntFlag('Perm', OrderedDict((('R', 2), ('W', 8), ('X', 32))))
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 3, Perm)
        self.assertEqual(lst, [Perm.R, Perm.W, Perm.X])
        for i, n in enumerate('R W X'.split()):
            v = 1<<(2*i+1)
            e = Perm(v)
            self.assertEqual(e.value, v)
            self.assertEqual(type(e.value), int)
            self.assertEqual(e, v)
            self.assertEqual(e.name, n)
            self.assertIn(e, Perm)
            self.assertIs(type(e), Perm)


    def test_programatic_function_from_empty_list(self):
        Perm = enum.IntFlag('Perm', [])
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 0, Perm)
        Thing = enum.Enum('Thing', [])
        lst = list(Thing)
        self.assertEqual(len(lst), len(Thing))
        self.assertEqual(len(Thing), 0, Thing)


    def test_programatic_function_from_empty_tuple(self):
        Perm = enum.IntFlag('Perm', ())
        lst = list(Perm)
        self.assertEqual(len(lst), len(Perm))
        self.assertEqual(len(Perm), 0, Perm)
        Thing = enum.Enum('Thing', ())
        self.assertEqual(len(lst), len(Thing))
        self.assertEqual(len(Thing), 0, Thing)

    @unittest.skipIf(
            python_version >= (3, 12),
            '__contains__ now returns True/False for all inputs',
            )
    def test_contains_er(self):
        Open = self.Open
        Color = self.Color
        self.assertTrue(Color.GREEN in Color)
        self.assertTrue(Open.RW in Open)
        self.assertFalse(Color.GREEN in Open)
        self.assertFalse(Open.RW in Color)
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                'GREEN' in Color
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                'RW' in Open
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                2 in Color
        with self.assertRaises(TypeError):
            with self.assertWarns(DeprecationWarning):
                2 in Open

    @unittest.skipIf(
            python_version < (3, 12),
            '__contains__ only works with enum memmbers before 3.12',
            )
    def test_contains_tf(self):
        Open = self.Open
        Color = self.Color
        self.assertTrue(Color.GREEN in Color)
        self.assertTrue(Open.RW in Open)
        self.assertTrue(Color.GREEN in Open)
        self.assertTrue(Open.RW in Color)
        self.assertFalse('GREEN' in Color)
        self.assertFalse('RW' in Open)
        self.assertTrue(2 in Color)
        self.assertTrue(2 in Open)

    def test_member_contains(self):
        Perm = self.Perm
        R, W, X = Perm
        RW = R | W
        RX = R | X
        WX = W | X
        RWX = R | W | X
        self.assertTrue(R in RW)
        self.assertTrue(R in RX)
        self.assertTrue(R in RWX)
        self.assertTrue(W in RW)
        self.assertTrue(W in WX)
        self.assertTrue(W in RWX)
        self.assertTrue(X in RX)
        self.assertTrue(X in WX)
        self.assertTrue(X in RWX)
        self.assertFalse(R in WX)
        self.assertFalse(W in RX)
        self.assertFalse(X in RW)
        with self.assertRaises(TypeError):
            self.assertFalse('test' in RW)

    def test_member_iter(self):
        Color = self.Color
        self.assertEqual(list(Color.BLACK), [])
        self.assertEqual(list(Color.PURPLE), [Color.RED, Color.BLUE])
        self.assertEqual(list(Color.BLUE), [Color.BLUE])
        self.assertEqual(list(Color.GREEN), [Color.GREEN])
        self.assertEqual(list(Color.WHITE), [Color.RED, Color.GREEN, Color.BLUE])

    def test_member_length(self):
        self.assertEqual(self.Color.__len__(self.Color.BLACK), 0)
        self.assertEqual(self.Color.__len__(self.Color.GREEN), 1)
        self.assertEqual(self.Color.__len__(self.Color.PURPLE), 2)
        self.assertEqual(self.Color.__len__(self.Color.BLANCO), 3)

    def test_aliases(self):
        Color = self.Color
        self.assertEqual(Color(1).name, 'RED')
        self.assertEqual(Color['ROJO'].name, 'RED')
        self.assertEqual(Color(7).name, 'WHITE')
        self.assertEqual(Color['BLANCO'].name, 'WHITE')
        self.assertIs(Color.BLANCO, Color.WHITE)
        Open = self.Open
        self.assertIs(Open['AC'], Open.AC)

    def test_bool(self):
        Perm = self.Perm
        for f in Perm:
            self.assertTrue(f)
        Open = self.Open
        for f in Open:
            self.assertEqual(bool(f.value), bool(f))


    def test_multiple_mixin(self):
        class AllMixin:
            @classproperty
            def ALL(cls):
                members = list(cls)
                all_value = None
                if members:
                    all_value = members[0]
                    for member in members[1:]:
                        all_value |= member
                cls.ALL = all_value
                return all_value
        class StrMixin:
            def __str__(self):
                return self._name_.lower()
        class Color(AllMixin, IntFlag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'BLUE')
        class Color(AllMixin, StrMixin, IntFlag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'blue')
        class Color(StrMixin, AllMixin, IntFlag):
            RED = auto()
            GREEN = auto()
            BLUE = auto()
        self.assertEqual(Color.RED.value, 1)
        self.assertEqual(Color.GREEN.value, 2)
        self.assertEqual(Color.BLUE.value, 4)
        self.assertEqual(Color.ALL.value, 7)
        self.assertEqual(str(Color.BLUE), 'blue')

    @threading_helper.reap_threads
    def test_unique_composite(self):
        # override __eq__ to be identity only
        class TestFlag(IntFlag):
            one = auto()
            two = auto()
            three = auto()
            four = auto()
            five = auto()
            six = auto()
            seven = auto()
            eight = auto()
            def __eq__(self, other):
                return self is other
            def __hash__(self):
                return hash(self._value_)
        # have multiple threads competing to complete the composite members
        seen = set()
        failed = False
        def cycle_enum():
            nonlocal failed
            try:
                for i in range(256):
                    seen.add(TestFlag(i))
            except Exception:
                failed = True
        threads = [
                threading.Thread(target=cycle_enum)
                for _ in range(8)
                ]
        with threading_helper.start_threads(threads):
            pass
        # check that only 248 members were created
        self.assertFalse(
                failed,
                'at least one thread failed while creating composite members')
        self.assertEqual(256, len(seen), 'too many composite members created')


class TestEmptyAndNonLatinStrings(unittest.TestCase):

    def test_empty_string(self):
        with self.assertRaises(ValueError):
            empty_abc = Enum('empty_abc', ('', 'B', 'C'))

    def test_non_latin_character_string(self):
        greek_abc = Enum('greek_abc', ('\u03B1', 'B', 'C'))
        item = getattr(greek_abc, '\u03B1')
        self.assertEqual(item.value, 1)

    def test_non_latin_number_string(self):
        hebrew_123 = Enum('hebrew_123', ('\u05D0', '2', '3'))
        item = getattr(hebrew_123, '\u05D0')
        self.assertEqual(item.value, 1)


class TestUnique(unittest.TestCase):

    def test_unique_clean(self):
        @unique
        class Clean(Enum):
            one = 1
            two = 'dos'
            tres = 4.0
        #
        @unique
        class Cleaner(IntEnum):
            single = 1
            double = 2
            triple = 3

    def test_unique_dirty(self):
        with self.assertRaisesRegex(ValueError, 'tres.*one'):
            @unique
            class Dirty(Enum):
                one = 1
                two = 'dos'
                tres = 1
        with self.assertRaisesRegex(
                ValueError,
                'double.*single.*turkey.*triple',
                ):
            @unique
            class Dirtier(IntEnum):
                single = 1
                double = 1
                triple = 3
                turkey = 3

    def test_unique_with_name(self):
        @verify(UNIQUE)
        class Silly(Enum):
            one = 1
            two = 'dos'
            name = 3
        #
        @verify(UNIQUE)
        class Sillier(IntEnum):
            single = 1
            name = 2
            triple = 3
            value = 4

class TestVerify(unittest.TestCase):

    def test_continuous(self):
        @verify(CONTINUOUS)
        class Auto(Enum):
            FIRST = auto()
            SECOND = auto()
            THIRD = auto()
            FORTH = auto()
        #
        @verify(CONTINUOUS)
        class Manual(Enum):
            FIRST = 3
            SECOND = 4
            THIRD = 5
            FORTH = 6
        #
        with self.assertRaisesRegex(ValueError, 'invalid enum .Missing.: missing values 5, 6, 7, 8, 9, 10, 12'):
            @verify(CONTINUOUS)
            class Missing(Enum):
                FIRST = 3
                SECOND = 4
                THIRD = 11
                FORTH = 13
        #
        with self.assertRaisesRegex(ValueError, 'invalid flag .Incomplete.: missing values 32'):
            @verify(CONTINUOUS)
            class Incomplete(Flag):
                FIRST = 4
                SECOND = 8
                THIRD = 16
                FORTH = 64
        #
        with self.assertRaisesRegex(ValueError, 'invalid flag .StillIncomplete.: missing values 16'):
            @verify(CONTINUOUS)
            class StillIncomplete(Flag):
                FIRST = 4
                SECOND = 8
                THIRD = 11
                FORTH = 32


    def test_composite(self):
        class Bizarre(Flag):
            b = 3
            c = 4
            d = 6
        self.assertEqual(list(Bizarre), [Bizarre.c])
        self.assertEqual(Bizarre.b.value, 3)
        self.assertEqual(Bizarre.c.value, 4)
        self.assertEqual(Bizarre.d.value, 6)
        with self.assertRaisesRegex(
                ValueError,
                "invalid Flag 'Bizarre': aliases b and d are missing combined values of 0x3 .use enum.show_flag_values.value. for details.",
            ):
            @verify(NAMED_FLAGS)
            class Bizarre(Flag):
                b = 3
                c = 4
                d = 6
        #
        self.assertEqual(enum.show_flag_values(3), [1, 2])
        class Bizarre(IntFlag):
            b = 3
            c = 4
            d = 6
        self.assertEqual(list(Bizarre), [Bizarre.c])
        self.assertEqual(Bizarre.b.value, 3)
        self.assertEqual(Bizarre.c.value, 4)
        self.assertEqual(Bizarre.d.value, 6)
        with self.assertRaisesRegex(
                ValueError,
                "invalid Flag 'Bizarre': alias d is missing value 0x2 .use enum.show_flag_values.value. for details.",
            ):
            @verify(NAMED_FLAGS)
            class Bizarre(IntFlag):
                c = 4
                d = 6
        self.assertEqual(enum.show_flag_values(2), [2])

    def test_unique_clean(self):
        @verify(UNIQUE)
        class Clean(Enum):
            one = 1
            two = 'dos'
            tres = 4.0
        #
        @verify(UNIQUE)
        class Cleaner(IntEnum):
            single = 1
            double = 2
            triple = 3

    def test_unique_dirty(self):
        with self.assertRaisesRegex(ValueError, 'tres.*one'):
            @verify(UNIQUE)
            class Dirty(Enum):
                one = 1
                two = 'dos'
                tres = 1
        with self.assertRaisesRegex(
                ValueError,
                'double.*single.*turkey.*triple',
                ):
            @verify(UNIQUE)
            class Dirtier(IntEnum):
                single = 1
                double = 1
                triple = 3
                turkey = 3

    def test_unique_with_name(self):
        @verify(UNIQUE)
        class Silly(Enum):
            one = 1
            two = 'dos'
            name = 3
        #
        @verify(UNIQUE)
        class Sillier(IntEnum):
            single = 1
            name = 2
            triple = 3
            value = 4

class TestHelpers(unittest.TestCase):

    sunder_names = '_bad_', '_good_', '_what_ho_'
    dunder_names = '__mal__', '__bien__', '__que_que__'
    private_names = '_MyEnum__private', '_MyEnum__still_private'
    private_and_sunder_names = '_MyEnum__private_', '_MyEnum__also_private_'
    random_names = 'okay', '_semi_private', '_weird__', '_MyEnum__'

    def test_sunder(self):
        for name in self.sunder_names + self.private_and_sunder_names:
            self.assertTrue(enum._is_sunder(name), '%r is a not sunder name?' % name)
        for name in self.dunder_names + self.private_names + self.random_names:
            self.assertFalse(enum._is_sunder(name), '%r is a sunder name?' % name)

    def test_dunder(self):
        for name in self.dunder_names:
            self.assertTrue(enum._is_dunder(name), '%r is a not dunder name?' % name)
        for name in self.sunder_names + self.private_names + self.private_and_sunder_names + self.random_names:
            self.assertFalse(enum._is_dunder(name), '%r is a dunder name?' % name)

    def test_is_private(self):
        for name in self.private_names + self.private_and_sunder_names:
            self.assertTrue(enum._is_private('MyEnum', name), '%r is a not private name?')
        for name in self.sunder_names + self.dunder_names + self.random_names:
            self.assertFalse(enum._is_private('MyEnum', name), '%r is a private name?')

class TestEnumTypeSubclassing(unittest.TestCase):
    pass

expected_help_output_with_docs = """\
Help on class Color in module %s:

class Color(enum.Enum)
 |  Color(value, names=None, *, module=None, qualname=None, type=None, start=1, boundary=None)
 |\x20\x20
 |  An enumeration.
 |\x20\x20
 |  Method resolution order:
 |      Color
 |      enum.Enum
 |      builtins.object
 |\x20\x20
 |  Data and other attributes defined here:
 |\x20\x20
 |  blue = Color.blue
 |\x20\x20
 |  green = Color.green
 |\x20\x20
 |  red = Color.red
 |\x20\x20
 |  ----------------------------------------------------------------------
 |  Data descriptors inherited from enum.Enum:
 |\x20\x20
 |  name
 |      The name of the Enum member.
 |\x20\x20
 |  value
 |      The value of the Enum member.
 |\x20\x20
 |  ----------------------------------------------------------------------
 |  Readonly properties inherited from enum.EnumType:
 |\x20\x20
 |  __members__
 |      Returns a mapping of member name->value.
 |\x20\x20\x20\x20\x20\x20
 |      This mapping lists all enum members, including aliases. Note that this
 |      is a read-only view of the internal mapping."""

expected_help_output_without_docs = """\
Help on class Color in module %s:

class Color(enum.Enum)
 |  Color(value, names=None, *, module=None, qualname=None, type=None, start=1)
 |\x20\x20
 |  Method resolution order:
 |      Color
 |      enum.Enum
 |      builtins.object
 |\x20\x20
 |  Data and other attributes defined here:
 |\x20\x20
 |  blue = Color.blue
 |\x20\x20
 |  green = Color.green
 |\x20\x20
 |  red = Color.red
 |\x20\x20
 |  ----------------------------------------------------------------------
 |  Data descriptors inherited from enum.Enum:
 |\x20\x20
 |  name
 |\x20\x20
 |  value
 |\x20\x20
 |  ----------------------------------------------------------------------
 |  Data descriptors inherited from enum.EnumType:
 |\x20\x20
 |  __members__"""

class TestStdLib(unittest.TestCase):

    maxDiff = None

    class Color(Enum):
        red = 1
        green = 2
        blue = 3

    def test_pydoc(self):
        # indirectly test __objclass__
        if StrEnum.__doc__ is None:
            expected_text = expected_help_output_without_docs % __name__
        else:
            expected_text = expected_help_output_with_docs % __name__
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(self.Color)
        result = output.getvalue().strip()
        self.assertEqual(result, expected_text)

    def test_inspect_getmembers(self):
        values = dict((
                ('__class__', EnumType),
                ('__doc__', 'An enumeration.'),
                ('__members__', self.Color.__members__),
                ('__module__', __name__),
                ('blue', self.Color.blue),
                ('green', self.Color.green),
                ('name', Enum.__dict__['name']),
                ('red', self.Color.red),
                ('value', Enum.__dict__['value']),
                ))
        result = dict(inspect.getmembers(self.Color))
        self.assertEqual(set(values.keys()), set(result.keys()))
        failed = False
        for k in values.keys():
            if result[k] != values[k]:
                print()
                print('\n%s\n     key: %s\n  result: %s\nexpected: %s\n%s\n' %
                        ('=' * 75, k, result[k], values[k], '=' * 75), sep='')
                failed = True
        if failed:
            self.fail("result does not equal expected, see print above")

    def test_inspect_classify_class_attrs(self):
        # indirectly test __objclass__
        from inspect import Attribute
        values = [
                Attribute(name='__class__', kind='data',
                    defining_class=object, object=EnumType),
                Attribute(name='__doc__', kind='data',
                    defining_class=self.Color, object='An enumeration.'),
                Attribute(name='__members__', kind='property',
                    defining_class=EnumType, object=EnumType.__members__),
                Attribute(name='__module__', kind='data',
                    defining_class=self.Color, object=__name__),
                Attribute(name='blue', kind='data',
                    defining_class=self.Color, object=self.Color.blue),
                Attribute(name='green', kind='data',
                    defining_class=self.Color, object=self.Color.green),
                Attribute(name='red', kind='data',
                    defining_class=self.Color, object=self.Color.red),
                Attribute(name='name', kind='data',
                    defining_class=Enum, object=Enum.__dict__['name']),
                Attribute(name='value', kind='data',
                    defining_class=Enum, object=Enum.__dict__['value']),
                ]
        values.sort(key=lambda item: item.name)
        result = list(inspect.classify_class_attrs(self.Color))
        result.sort(key=lambda item: item.name)
        self.assertEqual(
                len(values), len(result),
                "%s != %s" % ([a.name for a in values], [a.name for a in result])
                )
        failed = False
        for v, r in zip(values, result):
            if r != v:
                print('\n%s\n%s\n%s\n%s\n' % ('=' * 75, r, v, '=' * 75), sep='')
                failed = True
        if failed:
            self.fail("result does not equal expected, see print above")

    def test_test_simple_enum(self):
        @_simple_enum(Enum)
        class SimpleColor:
            RED = 1
            GREEN = 2
            BLUE = 3
        class CheckedColor(Enum):
            RED = 1
            GREEN = 2
            BLUE = 3
        self.assertTrue(_test_simple_enum(CheckedColor, SimpleColor) is None)
        SimpleColor.GREEN._value_ = 9
        self.assertRaisesRegex(
                TypeError, "enum mismatch",
                _test_simple_enum, CheckedColor, SimpleColor,
                )
        class CheckedMissing(IntFlag, boundary=KEEP):
            SIXTY_FOUR = 64
            ONE_TWENTY_EIGHT = 128
            TWENTY_FORTY_EIGHT = 2048
            ALL = 2048 + 128 + 64 + 12
        CM = CheckedMissing
        self.assertEqual(list(CheckedMissing), [CM.SIXTY_FOUR, CM.ONE_TWENTY_EIGHT, CM.TWENTY_FORTY_EIGHT])
        #
        @_simple_enum(IntFlag, boundary=KEEP)
        class Missing:
            SIXTY_FOUR = 64
            ONE_TWENTY_EIGHT = 128
            TWENTY_FORTY_EIGHT = 2048
            ALL = 2048 + 128 + 64 + 12
        M = Missing
        self.assertEqual(list(CheckedMissing), [M.SIXTY_FOUR, M.ONE_TWENTY_EIGHT, M.TWENTY_FORTY_EIGHT])
        #
        _test_simple_enum(CheckedMissing, Missing)


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, enum, not_exported={'bin', 'show_flag_values'})


# These are unordered here on purpose to ensure that declaration order
# makes no difference.
CONVERT_TEST_NAME_D = 5
CONVERT_TEST_NAME_C = 5
CONVERT_TEST_NAME_B = 5
CONVERT_TEST_NAME_A = 5  # This one should sort first.
CONVERT_TEST_NAME_E = 5
CONVERT_TEST_NAME_F = 5

CONVERT_STRING_TEST_NAME_D = 5
CONVERT_STRING_TEST_NAME_C = 5
CONVERT_STRING_TEST_NAME_B = 5
CONVERT_STRING_TEST_NAME_A = 5  # This one should sort first.
CONVERT_STRING_TEST_NAME_E = 5
CONVERT_STRING_TEST_NAME_F = 5

class TestIntEnumConvert(unittest.TestCase):
    def setUp(self):
        # Reset the module-level test variables to their original integer
        # values, otherwise the already created enum values get converted
        # instead.
        for suffix in ['A', 'B', 'C', 'D', 'E', 'F']:
            globals()[f'CONVERT_TEST_NAME_{suffix}'] = 5
            globals()[f'CONVERT_STRING_TEST_NAME_{suffix}'] = 5

    def test_convert_value_lookup_priority(self):
        test_type = enum.IntEnum._convert_(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_TEST_'))
        # We don't want the reverse lookup value to vary when there are
        # multiple possible names for a given value.  It should always
        # report the first lexigraphical name in that case.
        self.assertEqual(test_type(5).name, 'CONVERT_TEST_NAME_A')

    def test_convert(self):
        test_type = enum.IntEnum._convert_(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_TEST_'))
        # Ensure that test_type has all of the desired names and values.
        self.assertEqual(test_type.CONVERT_TEST_NAME_F,
                         test_type.CONVERT_TEST_NAME_A)
        self.assertEqual(test_type.CONVERT_TEST_NAME_B, 5)
        self.assertEqual(test_type.CONVERT_TEST_NAME_C, 5)
        self.assertEqual(test_type.CONVERT_TEST_NAME_D, 5)
        self.assertEqual(test_type.CONVERT_TEST_NAME_E, 5)
        # Ensure that test_type only picked up names matching the filter.
        self.assertEqual([name for name in dir(test_type)
                          if name[0:2] not in ('CO', '__')],
                         [], msg='Names other than CONVERT_TEST_* found.')

    @unittest.skipUnless(python_version == (3, 8),
                         '_convert was deprecated in 3.8')
    def test_convert_warn(self):
        with self.assertWarns(DeprecationWarning):
            enum.IntEnum._convert(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_TEST_'))

    @unittest.skipUnless(python_version >= (3, 9),
                         '_convert was removed in 3.9')
    def test_convert_raise(self):
        with self.assertRaises(AttributeError):
            enum.IntEnum._convert(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_TEST_'))

    def test_convert_repr_and_str(self):
        test_type = enum.IntEnum._convert_(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_STRING_TEST_'))
        self.assertEqual(repr(test_type.CONVERT_STRING_TEST_NAME_A), '%s.CONVERT_STRING_TEST_NAME_A' % SHORT_MODULE)
        self.assertEqual(str(test_type.CONVERT_STRING_TEST_NAME_A), 'CONVERT_STRING_TEST_NAME_A')
        self.assertEqual(format(test_type.CONVERT_STRING_TEST_NAME_A), '5')

# global names for StrEnum._convert_ test
CONVERT_STR_TEST_2 = 'goodbye'
CONVERT_STR_TEST_1 = 'hello'

class TestStrEnumConvert(unittest.TestCase):
    def setUp(self):
        global CONVERT_STR_TEST_1
        global CONVERT_STR_TEST_2
        CONVERT_STR_TEST_2 = 'goodbye'
        CONVERT_STR_TEST_1 = 'hello'

    def test_convert(self):
        test_type = enum.StrEnum._convert_(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_STR_'))
        # Ensure that test_type has all of the desired names and values.
        self.assertEqual(test_type.CONVERT_STR_TEST_1, 'hello')
        self.assertEqual(test_type.CONVERT_STR_TEST_2, 'goodbye')
        # Ensure that test_type only picked up names matching the filter.
        self.assertEqual([name for name in dir(test_type)
                          if name[0:2] not in ('CO', '__')],
                         [], msg='Names other than CONVERT_STR_* found.')

    def test_convert_repr_and_str(self):
        test_type = enum.StrEnum._convert_(
                'UnittestConvert',
                MODULE,
                filter=lambda x: x.startswith('CONVERT_STR_'))
        self.assertEqual(repr(test_type.CONVERT_STR_TEST_1), '%s.CONVERT_STR_TEST_1' % SHORT_MODULE)
        self.assertEqual(str(test_type.CONVERT_STR_TEST_2), 'goodbye')
        self.assertEqual(format(test_type.CONVERT_STR_TEST_1), 'hello')


if __name__ == '__main__':
    unittest.main()
