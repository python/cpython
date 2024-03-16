#! /usr/bin/env python3
# Originally written by Barry Warsaw <barry@python.org>
#
# Minimally patched to make it even more xgettext compatible
# by Peter Funk <pf@artcom-gmbh.de>
#
# 2002-11-22 Jürgen Hermann <jh@web.de>
# Added checks that _() only contains string literals, and
# command line args are resolved to module lists, i.e. you
# can now pass a filename, a module or package name, or a
# directory (including globbing chars, important for Win32).
# Made docstring fit in 80 chars wide displays using pydoc.
#

# for selftesting
try:
    import fintl
    _ = fintl.gettext
except ImportError:
    _ = lambda s: s

__doc__ = _("""pygettext -- Python equivalent of xgettext(1)

Many systems (Solaris, Linux, Gnu) provide extensive tools that ease the
internationalization of C programs. Most of these tools are independent of
the programming language and can be used from within Python programs.
Martin von Loewis' work[1] helps considerably in this regard.

There's one problem though; xgettext is the program that scans source code
looking for message strings, but it groks only C (or C++). Python
introduces a few wrinkles, such as dual quoting characters, triple quoted
strings, and raw strings. xgettext understands none of this.

Enter pygettext, which uses Python's standard tokenize module to scan
Python source code, generating .pot files identical to what GNU xgettext[2]
generates for C and C++ code. From there, the standard GNU tools can be
used.

A word about marking Python strings as candidates for translation. GNU
xgettext recognizes the following keywords: gettext, dgettext, dcgettext,
and gettext_noop. But those can be a lot of text to include all over your
code. C and C++ have a trick: they use the C preprocessor. Most
internationalized C source includes a #define for gettext() to _() so that
what has to be written in the source is much less. Thus these are both
translatable strings:

    gettext("Translatable String")
    _("Translatable String")

Python of course has no preprocessor so this doesn't work so well.  Thus,
pygettext searches only for _() by default, but see the -k/--keyword flag
below for how to augment this.

 [1] https://www.python.org/workshops/1997-10/proceedings/loewis.html
 [2] https://www.gnu.org/software/gettext/gettext.html

NOTE: pygettext attempts to be option and feature compatible with GNU
xgettext where ever possible. However some options are still missing or are
not fully implemented. Also, xgettext's use of command line switches with
option arguments is broken, and in these cases, pygettext just defines
additional switches.

Usage: pygettext [options] inputfile ...

Options:

    -a
    --extract-all
        Extract all strings.

    -d name
    --default-domain=name
        Rename the default output file from messages.pot to name.pot.

    --charset=charset
        Character set used to read the input files and to write the
        output file (default "utf-8").

    -E
    --escape
        Replace non-ASCII characters with octal escape sequences.

    -D
    --docstrings
        Extract module, class, method, and function docstrings.  These do
        not need to be wrapped in _() markers, and in fact cannot be for
        Python to consider them docstrings. (See also the -X option).

    -h
    --help
        Print this help message and exit.

    -k word
    --keyword=word
        Keywords to look for in addition to the default set, which are:
        %(DEFAULTKEYWORDS)s

        You can have multiple -k flags on the command line.

    -K
    --no-default-keywords
        Disable the default set of keywords (see above).  Any keywords
        explicitly added with the -k/--keyword option are still recognized.

    --no-location
        Do not write filename/lineno location comments.

    -n
    --add-location
        Write filename/lineno location comments indicating where each
        extracted string is found in the source.  These lines appear before
        each msgid.  The style of comments is controlled by the -S/--style
        option.  This is the default.

    -o filename
    --output=filename
        Rename the default output file from messages.pot to filename.  If
        filename is `-' then the output is sent to standard out.

    -p dir
    --output-dir=dir
        Output files will be placed in directory dir.

    -S stylename
    --style stylename
        Specify which style to use for location comments.  Two styles are
        supported:

        Solaris  # File: filename, line: line-number
        GNU      #: filename:line

        The style name is case insensitive.  GNU style is the default.

    -v
    --verbose
        Print the names of the files being processed.

    -V
    --version
        Print the version of pygettext and exit.

    -w columns
    --width=columns
        Set width of output to columns.

    -x filename
    --exclude-file=filename
        Specify a file that contains a list of strings that are not be
        extracted from the input files.  Each string to be excluded must
        appear on a line by itself in the file.

    -X filename
    --no-docstrings=filename
        Specify a file that contains a list of files (one per line) that
        should not have their docstrings extracted.  This is only useful in
        conjunction with the -D option above.

If `inputfile' is -, standard input is read.
""")

import ast
import getopt
import glob
import importlib.machinery
import importlib.util
import os
import sys
import time
from ast import (AsyncFunctionDef, ClassDef, FunctionDef, Module, NodeVisitor,
                 unparse)
from collections import defaultdict
from dataclasses import dataclass

__version__ = '1.5'

default_keywords = ['_']
DEFAULTKEYWORDS = ', '.join(default_keywords)


# The normal pot-file header. msgmerge and Emacs's po-mode work better if it's
# there.
pot_header = _('''\
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR ORGANIZATION
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\\n"
"POT-Creation-Date: %(time)s\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=%(charset)s\\n"
"Content-Transfer-Encoding: %(encoding)s\\n"
"Generated-By: pygettext.py %(version)s\\n"

''')


def usage(code, msg=''):
    print(__doc__ % globals(), file=sys.stderr)
    if msg:
        print(msg, file=sys.stderr)
    sys.exit(code)


def make_escapes(pass_nonascii):
    global escapes, escape
    if pass_nonascii:
        # Allow non-ascii characters to pass through so that e.g. 'msgid
        # "Höhe"' would not result in 'msgid "H\366he"'.  Otherwise we
        # escape any character outside the 32..126 range.
        mod = 128
        escape = escape_ascii
    else:
        mod = 256
        escape = escape_nonascii
    escapes = [r"\%03o" % i for i in range(mod)]
    for i in range(32, 127):
        escapes[i] = chr(i)
    escapes[ord('\\')] = r'\\'
    escapes[ord('\t')] = r'\t'
    escapes[ord('\r')] = r'\r'
    escapes[ord('\n')] = r'\n'
    escapes[ord('\"')] = r'\"'


def escape_ascii(s, encoding):
    return ''.join(escapes[ord(c)] if ord(c) < 128 else c for c in s)


def escape_nonascii(s, encoding):
    return ''.join(escapes[b] for b in s.encode(encoding))


def normalize(s, encoding):
    # This converts the various Python string types into a format that is
    # appropriate for .po files, namely much closer to C style.
    lines = s.split('\n')
    if len(lines) == 1:
        s = '"' + escape(s, encoding) + '"'
    else:
        if not lines[-1]:
            del lines[-1]
            lines[-1] = lines[-1] + '\n'
        for i in range(len(lines)):
            lines[i] = escape(lines[i], encoding)
        lineterm = '\\n"\n"'
        s = '""\n"' + lineterm.join(lines) + '"'
    return s


def containsAny(str, set):
    """Check whether 'str' contains ANY of the chars in 'set'"""
    return 1 in [c in str for c in set]


def getFilesForName(name):
    """Get a list of module files for a filename, a module or package name,
    or a directory.
    """
    if not os.path.exists(name):
        # check for glob chars
        if containsAny(name, "*?[]"):
            files = glob.glob(name)
            list = []
            for file in files:
                list.extend(getFilesForName(file))
            return list

        # try to find module or package
        try:
            spec = importlib.util.find_spec(name)
            name = spec.origin
        except ImportError:
            name = None
        if not name:
            return []

    if os.path.isdir(name):
        # find all python files in directory
        list = []
        # get extension for python source files
        _py_ext = importlib.machinery.SOURCE_SUFFIXES[0]
        for root, dirs, files in os.walk(name):
            # don't recurse into CVS directories
            if 'CVS' in dirs:
                dirs.remove('CVS')
            # add all *.py files to list
            list.extend(
                [os.path.join(root, file) for file in files
                 if os.path.splitext(file)[1] == _py_ext]
                )
        return list
    elif os.path.exists(name):
        # a single file
        return [name]

    return []


@dataclass
class Message:
    filename: str
    lineno: int
    msgid: str
    is_docstring: bool = False


def get_funcname(node):
    if isinstance(node.func, ast.Name):
        return node.func.id
    elif isinstance(node.func, ast.Attribute):
        return node.func.attr
    else:
        return None


class GettextVisitor(NodeVisitor):
    def __init__(self, options, filename=None):
        super().__init__()
        self.options = options
        self.filename = filename
        self.messages = defaultdict(list)

    def _is_string_const(self, node):
        return isinstance(node, ast.Constant) and isinstance(node.value, str)

    def _extract_docstring(self, node):
        if not self.options.docstrings or self.options.nodocstrings.get(self.filename):
            return

        if not node.body:
            return

        expr = node.body[0]
        if isinstance(expr, ast.Expr) and self._is_string_const(expr.value):
            if docstring := ast.get_docstring(node):
                message = Message(self.filename, expr.lineno, docstring, is_docstring=True)
                self.messages[docstring].append(message)

    def _extract_message(self, node):
        funcname = get_funcname(node)
        if funcname not in self.options.keywords:
            return

        filename = self.filename
        lineno = node.lineno

        if len(node.args) != 1:
            print(f'*** {filename}:{lineno}: Seen unexpected amount of '
                  f'positional arguments in gettext call: {unparse(node)}',
                  file=sys.stderr)
            return

        if node.keywords:
            print(f'*** {filename}:{lineno}: Seen unexpected keyword arguments '
                  f'in gettext call: {unparse(node)}', file=sys.stderr)
            return

        arg = node.args[0]
        if not self._is_string_const(arg):
            print(f'*** {filename}:{lineno}: Seen unexpected argument type '
                  f'in gettext call: {unparse(node)}', file=sys.stderr)
            return

        msgid = arg.value
        if msgid == '':
            print(f'*** {filename}:{lineno}: Empty msgid. It is reserved by GNU gettext: '
                  'gettext("") returns the header entry with '
                  'meta information, not the empty string.',
                  file=sys.stderr)
            return

        message = Message(filename, lineno, msgid)
        self.messages[msgid].append(message)

    def visit(self, node):
        if type(node) in {Module, FunctionDef, AsyncFunctionDef, ClassDef}:
            self._extract_docstring(node)
        super().visit(node)

    def visit_Call(self, node):
        self._extract_message(node)
        self.generic_visit(node)


def format_pot_file(all_messages, options, encoding):
    timestamp = time.strftime('%Y-%m-%d %H:%M%z')
    output = pot_header % {'time': timestamp, 'version': __version__,
                           'charset': encoding,
                           'encoding': '8bit'}
    inverted = defaultdict(dict)

    for msgid, messages in all_messages.items():
        occurrences = set((msg.filename, msg.lineno) for msg in messages)
        occurrences = tuple(sorted(occurrences))
        inverted[occurrences][msgid] = messages

    sorted_occurrences = sorted(list(inverted.keys()))

    for occurrences in sorted_occurrences:
        messages_dict = inverted[occurrences]
        for msgid, messages in messages_dict.items():
            if options.writelocations:
                locline = '#:'
                for (filename, lineno) in occurrences:
                    s = f' {filename}:{lineno}'
                    if len(locline) + len(s) <= options.width:
                        locline = locline + s
                    else:
                        output += locline + '\n'
                        locline = '#:' + s
                if len(locline) > 2:
                    output += locline + '\n'

            # If the entry was gleaned out of a docstring, then add a
            # comment stating so. This is to aid translators who may wish
            # to skip translating some unimportant docstrings.
            is_docstring = all(msg.is_docstring for msg in messages)
            if is_docstring:
                output += '#, docstring\n'

            output += f'msgid {normalize(msgid, encoding)}\n'
            output += 'msgstr ""\n'
            output += '\n'

    return output


def main():
    global default_keywords
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            'ad:DEhk:Kno:p:S:Vvw:x:X:',
            ['extract-all', 'default-domain=', 'charset=', 'escape', 'help',
             'keyword=', 'no-default-keywords',
             'add-location', 'no-location', 'output=', 'output-dir=',
             'style=', 'verbose', 'version', 'width=', 'exclude-file=',
             'docstrings', 'no-docstrings',
             ])
    except getopt.error as msg:
        usage(1, msg)

    # for holding option values
    class Options:
        # constants
        GNU = 1
        SOLARIS = 2
        # defaults
        extractall = 0 # FIXME: currently this option has no effect at all.
        charset = 'utf-8'
        escape = 0
        keywords = []
        outpath = ''
        outfile = 'messages.pot'
        writelocations = 1
        locationstyle = GNU
        verbose = 0
        width = 78
        excludefilename = ''
        docstrings = 0
        nodocstrings = {}

    options = Options()
    locations = {'gnu' : options.GNU,
                 'solaris' : options.SOLARIS,
                 }

    # parse options
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt in ('-a', '--extract-all'):
            options.extractall = 1
        elif opt in ('-d', '--default-domain'):
            options.outfile = arg + '.pot'
        elif opt in ('--charset',):
            options.charset = arg
        elif opt in ('-E', '--escape'):
            options.escape = 1
        elif opt in ('-D', '--docstrings'):
            options.docstrings = 1
        elif opt in ('-k', '--keyword'):
            options.keywords.append(arg)
        elif opt in ('-K', '--no-default-keywords'):
            default_keywords = []
        elif opt in ('-n', '--add-location'):
            options.writelocations = 1
        elif opt in ('--no-location',):
            options.writelocations = 0
        elif opt in ('-S', '--style'):
            options.locationstyle = locations.get(arg.lower())
            if options.locationstyle is None:
                usage(1, _('Invalid value for --style: %s') % arg)
        elif opt in ('-o', '--output'):
            options.outfile = arg
        elif opt in ('-p', '--output-dir'):
            options.outpath = arg
        elif opt in ('-v', '--verbose'):
            options.verbose = 1
        elif opt in ('-V', '--version'):
            print(_('pygettext.py (xgettext for Python) %s') % __version__)
            sys.exit(0)
        elif opt in ('-w', '--width'):
            try:
                options.width = int(arg)
            except ValueError:
                usage(1, _('--width argument must be an integer: %s') % arg)
        elif opt in ('-x', '--exclude-file'):
            options.excludefilename = arg
        elif opt in ('-X', '--no-docstrings'):
            fp = open(arg)
            try:
                while 1:
                    line = fp.readline()
                    if not line:
                        break
                    options.nodocstrings[line[:-1]] = 1
            finally:
                fp.close()

    # calculate escapes
    make_escapes(not options.escape)

    # calculate all keywords
    options.keywords.extend(default_keywords)

    # initialize list of strings to exclude
    if options.excludefilename:
        try:
            with open(options.excludefilename) as fp:
                options.toexclude = fp.readlines()
        except IOError:
            print(_(
                "Can't read --exclude-file: %s") % options.excludefilename, file=sys.stderr)
            sys.exit(1)
    else:
        options.toexclude = []

    # resolve args to module lists
    expanded = []
    for arg in args:
        if arg == '-':
            expanded.append(arg)
        else:
            expanded.extend(getFilesForName(arg))
    args = expanded

    # slurp through all the files
    visitor = GettextVisitor(options)
    for filename in args:
        if filename == '-':
            if options.verbose:
                print(_('Reading standard input'))
            fp = sys.stdin.buffer
            closep = 0
        else:
            if options.verbose:
                print(_('Working on %s') % filename)
            fp = open(filename, encoding=options.charset)
            closep = 1
        try:
            visitor.filename = filename
            visitor.visit(ast.parse(fp.read()))
        finally:
            if closep:
                fp.close()

    # write the output
    if options.outfile == '-':
        fp = sys.stdout
        closep = 0
    else:
        if options.outpath:
            options.outfile = os.path.join(options.outpath, options.outfile)
        fp = open(options.outfile, 'w', encoding=options.charset)
        closep = 1
    try:
        fp.write(format_pot_file(visitor.messages, options, options.charset))
    finally:
        if closep:
            fp.close()


if __name__ == '__main__':
    main()
    # some more test strings
    # this one creates a warning
    _('*** Seen unexpected token "%(token)s"') % {'token': 'test'}
    _('more' 'than' 'one' 'string')
