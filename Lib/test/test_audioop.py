import audioop
import sys
import unittest
import struct
from test.test_support import run_unittest


formats = {
    1: 'b',
    2: 'h',
    4: 'i',
}

def pack(width, data):
    return struct.pack('=%d%s' % (len(data), formats[width]), *data)

packs = {
    1: lambda *data: pack(1, data),
    2: lambda *data: pack(2, data),
    4: lambda *data: pack(4, data),
}
maxvalues = {w: (1 << (8 * w - 1)) - 1 for w in (1, 2, 4)}
minvalues = {w: -1 << (8 * w - 1) for w in (1, 2, 4)}

datas = {
    1: b'\x00\x12\x45\xbb\x7f\x80\xff',
    2: packs[2](0, 0x1234, 0x4567, -0x4567, 0x7fff, -0x8000, -1),
    4: packs[4](0, 0x12345678, 0x456789ab, -0x456789ab,
                0x7fffffff, -0x80000000, -1),
}

INVALID_DATA = [
    (b'abc', 0),
    (b'abc', 2),
    (b'abc', 4),
]


class TestAudioop(unittest.TestCase):

    def test_max(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.max(b'', w), 0)
            p = packs[w]
            self.assertEqual(audioop.max(p(5), w), 5)
            self.assertEqual(audioop.max(p(5, -8, -1), w), 8)
            self.assertEqual(audioop.max(p(maxvalues[w]), w), maxvalues[w])
            self.assertEqual(audioop.max(p(minvalues[w]), w), -minvalues[w])
            self.assertEqual(audioop.max(datas[w], w), -minvalues[w])

    def test_minmax(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.minmax(b'', w),
                             (0x7fffffff, -0x80000000))
            p = packs[w]
            self.assertEqual(audioop.minmax(p(5), w), (5, 5))
            self.assertEqual(audioop.minmax(p(5, -8, -1), w), (-8, 5))
            self.assertEqual(audioop.minmax(p(maxvalues[w]), w),
                             (maxvalues[w], maxvalues[w]))
            self.assertEqual(audioop.minmax(p(minvalues[w]), w),
                             (minvalues[w], minvalues[w]))
            self.assertEqual(audioop.minmax(datas[w], w),
                             (minvalues[w], maxvalues[w]))

    def test_maxpp(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.maxpp(b'', w), 0)
            self.assertEqual(audioop.maxpp(packs[w](*range(100)), w), 0)
            self.assertEqual(audioop.maxpp(packs[w](9, 10, 5, 5, 0, 1), w), 10)
            self.assertEqual(audioop.maxpp(datas[w], w),
                             maxvalues[w] - minvalues[w])

    def test_avg(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.avg(b'', w), 0)
            p = packs[w]
            self.assertEqual(audioop.avg(p(5), w), 5)
            self .assertEqual(audioop.avg(p(5, 8), w), 6)
            self.assertEqual(audioop.avg(p(5, -8), w), -2)
            self.assertEqual(audioop.avg(p(maxvalues[w], maxvalues[w]), w),
                             maxvalues[w])
            self.assertEqual(audioop.avg(p(minvalues[w], minvalues[w]), w),
                             minvalues[w])
        self.assertEqual(audioop.avg(packs[4](0x50000000, 0x70000000), 4),
                         0x60000000)
        self.assertEqual(audioop.avg(packs[4](-0x50000000, -0x70000000), 4),
                         -0x60000000)

    def test_avgpp(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.avgpp(b'', w), 0)
            self.assertEqual(audioop.avgpp(packs[w](*range(100)), w), 0)
            self.assertEqual(audioop.avgpp(packs[w](9, 10, 5, 5, 0, 1), w), 10)
        self.assertEqual(audioop.avgpp(datas[1], 1), 196)
        self.assertEqual(audioop.avgpp(datas[2], 2), 50534)
        self.assertEqual(audioop.avgpp(datas[4], 4), 3311897002)

    def test_rms(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.rms(b'', w), 0)
            p = packs[w]
            self.assertEqual(audioop.rms(p(*range(100)), w), 57)
            self.assertAlmostEqual(audioop.rms(p(maxvalues[w]) * 5, w),
                                   maxvalues[w], delta=1)
            self.assertAlmostEqual(audioop.rms(p(minvalues[w]) * 5, w),
                                   -minvalues[w], delta=1)
        self.assertEqual(audioop.rms(datas[1], 1), 77)
        self.assertEqual(audioop.rms(datas[2], 2), 20001)
        self.assertEqual(audioop.rms(datas[4], 4), 1310854152)

    def test_cross(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.cross(b'', w), -1)
            p = packs[w]
            self.assertEqual(audioop.cross(p(0, 1, 2), w), 0)
            self.assertEqual(audioop.cross(p(1, 2, -3, -4), w), 1)
            self.assertEqual(audioop.cross(p(-1, -2, 3, 4), w), 1)
            self.assertEqual(audioop.cross(p(0, minvalues[w]), w), 1)
            self.assertEqual(audioop.cross(p(minvalues[w], maxvalues[w]), w), 1)

    def test_add(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.add(b'', b'', w), b'')
            self.assertEqual(audioop.add(datas[w], b'\0' * len(datas[w]), w),
                             datas[w])
        self.assertEqual(audioop.add(datas[1], datas[1], 1),
                         b'\x00\x24\x7f\x80\x7f\x80\xfe')
        self.assertEqual(audioop.add(datas[2], datas[2], 2),
                packs[2](0, 0x2468, 0x7fff, -0x8000, 0x7fff, -0x8000, -2))
        self.assertEqual(audioop.add(datas[4], datas[4], 4),
                packs[4](0, 0x2468acf0, 0x7fffffff, -0x80000000,
                       0x7fffffff, -0x80000000, -2))

    def test_bias(self):
        for w in 1, 2, 4:
            for bias in 0, 1, -1, 127, -128, 0x7fffffff, -0x80000000:
                self.assertEqual(audioop.bias(b'', w, bias), b'')
        self.assertEqual(audioop.bias(datas[1], 1, 1),
                         b'\x01\x13\x46\xbc\x80\x81\x00')
        self.assertEqual(audioop.bias(datas[1], 1, -1),
                         b'\xff\x11\x44\xba\x7e\x7f\xfe')
        self.assertEqual(audioop.bias(datas[1], 1, 0x7fffffff),
                         b'\xff\x11\x44\xba\x7e\x7f\xfe')
        self.assertEqual(audioop.bias(datas[1], 1, -0x80000000),
                         datas[1])
        self.assertEqual(audioop.bias(datas[2], 2, 1),
                packs[2](1, 0x1235, 0x4568, -0x4566, -0x8000, -0x7fff, 0))
        self.assertEqual(audioop.bias(datas[2], 2, -1),
                packs[2](-1, 0x1233, 0x4566, -0x4568, 0x7ffe, 0x7fff, -2))
        self.assertEqual(audioop.bias(datas[2], 2, 0x7fffffff),
                packs[2](-1, 0x1233, 0x4566, -0x4568, 0x7ffe, 0x7fff, -2))
        self.assertEqual(audioop.bias(datas[2], 2, -0x80000000),
                datas[2])
        self.assertEqual(audioop.bias(datas[4], 4, 1),
                packs[4](1, 0x12345679, 0x456789ac, -0x456789aa,
                         -0x80000000, -0x7fffffff, 0))
        self.assertEqual(audioop.bias(datas[4], 4, -1),
                packs[4](-1, 0x12345677, 0x456789aa, -0x456789ac,
                         0x7ffffffe, 0x7fffffff, -2))
        self.assertEqual(audioop.bias(datas[4], 4, 0x7fffffff),
                packs[4](0x7fffffff, -0x6dcba989, -0x3a987656, 0x3a987654,
                         -2, -1, 0x7ffffffe))
        self.assertEqual(audioop.bias(datas[4], 4, -0x80000000),
                packs[4](-0x80000000, -0x6dcba988, -0x3a987655, 0x3a987655,
                         -1, 0, 0x7fffffff))

    def test_lin2lin(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.lin2lin(datas[w], w, w), datas[w])

        self.assertEqual(audioop.lin2lin(datas[1], 1, 2),
            packs[2](0, 0x1200, 0x4500, -0x4500, 0x7f00, -0x8000, -0x100))
        self.assertEqual(audioop.lin2lin(datas[1], 1, 4),
            packs[4](0, 0x12000000, 0x45000000, -0x45000000,
                     0x7f000000, -0x80000000, -0x1000000))
        self.assertEqual(audioop.lin2lin(datas[2], 2, 1),
            b'\x00\x12\x45\xba\x7f\x80\xff')
        self.assertEqual(audioop.lin2lin(datas[2], 2, 4),
            packs[4](0, 0x12340000, 0x45670000, -0x45670000,
                     0x7fff0000, -0x80000000, -0x10000))
        self.assertEqual(audioop.lin2lin(datas[4], 4, 1),
            b'\x00\x12\x45\xba\x7f\x80\xff')
        self.assertEqual(audioop.lin2lin(datas[4], 4, 2),
            packs[2](0, 0x1234, 0x4567, -0x4568, 0x7fff, -0x8000, -1))

    def test_adpcm2lin(self):
        self.assertEqual(audioop.adpcm2lin(b'\x07\x7f\x7f', 1, None),
                         (b'\x00\x00\x00\xff\x00\xff', (-179, 40)))
        self.assertEqual(audioop.adpcm2lin(b'\x07\x7f\x7f', 2, None),
                         (packs[2](0, 0xb, 0x29, -0x16, 0x72, -0xb3), (-179, 40)))
        self.assertEqual(audioop.adpcm2lin(b'\x07\x7f\x7f', 4, None),
                         (packs[4](0, 0xb0000, 0x290000, -0x160000, 0x720000,
                                   -0xb30000), (-179, 40)))

        # Very cursory test
        for w in 1, 2, 4:
            self.assertEqual(audioop.adpcm2lin(b'\0' * 5, w, None),
                             (b'\0' * w * 10, (0, 0)))

    def test_lin2adpcm(self):
        self.assertEqual(audioop.lin2adpcm(datas[1], 1, None),
                         (b'\x07\x7f\x7f', (-221, 39)))
        self.assertEqual(audioop.lin2adpcm(datas[2], 2, None),
                         (b'\x07\x7f\x7f', (31, 39)))
        self.assertEqual(audioop.lin2adpcm(datas[4], 4, None),
                         (b'\x07\x7f\x7f', (31, 39)))

        # Very cursory test
        for w in 1, 2, 4:
            self.assertEqual(audioop.lin2adpcm(b'\0' * w * 10, w, None),
                             (b'\0' * 5, (0, 0)))

    def test_invalid_adpcm_state(self):
        # state must be a tuple or None, not an integer
        self.assertRaises(TypeError, audioop.adpcm2lin, b'\0', 1, 555)
        self.assertRaises(TypeError, audioop.lin2adpcm, b'\0', 1, 555)
        # Issues #24456, #24457: index out of range
        self.assertRaises(ValueError, audioop.adpcm2lin, b'\0', 1, (0, -1))
        self.assertRaises(ValueError, audioop.adpcm2lin, b'\0', 1, (0, 89))
        self.assertRaises(ValueError, audioop.lin2adpcm, b'\0', 1, (0, -1))
        self.assertRaises(ValueError, audioop.lin2adpcm, b'\0', 1, (0, 89))
        # value out of range
        self.assertRaises(ValueError, audioop.adpcm2lin, b'\0', 1, (-0x8001, 0))
        self.assertRaises(ValueError, audioop.adpcm2lin, b'\0', 1, (0x8000, 0))
        self.assertRaises(ValueError, audioop.lin2adpcm, b'\0', 1, (-0x8001, 0))
        self.assertRaises(ValueError, audioop.lin2adpcm, b'\0', 1, (0x8000, 0))

    def test_lin2alaw(self):
        self.assertEqual(audioop.lin2alaw(datas[1], 1),
                         b'\xd5\x87\xa4\x24\xaa\x2a\x5a')
        self.assertEqual(audioop.lin2alaw(datas[2], 2),
                         b'\xd5\x87\xa4\x24\xaa\x2a\x55')
        self.assertEqual(audioop.lin2alaw(datas[4], 4),
                         b'\xd5\x87\xa4\x24\xaa\x2a\x55')

    def test_alaw2lin(self):
        encoded = b'\x00\x03\x24\x2a\x51\x54\x55\x58\x6b\x71\x7f'\
                  b'\x80\x83\xa4\xaa\xd1\xd4\xd5\xd8\xeb\xf1\xff'
        src = [-688, -720, -2240, -4032, -9, -3, -1, -27, -244, -82, -106,
               688, 720, 2240, 4032, 9, 3, 1, 27, 244, 82, 106]
        for w in 1, 2, 4:
            self.assertEqual(audioop.alaw2lin(encoded, w),
                             packs[w](*(x << (w * 8) >> 13 for x in src)))

        encoded = ''.join(chr(x) for x in xrange(256))
        for w in 2, 4:
            decoded = audioop.alaw2lin(encoded, w)
            self.assertEqual(audioop.lin2alaw(decoded, w), encoded)

    def test_lin2ulaw(self):
        self.assertEqual(audioop.lin2ulaw(datas[1], 1),
                         b'\xff\xad\x8e\x0e\x80\x00\x67')
        self.assertEqual(audioop.lin2ulaw(datas[2], 2),
                         b'\xff\xad\x8e\x0e\x80\x00\x7e')
        self.assertEqual(audioop.lin2ulaw(datas[4], 4),
                         b'\xff\xad\x8e\x0e\x80\x00\x7e')

    def test_ulaw2lin(self):
        encoded = b'\x00\x0e\x28\x3f\x57\x6a\x76\x7c\x7e\x7f'\
                  b'\x80\x8e\xa8\xbf\xd7\xea\xf6\xfc\xfe\xff'
        src = [-8031, -4447, -1471, -495, -163, -53, -18, -6, -2, 0,
               8031, 4447, 1471, 495, 163, 53, 18, 6, 2, 0]
        for w in 1, 2, 4:
            self.assertEqual(audioop.ulaw2lin(encoded, w),
                             packs[w](*(x << (w * 8) >> 14 for x in src)))

        # Current u-law implementation has two codes fo 0: 0x7f and 0xff.
        encoded = ''.join(chr(x) for x in range(127) + range(128, 256))
        for w in 2, 4:
            decoded = audioop.ulaw2lin(encoded, w)
            self.assertEqual(audioop.lin2ulaw(decoded, w), encoded)

    def test_mul(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.mul(b'', w, 2), b'')
            self.assertEqual(audioop.mul(datas[w], w, 0),
                             b'\0' * len(datas[w]))
            self.assertEqual(audioop.mul(datas[w], w, 1),
                             datas[w])
        self.assertEqual(audioop.mul(datas[1], 1, 2),
                         b'\x00\x24\x7f\x80\x7f\x80\xfe')
        self.assertEqual(audioop.mul(datas[2], 2, 2),
                packs[2](0, 0x2468, 0x7fff, -0x8000, 0x7fff, -0x8000, -2))
        self.assertEqual(audioop.mul(datas[4], 4, 2),
                packs[4](0, 0x2468acf0, 0x7fffffff, -0x80000000,
                         0x7fffffff, -0x80000000, -2))

    def test_ratecv(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.ratecv(b'', w, 1, 8000, 8000, None),
                             (b'', (-1, ((0, 0),))))
            self.assertEqual(audioop.ratecv(b'', w, 5, 8000, 8000, None),
                             (b'', (-1, ((0, 0),) * 5)))
            self.assertEqual(audioop.ratecv(b'', w, 1, 8000, 16000, None),
                             (b'', (-2, ((0, 0),))))
            self.assertEqual(audioop.ratecv(datas[w], w, 1, 8000, 8000, None)[0],
                             datas[w])
            self.assertEqual(audioop.ratecv(datas[w], w, 1, 8000, 8000, None, 1, 0)[0],
                             datas[w])

        state = None
        d1, state = audioop.ratecv(b'\x00\x01\x02', 1, 1, 8000, 16000, state)
        d2, state = audioop.ratecv(b'\x00\x01\x02', 1, 1, 8000, 16000, state)
        self.assertEqual(d1 + d2, b'\000\000\001\001\002\001\000\000\001\001\002')

        for w in 1, 2, 4:
            d0, state0 = audioop.ratecv(datas[w], w, 1, 8000, 16000, None)
            d, state = b'', None
            for i in range(0, len(datas[w]), w):
                d1, state = audioop.ratecv(datas[w][i:i + w], w, 1,
                                           8000, 16000, state)
                d += d1
            self.assertEqual(d, d0)
            self.assertEqual(state, state0)

        expected = {
            1: packs[1](0, 0x0d, 0x37, -0x26, 0x55, -0x4b, -0x14),
            2: packs[2](0, 0x0da7, 0x3777, -0x2630, 0x5673, -0x4a64, -0x129a),
            4: packs[4](0, 0x0da740da, 0x37777776, -0x262fc962,
                        0x56740da6, -0x4a62fc96, -0x1298bf26),
        }
        for w in 1, 2, 4:
            self.assertEqual(audioop.ratecv(datas[w], w, 1, 8000, 8000, None, 3, 1)[0],
                             expected[w])
            self.assertEqual(audioop.ratecv(datas[w], w, 1, 8000, 8000, None, 30, 10)[0],
                             expected[w])

    def test_reverse(self):
        for w in 1, 2, 4:
            self.assertEqual(audioop.reverse(b'', w), b'')
            self.assertEqual(audioop.reverse(packs[w](0, 1, 2), w),
                             packs[w](2, 1, 0))

    def test_tomono(self):
        for w in 1, 2, 4:
            data1 = datas[w]
            data2 = bytearray(2 * len(data1))
            for k in range(w):
                data2[k::2*w] = data1[k::w]
            self.assertEqual(audioop.tomono(str(data2), w, 1, 0), data1)
            self.assertEqual(audioop.tomono(str(data2), w, 0, 1), b'\0' * len(data1))
            for k in range(w):
                data2[k+w::2*w] = data1[k::w]
            self.assertEqual(audioop.tomono(str(data2), w, 0.5, 0.5), data1)

    def test_tostereo(self):
        for w in 1, 2, 4:
            data1 = datas[w]
            data2 = bytearray(2 * len(data1))
            for k in range(w):
                data2[k::2*w] = data1[k::w]
            self.assertEqual(audioop.tostereo(data1, w, 1, 0), data2)
            self.assertEqual(audioop.tostereo(data1, w, 0, 0), b'\0' * len(data2))
            for k in range(w):
                data2[k+w::2*w] = data1[k::w]
            self.assertEqual(audioop.tostereo(data1, w, 1, 1), data2)

    def test_findfactor(self):
        self.assertEqual(audioop.findfactor(datas[2], datas[2]), 1.0)
        self.assertEqual(audioop.findfactor(b'\0' * len(datas[2]), datas[2]),
                         0.0)

    def test_findfit(self):
        self.assertEqual(audioop.findfit(datas[2], datas[2]), (0, 1.0))
        self.assertEqual(audioop.findfit(datas[2], packs[2](1, 2, 0)),
                         (1, 8038.8))
        self.assertEqual(audioop.findfit(datas[2][:-2] * 5 + datas[2], datas[2]),
                         (30, 1.0))

    def test_findmax(self):
        self.assertEqual(audioop.findmax(datas[2], 1), 5)

    def test_getsample(self):
        for w in 1, 2, 4:
            data = packs[w](0, 1, -1, maxvalues[w], minvalues[w])
            self.assertEqual(audioop.getsample(data, w, 0), 0)
            self.assertEqual(audioop.getsample(data, w, 1), 1)
            self.assertEqual(audioop.getsample(data, w, 2), -1)
            self.assertEqual(audioop.getsample(data, w, 3), maxvalues[w])
            self.assertEqual(audioop.getsample(data, w, 4), minvalues[w])

    def test_negativelen(self):
        # from issue 3306, previously it segfaulted
        self.assertRaises(audioop.error,
            audioop.findmax, ''.join( chr(x) for x in xrange(256)), -2392392)

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
        data = b'abcdefgh'
        state = None
        for size in (-1, 0, 3, 5, 1024):
            self.assertRaises(audioop.error, audioop.ulaw2lin, data, size)
            self.assertRaises(audioop.error, audioop.alaw2lin, data, size)
            self.assertRaises(audioop.error, audioop.adpcm2lin, data, size, state)

def test_main():
    run_unittest(TestAudioop)

if __name__ == '__main__':
    test_main()
