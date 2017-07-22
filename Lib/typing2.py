import abc
from abc import abstractmethod, abstractproperty
import collections
import contextlib
import functools
import re as stdlib_re  # Avoid confusion with the re we export.
import sys
import types
import collections.abc
try:
    from types import WrapperDescriptorType, MethodWrapperType, MethodDescriptorType
except ImportError:
    WrapperDescriptorType = type(object.__init__)
    MethodWrapperType = type(object().__str__)
    MethodDescriptorType = type(str.join)


# Please keep __all__ alphabetized within each category.
__all__ = [
    # Super-special typing primitives.
    'Any',
    'Callable',
    'ClassVar',
    'Generic',
    'Optional',
    'Tuple',
    'Type',
    'TypeVar',
    'Union',

    # ABCs (from collections.abc).
    'AbstractSet',  # collections.abc.Set.
    'ByteString',
    'Container',
    'ContextManager',
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
    'Awaitable',
    'AsyncIterator',
    'AsyncIterable',
    'Coroutine',
    'Collection',
    'AsyncGenerator',
    #AsyncContextManager

    # Structural checks, a.k.a. protocols.
    'Reversible',
    'SupportsAbs',
    'SupportsBytes',
    'SupportsComplex',
    'SupportsFloat',
    'SupportsInt',
    'SupportsRound',

    # Concrete collection types.
    'Counter',
    'Deque',
    'Dict',
    'DefaultDict',
    'List',
    'Set',
    'FrozenSet',
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

###########################
# Internal helper functions
###########################

def _trim_name(nm):
    whitelist = ('_TypingBase', '_FinalTypingBase')
    if nm.startswith('_') and nm not in whitelist:
        nm = nm[1:]
    return nm


def _type_repr(obj):
    """Return the repr() of an object, special-casing types (internal helper).

    If obj is a type, we return a shorter version than the default
    type.__repr__, based on the module and qualified name, which is
    typically enough to uniquely identify a type.  For everything
    else, we fall back on repr(obj).
    """
    if isinstance(obj, type):
        if obj.__module__ == 'builtins':
            return obj.__qualname__
        qname = obj.__qualname__
        if obj.__module__ == 'typing2':
            qname = _trim_name(qname)
        return '%s.%s' % (obj.__module__, qname)
    if obj is ...:
        return('...')
    if isinstance(obj, types.FunctionType):
        return obj.__name__
    return repr(obj)


def _type_vars(types):
    tvars = []
    for t in types:
        if isinstance(t, _TypingBase):
            t._get_type_vars(tvars)
        elif isinstance(t, list):
            tvars.extend(_type_vars(t))
    return tuple(tvars)


def _eval_type(t, globalns, localns):
    if isinstance(t, _TypingBase):
        return t._eval_type(globalns, localns)
    return t


def _type_check(arg, msg):
    """Check that the argument is a type, and return it (internal helper).
    As a special case, accept None and return type(None) instead.
    The msg argument is a human-readable error message, e.g.

        "Union[arg, ...]: arg should be a type."

    We append the repr() of the actual value (truncated to 100 chars).
    """
    if arg is None:
        return type(None)
    if isinstance(arg, str):
        arg = _ForwardRef(arg)
    if (
        isinstance(arg, _TypingBase) and type(arg).__name__ == '_ClassVar' or
        not isinstance(arg, (type, _TypingBase)) and not callable(arg)
    ):
        raise TypeError(msg + " Got %.100r." % (arg,))
    # Bare Union etc. are not valid as type arguments
    if (
        type(arg).__name__ in ('_Union', '_Optional') and
        not getattr(arg, '__origin__', None) or
        isinstance(arg, _TypingBase) and arg in (Generic, Protocol)
    ):
        raise TypeError("Plain %s is not valid as type argument" % arg)
    return arg


# Special typing constructs Union, Optional, Generic, Callable and Tuple
# use three special attributes for internal bookkeeping of generic types:
# * __parameters__ is a tuple of unique free type parameters of a generic
#   type, for example, Dict[T, T].__parameters__ == (T,);
# * __origin__ keeps a reference to a type that was subscripted,
#   e.g., Union[T, int].__origin__ == Union;
# * __args__ is a tuple of all arguments used in subscripting,
#   e.g., Dict[T, int].__args__ == (T, int).


def _update_args(cls, args):
    """An internal helper function: calculate substitution tree
    for generic cls after replacing its type parameters with
    substitutions in tvars -> args (if any).
    Repeat the same following __origin__'s.

    Return a list of arguments with all possible substitutions
    performed. Arguments that are generic classes themselves are represented
    as tuples (so that no new classes are created by this function).
    For example: _subs_tree(List[Tuple[int, T]][str]) == [(Tuple, int, str)]
    """

    return GenericType(cls.__origin__, args)


def _remove_dups_flatten(parameters):
    """An internal helper for Union creation and substitution: flatten Union's
    among parameters, then remove duplicates and strict subclasses.
    """

    # Flatten out Union[Union[...], ...].
    params = []
    for p in parameters:
        if isinstance(p, _Union) and p.__origin__ is Union:
            params.extend(p.__args__)
        elif isinstance(p, tuple) and len(p) > 0 and p[0] is Union:
            params.extend(p[1:])
        else:
            params.append(p)
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
    # If object is present it will be sole survivor among proper classes.
    # Never discard type variables.
    # (In particular, Union[str, AnyStr] != AnyStr.)
    all_params = set(params)
    for t1 in params:
        if not isinstance(t1, type):
            continue
        if any(isinstance(t2, type) and issubclass(t1, t2)
               for t2 in all_params - {t1}
               if not (isinstance(t2, GenericMeta) and
                       t2.__origin__ is not None)):
            all_params.remove(t1)
    return tuple(t for t in params if t in all_params)


def _check_generic(cls, parameters):
    # Check correct count for parameters of a generic cls (internal helper).
    if not cls.__parameters__:
        raise TypeError("%s is not a generic class" % repr(cls))
    alen = len(parameters)
    elen = len(cls.__parameters__)
    if alen != elen:
        raise TypeError("Too %s parameters for %s; actual %s, expected %s" %
                        ("many" if alen > elen else "few", repr(cls), alen, elen))


###########################################################
# Special bases for everything: general, and for singletons
###########################################################

class _TypingBase:
    """Internal indicator of special typing constructs."""

    __slots__ = ('__weakref__')

    def __new__(cls, *args, **kwds):
        """Constructor.

        This only exists to give a better error message in case
        someone tries to subclass a special typing object (not a good idea).
        """
        if (len(args) == 3 and
                isinstance(args[0], str) and
                isinstance(args[1], tuple)):
            # Close enough.
            raise TypeError("Cannot subclass %r" % _type_repr(cls))
        return super().__new__(cls)

    def _eval_type(self, globalns, localns):
        """Override this in subclasses to interpret forward references.

        For example, List['C'] is internally stored as
        List[_ForwardRef('C')], which should evaluate to List[C],
        where C is an object found in globalns or localns (searching
        localns first, of course).
        """
        return self

    def _get_type_vars(self, tvars):
        pass

    def __repr__(self):
        cls = type(self)
        qname = _trim_name(cls.__qualname__)
        return '%s.%s' % (cls.__module__, qname)

    def __call__(self, *args, **kwds):
        raise TypeError("Cannot instantiate %r" % type(self))

    def __instancecheck__(self, obj):
        raise TypeError(f"{_type_repr(self)} cannot be used with isinstance().")

    def __subclasscheck__(self, cls):
        raise TypeError(f"{_type_repr(self)} cannot be used with issubclass().")


class _FinalTypingBase(_TypingBase):
    """Internal mix-in class to prevent instantiation.

    Prevents instantiation unless _root=True is given in class call.
    It is used to create pseudo-singleton instances Any, Union, Optional, etc.
    """

    __slots__ = ()

    def __new__(cls, *args, _root=False, **kwds):
        self = super().__new__(cls, *args, **kwds)
        if _root is True:
            return self
        raise TypeError("Cannot instantiate %r" % cls)

    def __reduce__(self):
        return _trim_name(type(self).__name__)


########################################################################
# Two non-singleton classes _ForwardRef (internal), and TypeVar (public)
########################################################################


class _ForwardRef(_TypingBase):
    """Internal wrapper to hold a forward reference."""

    __slots__ = ('__forward_arg__', '__forward_code__',
                 '__forward_evaluated__', '__forward_value__')

    def __init__(self, arg):
        super().__init__(arg)
        if not isinstance(arg, str):
            raise TypeError('Forward reference must be a string -- got %r' % (arg,))
        try:
            code = compile(arg, '<string>', 'eval')
        except SyntaxError:
            raise SyntaxError('Forward reference must be an expression -- got %r' %
                              (arg,))
        self.__forward_arg__ = arg
        self.__forward_code__ = code
        self.__forward_evaluated__ = False
        self.__forward_value__ = None

    def _eval_type(self, globalns, localns):
        if not self.__forward_evaluated__ or localns is not globalns:
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

    def __eq__(self, other):
        if not isinstance(other, _ForwardRef):
            return NotImplemented
        return (self.__forward_arg__ == other.__forward_arg__ and
                self.__forward_value__ == other.__forward_value__)

    def __hash__(self):
        return hash((self.__forward_arg__, self.__forward_value__))

    def __repr__(self):
        return '%r*' % (self.__forward_arg__,)


class TypeVar(_TypingBase):
    """Type variable.

    Usage::

      T = TypeVar('T')  # Can be anything
      A = TypeVar('A', str, bytes)  # Must be str or bytes

    Type variables exist primarily for the benefit of static type
    checkers.  They serve as the parameters for generic types as well
    as for generic function definitions.  See class Generic for more
    information on generic types.  Generic functions work as follows:

      def repeat(x: T, n: int) -> List[T]:
          '''Return a list containing n references to x.'''
          return [x]*n

      def longest(x: A, y: A) -> A:
          '''Return the longest of two strings.'''
          return x if len(x) >= len(y) else y

    The latter example's signature is essentially the overloading
    of (str, str) -> str and (bytes, bytes) -> bytes.  Also note
    that if the arguments are instances of some subclass of str,
    the return type is still plain str.

    At runtime, isinstance(x, T) and issubclass(C, T) will raise TypeError.

    Type variables defined with covariant=True or contravariant=True
    can be used do declare covariant or contravariant generic types.
    See PEP 484 for more details. By default generic types are invariant
    in all type variables.

    Type variables can be introspected. e.g.:

      T.__name__ == 'T'
      T.__constraints__ == ()
      T.__covariant__ == False
      T.__contravariant__ = False
      A.__constraints__ == (str, bytes)
    """

    __slots__ = ('__name__', '__bound__', '__constraints__',
                 '__covariant__', '__contravariant__')

    def __init__(self, name, *constraints, bound=None,
                 covariant=False, contravariant=False):
        self.__name__ = name
        if covariant and contravariant:
            raise ValueError("Bivariant types are not supported.")
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

####################################################
# Two non-subscriptable singletons: Any and NoReturn
####################################################

class _Any(_FinalTypingBase):
    """Special type indicating an unconstrained type.

    - Any is compatible with every type.
    - Any assumed to have all methods.
    - All values assumed to be instances of Any.

    Note that all the above statements are true from the point of view of
    static type checkers. At runtime, Any should not be used with instance
    or class checks.
    """

    __slots__ = ()


Any = _Any(_root=True)


class _NoReturn(_FinalTypingBase):
    """Special type indicating functions that never return.
    Example::

      from typing import NoReturn

      def stop() -> NoReturn:
          raise Exception('no way')

    This type is invalid in other positions, e.g., ``List[NoReturn]``
    will fail in static type checkers.
    """

    __slots__ = ()


NoReturn = _NoReturn(_root=True)

##################################################################
# The main class: represents subscriptions of all singletons below
##################################################################

class GenericType(_TypingBase):

    __slots__ = ('__origin__', '__args__', '__parameters__',
                 '_name', '_call')

    def __init__(self, origin, args, name=None, call=True):
        self.__origin__ = origin
        self.__args__ = args
        self.__parameters__ = ()
        self._call = call
        if name:
            self._name = name
        elif isinstance(origin, type):
            self._name = origin.__qualname__
        else:
            self._name = repr(origin)
    def __call__(self, *args, **kwargs):
        if not self._call:
            raise TypeError(f'{self} is not callable')
        return self.__origin__(*args, **kwargs)
    def __base_subclass__(self, bases):
        return self.__origin__
    def __getitem__(self, args):
        return _update_args(self, args)
    def _eval_type(self):
        return self
    def __repr__(self):
        args = ', '.join([_type_repr(a) for a in self.__args__])
        return self._name + f'[{args}]'


######################################################################
# Subscriptable non-subclassable singletons: ClassVar, Union, Optional
######################################################################

class _ClassVar(_FinalTypingBase):
    """Special type construct to mark class variables.

    An annotation wrapped in ClassVar indicates that a given
    attribute is intended to be used as a class variable and
    should not be set on instances of that class. Usage::

      class Starship:
          stats: ClassVar[Dict[str, int]] = {} # class variable
          damage: int = 10                     # instance variable

    ClassVar accepts only types and cannot be further subscribed.

    Note that ClassVar is not a class itself, and should not
    be used with isinstance() or issubclass().
    """

    __slots__ = ()

    def __getitem__(self, item):
        return GenericType(self, [item])


ClassVar = _ClassVar(_root=True)


class _Union(_FinalTypingBase):
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

    - Similar for object::

        Union[int, object] == object

    - You cannot subclass or instantiate a union.

    - You can use Optional[X] as a shorthand for Union[X, None].
    """

    __slots__ = ()

    def __getitem__(self, parameters):
        if parameters == ():
            raise TypeError("Cannot take a Union of no types.")
        if not isinstance(parameters, tuple):
            parameters = (parameters,)
        msg = "Union[arg, ...]: each arg must be a type."
        parameters = tuple(_type_check(p, msg) for p in parameters)
        return GenericType(self, parameters)

Union = _Union(_root=True)


class _Optional(_FinalTypingBase):
    """Optional type.

    Optional[X] is equivalent to Union[X, None].
    """

    __slots__ = ()

    def __getitem__(self, arg):
        arg = _type_check(arg, "Optional[t] requires a single type.")
        return Union[arg, type(None)]


Optional = _Optional(_root=True)


#############################################################
# Subclassable singletons: Generic, Callable, Tuple, Protocol
#############################################################

class Generic(_FinalTypingBase):
    """Abstract base class for generic types.

    A generic type is typically declared by inheriting from
    this class parameterized with one or more type variables.
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

    def __class_getitem__(self, args):
        return GenericType(self, args)


class Tuple(_FinalTypingBase):
    """Tuple type; Tuple[X, Y] is the cross-product type of X and Y.

    Example: Tuple[T1, T2] is a tuple of two elements corresponding
    to type variables T1 and T2.  Tuple[int, float, str] is a tuple
    of an int, a float and a string.

    To specify a variable-length tuple of homogeneous type, use Tuple[T, ...].
    """

    __slots__ = ()

    def __class_getitem__(self, args):
        return GenericType(tuple, args, name='Tuple', call=False)


class Callable(_FinalTypingBase):
    """Callable type; Callable[[int], str] is a function of (int) -> str.

    The subscription syntax must always be used with exactly two
    values: the argument list and the return type.  The argument list
    must be a list of types or ellipsis; the return type must be a single type.

    There is no syntax to indicate optional or keyword arguments,
    such function types are rarely used as callback types.
    """

    __slots__ = ()

    def __class_getitem__(self, args):
        return GenericType(collections.abc.Callable, args)


class Protocol(_FinalTypingBase):
    """Internal base class for protocol classes.

    This implements a simple-minded structural issubclass check
    (similar but more general than the one-offs in collections.abc
    such as Hashable).
    """

    __slots__ = ()

    def __class_getitem__(self, args):
        return GenericType(self, args)


#########################################
# Fancy classes: NamedTuple and TypedDict
#########################################

def _make_nmtuple(name, types):
    msg = "NamedTuple('Name', [(f0, t0), (f1, t1), ...]); each t must be a type"
    types = [(n, _type_check(t, msg)) for n, t in types]
    nm_tpl = collections.namedtuple(name, [n for n, t in types])
    # Prior to PEP 526, only _field_types attribute was assigned.
    # Now, both __annotations__ and _field_types are used to maintain compatibility.
    nm_tpl.__annotations__ = nm_tpl._field_types = collections.OrderedDict(types)
    try:
        nm_tpl.__module__ = sys._getframe(2).f_globals.get('__name__', '__main__')
    except (AttributeError, ValueError):
        pass
    return nm_tpl


_PY36 = sys.version_info[:2] >= (3, 6)

# attributes prohibited to set in NamedTuple class syntax
_prohibited = ('__new__', '__init__', '__slots__', '__getnewargs__',
               '_fields', '_field_defaults', '_field_types',
               '_make', '_replace', '_asdict', '_source')

_special = ('__module__', '__name__', '__qualname__', '__annotations__')


class NamedTupleMeta(type):

    def __new__(cls, typename, bases, ns):
        if ns.get('_root', False):
            return super().__new__(cls, typename, bases, ns)
        if not _PY36:
            raise TypeError("Class syntax for NamedTuple is only supported"
                            " in Python 3.6+")
        types = ns.get('__annotations__', {})
        nm_tpl = _make_nmtuple(typename, types.items())
        defaults = []
        defaults_dict = {}
        for field_name in types:
            if field_name in ns:
                default_value = ns[field_name]
                defaults.append(default_value)
                defaults_dict[field_name] = default_value
            elif defaults:
                raise TypeError("Non-default namedtuple field {field_name} cannot "
                                "follow default field(s) {default_names}"
                                .format(field_name=field_name,
                                        default_names=', '.join(defaults_dict.keys())))
        nm_tpl.__new__.__defaults__ = tuple(defaults)
        nm_tpl._field_defaults = defaults_dict
        # update from user namespace without overriding special namedtuple attributes
        for key in ns:
            if key in _prohibited:
                raise AttributeError("Cannot overwrite NamedTuple attribute " + key)
            elif key not in _special and key not in nm_tpl._fields:
                setattr(nm_tpl, key, ns[key])
        return nm_tpl


class NamedTuple(metaclass=NamedTupleMeta):
    """Typed version of namedtuple.

    Usage in Python versions >= 3.6::

        class Employee(NamedTuple):
            name: str
            id: int

    This is equivalent to::

        Employee = collections.namedtuple('Employee', ['name', 'id'])

    The resulting class has extra __annotations__ and _field_types
    attributes, giving an ordered dict mapping field names to types.
    __annotations__ should be preferred, while _field_types
    is kept to maintain pre PEP 526 compatibility. (The field names
    are in the _fields attribute, which is part of the namedtuple
    API.) Alternative equivalent keyword syntax is also accepted::

        Employee = NamedTuple('Employee', name=str, id=int)

    In Python versions <= 3.5 use::

        Employee = NamedTuple('Employee', [('name', str), ('id', int)])
    """
    _root = True

    def __new__(self, typename, fields=None, **kwargs):
        if kwargs and not _PY36:
            raise TypeError("Keyword syntax for NamedTuple is only supported"
                            " in Python 3.6+")
        if fields is None:
            fields = kwargs.items()
        elif kwargs:
            raise TypeError("Either list of fields or keywords"
                            " can be provided to NamedTuple, not both")
        return _make_nmtuple(typename, fields)


#################################
# Public functions (with helpers)
#################################

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
    try:
        code = func.__code__
    except AttributeError:
        # Some built-in functions don't have __code__, __defaults__, etc.
        return {}
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


_allowed_types = (types.FunctionType, types.BuiltinFunctionType,
                  types.MethodType, types.ModuleType,
                  WrapperDescriptorType, MethodWrapperType, MethodDescriptorType)


def get_type_hints(obj, globalns=None, localns=None):
    """Return type hints for an object.

    This is often the same as obj.__annotations__, but it handles
    forward references encoded as string literals, and if necessary
    adds Optional[t] if a default value equal to None is set.

    The argument may be a module, class, method, or function. The annotations
    are returned as a dictionary. For classes, annotations include also
    inherited members.

    TypeError is raised if the argument is not of a type that can contain
    annotations, and an empty dictionary is returned if no annotations are
    present.

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
    # Classes require a special treatment.
    if isinstance(obj, type):
        hints = {}
        for base in reversed(obj.__mro__):
            ann = base.__dict__.get('__annotations__', {})
            for name, value in ann.items():
                if value is None:
                    value = type(None)
                if isinstance(value, str):
                    value = _ForwardRef(value)
                value = _eval_type(value, globalns, localns)
                hints[name] = value
        return hints
    hints = getattr(obj, '__annotations__', None)
    if hints is None:
        # Return empty annotations for something that _could_ have them.
        if isinstance(obj, _allowed_types):
            return {}
        else:
            raise TypeError('{!r} is not a module, class, method, '
                            'or function.'.format(obj))
    defaults = _get_defaults(obj)
    hints = dict(hints)
    for name, value in hints.items():
        if value is None:
            value = type(None)
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
    applies recursively to all methods and classes defined in that class
    (but not to methods defined in its superclasses or subclasses).

    This mutates the function(s) or class(es) in place.
    """
    if isinstance(arg, type):
        arg_attrs = arg.__dict__.copy()
        for attr, val in arg.__dict__.items():
            if val in arg.__bases__ + (arg,):
                arg_attrs.pop(attr)
        for obj in arg_attrs.values():
            if isinstance(obj, types.FunctionType):
                obj.__no_type_check__ = True
            if isinstance(obj, type):
                no_type_check(obj)
    try:
        arg.__no_type_check__ = True
    except TypeError:  # built-in classes
        pass
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


######################################################################
# Various generics mimicking those in collections.abc and
# additional protocols. A few are simply re-exported for completeness.
######################################################################

Hashable = collections.abc.Hashable  # Not generic.

Awaitable = GenericType(collections.abc.Awaitable, [T_co])
Coroutine = GenericType(collections.abc.Coroutine, [T_co, T_contra, V_co])
AsyncIterable = GenericType(collections.abc.AsyncIterable, [T_co])
AsyncIterator = GenericType(collections.abc.AsyncIterator, [T_co])

Iterable = GenericType(collections.abc.Iterable, [T_co])
Iterator = GenericType(collections.abc.Iterator, [T_co])

class SupportsInt(Protocol):
    __slots__ = ()

    @abstractmethod
    def __int__(self) -> int:
        pass


class SupportsFloat(Protocol):
    __slots__ = ()

    @abstractmethod
    def __float__(self) -> float:
        pass


class SupportsComplex(Protocol):
    __slots__ = ()

    @abstractmethod
    def __complex__(self) -> complex:
        pass


class SupportsBytes(Protocol):
    __slots__ = ()

    @abstractmethod
    def __bytes__(self) -> bytes:
        pass


class SupportsAbs(Protocol[T_co]):
    __slots__ = ()

    @abstractmethod
    def __abs__(self) -> T_co:
        pass


class SupportsRound(Protocol[T_co]):
    __slots__ = ()

    @abstractmethod
    def __round__(self, ndigits: int = 0) -> T_co:
        pass


Reversible = GenericType(collections.abc.Reversible, [T_co])
Sized = collections.abc.Sized  # Not generic.
Container = GenericType(collections.abc.Container, [T_co])
Collection = GenericType(collections.abc.Collection, [T_co])

# Callable was defined earlier.

AbstractSet = GenericType(collections.abc.Set, [T_co])
MutableSet = GenericType(collections.abc.MutableSet, [T])
Set = GenericType(set, [T], name='Set')
FrozenSet = GenericType(frozenset, [T_co], name='FrozenSet')

# NOTE: It is only covariant in the value type.
Mapping = GenericType(collections.abc.Mapping, [KT, VT_co])
MutableMapping = GenericType(collections.abc.MutableMapping, [KT, VT])
Dict = GenericType(dict, [KT, VT], call=False, name='Dict')
DefaultDict = GenericType(collections.defaultdict, [KT, VT], call=False, name='DefaultDict')
Counter = GenericType(collections.Counter, [T], call=False)
ChainMap = GenericType(collections.ChainMap, [KT, VT], call=False)

Sequence = GenericType(collections.abc.Sequence, [T_co])
MutableSequence = GenericType(collections.abc.MutableSequence, [T])
ByteString = collections.abc.ByteString  # Not generic.
List = GenericType(list, [T], name='List')
Deque = GenericType(collections.deque, [T], name='Deque')

MappingView = GenericType(collections.abc.MappingView, [T_co])
KeysView = GenericType(collections.abc.KeysView, [KT])
ItemsView = GenericType(collections.abc.ItemsView, [KT, VT_co])
ValuesView = GenericType(collections.abc.ValuesView, [VT_co])

ContextManager = GenericType(contextlib.AbstractContextManager, [T_co])
if hasattr(contextlib, 'AbstractAsyncContextManager'):
    AsyncContextManager = GenericType(contextlib.AbstractAsyncContextManager, [T_co])
    __all__.append('AsyncContextManager')

Generator = GenericType(collections.abc.Generator, [T_co, T_contra, V_co])
AsyncGenerator = GenericType(collections.abc.AsyncGenerator, [T_co, T_contra])

# Internal type variable used for Type[].
CT_co = TypeVar('CT_co', covariant=True, bound=type)
# This is not a real generic class.  Don't use outside annotations.
Type = GenericType(type, [CT_co], name='Type')
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

##################
# Public constants
##################

# Python-version-specific alias (Python 2: unicode; Python 3: str)
Text = str


# Constant that's True when type checking, but False here.
TYPE_CHECKING = False

#########################################
# Special io types and re generic aliases
#########################################


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
    def errors(self) -> Optional[str]:
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


Pattern = GenericType(type(stdlib_re.compile('')), [AnyStr], name='Pattern')
Match = GenericType(type(stdlib_re.match('', '')), [AnyStr], name='Match')


class re:
    """Wrapper namespace for re type aliases."""

    __all__ = ['Pattern', 'Match']
    Pattern = Pattern
    Match = Match


re.__name__ = __name__ + '.re'
sys.modules[re.__name__] = re
