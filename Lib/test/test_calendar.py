import calendar
import unittest

from test import test_support


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

    def test_days(self):
        for attr in "day_name", "day_abbr":
            value = getattr(calendar, attr)
            self.assertEqual(len(value), 7)
            self.assertEqual(len(value[:]), 7)
            # ensure they're all unique
            self.assertEqual(len(set(value)), 7)
            # verify it "acts like a sequence" in two forms of iteration
            self.assertEqual(value[::-1], list(reversed(value)))

    def test_months(self):
        for attr in "month_name", "month_abbr":
            value = getattr(calendar, attr)
            self.assertEqual(len(value), 13)
            self.assertEqual(len(value[:]), 13)
            self.assertEqual(value[0], "")
            # ensure they're all unique
            self.assertEqual(len(set(value)), 13)
            # verify it "acts like a sequence" in two forms of iteration
            self.assertEqual(value[::-1], list(reversed(value)))


class MonthCalendarTestCase(unittest.TestCase):
    def setUp(self):
        self.oldfirstweekday = calendar.firstweekday()
        calendar.setfirstweekday(self.firstweekday)

    def tearDown(self):
        calendar.setfirstweekday(self.oldfirstweekday)

    def check_weeks(self, year, month, weeks):
        cal = calendar.monthcalendar(year, month)
        self.assertEqual(len(cal), len(weeks))
        for i in xrange(len(weeks)):
            self.assertEqual(weeks[i], sum(day != 0 for day in cal[i]))


class MondayTestCase(MonthCalendarTestCase):
    firstweekday = calendar.MONDAY

    def test_february(self):
        # A 28-day february starting of monday (7+7+7+7 days)
        self.check_weeks(1999, 2, (7, 7, 7, 7))

        # A 28-day february starting of tuesday (6+7+7+7+1 days)
        self.check_weeks(2005, 2, (6, 7, 7, 7, 1))

        # A 28-day february starting of sunday (1+7+7+7+6 days)
        self.check_weeks(1987, 2, (1, 7, 7, 7, 6))

        # A 29-day february starting of monday (7+7+7+7+1 days)
        self.check_weeks(1988, 2, (7, 7, 7, 7, 1))

        # A 29-day february starting of tuesday (6+7+7+7+2 days)
        self.check_weeks(1972, 2, (6, 7, 7, 7, 2))

        # A 29-day february starting of sunday (1+7+7+7+7 days)
        self.check_weeks(2004, 2, (1, 7, 7, 7, 7))

    def test_april(self):
        # A 30-day april starting of monday (7+7+7+7+2 days)
        self.check_weeks(1935, 4, (7, 7, 7, 7, 2))

        # A 30-day april starting of tuesday (6+7+7+7+3 days)
        self.check_weeks(1975, 4, (6, 7, 7, 7, 3))

        # A 30-day april starting of sunday (1+7+7+7+7+1 days)
        self.check_weeks(1945, 4, (1, 7, 7, 7, 7, 1))

        # A 30-day april starting of saturday (2+7+7+7+7 days)
        self.check_weeks(1995, 4, (2, 7, 7, 7, 7))

        # A 30-day april starting of friday (3+7+7+7+6 days)
        self.check_weeks(1994, 4, (3, 7, 7, 7, 6))

    def test_december(self):
        # A 31-day december starting of monday (7+7+7+7+3 days)
        self.check_weeks(1980, 12, (7, 7, 7, 7, 3))

        # A 31-day december starting of tuesday (6+7+7+7+4 days)
        self.check_weeks(1987, 12, (6, 7, 7, 7, 4))

        # A 31-day december starting of sunday (1+7+7+7+7+2 days)
        self.check_weeks(1968, 12, (1, 7, 7, 7, 7, 2))

        # A 31-day december starting of thursday (4+7+7+7+6 days)
        self.check_weeks(1988, 12, (4, 7, 7, 7, 6))

        # A 31-day december starting of friday (3+7+7+7+7 days)
        self.check_weeks(2017, 12, (3, 7, 7, 7, 7))

        # A 31-day december starting of saturday (2+7+7+7+7+1 days)
        self.check_weeks(2068, 12, (2, 7, 7, 7, 7, 1))


class SundayTestCase(MonthCalendarTestCase):
    firstweekday = calendar.SUNDAY

    def test_february(self):
        # A 28-day february starting of sunday (7+7+7+7 days)
        self.check_weeks(2009, 2, (7, 7, 7, 7))

        # A 28-day february starting of monday (6+7+7+7+1 days)
        self.check_weeks(1999, 2, (6, 7, 7, 7, 1))

        # A 28-day february starting of saturday (1+7+7+7+6 days)
        self.check_weeks(1997, 2, (1, 7, 7, 7, 6))

        # A 29-day february starting of sunday (7+7+7+7+1 days)
        self.check_weeks(2004, 2, (7, 7, 7, 7, 1))

        # A 29-day february starting of monday (6+7+7+7+2 days)
        self.check_weeks(1960, 2, (6, 7, 7, 7, 2))

        # A 29-day february starting of saturday (1+7+7+7+7 days)
        self.check_weeks(1964, 2, (1, 7, 7, 7, 7))

    def test_april(self):
        # A 30-day april starting of sunday (7+7+7+7+2 days)
        self.check_weeks(1923, 4, (7, 7, 7, 7, 2))

        # A 30-day april starting of monday (6+7+7+7+3 days)
        self.check_weeks(1918, 4, (6, 7, 7, 7, 3))

        # A 30-day april starting of saturday (1+7+7+7+7+1 days)
        self.check_weeks(1950, 4, (1, 7, 7, 7, 7, 1))

        # A 30-day april starting of friday (2+7+7+7+7 days)
        self.check_weeks(1960, 4, (2, 7, 7, 7, 7))

        # A 30-day april starting of thursday (3+7+7+7+6 days)
        self.check_weeks(1909, 4, (3, 7, 7, 7, 6))

    def test_december(self):
        # A 31-day december starting of sunday (7+7+7+7+3 days)
        self.check_weeks(2080, 12, (7, 7, 7, 7, 3))

        # A 31-day december starting of monday (6+7+7+7+4 days)
        self.check_weeks(1941, 12, (6, 7, 7, 7, 4))

        # A 31-day december starting of saturday (1+7+7+7+7+2 days)
        self.check_weeks(1923, 12, (1, 7, 7, 7, 7, 2))

        # A 31-day december starting of wednesday (4+7+7+7+6 days)
        self.check_weeks(1948, 12, (4, 7, 7, 7, 6))

        # A 31-day december starting of thursday (3+7+7+7+7 days)
        self.check_weeks(1927, 12, (3, 7, 7, 7, 7))

        # A 31-day december starting of friday (2+7+7+7+7+1 days)
        self.check_weeks(1995, 12, (2, 7, 7, 7, 7, 1))


def test_main():
    test_support.run_unittest(
        CalendarTestCase,
        MondayTestCase,
        SundayTestCase
    )


if __name__ == "__main__":
    test_main()
