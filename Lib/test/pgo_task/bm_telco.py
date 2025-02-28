# coding: UTF-8
""" Telco Benchmark for measuring the performance of decimal calculations

- http://speleotrove.com/decimal/telco.html
- http://speleotrove.com/decimal/telcoSpec.html

A call type indicator, c, is set from the bottom (least significant) bit of the duration (hence c is 0 or 1).
A rate, r, is determined from the call type. Those calls with c=0 have a low r: 0.0013; the remainder (‘distance calls’) have a ‘premium’ r: 0.00894. (The rates are, very roughly, in Euros or dollarates per second.)
A price, p, for the call is then calculated (p=r*n). This is rounded to exactly 2 fractional digits using round-half-even (Banker’s round to nearest).
A basic tax, b, is calculated: b=p*0.0675 (6.75%). This is truncated to exactly 2 fractional digits (round-down), and the total basic tax variable is then incremented (sumB=sumB+b).
For distance calls: a distance tax, d, is calculated: d=p*0.0341 (3.41%). This is truncated to exactly 2 fractional digits (round-down), and then the total distance tax variable is incremented (sumD=sumD+d).
The total price, t, is calculated (t=p+b, and, if a distance call, t=t+d).
The total prices variable is incremented (sumT=sumT+t).
The total price, t, is converted to a string, s.

"""

from decimal import ROUND_HALF_EVEN, ROUND_DOWN, Decimal, getcontext, Context
import io
from struct import unpack
import gzip
import base64

# The first 8000 bytes of the "telco-bench.b" file from pyperformance.
DATA = b'''\
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
l8AWQB8AAA==
'''

def bench_telco(loops):
    getcontext().rounding = ROUND_DOWN
    rates = list(map(Decimal, ('0.0013', '0.00894')))
    twodig = Decimal('0.01')
    Banker = Context(rounding=ROUND_HALF_EVEN)
    basictax = Decimal("0.0675")
    disttax = Decimal("0.0341")

    data = gzip.decompress(base64.decodebytes(DATA))

    infil = io.BytesIO(data)
    outfil = io.StringIO()

    for _ in range(loops):
        infil.seek(0)

        sumT = Decimal("0")   # sum of total prices
        sumB = Decimal("0")   # sum of basic tax
        sumD = Decimal("0")   # sum of 'distance' tax

        for i in range(1000):
            datum = infil.read(8)
            if datum == '':
                break
            n, =  unpack('>Q', datum)

            calltype = n & 1
            r = rates[calltype]

            p = Banker.quantize(r * n, twodig)

            b = p * basictax
            b = b.quantize(twodig)
            sumB += b

            t = p + b

            if calltype:
                d = p * disttax
                d = d.quantize(twodig)
                sumD += d
                t += d

            sumT += t
            print(t, file=outfil)

        outfil.seek(0)
        outfil.truncate()


def run_pgo():
    bench_telco(100)
