@rem Run Tests.  Run the regression test suite.
@rem Plain "rt" runs Release build, arguments passed on to regrtest.
@rem "rt -d" runs Debug build similarly, after shifting off -d.
@set _exe=python
@if "%1" =="-d" set _exe=python_d
@if "%1" =="-d" shift
%_exe% ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@set _exe=
