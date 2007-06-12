@rem Fetches (and builds if necessary) external dependencies
setlocal

@rem Assume we start inside the Python source directory
cd ..

@rem sqlite
if not exist sqlite-source-3.3.4 (
   svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
   if exist trunk\PCbuild\sqlite3.dll del trunk\PCbuild\sqlite3.dll
)

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3

@rem Sleepycat db
call "%MSSdk%\SetEnv" /XP64 /RETAIL
if not exist db-4.4.20 svn export http://svn.python.org/projects/external/db-4.4.20

@rem OpenSSL
if not exist openssl-0.9.8a svn export http://svn.python.org/projects/external/openssl-0.9.8a

@rem tcltk
if not exist tcl8.4.12 (
   if exist tcltk rd /s/q tcltk
   svn export http://svn.python.org/projects/external/tcl8.4.12
   svn export http://svn.python.org/projects/external/tk8.4.12
)

