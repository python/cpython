import abc
from abc import abstractmethod, abstractproperty
import collections
import contextlib
import functools
import re as stdlib_re  # Avoid confusion with the re we export.
import sys
import types
try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc  # Fallback for PY3.2.


# Please keep __all__ alphabetized within each category.
__all__ = [
    # Super-special typing primitives.
    'Any',
    'Callable',
    'Generic',
    'Optional',
    'Tuple',
    'Type',
    'TypeVar',
    'Union',

    # ABCs (from collections.abc).
    'AbstractSet',  # collections.abc.Set.
    'Awaitable',
    'AsyncIterator',
    'AsyncIterable',
    'ByteString',
    'Container',
    'Hashable',
    'ItemsView',
    'Iterable',
    'Iterator',
    'KeysView',
    'Mapping',
    'MappingView',
    'MutableMapping',
    'MutableSequence',
    'MutableSet',
    'Sequence',
    'Sized',
    'ValuesView',

    # Structural checks, a.k.a. protocols.
    'Reversible',
    'SupportsAbs',
    'SupportsFloat',
    'SupportsInt',
    'SupportsRound',

    # Concrete collection types.
    'Dict',
    'DefaultDict',
    'List',
    'Set',
    'NamedTuple',  # Not really a type.
    'Generator',

    # One-off things.
    'AnyStr',
    'cast',
    'get_type_hints',
    'NewType',
    'no_type_check',
    'no_type_check_decorator',
    'overload',
    'Text',
    'TYPE_CHECKING',
]

# The pseudo-submodules 're' and 'io' are part of the public
# namespace, but excluded from __all__ because they might stomp on
# legitimate imports of those modules.


def _qualname(x):
    if sys.version_info[:2] >= (3, 3):
        return x.__qualname__
    else:
        # Fall back to just name.
        return x.__name__


class TypingMeta(type):
    """Metaclass for every type defined below.

    This overrides __new__() to require an extra keyword parameter
    '_root', which serves as a guard against naive subclassing of the
    typing classes.  Any legitimate class defined using a metaclass
    derived from TypingMeta (including internal subclasses created by
    e.g.  Union[X, Y]) must pass _root=True.

    This also defines a dummy constructor (all the work is done in
    __new__) and a nicer repr().
    """

    _is_protocol = False

    def __new__(cls, name, bases, namespace, *, _root=False):
        if not _root:
            raise TypeError("Cannot subclass %s" %
                            (', '.join(map(_type_repr, bases)) or '()'))
        return super().__new__(cls, name, bases, namespace)

    def __init__(self, *args, **kwds):
        pass

    def _eval_type(self, globalns, localns):
        """Override this in subclasses to interpret forward references.

        For example, Union['C'] is internally stored as
        Union[_ForwardRef('C')], which should evaluate to _Union[C],
        where C is an object found in globalns or localns (searching
        localns first, of course).
        """
        return self

    def _get_type_vars(self, tvars):
        pass

    def __repr__(self):
        return '%s.%s' % (self.__module__, _qualname(self))


class Final:
    """Mix-in class to prevent instantiation."""

    __slots__ = ()

    def __new__(self, *args, **kwds):
        raise TypeError("Cannot instantiate %r" % self.__class__)


class _ForwardRef(TypingMeta):
    """Wrapper to hold a forward reference."""

    def __new__(cls, arg):
        if not isinstance(arg, str):
            raise TypeError('ForwardRef must be a string -- got %r' % (arg,))
        try:
            code = compile(arg, '<string>', 'eval')
        except SyntaxError:
            raise SyntaxError('ForwardRef must be an expression -- got %r' %
                              (arg,))
        self = super().__new__(cls, arg, (), {}, _root=True)
        self.__forward_arg__ = arg
        self.__forward_code__ = code
        self.__forward_evaluated__ = False
        self.__forward_value__ = None
        typing_globals = globals()
        frame = sys._getframe(1)
        while frame is not None and frame.f_globals is typing_globals:
            frame = frame.f_back
        assert frame is not None
        self.__forward_frame__ = frame
        return self

    def _eval_type(self, globalns, localns):
        if not isinstance(localns, dict):
            raise TypeError('ForwardRef localns must be a dict -- got %r' %
                            (localns,))
        if not isinstance(globalns, dict):
            raise TypeError('ForwardRef globalns must be a dict -- got %r' %
                            (globalns,))
        if not self.__forward_evaluated__:
            if globalns is None and localns is None:
                globalns = localns = {}
            elif globalns is None:
                globalns = localns
            elif localns is None:
                localns = globalns
            self.__forward_value__ = _type_check(
                eval(self.__forward_code__, globalns, localns),
                "Forward references must evaluate to types.")
            self.__forward_evaluated__ = True
        return self.__forward_value__

    def __instancecheck__(self, obj):
        raise TypeError("Forward references cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if not self.__forward_evaluated__:
            globalns = self.__forward_frame__.f_globals
            localns = self.__forward_frame__.f_locals
            try:
                self._eval_type(globalns, localns)
            except NameError:
                return False  # Too early.
        return issubclass(cls, self.__forward_value__)

    def __repr__(self):
        return '_ForwardRef(%r)' % (self.__forward_arg__,)


class _TypeAlias:
    """Internal helper class for defining generic variants of concrete types.

    Note that this is not a type; let's call it a pseudo-type.  It can
    be used in instance and subclass checks, e.g. isinstance(m, Match)
    or issubclass(type(m), Match).  However, it cannot be itself the
    target of an issubclass() call; e.g. issubclass(Match, C) (for
    some arbitrary class C) raises TypeError rather than returning
    False.
    """

    __slots__ = ('name', 'type_var', 'impl_type', 'type_checker')

    def __new__(cls, *args, **kwds):
        """Constructor.

        This only exists to give a better error message in case
        someone tries to subclass a type alias (not a good idea).
        """
        if (len(args) == 3 and
                isinstance(args[0], str) and
                isinstance(args[1], tuple)):
            # Close enough.
            raise TypeError("A type alias cannot be subclassed")
        return object.__new__(cls)

    def __init__(self, name, type_var, impl_type, type_checker):
        """Initializer.

        Args:
            name: The name, e.g. 'Pattern'.
            type_var: The type parameter, e.g. AnyStr, or the
                specific type, e.g. str.
            impl_type: The implementation type.
            type_checker: Function that takes an impl_type instance.
                and returns a value that should be a type_var instance.
        """
        assert isinstance(name, str), repr(name)
        assert isinstance(type_var, type), repr(type_var)
        assert isinstance(impl_type, type), repr(impl_type)
        assert not isinstance(impl_type, TypingMeta), repr(impl_type)
        self.name = name
        self.type_var = type_var
        self.impl_type = impl_type
        self.type_checker = type_checker

    def __repr__(self):
        return "%s[%s]" % (self.name, _type_repr(self.type_var))

    def __getitem__(self, parameter):
        assert isinstance(parameter, type), repr(parameter)
        if not isinstance(self.type_var, TypeVar):
            raise TypeError("%s cannot be further parameterized." % self)
        if self.type_var.__constraints__:
            if not issubclass(parameter, Union[self.type_var.__constraints__]):
                raise TypeError("%s is not a valid substitution for %s." %
                                (parameter, self.type_var))
        return self.__class__(self.name, parameter,
                              self.impl_type, self.type_checker)

    def __instancecheck__(self, obj):
        raise TypeError("Type aliases cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if cls is Any:
            return True
        if isinstance(cls, _TypeAlias):
            # Covariance.  For now, we compare by name.
            return (cls.name == self.name and
                    issubclass(cls.type_var, self.type_var))
        else:
            # Note that this is too lenient, because the
            # implementation type doesn't carry information about
            # whether it is about bytes or str (for example).
            return issubclass(cls, self.impl_type)


def _get_type_vars(types, tvars):
    for t in types:
        if isinstance(t, TypingMeta):
            t._get_type_vars(tvars)


def _type_vars(types):
    tvars = []
    _get_type_vars(types, tvars)
    return tuple(tvars)


def _eval_type(t, globalns, localns):
    if isinstance(t, TypingMeta):
        return t._eval_type(globalns, localns)
    else:
        return t


def _type_check(arg, msg):
    """Check that the argument is a type, and return it.

    As a special case, accept None and return type(None) instead.
    Also, _TypeAlias instances (e.g. Match, Pattern) are acceptable.

    The msg argument is a human-readable error message, e.g.

        "Union[arg, ...]: arg should be a type."

    We append the repr() of the actual value (truncated to 100 chars).
    """
    if arg is None:
        return type(None)
    if isinstance(arg, str):
        arg = _ForwardRef(arg)
    if not isinstance(arg, (type, _TypeAlias)) and not callable(arg):
        raise TypeError(msg + " Got %.100r." % (arg,))
    return arg


def _type_repr(obj):
    """Return the repr() of an object, special-casing types.

    If obj is a type, we return a shorter version than the default
    type.__repr__, based on the module and qualified name, which is
    typically enough to uniquely identify a type.  For everything
    else, we fall back on repr(obj).
    """
    if isinstance(obj, type) and not isinstance(obj, TypingMeta):
        if obj.__module__ == 'builtins':
            return _qualname(obj)
        else:
            return '%s.%s' % (obj.__module__, _qualname(obj))
    else:
        return repr(obj)


class AnyMeta(TypingMeta):
    """Metaclass for Any."""

    def __new__(cls, name, bases, namespace, _root=False):
        self = super().__new__(cls, name, bases, namespace, _root=_root)
        return self

    def __instancecheck__(self, obj):
        raise TypeError("Any cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if not isinstance(cls, type):
            return super().__subclasscheck__(cls)  # To TypeError.
        return True


class Any(Final, metaclass=AnyMeta, _root=True):
    """Special type indicating an unconstrained type.

    - Any object is an instance of Any.
    - Any class is a subclass of Any.
    - As a special case, Any and object are subclasses of each other.
    """

    __slots__ = ()


class TypeVar(TypingMeta, metaclass=TypingMeta, _root=True):
    """Type variable.

    Usage::

      T = TypeVar('T')  # Can be anything
      A = TypeVar('A', str, bytes)  # Must be str or bytes

    Type variables exist primarily for the benefit of static type
    checkers.  They serve as the parameters for generic types as well
    as for generic function definitions.  See class Generic for more
    information on generic types.  Generic functions work as follows:

      def repeat(x: T, n: int) -> Sequence[T]:
          '''Return a list containing n references to x.'''
          return [x]*n

      def longest(x: A, y: A) -> A:
          '''Return the longest of two strings.'''
          return x if len(x) >= len(y) else y

    The latter example's signature is essentially the overloading
    of (str, str) -> str and (bytes, bytes) -> bytes.  Also note
    that if the arguments are instances of some subclass of str,
    the return type is still plain str.

    At runtime, isinstance(x, T) will raise TypeError.  However,
    issubclass(C, T) is true for any class C, and issubclass(str, A)
    and issubclass(bytes, A) are true, and issubclass(int, A) is
    false.  (TODO: Why is this needed?  This may change.  See #136.)

    Type variables may be marked covariant or contravariant by passing
    covariant=True or contravariant=True.  See PEP 484 for more
    details.  By default type variables are invariant.

    Type variables can be introspected. e.g.:

      T.__name__ == 'T'
      T.__constraints__ == ()
      T.__covariant__ == False
      T.__contravariant__ = False
      A.__constraints__ == (str, bytes)
    """

    def __new__(cls, name, *constraints, bound=None,
                covariant=False, contravariant=False):
        self = super().__new__(cls, name, (Final,), {}, _root=True)
        if covariant and contravariant:
            raise ValueError("Bivariant type variables are not supported.")
        self.__covariant__ = bool(covariant)
        self.__contravariant__ = bool(contravariant)
        if constraints and bound is not None:
            raise TypeError("Constraints cannot be combined with bound=...")
        if constraints and len(constraints) == 1:
            raise TypeError("A single constraint is not allowed")
        msg = "TypeVar(name, constraint, ...): constraints must be types."
        self.__constraints__ = tuple(_type_check(t, msg) for t in constraints)
        if bound:
            self.__bound__ = _type_check(bound, "Bound must be a type.")
        else:
            self.__bound__ = None
        return self

    def _get_type_vars(self, tvars):
        if self not in tvars:
            tvars.append(self)

    def __repr__(self):
        if self.__covariant__:
            prefix = '+'
        elif self.__contravariant__:
            prefix = '-'
        else:
            prefix = '~'
        return prefix + self.__name__

    def __instancecheck__(self, instance):
        raise TypeError("Type variables cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        # TODO: Make this raise TypeError too?
        if cls is self:
            return True
        if cls is Any:
            return True
        if self.__bound__ is not None:
            return issubclass(cls, self.__bound__)
        if self.__constraints__:
            return any(issubclass(cls, c) for c in self.__constraints__)
        return True


# Some unconstrained type variables.  These are used by the container types.
# (These are not for export.)
T = TypeVar('T')  # Any type.
KT = TypeVar('KT')  # Key type.
VT = TypeVar('VT')  # Value type.
T_co = TypeVar('T_co', covariant=True)  # Any type covariant containers.
V_co = TypeVar('V_co', covariant=True)  # Any type covariant containers.
VT_co = TypeVar('VT_co', covariant=True)  # Value type covariant containers.
T_contra = TypeVar('T_contra', contravariant=True)  # Ditto contravariant.

# A useful type variable with constraints.  This represents string types.
# (This one *is* for export!)
AnyStr = TypeVar('AnyStr', bytes, str)


class UnionMeta(TypingMeta):
    """Metaclass for Union."""

    def __new__(cls, name, bases, namespace, parameters=None, _root=False):
        if parameters is None:
            return super().__new__(cls, name, bases, namespace, _root=_root)
        if not isinstance(parameters, tuple):
            raise TypeError("Expected parameters=<tuple>")
        # Flatten out Union[Union[...], ...] and type-check non-Union args.
        params = []
        msg = "Union[arg, ...]: each arg must be a type."
        for p in parameters:
            if isinstance(p, UnionMeta):
                params.extend(p.__union_params__)
            else:
                params.append(_type_check(p, msg))
        # Weed out strict duplicates, preserving the first of each occurrence.
        all_params = set(params)
        if len(all_params) < len(params):
            new_params = []
            for t in params:
                if t in all_params:
                    new_params.append(t)
                    all_params.remove(t)
            params = new_params
            assert not all_params, all_params
        # Weed out subclasses.
        # E.g. Union[int, Employee, Manager] == Union[int, Employee].
        # If Any or object is present it will be the sole survivor.
        # If both Any and object are present, Any wins.
        # Never discard type variables, except against Any.
        # (In particular, Union[str, AnyStr] != AnyStr.)
        all_params = set(params)
        for t1 in params:
            if t1 is Any:
                return Any
            if isinstance(t1, TypeVar):
                continue
            if isinstance(t1, _TypeAlias):
                # _TypeAlias is not a real class.
                continue
            if not isinstance(t1, type):
                assert callable(t1)  # A callable might sneak through.
                continue
            if any(isinstance(t2, type) and issubclass(t1, t2)
                   for t2 in all_params - {t1} if not isinstance(t2, TypeVar)):
                all_params.remove(t1)
        # It's not a union if there's only one type left.
        if len(all_params) == 1:
            return all_params.pop()
        # Create a new class with these params.
        self = super().__new__(cls, name, bases, {}, _root=True)
        self.__union_params__ = tuple(t for t in params if t in all_params)
        self.__union_set_params__ = frozenset(self.__union_params__)
        return self

    def _eval_type(self, globalns, localns):
        p = tuple(_eval_type(t, globalns, localns)
                  for t in self.__union_params__)
        if p == self.__union_params__:
            return self
        else:
            return self.__class__(self.__name__, self.__bases__, {},
                                  p, _root=True)

    def _get_type_vars(self, tvars):
        if self.__union_params__:
            _get_type_vars(self.__union_params__, tvars)

    def __repr__(self):
        r = super().__repr__()
        if self.__union_params__:
            r += '[%s]' % (', '.join(_type_repr(t)
                                     for t in self.__union_params__))
        return r

    def __getitem__(self, parameters):
        if self.__union_params__ is not None:
            raise TypeError(
                "Cannot subscript an existing Union. Use Union[u, t] instead.")
        if parameters == ():
            raise TypeError("Cannot take a Union of no types.")
        if not isinstance(parameters, tuple):
            parameters = (parameters,)
        return self.__class__(self.__name__, self.__bases__,
                              dict(self.__dict__), parameters, _root=True)

    def __eq__(self, other):
        if not isinstance(other, UnionMeta):
            return NotImplemented
        return self.__union_set_params__ == other.__union_set_params__

    def __hash__(self):
        return hash(self.__union_set_params__)

    def __instancecheck__(self, obj):
        raise TypeError("Unions cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if cls is Any:
            return True
        if self.__union_params__ is None:
            return isinstance(cls, UnionMeta)
        elif isinstance(cls, UnionMeta):
            if cls.__union_params__ is None:
                return False
            return all(issubclass(c, self) for c in (cls.__union_params__))
        elif isinstance(cls, TypeVar):
            if cls in self.__union_params__:
                return True
            if cls.__constraints__:
                return issubclass(Union[cls.__constraints__], self)
            return False
        else:
            return any(issubclass(cls, t) for t in self.__union_params__)


class Union(Final, metaclass=UnionMeta, _root=True):
    """Union type; Union[X, Y] means either X or Y.

    To define a union, use e.g. Union[int, str].  Details:

    - The arguments must be types and there must be at least one.

    - None as an argument is a special case and is replaced by
      type(None).

    - Unions of unions are flattened, e.g.::

        Union[Union[int, str], float] == Union[int, str, float]

    - Unions of a single argument vanish, e.g.::

        Union[int] == int  # The constructor actually returns int

    - Redundant arguments are skipped, e.g.::

        Union[int, str, int] == Union[int, str]

    - When comparing unions, the argument order is ignored, e.g.::

        Union[int, str] == Union[str, int]

    - When two arguments have a subclass relationship, the least
      derived argument is kept, e.g.::

        class Employee: pass
        class Manager(Employee): pass
        Union[int, Employee, Manager] == Union[int, Employee]
        Union[Manager, int, Employee] == Union[int, Employee]
        Union[Employee, Manager] == Employee

    - Corollary: if Any is present it is the sole survivor, e.g.::

        Union[int, Any] == Any

    - Similar for object::

        Union[int, object] == object

    - To cut a tie: Union[object, Any] == Union[Any, object] == Any.

    - You cannot subclass or instantiate a union.

    - You cannot write Union[X][Y] (what would it mean?).

    - You can use Optional[X] as a shorthand for Union[X, None].
    """

    # Unsubscripted Union type has params set to None.
    __union_params__ = None
    __union_set_params__ = None


class OptionalMeta(TypingMeta):
    """Metaclass for Optional."""

    def __new__(cls, name, bases, namespace, _root=False):
        return super().__new__(cls, name, bases, namespace, _root=_root)

    def __getitem__(self, arg):
        arg = _type_check(arg, "Optional[t] requires a single type.")
        return Union[arg, type(None)]


class Optional(Final, metaclass=OptionalMeta, _root=True):
    """Optional type.

    Optional[X] is equivalent to Union[X, type(None)].
    """

    __slots__ = ()


class TupleMeta(TypingMeta):
    """Metaclass for Tuple."""

    def __new__(cls, name, bases, namespace, parameters=None,
                use_ellipsis=False, _root=False):
        self = super().__new__(cls, name, bases, namespace, _root=_root)
        self.__tuple_params__ = parameters
        self.__tuple_use_ellipsis__ = use_ellipsis
        return self

    def _get_type_vars(self, tvars):
        if self.__tuple_params__:
            _get_type_vars(self.__tuple_params__, tvars)

    def _eval_type(self, globalns, localns):
        tp = self.__tuple_params__
        if tp is None:
            return self
        p = tuple(_eval_type(t, globalns, localns) for t in tp)
        if p == self.__tuple_params__:
            return self
        else:
            return self.__class__(self.__name__, self.__bases__, {},
                                  p, _root=True)

    def __repr__(self):
        r = super().__repr__()
        if self.__tuple_params__ is not None:
            params = [_type_repr(p) for p in self.__tuple_params__]
            if self.__tuple_use_ellipsis__:
                params.append('...')
            if not params:
                params.append('()')
            r += '[%s]' % (
                ', '.join(params))
        return r

    def __getitem__(self, parameters):
        if self.__tuple_params__ is not None:
            raise TypeError("Cannot re-parameterize %r" % (self,))
        if not isinstance(parameters, tuple):
            parameters = (parameters,)
        if len(parameters) == 2 and parameters[1] == Ellipsis:
            parameters = parameters[:1]
            use_ellipsis = True
            msg = "Tuple[t, ...]: t must be a type."
        else:
            use_ellipsis = False
            msg = "Tuple[t0, t1, ...]: each t must be a type."
        parameters = tuple(_type_check(p, msg) for p in parameters)
        return self.__class__(self.__name__, self.__bases__,
                              dict(self.__dict__), parameters,
                              use_ellipsis=use_ellipsis, _root=True)

    def __eq__(self, other):
        if not isinstance(other, TupleMeta):
            return NotImplemented
        return (self.__tuple_params__ == other.__tuple_params__ and
                self.__tuple_use_ellipsis__ == other.__tuple_use_ellipsis__)

    def __hash__(self):
        return hash(self.__tuple_params__)

    def __instancecheck__(self, obj):
        raise TypeError("Tuples cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if cls is Any:
            return True
        if not isinstance(cls, type):
            return super().__subclasscheck__(cls)  # To TypeError.
        if issubclass(cls, tuple):
            return True  # Special case.
        if not isinstance(cls, TupleMeta):
            return super().__subclasscheck__(cls)  # False.
        if self.__tuple_params__ is None:
            return True
        if cls.__tuple_params__ is None:
            return False  # ???
        if cls.__tuple_use_ellipsis__ != self.__tuple_use_ellipsis__:
            return False
        # Covariance.
        return (len(self.__tuple_params__) == len(cls.__tuple_params__) and
                all(issubclass(x, p)
                    for x, p in zip(cls.__tuple_params__,
                                    self.__tuple_params__)))


class Tuple(Final, metaclass=TupleMeta, _root=True):
    """Tuple type; Tuple[X, Y] is the cross-product type of X and Y.

    Example: Tuple[T1, T2] is a tuple of two elements corresponding
    to type variables T1 and T2.  Tuple[int, float, str] is a tuple
    of an int, a float and a string.

    To specify a variable-length tuple of homogeneous type, use Sequence[T].
    """

    __slots__ = ()


class CallableMeta(TypingMeta):
    """Metaclass for Callable."""

    def __new__(cls, name, bases, namespace, _root=False,
                args=None, result=None):
        if args is None and result is None:
            pass  # Must be 'class Callable'.
        else:
            if args is not Ellipsis:
                if not isinstance(args, list):
                    raise TypeError("Callable[args, result]: "
                                    "args must be a list."
                                    " Got %.100r." % (args,))
                msg = "Callable[[arg, ...], result]: each arg must be a type."
                args = tuple(_type_check(arg, msg) for arg in args)
            msg = "Callable[args, result]: result must be a type."
            result = _type_check(result, msg)
        self = super().__new__(cls, name, bases, namespace, _root=_root)
        self.__args__ = args
        self.__result__ = result
        return self

    def _get_type_vars(self, tvars):
        if self.__args__:
            _get_type_vars(self.__args__, tvars)

    def _eval_type(self, globalns, localns):
        if self.__args__ is None and self.__result__ is None:
            return self
        if self.__args__ is Ellipsis:
            args = self.__args__
        else:
            args = [_eval_type(t, globalns, localns) for t in self.__args__]
        result = _eval_type(self.__result__, globalns, localns)
        if args == self.__args__ and result == self.__result__:
            return self
        else:
            return self.__class__(self.__name__, self.__bases__, {},
                                  args=args, result=result, _root=True)

    def __repr__(self):
        r = super().__repr__()
        if self.__args__ is not None or self.__result__ is not None:
            if self.__args__ is Ellipsis:
                args_r = '...'
            else:
                args_r = '[%s]' % ', '.join(_type_repr(t)
                                            for t in self.__args__)
            r += '[%s, %s]' % (args_r, _type_repr(self.__result__))
        return r

    def __getitem__(self, parameters):
        if self.__args__ is not None or self.__result__ is not None:
            raise TypeError("This Callable type is already parameterized.")
        if not isinstance(parameters, tuple) or len(parameters) != 2:
            raise TypeError(
                "Callable must be used as Callable[[arg, ...], result].")
        args, result = parameters
        return self.__class__(self.__name__, self.__bases__,
                              dict(self.__dict__), _root=True,
                              args=args, result=result)

    def __eq__(self, other):
        if not isinstance(other, CallableMeta):
            return NotImplemented
        return (self.__args__ == other.__args__ and
                self.__result__ == other.__result__)

    def __hash__(self):
        return hash(self.__args__) ^ hash(self.__result__)

    def __instancecheck__(self, obj):
        # For unparametrized Callable we allow this, because
        # typing.Callable should be equivalent to
        # collections.abc.Callable.
        if self.__args__ is None and self.__result__ is None:
            return isinstance(obj, collections_abc.Callable)
        else:
            raise TypeError("Callable[] cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if cls is Any:
            return True
        if not isinstance(cls, CallableMeta):
            return super().__subclasscheck__(cls)
        if self.__args__ is None and self.__result__ is None:
            return True
        # We're not doing covariance or contravariance -- this is *invariance*.
        return self == cls


class Callable(Final, metaclass=CallableMeta, _root=True):
    """Callable type; Callable[[int], str] is a function of (int) -> str.

    The subscription syntax must always be used with exactly two
    values: the argument list and the return type.  The argument list
    must be a list of types; the return type must be a single type.

    There is no syntax to indicate optional or keyword arguments,
    such function types are rarely used as callback types.
    """

    __slots__ = ()


def _gorg(a):
    """Return the farthest origin of a generic class."""
    assert isinstance(a, GenericMeta)
    while a.__origin__ is not None:
        a = a.__origin__
    return a


def _geqv(a, b):
    """Return whether two generic classes are equivalent.

    The intention is to consider generic class X and any of its
    parameterized forms (X[T], X[int], etc.)  as equivalent.

    However, X is not equivalent to a subclass of X.

    The relation is reflexive, symmetric and transitive.
    """
    assert isinstance(a, GenericMeta) and isinstance(b, GenericMeta)
    # Reduce each to its origin.
    return _gorg(a) is _gorg(b)


def _next_in_mro(cls):
    """Helper for Generic.__new__.

    Returns the class after the last occurrence of Generic or
    Generic[...] in cls.__mro__.
    """
    next_in_mro = object
    # Look for the last occurrence of Generic or Generic[...].
    for i, c in enumerate(cls.__mro__[:-1]):
        if isinstance(c, GenericMeta) and _gorg(c) is Generic:
            next_in_mro = cls.__mro__[i+1]
    return next_in_mro


class GenericMeta(TypingMeta, abc.ABCMeta):
    """Metaclass for generic types."""

    def __new__(cls, name, bases, namespace,
                tvars=None, args=None, origin=None, extra=None):
        self = super().__new__(cls, name, bases, namespace, _root=True)

        if tvars is not None:
            # Called from __getitem__() below.
            assert origin is not None
            assert all(isinstance(t, TypeVar) for t in tvars), tvars
        else:
            # Called from class statement.
            assert tvars is None, tvars
            assert args is None, args
            assert origin is None, origin

            # Get the full set of tvars from the bases.
            tvars = _type_vars(bases)
            # Look for Generic[T1, ..., Tn].
            # If found, tvars must be a subset of it.
            # If not found, tvars is it.
            # Also check for and reject plain Generic,
            # and reject multiple Generic[...].
            gvars = None
            for base in bases:
                if base is Generic:
                    raise TypeError("Cannot inherit from plain Generic")
                if (isinstance(base, GenericMeta) and
                        base.__origin__ is Generic):
                    if gvars is not None:
                        raise TypeError(
                            "Cannot inherit from Generic[...] multiple types.")
                    gvars = base.__parameters__
            if gvars is None:
                gvars = tvars
            else:
                tvarset = set(tvars)
                gvarset = set(gvars)
                if not tvarset <= gvarset:
                    raise TypeError(
                        "Some type variables (%s) "
                        "are not listed in Generic[%s]" %
                        (", ".join(str(t) for t in tvars if t not in gvarset),
                         ", ".join(str(g) for g in gvars)))
                tvars = gvars

        self.__parameters__ = tvars
        self.__args__ = args
        self.__origin__ = origin
        self.__extra__ = extra
        # Speed hack (https://github.com/python/typing/issues/196).
        self.__next_in_mro__ = _next_in_mro(self)
        return self

    def _get_type_vars(self, tvars):
        if self.__origin__ and self.__parameters__:
            _get_type_vars(self.__parameters__, tvars)

    def __repr__(self):
        if self.__origin__ is not None:
            r = repr(self.__origin__)
        else:
            r = super().__repr__()
        if self.__args__:
            r += '[%s]' % (
                ', '.join(_type_repr(p) for p in self.__args__))
        if self.__parameters__:
            r += '<%s>' % (
                ', '.join(_type_repr(p) for p in self.__parameters__))
        return r

    def __eq__(self, other):
        if not isinstance(other, GenericMeta):
            return NotImplemented
        if self.__origin__ is not None:
            return (self.__origin__ is other.__origin__ and
                    self.__args__ == other.__args__ and
                    self.__parameters__ == other.__parameters__)
        else:
            return self is other

    def __hash__(self):
        return hash((self.__name__, self.__parameters__))

    def __getitem__(self, params):
        if not isinstance(params, tuple):
            params = (params,)
        if not params:
            raise TypeError(
                "Parameter list to %s[...] cannot be empty" % _qualname(self))
        msg = "Parameters to generic types must be types."
        params = tuple(_type_check(p, msg) for p in params)
        if self is Generic:
            # Generic can only be subscripted with unique type variables.
            if not all(isinstance(p, TypeVar) for p in params):
                raise TypeError(
                    "Parameters to Generic[...] must all be type variables")
            if len(set(params)) != len(params):
                raise TypeError(
                    "Parameters to Generic[...] must all be unique")
            tvars = params
            args = None
        elif self is _Protocol:
            # _Protocol is internal, don't check anything.
            tvars = params
            args = None
        elif self.__origin__ in (Generic, _Protocol):
            # Can't subscript Generic[...] or _Protocol[...].
            raise TypeError("Cannot subscript already-subscripted %s" %
                            repr(self))
        else:
            # Subscripting a regular Generic subclass.
            if not self.__parameters__:
                raise TypeError("%s is not a generic class" % repr(self))
            alen = len(params)
            elen = len(self.__parameters__)
            if alen != elen:
                raise TypeError(
                    "Too %s parameters for %s; actual %s, expected %s" %
                    ("many" if alen > elen else "few", repr(self), alen, elen))
            tvars = _type_vars(params)
            args = params
        return self.__class__(self.__name__,
                              (self,) + self.__bases__,
                              dict(self.__dict__),
                              tvars=tvars,
                              args=args,
                              origin=self,
                              extra=self.__extra__)

    def __instancecheck__(self, instance):
        # Since we extend ABC.__subclasscheck__ and
        # ABC.__instancecheck__ inlines the cache checking done by the
        # latter, we must extend __instancecheck__ too. For simplicity
        # we just skip the cache check -- instance checks for generic
        # classes are supposed to be rare anyways.
        return self.__subclasscheck__(instance.__class__)

    def __subclasscheck__(self, cls):
        if cls is Any:
            return True
        if isinstance(cls, GenericMeta):
            # For a class C(Generic[T]) where T is co-variant,
            # C[X] is a subclass of C[Y] iff X is a subclass of Y.
            origin = self.__origin__
            if origin is not None and origin is cls.__origin__:
                assert len(self.__args__) == len(origin.__parameters__)
                assert len(cls.__args__) == len(origin.__parameters__)
                for p_self, p_cls, p_origin in zip(self.__args__,
                                                   cls.__args__,
                                                   origin.__parameters__):
                    if isinstance(p_origin, TypeVar):
                        if p_origin.__covariant__:
                            # Covariant -- p_cls must be a subclass of p_self.
                            if not issubclass(p_cls, p_self):
                                break
                        elif p_origin.__contravariant__:
                            # Contravariant.  I think it's the opposite. :-)
                            if not issubclass(p_self, p_cls):
                                break
                        else:
                            # Invariant -- p_cls and p_self must equal.
                            if p_self != p_cls:
                                break
                    else:
                        # If the origin's parameter is not a typevar,
                        # insist on invariance.
                        if p_self != p_cls:
                            break
                else:
                    return True
                # If we break out of the loop, the superclass gets a chance.
        if super().__subclasscheck__(cls):
            return True
        if self.__extra__ is None or isinstance(cls, GenericMeta):
            return False
        return issubclass(cls, self.__extra__)


# Prevent checks for Generic to crash when defining Generic.
Generic = None


class Generic(metaclass=GenericMeta):
    """Abstract base class for generic types.

    A generic type is typically declared by inheriting from an
    instantiation of this class with one or more type variables.
    For example, a generic mapping type might be defined as::

      class Mapping(Generic[KT, VT]):
          def __getitem__(self, key: KT) -> VT:
              ...
          # Etc.

    This class can then be used as follows::

      def lookup_name(mapping: Mapping[KT, VT], key: KT, default: VT) -> VT:
          try:
              return mapping[key]
          except KeyError:
              return default
    """

    __slots__ = ()

    def __new__(cls, *args, **kwds):
        if cls.__origin__ is None:
            return cls.__next_in_mro__.__new__(cls)
        else:
            origin = _gorg(cls)
            obj = cls.__next_in_mro__.__new__(origin)
            obj.__init__(*args, **kwds)
            return obj


def cast(typ, val):
    """Cast a value to a type.

    This returns the value unchanged.  To the type checker this
    signals that the return value has the designated type, but at
    runtime we intentionally don't check anything (we want this
    to be as fast as possible).
    """
    return val


def _get_defaults(func):
    """Internal helper to extract the default arguments, by name."""
    code = func.__code__
    pos_count = code.co_argcount
    arg_names = code.co_varnames
    arg_names = arg_names[:pos_count]
    defaults = func.__defaults__ or ()
    kwdefaults = func.__kwdefaults__
    res = dict(kwdefaults) if kwdefaults else {}
    pos_offset = pos_count - len(defaults)
    for name, value in zip(arg_names[pos_offset:], defaults):
        assert name not in res
        res[name] = value
    return res


def get_type_hints(obj, globalns=None, localns=None):
    """Return type hints for a function or method object.

    This is often the same as obj.__annotations__, but it handles
    forward references encoded as string literals, and if necessary
    adds Optional[t] if a default value equal to None is set.

    BEWARE -- the behavior of globalns and localns is counterintuitive
    (unless you are familiar with how eval() and exec() work).  The
    search order is locals first, then globals.

    - If no dict arguments are passed, an attempt is made to use the
      globals from obj, and these are also used as the locals.  If the
      object does not appear to have globals, an exception is raised.

    - If one dict argument is passed, it is used for both globals and
      locals.

    - If two dict arguments are passed, they specify globals and
      locals, respectively.
    """
    if getattr(obj, '__no_type_check__', None):
        return {}
    if globalns is None:
        globalns = getattr(obj, '__globals__', {})
        if localns is None:
            localns = globalns
    elif localns is None:
        localns = globalns
    defaults = _get_defaults(obj)
    hints = dict(obj.__annotations__)
    for name, value in hints.items():
        if isinstance(value, str):
            value = _ForwardRef(value)
        value = _eval_type(value, globalns, localns)
        if name in defaults and defaults[name] is None:
            value = Optional[value]
        hints[name] = value
    return hints


def no_type_check(arg):
    """Decorator to indicate that annotations are not type hints.

    The argument must be a class or function; if it is a class, it
    applies recursively to all methods defined in that class (but not
    to methods defined in its superclasses or subclasses).

    This mutates the function(s) in place.
    """
    if isinstance(arg, type):
        for obj in arg.__dict__.values():
            if isinstance(obj, types.FunctionType):
                obj.__no_type_check__ = True
    else:
        arg.__no_type_check__ = True
    return arg


def no_type_check_decorator(decorator):
    """Decorator to give another decorator the @no_type_check effect.

    This wraps the decorator with something that wraps the decorated
    function in @no_type_check.
    """

    @functools.wraps(decorator)
    def wrapped_decorator(*args, **kwds):
        func = decorator(*args, **kwds)
        func = no_type_check(func)
        return func

    return wrapped_decorator


def _overload_dummy(*args, **kwds):
    """Helper for @overload to raise when called."""
    raise NotImplementedError(
        "You should not call an overloaded function. "
        "A series of @overload-decorated functions "
        "outside a stub module should always be followed "
        "by an implementation that is not @overload-ed.")


def overload(func):
    """Decorator for overloaded functions/methods.

    In a stub file, place two or more stub definitions for the same
    function in a row, each decorated with @overload.  For example:

      @overload
      def utf8(value: None) -> None: ...
      @overload
      def utf8(value: bytes) -> bytes: ...
      @overload
      def utf8(value: str) -> bytes: ...

    In a non-stub file (i.e. a regular .py file), do the same but
    follow it with an implementation.  The implementation should *not*
    be decorated with @overload.  For example:

      @overload
      def utf8(value: None) -> None: ...
      @overload
      def utf8(value: bytes) -> bytes: ...
      @overload
      def utf8(value: str) -> bytes: ...
      def utf8(value):
          # implementation goes here
    """
    return _overload_dummy


class _ProtocolMeta(GenericMeta):
    """Internal metaclass for _Protocol.

    This exists so _Protocol classes can be generic without deriving
    from Generic.
    """

    def __instancecheck__(self, obj):
        raise TypeError("Protocols cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        if not self._is_protocol:
            # No structural checks since this isn't a protocol.
            return NotImplemented

        if self is _Protocol:
            # Every class is a subclass of the empty protocol.
            return True

        # Find all attributes defined in the protocol.
        attrs = self._get_protocol_attrs()

        for attr in attrs:
            if not any(attr in d.__dict__ for d in cls.__mro__):
                return False
        return True

    def _get_protocol_attrs(self):
        # Get all Protocol base classes.
        protocol_bases = []
        for c in self.__mro__:
            if getattr(c, '_is_protocol', False) and c.__name__ != '_Protocol':
                protocol_bases.append(c)

        # Get attributes included in protocol.
        attrs = set()
        for base in protocol_bases:
            for attr in base.__dict__.keys():
                # Include attributes not defined in any non-protocol bases.
                for c in self.__mro__:
                    if (c is not base and attr in c.__dict__ and
                            not getattr(c, '_is_protocol', False)):
                        break
                else:
                    if (not attr.startswith('_abc_') and
                            attr != '__abstractmethods__' and
                            attr != '_is_protocol' and
                            attr != '__dict__' and
                            attr != '__args__' and
                            attr != '__slots__' and
                            attr != '_get_protocol_attrs' and
                            attr != '__next_in_mro__' and
                            attr != '__parameters__' and
                            attr != '__origin__' and
                            attr != '__extra__' and
                            attr != '__module__'):
                        attrs.add(attr)

        return attrs


class _Protocol(metaclass=_ProtocolMeta):
    """Internal base class for protocol classes.

    This implements a simple-minded structural isinstance check
    (similar but more general than the one-offs in collections.abc
    such as Hashable).
    """

    __slots__ = ()

    _is_protocol = True


# Various ABCs mimicking those in collections.abc.
# A few are simply re-exported for completeness.

Hashable = collections_abc.Hashable  # Not generic.


if hasattr(collections_abc, 'Awaitable'):
    class Awaitable(Generic[T_co], extra=collections_abc.Awaitable):
        __slots__ = ()
else:
    Awaitable = None


if hasattr(collections_abc, 'AsyncIterable'):

    class AsyncIterable(Generic[T_co], extra=collections_abc.AsyncIterable):
        __slots__ = ()

    class AsyncIterator(AsyncIterable[T_co],
                        extra=collections_abc.AsyncIterator):
        __slots__ = ()

else:
    AsyncIterable = None
    AsyncIterator = None


class Iterable(Generic[T_co], extra=collections_abc.Iterable):
    __slots__ = ()


class Iterator(Iterable[T_co], extra=collections_abc.Iterator):
    __slots__ = ()


class SupportsInt(_Protocol):
    __slots__ = ()

    @abstractmethod
    def __int__(self) -> int:
        pass


class SupportsFloat(_Protocol):
    __slots__ = ()

    @abstractmethod
    def __float__(self) -> float:
        pass


class SupportsComplex(_Protocol):
    __slots__ = ()

    @abstractmethod
    def __complex__(self) -> complex:
        pass


class SupportsBytes(_Protocol):
    __slots__ = ()

    @abstractmethod
    def __bytes__(self) -> bytes:
        pass


class SupportsAbs(_Protocol[T_co]):
    __slots__ = ()

    @abstractmethod
    def __abs__(self) -> T_co:
        pass


class SupportsRound(_Protocol[T_co]):
    __slots__ = ()

    @abstractmethod
    def __round__(self, ndigits: int = 0) -> T_co:
        pass


if hasattr(collections_abc, 'Reversible'):
    class Reversible(Iterable[T_co], extra=collections_abc.Reversible):
        __slots__ = ()
else:
    class Reversible(_Protocol[T_co]):
        __slots__ = ()

        @abstractmethod
        def __reversed__(self) -> 'Iterator[T_co]':
            pass


Sized = collections_abc.Sized  # Not generic.


class Container(Generic[T_co], extra=collections_abc.Container):
    __slots__ = ()


# Callable was defined earlier.


class AbstractSet(Sized, Iterable[T_co], Container[T_co],
                  extra=collections_abc.Set):
    pass


class MutableSet(AbstractSet[T], extra=collections_abc.MutableSet):
    pass


# NOTE: Only the value type is covariant.
class Mapping(Sized, Iterable[KT], Container[KT], Generic[KT, VT_co],
              extra=collections_abc.Mapping):
    pass


class MutableMapping(Mapping[KT, VT], extra=collections_abc.MutableMapping):
    pass

if hasattr(collections_abc, 'Reversible'):
    class Sequence(Sized, Reversible[T_co], Container[T_co],
               extra=collections_abc.Sequence):
        pass
else:
    class Sequence(Sized, Iterable[T_co], Container[T_co],
                   extra=collections_abc.Sequence):
        pass


class MutableSequence(Sequence[T], extra=collections_abc.MutableSequence):
    pass


class ByteString(Sequence[int], extra=collections_abc.ByteString):
    pass


ByteString.register(type(memoryview(b'')))


class List(list, MutableSequence[T], extra=list):

    def __new__(cls, *args, **kwds):
        if _geqv(cls, List):
            raise TypeError("Type List cannot be instantiated; "
                            "use list() instead")
        return list.__new__(cls, *args, **kwds)


class Set(set, MutableSet[T], extra=set):

    def __new__(cls, *args, **kwds):
        if _geqv(cls, Set):
            raise TypeError("Type Set cannot be instantiated; "
                            "use set() instead")
        return set.__new__(cls, *args, **kwds)


class _FrozenSetMeta(GenericMeta):
    """This metaclass ensures set is not a subclass of FrozenSet.

    Without this metaclass, set would be considered a subclass of
    FrozenSet, because FrozenSet.__extra__ is collections.abc.Set, and
    set is a subclass of that.
    """

    def __subclasscheck__(self, cls):
        if issubclass(cls, Set):
            return False
        return super().__subclasscheck__(cls)


class FrozenSet(frozenset, AbstractSet[T_co], metaclass=_FrozenSetMeta,
                extra=frozenset):
    __slots__ = ()

    def __new__(cls, *args, **kwds):
        if _geqv(cls, FrozenSet):
            raise TypeError("Type FrozenSet cannot be instantiated; "
                            "use frozenset() instead")
        return frozenset.__new__(cls, *args, **kwds)


class MappingView(Sized, Iterable[T_co], extra=collections_abc.MappingView):
    pass


class KeysView(MappingView[KT], AbstractSet[KT],
               extra=collections_abc.KeysView):
    pass


class ItemsView(MappingView[Tuple[KT, VT_co]],
                AbstractSet[Tuple[KT, VT_co]],
                Generic[KT, VT_co],
                extra=collections_abc.ItemsView):
    pass


class ValuesView(MappingView[VT_co], extra=collections_abc.ValuesView):
    pass


if hasattr(contextlib, 'AbstractContextManager'):
    class ContextManager(Generic[T_co], extra=contextlib.AbstractContextManager):
        __slots__ = ()
    __all__.append('ContextManager')


class Dict(dict, MutableMapping[KT, VT], extra=dict):

    def __new__(cls, *args, **kwds):
        if _geqv(cls, Dict):
            raise TypeError("Type Dict cannot be instantiated; "
                            "use dict() instead")
        return dict.__new__(cls, *args, **kwds)

class DefaultDict(collections.defaultdict, MutableMapping[KT, VT],
                  extra=collections.defaultdict):

    def __new__(cls, *args, **kwds):
        if _geqv(cls, DefaultDict):
            raise TypeError("Type DefaultDict cannot be instantiated; "
                            "use collections.defaultdict() instead")
        return collections.defaultdict.__new__(cls, *args, **kwds)

# Determine what base class to use for Generator.
if hasattr(collections_abc, 'Generator'):
    # Sufficiently recent versions of 3.5 have a Generator ABC.
    _G_base = collections_abc.Generator
else:
    # Fall back on the exact type.
    _G_base = types.GeneratorType


class Generator(Iterator[T_co], Generic[T_co, T_contra, V_co],
                extra=_G_base):
    __slots__ = ()

    def __new__(cls, *args, **kwds):
        if _geqv(cls, Generator):
            raise TypeError("Type Generator cannot be instantiated; "
                            "create a subclass instead")
        return super().__new__(cls, *args, **kwds)


# Internal type variable used for Type[].
CT = TypeVar('CT', covariant=True, bound=type)


# This is not a real generic class.  Don't use outside annotations.
class Type(type, Generic[CT], extra=type):
    """A special construct usable to annotate class objects.

    For example, suppose we have the following classes::

      class User: ...  # Abstract base for User classes
      class BasicUser(User): ...
      class ProUser(User): ...
      class TeamUser(User): ...

    And a function that takes a class argument that's a subclass of
    User and returns an instance of the corresponding class::

      U = TypeVar('U', bound=User)
      def new_user(user_class: Type[U]) -> U:
          user = user_class()
          # (Here we could write the user object to a database)
          return user

      joe = new_user(BasicUser)

    At this point the type checker knows that joe has type BasicUser.
    """


def NamedTuple(typename, fields):
    """Typed version of namedtuple.

    Usage::

        Employee = typing.NamedTuple('Employee', [('name', str), 'id', int)])

    This is equivalent to::

        Employee = collections.namedtuple('Employee', ['name', 'id'])

    The resulting class has one extra attribute: _field_types,
    giving a dict mapping field names to types.  (The field names
    are in the _fields attribute, which is part of the namedtuple
    API.)
    """
    fields = [(n, t) for n, t in fields]
    cls = collections.namedtuple(typename, [n for n, t in fields])
    cls._field_types = dict(fields)
    # Set the module to the caller's module (otherwise it'd be 'typing').
    try:
        cls.__module__ = sys._getframe(1).f_globals.get('__name__', '__main__')
    except (AttributeError, ValueError):
        pass
    return cls


def NewType(name, tp):
    """NewType creates simple unique types with almost zero
    runtime overhead. NewType(name, tp) is considered a subtype of tp
    by static type checkers. At runtime, NewType(name, tp) returns
    a dummy function that simply returns its argument. Usage::

        UserId = NewType('UserId', int)

        def name_by_id(user_id: UserId) -> str:
            ...

        UserId('user')          # Fails type check

        name_by_id(42)          # Fails type check
        name_by_id(UserId(42))  # OK

        num = UserId(5) + 1     # type: int
    """

    def new_type(x):
        return x

    new_type.__name__ = name
    new_type.__supertype__ = tp
    return new_type


# Python-version-specific alias (Python 2: unicode; Python 3: str)
Text = str


# Constant that's True when type checking, but False here.
TYPE_CHECKING = False


class IO(Generic[AnyStr]):
    """Generic base class for TextIO and BinaryIO.

    This is an abstract, generic version of the return of open().

    NOTE: This does not distinguish between the different possible
    classes (text vs. binary, read vs. write vs. read/write,
    append-only, unbuffered).  The TextIO and BinaryIO subclasses
    below capture the distinctions between text vs. binary, which is
    pervasive in the interface; however we currently do not offer a
    way to track the other distinctions in the type system.
    """

    __slots__ = ()

    @abstractproperty
    def mode(self) -> str:
        pass

    @abstractproperty
    def name(self) -> str:
        pass

    @abstractmethod
    def close(self) -> None:
        pass

    @abstractmethod
    def closed(self) -> bool:
        pass

    @abstractmethod
    def fileno(self) -> int:
        pass

    @abstractmethod
    def flush(self) -> None:
        pass

    @abstractmethod
    def isatty(self) -> bool:
        pass

    @abstractmethod
    def read(self, n: int = -1) -> AnyStr:
        pass

    @abstractmethod
    def readable(self) -> bool:
        pass

    @abstractmethod
    def readline(self, limit: int = -1) -> AnyStr:
        pass

    @abstractmethod
    def readlines(self, hint: int = -1) -> List[AnyStr]:
        pass

    @abstractmethod
    def seek(self, offset: int, whence: int = 0) -> int:
        pass

    @abstractmethod
    def seekable(self) -> bool:
        pass

    @abstractmethod
    def tell(self) -> int:
        pass

    @abstractmethod
    def truncate(self, size: int = None) -> int:
        pass

    @abstractmethod
    def writable(self) -> bool:
        pass

    @abstractmethod
    def write(self, s: AnyStr) -> int:
        pass

    @abstractmethod
    def writelines(self, lines: List[AnyStr]) -> None:
        pass

    @abstractmethod
    def __enter__(self) -> 'IO[AnyStr]':
        pass

    @abstractmethod
    def __exit__(self, type, value, traceback) -> None:
        pass


class BinaryIO(IO[bytes]):
    """Typed version of the return of open() in binary mode."""

    __slots__ = ()

    @abstractmethod
    def write(self, s: Union[bytes, bytearray]) -> int:
        pass

    @abstractmethod
    def __enter__(self) -> 'BinaryIO':
        pass


class TextIO(IO[str]):
    """Typed version of the return of open() in text mode."""

    __slots__ = ()

    @abstractproperty
    def buffer(self) -> BinaryIO:
        pass

    @abstractproperty
    def encoding(self) -> str:
        pass

    @abstractproperty
    def errors(self) -> str:
        pass

    @abstractproperty
    def line_buffering(self) -> bool:
        pass

    @abstractproperty
    def newlines(self) -> Any:
        pass

    @abstractmethod
    def __enter__(self) -> 'TextIO':
        pass


class io:
    """Wrapper namespace for IO generic classes."""

    __all__ = ['IO', 'TextIO', 'BinaryIO']
    IO = IO
    TextIO = TextIO
    BinaryIO = BinaryIO

io.__name__ = __name__ + '.io'
sys.modules[io.__name__] = io


Pattern = _TypeAlias('Pattern', AnyStr, type(stdlib_re.compile('')),
                     lambda p: p.pattern)
Match = _TypeAlias('Match', AnyStr, type(stdlib_re.match('', '')),
                   lambda m: m.re.pattern)


class re:
    """Wrapper namespace for re type aliases."""

    __all__ = ['Pattern', 'Match']
    Pattern = Pattern
    Match = Match

re.__name__ = __name__ + '.re'
sys.modules[re.__name__] = re
