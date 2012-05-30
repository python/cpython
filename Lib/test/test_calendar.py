import calendar
import unittest

from test import support
from test.script_helper import assert_python_ok
import time
import locale
import sys

result_2004_01_text = """
    January 2004
Mo Tu We Th Fr Sa Su
          1  2  3  4
 5  6  7  8  9 10 11
12 13 14 15 16 17 18
19 20 21 22 23 24 25
26 27 28 29 30 31
"""

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
<?xml version="1.0" encoding="%(e)s"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=%(e)s" />
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

result_2004_days = [
    [[[0, 0, 0, 1, 2, 3, 4],
      [5, 6, 7, 8, 9, 10, 11],
      [12, 13, 14, 15, 16, 17, 18],
      [19, 20, 21, 22, 23, 24, 25],
      [26, 27, 28, 29, 30, 31, 0]],
     [[0, 0, 0, 0, 0, 0, 1],
      [2, 3, 4, 5, 6, 7, 8],
      [9, 10, 11, 12, 13, 14, 15],
      [16, 17, 18, 19, 20, 21, 22],
      [23, 24, 25, 26, 27, 28, 29]],
     [[1, 2, 3, 4, 5, 6, 7],
      [8, 9, 10, 11, 12, 13, 14],
      [15, 16, 17, 18, 19, 20, 21],
      [22, 23, 24, 25, 26, 27, 28],
      [29, 30, 31, 0, 0, 0, 0]]],
    [[[0, 0, 0, 1, 2, 3, 4],
      [5, 6, 7, 8, 9, 10, 11],
      [12, 13, 14, 15, 16, 17, 18],
      [19, 20, 21, 22, 23, 24, 25],
      [26, 27, 28, 29, 30, 0, 0]],
     [[0, 0, 0, 0, 0, 1, 2],
      [3, 4, 5, 6, 7, 8, 9],
      [10, 11, 12, 13, 14, 15, 16],
      [17, 18, 19, 20, 21, 22, 23],
      [24, 25, 26, 27, 28, 29, 30],
      [31, 0, 0, 0, 0, 0, 0]],
     [[0, 1, 2, 3, 4, 5, 6],
      [7, 8, 9, 10, 11, 12, 13],
      [14, 15, 16, 17, 18, 19, 20],
      [21, 22, 23, 24, 25, 26, 27],
      [28, 29, 30, 0, 0, 0, 0]]],
    [[[0, 0, 0, 1, 2, 3, 4],
      [5, 6, 7, 8, 9, 10, 11],
      [12, 13, 14, 15, 16, 17, 18],
      [19, 20, 21, 22, 23, 24, 25],
      [26, 27, 28, 29, 30, 31, 0]],
     [[0, 0, 0, 0, 0, 0, 1],
      [2, 3, 4, 5, 6, 7, 8],
      [9, 10, 11, 12, 13, 14, 15],
      [16, 17, 18, 19, 20, 21, 22],
      [23, 24, 25, 26, 27, 28, 29],
      [30, 31, 0, 0, 0, 0, 0]],
     [[0, 0, 1, 2, 3, 4, 5],
      [6, 7, 8, 9, 10, 11, 12],
      [13, 14, 15, 16, 17, 18, 19],
      [20, 21, 22, 23, 24, 25, 26],
      [27, 28, 29, 30, 0, 0, 0]]],
    [[[0, 0, 0, 0, 1, 2, 3],
      [4, 5, 6, 7, 8, 9, 10],
      [11, 12, 13, 14, 15, 16, 17],
      [18, 19, 20, 21, 22, 23, 24],
      [25, 26, 27, 28, 29, 30, 31]],
     [[1, 2, 3, 4, 5, 6, 7],
      [8, 9, 10, 11, 12, 13, 14],
      [15, 16, 17, 18, 19, 20, 21],
      [22, 23, 24, 25, 26, 27, 28],
      [29, 30, 0, 0, 0, 0, 0]],
     [[0, 0, 1, 2, 3, 4, 5],
      [6, 7, 8, 9, 10, 11, 12],
      [13, 14, 15, 16, 17, 18, 19],
      [20, 21, 22, 23, 24, 25, 26],
      [27, 28, 29, 30, 31, 0, 0]]]
]

result_2004_dates = \
    [[['12/29/03 12/30/03 12/31/03 01/01/04 01/02/04 01/03/04 01/04/04',
       '01/05/04 01/06/04 01/07/04 01/08/04 01/09/04 01/10/04 01/11/04',
       '01/12/04 01/13/04 01/14/04 01/15/04 01/16/04 01/17/04 01/18/04',
       '01/19/04 01/20/04 01/21/04 01/22/04 01/23/04 01/24/04 01/25/04',
       '01/26/04 01/27/04 01/28/04 01/29/04 01/30/04 01/31/04 02/01/04'],
      ['01/26/04 01/27/04 01/28/04 01/29/04 01/30/04 01/31/04 02/01/04',
       '02/02/04 02/03/04 02/04/04 02/05/04 02/06/04 02/07/04 02/08/04',
       '02/09/04 02/10/04 02/11/04 02/12/04 02/13/04 02/14/04 02/15/04',
       '02/16/04 02/17/04 02/18/04 02/19/04 02/20/04 02/21/04 02/22/04',
       '02/23/04 02/24/04 02/25/04 02/26/04 02/27/04 02/28/04 02/29/04'],
      ['03/01/04 03/02/04 03/03/04 03/04/04 03/05/04 03/06/04 03/07/04',
       '03/08/04 03/09/04 03/10/04 03/11/04 03/12/04 03/13/04 03/14/04',
       '03/15/04 03/16/04 03/17/04 03/18/04 03/19/04 03/20/04 03/21/04',
       '03/22/04 03/23/04 03/24/04 03/25/04 03/26/04 03/27/04 03/28/04',
       '03/29/04 03/30/04 03/31/04 04/01/04 04/02/04 04/03/04 04/04/04']],
     [['03/29/04 03/30/04 03/31/04 04/01/04 04/02/04 04/03/04 04/04/04',
       '04/05/04 04/06/04 04/07/04 04/08/04 04/09/04 04/10/04 04/11/04',
       '04/12/04 04/13/04 04/14/04 04/15/04 04/16/04 04/17/04 04/18/04',
       '04/19/04 04/20/04 04/21/04 04/22/04 04/23/04 04/24/04 04/25/04',
       '04/26/04 04/27/04 04/28/04 04/29/04 04/30/04 05/01/04 05/02/04'],
      ['04/26/04 04/27/04 04/28/04 04/29/04 04/30/04 05/01/04 05/02/04',
       '05/03/04 05/04/04 05/05/04 05/06/04 05/07/04 05/08/04 05/09/04',
       '05/10/04 05/11/04 05/12/04 05/13/04 05/14/04 05/15/04 05/16/04',
       '05/17/04 05/18/04 05/19/04 05/20/04 05/21/04 05/22/04 05/23/04',
       '05/24/04 05/25/04 05/26/04 05/27/04 05/28/04 05/29/04 05/30/04',
       '05/31/04 06/01/04 06/02/04 06/03/04 06/04/04 06/05/04 06/06/04'],
      ['05/31/04 06/01/04 06/02/04 06/03/04 06/04/04 06/05/04 06/06/04',
       '06/07/04 06/08/04 06/09/04 06/10/04 06/11/04 06/12/04 06/13/04',
       '06/14/04 06/15/04 06/16/04 06/17/04 06/18/04 06/19/04 06/20/04',
       '06/21/04 06/22/04 06/23/04 06/24/04 06/25/04 06/26/04 06/27/04',
       '06/28/04 06/29/04 06/30/04 07/01/04 07/02/04 07/03/04 07/04/04']],
     [['06/28/04 06/29/04 06/30/04 07/01/04 07/02/04 07/03/04 07/04/04',
       '07/05/04 07/06/04 07/07/04 07/08/04 07/09/04 07/10/04 07/11/04',
       '07/12/04 07/13/04 07/14/04 07/15/04 07/16/04 07/17/04 07/18/04',
       '07/19/04 07/20/04 07/21/04 07/22/04 07/23/04 07/24/04 07/25/04',
       '07/26/04 07/27/04 07/28/04 07/29/04 07/30/04 07/31/04 08/01/04'],
      ['07/26/04 07/27/04 07/28/04 07/29/04 07/30/04 07/31/04 08/01/04',
       '08/02/04 08/03/04 08/04/04 08/05/04 08/06/04 08/07/04 08/08/04',
       '08/09/04 08/10/04 08/11/04 08/12/04 08/13/04 08/14/04 08/15/04',
       '08/16/04 08/17/04 08/18/04 08/19/04 08/20/04 08/21/04 08/22/04',
       '08/23/04 08/24/04 08/25/04 08/26/04 08/27/04 08/28/04 08/29/04',
       '08/30/04 08/31/04 09/01/04 09/02/04 09/03/04 09/04/04 09/05/04'],
      ['08/30/04 08/31/04 09/01/04 09/02/04 09/03/04 09/04/04 09/05/04',
       '09/06/04 09/07/04 09/08/04 09/09/04 09/10/04 09/11/04 09/12/04',
       '09/13/04 09/14/04 09/15/04 09/16/04 09/17/04 09/18/04 09/19/04',
       '09/20/04 09/21/04 09/22/04 09/23/04 09/24/04 09/25/04 09/26/04',
       '09/27/04 09/28/04 09/29/04 09/30/04 10/01/04 10/02/04 10/03/04']],
     [['09/27/04 09/28/04 09/29/04 09/30/04 10/01/04 10/02/04 10/03/04',
       '10/04/04 10/05/04 10/06/04 10/07/04 10/08/04 10/09/04 10/10/04',
       '10/11/04 10/12/04 10/13/04 10/14/04 10/15/04 10/16/04 10/17/04',
       '10/18/04 10/19/04 10/20/04 10/21/04 10/22/04 10/23/04 10/24/04',
       '10/25/04 10/26/04 10/27/04 10/28/04 10/29/04 10/30/04 10/31/04'],
      ['11/01/04 11/02/04 11/03/04 11/04/04 11/05/04 11/06/04 11/07/04',
       '11/08/04 11/09/04 11/10/04 11/11/04 11/12/04 11/13/04 11/14/04',
       '11/15/04 11/16/04 11/17/04 11/18/04 11/19/04 11/20/04 11/21/04',
       '11/22/04 11/23/04 11/24/04 11/25/04 11/26/04 11/27/04 11/28/04',
       '11/29/04 11/30/04 12/01/04 12/02/04 12/03/04 12/04/04 12/05/04'],
      ['11/29/04 11/30/04 12/01/04 12/02/04 12/03/04 12/04/04 12/05/04',
       '12/06/04 12/07/04 12/08/04 12/09/04 12/10/04 12/11/04 12/12/04',
       '12/13/04 12/14/04 12/15/04 12/16/04 12/17/04 12/18/04 12/19/04',
       '12/20/04 12/21/04 12/22/04 12/23/04 12/24/04 12/25/04 12/26/04',
       '12/27/04 12/28/04 12/29/04 12/30/04 12/31/04 01/01/05 01/02/05']]]


class OutputTestCase(unittest.TestCase):
    def normalize_calendar(self, s):
        # Filters out locale dependent strings
        def neitherspacenordigit(c):
            return not c.isspace() and not c.isdigit()

        lines = []
        for line in s.splitlines(keepends=False):
            # Drop texts, as they are locale dependent
            if line and not filter(neitherspacenordigit, line):
                lines.append(line)
        return lines

    def check_htmlcalendar_encoding(self, req, res):
        cal = calendar.HTMLCalendar()
        self.assertEqual(
            cal.formatyearpage(2004, encoding=req).strip(b' \t\n'),
            (result_2004_html % {'e': res}).strip(' \t\n').encode(res)
        )

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

    def test_output_htmlcalendar_encoding_ascii(self):
        self.check_htmlcalendar_encoding('ascii', 'ascii')

    def test_output_htmlcalendar_encoding_utf8(self):
        self.check_htmlcalendar_encoding('utf-8', 'utf-8')

    def test_output_htmlcalendar_encoding_default(self):
        self.check_htmlcalendar_encoding(None, sys.getdefaultencoding())

    def test_yeardatescalendar(self):
        def shrink(cal):
            return [[[' '.join('{:02d}/{:02d}/{}'.format(
                                d.month, d.day, str(d.year)[-2:]) for d in z)
                            for z in y] for y in x] for x in cal]
        self.assertEqual(
            shrink(calendar.Calendar().yeardatescalendar(2004)),
            result_2004_dates
        )

    def test_yeardayscalendar(self):
        self.assertEqual(
            calendar.Calendar().yeardayscalendar(2004),
            result_2004_days
        )

    def test_formatweekheader_short(self):
        self.assertEqual(
            calendar.TextCalendar().formatweekheader(2),
            'Mo Tu We Th Fr Sa Su'
        )

    def test_formatweekheader_long(self):
        self.assertEqual(
            calendar.TextCalendar().formatweekheader(9),
            '  Monday   Tuesday  Wednesday  Thursday '
            '  Friday   Saturday   Sunday '
        )

    def test_formatmonth(self):
        self.assertEqual(
            calendar.TextCalendar().formatmonth(2004, 1).strip(),
            result_2004_01_text.strip()
        )

    def test_formatmonthname_with_year(self):
        self.assertEqual(
            calendar.HTMLCalendar().formatmonthname(2004, 1, withyear=True),
            '<tr><th colspan="7" class="month">January 2004</th></tr>'
        )

    def test_formatmonthname_without_year(self):
        self.assertEqual(
            calendar.HTMLCalendar().formatmonthname(2004, 1, withyear=False),
            '<tr><th colspan="7" class="month">January</th></tr>'
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

    def test_illegal_weekday_reported(self):
        with self.assertRaisesRegex(calendar.IllegalWeekdayError, '123'):
            calendar.setfirstweekday(123)

    def test_enumerate_weekdays(self):
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

    def test_locale_calendars(self):
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

    def test_illegal_month_reported(self):
        with self.assertRaisesRegex(calendar.IllegalMonthError, '65'):
            calendar.monthrange(2004, 65)

class LeapdaysTestCase(unittest.TestCase):
    def test_no_range(self):
        # test when no range i.e. two identical years as args
        self.assertEqual(calendar.leapdays(2010,2010), 0)

    def test_no_leapdays(self):
        # test when no leap years in range
        self.assertEqual(calendar.leapdays(2010,2011), 0)

    def test_no_leapdays_upper_boundary(self):
        # test no leap years in range, when upper boundary is a leap year
        self.assertEqual(calendar.leapdays(2010,2012), 0)

    def test_one_leapday_lower_boundary(self):
        # test when one leap year in range, lower boundary is leap year
        self.assertEqual(calendar.leapdays(2012,2013), 1)

    def test_several_leapyears_in_range(self):
        self.assertEqual(calendar.leapdays(1997,2020), 5)


class ConsoleOutputTestCase(unittest.TestCase):
    def test_outputs_bytes(self):
        (return_code, stdout, stderr) = assert_python_ok('-m', 'calendar', '--type=html', '2010')
        self.assertEqual(stdout[:6], b'<?xml ')

def test_main():
    support.run_unittest(
        OutputTestCase,
        CalendarTestCase,
        MondayTestCase,
        SundayTestCase,
        TimegmTestCase,
        MonthRangeTestCase,
        LeapdaysTestCase,
        ConsoleOutputTestCase
    )


if __name__ == "__main__":
    test_main()
