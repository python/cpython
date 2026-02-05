import contextlib
import enum
import functools
import importlib
import inspect
import unittest
import unittest.mock
from test.support import import_helper
from types import MappingProxyType


def _parse_fullname(fullname, *, strict=False):
    """Parse a fully-qualified name ``<module_name>.<member_name>``.

    The ``module_name`` component contains one or more dots.
    The ``member_name`` component does not contain any dot.

    If *strict* is true, *fullname* must be a string. Otherwise,
    it can be None, and, ``module_name`` and ``member_name`` will
    also be None.
    """
    if fullname is None:
        assert not strict
        return None, None
    assert isinstance(fullname, str), fullname
    assert fullname.count(".") >= 1, fullname
    module_name, member_name = fullname.rsplit(".", maxsplit=1)
    return module_name, member_name


def _import_module(module_name, *, strict=False):
    """Import a module from its fully-qualified name.

    If *strict* is false, import failures are suppressed and None is returned.
    """
    if module_name is None:
        # To prevent a TypeError in importlib.import_module
        if strict:
            raise ImportError("no module to import")
        return None
    try:
        return importlib.import_module(module_name)
    except ImportError as exc:
        if strict:
            raise exc
        return None


def _import_member(module_name, member_name, *, strict=False):
    """Import a member from a module.

    If *strict* is false, import failures are suppressed and None is returned.
    """
    if member_name is None:
        if strict:
            raise ImportError(f"no member to import from {module_name}")
        return None
    module = _import_module(module_name, strict=strict)
    if strict:
        return getattr(module, member_name)
    return getattr(module, member_name, None)


class Implementation(enum.StrEnum):
    # Indicate that the hash function is implemented by a built-in module.
    builtin = enum.auto()
    # Indicate that the hash function is implemented by OpenSSL.
    openssl = enum.auto()
    # Indicate that the hash function is provided through the public API.
    hashlib = enum.auto()


class _HashId(enum.StrEnum):
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


CANONICAL_DIGEST_NAMES = frozenset(map(str, _HashId.__members__))
NON_HMAC_DIGEST_NAMES = frozenset((
    _HashId.shake_128, _HashId.shake_256,
    _HashId.blake2s, _HashId.blake2b,
))


class _HashInfoItem:
    """Interface for interacting with a named object.

    The object is entirely described by its fully-qualified *fullname*.

    *fullname* must be None or a string "<module_name>.<member_name>".
    """

    def __init__(self, fullname=None, *, strict=False):
        module_name, member_name = _parse_fullname(fullname, strict=strict)
        self.fullname = fullname
        self.module_name = module_name
        self.member_name = member_name

    def import_module(self, *, strict=False):
        """Import the described module.

        If *strict* is true, an ImportError may be raised if importing fails,
        otherwise, None is returned on error.
        """
        return _import_module(self.module_name, strict=strict)

    def import_member(self, *, strict=False):
        """Import the described member.

        If *strict* is true, an AttributeError or an ImportError may be
        raised if importing fails; otherwise, None is returned on error.
        """
        return _import_member(
            self.module_name, self.member_name, strict=strict
        )


class _HashInfoBase:
    """Base dataclass containing "backend" information.

    Subclasses may define an attribute named after one of the known
    implementations ("builtin", "openssl" or "hashlib") which stores
    an _HashInfoItem object.

    Those attributes can be retrieved through __getitem__(), e.g.,
    ``info["builtin"]`` returns the _HashInfoItem corresponding to
    the builtin implementation.
    """

    def __init__(self, canonical_name):
        assert isinstance(canonical_name, _HashId), canonical_name
        self.canonical_name = canonical_name

    def __getitem__(self, implementation):
        try:
            attrname = Implementation(implementation)
        except ValueError:
            raise self.invalid_implementation_error(implementation) from None

        try:
            provider = getattr(self, attrname)
        except AttributeError:
            raise self.invalid_implementation_error(implementation) from None

        if not isinstance(provider, _HashInfoItem):
            raise KeyError(implementation)
        return provider

    def invalid_implementation_error(self, implementation):
        msg = f"no implementation {implementation} for {self.canonical_name}"
        return AssertionError(msg)


class _HashTypeInfo(_HashInfoBase):
    """Dataclass containing information for hash functions types.

    - *canonical_name* must be a _HashId.

    - *builtin* is the fully-qualified name for the builtin HACL* type,
      e.g., "_md5.MD5Type".

    - *openssl* is the fully-qualified name for the OpenSSL wrapper type,
      e.g., "_hashlib.HASH".
    """

    def __init__(self, canonical_name, builtin, openssl):
        super().__init__(canonical_name)
        self.builtin = _HashInfoItem(builtin, strict=True)
        self.openssl = _HashInfoItem(openssl, strict=True)

    def fullname(self, implementation):
        """Get the fully qualified name of a given implementation.

        This returns a string of the form "MODULE_NAME.OBJECT_NAME" or None
        if the hash function does not have a corresponding implementation.

        *implementation* must be "builtin" or "openssl".
        """
        return self[implementation].fullname

    def module_name(self, implementation):
        """Get the name of the module containing the hash object type."""
        return self[implementation].module_name

    def object_type_name(self, implementation):
        """Get the name of the hash object class name."""
        return self[implementation].member_name

    def import_module(self, implementation, *, allow_skip=False):
        """Import the module containing the hash object type.

        On error, return None if *allow_skip* is false, or raise SkipNoHash.
        """
        target = self[implementation]
        module = target.import_module()
        if allow_skip and module is None:
            reason = f"cannot import module {target.module_name}"
            raise SkipNoHash(self.canonical_name, implementation, reason)
        return module

    def import_object_type(self, implementation, *, allow_skip=False):
        """Get the runtime hash object type.

        On error, return None if *allow_skip* is false, or raise SkipNoHash.
        """
        target = self[implementation]
        member = target.import_member()
        if allow_skip and member is None:
            reason = f"cannot import class {target.fullname}"
            raise SkipNoHash(self.canonical_name, implementation, reason)
        return member


class _HashFuncInfo(_HashInfoBase):
    """Dataclass containing information for hash functions constructors.

    - *canonical_name* must be a _HashId.

    - *builtin* is the fully-qualified name of the HACL*
      hash constructor function, e.g., "_md5.md5".

    - *openssl* is the fully-qualified name of the "_hashlib" method
      for the OpenSSL named constructor, e.g., "_hashlib.openssl_md5".

    - *hashlib* is the fully-qualified name of the "hashlib" method
      for the explicit named hash constructor, e.g., "hashlib.md5".
    """

    def __init__(self, canonical_name, builtin, openssl=None, hashlib=None):
        super().__init__(canonical_name)
        self.builtin = _HashInfoItem(builtin, strict=True)
        self.openssl = _HashInfoItem(openssl, strict=False)
        self.hashlib = _HashInfoItem(hashlib, strict=False)

    def fullname(self, implementation):
        """Get the fully qualified name of a given implementation.

        This returns a string of the form "MODULE_NAME.METHOD_NAME" or None
        if the hash function does not have a corresponding implementation.

        *implementation* must be "builtin", "openssl" or "hashlib".
        """
        return self[implementation].fullname

    def module_name(self, implementation):
        """Get the name of the constructor function module.

        The *implementation* must be "builtin", "openssl" or "hashlib".
        """
        return self[implementation].module_name

    def method_name(self, implementation):
        """Get the name of the constructor function module method.

        Use fullname() to get the constructor function fully-qualified name.

        The *implementation* must be "builtin", "openssl" or "hashlib".
        """
        return self[implementation].member_name


class _HashInfo:
    """Dataclass containing information for supported hash functions.

    Attributes
    ----------
    canonical_name : _HashId
        The hash function canonical name.
    type : _HashTypeInfo
        The hash object types information.
    func : _HashTypeInfo
        The hash object constructors information.
    """

    def __init__(
        self,
        canonical_name,
        builtin_object_type_fullname,
        openssl_object_type_fullname,
        builtin_method_fullname,
        openssl_method_fullname=None,
        hashlib_method_fullname=None,
    ):
        """
        - *canonical_name* must be a _HashId.

        - *builtin_object_type_fullname* is the fully-qualified name
          for the builtin HACL* type, e.g., "_md5.MD5Type".

        - *openssl_object_type_fullname* is the fully-qualified name
          for the OpenSSL wrapper type, e.g., "_hashlib.HASH".

        - *builtin_method_fullname* is the fully-qualified name
          of the HACL* hash constructor function, e.g., "_md5.md5".

        - *openssl_method_fullname* is the fully-qualified name
          of the "_hashlib" module method for the explicit OpenSSL
          hash constructor function, e.g., "_hashlib.openssl_md5".

        - *hashlib_method_fullname* is the fully-qualified name
          of the "hashlib"  module method for the explicit hash
          constructor function, e.g., "hashlib.md5".
        """
        assert isinstance(canonical_name, _HashId), canonical_name
        self.canonical_name = canonical_name
        self.type = _HashTypeInfo(
            canonical_name,
            builtin_object_type_fullname,
            openssl_object_type_fullname,
        )
        self.func = _HashFuncInfo(
            canonical_name,
            builtin_method_fullname,
            openssl_method_fullname,
            hashlib_method_fullname,
        )


_HASHINFO_DATABASE = MappingProxyType({
    _HashId.md5: _HashInfo(
        _HashId.md5,
        "_md5.MD5Type",
        "_hashlib.HASH",
        "_md5.md5",
        "_hashlib.openssl_md5",
        "hashlib.md5",
    ),
    _HashId.sha1: _HashInfo(
        _HashId.sha1,
        "_sha1.SHA1Type",
        "_hashlib.HASH",
        "_sha1.sha1",
        "_hashlib.openssl_sha1",
        "hashlib.sha1",
    ),
    _HashId.sha224: _HashInfo(
        _HashId.sha224,
        "_sha2.SHA224Type",
        "_hashlib.HASH",
        "_sha2.sha224",
        "_hashlib.openssl_sha224",
        "hashlib.sha224",
    ),
    _HashId.sha256: _HashInfo(
        _HashId.sha256,
        "_sha2.SHA256Type",
        "_hashlib.HASH",
        "_sha2.sha256",
        "_hashlib.openssl_sha256",
        "hashlib.sha256",
    ),
    _HashId.sha384: _HashInfo(
        _HashId.sha384,
        "_sha2.SHA384Type",
        "_hashlib.HASH",
        "_sha2.sha384",
        "_hashlib.openssl_sha384",
        "hashlib.sha384",
    ),
    _HashId.sha512: _HashInfo(
        _HashId.sha512,
        "_sha2.SHA512Type",
        "_hashlib.HASH",
        "_sha2.sha512",
        "_hashlib.openssl_sha512",
        "hashlib.sha512",
    ),
    _HashId.sha3_224: _HashInfo(
        _HashId.sha3_224,
        "_sha3.sha3_224",
        "_hashlib.HASH",
        "_sha3.sha3_224",
        "_hashlib.openssl_sha3_224",
        "hashlib.sha3_224",
    ),
    _HashId.sha3_256: _HashInfo(
        _HashId.sha3_256,
        "_sha3.sha3_256",
        "_hashlib.HASH",
        "_sha3.sha3_256",
        "_hashlib.openssl_sha3_256",
        "hashlib.sha3_256",
    ),
    _HashId.sha3_384: _HashInfo(
        _HashId.sha3_384,
        "_sha3.sha3_384",
        "_hashlib.HASH",
        "_sha3.sha3_384",
        "_hashlib.openssl_sha3_384",
        "hashlib.sha3_384",
    ),
    _HashId.sha3_512: _HashInfo(
        _HashId.sha3_512,
        "_sha3.sha3_512",
        "_hashlib.HASH",
        "_sha3.sha3_512",
        "_hashlib.openssl_sha3_512",
        "hashlib.sha3_512",
    ),
    _HashId.shake_128: _HashInfo(
        _HashId.shake_128,
        "_sha3.shake_128",
        "_hashlib.HASHXOF",
        "_sha3.shake_128",
        "_hashlib.openssl_shake_128",
        "hashlib.shake_128",
    ),
    _HashId.shake_256: _HashInfo(
        _HashId.shake_256,
        "_sha3.shake_256",
        "_hashlib.HASHXOF",
        "_sha3.shake_256",
        "_hashlib.openssl_shake_256",
        "hashlib.shake_256",
    ),
    _HashId.blake2s: _HashInfo(
        _HashId.blake2s,
        "_blake2.blake2s",
        "_hashlib.HASH",
        "_blake2.blake2s",
        None,
        "hashlib.blake2s",
    ),
    _HashId.blake2b: _HashInfo(
        _HashId.blake2b,
        "_blake2.blake2b",
        "_hashlib.HASH",
        "_blake2.blake2b",
        None,
        "hashlib.blake2b",
    ),
})
assert _HASHINFO_DATABASE.keys() == CANONICAL_DIGEST_NAMES


def get_hash_type_info(name):
    info = _HASHINFO_DATABASE[name]
    assert isinstance(info, _HashInfo), info
    return info.type


def get_hash_func_info(name):
    info = _HASHINFO_DATABASE[name]
    assert isinstance(info, _HashInfo), info
    return info.func


def _iter_hash_func_info(excluded):
    for name, info in _HASHINFO_DATABASE.items():
        if name not in excluded:
            yield info.func


# Mapping from canonical hash names to their explicit HACL* HMAC constructor.
# There is currently no OpenSSL one-shot named function and there will likely
# be none in the future.
_HMACINFO_DATABASE = {
    _HashId(canonical_name): _HashInfoItem(f"_hmac.compute_{canonical_name}")
    for canonical_name in CANONICAL_DIGEST_NAMES
}
# Neither HACL* nor OpenSSL supports HMAC over XOFs.
_HMACINFO_DATABASE[_HashId.shake_128] = _HashInfoItem()
_HMACINFO_DATABASE[_HashId.shake_256] = _HashInfoItem()
# Strictly speaking, HMAC-BLAKE is meaningless as BLAKE2 is already a
# keyed hash function. However, as it's exposed by HACL*, we test it.
_HMACINFO_DATABASE[_HashId.blake2s] = _HashInfoItem('_hmac.compute_blake2s_32')
_HMACINFO_DATABASE[_HashId.blake2b] = _HashInfoItem('_hmac.compute_blake2b_32')
_HMACINFO_DATABASE = MappingProxyType(_HMACINFO_DATABASE)
assert _HMACINFO_DATABASE.keys() == CANONICAL_DIGEST_NAMES


def get_hmac_item_info(name):
    info = _HMACINFO_DATABASE[name]
    assert isinstance(info, _HashInfoItem), info
    return info


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


def _make_conditional_decorator(test, /, *test_args, **test_kwargs):
    def decorator_func(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            test(*test_args, **test_kwargs)
            return func(*args, **kwargs)
        return wrapper
    return functools.partial(_decorate_func_or_class, decorator_func)


def requires_openssl_hashlib():
    _hashlib = _import_module("_hashlib")
    return unittest.skipIf(_hashlib is None, "requires _hashlib")


def requires_builtin_hmac():
    _hmac = _import_module("_hmac")
    return unittest.skipIf(_hmac is None, "requires _hmac")


class SkipNoHash(unittest.SkipTest):
    """A SkipTest exception raised when a hash is not available."""

    def __init__(self, digestname, implementation=None, reason=None):
        parts = ["missing", implementation, f"hash algorithm {digestname!r}"]
        if reason is not None:
            parts.insert(0, f"{reason}: ")
        super().__init__(" ".join(filter(None, parts)))


class SkipNoHashInCall(SkipNoHash):

    def __init__(self, func, digestname, implementation=None):
        super().__init__(digestname, implementation, f"cannot use {func}")


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
    _hashlib = _import_module("_hashlib")
    module = _hashlib if openssl and _hashlib is not None else hashlib
    try:
        module.new(digestname, **kwargs)
    except ValueError as exc:
        raise SkipNoHashInCall(f"{module.__name__}.new", digestname) from exc
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
        raise SkipNoHashInCall("_hashlib.new", digestname) from exc
    return functools.partial(_hashlib.new, digestname)


def _openssl_hash(digestname, /, **kwargs):
    """Check availability of _hashlib.openssl_<digestname>(**kwargs).

    The constructor function is returned (without binding **kwargs),
    or SkipTest is raised if none exists.
    """
    assert isinstance(digestname, str), digestname
    method_name = f"openssl_{digestname}"
    fullname = f"_hashlib.{method_name}"
    try:
        # re-import '_hashlib' in case it was mocked
        _hashlib = importlib.import_module("_hashlib")
    except ImportError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    try:
        constructor = getattr(_hashlib, method_name)
    except AttributeError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    try:
        constructor(**kwargs)
    except ValueError as exc:
        raise SkipNoHash(fullname, "openssl") from exc
    return constructor


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
    return _make_conditional_decorator(
        _hashlib_new, digestname, openssl, usedforsecurity=usedforsecurity
    )


def requires_openssl_hashdigest(digestname, *, usedforsecurity=True):
    """Decorator raising SkipTest if an OpenSSL hashing algorithm is missing.

    The hashing algorithm may be missing or blocked by a strict crypto policy.
    """
    return _make_conditional_decorator(
        _openssl_new, digestname, usedforsecurity=usedforsecurity
    )


def _make_requires_builtin_hashdigest_decorator(item, *, usedforsecurity=True):
    assert isinstance(item, _HashInfoItem), item
    return _make_conditional_decorator(
        _builtin_hash,
        item.module_name,
        item.member_name,
        usedforsecurity=usedforsecurity,
    )


def requires_builtin_hashdigest(canonical_name, *, usedforsecurity=True):
    """Decorator raising SkipTest if a HACL* hashing algorithm is missing."""
    info = get_hash_func_info(canonical_name)
    return _make_requires_builtin_hashdigest_decorator(
        info.builtin, usedforsecurity=usedforsecurity
    )


def requires_builtin_hashes(*, exclude=(), usedforsecurity=True):
    """Decorator raising SkipTest if one HACL* hashing algorithm is missing."""
    return _chain_decorators((
        _make_requires_builtin_hashdigest_decorator(
            info.builtin, usedforsecurity=usedforsecurity
        ) for info in _iter_hash_func_info(exclude)
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

    # Default 'usedforsecurity' to use when checking a hash function.
    # When the trait properties are callables (e.g., _md5.md5) and
    # not strings, they must be called with the same 'usedforsecurity'.
    usedforsecurity = True

    def is_valid_digest_name(self, digestname):
        self.assertIn(digestname, _HashId)

    def _find_constructor(self, digestname):
        # By default, a missing algorithm skips the test that uses it.
        self.is_valid_digest_name(digestname)
        self.skipTest(f"missing hash function: {digestname}")

    md5 = property(lambda self: self._find_constructor("md5"))
    sha1 = property(lambda self: self._find_constructor("sha1"))

    sha224 = property(lambda self: self._find_constructor("sha224"))
    sha256 = property(lambda self: self._find_constructor("sha256"))
    sha384 = property(lambda self: self._find_constructor("sha384"))
    sha512 = property(lambda self: self._find_constructor("sha512"))

    sha3_224 = property(lambda self: self._find_constructor("sha3_224"))
    sha3_256 = property(lambda self: self._find_constructor("sha3_256"))
    sha3_384 = property(lambda self: self._find_constructor("sha3_384"))
    sha3_512 = property(lambda self: self._find_constructor("sha3_512"))


class NamedHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing named hash functions.

    Hash functions are available if and only if they are available in hashlib.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        return str(digestname)  # ensure that we are an exact string


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
        info = get_hash_func_info(digestname)
        return _builtin_hash(
            info.builtin.module_name,
            info.builtin.member_name,
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
        module = _import_module(module_name)
        if module is not None:
            sizes.append(getattr(module, '_GIL_MINSIZE', default))
    return max(sizes, default=default)


def _block_openssl_hash_new(blocked_name):
    """Block OpenSSL implementation of _hashlib.new()."""
    assert isinstance(blocked_name, str), blocked_name

    # re-import '_hashlib' in case it was mocked
    if (_hashlib := _import_module("_hashlib")) is None:
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
    if (_hashlib := _import_module("_hashlib")) is None:
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
    if (_hashlib := _import_module("_hashlib")) is None:
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
    assert name in _HashId, f"invalid hash: {name}"

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
    builtin_module_name = get_hash_func_info(name).builtin.module_name

    @functools.wraps(get_builtin_constructor)
    def get_builtin_constructor_mock(name):
        with import_helper.isolated_modules():
            sys = importlib.import_module("sys")
            sys.modules[builtin_module_name] = None  # block module's import
            return get_builtin_constructor(name)

    return unittest.mock.patch.multiple(
        hashlib,
        __get_builtin_constructor=get_builtin_constructor_mock,
        __builtin_constructor_cache=builtin_constructor_cache_mock,
    )


def _block_builtin_hmac_new(blocked_name):
    assert isinstance(blocked_name, str), blocked_name

    # re-import '_hmac' in case it was mocked
    if (_hmac := _import_module("_hmac")) is None:
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
    if (_hmac := _import_module("_hmac")) is None:
        return contextlib.nullcontext()

    @functools.wraps(wrapped := _hmac.compute_digest)
    def _hmac_compute_digest(key, msg, digest):
        if digest == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digest)

    _ensure_wrapper_signature(_hmac_compute_digest, wrapped)
    return unittest.mock.patch('_hmac.compute_digest', _hmac_compute_digest)


def _make_hash_constructor_blocker(name, dummy, implementation):
    info = get_hash_func_info(name)[implementation]
    if (wrapped := info.import_member()) is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()
    wrapper = functools.wraps(wrapped)(dummy)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(info.fullname, wrapper)


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
    info = get_hmac_item_info(name)
    assert info.module_name is None or info.module_name == "_hmac", info
    if (wrapped := info.import_member()) is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()

    @functools.wraps(wrapped)
    def wrapper(key, obj):
        raise ValueError(f"blocked hash name: {name}")

    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(info.fullname, wrapper)


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
            (_hashlib := _import_module("_hashlib"))
            and _hashlib.get_fips_mode()
            and not allow_openssl
        ) or (
            # Without OpenSSL, hashlib.<name>() functions are aliases
            # to built-in functions, so both of them must be blocked
            # as the module may have been imported before the HACL ones.
            not (_hashlib := _import_module("_hashlib"))
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


@contextlib.contextmanager
def block_openssl_algorithms(*, exclude=()):
    """Block OpenSSL implementations, except those given in *exclude*."""
    with contextlib.ExitStack() as stack:
        for name in CANONICAL_DIGEST_NAMES.difference(exclude):
            stack.enter_context(block_algorithm(name, allow_builtin=True))
        yield


@contextlib.contextmanager
def block_builtin_algorithms(*, exclude=()):
    """Block HACL* implementations, except those given in *exclude*."""
    with contextlib.ExitStack() as stack:
        for name in CANONICAL_DIGEST_NAMES.difference(exclude):
            stack.enter_context(block_algorithm(name, allow_openssl=True))
        yield
