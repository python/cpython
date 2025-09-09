import contextlib
import enum
import functools
import importlib
import inspect
import unittest
import unittest.mock
from test.support import import_helper
from types import MappingProxyType


def try_import_module(module_name):
    """Try to import a module and return None on failure."""
    try:
        return importlib.import_module(module_name)
    except ImportError:
        return None


class HID(enum.StrEnum):
    """Enumeration containing the canonical digest names.

    Those names should only be used by hashlib.new() or hmac.new().
    Their support by _hashlib.new() is not necessarily guaranteed.
    """

    md5 = enum.auto()
    sha1 = enum.auto()

    sha224 = enum.auto()
    sha256 = enum.auto()
    sha384 = enum.auto()
    sha512 = enum.auto()

    sha3_224 = enum.auto()
    sha3_256 = enum.auto()
    sha3_384 = enum.auto()
    sha3_512 = enum.auto()

    shake_128 = enum.auto()
    shake_256 = enum.auto()

    blake2s = enum.auto()
    blake2b = enum.auto()

    def __repr__(self):
        return str(self)

    @property
    def is_xof(self):
        """Indicate whether the hash is an extendable-output hash function."""
        return self.startswith("shake_")

    @property
    def is_keyed(self):
        """Indicate whether the hash is a keyed hash function."""
        return self.startswith("blake2")


CANONICAL_DIGEST_NAMES = frozenset(map(str, HID.__members__))
NON_HMAC_DIGEST_NAMES = frozenset((
    HID.shake_128, HID.shake_256,
    HID.blake2s, HID.blake2b,
))


class HashInfo:
    """Dataclass storing explicit hash constructor names.

    - *builtin* is the fully-qualified name for the explicit HACL*
      hash constructor function, e.g., "_md5.md5".

    - *openssl* is the name of the "_hashlib" module method for the explicit
      OpenSSL hash constructor function, e.g., "openssl_md5".

    - *hashlib* is the name of the "hashlib" module method for the explicit
      hash constructor function, e.g., "md5".
    """

    def __init__(self, builtin, openssl=None, hashlib=None):
        assert isinstance(builtin, str), builtin
        assert len(builtin.split(".")) == 2, builtin

        self.builtin = builtin
        self.builtin_module_name, self.builtin_method_name = (
            self.builtin.split(".", maxsplit=1)
        )

        assert openssl is None or openssl.startswith("openssl_")
        self.openssl = self.openssl_method_name = openssl
        self.openssl_module_name = "_hashlib" if openssl else None

        assert hashlib is None or isinstance(hashlib, str)
        self.hashlib = self.hashlib_method_name = hashlib
        self.hashlib_module_name = "hashlib" if hashlib else None

    def module_name(self, implementation):
        match implementation:
            case "builtin":
                return self.builtin_module_name
            case "openssl":
                return self.openssl_module_name
            case "hashlib":
                return self.hashlib_module_name
        raise AssertionError(f"invalid implementation {implementation}")

    def method_name(self, implementation):
        match implementation:
            case "builtin":
                return self.builtin_method_name
            case "openssl":
                return self.openssl_method_name
            case "hashlib":
                return self.hashlib_method_name
        raise AssertionError(f"invalid implementation {implementation}")

    def fullname(self, implementation):
        """Get the fully qualified name of a given implementation.

        This returns a string of the form "MODULE_NAME.METHOD_NAME" or None
        if the hash function does not have a corresponding implementation.

        *implementation* must be "builtin", "openssl" or "hashlib".
        """
        module_name = self.module_name(implementation)
        method_name = self.method_name(implementation)
        if module_name is None or method_name is None:
            return None
        return f"{module_name}.{method_name}"


# Mapping from a "canonical" name to a pair (HACL*, _hashlib.*, hashlib.*)
# constructors. If the constructor name is None, then this means that the
# algorithm can only be used by the "agile" new() interfaces.
_EXPLICIT_CONSTRUCTORS = MappingProxyType({  # fmt: skip
    HID.md5: HashInfo("_md5.md5", "openssl_md5", "md5"),
    HID.sha1: HashInfo("_sha1.sha1", "openssl_sha1", "sha1"),
    HID.sha224: HashInfo("_sha2.sha224", "openssl_sha224", "sha224"),
    HID.sha256: HashInfo("_sha2.sha256", "openssl_sha256", "sha256"),
    HID.sha384: HashInfo("_sha2.sha384", "openssl_sha384", "sha384"),
    HID.sha512: HashInfo("_sha2.sha512", "openssl_sha512", "sha512"),
    HID.sha3_224: HashInfo(
        "_sha3.sha3_224", "openssl_sha3_224", "sha3_224"
    ),
    HID.sha3_256: HashInfo(
        "_sha3.sha3_256", "openssl_sha3_256", "sha3_256"
    ),
    HID.sha3_384: HashInfo(
        "_sha3.sha3_384", "openssl_sha3_384", "sha3_384"
    ),
    HID.sha3_512: HashInfo(
        "_sha3.sha3_512", "openssl_sha3_512", "sha3_512"
    ),
    HID.shake_128: HashInfo(
        "_sha3.shake_128", "openssl_shake_128", "shake_128"
    ),
    HID.shake_256: HashInfo(
        "_sha3.shake_256", "openssl_shake_256", "shake_256"
    ),
    HID.blake2s: HashInfo("_blake2.blake2s", None, "blake2s"),
    HID.blake2b: HashInfo("_blake2.blake2b", None, "blake2b"),
})
assert _EXPLICIT_CONSTRUCTORS.keys() == CANONICAL_DIGEST_NAMES
get_hash_info = _EXPLICIT_CONSTRUCTORS.__getitem__

# Mapping from canonical hash names to their explicit HACL* HMAC constructor.
# There is currently no OpenSSL one-shot named function and there will likely
# be none in the future.
_EXPLICIT_HMAC_CONSTRUCTORS = {
    HID(name): f"_hmac.compute_{name}"
    for name in CANONICAL_DIGEST_NAMES
}
# Neither HACL* nor OpenSSL supports HMAC over XOFs.
_EXPLICIT_HMAC_CONSTRUCTORS[HID.shake_128] = None
_EXPLICIT_HMAC_CONSTRUCTORS[HID.shake_256] = None
# Strictly speaking, HMAC-BLAKE is meaningless as BLAKE2 is already a
# keyed hash function. However, as it's exposed by HACL*, we test it.
_EXPLICIT_HMAC_CONSTRUCTORS[HID.blake2s] = '_hmac.compute_blake2s_32'
_EXPLICIT_HMAC_CONSTRUCTORS[HID.blake2b] = '_hmac.compute_blake2b_32'
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
    return functools.partial(_decorate_func_or_class, decorator_func)


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


def requires_hashlib():
    _hashlib = try_import_module("_hashlib")
    return unittest.skipIf(_hashlib is None, "requires _hashlib")


def requires_builtin_hmac():
    _hmac = try_import_module("_hmac")
    return unittest.skipIf(_hmac is None, "requires _hmac")


class SkipNoHash(unittest.SkipTest):
    """A SkipTest exception raised when a hash is not available."""

    def __init__(self, digestname, implementation=None, interface=None):
        parts = ["missing", implementation, f"hash algorithm {digestname!r}"]
        if interface is not None:
            parts.append(f"for {interface}")
        super().__init__(" ".join(filter(None, parts)))


def _hashlib_new(digestname, openssl, /, **kwargs):
    """Check availability of [hashlib|_hashlib].new(digestname, **kwargs).

    If *openssl* is True, module is "_hashlib" (C extension module),
    otherwise it is "hashlib" (pure Python interface).

    The constructor function is returned (without binding **kwargs),
    or SkipTest is raised if none exists.
    """
    assert isinstance(digestname, str), digestname
    # Re-import 'hashlib' in case it was mocked, but propagate
    # exceptions as it should be unconditionally available.
    hashlib = importlib.import_module("hashlib")
    # re-import '_hashlib' in case it was mocked
    _hashlib = try_import_module("_hashlib")
    module = _hashlib if openssl and _hashlib is not None else hashlib
    try:
        module.new(digestname, **kwargs)
    except ValueError as exc:
        interface = f"{module.__name__}.new"
        raise SkipNoHash(digestname, interface=interface) from exc
    return functools.partial(module.new, digestname)


def _builtin_hash(module_name, digestname, /, **kwargs):
    """Check availability of <module_name>.<digestname>(**kwargs).

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

    The constructor function is returned (without binding **kwargs),
    or SkipTest is raised if none exists.
    """
    assert isinstance(digestname, str), digestname
    try:
        # re-import '_hashlib' in case it was mocked
        _hashlib = importlib.import_module("_hashlib")
    except ImportError as exc:
        raise SkipNoHash(digestname, "openssl") from exc
    try:
        _hashlib.new(digestname, **kwargs)
    except ValueError as exc:
        raise SkipNoHash(digestname, interface="_hashlib.new") from exc
    return functools.partial(_hashlib.new, digestname)


def _openssl_hash(digestname, /, **kwargs):
    """Check availability of _hashlib.openssl_<digestname>(**kwargs).

    The constructor function is returned (without binding **kwargs),
    or SkipTest is raised if none exists.
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


def _make_requires_hashdigest_decorator(test, /, *test_args, **test_kwargs):
    def decorator_func(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            test(*test_args, **test_kwargs)
            return func(*args, **kwargs)
        return wrapper
    return functools.partial(_decorate_func_or_class, decorator_func)


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
    return _make_requires_hashdigest_decorator(
        _hashlib_new, digestname, openssl, usedforsecurity=usedforsecurity
    )


def requires_openssl_hashdigest(digestname, *, usedforsecurity=True):
    """Decorator raising SkipTest if an OpenSSL hashing algorithm is missing.

    The hashing algorithm may be missing or blocked by a strict crypto policy.
    """
    return _make_requires_hashdigest_decorator(
        _openssl_new, digestname, usedforsecurity=usedforsecurity
    )


def requires_builtin_hashdigest(
    module_name, digestname, *, usedforsecurity=True
):
    """Decorator raising SkipTest if a HACL* hashing algorithm is missing.

    - The *module_name* is the C extension module name based on HACL*.
    - The *digestname* is one of its member, e.g., 'md5'.
    """
    return _make_requires_hashdigest_decorator(
        _builtin_hash, module_name, digestname, usedforsecurity=usedforsecurity
    )


def requires_builtin_hashes(*ignored, usedforsecurity=True):
    """Decorator raising SkipTest if one HACL* hashing algorithm is missing."""
    return _chain_decorators((
        requires_builtin_hashdigest(
            api.builtin_module_name,
            api.builtin_method_name,
            usedforsecurity=usedforsecurity,
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

    # Default 'usedforsecurity' to use when checking a hash function.
    # When the trait properties are callables (e.g., _md5.md5) and
    # not strings, they must be called with the same 'usedforsecurity'.
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
        return _openssl_hash(digestname, usedforsecurity=self.usedforsecurity)


class BuiltinHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing HACL* hash functions.

    Hash functions are available if and only if they are available in C.
    In particular, HACL* HMAC-MD5 may be available even though HACL* md5
    is not since the former is unconditionally built.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        info = _EXPLICIT_CONSTRUCTORS[digestname]
        return _builtin_hash(
            info.builtin_module_name,
            info.builtin_method_name,
            usedforsecurity=self.usedforsecurity,
        )


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
    def _hashlib_new(name, data=b'', *, usedforsecurity=True, string=None):
        if name == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(name, data,
                       usedforsecurity=usedforsecurity, string=string)

    _ensure_wrapper_signature(_hashlib_new, wrapped)
    return unittest.mock.patch('_hashlib.new', _hashlib_new)


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
    def _hashlib_hmac_digest(key, msg, digest):
        if digest == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(key, msg, digest)

    _ensure_wrapper_signature(_hashlib_hmac_digest, wrapped)
    return unittest.mock.patch('_hashlib.hmac_digest', _hashlib_hmac_digest)


def _block_builtin_hash_new(name):
    """Block a buitin-in hash name from the hashlib.new() interface."""
    assert isinstance(name, str), name
    assert name.lower() == name, f"invalid name: {name}"
    assert name in HID, f"invalid hash: {name}"

    # Re-import 'hashlib' in case it was mocked
    hashlib = importlib.import_module('hashlib')
    builtin_constructor_cache = getattr(hashlib, '__builtin_constructor_cache')
    builtin_constructor_cache_mock = builtin_constructor_cache.copy()
    builtin_constructor_cache_mock.pop(name, None)
    builtin_constructor_cache_mock.pop(name.upper(), None)

    # __get_builtin_constructor() imports the HACL* modules on demand,
    # so we need to block the possibility of importing it, but only
    # during the call to __get_builtin_constructor().
    get_builtin_constructor = getattr(hashlib, '__get_builtin_constructor')
    builtin_module_name = _EXPLICIT_CONSTRUCTORS[name].builtin_module_name

    @functools.wraps(get_builtin_constructor)
    def get_builtin_constructor_mock(name):
        with import_helper.isolated_modules():
            sys = importlib.import_module("sys")
            sys.modules[builtin_module_name] = None  # block module's import
            return get_builtin_constructor(name)

    return unittest.mock.patch.multiple(
        hashlib,
        __get_builtin_constructor=get_builtin_constructor_mock,
        __builtin_constructor_cache=builtin_constructor_cache_mock
    )


def _block_builtin_hmac_new(blocked_name):
    assert isinstance(blocked_name, str), blocked_name

    # re-import '_hmac' in case it was mocked
    if (_hmac := try_import_module("_hmac")) is None:
        return contextlib.nullcontext()

    @functools.wraps(wrapped := _hmac.new)
    def _hmac_new(key, msg=None, digestmod=None):
        if digestmod == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digestmod)

    _ensure_wrapper_signature(_hmac_new, wrapped)
    return unittest.mock.patch('_hmac.new', _hmac_new)


def _block_builtin_hmac_digest(blocked_name):
    assert isinstance(blocked_name, str), blocked_name

    # re-import '_hmac' in case it was mocked
    if (_hmac := try_import_module("_hmac")) is None:
        return contextlib.nullcontext()

    @functools.wraps(wrapped := _hmac.compute_digest)
    def _hmac_compute_digest(key, msg, digest):
        if digest == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digest)

    _ensure_wrapper_signature(_hmac_compute_digest, wrapped)
    return unittest.mock.patch('_hmac.compute_digest', _hmac_compute_digest)


def _make_hash_constructor_blocker(name, dummy, implementation):
    info = _EXPLICIT_CONSTRUCTORS[name]
    module_name = info.module_name(implementation)
    method_name = info.method_name(implementation)
    if module_name is None or method_name is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()

    try:
        module = importlib.import_module(module_name)
    except ImportError:
        # module is already disabled
        return contextlib.nullcontext()

    wrapped = getattr(module, method_name)
    wrapper = functools.wraps(wrapped)(dummy)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(info.fullname(implementation), wrapper)


def _block_hashlib_hash_constructor(name):
    """Block explicit public constructors."""
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"blocked explicit public hash name: {name}")

    return _make_hash_constructor_blocker(name, dummy, 'hashlib')


def _block_openssl_hash_constructor(name):
    """Block explicit OpenSSL constructors."""
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"blocked explicit OpenSSL hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, 'openssl')


def _block_builtin_hash_constructor(name):
    """Block explicit HACL* constructors."""
    def dummy(data=b'', *, usedforsecurity=True, string=b''):
        raise ValueError(f"blocked explicit builtin hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, 'builtin')


def _block_builtin_hmac_constructor(name):
    """Block explicit HACL* HMAC constructors."""
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
    with contextlib.ExitStack() as stack:
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
