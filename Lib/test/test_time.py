import test_support
import time
import unittest


class TimeTestCase(unittest.TestCase):

    def setUp(self):
        self.t = time.time()

    def test_data_attributes(self):
        time.altzone
        time.daylight
        time.timezone
        time.tzname

    def test_clock(self):
        time.clock()

    def test_conversions(self):
        self.assert_(time.ctime(self.t)
                     == time.asctime(time.localtime(self.t)))
        self.assert_(long(time.mktime(time.localtime(self.t)))
                     == long(self.t))

    def test_sleep(self):
        time.sleep(1.2)

    def test_strftime(self):
        tt = time.gmtime(self.t)
        for directive in ('a', 'A', 'b', 'B', 'c', 'd', 'H', 'I',
                          'j', 'm', 'M', 'p', 'S',
                          'U', 'w', 'W', 'x', 'X', 'y', 'Y', 'Z', '%'):
            format = ' %' + directive
            try:
                time.strftime(format, tt)
            except ValueError:
                self.fail('conversion specifier: %r failed.' % format)

    def test_asctime(self):
        time.asctime(time.gmtime(self.t))
        self.assertRaises(TypeError, time.asctime, 0)

    def test_mktime(self):
        self.assertRaises(OverflowError,
                          time.mktime, (999999, 999999, 999999, 999999,
                                        999999, 999999, 999999, 999999,
                                        999999))


def test_main():
    test_support.run_unittest(TimeTestCase)


if __name__ == "__main__":
    test_main()
