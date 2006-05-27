"""Do a minimal test of all the modules that aren't otherwise tested."""

import warnings
warnings.filterwarnings('ignore', r".*posixfile module",
                        DeprecationWarning, 'posixfile$')

warnings.filterwarnings("ignore",
                        "the gopherlib module is deprecated",
                        DeprecationWarning,
                        ".*test_sundry")

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
import encodings
import formatter
import ftplib
import getpass
import gopherlib
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
import pipes
#import poplib
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
