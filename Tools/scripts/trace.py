#!/usr/bin/env python

# portions copyright 2001, Autonomous Zones Industries, Inc., all rights...
# err...  reserved and offered to the public under the terms of the
# Python 2.2 license.
# Author: Zooko O'Whielacronx
# http://zooko.com/
# mailto:zooko@zooko.com
#
# Copyright 2000, Mojam Media, Inc., all rights reserved.
# Author: Skip Montanaro
#
# Copyright 1999, Bioreason, Inc., all rights reserved.
# Author: Andrew Dalke
#
# Copyright 1995-1997, Automatrix, Inc., all rights reserved.
# Author: Skip Montanaro
#
# Copyright 1991-1995, Stichting Mathematisch Centrum, all rights reserved.
#
#
# Permission to use, copy, modify, and distribute this Python software and
# its associated documentation for any purpose without fee is hereby
# granted, provided that the above copyright notice appears in all copies,
# and that both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of neither Automatrix,
# Bioreason or Mojam Media be used in advertising or publicity pertaining to
# distribution of the software without specific, written prior permission.
#
#
# Cleaned up the usage message --GvR 11/28/01
#
# Summary of even more recent changes, --Zooko 2001-10-14
#   Used new `inspect' module for better (?) determination of file<->module
#      mappings, line numbers, and source code.
#   Used new local trace function for faster (and better?) operation.
#   Removed "speed hack", which, as far as I can tell, meant that it would
#      ignore all files ??? (When I tried it, it would ignore only *most* of my
#      files.  In any case with the speed hack removed in favor of actually
#      calling `Ignore.names()', it ignores only those files that I told it to
#      ignore, so I am happy.)
#   Rolled the `Coverage' class into `Trace', which now does either tracing or
#      counting or both according to constructor flags.
#   Moved the construction of the `Ignore' object inside the constructor of
#      `Trace', simplifying usage.
#   Changed function `create_results_log()' into method
#      `CoverageResults.write_results()'.
#   Add new mode "countfuncs" which is faster and which just reports which
#      functions were invoked.

#   Made `write_results' create `coverdir' if it doesn't already exist.
#   Moved the `run' funcs into `Trace' for simpler usage.
#   Use pickle instead of marshal for persistence.
#
# Summary of recent changes:
#   Support for files with the same basename (submodules in packages)
#   Expanded the idea of how to ignore files or modules
#   Split tracing and counting into different classes
#   Extracted count information and reporting from the count class
#   Added some ability to detect which missing lines could be executed
#   Added pseudo-pragma to prohibit complaining about unexecuted lines
#   Rewrote the main program

# Summary of older changes:
#   Added run-time display of statements being executed
#   Incorporated portability and performance fixes from Greg Stein
#   Incorporated main program from Michael Scharf

"""
program/module to trace Python program or function execution

Sample use, command line:
  trace.py -c -f counts --ignore-dir '$prefix' spam.py eggs
  trace.py -t --ignore-dir '$prefix' spam.py eggs

Sample use, programmatically
   # create a Trace object, telling it what to ignore, and whether to do tracing
   # or line-counting or both.
   trace = trace.Trace(ignoredirs=[sys.prefix, sys.exec_prefix,], trace=0, count=1)
   # run the new command using the given trace
   trace.run(coverage.globaltrace, 'main()')
   # make a report, telling it where you want output
   trace.print_results(show_missing=1)
"""

import sys, os, string, tempfile, types, copy, operator, inspect, exceptions, marshal
try:
    import cPickle
    pickle = cPickle
except ImportError:
    import pickle

true = 1
false = None

# DEBUG_MODE=1  # make this true to get printouts which help you understand what's going on

def usage(outfile):
    outfile.write("""Usage: %s [OPTIONS] <file> [ARGS]

Meta-options:
--help                Display this help then exit.
--version             Output version information then exit.

Otherwise, exactly one of the following three options must be given:
-t, --trace           Print each line to sys.stdout before it is executed.
-c, --count           Count the number of times each line is executed
                      and write the counts to <module>.cover for each
                      module executed, in the module's directory.
                      See also `--coverdir', `--file', `--no-report' below.
-r, --report          Generate a report from a counts file; do not execute
                      any code.  `--file' must specify the results file to
                      read, which must have been created in a previous run
                      with `--count --file=FILE'.

Modifiers:
-f, --file=<file>     File to accumulate counts over several runs.
-R, --no-report       Do not generate the coverage report files.
                      Useful if you want to accumulate over several runs.
-C, --coverdir=<dir>  Directory where the report files.  The coverage
                      report for <package>.<module> is written to file
                      <dir>/<package>/<module>.cover.
-m, --missing         Annotate executable lines that were not executed
                      with '>>>>>> '.
-s, --summary         Write a brief summary on stdout for each file.
                      (Can only be used with --count or --report.)

Filters, may be repeated multiple times:
--ignore-module=<mod> Ignore the given module and its submodules
                      (if it is a package).
--ignore-dir=<dir>    Ignore files in the given directory (multiple
                      directories can be joined by os.pathsep).
""" % sys.argv[0])

class Ignore:
    def __init__(self, modules = None, dirs = None):
        self._mods = modules or []
        self._dirs = dirs or []

        self._dirs = map(os.path.normpath, self._dirs)
        self._ignore = { '<string>': 1 }

    def names(self, filename, modulename):
        if self._ignore.has_key(modulename):
            return self._ignore[modulename]

        # haven't seen this one before, so see if the module name is
        # on the ignore list.  Need to take some care since ignoring
        # "cmp" musn't mean ignoring "cmpcache" but ignoring
        # "Spam" must also mean ignoring "Spam.Eggs".
        for mod in self._mods:
            if mod == modulename:  # Identical names, so ignore
                self._ignore[modulename] = 1
                return 1
            # check if the module is a proper submodule of something on
            # the ignore list
            n = len(mod)
            # (will not overflow since if the first n characters are the
            # same and the name has not already occured, then the size
            # of "name" is greater than that of "mod")
            if mod == modulename[:n] and modulename[n] == '.':
                self._ignore[modulename] = 1
                return 1

        # Now check that __file__ isn't in one of the directories
        if filename is None:
            # must be a built-in, so we must ignore
            self._ignore[modulename] = 1
            return 1

        # Ignore a file when it contains one of the ignorable paths
        for d in self._dirs:
            # The '+ os.sep' is to ensure that d is a parent directory,
            # as compared to cases like:
            #  d = "/usr/local"
            #  filename = "/usr/local.py"
            # or
            #  d = "/usr/local.py"
            #  filename = "/usr/local.py"
            if string.find(filename, d + os.sep) == 0:
                self._ignore[modulename] = 1
                return 1

        # Tried the different ways, so we don't ignore this module
        self._ignore[modulename] = 0
        return 0

class CoverageResults:
    def __init__(self, counts=None, calledfuncs=None, infile=None, outfile=None):
        self.counts = counts
        if self.counts is None:
            self.counts = {}
        self.counter = self.counts.copy() # map (filename, lineno) to count
        self.calledfuncs = calledfuncs
        if self.calledfuncs is None:
            self.calledfuncs = {}
        self.calledfuncs = self.calledfuncs.copy()
        self.infile = infile
        self.outfile = outfile
        if self.infile:
            # try and merge existing counts file
            try:
                thingie = pickle.load(open(self.infile, 'r'))
                if type(thingie) is types.DictType:
                    # backwards compatibility for old trace.py after Zooko touched it but before calledfuncs  --Zooko 2001-10-24
                    self.update(self.__class__(thingie))
                elif type(thingie) is types.TupleType and len(thingie) == 2:
                    (counts, calledfuncs,) = thingie
                    self.update(self.__class__(counts, calledfuncs))
            except (IOError, EOFError,):
                pass
            except pickle.UnpicklingError:
                # backwards compatibility for old trace.py before Zooko touched it  --Zooko 2001-10-24
                self.update(self.__class__(marshal.load(open(self.infile))))

    def update(self, other):
        """Merge in the data from another CoverageResults"""
        counts = self.counts
        calledfuncs = self.calledfuncs
        other_counts = other.counts
        other_calledfuncs = other.calledfuncs

        for key in other_counts.keys():
            if key != 'calledfuncs': # backwards compatibility for abortive attempt to stuff calledfuncs into self.counts, by Zooko  --Zooko 2001-10-24
                counts[key] = counts.get(key, 0) + other_counts[key]

        for key in other_calledfuncs.keys():
            calledfuncs[key] = 1

    def write_results(self, show_missing = 1, summary = 0, coverdir = None):
        """
        @param coverdir
        """
        for (filename, modulename, funcname,) in self.calledfuncs.keys():
            print "filename: %s, modulename: %s, funcname: %s" % (filename, modulename, funcname,)

        import re
        # turn the counts data ("(filename, lineno) = count") into something
        # accessible on a per-file basis
        per_file = {}
        for thingie in self.counts.keys():
            if thingie != "calledfuncs": # backwards compatibility for abortive attempt to stuff calledfuncs into self.counts, by Zooko  --Zooko 2001-10-24
                (filename, lineno,) = thingie
                lines_hit = per_file[filename] = per_file.get(filename, {})
                lines_hit[lineno] = self.counts[(filename, lineno)]

        # there are many places where this is insufficient, like a blank
        # line embedded in a multiline string.
        blank = re.compile(r'^\s*(#.*)?$')

        # accumulate summary info, if needed
        sums = {}

        # generate file paths for the coverage files we are going to write...
        fnlist = []
        tfdir = tempfile.gettempdir()
        for key in per_file.keys():
            filename = key

            # skip some "files" we don't care about...
            if filename == "<string>":
                continue
            # are these caused by code compiled using exec or something?
            if filename.startswith(tfdir):
                continue

            modulename = inspect.getmodulename(filename)

            if filename.endswith(".pyc") or filename.endswith(".pyo"):
                filename = filename[:-1]

            if coverdir:
                thiscoverdir = coverdir
            else:
                thiscoverdir = os.path.dirname(os.path.abspath(filename))

            # the code from here to "<<<" is the contents of the `fileutil.make_dirs()' function in the Mojo Nation project.  --Zooko 2001-10-14
            # http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/mojonation/evil/common/fileutil.py?rev=HEAD&content-type=text/vnd.viewcvs-markup
            tx = None
            try:
                os.makedirs(thiscoverdir)
            except OSError, x:
                tx = x

            if not os.path.isdir(thiscoverdir):
                if tx:
                    raise tx
                raise exceptions.IOError, "unknown error prevented creation of directory: %s" % thiscoverdir # careful not to construct an IOError with a 2-tuple, as that has a special meaning...
            # <<<

            # build list file name by appending a ".cover" to the module name
            # and sticking it into the specified directory
            if "." in modulename:
                # A module in a package
                finalname = modulename.split(".")[-1]
                listfilename = os.path.join(thiscoverdir, finalname + ".cover")
            else:
                listfilename = os.path.join(thiscoverdir, modulename + ".cover")

            # Get the original lines from the .py file
            try:
                lines = open(filename, 'r').readlines()
            except IOError, err:
                sys.stderr.write("trace: Could not open %s for reading because: %s - skipping\n" % (`filename`, err))
                continue

            try:
                outfile = open(listfilename, 'w')
            except IOError, err:
                sys.stderr.write(
                    '%s: Could not open %s for writing because: %s" \
                    "- skipping\n' % ("trace", `listfilename`, err))
                continue

            # If desired, get a list of the line numbers which represent
            # executable content (returned as a dict for better lookup speed)
            if show_missing:
                executable_linenos = find_executable_linenos(filename)
            else:
                executable_linenos = {}

            n_lines = 0
            n_hits = 0
            lines_hit = per_file[key]
            for i in range(len(lines)):
                line = lines[i]

                # do the blank/comment match to try to mark more lines
                # (help the reader find stuff that hasn't been covered)
                if lines_hit.has_key(i+1):
                    # count precedes the lines that we captured
                    outfile.write('%5d: ' % lines_hit[i+1])
                    n_hits = n_hits + 1
                    n_lines = n_lines + 1
                elif blank.match(line):
                    # blank lines and comments are preceded by dots
                    outfile.write('    . ')
                else:
                    # lines preceded by no marks weren't hit
                    # Highlight them if so indicated, unless the line contains
                    # '#pragma: NO COVER' (it is possible to embed this into
                    # the text as a non-comment; no easy fix)
                    if executable_linenos.has_key(i+1) and \
                       string.find(lines[i],
                                   string.join(['#pragma', 'NO COVER'])) == -1:
                        outfile.write('>>>>>> ')
                    else:
                        outfile.write(' '*7)
                    n_lines = n_lines + 1
                outfile.write(string.expandtabs(lines[i], 8))

            outfile.close()

            if summary and n_lines:
                percent = int(100 * n_hits / n_lines)
                sums[modulename] = n_lines, percent, modulename, filename

        if summary and sums:
            mods = sums.keys()
            mods.sort()
            print "lines   cov%   module   (path)"
            for m in mods:
                n_lines, percent, modulename, filename = sums[m]
                print "%5d   %3d%%   %s   (%s)" % sums[m]

        if self.outfile:
            # try and store counts and module info into self.outfile
            try:
                pickle.dump((self.counts, self.calledfuncs,), open(self.outfile, 'w'), 1)
            except IOError, err:
                sys.stderr.write("cannot save counts files because %s" % err)

# Given a code string, return the SET_LINENO information
def _find_LINENO_from_string(co_code):
    """return all of the SET_LINENO information from a code string"""
    import dis
    linenos = {}

    # This code was filched from the `dis' module then modified
    n = len(co_code)
    i = 0
    prev_op = None
    prev_lineno = 0
    while i < n:
        c = co_code[i]
        op = ord(c)
        if op == dis.SET_LINENO:
            if prev_op == op:
                # two SET_LINENO in a row, so the previous didn't
                # indicate anything.  This occurs with triple
                # quoted strings (?).  Remove the old one.
                del linenos[prev_lineno]
            prev_lineno = ord(co_code[i+1]) + ord(co_code[i+2])*256
            linenos[prev_lineno] = 1
        if op >= dis.HAVE_ARGUMENT:
            i = i + 3
        else:
            i = i + 1
        prev_op = op
    return linenos

def _find_LINENO(code):
    """return all of the SET_LINENO information from a code object"""
    import types

    # get all of the lineno information from the code of this scope level
    linenos = _find_LINENO_from_string(code.co_code)

    # and check the constants for references to other code objects
    for c in code.co_consts:
        if type(c) == types.CodeType:
            # find another code object, so recurse into it
            linenos.update(_find_LINENO(c))
    return linenos

def find_executable_linenos(filename):
    """return a dict of the line numbers from executable statements in a file

    Works by finding all of the code-like objects in the module then searching
    the byte code for 'SET_LINENO' terms (so this won't work one -O files).

    """
    import parser

    assert filename.endswith('.py')

    prog = open(filename).read()
    ast = parser.suite(prog)
    code = parser.compileast(ast, filename)

    # The only way I know to find line numbers is to look for the
    # SET_LINENO instructions.  Isn't there some way to get it from
    # the AST?

    return _find_LINENO(code)

### XXX because os.path.commonprefix seems broken by my way of thinking...
def commonprefix(dirs):
    "Given a list of pathnames, returns the longest common leading component"
    if not dirs: return ''
    n = copy.copy(dirs)
    for i in range(len(n)):
        n[i] = n[i].split(os.sep)
    prefix = n[0]
    for item in n:
        for i in range(len(prefix)):
            if prefix[:i+1] <> item[:i+1]:
                prefix = prefix[:i]
                if i == 0: return ''
                break
    return os.sep.join(prefix)

class Trace:
    def __init__(self, count=1, trace=1, countfuncs=0, ignoremods=(), ignoredirs=(), infile=None, outfile=None):
        """
        @param count true iff it should count number of times each line is executed
        @param trace true iff it should print out each line that is being counted
        @param countfuncs true iff it should just output a list of (filename, modulename, funcname,) for functions that were called at least once;  This overrides `count' and `trace'
        @param ignoremods a list of the names of modules to ignore
        @param ignoredirs a list of the names of directories to ignore all of the (recursive) contents of
        @param infile file from which to read stored counts to be added into the results
        @param outfile file in which to write the results
        """
        self.infile = infile
        self.outfile = outfile
        self.ignore = Ignore(ignoremods, ignoredirs)
        self.counts = {}   # keys are (filename, linenumber)
        self.blabbed = {} # for debugging
        self.pathtobasename = {} # for memoizing os.path.basename
        self.donothing = 0
        self.trace = trace
        self._calledfuncs = {}
        if countfuncs:
            self.globaltrace = self.globaltrace_countfuncs
        elif trace and count:
            self.globaltrace = self.globaltrace_lt
            self.localtrace = self.localtrace_trace_and_count
        elif trace:
            self.globaltrace = self.globaltrace_lt
            self.localtrace = self.localtrace_trace
        elif count:
            self.globaltrace = self.globaltrace_lt
            self.localtrace = self.localtrace_count
        else:
            # Ahem -- do nothing?  Okay.
            self.donothing = 1

    def run(self, cmd):
        import __main__
        dict = __main__.__dict__
        if not self.donothing:
            sys.settrace(self.globaltrace)
        try:
            exec cmd in dict, dict
        finally:
            if not self.donothing:
                sys.settrace(None)

    def runctx(self, cmd, globals=None, locals=None):
        if globals is None: globals = {}
        if locals is None: locals = {}
        if not self.donothing:
            sys.settrace(self.globaltrace)
        try:
            exec cmd in globals, locals
        finally:
            if not self.donothing:
                sys.settrace(None)

    def runfunc(self, func, *args, **kw):
        result = None
        if not self.donothing:
            sys.settrace(self.globaltrace)
        try:
            result = apply(func, args, kw)
        finally:
            if not self.donothing:
                sys.settrace(None)
        return result

    def globaltrace_countfuncs(self, frame, why, arg):
        """
        Handles `call' events (why == 'call') and adds the (filename, modulename, funcname,) to the self._calledfuncs dict.
        """
        if why == 'call':
            (filename, lineno, funcname, context, lineindex,) = inspect.getframeinfo(frame, 0)
            if filename:
                modulename = inspect.getmodulename(filename)
            else:
                modulename = None
            self._calledfuncs[(filename, modulename, funcname,)] = 1

    def globaltrace_lt(self, frame, why, arg):
        """
        Handles `call' events (why == 'call') and if the code block being entered is to be ignored then it returns `None', else it returns `self.localtrace'.
        """
        if why == 'call':
            (filename, lineno, funcname, context, lineindex,) = inspect.getframeinfo(frame, 0)
            # if DEBUG_MODE and not filename:
            #     print "%s.globaltrace(frame: %s, why: %s, arg: %s): filename: %s, lineno: %s, funcname: %s, context: %s, lineindex: %s\n" % (self, frame, why, arg, filename, lineno, funcname, context, lineindex,)
            if filename:
                modulename = inspect.getmodulename(filename)
                if modulename is not None:
                    ignore_it = self.ignore.names(filename, modulename)
                    # if DEBUG_MODE and not self.blabbed.has_key((filename, modulename,)):
                    #     self.blabbed[(filename, modulename,)] = None
                    #     print "%s.globaltrace(frame: %s, why: %s, arg: %s, filename: %s, modulename: %s, ignore_it: %s\n" % (self, frame, why, arg, filename, modulename, ignore_it,)
                    if not ignore_it:
                        if self.trace:
                            print " --- modulename: %s, funcname: %s" % (modulename, funcname,)
                        # if DEBUG_MODE:
                        #     print "%s.globaltrace(frame: %s, why: %s, arg: %s, filename: %s, modulename: %s, ignore_it: %s -- about to localtrace\n" % (self, frame, why, arg, filename, modulename, ignore_it,)
                        return self.localtrace
            else:
                # XXX why no filename?
                return None

    def localtrace_trace_and_count(self, frame, why, arg):
        if why == 'line':
            # record the file name and line number of every trace
            # XXX I wish inspect offered me an optimized `getfilename(frame)' to use in place of the presumably heavier `getframeinfo()'.  --Zooko 2001-10-14
            (filename, lineno, funcname, context, lineindex,) = inspect.getframeinfo(frame, 1)
            key = (filename, lineno,)
            self.counts[key] = self.counts.get(key, 0) + 1
            # XXX not convinced that this memoizing is a performance win -- I don't know enough about Python guts to tell.  --Zooko 2001-10-14
            bname = self.pathtobasename.get(filename)
            if bname is None:
                # Using setdefault faster than two separate lines?  --Zooko 2001-10-14
                bname = self.pathtobasename.setdefault(filename, os.path.basename(filename))
            try:
                print "%s(%d): %s" % (bname, lineno, context[lineindex],),
            except IndexError:
                # Uh.. sometimes getframeinfo gives me a context of length 1 and a lineindex of -2.  Oh well.
                pass
        return self.localtrace

    def localtrace_trace(self, frame, why, arg):
        if why == 'line':
            # XXX shouldn't do the count increment when arg is exception?  But be careful to return self.localtrace when arg is exception! ?  --Zooko 2001-10-14
            # record the file name and line number of every trace
            # XXX I wish inspect offered me an optimized `getfilename(frame)' to use in place of the presumably heavier `getframeinfo()'.  --Zooko 2001-10-14
            (filename, lineno, funcname, context, lineindex,) = inspect.getframeinfo(frame)
            # if DEBUG_MODE:
            #     print "%s.localtrace_trace(frame: %s, why: %s, arg: %s); filename: %s, lineno: %s, funcname: %s, context: %s, lineindex: %s\n" % (self, frame, why, arg, filename, lineno, funcname, context, lineindex,)
            # XXX not convinced that this memoizing is a performance win -- I don't know enough about Python guts to tell.  --Zooko 2001-10-14
            bname = self.pathtobasename.get(filename)
            if bname is None:
                # Using setdefault faster than two separate lines?  --Zooko 2001-10-14
                bname = self.pathtobasename.setdefault(filename, os.path.basename(filename))
            if context is not None:
                try:
                    print "%s(%d): %s" % (bname, lineno, context[lineindex],),
                except IndexError:
                    # Uh.. sometimes getframeinfo gives me a context of length 1 and a lineindex of -2.  Oh well.
                    pass
            else:
                print "%s(???): ???" % bname
        return self.localtrace

    def localtrace_count(self, frame, why, arg):
        if why == 'line':
            # XXX shouldn't do the count increment when arg is exception?  But be careful to return self.localtrace when arg is exception! ?  --Zooko 2001-10-14
            # record the file name and line number of every trace
            # XXX I wish inspect offered me an optimized `getfilename(frame)' to use in place of the presumably heavier `getframeinfo()'.  --Zooko 2001-10-14
            (filename, lineno, funcname, context, lineindex,) = inspect.getframeinfo(frame)
            key = (filename, lineno,)
            self.counts[key] = self.counts.get(key, 0) + 1
        return self.localtrace

    def results(self):
        return CoverageResults(self.counts, infile=self.infile, outfile=self.outfile, calledfuncs=self._calledfuncs)

def _err_exit(msg):
    sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.exit(1)

def main(argv=None):
    import getopt

    if argv is None:
        argv = sys.argv
    try:
        opts, prog_argv = getopt.getopt(argv[1:], "tcrRf:d:msC:l",
                                        ["help", "version", "trace", "count",
                                         "report", "no-report",
                                         "file=", "missing",
                                         "ignore-module=", "ignore-dir=",
                                         "coverdir=", "listfuncs",])

    except getopt.error, msg:
        sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
        sys.stderr.write("Try `%s --help' for more information\n" % sys.argv[0])
        sys.exit(1)

    trace = 0
    count = 0
    report = 0
    no_report = 0
    counts_file = None
    missing = 0
    ignore_modules = []
    ignore_dirs = []
    coverdir = None
    summary = 0
    listfuncs = false

    for opt, val in opts:
        if opt == "--help":
            usage(sys.stdout)
            sys.exit(0)

        if opt == "--version":
            sys.stdout.write("trace 2.0\n")
            sys.exit(0)

        if opt == "-l" or opt == "--listfuncs":
            listfuncs = true
            continue

        if opt == "-t" or opt == "--trace":
            trace = 1
            continue

        if opt == "-c" or opt == "--count":
            count = 1
            continue

        if opt == "-r" or opt == "--report":
            report = 1
            continue

        if opt == "-R" or opt == "--no-report":
            no_report = 1
            continue

        if opt == "-f" or opt == "--file":
            counts_file = val
            continue

        if opt == "-m" or opt == "--missing":
            missing = 1
            continue

        if opt == "-C" or opt == "--coverdir":
            coverdir = val
            continue

        if opt == "-s" or opt == "--summary":
            summary = 1
            continue

        if opt == "--ignore-module":
            ignore_modules.append(val)
            continue

        if opt == "--ignore-dir":
            for s in string.split(val, os.pathsep):
                s = os.path.expandvars(s)
                # should I also call expanduser? (after all, could use $HOME)

                s = string.replace(s, "$prefix",
                                   os.path.join(sys.prefix, "lib",
                                                "python" + sys.version[:3]))
                s = string.replace(s, "$exec_prefix",
                                   os.path.join(sys.exec_prefix, "lib",
                                                "python" + sys.version[:3]))
                s = os.path.normpath(s)
                ignore_dirs.append(s)
            continue

        assert 0, "Should never get here"

    if listfuncs and (count or trace):
        _err_exit("cannot specify both --listfuncs and (--trace or --count)")

    if not count and not trace and not report and not listfuncs:
        _err_exit("must specify one of --trace, --count, --report or --listfuncs")

    if report and no_report:
        _err_exit("cannot specify both --report and --no-report")

    if report and not counts_file:
        _err_exit("--report requires a --file")

    if no_report and len(prog_argv) == 0:
        _err_exit("missing name of file to run")

    # everything is ready
    if report:
        results = CoverageResults(infile=counts_file, outfile=counts_file)
        results.write_results(missing, summary=summary, coverdir=coverdir)
    else:
        sys.argv = prog_argv
        progname = prog_argv[0]
        if eval(sys.version[:3])>1.3:
            sys.path[0] = os.path.split(progname)[0] # ???

        t = Trace(count, trace, countfuncs=listfuncs, ignoremods=ignore_modules, ignoredirs=ignore_dirs, infile=counts_file, outfile=counts_file)
        try:
            t.run('execfile(' + `progname` + ')')
        except IOError, err:
            _err_exit("Cannot run file %s because: %s" % (`sys.argv[0]`, err))
        except SystemExit:
            pass

        results = t.results()

        if not no_report:
            results.write_results(missing, summary=summary, coverdir=coverdir)

if __name__=='__main__':
    main()
