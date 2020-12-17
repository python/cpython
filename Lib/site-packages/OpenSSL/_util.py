import sys
import warnings

from six import PY2, text_type

from cryptography.hazmat.bindings.openssl.binding import Binding


binding = Binding()
binding.init_static_locks()
ffi = binding.ffi
lib = binding.lib


# This is a special CFFI allocator that does not bother to zero its memory
# after allocation. This has vastly better performance on large allocations and
# so should be used whenever we don't need the memory zeroed out.
no_zero_allocator = ffi.new_allocator(should_clear_after_alloc=False)


def text(charp):
    """
    Get a native string type representing of the given CFFI ``char*`` object.

    :param charp: A C-style string represented using CFFI.

    :return: :class:`str`
    """
    if not charp:
        return ""
    return native(ffi.string(charp))


def exception_from_error_queue(exception_type):
    """
    Convert an OpenSSL library failure into a Python exception.

    When a call to the native OpenSSL library fails, this is usually signalled
    by the return value, and an error code is stored in an error queue
    associated with the current thread. The err library provides functions to
    obtain these error codes and textual error messages.
    """
    errors = []

    while True:
        error = lib.ERR_get_error()
        if error == 0:
            break
        errors.append(
            (
                text(lib.ERR_lib_error_string(error)),
                text(lib.ERR_func_error_string(error)),
                text(lib.ERR_reason_error_string(error)),
            )
        )

    raise exception_type(errors)


def make_assert(error):
    """
    Create an assert function that uses :func:`exception_from_error_queue` to
    raise an exception wrapped by *error*.
    """

    def openssl_assert(ok):
        """
        If *ok* is not True, retrieve the error from OpenSSL and raise it.
        """
        if ok is not True:
            exception_from_error_queue(error)

    return openssl_assert


def native(s):
    """
    Convert :py:class:`bytes` or :py:class:`unicode` to the native
    :py:class:`str` type, using UTF-8 encoding if conversion is necessary.

    :raise UnicodeError: The input string is not UTF-8 decodeable.

    :raise TypeError: The input is neither :py:class:`bytes` nor
        :py:class:`unicode`.
    """
    if not isinstance(s, (bytes, text_type)):
        raise TypeError("%r is neither bytes nor unicode" % s)
    if PY2:
        if isinstance(s, text_type):
            return s.encode("utf-8")
    else:
        if isinstance(s, bytes):
            return s.decode("utf-8")
    return s


def path_string(s):
    """
    Convert a Python string to a :py:class:`bytes` string identifying the same
    path and which can be passed into an OpenSSL API accepting a filename.

    :param s: An instance of :py:class:`bytes` or :py:class:`unicode`.

    :return: An instance of :py:class:`bytes`.
    """
    if isinstance(s, bytes):
        return s
    elif isinstance(s, text_type):
        return s.encode(sys.getfilesystemencoding())
    else:
        raise TypeError("Path must be represented as bytes or unicode string")


if PY2:

    def byte_string(s):
        return s


else:

    def byte_string(s):
        return s.encode("charmap")


# A marker object to observe whether some optional arguments are passed any
# value or not.
UNSPECIFIED = object()

_TEXT_WARNING = (
    text_type.__name__ + " for {0} is no longer accepted, use bytes"
)


def text_to_bytes_and_warn(label, obj):
    """
    If ``obj`` is text, emit a warning that it should be bytes instead and try
    to convert it to bytes automatically.

    :param str label: The name of the parameter from which ``obj`` was taken
        (so a developer can easily find the source of the problem and correct
        it).

    :return: If ``obj`` is the text string type, a ``bytes`` object giving the
        UTF-8 encoding of that text is returned.  Otherwise, ``obj`` itself is
        returned.
    """
    if isinstance(obj, text_type):
        warnings.warn(
            _TEXT_WARNING.format(label),
            category=DeprecationWarning,
            stacklevel=3,
        )
        return obj.encode("utf-8")
    return obj


from_buffer = ffi.from_buffer
