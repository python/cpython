@rem Fetches (and builds if necessary) external dependencies

@REM XXX FIXME - building for x64 disabled for now.

@rem Assume we start inside the Python source directory
cd ..
call "%VS90COMNTOOLS%vsvars32.bat"

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3

@rem Sleepycat db
@rem Remove VS 2003 builds
if exist db-4.4.20 if not exist db-4.4.20\build_win32\this_is_for_vs9 (
   echo Removing old build
   rd /s/q db-4.4.20
)
if not exist db-4.4.20 svn export http://svn.python.org/projects/external/db-4.4.20-vs9 db-4.4.20
if not exist db-4.4.20\build_win32\debug\libdb44sd.lib (
   vcbuild db-4.4.20\build_win32\db_static.vcproj "Debug AMD64|x64"
)

@rem OpenSSL
if not exist openssl-0.9.8g (
  if exist openssl-0.9.8a rd /s/q openssl-0.9.8a
  svn export http://svn.python.org/projects/external/openssl-0.9.8g
)

@rem tcltk
if not exist tcl8.4.16 (
   if exist tcltk rd /s/q tcltk
   if exist tcl8.4.12 rd /s/q tcl8.4.12
   if exist tk8.4.12 rd /s/q tk8.4.12
   svn export http://svn.python.org/projects/external/tcl8.4.16
   svn export http://svn.python.org/projects/external/tk8.4.16
@REM    cd tcl8.4.16\win
@REM    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500
@REM    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 INSTALLDIR=..\..\tcltk install
@REM    cd ..\..
@REM    cd tk8.4.16\win
@REM    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 TCLDIR=..\..\tcl8.4.16
@REM    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 TCLDIR=..\..\tcl8.4.16 INSTALLDIR=..\..\tcltk install
@REM    cd ..\..
)

@rem sqlite
if not exist sqlite-source-3.3.4 svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
@REM if not exist build\PCbuild\sqlite3.dll copy sqlite-source-3.3.4\sqlite3.dll build\PCbuild
