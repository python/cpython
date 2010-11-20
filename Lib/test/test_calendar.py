import calendar
import unittest

from test import support
import time
import locale

result_2004_text = """
                                  2004

      January                   February                   March
Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su
          1  2  3  4                         1       1  2  3  4  5  6  7
 5  6  7  8  9 10 11       2  3  4  5  6  7  8       8  9 10 11 12 13 14
12 13 14 15 16 17 18       9 10 11 12 13 14 15      15 16 17 18 19 20 21
19 20 21 22 23 24 25      16 17 18 19 20 21 22      22 23 24 25 26 27 28
26 27 28 29 30 31         23 24 25 26 27 28 29      29 30 31

       April                      May                       June
Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su
          1  2  3  4                      1  2          1  2  3  4  5  6
 5  6  7  8  9 10 11       3  4  5  6  7  8  9       7  8  9 10 11 12 13
12 13 14 15 16 17 18      10 11 12 13 14 15 16      14 15 16 17 18 19 20
19 20 21 22 23 24 25      17 18 19 20 21 22 23      21 22 23 24 25 26 27
26 27 28 29 30            24 25 26 27 28 29 30      28 29 30
                          31

        July                     August                  September
Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su
          1  2  3  4                         1             1  2  3  4  5
 5  6  7  8  9 10 11       2  3  4  5  6  7  8       6  7  8  9 10 11 12
12 13 14 15 16 17 18       9 10 11 12 13 14 15      13 14 15 16 17 18 19
19 20 21 22 23 24 25      16 17 18 19 20 21 22      20 21 22 23 24 25 26
26 27 28 29 30 31         23 24 25 26 27 28 29      27 28 29 30
                          30 31

      October                   November                  December
Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su      Mo Tu We Th Fr Sa Su
             1  2  3       1  2  3  4  5  6  7             1  2  3  4  5
 4  5  6  7  8  9 10       8  9 10 11 12 13 14       6  7  8  9 10 11 12
11 12 13 14 15 16 17      15 16 17 18 19 20 21      13 14 15 16 17 18 19
18 19 20 21 22 23 24      22 23 24 25 26 27 28      20 21 22 23 24 25 26
25 26 27 28 29 30 31      29 30                     27 28 29 30 31
"""

result_2004_html = """
<?xml version="1.0" encoding="ascii"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ascii" />
<link rel="stylesheet" type="text/css" href="calendar.css" />
<title>Calendar for 2004</title>
</head>
<body>
<table border="0" cellpadding="0" cellspacing="0" class="year">
<tr><th colspan="3" class="year">2004</th></tr><tr><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">January</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="thu">1</td><td class="fri">2</td><td class="sat">3</td><td class="sun">4</td></tr>
<tr><td class="mon">5</td><td class="tue">6</td><td class="wed">7</td><td class="thu">8</td><td class="fri">9</td><td class="sat">10</td><td class="sun">11</td></tr>
<tr><td class="mon">12</td><td class="tue">13</td><td class="wed">14</td><td class="thu">15</td><td class="fri">16</td><td class="sat">17</td><td class="sun">18</td></tr>
<tr><td class="mon">19</td><td class="tue">20</td><td class="wed">21</td><td class="thu">22</td><td class="fri">23</td><td class="sat">24</td><td class="sun">25</td></tr>
<tr><td class="mon">26</td><td class="tue">27</td><td class="wed">28</td><td class="thu">29</td><td class="fri">30</td><td class="sat">31</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">February</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="sun">1</td></tr>
<tr><td class="mon">2</td><td class="tue">3</td><td class="wed">4</td><td class="thu">5</td><td class="fri">6</td><td class="sat">7</td><td class="sun">8</td></tr>
<tr><td class="mon">9</td><td class="tue">10</td><td class="wed">11</td><td class="thu">12</td><td class="fri">13</td><td class="sat">14</td><td class="sun">15</td></tr>
<tr><td class="mon">16</td><td class="tue">17</td><td class="wed">18</td><td class="thu">19</td><td class="fri">20</td><td class="sat">21</td><td class="sun">22</td></tr>
<tr><td class="mon">23</td><td class="tue">24</td><td class="wed">25</td><td class="thu">26</td><td class="fri">27</td><td class="sat">28</td><td class="sun">29</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">March</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="mon">1</td><td class="tue">2</td><td class="wed">3</td><td class="thu">4</td><td class="fri">5</td><td class="sat">6</td><td class="sun">7</td></tr>
<tr><td class="mon">8</td><td class="tue">9</td><td class="wed">10</td><td class="thu">11</td><td class="fri">12</td><td class="sat">13</td><td class="sun">14</td></tr>
<tr><td class="mon">15</td><td class="tue">16</td><td class="wed">17</td><td class="thu">18</td><td class="fri">19</td><td class="sat">20</td><td class="sun">21</td></tr>
<tr><td class="mon">22</td><td class="tue">23</td><td class="wed">24</td><td class="thu">25</td><td class="fri">26</td><td class="sat">27</td><td class="sun">28</td></tr>
<tr><td class="mon">29</td><td class="tue">30</td><td class="wed">31</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td></tr><tr><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">April</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="thu">1</td><td class="fri">2</td><td class="sat">3</td><td class="sun">4</td></tr>
<tr><td class="mon">5</td><td class="tue">6</td><td class="wed">7</td><td class="thu">8</td><td class="fri">9</td><td class="sat">10</td><td class="sun">11</td></tr>
<tr><td class="mon">12</td><td class="tue">13</td><td class="wed">14</td><td class="thu">15</td><td class="fri">16</td><td class="sat">17</td><td class="sun">18</td></tr>
<tr><td class="mon">19</td><td class="tue">20</td><td class="wed">21</td><td class="thu">22</td><td class="fri">23</td><td class="sat">24</td><td class="sun">25</td></tr>
<tr><td class="mon">26</td><td class="tue">27</td><td class="wed">28</td><td class="thu">29</td><td class="fri">30</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">May</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="sat">1</td><td class="sun">2</td></tr>
<tr><td class="mon">3</td><td class="tue">4</td><td class="wed">5</td><td class="thu">6</td><td class="fri">7</td><td class="sat">8</td><td class="sun">9</td></tr>
<tr><td class="mon">10</td><td class="tue">11</td><td class="wed">12</td><td class="thu">13</td><td class="fri">14</td><td class="sat">15</td><td class="sun">16</td></tr>
<tr><td class="mon">17</td><td class="tue">18</td><td class="wed">19</td><td class="thu">20</td><td class="fri">21</td><td class="sat">22</td><td class="sun">23</td></tr>
<tr><td class="mon">24</td><td class="tue">25</td><td class="wed">26</td><td class="thu">27</td><td class="fri">28</td><td class="sat">29</td><td class="sun">30</td></tr>
<tr><td class="mon">31</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">June</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="tue">1</td><td class="wed">2</td><td class="thu">3</td><td class="fri">4</td><td class="sat">5</td><td class="sun">6</td></tr>
<tr><td class="mon">7</td><td class="tue">8</td><td class="wed">9</td><td class="thu">10</td><td class="fri">11</td><td class="sat">12</td><td class="sun">13</td></tr>
<tr><td class="mon">14</td><td class="tue">15</td><td class="wed">16</td><td class="thu">17</td><td class="fri">18</td><td class="sat">19</td><td class="sun">20</td></tr>
<tr><td class="mon">21</td><td class="tue">22</td><td class="wed">23</td><td class="thu">24</td><td class="fri">25</td><td class="sat">26</td><td class="sun">27</td></tr>
<tr><td class="mon">28</td><td class="tue">29</td><td class="wed">30</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td></tr><tr><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">July</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="thu">1</td><td class="fri">2</td><td class="sat">3</td><td class="sun">4</td></tr>
<tr><td class="mon">5</td><td class="tue">6</td><td class="wed">7</td><td class="thu">8</td><td class="fri">9</td><td class="sat">10</td><td class="sun">11</td></tr>
<tr><td class="mon">12</td><td class="tue">13</td><td class="wed">14</td><td class="thu">15</td><td class="fri">16</td><td class="sat">17</td><td class="sun">18</td></tr>
<tr><td class="mon">19</td><td class="tue">20</td><td class="wed">21</td><td class="thu">22</td><td class="fri">23</td><td class="sat">24</td><td class="sun">25</td></tr>
<tr><td class="mon">26</td><td class="tue">27</td><td class="wed">28</td><td class="thu">29</td><td class="fri">30</td><td class="sat">31</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">August</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="sun">1</td></tr>
<tr><td class="mon">2</td><td class="tue">3</td><td class="wed">4</td><td class="thu">5</td><td class="fri">6</td><td class="sat">7</td><td class="sun">8</td></tr>
<tr><td class="mon">9</td><td class="tue">10</td><td class="wed">11</td><td class="thu">12</td><td class="fri">13</td><td class="sat">14</td><td class="sun">15</td></tr>
<tr><td class="mon">16</td><td class="tue">17</td><td class="wed">18</td><td class="thu">19</td><td class="fri">20</td><td class="sat">21</td><td class="sun">22</td></tr>
<tr><td class="mon">23</td><td class="tue">24</td><td class="wed">25</td><td class="thu">26</td><td class="fri">27</td><td class="sat">28</td><td class="sun">29</td></tr>
<tr><td class="mon">30</td><td class="tue">31</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">September</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="wed">1</td><td class="thu">2</td><td class="fri">3</td><td class="sat">4</td><td class="sun">5</td></tr>
<tr><td class="mon">6</td><td class="tue">7</td><td class="wed">8</td><td class="thu">9</td><td class="fri">10</td><td class="sat">11</td><td class="sun">12</td></tr>
<tr><td class="mon">13</td><td class="tue">14</td><td class="wed">15</td><td class="thu">16</td><td class="fri">17</td><td class="sat">18</td><td class="sun">19</td></tr>
<tr><td class="mon">20</td><td class="tue">21</td><td class="wed">22</td><td class="thu">23</td><td class="fri">24</td><td class="sat">25</td><td class="sun">26</td></tr>
<tr><td class="mon">27</td><td class="tue">28</td><td class="wed">29</td><td class="thu">30</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td></tr><tr><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">October</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="fri">1</td><td class="sat">2</td><td class="sun">3</td></tr>
<tr><td class="mon">4</td><td class="tue">5</td><td class="wed">6</td><td class="thu">7</td><td class="fri">8</td><td class="sat">9</td><td class="sun">10</td></tr>
<tr><td class="mon">11</td><td class="tue">12</td><td class="wed">13</td><td class="thu">14</td><td class="fri">15</td><td class="sat">16</td><td class="sun">17</td></tr>
<tr><td class="mon">18</td><td class="tue">19</td><td class="wed">20</td><td class="thu">21</td><td class="fri">22</td><td class="sat">23</td><td class="sun">24</td></tr>
<tr><td class="mon">25</td><td class="tue">26</td><td class="wed">27</td><td class="thu">28</td><td class="fri">29</td><td class="sat">30</td><td class="sun">31</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">November</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="mon">1</td><td class="tue">2</td><td class="wed">3</td><td class="thu">4</td><td class="fri">5</td><td class="sat">6</td><td class="sun">7</td></tr>
<tr><td class="mon">8</td><td class="tue">9</td><td class="wed">10</td><td class="thu">11</td><td class="fri">12</td><td class="sat">13</td><td class="sun">14</td></tr>
<tr><td class="mon">15</td><td class="tue">16</td><td class="wed">17</td><td class="thu">18</td><td class="fri">19</td><td class="sat">20</td><td class="sun">21</td></tr>
<tr><td class="mon">22</td><td class="tue">23</td><td class="wed">24</td><td class="thu">25</td><td class="fri">26</td><td class="sat">27</td><td class="sun">28</td></tr>
<tr><td class="mon">29</td><td class="tue">30</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td><td><table border="0" cellpadding="0" cellspacing="0" class="month">
<tr><th colspan="7" class="month">December</th></tr>
<tr><th class="mon">Mon</th><th class="tue">Tue</th><th class="wed">Wed</th><th class="thu">Thu</th><th class="fri">Fri</th><th class="sat">Sat</th><th class="sun">Sun</th></tr>
<tr><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td><td class="wed">1</td><td class="thu">2</td><td class="fri">3</td><td class="sat">4</td><td class="sun">5</td></tr>
<tr><td class="mon">6</td><td class="tue">7</td><td class="wed">8</td><td class="thu">9</td><td class="fri">10</td><td class="sat">11</td><td class="sun">12</td></tr>
<tr><td class="mon">13</td><td class="tue">14</td><td class="wed">15</td><td class="thu">16</td><td class="fri">17</td><td class="sat">18</td><td class="sun">19</td></tr>
<tr><td class="mon">20</td><td class="tue">21</td><td class="wed">22</td><td class="thu">23</td><td class="fri">24</td><td class="sat">25</td><td class="sun">26</td></tr>
<tr><td class="mon">27</td><td class="tue">28</td><td class="wed">29</td><td class="thu">30</td><td class="fri">31</td><td class="noday">&nbsp;</td><td class="noday">&nbsp;</td></tr>
</table>
</td></tr></table></body>
</html>
"""


class OutputTestCase(unittest.TestCase):
    def normalize_calendar(self, s):
        # Filters out locale dependent strings
        def neitherspacenordigit(c):
            return not c.isspace() and not c.isdigit()

        lines = []
        for line in s.splitlines(False):
            # Drop texts, as they are locale dependent
            if line and not filter(neitherspacenordigit, line):
                lines.append(line)
        return lines

    def test_output(self):
        self.assertEqual(
            self.normalize_calendar(calendar.calendar(2004)),
            self.normalize_calendar(result_2004_text)
        )

    def test_output_textcalendar(self):
        self.assertEqual(
            calendar.TextCalendar().formatyear(2004).strip(),
            result_2004_text.strip()
        )

    def test_output_htmlcalendar(self):
        encoding = 'ascii'
        cal = calendar.HTMLCalendar()
        self.assertEqual(
            cal.formatyearpage(2004, encoding=encoding).strip(b' \t\n'),
            result_2004_html.strip(' \t\n').encode(encoding)
        )


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
        self.assertRaises(TypeError, calendar.setfirstweekday, 'flabber')
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

    def test_localecalendars(self):
        # ensure that Locale{Text,HTML}Calendar resets the locale properly
        # (it is still not thread-safe though)
        old_october = calendar.TextCalendar().formatmonthname(2010, 10, 10)
        try:
            calendar.LocaleTextCalendar(locale='').formatmonthname(2010, 10, 10)
        except locale.Error:
            # cannot set the system default locale -- skip rest of test
            return
        calendar.LocaleHTMLCalendar(locale='').formatmonthname(2010, 10)
        new_october = calendar.TextCalendar().formatmonthname(2010, 10, 10)
        self.assertEqual(old_october, new_october)


class MonthCalendarTestCase(unittest.TestCase):
    def setUp(self):
        self.oldfirstweekday = calendar.firstweekday()
        calendar.setfirstweekday(self.firstweekday)

    def tearDown(self):
        calendar.setfirstweekday(self.oldfirstweekday)

    def check_weeks(self, year, month, weeks):
        cal = calendar.monthcalendar(year, month)
        self.assertEqual(len(cal), len(weeks))
        for i in range(len(weeks)):
            self.assertEqual(weeks[i], sum(day != 0 for day in cal[i]))


class MondayTestCase(MonthCalendarTestCase):
    firstweekday = calendar.MONDAY

    def test_february(self):
        # A 28-day february starting on monday (7+7+7+7 days)
        self.check_weeks(1999, 2, (7, 7, 7, 7))

        # A 28-day february starting on tuesday (6+7+7+7+1 days)
        self.check_weeks(2005, 2, (6, 7, 7, 7, 1))

        # A 28-day february starting on sunday (1+7+7+7+6 days)
        self.check_weeks(1987, 2, (1, 7, 7, 7, 6))

        # A 29-day february starting on monday (7+7+7+7+1 days)
        self.check_weeks(1988, 2, (7, 7, 7, 7, 1))

        # A 29-day february starting on tuesday (6+7+7+7+2 days)
        self.check_weeks(1972, 2, (6, 7, 7, 7, 2))

        # A 29-day february starting on sunday (1+7+7+7+7 days)
        self.check_weeks(2004, 2, (1, 7, 7, 7, 7))

    def test_april(self):
        # A 30-day april starting on monday (7+7+7+7+2 days)
        self.check_weeks(1935, 4, (7, 7, 7, 7, 2))

        # A 30-day april starting on tuesday (6+7+7+7+3 days)
        self.check_weeks(1975, 4, (6, 7, 7, 7, 3))

        # A 30-day april starting on sunday (1+7+7+7+7+1 days)
        self.check_weeks(1945, 4, (1, 7, 7, 7, 7, 1))

        # A 30-day april starting on saturday (2+7+7+7+7 days)
        self.check_weeks(1995, 4, (2, 7, 7, 7, 7))

        # A 30-day april starting on friday (3+7+7+7+6 days)
        self.check_weeks(1994, 4, (3, 7, 7, 7, 6))

    def test_december(self):
        # A 31-day december starting on monday (7+7+7+7+3 days)
        self.check_weeks(1980, 12, (7, 7, 7, 7, 3))

        # A 31-day december starting on tuesday (6+7+7+7+4 days)
        self.check_weeks(1987, 12, (6, 7, 7, 7, 4))

        # A 31-day december starting on sunday (1+7+7+7+7+2 days)
        self.check_weeks(1968, 12, (1, 7, 7, 7, 7, 2))

        # A 31-day december starting on thursday (4+7+7+7+6 days)
        self.check_weeks(1988, 12, (4, 7, 7, 7, 6))

        # A 31-day december starting on friday (3+7+7+7+7 days)
        self.check_weeks(2017, 12, (3, 7, 7, 7, 7))

        # A 31-day december starting on saturday (2+7+7+7+7+1 days)
        self.check_weeks(2068, 12, (2, 7, 7, 7, 7, 1))


class SundayTestCase(MonthCalendarTestCase):
    firstweekday = calendar.SUNDAY

    def test_february(self):
        # A 28-day february starting on sunday (7+7+7+7 days)
        self.check_weeks(2009, 2, (7, 7, 7, 7))

        # A 28-day february starting on monday (6+7+7+7+1 days)
        self.check_weeks(1999, 2, (6, 7, 7, 7, 1))

        # A 28-day february starting on saturday (1+7+7+7+6 days)
        self.check_weeks(1997, 2, (1, 7, 7, 7, 6))

        # A 29-day february starting on sunday (7+7+7+7+1 days)
        self.check_weeks(2004, 2, (7, 7, 7, 7, 1))

        # A 29-day february starting on monday (6+7+7+7+2 days)
        self.check_weeks(1960, 2, (6, 7, 7, 7, 2))

        # A 29-day february starting on saturday (1+7+7+7+7 days)
        self.check_weeks(1964, 2, (1, 7, 7, 7, 7))

    def test_april(self):
        # A 30-day april starting on sunday (7+7+7+7+2 days)
        self.check_weeks(1923, 4, (7, 7, 7, 7, 2))

        # A 30-day april starting on monday (6+7+7+7+3 days)
        self.check_weeks(1918, 4, (6, 7, 7, 7, 3))

        # A 30-day april starting on saturday (1+7+7+7+7+1 days)
        self.check_weeks(1950, 4, (1, 7, 7, 7, 7, 1))

        # A 30-day april starting on friday (2+7+7+7+7 days)
        self.check_weeks(1960, 4, (2, 7, 7, 7, 7))

        # A 30-day april starting on thursday (3+7+7+7+6 days)
        self.check_weeks(1909, 4, (3, 7, 7, 7, 6))

    def test_december(self):
        # A 31-day december starting on sunday (7+7+7+7+3 days)
        self.check_weeks(2080, 12, (7, 7, 7, 7, 3))

        # A 31-day december starting on monday (6+7+7+7+4 days)
        self.check_weeks(1941, 12, (6, 7, 7, 7, 4))

        # A 31-day december starting on saturday (1+7+7+7+7+2 days)
        self.check_weeks(1923, 12, (1, 7, 7, 7, 7, 2))

        # A 31-day december starting on wednesday (4+7+7+7+6 days)
        self.check_weeks(1948, 12, (4, 7, 7, 7, 6))

        # A 31-day december starting on thursday (3+7+7+7+7 days)
        self.check_weeks(1927, 12, (3, 7, 7, 7, 7))

        # A 31-day december starting on friday (2+7+7+7+7+1 days)
        self.check_weeks(1995, 12, (2, 7, 7, 7, 7, 1))

class TimegmTestCase(unittest.TestCase):
    TIMESTAMPS = [0, 10, 100, 1000, 10000, 100000, 1000000,
                  1234567890, 1262304000, 1275785153,]
    def test_timegm(self):
        for secs in self.TIMESTAMPS:
            tuple = time.gmtime(secs)
            self.assertEqual(secs, calendar.timegm(tuple))

class MonthRangeTestCase(unittest.TestCase):
    def test_january(self):
        # Tests valid lower boundary case.
        self.assertEqual(calendar.monthrange(2004,1), (3,31))

    def test_february_leap(self):
        # Tests February during leap year.
        self.assertEqual(calendar.monthrange(2004,2), (6,29))

    def test_february_nonleap(self):
        # Tests February in non-leap year.
        self.assertEqual(calendar.monthrange(2010,2), (0,28))

    def test_december(self):
        # Tests valid upper boundary case.
        self.assertEqual(calendar.monthrange(2004,12), (2,31))

    def test_zeroth_month(self):
        # Tests low invalid boundary case.
        with self.assertRaises(calendar.IllegalMonthError):
            calendar.monthrange(2004, 0)

    def test_thirteenth_month(self):
        # Tests high invalid boundary case.
        with self.assertRaises(calendar.IllegalMonthError):
            calendar.monthrange(2004, 13)


def test_main():
    support.run_unittest(
        OutputTestCase,
        CalendarTestCase,
        MondayTestCase,
        SundayTestCase,
        TimegmTestCase,
        MonthRangeTestCase,
    )


if __name__ == "__main__":
    test_main()
