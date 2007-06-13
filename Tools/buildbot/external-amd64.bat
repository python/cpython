@rem Fetches (and builds if necessary) external dependencies
setlocal

@rem need this so that 'devenv' is found
call "%VS71COMNTOOLS%vsvars32.bat"
@rem set the build environment
call "%MSSdk%\SetEnv" /XP64 /RETAIL

@rem Assume we start inside the Python source directory
cd ..

@rem sqlite
if not exist sqlite-source-3.3.4 (
   svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
   if exist build\PCbuild\sqlite3.dll del build\PCbuild\sqlite3.dll
)
if not exist build\PCbuild\sqlite3.dll (
   cd sqlite-source-3.3.4\amd64
   cl ..\*.c
   link /def:..\sqlite3.def  /dll *.obj /out:sqlite3.dll bufferoverflowU.lib
   cd ..\..
   copy sqlite-source-3.3.4\amd64\sqlite3.dll build\PCbuild
)

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3

@rem Sleepycat db
if not exist db-4.4.20 svn export http://svn.python.org/projects/external/db-4.4.20
if not exist "db-4.4.20\build_win32\Release_AMD64\libdb44s.lib" (
   cd db-4.4.20\build_win32
   devenv Berkeley_DB.sln /build "Release AMD64" /project db_static /useenv
   cd ..\..
)

@rem OpenSSL
if not exist openssl-0.9.8a svn export http://svn.python.org/projects/external/openssl-0.9.8a

@rem tcltk
if not exist tcl8.4.12 (
   if exist tcltk rd /s/q tcltk
   svn export http://svn.python.org/projects/external/tcl8.4.12
   svn export http://svn.python.org/projects/external/tk8.4.12
)
