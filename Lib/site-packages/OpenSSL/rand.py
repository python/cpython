"""
PRNG management routines, thin wrappers.
"""

from OpenSSL._util import lib as _lib


def add(buffer, entropy):
    """
    Mix bytes from *string* into the PRNG state.

    The *entropy* argument is (the lower bound of) an estimate of how much
    randomness is contained in *string*, measured in bytes.

    For more information, see e.g. :rfc:`1750`.

    This function is only relevant if you are forking Python processes and
    need to reseed the CSPRNG after fork.

    :param buffer: Buffer with random data.
    :param entropy: The entropy (in bytes) measurement of the buffer.

    :return: :obj:`None`
    """
    if not isinstance(buffer, bytes):
        raise TypeError("buffer must be a byte string")

    if not isinstance(entropy, int):
        raise TypeError("entropy must be an integer")

    _lib.RAND_add(buffer, len(buffer), entropy)


def status():
    """
    Check whether the PRNG has been seeded with enough data.

    :return: 1 if the PRNG is seeded enough, 0 otherwise.
    """
    return _lib.RAND_status()
