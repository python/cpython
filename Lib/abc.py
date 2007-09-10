# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Abstract Base Classes (ABCs) according to PEP 3119."""


def abstractmethod(funcobj):
    """A decorator indicating abstract methods.

    Requires that the metaclass is ABCMeta or derived from it.  A
    class that has a metaclass derived from ABCMeta cannot be
    instantiated unless all of its abstract methods are overridden.
    The abstract methods can be called using any of the the normal
    'super' call mechanisms.

    Usage:

        class C(metaclass=ABCMeta):
            @abstractmethod
            def my_abstract_method(self, ...):
                ...
    """
    funcobj.__isabstractmethod__ = True
    return funcobj


class abstractproperty(property):
    """A decorator indicating abstract properties.

    Requires that the metaclass is ABCMeta or derived from it.  A
    class that has a metaclass derived from ABCMeta cannot be
    instantiated unless all of its abstract properties are overridden.
    The abstract properties can be called using any of the the normal
    'super' call mechanisms.

    Usage:

        class C(metaclass=ABCMeta):
            @abstractproperty
            def my_abstract_property(self):
                ...

    This defines a read-only property; you can also define a read-write
    abstract property using the 'long' form of property declaration:

        class C(metaclass=ABCMeta):
            def getx(self): ...
            def setx(self, value): ...
            x = abstractproperty(getx, setx)
    """
    __isabstractmethod__ = True


class _Abstract(object):

    """Helper class inserted into the bases by ABCMeta (using _fix_bases()).

    You should never need to explicitly subclass this class.

    There should never be a base class between _Abstract and object.
    """

    def __new__(cls, *args, **kwds):
        am = cls.__dict__.get("__abstractmethods__")
        if am:
            raise TypeError("Can't instantiate abstract class %s "
                            "with abstract methods %s" %
                            (cls.__name__, ", ".join(sorted(am))))
        if (args or kwds) and cls.__init__ is object.__init__:
            raise TypeError("Can't pass arguments to __new__ "
                            "without overriding __init__")
        return object.__new__(cls)

    @classmethod
    def __subclasshook__(cls, subclass):
        """Abstract classes can override this to customize issubclass().

        This is invoked early on by __subclasscheck__() below.  It
        should return True, False or NotImplemented.  If it returns
        NotImplemented, the normal algorithm is used.  Otherwise, it
        overrides the normal algorithm (and the outcome is cached).
        """
        return NotImplemented


def _fix_bases(bases):
    """Helper method that inserts _Abstract in the bases if needed."""
    for base in bases:
        if issubclass(base, _Abstract):
            # _Abstract is already a base (maybe indirectly)
            return bases
    if object in bases:
        # Replace object with _Abstract
        return tuple([_Abstract if base is object else base
                      for base in bases])
    # Append _Abstract to the end
    return bases + (_Abstract,)


class ABCMeta(type):

    """Metaclass for defining Abstract Base Classes (ABCs).

    Use this metaclass to create an ABC.  An ABC can be subclassed
    directly, and then acts as a mix-in class.  You can also register
    unrelated concrete classes (even built-in classes) and unrelated
    ABCs as 'virtual subclasses' -- these and their descendants will
    be considered subclasses of the registering ABC by the built-in
    issubclass() function, but the registering ABC won't show up in
    their MRO (Method Resolution Order) nor will method
    implementations defined by the registering ABC be callable (not
    even via super()).

    """

    # A global counter that is incremented each time a class is
    # registered as a virtual subclass of anything.  It forces the
    # negative cache to be cleared before its next use.
    _abc_invalidation_counter = 0

    def __new__(mcls, name, bases, namespace):
        bases = _fix_bases(bases)
        cls = super(ABCMeta, mcls).__new__(mcls, name, bases, namespace)
        # Compute set of abstract method names
        abstracts = set(name
                     for name, value in namespace.items()
                     if getattr(value, "__isabstractmethod__", False))
        for base in bases:
            for name in getattr(base, "__abstractmethods__", set()):
                value = getattr(cls, name, None)
                if getattr(value, "__isabstractmethod__", False):
                    abstracts.add(name)
        cls.__abstractmethods__ = abstracts
        # Set up inheritance registry
        cls._abc_registry = set()
        cls._abc_cache = set()
        cls._abc_negative_cache = set()
        cls._abc_negative_cache_version = ABCMeta._abc_invalidation_counter
        return cls

    def register(cls, subclass):
        """Register a virtual subclass of an ABC."""
        if not isinstance(cls, type):
            raise TypeError("Can only register classes")
        if issubclass(subclass, cls):
            return  # Already a subclass
        # Subtle: test for cycles *after* testing for "already a subclass";
        # this means we allow X.register(X) and interpret it as a no-op.
        if issubclass(cls, subclass):
            # This would create a cycle, which is bad for the algorithm below
            raise RuntimeError("Refusing to create an inheritance cycle")
        cls._abc_registry.add(subclass)
        ABCMeta._abc_invalidation_counter += 1  # Invalidate negative cache

    def _dump_registry(cls, file=None):
        """Debug helper to print the ABC registry."""
        print >> file, "Class: %s.%s" % (cls.__module__, cls.__name__)
        print >> file, "Inv.counter: %s" % ABCMeta._abc_invalidation_counter
        for name in sorted(cls.__dict__.keys()):
            if name.startswith("_abc_"):
                value = getattr(cls, name)
                print >> file, "%s: %r" % (name, value)

    def __instancecheck__(cls, instance):
        """Override for isinstance(instance, cls)."""
        return any(cls.__subclasscheck__(c)
                   for c in set([instance.__class__, type(instance)]))

    def __subclasscheck__(cls, subclass):
        """Override for issubclass(subclass, cls)."""
        # Check cache
        if subclass in cls._abc_cache:
            return True
        # Check negative cache; may have to invalidate
        if cls._abc_negative_cache_version < ABCMeta._abc_invalidation_counter:
            # Invalidate the negative cache
            cls._abc_negative_cache = set()
            cls._abc_negative_cache_version = ABCMeta._abc_invalidation_counter
        elif subclass in cls._abc_negative_cache:
            return False
        # Check the subclass hook
        ok = cls.__subclasshook__(subclass)
        if ok is not NotImplemented:
            assert isinstance(ok, bool)
            if ok:
                cls._abc_cache.add(subclass)
            else:
                cls._abc_negative_cache.add(subclass)
            return ok
        # Check if it's a direct subclass
        if cls in subclass.__mro__:
            cls._abc_cache.add(subclass)
            return True
        # Check if it's a subclass of a registered class (recursive)
        for rcls in cls._abc_registry:
            if issubclass(subclass, rcls):
                cls._abc_registry.add(subclass)
                return True
        # Check if it's a subclass of a subclass (recursive)
        for scls in cls.__subclasses__():
            if issubclass(subclass, scls):
                cls._abc_registry.add(subclass)
                return True
        # No dice; update negative cache
        cls._abc_negative_cache.add(subclass)
        return False
