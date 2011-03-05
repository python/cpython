@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
cd ..
call "%VS71COMNTOOLS%vsvars32.bat"

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3

@rem Sleepycat db
if not exist db-4.4.20 svn export http://svn.python.org/projects/external/db-4.4.20
if not exist db-4.4.20\build_win32\debug\libdb44sd.lib (
   devenv db-4.4.20\build_win32\Berkeley_DB.sln /build Debug /project db_static
)

@rem OpenSSL
if not exist openssl-0.9.8a svn export http://svn.python.org/projects/external/openssl-0.9.8a

@rem tcltk
if not exist tcl8.4.12 (
   if exist tcltk rd /s/q tcltk
   svn export http://svn.python.org/projects/external/tcl8.4.12
   svn export http://svn.python.org/projects/external/tk8.4.12
   cd tcl8.4.12\win
   nmake -f makefile.vc
   nmake -f makefile.vc INSTALLDIR=..\..\tcltk install
   cd ..\..
   cd tk8.4.12\win
   nmake -f makefile.vc TCLDIR=..\..\tcl8.4.12
   nmake -f makefile.vc TCLDIR=..\..\tcl8.4.12 INSTALLDIR=..\..\tcltk install
   cd ..\..
)

@rem sqlite
if not exist sqlite-source-3.3.4 svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
if not exist build\PCbuild\sqlite3.dll copy sqlite-source-3.3.4\sqlite3.dll build\PCbuild
