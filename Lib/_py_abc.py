from _weakrefset import WeakSet


def get_cache_token():
    """Returns the current ABC cache token.

    The token is an opaque object (supporting equality testing) identifying the
    current version of the ABC cache for virtual subclasses. The token changes
    with every call to ``register()`` on any ABC.
    """
    return ABCMeta._abc_invalidation_counter


class _abc_data(object):
    def __init__(self):
        self._abc_registry = WeakSet()
        self._abc_cache = WeakSet()
        self._abc_negative_cache = WeakSet()
        self._abc_negative_cache_version = ABCMeta._abc_invalidation_counter


def _compute_abstract_methods(cls):
    """Compute set of abstract method names"""
    abstracts = {name
                 for name, value in cls.__dict__.items()
                 if getattr(value, "__isabstractmethod__", False)}
    for base in cls.__bases__:
        for name in getattr(base, "__abstractmethods__", set()):
            value = getattr(cls, name, None)
            if getattr(value, "__isabstractmethod__", False):
                abstracts.add(name)
    cls.__abstractmethods__ = frozenset(abstracts)


def _abc_init(cls):
    _compute_abstract_methods(cls)
    # Set up inheritance registry
    cls._abc_impl = _abc_data()


def _get_impl(cls):
    if '_abc_impl' not in cls.__dict__:
        _abc_init(cls)
    return cls._abc_impl


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
    # Note: this counter is private. Use `abc.get_cache_token()` for
    #       external code.
    _abc_invalidation_counter = 0

    def __new__(mcls, name, bases, namespace, /, **kwargs):
        cls = super().__new__(mcls, name, bases, namespace, **kwargs)
        _abc_init(cls)
        return cls

    def register(cls, subclass):
        """Register a virtual subclass of an ABC.

        Returns the subclass, to allow usage as a class decorator.
        """
        if not isinstance(subclass, type):
            raise TypeError("Can only register classes")
        if issubclass(subclass, cls):
            return subclass  # Already a subclass
        # Subtle: test for cycles *after* testing for "already a subclass";
        # this means we allow X.register(X) and interpret it as a no-op.
        if issubclass(cls, subclass):
            # This would create a cycle, which is bad for the algorithm below
            raise RuntimeError("Refusing to create an inheritance cycle")
        abc_impl = _get_impl(cls)
        abc_impl._abc_registry.add(subclass)
        ABCMeta._abc_invalidation_counter += 1  # Invalidate negative cache
        return subclass

    def _dump_registry(cls, file=None):
        """Debug helper to print the ABC registry."""
        print(f"Class: {cls.__module__}.{cls.__qualname__}", file=file)
        print(f"Inv. counter: {get_cache_token()}", file=file)
        for name in cls.__dict__:
            if name.startswith("_abc_"):
                value = getattr(cls, name)
                if isinstance(value, WeakSet):
                    value = set(value)
                print(f"{name}: {value!r}", file=file)

    def _abc_registry_clear(cls):
        """Clear the registry (for debugging or testing)."""
        abc_impl = _get_impl(cls)
        abc_impl._abc_registry.clear()

    def _abc_caches_clear(cls):
        """Clear the caches (for debugging or testing)."""
        abc_impl = _get_impl(cls)
        abc_impl._abc_cache.clear()
        abc_impl._abc_negative_cache.clear()

    def __instancecheck__(cls, instance):
        """Override for isinstance(instance, cls)."""
        # Inline the cache checking
        subclass = instance.__class__
        abc_impl = _get_impl(cls)
        if subclass in abc_impl._abc_cache:
            return True
        subtype = type(instance)
        if subtype is subclass:
            if (abc_impl._abc_negative_cache_version ==
                ABCMeta._abc_invalidation_counter and
                subclass in abc_impl._abc_negative_cache):
                return False
            # Fall back to the subclass check.
            return cls.__subclasscheck__(subclass)
        return any(cls.__subclasscheck__(c) for c in (subclass, subtype))

    def __subclasscheck__(cls, subclass):
        """Override for issubclass(subclass, cls)."""
        if not isinstance(subclass, type):
            raise TypeError('issubclass() arg 1 must be a class')
        abc_impl = _get_impl(cls)
        # Check cache
        if subclass in abc_impl._abc_cache:
            return True
        # Check negative cache; may have to invalidate
        if abc_impl._abc_negative_cache_version < ABCMeta._abc_invalidation_counter:
            # Invalidate the negative cache
            abc_impl._abc_negative_cache = WeakSet()
            abc_impl._abc_negative_cache_version = ABCMeta._abc_invalidation_counter
        elif subclass in abc_impl._abc_negative_cache:
            return False
        # Check the subclass hook
        ok = cls.__subclasshook__(subclass)
        if ok is not NotImplemented:
            assert isinstance(ok, bool)
            if ok:
                abc_impl._abc_cache.add(subclass)
            else:
                abc_impl._abc_negative_cache.add(subclass)
            return ok
        # Check if it's a direct subclass
        if cls in getattr(subclass, '__mro__', ()):
            abc_impl._abc_cache.add(subclass)
            return True
        # Check if it's a subclass of a registered class (recursive)
        for rcls in abc_impl._abc_registry:
            if issubclass(subclass, rcls):
                abc_impl._abc_cache.add(subclass)
                return True
        # Check if it's a subclass of a subclass (recursive)
        for scls in cls.__subclasses__():
            if issubclass(subclass, scls):
                abc_impl._abc_cache.add(subclass)
                return True
        # No dice; update negative cache
        abc_impl._abc_negative_cache.add(subclass)
        return False
