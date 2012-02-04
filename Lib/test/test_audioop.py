import audioop
import unittest
from test.support import run_unittest

endian = 'big' if audioop.getsample(b'\0\1', 2, 0) == 1 else 'little'

def gendata1():
    return b'\0\1\2'

def gendata2():
    if endian == 'big':
        return b'\0\0\0\1\0\2'
    else:
        return b'\0\0\1\0\2\0'

def gendata4():
    if endian == 'big':
        return b'\0\0\0\0\0\0\0\1\0\0\0\2'
    else:
        return b'\0\0\0\0\1\0\0\0\2\0\0\0'

data = [gendata1(), gendata2(), gendata4()]

INVALID_DATA = [
    (b'abc', 0),
    (b'abc', 2),
    (b'abc', 4),
]


class TestAudioop(unittest.TestCase):

    def test_max(self):
        self.assertEqual(audioop.max(data[0], 1), 2)
        self.assertEqual(audioop.max(data[1], 2), 2)
        self.assertEqual(audioop.max(data[2], 4), 2)

    def test_minmax(self):
        self.assertEqual(audioop.minmax(data[0], 1), (0, 2))
        self.assertEqual(audioop.minmax(data[1], 2), (0, 2))
        self.assertEqual(audioop.minmax(data[2], 4), (0, 2))

    def test_maxpp(self):
        self.assertEqual(audioop.maxpp(data[0], 1), 0)
        self.assertEqual(audioop.maxpp(data[1], 2), 0)
        self.assertEqual(audioop.maxpp(data[2], 4), 0)

    def test_avg(self):
        self.assertEqual(audioop.avg(data[0], 1), 1)
        self.assertEqual(audioop.avg(data[1], 2), 1)
        self.assertEqual(audioop.avg(data[2], 4), 1)

    def test_avgpp(self):
        self.assertEqual(audioop.avgpp(data[0], 1), 0)
        self.assertEqual(audioop.avgpp(data[1], 2), 0)
        self.assertEqual(audioop.avgpp(data[2], 4), 0)

    def test_rms(self):
        self.assertEqual(audioop.rms(data[0], 1), 1)
        self.assertEqual(audioop.rms(data[1], 2), 1)
        self.assertEqual(audioop.rms(data[2], 4), 1)

    def test_cross(self):
        self.assertEqual(audioop.cross(data[0], 1), 0)
        self.assertEqual(audioop.cross(data[1], 2), 0)
        self.assertEqual(audioop.cross(data[2], 4), 0)

    def test_add(self):
        data2 = []
        for d in data:
            str = bytearray(len(d))
            for i,b in enumerate(d):
                str[i] = 2*b
            data2.append(str)
        self.assertEqual(audioop.add(data[0], data[0], 1), data2[0])
        self.assertEqual(audioop.add(data[1], data[1], 2), data2[1])
        self.assertEqual(audioop.add(data[2], data[2], 4), data2[2])

    def test_bias(self):
        # Note: this test assumes that avg() works
        d1 = audioop.bias(data[0], 1, 100)
        d2 = audioop.bias(data[1], 2, 100)
        d4 = audioop.bias(data[2], 4, 100)
        self.assertEqual(audioop.avg(d1, 1), 101)
        self.assertEqual(audioop.avg(d2, 2), 101)
        self.assertEqual(audioop.avg(d4, 4), 101)

    def test_lin2lin(self):
        # too simple: we test only the size
        for d1 in data:
            for d2 in data:
                got = len(d1)//3
                wtd = len(d2)//3
                self.assertEqual(len(audioop.lin2lin(d1, got, wtd)), len(d2))

    def test_adpcm2lin(self):
        # Very cursory test
        self.assertEqual(audioop.adpcm2lin(b'\0\0', 1, None), (b'\0' * 4, (0,0)))
        self.assertEqual(audioop.adpcm2lin(b'\0\0', 2, None), (b'\0' * 8, (0,0)))
        self.assertEqual(audioop.adpcm2lin(b'\0\0', 4, None), (b'\0' * 16, (0,0)))

    def test_lin2adpcm(self):
        # Very cursory test
        self.assertEqual(audioop.lin2adpcm(b'\0\0\0\0', 1, None), (b'\0\0', (0,0)))

    def test_lin2alaw(self):
        self.assertEqual(audioop.lin2alaw(data[0], 1), b'\xd5\xc5\xf5')
        self.assertEqual(audioop.lin2alaw(data[1], 2), b'\xd5\xd5\xd5')
        self.assertEqual(audioop.lin2alaw(data[2], 4), b'\xd5\xd5\xd5')

    def test_alaw2lin(self):
        # Cursory
        d = audioop.lin2alaw(data[0], 1)
        self.assertEqual(audioop.alaw2lin(d, 1), data[0])
        if endian == 'big':
            self.assertEqual(audioop.alaw2lin(d, 2),
                             b'\x00\x08\x01\x08\x02\x10')
            self.assertEqual(audioop.alaw2lin(d, 4),
                             b'\x00\x08\x00\x00\x01\x08\x00\x00\x02\x10\x00\x00')
        else:
            self.assertEqual(audioop.alaw2lin(d, 2),
                             b'\x08\x00\x08\x01\x10\x02')
            self.assertEqual(audioop.alaw2lin(d, 4),
                             b'\x00\x00\x08\x00\x00\x00\x08\x01\x00\x00\x10\x02')

    def test_lin2ulaw(self):
        self.assertEqual(audioop.lin2ulaw(data[0], 1), b'\xff\xe7\xdb')
        self.assertEqual(audioop.lin2ulaw(data[1], 2), b'\xff\xff\xff')
        self.assertEqual(audioop.lin2ulaw(data[2], 4), b'\xff\xff\xff')

    def test_ulaw2lin(self):
        # Cursory
        d = audioop.lin2ulaw(data[0], 1)
        self.assertEqual(audioop.ulaw2lin(d, 1), data[0])
        if endian == 'big':
            self.assertEqual(audioop.ulaw2lin(d, 2),
                             b'\x00\x00\x01\x04\x02\x0c')
            self.assertEqual(audioop.ulaw2lin(d, 4),
                             b'\x00\x00\x00\x00\x01\x04\x00\x00\x02\x0c\x00\x00')
        else:
            self.assertEqual(audioop.ulaw2lin(d, 2),
                             b'\x00\x00\x04\x01\x0c\x02')
            self.assertEqual(audioop.ulaw2lin(d, 4),
                             b'\x00\x00\x00\x00\x00\x00\x04\x01\x00\x00\x0c\x02')

    def test_mul(self):
        data2 = []
        for d in data:
            str = bytearray(len(d))
            for i,b in enumerate(d):
                str[i] = 2*b
            data2.append(str)
        self.assertEqual(audioop.mul(data[0], 1, 2), data2[0])
        self.assertEqual(audioop.mul(data[1],2, 2), data2[1])
        self.assertEqual(audioop.mul(data[2], 4, 2), data2[2])

    def test_ratecv(self):
        state = None
        d1, state = audioop.ratecv(data[0], 1, 1, 8000, 16000, state)
        d2, state = audioop.ratecv(data[0], 1, 1, 8000, 16000, state)
        self.assertEqual(d1 + d2, b'\000\000\001\001\002\001\000\000\001\001\002')

    def test_reverse(self):
        self.assertEqual(audioop.reverse(data[0], 1), b'\2\1\0')

    def test_tomono(self):
        data2 = bytearray()
        for d in data[0]:
            data2.append(d)
            data2.append(d)
        self.assertEqual(audioop.tomono(data2, 1, 0.5, 0.5), data[0])

    def test_tostereo(self):
        data2 = bytearray()
        for d in data[0]:
            data2.append(d)
            data2.append(d)
        self.assertEqual(audioop.tostereo(data[0], 1, 1, 1), data2)

    def test_findfactor(self):
        self.assertEqual(audioop.findfactor(data[1], data[1]), 1.0)

    def test_findfit(self):
        self.assertEqual(audioop.findfit(data[1], data[1]), (0, 1.0))

    def test_findmax(self):
        self.assertEqual(audioop.findmax(data[1], 1), 2)

    def test_getsample(self):
        for i in range(3):
            self.assertEqual(audioop.getsample(data[0], 1, i), i)
            self.assertEqual(audioop.getsample(data[1], 2, i), i)
            self.assertEqual(audioop.getsample(data[2], 4, i), i)

    def test_negativelen(self):
        # from issue 3306, previously it segfaulted
        self.assertRaises(audioop.error,
            audioop.findmax, ''.join(chr(x) for x in range(256)), -2392392)

    def test_issue7673(self):
        state = None
        for data, size in INVALID_DATA:
            size2 = size
            self.assertRaises(audioop.error, audioop.getsample, data, size, 0)
            self.assertRaises(audioop.error, audioop.max, data, size)
            self.assertRaises(audioop.error, audioop.minmax, data, size)
            self.assertRaises(audioop.error, audioop.avg, data, size)
            self.assertRaises(audioop.error, audioop.rms, data, size)
            self.assertRaises(audioop.error, audioop.avgpp, data, size)
            self.assertRaises(audioop.error, audioop.maxpp, data, size)
            self.assertRaises(audioop.error, audioop.cross, data, size)
            self.assertRaises(audioop.error, audioop.mul, data, size, 1.0)
            self.assertRaises(audioop.error, audioop.tomono, data, size, 0.5, 0.5)
            self.assertRaises(audioop.error, audioop.tostereo, data, size, 0.5, 0.5)
            self.assertRaises(audioop.error, audioop.add, data, data, size)
            self.assertRaises(audioop.error, audioop.bias, data, size, 0)
            self.assertRaises(audioop.error, audioop.reverse, data, size)
            self.assertRaises(audioop.error, audioop.lin2lin, data, size, size2)
            self.assertRaises(audioop.error, audioop.ratecv, data, size, 1, 1, 1, state)
            self.assertRaises(audioop.error, audioop.lin2ulaw, data, size)
            self.assertRaises(audioop.error, audioop.lin2alaw, data, size)
            self.assertRaises(audioop.error, audioop.lin2adpcm, data, size, state)

    def test_wrongsize(self):
        data = b'abc'
        state = None
        for size in (-1, 3, 5):
            self.assertRaises(audioop.error, audioop.ulaw2lin, data, size)
            self.assertRaises(audioop.error, audioop.alaw2lin, data, size)
            self.assertRaises(audioop.error, audioop.adpcm2lin, data, size, state)

def test_main():
    run_unittest(TestAudioop)

if __name__ == '__main__':
    test_main()
