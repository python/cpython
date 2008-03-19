@rem Fetches (and builds if necessary) external dependencies

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

@rem OpenSSL
if not exist openssl-0.9.8g (
  if exist openssl-0.9.8a rd /s/q openssl-0.9.8a
  svn export http://svn.python.org/projects/external/openssl-0.9.8g
)

@rem tcltk
if not exist tcl8.4.18.1 (
   if exist tcltk rd /s/q tcltk
   if exist tcl8.4.12 rd /s/q tcl8.4.12
   if exist tk8.4.12 rd /s/q tk8.4.12
   if exist tcl8.4.16 rd /s/q tcl8.4.16
   if exist tk8.4.16 rd /s/q tk8.4.16
   svn export http://svn.python.org/projects/external/tcl-8.4.18.1
   svn export http://svn.python.org/projects/external/tk-8.4.18.1
   cd tcl-8.4.18.1
   nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 MACHINE=AMD64 INSTALLDIR=../../tcltk64 clean all install
   cd ..\..
   cd tk-8.4.18.1
   nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 MACHINE=AMD64 INSTALLDIR=../../tcltk64 TCLDIR=../../tcl-8.4.18.1 clean all install
)

@rem sqlite
if not exist sqlite-source-3.3.4 svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
