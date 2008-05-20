import unittest, test.support
import colorsys

def frange(start, stop, step):
    while start <= stop:
        yield start
        start += step

class ColorsysTest(unittest.TestCase):

    def assertTripleEqual(self, tr1, tr2):
        self.assertEqual(len(tr1), 3)
        self.assertEqual(len(tr2), 3)
        self.assertAlmostEqual(tr1[0], tr2[0])
        self.assertAlmostEqual(tr1[1], tr2[1])
        self.assertAlmostEqual(tr1[2], tr2[2])

    def test_hsv_roundtrip(self):
        for r in frange(0.0, 1.0, 0.2):
            for g in frange(0.0, 1.0, 0.2):
                for b in frange(0.0, 1.0, 0.2):
                    rgb = (r, g, b)
                    self.assertTripleEqual(
                        rgb,
                        colorsys.hsv_to_rgb(*colorsys.rgb_to_hsv(*rgb))
                    )

    def test_hsv_values(self):
        values = [
            # rgb, hsv
            ((0.0, 0.0, 0.0), (  0  , 0.0, 0.0)), # black
            ((0.0, 0.0, 1.0), (4./6., 1.0, 1.0)), # blue
            ((0.0, 1.0, 0.0), (2./6., 1.0, 1.0)), # green
            ((0.0, 1.0, 1.0), (3./6., 1.0, 1.0)), # cyan
            ((1.0, 0.0, 0.0), (  0  , 1.0, 1.0)), # red
            ((1.0, 0.0, 1.0), (5./6., 1.0, 1.0)), # purple
            ((1.0, 1.0, 0.0), (1./6., 1.0, 1.0)), # yellow
            ((1.0, 1.0, 1.0), (  0  , 0.0, 1.0)), # white
            ((0.5, 0.5, 0.5), (  0  , 0.0, 0.5)), # grey
        ]
        for (rgb, hsv) in values:
            self.assertTripleEqual(hsv, colorsys.rgb_to_hsv(*rgb))
            self.assertTripleEqual(rgb, colorsys.hsv_to_rgb(*hsv))

    def test_hls_roundtrip(self):
        for r in frange(0.0, 1.0, 0.2):
            for g in frange(0.0, 1.0, 0.2):
                for b in frange(0.0, 1.0, 0.2):
                    rgb = (r, g, b)
                    self.assertTripleEqual(
                        rgb,
                        colorsys.hls_to_rgb(*colorsys.rgb_to_hls(*rgb))
                    )

    def test_hls_values(self):
        values = [
            # rgb, hls
            ((0.0, 0.0, 0.0), (  0  , 0.0, 0.0)), # black
            ((0.0, 0.0, 1.0), (4./6., 0.5, 1.0)), # blue
            ((0.0, 1.0, 0.0), (2./6., 0.5, 1.0)), # green
            ((0.0, 1.0, 1.0), (3./6., 0.5, 1.0)), # cyan
            ((1.0, 0.0, 0.0), (  0  , 0.5, 1.0)), # red
            ((1.0, 0.0, 1.0), (5./6., 0.5, 1.0)), # purple
            ((1.0, 1.0, 0.0), (1./6., 0.5, 1.0)), # yellow
            ((1.0, 1.0, 1.0), (  0  , 1.0, 0.0)), # white
            ((0.5, 0.5, 0.5), (  0  , 0.5, 0.0)), # grey
        ]
        for (rgb, hls) in values:
            self.assertTripleEqual(hls, colorsys.rgb_to_hls(*rgb))
            self.assertTripleEqual(rgb, colorsys.hls_to_rgb(*hls))

def test_main():
    test.support.run_unittest(ColorsysTest)

if __name__ == "__main__":
    test_main()
