from math import gcd
from random import randrange


def _miller_rabin_witness(a, u, t, n):
    """Return True if a witness is found for n, we are sure that n is composite."""
    a %= n
    if a <= 1:
        return False

    x = pow(a, u, n)
    if x == 1 or x == n-1:
        return False
    for _ in range(t-1):
        x = pow(x, 2, n)
        if x == n-1:
            return False
    return True

def _miller_rabin(bases, n):
    """Perform a Miller-Rabin primality test of an odd integer n."""
    q, t = n-1, 0
    while q & 1 == 0:
        t += 1
        q >>= 1
    u = (n-1)//(2**t)

    return not any(
        _miller_rabin_witness(a, u, t, n)
        for a in bases
    )


def is_prime(n: int, s: int=25) -> bool:
    """Returns whether n is prime.

    A deterministic version of the Miller-Rabin primality test is used for smalls
    n. For large n, a probabilistic Miller-Rabin test is used and n is either
    known to be composite when is_prime(n) returns True, or probably prime with
    with a probability of failure of at most 4^(−s).

    >>> is_prime(17)
    True
    >>> is_prime(121)
    False
    """

    if n <= 1:
        return False
    if n%2 == 0:
        return (n == 2)

    # If n is small with test we use a deterministic Miller-Rabin test,
    # see https://miller-rabin.appspot.com/ and
    # https://arxiv.org/pdf/1509.00864.pdf
    if n < 341531:
        bases = [9345883071009581737]
    elif n < 1050535501:
        bases = [336781006125, 9639812373923155]
    elif n < 350269456337:
        bases = [4230279247111683200, 14694767155120705706, 16641139526367750375]
    elif n < 55245642489451:
        bases = [2, 141889084524735, 1199124725622454117, 11096072698276303650]
    elif n < 7999252175582851:
        bases = [2, 4130806001517, 149795463772692060, 186635894390467037, 3967304179347715805]
    elif n < 585226005592931977:
        bases = [2, 123635709730000, 9233062284813009, 43835965440333360, 761179012939631437, 1263739024124850375]
    elif n < 18446744073709551616:
        bases = [2, 325, 9375, 28178, 450775, 9780504, 1795265022]
    elif n < 318665857834031151167461:
        bases = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37]
    elif n < 3317044064679887385961981:
        bases = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41]
    else:
        bases = (randrange(2, n) for _ in range(s))

    return _miller_rabin(bases, n)


def next_prime(n: int) -> int:
    """Returns the smallest prime number greater than n.

    >>> next_prime(6)
    7
    """
    if n < 0:
        n = 0
    if n < 10:
        return [2, 2, 3, 5, 5, 7, 7, 11, 11, 11][n]

    if 1 <= n % 6 <= 4:
        n += 5 - (n%6)
        if is_prime(n):
            return n

    if n % 6 == 0:
        n += 1
    else:
        n += 2

    while True:
        if is_prime(n):
            return n
        n += 4
        if is_prime(n):
            return n
        n += 2


def previous_prime(n: int) -> int:
    """Returns the largest prime number less than n.

    >>> previous_prime(23)
    19
    """
    if n <= 2:
        raise ValueError(f'There is no prime number less than {n}.')

    if n < 10:
        return [0, 0, 0, 2, 3, 3, 5, 5, 7, 7, 7][n]

    if 1 < n % 6 <= 5:
        n -= (n%6) - 1
        if is_prime(n):
            return n

    if n % 6 == 0:
        n -= 1
    else:
        n -= 2

    while True:
        if is_prime(n):
            return n
        n -= 4
        if is_prime(n):
            return n
        n -= 2


def _rho_pollard(n):
    x = y = randrange(n)
    i = k = 2

    while True:
        x = (x**2 - 1) % n
        d = gcd(y-x, n)
        if d == n:
            # break the loop
            x = y = randrange(n)
            i = k = 2
        elif d != 1:
            return d

        if i == k:
            y = x
            k *= 2
        i += 1


def _factorise(n):
    if is_prime(n):
        yield n
    else:
        d = _rho_pollard(n)
        yield from _factorise(d)
        yield from _factorise(n // d)


def factorise(n: int) -> tuple[int]:
    """Factorise an integer n using Pollard's rho algorithm.

    Returns an iterator of all prime divisors of n repeated so that
    prod(factorise(n)) == n.

    If n is less than 0, -1 is the first element of the result.

    Example:
    >>> sorted(factorise(1729))
    [7, 13, 19]
    >>> sorted(factorise(-21))
    [-1, 3, 7]
    """
    if n == 0:
        raise ValueError("Cannot factorise 0.")
    elif n < 0:
        yield -1
        n *= -1

    if n == 1:
        return

    yield from _factorise(n)


__all__ = ['is_prime', 'next_prime', 'previous_prime', 'factorise']
