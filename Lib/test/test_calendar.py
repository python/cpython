import calendar
import unittest

from test_support import run_unittest


class CalendarTestCase(unittest.TestCase):
    def test_isleap(self):
        # Make sure that the return is right for a few years, and
        # ensure that the return values are 1 or 0, not just true or
        # false (see SF bug #485794).  Specific additional tests may
        # be appropriate; this tests a single "cycle".
        self.assertEqual(calendar.isleap(2000), 1)
        self.assertEqual(calendar.isleap(2001), 0)
        self.assertEqual(calendar.isleap(2002), 0)
        self.assertEqual(calendar.isleap(2003), 0)

    def test_setfirstweekday(self):
        self.assertRaises(ValueError, calendar.setfirstweekday, 'flabber')
        self.assertRaises(ValueError, calendar.setfirstweekday, -1)
        self.assertRaises(ValueError, calendar.setfirstweekday, 200)
        orig = calendar.firstweekday()
        calendar.setfirstweekday(calendar.SUNDAY)
        self.assertEqual(calendar.firstweekday(), calendar.SUNDAY)
        calendar.setfirstweekday(calendar.MONDAY)
        self.assertEqual(calendar.firstweekday(), calendar.MONDAY)
        calendar.setfirstweekday(orig)

    def test_enumerateweekdays(self):
        self.assertRaises(IndexError, calendar.day_abbr.__getitem__, -10)
        self.assertRaises(IndexError, calendar.day_name.__getitem__, 10)
        self.assertEqual(len([d for d in calendar.day_abbr]), 7)

def test_main():
    run_unittest(CalendarTestCase)

if __name__ == "__main__":
    test_main()
