
import mpz
from test.test_support import vereq, TestFailed

def check_conversion(num):
    mpz_num = mpz.mpz(num)
    vereq(int(mpz_num), num)
    vereq(long(mpz_num), num)
    vereq(str(mpz_num), 'mpz(%s)' % `int(num)`)

check_conversion(10)
check_conversion(10L)
# FIXME: should check strings, but I'm not sure it works, this seems odd:
#   mpz.mpz('10') == mpz(12337)

vereq(mpz.divm(100,  200,  3), 2)
vereq(mpz.divm(100L, 200,  3), 2)
vereq(mpz.divm(100,  200L, 3), 2)
vereq(mpz.divm(100L, 200L, 3), 2)

vereq(mpz.gcd(100,  200),  100)
vereq(mpz.gcd(100L, 200),  100)
vereq(mpz.gcd(100,  200L), 100)
vereq(mpz.gcd(100L, 200L), 100)

vereq(mpz.gcdext(100,  200),  (100, 1, 0))
vereq(mpz.gcdext(100L, 200),  (100, 1, 0))
vereq(mpz.gcdext(100,  200L), (100, 1, 0))
vereq(mpz.gcdext(100L, 200L), (100, 1, 0))

vereq(mpz.powm(100,  0,  3), 1)
vereq(mpz.powm(100L, 0,  3), 1)
vereq(mpz.powm(100,  0L, 3), 1)
vereq(mpz.powm(100L, 0L, 3), 1)

vereq(mpz.powm(101,  5,  3333), 1616)
vereq(mpz.powm(101L, 5,  3333), 1616)
vereq(mpz.powm(101,  5L, 3333), 1616)
vereq(mpz.powm(101L, 5L, 3333), 1616)

vereq(mpz.sqrt(100),  10)
vereq(mpz.sqrt(100L), 10)
vereq(mpz.sqrt(200),  14)
vereq(mpz.sqrt(200L), 14)

vereq(mpz.sqrtrem(100),  (10, 0))
vereq(mpz.sqrtrem(100L), (10, 0))
vereq(mpz.sqrtrem(200),  (14, 4))
vereq(mpz.sqrtrem(200L), (14, 4))

try: mpz.mpz(10.)
except TypeError: pass
else: raise TestFailed, 'mpz(10.) should raise a TypeError'

try: mpz.powm(10.)
except TypeError: pass
else: raise TestFailed, 'powm(10.) should raise a TypeError'

try: mpz.powm(100,  1,  0)
except ValueError: pass
else: raise TestFailed, 'powm(100, 1, 0) should raise a ValueError'

try: mpz.divm(10, 10)
except TypeError: pass
else: raise TestFailed, 'divm(10, 10) should raise a TypeError'

try: mpz.divm(10, 10, 10.)
except TypeError: pass
else: raise TestFailed, 'divm(10, 10, 10.) should raise a TypeError'

try: mpz.gcd(10)
except TypeError: pass
else: raise TestFailed, 'gcd(10) should raise a TypeError'

try: mpz.gcd(10, 10.)
except TypeError: pass
else: raise TestFailed, 'gcd(10, 10.) should raise a TypeError'

try: mpz.gcdext(10)
except TypeError: pass
else: raise TestFailed, 'gcdext(10) should raise a TypeError'

try: mpz.gcdext(10, 10.)
except TypeError: pass
else: raise TestFailed, 'gcdext(10, 10.) should raise a TypeError'

try: mpz.mpz(-10).binary()
except ValueError: pass
else: raise TestFailed, 'mpz(-10).binary() should raise a ValueError'
