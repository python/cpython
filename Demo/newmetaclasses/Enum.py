"""Enumeration metaclass."""

class EnumMetaclass(type):
    """Metaclass for enumeration.

    To define your own enumeration, do something like

    class Color(Enum):
        red = 1
        green = 2
        blue = 3

    Now, Color.red, Color.green and Color.blue behave totally
    different: they are enumerated values, not integers.

    Enumerations cannot be instantiated; however they can be
    subclassed.
    """

    def __init__(cls, name, bases, dict):
        super(EnumMetaclass, cls).__init__(name, bases, dict)
        cls._members = []
        for attr in dict.keys():
            if not (attr.startswith('__') and attr.endswith('__')):
                enumval = EnumInstance(name, attr, dict[attr])
                setattr(cls, attr, enumval)
                cls._members.append(attr)

    def __getattr__(cls, name):
        if name == "__members__":
            return cls._members
        raise AttributeError(name)

    def __repr__(cls):
        s1 = s2 = ""
        enumbases = [base.__name__ for base in cls.__bases__
                     if isinstance(base, EnumMetaclass) and not base is Enum]
        if enumbases:
            s1 = "(%s)" % ", ".join(enumbases)
        enumvalues = ["%s: %d" % (val, getattr(cls, val))
                      for val in cls._members]
        if enumvalues:
            s2 = ": {%s}" % ", ".join(enumvalues)
        return "%s%s%s" % (cls.__name__, s1, s2)

class FullEnumMetaclass(EnumMetaclass):
    """Metaclass for full enumerations.

    A full enumeration displays all the values defined in base classes.
    """

    def __init__(cls, name, bases, dict):
        super(FullEnumMetaclass, cls).__init__(name, bases, dict)
        for obj in cls.__mro__:
            if isinstance(obj, EnumMetaclass):
                for attr in obj._members:
                    # XXX inefficient
                    if not attr in cls._members:
                        cls._members.append(attr)

class EnumInstance(int):
    """Class to represent an enumeration value.

    EnumInstance('Color', 'red', 12) prints as 'Color.red' and behaves
    like the integer 12 when compared, but doesn't support arithmetic.

    XXX Should it record the actual enumeration rather than just its
    name?
    """

    def __new__(cls, classname, enumname, value):
        return int.__new__(cls, value)

    def __init__(self, classname, enumname, value):
        self.__classname = classname
        self.__enumname = enumname

    def __repr__(self):
        return "EnumInstance(%s, %s, %d)" % (self.__classname, self.__enumname,
                                             self)

    def __str__(self):
        return "%s.%s" % (self.__classname, self.__enumname)

class Enum:
    __metaclass__ = EnumMetaclass

class FullEnum:
    __metaclass__ = FullEnumMetaclass

def _test():

    class Color(Enum):
        red = 1
        green = 2
        blue = 3

    print(Color.red)

    print(repr(Color.red))
    print(Color.red == Color.red)
    print(Color.red == Color.blue)
    print(Color.red == 1)
    print(Color.red == 2)

    class ExtendedColor(Color):
        white = 0
        orange = 4
        yellow = 5
        purple = 6
        black = 7

    print(ExtendedColor.orange)
    print(ExtendedColor.red)

    print(Color.red == ExtendedColor.red)

    class OtherColor(Enum):
        white = 4
        blue = 5

    class MergedColor(Color, OtherColor):
        pass

    print(MergedColor.red)
    print(MergedColor.white)

    print(Color)
    print(ExtendedColor)
    print(OtherColor)
    print(MergedColor)

def _test2():

    class Color(FullEnum):
        red = 1
        green = 2
        blue = 3

    print(Color.red)

    print(repr(Color.red))
    print(Color.red == Color.red)
    print(Color.red == Color.blue)
    print(Color.red == 1)
    print(Color.red == 2)

    class ExtendedColor(Color):
        white = 0
        orange = 4
        yellow = 5
        purple = 6
        black = 7

    print(ExtendedColor.orange)
    print(ExtendedColor.red)

    print(Color.red == ExtendedColor.red)

    class OtherColor(FullEnum):
        white = 4
        blue = 5

    class MergedColor(Color, OtherColor):
        pass

    print(MergedColor.red)
    print(MergedColor.white)

    print(Color)
    print(ExtendedColor)
    print(OtherColor)
    print(MergedColor)

if __name__ == '__main__':
    _test()
    _test2()
