@rem Run Tests.  Run the regression test suite.
@rem Usage:  rt [-d] [-O] [-q] regrtest_args
@rem -d   Run Debug build (python_d.exe).  Else release build.
@rem -O   Run python.exe or python_d.exe (see -d) with -O.
@rem -q   "quick" -- normally the tests are run twice, the first time
@rem      after deleting all the .py[co] files reachable from Lib/.
@rem      -q runs the tests just once, and without deleting .py[co] files.
@rem All leading instances of these switches are shifted off, and
@rem whatever remains is passed to regrtest.py.  For example,
@rem     rt -O -d -x test_thread
@rem runs
@rem     python_d -O ../../lib/test/regrtest.py -x test_thread
@rem twice, and
@rem     rt -q -g test_binascii
@rem runs
@rem     python_d ../../lib/test/regrtest.py -g test_binascii
@rem to generate the expected-output file for binascii quickly.
@set _exe=python
@set _qmode=no
@set _dashO=
@goto CheckOpts
:Again
@shift
:CheckOpts
@if "%1"=="-O" set _dashO=-O
@if "%1"=="-O" goto Again
@if "%1"=="-q" set _qmode=yes
@if "%1"=="-q" goto Again
@if "%1"=="-d" set _exe=python_d
@if "%1"=="-d" goto Again
@if "%_qmode%"=="yes" goto Qmode
@echo Deleting .pyc/.pyo files ...
@%_exe% rmpyc.py
%_exe% %_dashO% -E -tt ../../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@echo About to run again without deleting .pyc/.pyo first:
@pause
:Qmode
%_exe% %_dashO% -E -tt ../../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@set _exe=
@set _qmode=
@set _dashO=
