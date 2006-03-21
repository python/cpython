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
