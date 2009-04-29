"""Do a minimal test of all the modules that aren't otherwise tested."""

from test import test_support
import sys
import unittest
import warnings


class TestUntestedModules(unittest.TestCase):
    def test_at_least_import_untested_modules(self):
        with warnings.catch_warnings():
            import CGIHTTPServer
            import audiodev
            import bdb
            import cgitb
            import code
            import compileall

            import distutils.bcppcompiler
            import distutils.ccompiler
            import distutils.cygwinccompiler
            import distutils.emxccompiler
            import distutils.filelist
            if sys.platform.startswith('win'):
                import distutils.msvccompiler
            import distutils.mwerkscompiler
            import distutils.text_file
            import distutils.unixccompiler

            import distutils.command.bdist_dumb
            if sys.platform.startswith('win'):
                import distutils.command.bdist_msi
            import distutils.command.bdist
            import distutils.command.bdist_rpm
            import distutils.command.bdist_wininst
            import distutils.command.build_clib
            import distutils.command.build_ext
            import distutils.command.build
            import distutils.command.clean
            import distutils.command.config
            import distutils.command.install_data
            import distutils.command.install_egg_info
            import distutils.command.install_headers
            import distutils.command.install_lib
            import distutils.command.register
            import distutils.command.sdist
            import distutils.command.upload

            import encodings
            import formatter
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
            import nntplib
            import nturl2path
            import opcode
            import os2emxpath
            import pdb
            import posixfile
            import pstats
            import py_compile
            import rexec
            import rlcompleter
            import sched
            import sndhdr
            import statvfs
            import stringold
            import sunau
            import sunaudio
            import symbol
            import tabnanny
            import timeit
            import toaiff
            import token
            try:
                import tty     # not available on Windows
            except ImportError:
                if test_support.verbose:
                    print "skipping tty"

            # Can't test the "user" module -- if the user has a ~/.pythonrc.py, it
            # can screw up all sorts of things (esp. if it prints!).
            #import user
            import webbrowser
            import xml


def test_main():
    test_support.run_unittest(TestUntestedModules)

if __name__ == "__main__":
    test_main()
