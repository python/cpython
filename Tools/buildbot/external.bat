@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
cd ..

@rem bzip
if not exist bzip2-1.0.3 svn export http://svn.python.org/projects/external/bzip2-1.0.3