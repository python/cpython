@rem Run Tests.  Run the regression test suite.
@rem Plain "rt" runs Release build, arguments passed on to regrtest.
@rem "rt -d" runs Debug build similarly, after shifting off -d.
@rem Normally the tests are run twice, the first time after deleting
@rem all the .py[co] files from Lib/ and Lib/test.  But
@rem "rt -q" (for Quick) runs the tests just once, and without
@rem bothering to delete .py[co] files.
@set _exe=python
@set _qmode=no
@if "%1"=="-q" set _qmode=yes
@if "%1"=="-q" shift
@if "%1"=="-d" set _exe=python_d
@if "%1"=="-d" shift
@if "%_qmode%"=="yes" goto Qmode
@if "%1"=="-q" set _qmode=yes
@if "%1"=="-q" shift
@if "%_qmode%"=="yes" goto Qmode
@echo Deleting .pyc/.pyo files ...
@del ..\Lib\*.pyc
@del ..\Lib\*.pyo
@del ..\Lib\test\*.pyc
@del ..\Lib\test\*.pyo
%_exe% ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@echo About to run again without deleting .pyc/.pyo first:
@pause
:Qmode
%_exe% ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
@set _exe=
@set _qmode=
