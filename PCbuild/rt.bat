@rem Run Tests.  Run the regression test suite.
@rem Plain "rt" runs Release build, arguments passed on to regrtest.
@rem "rt -d" runs Debug build similarly, after shifting off -d.
@rem Normally the tests are run twice, the first time after deleting
@rem all the .py[co] files from Lib/ and Lib/test/.  But
@rem "rt -q" (for Quick) runs the tests just once, and without
@rem bothering to delete .py[co] files.
@rem "rt -O" runs python or python_d with -O (depending on -d).
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
%_exe% %_dashO% ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@echo About to run again without deleting .pyc/.pyo first:
@pause
:Qmode
%_exe% %_dashO% ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@set _exe=
@set _qmode=
@set _dashO=
