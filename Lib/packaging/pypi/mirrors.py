"""Utilities related to the mirror infrastructure defined in PEP 381."""

from string import ascii_lowercase
import socket

DEFAULT_MIRROR_URL = "last.pypi.python.org"


def get_mirrors(hostname=None):
    """Return the list of mirrors from the last record found on the DNS
    entry::

    >>> from packaging.pypi.mirrors import get_mirrors
    >>> get_mirrors()
    ['a.pypi.python.org', 'b.pypi.python.org', 'c.pypi.python.org',
    'd.pypi.python.org']

    """
    if hostname is None:
        hostname = DEFAULT_MIRROR_URL

    # return the last mirror registered on PyPI.
    try:
        hostname = socket.gethostbyname_ex(hostname)[0]
    except socket.gaierror:
        return []
    end_letter = hostname.split(".", 1)

    # determine the list from the last one.
    return ["%s.%s" % (s, end_letter[1]) for s in string_range(end_letter[0])]


def string_range(last):
    """Compute the range of string between "a" and last.

    This works for simple "a to z" lists, but also for "a to zz" lists.
    """
    for k in range(len(last)):
        for x in product(ascii_lowercase, repeat=(k + 1)):
            result = ''.join(x)
            yield result
            if result == last:
                return


def product(*args, **kwds):
    pools = [tuple(arg) for arg in args] * kwds.get('repeat', 1)
    result = [[]]
    for pool in pools:
        result = [x + [y] for x in result for y in pool]
    for prod in result:
        yield tuple(prod)
