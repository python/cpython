@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
cd ..

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3

@rem OpenSSL
if exist openssl-0.9.7d rd /s/q openssl-0.9.7d
if exist openssl-0.9.8a rd /s/q openssl-0.9.8a
if not exist openssl-0.9.7l svn export http://svn.python.org/projects/external/openssl-0.9.7l

