"""
compilerlike -- framework code for building compiler-like programs.

There is a common `compiler-like' pattern in Unix scripts which is useful
for translation utilities of all sorts.  A program following this pattern
behaves as a filter when no argument files are specified on the command
line, but otherwise transforms each file individually into a corresponding
output file.

This module provides framework and glue code to make such programs easy
to write.  You supply a function to massage the file data; depending
on which entry point you use, it can take input and output file pointers,
or it can take a string consisting of the entire file's data and return
a replacement, or it can take in succession strings consisting of each
of the file's lines and return a translated line for each.

Argument files are transformed in left to right order in the argument list.
A filename consisting of a dash is interpreted as a directive to read from
standard input (this can be useful in pipelines).

Replacement of each file is atomic and doesn't occur until the
translation of that file has completed.  Any tempfiles are removed
automatically on any exception thrown by the translation function,
and the exception is then passed upwards.
"""

# Requires Python 2.
from __future__ import nested_scopes

import sys, os, filecmp, traceback

def filefilter(name, arguments, trans_data, trans_filename=None):
    "Filter stdin to stdout, or file arguments to renamed files."
    if not arguments:
        trans_data("stdin", sys.stdin, sys.stdout)
    else:
        for file in arguments:
            if file == '-':		# - is conventional for stdin
                file = "stdin"
                infp = sys.stdin
            else:
                infp = open(file)
            tempfile = file + ".~%s-%d~" % (name, os.getpid())
            outfp = open(tempfile, "w")
            try:
                trans_data(file, infp, outfp)
            except:
                os.remove(tempfile)
                # Pass the exception upwards
                (exc_type, exc_value, exc_traceback) = sys.exc_info()
                raise exc_type, exc_value, exc_traceback
            if filecmp.cmp(file, tempfile):
                os.remove(tempfile)
            else:
                if not trans_filename:
                    os.rename(tempfile, file)
                elif type(trans_filename) == type(""):
                    i = file.rfind(trans_filename[0])
                    if i > -1:
                        file = file[:i]
                    os.rename(tempfile, stem + trans_filename)
                else:
                    os.rename(tempfile, trans_filename(file))

def line_by_line(name, infp, outfp, translate_line):
    "Hook to do line-by-line translation for filters."
    while 1:
        line = infp.readline()
        if line == "":
            break
        elif line:	# None returns are skipped
            outfp.write(translate_line(name, line))

def linefilter(name, arguments, trans_data, trans_filename=None):
    "Filter framework for line-by-line transformation."
    return filefilter(name,
                  arguments,
                  lambda name, infp, outfp: line_by_line(name, infp, outfp, trans_data),
                  trans_filename)

def sponge(name, arguments, trans_data, trans_filename=None):
    "Read input sources entire and transform them in memory."
    if not arguments:
        sys.stdout.write(trans_data(name, sys.stdin.read()))
    else:
        for file in arguments:
            infp = open(file)
            indoc = infp.read()
            infp.close()
            tempfile = file + ".~%s-%d~" % (name, os.getpid())
            try:
                outfp = open(tempfile, "w")
            except OSError:
                sys.stderr.write("%s: can't open tempfile" % name)
                return 1
            try:
                outdoc = trans_data(name, indoc)
            except:
                os.remove(tempfile)
                # Pass the exception upwards
                (exc_type, exc_value, exc_traceback) = sys.exc_info()
                raise exc_type, exc_value, exc_traceback
            if outdoc == indoc:
                os.remove(tempfile)
            else:
                outfp.write(outdoc)
                if not trans_filename:
                    os.rename(tempfile, file)
                elif type(trans_filename) == type(""):
                    i = file.rfind(trans_filename[0])
                    if i > -1:
                        file = file[:i]
                    os.rename(tempfile, file + trans_filename)
                else:
                    os.rename(tempfile, trans_filename(file))

if __name__ == '__main__':
    import getopt

    def nametrans(name):
        return name + ".out"

    def filefilter_test(name, infp, outfp):
        "Test hook for filefilter entry point -- put dashes before blank lines."
        while 1:
            line = infp.readline()
            if not line:
                break
            if line == "\n":
                outfp.write("------------------------------------------\n")
            outfp.write(line)

    def linefilter_test(name, data):
        "Test hook for linefilter entry point -- wrap lines in brackets."
        return "<" + data[:-1] + ">\n"

    def sponge_test(name, data):
        "Test hook for the sponge entry point -- reverse file lines."
        lines = data.split("\n")
        lines.reverse()
        return "\n".join(lines)

    (options, arguments) = getopt.getopt(sys.argv[1:], "fls")
    for (switch, val) in options:
        if switch == '-f':
            filefilter("filefilter_test", arguments, filefilter_test,nametrans)
        elif switch == '-l':
            linefilter("linefilter_test", arguments, linefilter_test,nametrans)
        elif switch == '-s':
            sponge("sponge_test", arguments, sponge_test, ".foo")
        else:
            print "Unknown option."

# End
