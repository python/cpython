#!/usr/bin/env python

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

Sample use, programmatically (still more complicated than it should be)
   # create an Ignore option, telling it what you want to ignore
   ignore = trace.Ignore(dirs = [sys.prefix, sys.exec_prefix])
   # create a Coverage object, telling it what to ignore
   coverage = trace.Coverage(ignore)
   # run the new command using the given trace
   trace.run(coverage.trace, 'main()')

   # make a report, telling it where you want output
   t = trace.create_results_log(coverage.results(),
                                '/usr/local/Automatrix/concerts/coverage')
                                show_missing = 1)

   The Trace class can be instantited instead of the Coverage class if
   runtime display of executable lines is desired instead of statement
   converage measurement.
"""

import sys, os, string, marshal, tempfile, copy, operator

def usage(outfile):
    outfile.write("""Usage: %s [OPTIONS] <file> [ARGS]

Execution:
      --help           Display this help then exit.
      --version        Output version information then exit.
   -t,--trace          Print the line to be executed to sys.stdout.
   -c,--count          Count the number of times a line is executed.
                         Results are written in the results file, if given.
   -r,--report         Generate a report from a results file; do not
                         execute any code.
        (One of `-t', `-c' or `-r' must be specified)

I/O:
   -f,--file=          File name for accumulating results over several runs.
                         (No file name means do not archive results)
   -d,--logdir=        Directory to use when writing annotated log files.
                         Log files are the module __name__ with `.` replaced
                         by os.sep and with '.pyl' added.
   -m,--missing        Annotate all executable lines which were not executed
                         with a '>>>>>> '.
   -R,--no-report      Do not generate the annotated reports.  Useful if
                         you want to accumulate several over tests.

Selection:                 Do not trace or log lines from ...
  --ignore-module=[string]   modules with the given __name__, and submodules
                              of that module
  --ignore-dir=[string]      files in the stated directory (multiple
                              directories can be joined by os.pathsep)

  The selection options can be listed multiple times to ignore different
modules.
""" % sys.argv[0])


class Ignore:
    def __init__(self, modules = None, dirs = None):
        self._mods = modules or []
        self._dirs = dirs or []

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

def run(trace, cmd):
    import __main__
    dict = __main__.__dict__
    sys.settrace(trace)
    try:
        exec cmd in dict, dict
    finally:
        sys.settrace(None)

def runctx(trace, cmd, globals=None, locals=None):
    if globals is None: globals = {}
    if locals is None: locals = {}
    sys.settrace(trace)
    try:
        exec cmd in dict, dict
    finally:
        sys.settrace(None)

def runfunc(trace, func, *args, **kw):
    result = None
    sys.settrace(trace)
    try:
        result = apply(func, args, kw)
    finally:
        sys.settrace(None)
    return result


class CoverageResults:
    def __init__(self, counts = {}, modules = {}):
        self.counts = counts.copy()    # map (filename, lineno) to count
        self.modules = modules.copy()  # map filenames to modules

    def update(self, other):
        """Merge in the data from another CoverageResults"""
        counts = self.counts
        other_counts = other.counts
        modules = self.modules
        other_modules = other.modules

        for key in other_counts.keys():
            counts[key] = counts.get(key, 0) + other_counts[key]

        for key in other_modules.keys():
            if modules.has_key(key):
                # make sure they point to the same file
                assert modules[key] == other_modules[key], \
                      "Strange! filename %s has two different module names" % \
                      (key, modules[key], other_module[key])
            else:
                modules[key] = other_modules[key]

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
    
def create_results_log(results, dirname = ".", show_missing = 1,
                       save_counts = 0):
    import re
    # turn the counts data ("(filename, lineno) = count") into something
    # accessible on a per-file basis
    per_file = {}
    for filename, lineno in results.counts.keys():
        lines_hit = per_file[filename] = per_file.get(filename, {})
        lines_hit[lineno] = results.counts[(filename, lineno)]

    # try and merge existing counts and modules file from dirname
    try:
        counts = marshal.load(open(os.path.join(dirname, "counts")))
        modules = marshal.load(open(os.path.join(dirname, "modules")))
        results.update(results.__class__(counts, modules))
    except IOError:
        pass
    
    # there are many places where this is insufficient, like a blank
    # line embedded in a multiline string.
    blank = re.compile(r'^\s*(#.*)?$')

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

        # XXX this is almost certainly not portable!!!
        fndir = os.path.dirname(filename)
        if filename[:1] == os.sep:
            coverpath = os.path.join(dirname, "."+fndir)
        else:
            coverpath = os.path.join(dirname, fndir)

        if filename.endswith(".pyc") or filename.endswith(".pyo"):
            filename = filename[:-1]

        # Get the original lines from the .py file
        try:
            lines = open(filename, 'r').readlines()
        except IOError, err:
            sys.stderr.write("%s: Could not open %s for reading " \
                             "because: %s - skipping\n" % \
                             ("trace", `filename`, err.strerror))
            continue

        modulename = os.path.split(results.modules[key])[1]

        # build list file name by appending a ".cover" to the module name
        # and sticking it into the specified directory
        listfilename = os.path.join(coverpath, modulename + ".cover")
        #sys.stderr.write("modulename: %(modulename)s\n"
        #                 "filename: %(filename)s\n"
        #                 "coverpath: %(coverpath)s\n"
        #                 "listfilename: %(listfilename)s\n"
        #                 "dirname: %(dirname)s\n"
        #                 % locals())
        try:
            outfile = open(listfilename, 'w')
        except IOError, err:
            sys.stderr.write(
                '%s: Could not open %s for writing because: %s" \
                "- skipping\n' % ("trace", `listfilename`, err.strerror))
            continue

        # If desired, get a list of the line numbers which represent
        # executable content (returned as a dict for better lookup speed)
        if show_missing:
            executable_linenos = find_executable_linenos(filename)
        else:
            executable_linenos = {}

        lines_hit = per_file[key]
        for i in range(len(lines)):
            line = lines[i]

            # do the blank/comment match to try to mark more lines
            # (help the reader find stuff that hasn't been covered)
            if lines_hit.has_key(i+1):
                # count precedes the lines that we captured
                outfile.write('%5d: ' % lines_hit[i+1])
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
            outfile.write(string.expandtabs(lines[i], 8))

        outfile.close()

        if save_counts:
            # try and store counts and module info into dirname
            try:
                marshal.dump(results.counts,
                             open(os.path.join(dirname, "counts"), "w"))
                marshal.dump(results.modules,
                             open(os.path.join(dirname, "modules"), "w"))
            except IOError, err:
                sys.stderr.write("cannot save counts/modules " \
                                 "files because %s" % err.strerror)

# There is a lot of code shared between these two classes even though
# it is straightforward to make a super class to share code.  However,
# for performance reasons (remember, this is called at every step) I
# wanted to keep everything to a single function call.  Also, by
# staying within a single scope, I don't have to temporarily nullify
# sys.settrace, which would slow things down even more.

class Coverage:
    def __init__(self, ignore = Ignore()):
        self.ignore = ignore
        self.ignore_names = ignore._ignore # access ignore's cache (speed hack)

        self.counts = {}   # keys are (filename, linenumber)
        self.modules = {}  # maps filename -> module name

    def trace(self, frame, why, arg):
        if why == 'line':
            # something is fishy about getting the file name
            filename = frame.f_globals.get("__file__", None)
            if filename is None:
                filename = frame.f_code.co_filename
            modulename = frame.f_globals["__name__"]

            # We do this next block to keep from having to make methods
            # calls, which also requires resetting the trace
            ignore_it = self.ignore_names.get(modulename, -1)
            if ignore_it == -1:  # unknown filename
                sys.settrace(None)
                ignore_it = self.ignore.names(filename, modulename)
                sys.settrace(self.trace)

                # record the module name for every file
                self.modules[filename] = modulename

            if not ignore_it:
                lineno = frame.f_lineno

                # record the file name and line number of every trace
                key = (filename, lineno)
                self.counts[key] = self.counts.get(key, 0) + 1

        return self.trace

    def results(self):
        return CoverageResults(self.counts, self.modules)

class Trace:
    def __init__(self, ignore = Ignore()):
        self.ignore = ignore
        self.ignore_names = ignore._ignore # access ignore's cache (speed hack)

        self.files = {'<string>': None}  # stores lines from the .py file, or None

    def trace(self, frame, why, arg):
        if why == 'line':
            filename = frame.f_code.co_filename
            modulename = frame.f_globals["__name__"]

            # We do this next block to keep from having to make methods
            # calls, which also requires resetting the trace
            ignore_it = self.ignore_names.get(modulename, -1)
            if ignore_it == -1:  # unknown filename
                sys.settrace(None)
                ignore_it = self.ignore.names(filename, modulename)
                sys.settrace(self.trace)

            if not ignore_it:
                lineno = frame.f_lineno
                files = self.files

                if filename != '<string>' and not files.has_key(filename):
                    files[filename] = map(string.rstrip,
                                          open(filename).readlines())

                # If you want to see filenames (the original behaviour), try:
                #   modulename = filename
                # or, prettier but confusing when several files have the same name
                #   modulename = os.path.basename(filename)

                if files[filename] != None:
                    print '%s(%d): %s' % (os.path.basename(filename), lineno,
                                          files[filename][lineno-1])
                else:
                    print '%s(%d): ??' % (modulename, lineno)

        return self.trace
    

def _err_exit(msg):
    sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.exit(1)

def main(argv = None):
    import getopt

    if argv is None:
        argv = sys.argv
    try:
        opts, prog_argv = getopt.getopt(argv[1:], "tcrRf:d:m",
                                        ["help", "version", "trace", "count",
                                         "report", "no-report",
                                         "file=", "logdir=", "missing",
                                         "ignore-module=", "ignore-dir="])

    except getopt.error, msg:
        sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
        sys.stderr.write("Try `%s --help' for more information\n" % sys.argv[0])
        sys.exit(1)

    trace = 0
    count = 0
    report = 0
    no_report = 0
    counts_file = None
    logdir = "."
    missing = 0
    ignore_modules = []
    ignore_dirs = []

    for opt, val in opts:
        if opt == "--help":
            usage(sys.stdout)
            sys.exit(0)

        if opt == "--version":
            sys.stdout.write("trace 2.0\n")
            sys.exit(0)

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

        if opt == "-d" or opt == "--logdir":
            logdir = val
            continue

        if opt == "-m" or opt == "--missing":
            missing = 1
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

    if len(prog_argv) == 0:
        _err_exit("missing name of file to run")

    if count + trace + report > 1:
        _err_exit("can only specify one of --trace, --count or --report")

    if count + trace + report == 0:
        _err_exit("must specify one of --trace, --count or --report")

    if report and counts_file is None:
        _err_exit("--report requires a --file")

    if report and no_report:
        _err_exit("cannot specify both --report and --no-report")

    if logdir is not None:
        # warn if the directory doesn't exist, but keep on going
        # (is this the correct behaviour?)
        if not os.path.isdir(logdir):
            sys.stderr.write(
                "trace: WARNING, --logdir directory %s is not available\n" %
                       `logdir`)

    sys.argv = prog_argv
    progname = prog_argv[0]
    if eval(sys.version[:3])>1.3:
        sys.path[0] = os.path.split(progname)[0] # ???

    # everything is ready
    ignore = Ignore(ignore_modules, ignore_dirs)
    if trace:
        t = Trace(ignore)
        try:
            run(t.trace, 'execfile(' + `progname` + ')')
        except IOError, err:
            _err_exit("Cannot run file %s because: %s" % \
                      (`sys.argv[0]`, err.strerror))

    elif count:
        t = Coverage(ignore)
        try:
            run(t.trace, 'execfile(' + `progname` + ')')
        except IOError, err:
            _err_exit("Cannot run file %s because: %s" % \
                      (`sys.argv[0]`, err.strerror))
        except SystemExit:
            pass

        results = t.results()
        # Add another lookup from the program's file name to its import name
        # This give the right results, but I'm not sure why ...
        results.modules[progname] = os.path.splitext(progname)[0]

        if counts_file:
            # add in archived data, if available
            try:
                old_counts, old_modules = marshal.load(open(counts_file, 'rb'))
            except IOError:
                pass
            else:
                results.update(CoverageResults(old_counts, old_modules))

        if not no_report:
            create_results_log(results, logdir, missing)

        if counts_file:
            try:
                marshal.dump( (results.counts, results.modules),
                              open(counts_file, 'wb'))
            except IOError, err:
                _err_exit("Cannot save counts file %s because: %s" % \
                          (`counts_file`, err.strerror))

    elif report:
        old_counts, old_modules = marshal.load(open(counts_file, 'rb'))
        results = CoverageResults(old_counts, old_modules)
        create_results_log(results, logdir, missing)

    else:
        assert 0, "Should never get here"

if __name__=='__main__':
    main()
