import contextlib
import functools
import importlib
import inspect
import unittest
import unittest.mock
from collections import namedtuple
from functools import partial
from test.support import import_helper
from types import MappingProxyType


def try_import_module(name, default=None):
    try:
        return importlib.import_module(name)
    except ImportError:
        return None


CANONICAL_DIGEST_NAMES = frozenset((
    'md5', 'sha1',
    'sha224', 'sha256', 'sha384', 'sha512',
    'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    'shake_128', 'shake_256',
    'blake2s', 'blake2b',
))

NON_HMAC_DIGEST_NAMES = frozenset({
    'shake_128', 'shake_256',
    'blake2s', 'blake2b',
})


class HashAPI(namedtuple("HashAPI", "builtin openssl hashlib")):

    @property
    def builtin_module_name(self):
        return self.builtin.split(".", maxsplit=1)[0]

    @property
    def builtin_method_name(self):
        return self.builtin.split(".", maxsplit=1)[1]

    def fullname(self, impl):
        match impl:
            case "builtin":
                return self.builtin
            case "openssl":
                return f"_hashlib.{self.openssl}" if self.openssl else None
            case "hashlib":
                return f"hashlib.{self.hashlib}" if self.hashlib else None
            case _:
                raise AssertionError(f"unknown implementation: {impl}")


# Mapping from a "canonical" name to a pair (HACL*, _hashlib.*, hashlib.*)
# constructors. If the constructor name is None, then this means that the
# algorithm can only be used by the "agile" new() interfaces.
_EXPLICIT_CONSTRUCTORS = MappingProxyType({
    "md5": HashAPI("_md5.md5", "openssl_md5", "md5"),
    "sha1": HashAPI("_sha1.sha1", "openssl_sha1", "sha1"),
    "sha224": HashAPI("_sha2.sha224", "openssl_sha224", "sha224"),
    "sha256": HashAPI("_sha2.sha256", "openssl_sha256", "sha256"),
    "sha384": HashAPI("_sha2.sha384", "openssl_sha384", "sha384"),
    "sha512": HashAPI("_sha2.sha512", "openssl_sha512", "sha512"),
    "sha3_224": HashAPI("_sha3.sha3_224", "openssl_sha3_224", "sha3_224"),
    "sha3_256": HashAPI("_sha3.sha3_256", "openssl_sha3_256", "sha3_256"),
    "sha3_384": HashAPI("_sha3.sha3_384", "openssl_sha3_384", "sha3_384"),
    "sha3_512": HashAPI("_sha3.sha3_512", "openssl_sha3_512", "sha3_512"),
    "shake_128": HashAPI("_sha3.shake_128", "openssl_shake_128", "shake_128"),
    "shake_256": HashAPI("_sha3.shake_256", "openssl_shake_256", "shake_256"),
    "blake2s": HashAPI("_blake2.blake2s", None, "blake2s"),
    "blake2b": HashAPI("_blake2.blake2b", None, "blake2b"),
})
assert _EXPLICIT_CONSTRUCTORS.keys() == CANONICAL_DIGEST_NAMES

_EXPLICIT_HMAC_CONSTRUCTORS = {
    name: f'_hmac.compute_{name}' for name in (
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
        'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    )
}
_EXPLICIT_HMAC_CONSTRUCTORS['shake_128'] = None
_EXPLICIT_HMAC_CONSTRUCTORS['shake_256'] = None
# Strictly speaking, HMAC-BLAKE is meaningless as BLAKE2 is already a
# keyed hash function. However, as it's exposed by HACL*, we test it.
_EXPLICIT_HMAC_CONSTRUCTORS['blake2s'] = '_hmac.compute_blake2s_32'
_EXPLICIT_HMAC_CONSTRUCTORS['blake2b'] = '_hmac.compute_blake2b_32'
_EXPLICIT_HMAC_CONSTRUCTORS = MappingProxyType(_EXPLICIT_HMAC_CONSTRUCTORS)
assert _EXPLICIT_HMAC_CONSTRUCTORS.keys() == CANONICAL_DIGEST_NAMES


def _decorate_func_or_class(decorator_func, func_or_class):
    if not isinstance(func_or_class, type):
        return decorator_func(func_or_class)

    decorated_class = func_or_class
    setUpClass = decorated_class.__dict__.get('setUpClass')
    if setUpClass is None:
        def setUpClass(cls):
            super(decorated_class, cls).setUpClass()
        setUpClass.__qualname__ = decorated_class.__qualname__ + '.setUpClass'
        setUpClass.__module__ = decorated_class.__module__
    else:
        setUpClass = setUpClass.__func__
    setUpClass = classmethod(decorator_func(setUpClass))
    decorated_class.setUpClass = setUpClass
    return decorated_class


def _chain_decorators(decorators):
    """Obtain a decorator by chaining multiple decorators.

    The decorators are applied in the order they are given.
    """
    def decorator_func(func):
        return functools.reduce(lambda w, deco: deco(w), decorators, func)
    return partial(_decorate_func_or_class, decorator_func)


def _ensure_wrapper_signature(wrapper, wrapped):
    """Ensure that a wrapper has the same signature as the wrapped function.

    This is used to guarantee that a TypeError raised due to a bad API call
    is raised consistently (using variadic signatures would hide such errors).
    """
    try:
        wrapped_sig = inspect.signature(wrapped)
    except ValueError:  # built-in signature cannot be found
        return

    wrapper_sig = inspect.signature(wrapper)
    if wrapped_sig != wrapper_sig:
        fullname = f"{wrapped.__module__}.{wrapped.__qualname__}"
        raise AssertionError(
            f"signature for {fullname}() is incorrect:\n"
            f"  expect: {wrapped_sig}\n"
            f"  actual: {wrapper_sig}"
        )


def _requires_module(name):
    def decorator_func(func):
        module = try_import_module(name, missing := object())
        return unittest.skipIf(module is missing, f"requires {name}")(func)
    return partial(_decorate_func_or_class, decorator_func)


requires_hashlib = partial(_requires_module, "_hashlib")
requires_builtin_hmac = partial(_requires_module, "_hmac")


class SkipNoHash(unittest.SkipTest):

    def __init__(self, digestname, implementation=None):
        parts = ["missing", implementation, f"hash algorithm: {digestname!r}"]
        super().__init__(" ".join(filter(None, parts)))


def _hashlib_new(digestname, openssl, /, **kwargs):
    """Check availability of [hashlib|_hashlib].new(digestname, **kwargs).

    If *openssl* is True, module is "_hashlib" (C extension module),
    otherwise it is "hashlib" (pure Python interface).

    The constructor function is returned, or SkipTest is raised if none exists.
    """
    # Re-import 'hashlib' in case it was mocked, but propagate
    # exceptions as it should be unconditionally available.
    hashlib = importlib.import_module("hashlib")
    # re-import '_hashlib' in case it was mocked
    _hashlib = try_import_module("_hashlib")
    mod = _hashlib if openssl and _hashlib is not None else hashlib
    constructor = partial(mod.new, digestname, **kwargs)
    try:
        constructor()
    except ValueError:
        implementation = f"{mod.__name__}.{new.__name__}"
        raise SkipNoHash(digestname, implementation) from exc
    return constructor


def _builtin_hash(module_name, digestname, /, **kwargs):
    """Check availability of module_name.digestname(**kwargs).

    - The *module_name* is the C extension module name based on HACL*.
    - The *digestname* is one of its member, e.g., 'md5'.

    The constructor function is returned, or SkipTest is raised if none exists.
    """
    assert isinstance(module_name, str), module_name
    assert isinstance(digestname, str), digestname
    fullname = f'{module_name}.{digestname}'
    try:
        builtin_module = importlib.import_module(module_name)
    except ImportError as exc:
        raise SkipNoHash(fullname, "builtin") from exc
    try:
        constructor = getattr(builtin_module, digestname)
    except AttributeError as exc:
        raise SkipNoHash(fullname, "builtin") from exc
    try:
        constructor(**kwargs)
    except ValueError as exc:
        raise SkipNoHash(fullname, "builtin") from exc
    return constructor


def _openssl_new(digestname, /, **kwargs):
    """Check availability of _hashlib.new(digestname, **kwargs).

    The constructor function is returned, or SkipTest is raised if none exists.
    """
    assert isinstance(digestname, str), digestname
    try:
        # re-import '_hashlib' in case it was mocked
        _hashlib = importlib.import_module("_hashlib")
    except ImportError as exc:
        raise SkipNoHash(digestname, "openssl") from exc
    constructor = partial(_hashlib.new, digestname, **kwargs)
    try:
        constructor()
    except ValueError as exc:
        raise SkipNoHash(digestname, "_hashlib.new") from exc
    return constructor


def _get_openssl_hash_constructor(digestname, **kwargs):
    """Check availability of _hashlib.openssl_<digestname>(**kwargs).

    The constructor function is returned, or SkipTest is raised if none exists.
    """
    assert isinstance(digestname, str), digestname
    fullname = f"_hashlib.openssl_{digestname}"
    try:
        # re-import '_hashlib' in case it was mocked
        _hashlib = importlib.import_module("_hashlib")
    except ImportError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    try:
        constructor = getattr(_hashlib, f"openssl_{digestname}", None)
    except AttributeError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    try:
        constructor(**kwargs)
    except ValueError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    return constructor


def _make_requires_hashdigest_decorator(check_availability):
    def decorator_func(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            check_availability()
            return func(*args, **kwargs)
        return wrapper
    return partial(_decorate_func_or_class, decorator_func)


def requires_hashdigest(digestname, openssl=None, *, usedforsecurity=True):
    """Decorator raising SkipTest if a hashing algorithm is not available.

    The hashing algorithm may be missing, blocked by a strict crypto policy,
    or Python may be configured with `--with-builtin-hashlib-hashes=no`.

    If 'openssl' is True, then the decorator checks that OpenSSL provides
    the algorithm. Otherwise the check falls back to (optional) built-in
    HACL* implementations.

    The usedforsecurity flag is passed to the constructor but has no effect
    on HACL* implementations.

    Examples of exceptions being suppressed:
    ValueError: [digital envelope routines: EVP_DigestInit_ex] disabled for FIPS
    ValueError: unsupported hash type md4
    """
    def check_availability():
        _hashlib_new(digestname, openssl, usedforsecurity=usedforsecurity)

    return _make_requires_hashdigest_decorator(check_availability)


def requires_openssl_hashdigest(digestname, *, usedforsecurity=True):
    """Decorator raising SkipTest if an OpenSSL hashing algorithm is missing.

    The hashing algorithm may be missing or blocked by a strict crypto policy.
    """
    def check_availability():
        _openssl_new(digestname, usedforsecurity=usedforsecurity)

    return _make_requires_hashdigest_decorator(check_availability)


def requires_builtin_hashdigest(
    module_name, digestname, *, usedforsecurity=True
):
    """Decorator raising SkipTest if a HACL* hashing algorithm is missing.

    - The *module_name* is the C extension module name based on HACL*.
    - The *digestname* is one of its member, e.g., 'md5'.
    """
    def check_availability():
        _builtin_hash(module_name, digestname, usedforsecurity=usedforsecurity)

    return _make_requires_hashdigest_decorator(check_availability)


def requires_builtin_hashes(*ignored, usedforsecurity=True):
    """Decorator raising SkipTest if one HACL* hashing algorithm is missing."""
    return _chain_decorators((
        requires_builtin_hashdigest(
            api.builtin_module_name,
            api.builtin_method_name,
            usedforsecurity=usedforsecurity
        )
        for name, api in _EXPLICIT_CONSTRUCTORS.items()
        if name not in ignored
    ))


class HashFunctionsTrait:
    """Mixin trait class containing hash functions.

    This class is assumed to have all unitest.TestCase methods but should
    not directly inherit from it to prevent the test suite being run on it.

    Subclasses should implement the hash functions by returning an object
    that can be recognized as a valid digestmod parameter for both hashlib
    and HMAC. In particular, it cannot be a lambda function as it will not
    be recognized by hashlib (it will still be accepted by the pure Python
    implementation of HMAC).
    """

    DIGEST_NAMES = [
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
        'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    ]

    # Default 'usedforsecurity' to use when looking up a hash function.
    usedforsecurity = True

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        assert CANONICAL_DIGEST_NAMES.issuperset(cls.DIGEST_NAMES)

    def is_valid_digest_name(self, digestname):
        self.assertIn(digestname, self.DIGEST_NAMES)

    def _find_constructor(self, digestname):
        # By default, a missing algorithm skips the test that uses it.
        self.is_valid_digest_name(digestname)
        self.skipTest(f"missing hash function: {digestname}")

    @property
    def md5(self):
        return self._find_constructor("md5")

    @property
    def sha1(self):
        return self._find_constructor("sha1")

    @property
    def sha224(self):
        return self._find_constructor("sha224")

    @property
    def sha256(self):
        return self._find_constructor("sha256")

    @property
    def sha384(self):
        return self._find_constructor("sha384")

    @property
    def sha512(self):
        return self._find_constructor("sha512")

    @property
    def sha3_224(self):
        return self._find_constructor("sha3_224")

    @property
    def sha3_256(self):
        return self._find_constructor("sha3_256")

    @property
    def sha3_384(self):
        return self._find_constructor("sha3_384")

    @property
    def sha3_512(self):
        return self._find_constructor("sha3_512")


class NamedHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing named hash functions.

    Hash functions are available if and only if they are available in hashlib.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        return digestname


class OpenSSLHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing OpenSSL hash functions.

    Hash functions are available if and only if they are available in _hashlib.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        # This returns a function of the form _hashlib.openssl_<name> and
        # not a lambda function as it is rejected by _hashlib.hmac_new().
        return _get_openssl_hash_constructor(
            digestname, usedforsecurity=self.usedforsecurity)


class BuiltinHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing HACL* hash functions.

    Hash functions are available if and only if they are available in C.
    In particular, HACL* HMAC-MD5 may be available even though HACL* md5
    is not since the former is unconditionally built.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        info = _EXPLICIT_CONSTRUCTORS[digestname].builtin
        self.assertIsNotNone(info, f"no built-in implementation "
                                   f"for {digestname!r}")
        module_name, digestname = info.split('.', maxsplit=1)
        return _builtin_hash(
            module_name, digestname, usedforsecurity=self.usedforsecurity)


def find_gil_minsize(modules_names, default=2048):
    """Get the largest GIL_MINSIZE value for the given cryptographic modules.

    The valid module names are the following:

    - _hashlib
    - _md5, _sha1, _sha2, _sha3, _blake2
    - _hmac
    """
    sizes = []
    for module_name in modules_names:
        module = try_import_module(module_name)
        if module is not None:
            sizes.append(getattr(module, '_GIL_MINSIZE', default))
    return max(sizes, default=default)


def _block_openssl_hash_new(blocked_name):
    """Block OpenSSL implementation of _hashlib.new()."""
    assert isinstance(blocked_name, str), blocked_name
    # re-import '_hashlib' in case it was mocked
    if (_hashlib := try_import_module("_hashlib")) is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.new)
    def wrapper(name, data=b'', *, usedforsecurity=True, string=None):
        if name == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(*args, **kwargs)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.new', wrapper)


def _block_openssl_hmac_new(blocked_name):
    """Block OpenSSL HMAC-HASH implementation."""
    assert isinstance(blocked_name, str), blocked_name
    # re-import '_hashlib' in case it was mocked
    if (_hashlib := try_import_module("_hashlib")) is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.hmac_new)
    def wrapper(key, msg=b'', digestmod=None):
        if digestmod == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.hmac_new', wrapper)


def _block_openssl_hmac_digest(blocked_name):
    """Block OpenSSL HMAC-HASH one-shot digest implementation."""
    assert isinstance(blocked_name, str), blocked_name
    # re-import '_hashlib' in case it was mocked
    if (_hashlib := try_import_module("_hashlib")) is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.hmac_digest)
    def wrapper(key, msg, digest):
        if digest == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.hmac_digest', wrapper)


@contextlib.contextmanager
def _block_builtin_hash_new(name):
    """Block a buitin-in hash name from the hashlib.new() interface."""
    assert isinstance(name, str), name
    assert name.lower() == name, f"invalid name: {name}"
    assert name in _EXPLICIT_CONSTRUCTORS, name

    # Re-import 'hashlib' in case it was mocked, but propagate
    # exceptions as it should be unconditionally available.
    hashlib = importlib.import_module("hashlib")
    builtin_cache = getattr(hashlib, '__builtin_constructor_cache')
    if name in builtin_cache:
        f = builtin_cache.pop(name)
        F = builtin_cache.pop(name.upper(), None)
    else:
        f = F = None

    # __get_builtin_constructor() imports the HACL* modules on demand,
    # so we need to block the possibility of importing it, but only
    # during the call to __get_builtin_constructor().
    get_builtin_constructor = getattr(hashlib, '__get_builtin_constructor')
    builtin_module_name = _EXPLICIT_CONSTRUCTORS[name].builtin_module_name

    def get_builtin_constructor_wrapper(name):
        with import_helper.isolated_modules():
            sys = importlib.import_module("sys")
            sys.modules[builtin_module_name] = None  # block module's import
            return get_builtin_constructor(name)

    try:
        setattr(hashlib, "__get_builtin_constructor",
                get_builtin_constructor_wrapper)
        yield
    finally:
        setattr(hashlib, "__get_builtin_constructor", get_builtin_constructor)
        if f is not None:
            builtin_cache[name] = f
        if F is not None:
            builtin_cache[name.upper()] = F


def _block_builtin_hmac_new(blocked_name):
    assert isinstance(blocked_name, str), blocked_name
    # re-import '_hmac' in case it was mocked
    if (_hmac := try_import_module("_hmac")) is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hmac.new)
    def wrapper(key, msg=None, digestmod=None):
        if digestmod == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hmac.new', wrapper)


def _block_builtin_hmac_digest(blocked_name):
    assert isinstance(blocked_name, str), blocked_name
    # re-import '_hmac' in case it was mocked
    if (_hmac := try_import_module("_hmac")) is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hmac.compute_digest)
    def wrapper(key, msg, digest):
        if digest == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digest)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hmac.compute_digest', wrapper)


def _make_hash_constructor_blocker(name, dummy, *, interface):
    assert isinstance(name, str), name
    assert interface in ('builtin', 'openssl', 'hashlib')
    assert name in _EXPLICIT_CONSTRUCTORS, f"invalid hash: {name}"
    fullname = _EXPLICIT_CONSTRUCTORS[name].fullname(interface)
    if fullname is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()
    assert fullname.count('.') == 1, fullname
    module_name, method = fullname.split('.', maxsplit=1)
    try:
        module = importlib.import_module(module_name)
    except ImportError:
        # module is already disabled
        return contextlib.nullcontext()
    wrapped = getattr(module, method)
    wrapper = functools.wraps(wrapped)(dummy)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(fullname, wrapper)


def _block_hashlib_hash_constructor(name):
    """Block explicit public constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"blocked explicit public hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='hashlib')


def _block_openssl_hash_constructor(name):
    """Block explicit OpenSSL constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"blocked explicit OpenSSL hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='openssl')


def _block_builtin_hash_constructor(name):
    """Block explicit HACL* constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=b''):
        raise ValueError(f"blocked explicit builtin hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='builtin')


def _block_builtin_hmac_constructor(name):
    """Block explicit HACL* HMAC constructors."""
    assert isinstance(name, str), name
    assert name in _EXPLICIT_HMAC_CONSTRUCTORS, f"invalid hash: {name}"
    fullname = _EXPLICIT_HMAC_CONSTRUCTORS[name]
    if fullname is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()
    assert fullname.count('.') == 1, fullname
    module_name, method = fullname.split('.', maxsplit=1)
    assert module_name == '_hmac', module_name
    try:
        module = importlib.import_module(module_name)
    except ImportError:
        # module is already disabled
        return contextlib.nullcontext()
    @functools.wraps(wrapped := getattr(module, method))
    def wrapper(key, obj):
        raise ValueError(f"blocked hash name: {name}")
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(fullname, wrapper)


@contextlib.contextmanager
def block_algorithm(name, *, allow_openssl=False, allow_builtin=False):
    """Block a hash algorithm for both hashing and HMAC.

    Be careful with this helper as a function may be allowed, but can
    still raise a ValueError at runtime if the OpenSSL security policy
    disables it, e.g., if allow_openssl=True and FIPS mode is on.
    """
    with (contextlib.ExitStack() as stack):
        if not (allow_openssl or allow_builtin):
            # Named constructors have a different behavior in the sense
            # that they are either built-ins or OpenSSL ones, but not
            # "agile" ones (namely once "hashlib" has been imported,
            # they are fixed).
            #
            # If OpenSSL is not available, hashes fall back to built-in ones,
            # in which case we don't need to block the explicit public hashes
            # as they will call a mocked one.
            #
            # If OpenSSL is available, hashes fall back to "openssl_*" ones,
            # except for BLAKE2b and BLAKE2s.
            stack.enter_context(_block_hashlib_hash_constructor(name))
        elif (
            # In FIPS mode, hashlib.<name>() functions may raise if they use
            # the OpenSSL implementation, except with usedforsecurity=False.
            # However, blocking such functions also means blocking them
            # so we again need to block them if we want to.
            (_hashlib := try_import_module("_hashlib"))
            and _hashlib.get_fips_mode()
            and not allow_openssl
        ) or (
            # Without OpenSSL, hashlib.<name>() functions are aliases
            # to built-in functions, so both of them must be blocked
            # as the module may have been imported before the HACL ones.
            not (_hashlib := try_import_module("_hashlib"))
            and not allow_builtin
        ):
            stack.enter_context(_block_hashlib_hash_constructor(name))

        if not allow_openssl:
            # _hashlib.new()
            stack.enter_context(_block_openssl_hash_new(name))
            # _hashlib.openssl_*()
            stack.enter_context(_block_openssl_hash_constructor(name))
            # _hashlib.hmac_new()
            stack.enter_context(_block_openssl_hmac_new(name))
            # _hashlib.hmac_digest()
            stack.enter_context(_block_openssl_hmac_digest(name))

        if not allow_builtin:
            # __get_builtin_constructor(name)
            stack.enter_context(_block_builtin_hash_new(name))
            # <built-in module>.<built-in name>()
            stack.enter_context(_block_builtin_hash_constructor(name))
            # _hmac.new(..., name)
            stack.enter_context(_block_builtin_hmac_new(name))
            # _hmac.compute_<name>()
            stack.enter_context(_block_builtin_hmac_constructor(name))
            # _hmac.compute_digest(..., name)
            stack.enter_context(_block_builtin_hmac_digest(name))
        yield
