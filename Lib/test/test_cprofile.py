"""Test suite for the cProfile module."""

import sys
from test.test_support import run_unittest, TESTFN, unlink

# rip off all interesting stuff from test_profile
import cProfile
from test.test_profile import ProfileTest, regenerate_expected_output

class CProfileTest(ProfileTest):
    profilerclass = cProfile.Profile
    expected_list_sort_output = "{method 'sort' of 'list' objects}"

    # Issue 3895.
    def test_bad_counter_during_dealloc(self):
        import _lsprof
        # Must use a file as StringIO doesn't trigger the bug.
        sys.stderr = open(TESTFN, 'w')
        try:
            obj = _lsprof.Profiler(lambda: int)
            obj.enable()
            obj = _lsprof.Profiler(1)
            obj.disable()
        finally:
            sys.stderr = sys.__stderr__
            unlink(TESTFN)


def test_main():
    run_unittest(CProfileTest)

def main():
    if '-r' not in sys.argv:
        test_main()
    else:
        regenerate_expected_output(__file__, CProfileTest)


# Don't remove this comment. Everything below it is auto-generated.
#--cut--------------------------------------------------------------------------
CProfileTest.expected_output['print_stats'] = """\
         126 function calls (106 primitive calls) in 1.000 CPU seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000    1.000    1.000 <string>:1(<module>)
       28    0.028    0.001    0.028    0.001 profilee.py:110(__getattr__)
        1    0.270    0.270    1.000    1.000 profilee.py:25(testfunc)
     23/3    0.150    0.007    0.170    0.057 profilee.py:35(factorial)
       20    0.020    0.001    0.020    0.001 profilee.py:48(mul)
        2    0.040    0.020    0.600    0.300 profilee.py:55(helper)
        4    0.116    0.029    0.120    0.030 profilee.py:73(helper1)
        2    0.000    0.000    0.140    0.070 profilee.py:84(helper2_indirect)
        8    0.312    0.039    0.400    0.050 profilee.py:88(helper2)
        8    0.064    0.008    0.080    0.010 profilee.py:98(subhelper)
       12    0.000    0.000    0.012    0.001 {hasattr}
        4    0.000    0.000    0.000    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        8    0.000    0.000    0.000    0.000 {range}
        4    0.000    0.000    0.000    0.000 {sys.exc_info}


"""
CProfileTest.expected_output['print_callers'] = """\
   Ordered by: standard name

Function                                          was called by...
                                                      ncalls  tottime  cumtime
<string>:1(<module>)                              <-
profilee.py:110(__getattr__)                      <-      16    0.016    0.016  profilee.py:98(subhelper)
                                                          12    0.012    0.012  {hasattr}
profilee.py:25(testfunc)                          <-       1    0.270    1.000  <string>:1(<module>)
profilee.py:35(factorial)                         <-       1    0.014    0.130  profilee.py:25(testfunc)
                                                        20/3    0.130    0.147  profilee.py:35(factorial)
                                                           2    0.006    0.040  profilee.py:84(helper2_indirect)
profilee.py:48(mul)                               <-      20    0.020    0.020  profilee.py:35(factorial)
profilee.py:55(helper)                            <-       2    0.040    0.600  profilee.py:25(testfunc)
profilee.py:73(helper1)                           <-       4    0.116    0.120  profilee.py:55(helper)
profilee.py:84(helper2_indirect)                  <-       2    0.000    0.140  profilee.py:55(helper)
profilee.py:88(helper2)                           <-       6    0.234    0.300  profilee.py:55(helper)
                                                           2    0.078    0.100  profilee.py:84(helper2_indirect)
profilee.py:98(subhelper)                         <-       8    0.064    0.080  profilee.py:88(helper2)
{hasattr}                                         <-       4    0.000    0.004  profilee.py:73(helper1)
                                                           8    0.000    0.008  profilee.py:88(helper2)
{method 'append' of 'list' objects}               <-       4    0.000    0.000  profilee.py:73(helper1)
{method 'disable' of '_lsprof.Profiler' objects}  <-
{range}                                           <-       8    0.000    0.000  profilee.py:98(subhelper)
{sys.exc_info}                                    <-       4    0.000    0.000  profilee.py:73(helper1)


"""
CProfileTest.expected_output['print_callees'] = """\
   Ordered by: standard name

Function                                          called...
                                                      ncalls  tottime  cumtime
<string>:1(<module>)                              ->       1    0.270    1.000  profilee.py:25(testfunc)
profilee.py:110(__getattr__)                      ->
profilee.py:25(testfunc)                          ->       1    0.014    0.130  profilee.py:35(factorial)
                                                           2    0.040    0.600  profilee.py:55(helper)
profilee.py:35(factorial)                         ->    20/3    0.130    0.147  profilee.py:35(factorial)
                                                          20    0.020    0.020  profilee.py:48(mul)
profilee.py:48(mul)                               ->
profilee.py:55(helper)                            ->       4    0.116    0.120  profilee.py:73(helper1)
                                                           2    0.000    0.140  profilee.py:84(helper2_indirect)
                                                           6    0.234    0.300  profilee.py:88(helper2)
profilee.py:73(helper1)                           ->       4    0.000    0.004  {hasattr}
                                                           4    0.000    0.000  {method 'append' of 'list' objects}
                                                           4    0.000    0.000  {sys.exc_info}
profilee.py:84(helper2_indirect)                  ->       2    0.006    0.040  profilee.py:35(factorial)
                                                           2    0.078    0.100  profilee.py:88(helper2)
profilee.py:88(helper2)                           ->       8    0.064    0.080  profilee.py:98(subhelper)
                                                           8    0.000    0.008  {hasattr}
profilee.py:98(subhelper)                         ->      16    0.016    0.016  profilee.py:110(__getattr__)
                                                           8    0.000    0.000  {range}
{hasattr}                                         ->      12    0.012    0.012  profilee.py:110(__getattr__)
{method 'append' of 'list' objects}               ->
{method 'disable' of '_lsprof.Profiler' objects}  ->
{range}                                           ->
{sys.exc_info}                                    ->


"""

if __name__ == "__main__":
    main()
