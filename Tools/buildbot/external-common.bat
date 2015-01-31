@rem Common file shared between external.bat and external-amd64.bat.  Responsible for
@rem fetching external components into the root\.. buildbot directories.

if "%SVNROOT%"=="" set SVNROOT=http://svn.python.org/projects/external/

if not exist externals mkdir externals
cd externals
@rem XXX: If you need to force the buildbots to start from a fresh environment, uncomment
@rem the following, check it in, then check it out, comment it out, then check it back in.
@rem if exist bzip2-1.0.6 rd /s/q bzip2-1.0.6
@rem if exist tcltk rd /s/q tcltk
@rem if exist tcltk64 rd /s/q tcltk64
@rem if exist tcl-8.6.1.0 rd /s/q tcl-8.6.1.0
@rem if exist tk-8.6.1.0 rd /s/q tk-8.6.1.0
@rem if exist tix-8.4.3.4 rd /s/q tix-8.4.3.4
@rem if exist db-4.4.20 rd /s/q db-4.4.20
@rem if exist openssl-1.0.1l rd /s/q openssl-1.0.1l
@rem if exist sqlite-3.7.12 rd /s/q sqlite-3.7.12

@rem bzip
if not exist bzip2-1.0.6 (
   rd /s/q bzip2-1.0.5
  svn export %SVNROOT%bzip2-1.0.6
)

@rem NASM, for OpenSSL build
@rem if exist nasm-2.11.06 rd /s/q nasm-2.11.06
if not exist nasm-2.11.06 svn export %SVNROOT%nasm-2.11.06

@rem OpenSSL
if not exist openssl-1.0.1l (
    rd /s/q openssl-1.0.1j
    svn export %SVNROOT%openssl-1.0.1l
)

@rem tcl/tk
if not exist tcl-8.6.1.0 (
   rd /s/q tcltk tcltk64 tcl-8.5.11.0 tk-8.5.11.0
   svn export %SVNROOT%tcl-8.6.1.0
)
if not exist tk-8.6.1.0 svn export %SVNROOT%tk-8.6.1.0
if not exist tix-8.4.3.4 svn export %SVNROOT%tix-8.4.3.4

@rem sqlite3
if not exist sqlite-3.8.3.1 (
  rd /s/q sqlite-source-3.8.1
  svn export %SVNROOT%sqlite-3.8.3.1
)

@rem lzma
if not exist xz-5.0.5 (
  rd /s/q xz-5.0.3
  svn export %SVNROOT%xz-5.0.5
)
