# Tests for the correctly-rounded string -> float conversions
# introduced in Python 2.7 and 3.1.

import random
import unittest
import re
import sys
import test.support

if getattr(sys, 'float_repr_style', '') != 'short':
    raise unittest.SkipTest('correctly-rounded string->float conversions '
                            'not available on this system')

# Correctly rounded str -> float in pure Python, for comparison.

strtod_parser = re.compile(r"""    # A numeric string consists of:
    (?P<sign>[-+])?          # an optional sign, followed by
    (?=\d|\.\d)              # a number with at least one digit
    (?P<int>\d*)             # having a (possibly empty) integer part
    (?:\.(?P<frac>\d*))?     # followed by an optional fractional part
    (?:E(?P<exp>[-+]?\d+))?  # and an optional exponent
    \Z
""", re.VERBOSE | re.IGNORECASE).match

# Pure Python version of correctly rounded string->float conversion.
# Avoids any use of floating-point by returning the result as a hex string.
def strtod(s, mant_dig=53, min_exp = -1021, max_exp = 1024):
    """Convert a finite decimal string to a hex string representing an
    IEEE 754 binary64 float.  Return 'inf' or '-inf' on overflow.
    This function makes no use of floating-point arithmetic at any
    stage."""

    # parse string into a pair of integers 'a' and 'b' such that
    # abs(decimal value) = a/b, along with a boolean 'negative'.
    m = strtod_parser(s)
    if m is None:
        raise ValueError('invalid numeric string')
    fraction = m.group('frac') or ''
    intpart = int(m.group('int') + fraction)
    exp = int(m.group('exp') or '0') - len(fraction)
    negative = m.group('sign') == '-'
    a, b = intpart*10**max(exp, 0), 10**max(0, -exp)

    # quick return for zeros
    if not a:
        return '-0x0.0p+0' if negative else '0x0.0p+0'

    # compute exponent e for result; may be one too small in the case
    # that the rounded value of a/b lies in a different binade from a/b
    d = a.bit_length() - b.bit_length()
    d += (a >> d if d >= 0 else a << -d) >= b
    e = max(d, min_exp) - mant_dig

    # approximate a/b by number of the form q * 2**e; adjust e if necessary
    a, b = a << max(-e, 0), b << max(e, 0)
    q, r = divmod(a, b)
    if 2*r > b or 2*r == b and q & 1:
        q += 1
        if q.bit_length() == mant_dig+1:
            q //= 2
            e += 1

    # double check that (q, e) has the right form
    assert q.bit_length() <= mant_dig and e >= min_exp - mant_dig
    assert q.bit_length() == mant_dig or e == min_exp - mant_dig

    # check for overflow and underflow
    if e + q.bit_length() > max_exp:
        return '-inf' if negative else 'inf'
    if not q:
        return '-0x0.0p+0' if negative else '0x0.0p+0'

    # for hex representation, shift so # bits after point is a multiple of 4
    hexdigs = 1 + (mant_dig-2)//4
    shift = 3 - (mant_dig-2)%4
    q, e = q << shift, e - shift
    return '{}0x{:x}.{:0{}x}p{:+d}'.format(
        '-' if negative else '',
        q // 16**hexdigs,
        q % 16**hexdigs,
        hexdigs,
        e + 4*hexdigs)

TEST_SIZE = 10

class StrtodTests(unittest.TestCase):
    def check_strtod(self, s):
        """Compare the result of Python's builtin correctly rounded
        string->float conversion (using float) to a pure Python
        correctly rounded string->float implementation.  Fail if the
        two methods give different results."""

        try:
            fs = float(s)
        except OverflowError:
            got = '-inf' if s[0] == '-' else 'inf'
        except MemoryError:
            got = 'memory error'
        else:
            got = fs.hex()
        expected = strtod(s)
        self.assertEqual(expected, got,
                         "Incorrectly rounded str->float conversion for {}: "
                         "expected {}, got {}".format(s, expected, got))

    def test_short_halfway_cases(self):
        # exact halfway cases with a small number of significant digits
        for k in 0, 5, 10, 15, 20:
            # upper = smallest integer >= 2**54/5**k
            upper = -(-2**54//5**k)
            # lower = smallest odd number >= 2**53/5**k
            lower = -(-2**53//5**k)
            if lower % 2 == 0:
                lower += 1
            for i in range(TEST_SIZE):
                # Select a random odd n in [2**53/5**k,
                # 2**54/5**k). Then n * 10**k gives a halfway case
                # with small number of significant digits.
                n, e = random.randrange(lower, upper, 2), k

                # Remove any additional powers of 5.
                while n % 5 == 0:
                    n, e = n // 5, e + 1
                assert n % 10 in (1, 3, 7, 9)

                # Try numbers of the form n * 2**p2 * 10**e, p2 >= 0,
                # until n * 2**p2 has more than 20 significant digits.
                digits, exponent = n, e
                while digits < 10**20:
                    s = '{}e{}'.format(digits, exponent)
                    self.check_strtod(s)
                    # Same again, but with extra trailing zeros.
                    s = '{}e{}'.format(digits * 10**40, exponent - 40)
                    self.check_strtod(s)
                    digits *= 2

                # Try numbers of the form n * 5**p2 * 10**(e - p5), p5
                # >= 0, with n * 5**p5 < 10**20.
                digits, exponent = n, e
                while digits < 10**20:
                    s = '{}e{}'.format(digits, exponent)
                    self.check_strtod(s)
                    # Same again, but with extra trailing zeros.
                    s = '{}e{}'.format(digits * 10**40, exponent - 40)
                    self.check_strtod(s)
                    digits *= 5
                    exponent -= 1

    def test_halfway_cases(self):
        # test halfway cases for the round-half-to-even rule
        for i in range(100 * TEST_SIZE):
            # bit pattern for a random finite positive (or +0.0) float
            bits = random.randrange(2047*2**52)

            # convert bit pattern to a number of the form m * 2**e
            e, m = divmod(bits, 2**52)
            if e:
                m, e = m + 2**52, e - 1
            e -= 1074

            # add 0.5 ulps
            m, e = 2*m + 1, e - 1

            # convert to a decimal string
            if e >= 0:
                digits = m << e
                exponent = 0
            else:
                # m * 2**e = (m * 5**-e) * 10**e
                digits = m * 5**-e
                exponent = e
            s = '{}e{}'.format(digits, exponent)
            self.check_strtod(s)

    def test_boundaries(self):
        # boundaries expressed as triples (n, e, u), where
        # n*10**e is an approximation to the boundary value and
        # u*10**e is 1ulp
        boundaries = [
            (10000000000000000000, -19, 1110),   # a power of 2 boundary (1.0)
            (17976931348623159077, 289, 1995),   # overflow boundary (2.**1024)
            (22250738585072013831, -327, 4941),  # normal/subnormal (2.**-1022)
            (0, -327, 4941),                     # zero
            ]
        for n, e, u in boundaries:
            for j in range(1000):
                digits = n + random.randrange(-3*u, 3*u)
                exponent = e
                s = '{}e{}'.format(digits, exponent)
                self.check_strtod(s)
                n *= 10
                u *= 10
                e -= 1

    def test_underflow_boundary(self):
        # test values close to 2**-1075, the underflow boundary; similar
        # to boundary_tests, except that the random error doesn't scale
        # with n
        for exponent in range(-400, -320):
            base = 10**-exponent // 2**1075
            for j in range(TEST_SIZE):
                digits = base + random.randrange(-1000, 1000)
                s = '{}e{}'.format(digits, exponent)
                self.check_strtod(s)

    def test_bigcomp(self):
        for ndigs in 5, 10, 14, 15, 16, 17, 18, 19, 20, 40, 41, 50:
            dig10 = 10**ndigs
            for i in range(10 * TEST_SIZE):
                digits = random.randrange(dig10)
                exponent = random.randrange(-400, 400)
                s = '{}e{}'.format(digits, exponent)
                self.check_strtod(s)

    def test_parsing(self):
        # make '0' more likely to be chosen than other digits
        digits = '000000123456789'
        signs = ('+', '-', '')

        # put together random short valid strings
        # \d*[.\d*]?e
        for i in range(1000):
            for j in range(TEST_SIZE):
                s = random.choice(signs)
                intpart_len = random.randrange(5)
                s += ''.join(random.choice(digits) for _ in range(intpart_len))
                if random.choice([True, False]):
                    s += '.'
                    fracpart_len = random.randrange(5)
                    s += ''.join(random.choice(digits)
                                 for _ in range(fracpart_len))
                else:
                    fracpart_len = 0
                if random.choice([True, False]):
                    s += random.choice(['e', 'E'])
                    s += random.choice(signs)
                    exponent_len = random.randrange(1, 4)
                    s += ''.join(random.choice(digits)
                                 for _ in range(exponent_len))

                if intpart_len + fracpart_len:
                    self.check_strtod(s)
                else:
                    try:
                        float(s)
                    except ValueError:
                        pass
                    else:
                        assert False, "expected ValueError"

    def test_particular(self):
        # inputs that produced crashes or incorrectly rounded results with
        # previous versions of dtoa.c, for various reasons
        test_strings = [
            # issue 7632 bug 1, originally reported failing case
            '2183167012312112312312.23538020374420446192e-370',
            # 5 instances of issue 7632 bug 2
            '12579816049008305546974391768996369464963024663104e-357',
            '17489628565202117263145367596028389348922981857013e-357',
            '18487398785991994634182916638542680759613590482273e-357',
            '32002864200581033134358724675198044527469366773928e-358',
            '94393431193180696942841837085033647913224148539854e-358',
            '73608278998966969345824653500136787876436005957953e-358',
            '64774478836417299491718435234611299336288082136054e-358',
            '13704940134126574534878641876947980878824688451169e-357',
            '46697445774047060960624497964425416610480524760471e-358',
            # failing case for bug introduced by METD in r77451 (attempted
            # fix for issue 7632, bug 2), and fixed in r77482.
            '28639097178261763178489759107321392745108491825303e-311',
            # two numbers demonstrating a flaw in the bigcomp 'dig == 0'
            # correction block (issue 7632, bug 3)
            '1.00000000000000001e44',
            '1.0000000000000000100000000000000000000001e44',
            # dtoa.c bug for numbers just smaller than a power of 2 (issue
            # 7632, bug 4)
            '99999999999999994487665465554760717039532578546e-47',
            # failing case for off-by-one error introduced by METD in
            # r77483 (dtoa.c cleanup), fixed in r77490
            '965437176333654931799035513671997118345570045914469' #...
            '6213413350821416312194420007991306908470147322020121018368e0',
            # incorrect lsb detection for round-half-to-even when
            # bc->scale != 0 (issue 7632, bug 6).
            '104308485241983990666713401708072175773165034278685' #...
            '682646111762292409330928739751702404658197872319129' #...
            '036519947435319418387839758990478549477777586673075' #...
            '945844895981012024387992135617064532141489278815239' #...
            '849108105951619997829153633535314849999674266169258' #...
            '928940692239684771590065027025835804863585454872499' #...
            '320500023126142553932654370362024104462255244034053' #...
            '203998964360882487378334860197725139151265590832887' #...
            '433736189468858614521708567646743455601905935595381' #...
            '852723723645799866672558576993978025033590728687206' #...
            '296379801363024094048327273913079612469982585674824' #...
            '156000783167963081616214710691759864332339239688734' #...
            '656548790656486646106983450809073750535624894296242' #...
            '072010195710276073042036425579852459556183541199012' #...
            '652571123898996574563824424330960027873516082763671875e-1075',
            # demonstration that original fix for issue 7632 bug 1 was
            # buggy; the exit condition was too strong
            '247032822920623295e-341',
            # demonstrate similar problem to issue 7632 bug1: crash
            # with 'oversized quotient in quorem' message.
            '99037485700245683102805043437346965248029601286431e-373',
            '99617639833743863161109961162881027406769510558457e-373',
            '98852915025769345295749278351563179840130565591462e-372',
            '99059944827693569659153042769690930905148015876788e-373',
            '98914979205069368270421829889078356254059760327101e-372',
            # issue 7632 bug 5: the following 2 strings convert differently
            '1000000000000000000000000000000000000000e-16',
            '10000000000000000000000000000000000000000e-17',
            # issue 7632 bug 7
            '991633793189150720000000000000000000000000000000000000000e-33',
            # And another, similar, failing halfway case
            '4106250198039490000000000000000000000000000000000000000e-38',
            # issue 7632 bug 8:  the following produced 10.0
            '10.900000000000000012345678912345678912345',

            # two humongous values from issue 7743
            '116512874940594195638617907092569881519034793229385' #...
            '228569165191541890846564669771714896916084883987920' #...
            '473321268100296857636200926065340769682863349205363' #...
            '349247637660671783209907949273683040397979984107806' #...
            '461822693332712828397617946036239581632976585100633' #...
            '520260770761060725403904123144384571612073732754774' #...
            '588211944406465572591022081973828448927338602556287' #...
            '851831745419397433012491884869454462440536895047499' #...
            '436551974649731917170099387762871020403582994193439' #...
            '761933412166821484015883631622539314203799034497982' #...
            '130038741741727907429575673302461380386596501187482' #...
            '006257527709842179336488381672818798450229339123527' #...
            '858844448336815912020452294624916993546388956561522' #...
            '161875352572590420823607478788399460162228308693742' #...
            '05287663441403533948204085390898399055004119873046875e-1075',

            '525440653352955266109661060358202819561258984964913' #...
            '892256527849758956045218257059713765874251436193619' #...
            '443248205998870001633865657517447355992225852945912' #...
            '016668660000210283807209850662224417504752264995360' #...
            '631512007753855801075373057632157738752800840302596' #...
            '237050247910530538250008682272783660778181628040733' #...
            '653121492436408812668023478001208529190359254322340' #...
            '397575185248844788515410722958784640926528544043090' #...
            '115352513640884988017342469275006999104519620946430' #...
            '818767147966495485406577703972687838176778993472989' #...
            '561959000047036638938396333146685137903018376496408' #...
            '319705333868476925297317136513970189073693314710318' #...
            '991252811050501448326875232850600451776091303043715' #...
            '157191292827614046876950225714743118291034780466325' #...
            '085141343734564915193426994587206432697337118211527' #...
            '278968731294639353354774788602467795167875117481660' #...
            '4738791256853675690543663283782215866825e-1180',

            # exercise exit conditions in bigcomp comparison loop
            '2602129298404963083833853479113577253105939995688e2',
            '260212929840496308383385347911357725310593999568896e0',
            '26021292984049630838338534791135772531059399956889601e-2',
            '260212929840496308383385347911357725310593999568895e0',
            '260212929840496308383385347911357725310593999568897e0',
            '260212929840496308383385347911357725310593999568996e0',
            '260212929840496308383385347911357725310593999568866e0',
            # 2**53
            '9007199254740992.00',
            # 2**1024 - 2**970:  exact overflow boundary.  All values
            # smaller than this should round to something finite;  any value
            # greater than or equal to this one overflows.
            '179769313486231580793728971405303415079934132710037' #...
            '826936173778980444968292764750946649017977587207096' #...
            '330286416692887910946555547851940402630657488671505' #...
            '820681908902000708383676273854845817711531764475730' #...
            '270069855571366959622842914819860834936475292719074' #...
            '168444365510704342711559699508093042880177904174497792',
            # 2**1024 - 2**970 - tiny
            '179769313486231580793728971405303415079934132710037' #...
            '826936173778980444968292764750946649017977587207096' #...
            '330286416692887910946555547851940402630657488671505' #...
            '820681908902000708383676273854845817711531764475730' #...
            '270069855571366959622842914819860834936475292719074' #...
            '168444365510704342711559699508093042880177904174497791.999',
            # 2**1024 - 2**970 + tiny
            '179769313486231580793728971405303415079934132710037' #...
            '826936173778980444968292764750946649017977587207096' #...
            '330286416692887910946555547851940402630657488671505' #...
            '820681908902000708383676273854845817711531764475730' #...
            '270069855571366959622842914819860834936475292719074' #...
            '168444365510704342711559699508093042880177904174497792.001',
            # 1 - 2**-54, +-tiny
            '999999999999999944488848768742172978818416595458984375e-54',
            '9999999999999999444888487687421729788184165954589843749999999e-54',
            '9999999999999999444888487687421729788184165954589843750000001e-54',
            # Value found by Rick Regan that gives a result of 2**-968
            # under Gay's dtoa.c (as of Nov 04, 2010);  since fixed.
            # (Fixed some time ago in Python's dtoa.c.)
            '0.0000000000000000000000000000000000000000100000000' #...
            '000000000576129113423785429971690421191214034235435' #...
            '087147763178149762956868991692289869941246658073194' #...
            '51982237978882039897143840789794921875',
            ]
        for s in test_strings:
            self.check_strtod(s)

def test_main():
    test.support.run_unittest(StrtodTests)

if __name__ == "__main__":
    test_main()
