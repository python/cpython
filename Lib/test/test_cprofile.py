import sys
import cProfile
import pstats
import test.test_support

#################################
#       Warning!
# This stuff is touchy. If you modify anything above the test_main function,
# you'll have to regenerate the stats for the doctest!
################################

TICKS = 42000

def timer():
    return TICKS

def testfunc():
    # 1 call
    # 1000 ticks total: 270 ticks local, 730 ticks in subfunctions
    global TICKS
    TICKS += 99
    helper()                            # 300
    helper()                            # 300
    TICKS += 171
    factorial(14)                       # 130

def factorial(n):
    # 23 calls total
    # 170 ticks total, 150 ticks local
    # 3 primitive calls, 130, 20 and 20 ticks total
    # including 116, 17, 17 ticks local
    global TICKS
    if n > 0:
        TICKS += n
        return mul(n, factorial(n-1))
    else:
        TICKS += 11
        return 1

def mul(a, b):
    # 20 calls
    # 1 tick, local
    global TICKS
    TICKS += 1
    return a * b

def helper():
    # 2 calls
    # 300 ticks total: 20 ticks local, 260 ticks in subfunctions
    global TICKS
    TICKS += 1
    helper1()                           # 30
    TICKS += 2
    helper1()                           # 30
    TICKS += 6
    helper2()                           # 50
    TICKS += 3
    helper2()                           # 50
    TICKS += 2
    helper2()                           # 50
    TICKS += 5
    helper2_indirect()                  # 70
    TICKS += 1

def helper1():
    # 4 calls
    # 30 ticks total: 29 ticks local, 1 tick in subfunctions
    global TICKS
    TICKS += 10
    hasattr(C(), "foo")                 # 1
    TICKS += 19
    lst = []
    lst.append(42)                      # 0
    sys.exc_info()                      # 0

def helper2_indirect():
    helper2()                           # 50
    factorial(3)                        # 20

def helper2():
    # 8 calls
    # 50 ticks local: 39 ticks local, 11 ticks in subfunctions
    global TICKS
    TICKS += 11
    hasattr(C(), "bar")                 # 1
    TICKS += 13
    subhelper()                         # 10
    TICKS += 15

def subhelper():
    # 8 calls
    # 10 ticks total: 8 ticks local, 2 ticks in subfunctions
    global TICKS
    TICKS += 2
    for i in range(2):                  # 0
        try:
            C().foo                     # 1 x 2
        except AttributeError:
            TICKS += 3                  # 3 x 2

class C:
    def __getattr__(self, name):
        # 28 calls
        # 1 tick, local
        global TICKS
        TICKS += 1
        raise AttributeError

def test_main():
    """
    >>> prof = cProfile.Profile(timer, 0.001)
    >>> prof.runctx("testfunc()", globals(), locals()) #doctest: +ELLIPSIS
    <cProfile.Profile object at ...>
    >>> timer()
    43000
    >>> stats = pstats.Stats(prof)
    >>> stats.strip_dirs().sort_stats("stdname") #doctest: +ELLIPSIS
    <pstats.Stats ...>
    >>> stats.print_stats() #doctest: +ELLIPSIS
             126 function calls (106 primitive calls) in 1.000 CPU seconds
    <BLANKLINE>
       Ordered by: standard name
    <BLANKLINE>
       ncalls  tottime  percall  cumtime  percall filename:lineno(function)
            1    0.000    0.000    1.000    1.000 <string>:1(<module>)
           28    0.028    0.001    0.028    0.001 test_cprofile.py:102(__getattr__)
            1    0.270    0.270    1.000    1.000 test_cprofile.py:17(testfunc)
         23/3    0.150    0.007    0.170    0.057 test_cprofile.py:27(factorial)
           20    0.020    0.001    0.020    0.001 test_cprofile.py:40(mul)
            2    0.040    0.020    0.600    0.300 test_cprofile.py:47(helper)
            4    0.116    0.029    0.120    0.030 test_cprofile.py:65(helper1)
            2    0.000    0.000    0.140    0.070 test_cprofile.py:76(helper2_indirect)
            8    0.312    0.039    0.400    0.050 test_cprofile.py:80(helper2)
            8    0.064    0.008    0.080    0.010 test_cprofile.py:90(subhelper)
           12    0.000    0.000    0.012    0.001 {hasattr}
            4    0.000    0.000    0.000    0.000 {method 'append' of 'list' objects}
            1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
            8    0.000    0.000    0.000    0.000 {range}
            4    0.000    0.000    0.000    0.000 {sys.exc_info}
    <BLANKLINE>
    <BLANKLINE>
    <pstats.Stats instance at ...>
    >>> stats.print_callers() #doctest: +ELLIPSIS
       Ordered by: standard name
    <BLANKLINE>
    Function                                          was called by...
                                                          ncalls  tottime  cumtime
    <string>:1(<module>)                              <-
    test_cprofile.py:102(__getattr__)                 <-      16    0.016    0.016  test_cprofile.py:90(subhelper)
                                                              12    0.012    0.012  {hasattr}
    test_cprofile.py:17(testfunc)                     <-       1    0.270    1.000  <string>:1(<module>)
    test_cprofile.py:27(factorial)                    <-       1    0.014    0.130  test_cprofile.py:17(testfunc)
                                                            20/3    0.130    0.147  test_cprofile.py:27(factorial)
                                                               2    0.006    0.040  test_cprofile.py:76(helper2_indirect)
    test_cprofile.py:40(mul)                          <-      20    0.020    0.020  test_cprofile.py:27(factorial)
    test_cprofile.py:47(helper)                       <-       2    0.040    0.600  test_cprofile.py:17(testfunc)
    test_cprofile.py:65(helper1)                      <-       4    0.116    0.120  test_cprofile.py:47(helper)
    test_cprofile.py:76(helper2_indirect)             <-       2    0.000    0.140  test_cprofile.py:47(helper)
    test_cprofile.py:80(helper2)                      <-       6    0.234    0.300  test_cprofile.py:47(helper)
                                                               2    0.078    0.100  test_cprofile.py:76(helper2_indirect)
    test_cprofile.py:90(subhelper)                    <-       8    0.064    0.080  test_cprofile.py:80(helper2)
    {hasattr}                                         <-       4    0.000    0.004  test_cprofile.py:65(helper1)
                                                               8    0.000    0.008  test_cprofile.py:80(helper2)
    {method 'append' of 'list' objects}               <-       4    0.000    0.000  test_cprofile.py:65(helper1)
    {method 'disable' of '_lsprof.Profiler' objects}  <-
    {range}                                           <-       8    0.000    0.000  test_cprofile.py:90(subhelper)
    {sys.exc_info}                                    <-       4    0.000    0.000  test_cprofile.py:65(helper1)
    <BLANKLINE>
    <BLANKLINE>
    <pstats.Stats instance at ...>
    >>> stats.print_callees() #doctest: +ELLIPSIS
       Ordered by: standard name
    <BLANKLINE>
    Function                                          called...
                                                          ncalls  tottime  cumtime
    <string>:1(<module>)                              ->       1    0.270    1.000  test_cprofile.py:17(testfunc)
    test_cprofile.py:102(__getattr__)                 ->
    test_cprofile.py:17(testfunc)                     ->       1    0.014    0.130  test_cprofile.py:27(factorial)
                                                               2    0.040    0.600  test_cprofile.py:47(helper)
    test_cprofile.py:27(factorial)                    ->    20/3    0.130    0.147  test_cprofile.py:27(factorial)
                                                              20    0.020    0.020  test_cprofile.py:40(mul)
    test_cprofile.py:40(mul)                          ->
    test_cprofile.py:47(helper)                       ->       4    0.116    0.120  test_cprofile.py:65(helper1)
                                                               2    0.000    0.140  test_cprofile.py:76(helper2_indirect)
                                                               6    0.234    0.300  test_cprofile.py:80(helper2)
    test_cprofile.py:65(helper1)                      ->       4    0.000    0.004  {hasattr}
                                                               4    0.000    0.000  {method 'append' of 'list' objects}
                                                               4    0.000    0.000  {sys.exc_info}
    test_cprofile.py:76(helper2_indirect)             ->       2    0.006    0.040  test_cprofile.py:27(factorial)
                                                               2    0.078    0.100  test_cprofile.py:80(helper2)
    test_cprofile.py:80(helper2)                      ->       8    0.064    0.080  test_cprofile.py:90(subhelper)
                                                               8    0.000    0.008  {hasattr}
    test_cprofile.py:90(subhelper)                    ->      16    0.016    0.016  test_cprofile.py:102(__getattr__)
                                                               8    0.000    0.000  {range}
    {hasattr}                                         ->      12    0.012    0.012  test_cprofile.py:102(__getattr__)
    {method 'append' of 'list' objects}               ->
    {method 'disable' of '_lsprof.Profiler' objects}  ->
    {range}                                           ->
    {sys.exc_info}                                    ->
    <BLANKLINE>
    <BLANKLINE>
    <pstats.Stats instance at ...>
    """
    from test import test_cprofile
    test.test_support.run_doctest(test_cprofile)

if __name__ == "__main__":
    test_main()
