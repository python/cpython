"""Do a minimal test of all the modules that aren't otherwise tested."""

from test.test_support import catch_warning
import sys
import warnings

with catch_warning():
    warnings.filterwarnings('ignore', r".*posixfile",
                            DeprecationWarning)
    warnings.filterwarnings('ignore', r".*mimify", DeprecationWarning)

    from test.test_support import verbose

    import BaseHTTPServer
    import DocXMLRPCServer
    import CGIHTTPServer
    import SimpleHTTPServer
    import SimpleXMLRPCServer
    import aifc
    import audiodev
    import bdb
    import cgitb
    import cmd
    import code
    import compileall

    import distutils.archive_util
    import distutils.bcppcompiler
    import distutils.ccompiler
    import distutils.cmd
    import distutils.core
    import distutils.cygwinccompiler
    import distutils.dep_util
    import distutils.dir_util
    import distutils.emxccompiler
    import distutils.errors
    import distutils.extension
    import distutils.file_util
    import distutils.filelist
    import distutils.log
    if sys.platform.startswith('win'):
        import distutils.msvccompiler
    import distutils.mwerkscompiler
    import distutils.sysconfig
    import distutils.text_file
    import distutils.unixccompiler
    import distutils.util
    import distutils.version

    import distutils.command.bdist_dumb
    if sys.platform.startswith('win'):
        import distutils.command.bdist_msi
    import distutils.command.bdist
    import distutils.command.bdist_rpm
    import distutils.command.bdist_wininst
    import distutils.command.build_clib
    import distutils.command.build_ext
    import distutils.command.build
    import distutils.command.build_py
    import distutils.command.build_scripts
    import distutils.command.clean
    import distutils.command.config
    import distutils.command.install_data
    import distutils.command.install_egg_info
    import distutils.command.install_headers
    import distutils.command.install_lib
    import distutils.command.install
    import distutils.command.install_scripts
    import distutils.command.register
    import distutils.command.sdist
    import distutils.command.upload

    import encodings
    import formatter
    import ftplib
    import getpass
    import htmlentitydefs
    import ihooks
    import imghdr
    import imputil
    import keyword
    import linecache
    import macurl2path
    import mailcap
    import mimify
    import mutex
    import nntplib
    import nturl2path
    import opcode
    import os2emxpath
    import pdb
    import posixfile
    import pstats
    import py_compile
    import pydoc
    import rexec
    import rlcompleter
    import sched
    import smtplib
    import sndhdr
    import statvfs
    import stringold
    import sunau
    import sunaudio
    import symbol
    import tabnanny
    import telnetlib
    import timeit
    import toaiff
    import token
    try:
        import tty     # not available on Windows
    except ImportError:
        if verbose:
            print "skipping tty"

    # Can't test the "user" module -- if the user has a ~/.pythonrc.py, it
    # can screw up all sorts of things (esp. if it prints!).
    #import user
    import webbrowser
    import xml
