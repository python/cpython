#! /usr/bin/env python3
# -*- coding: iso-8859-1 -*-
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

import os
import importlib.machinery
import importlib.util
import sys
import glob
import time
import getopt
import ast
import tokenize
from collections import defaultdict
from dataclasses import dataclass, field
from operator import itemgetter

__version__ = '1.5'


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
        # "Höhe"' would result not result in 'msgid "H\366he"'.  Otherwise we
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


def is_literal_string(s):
    return s[0] in '\'"' or (s[0] in 'rRuU' and s[1] in '\'"')


def safe_eval(s):
    # unwrap quotes, safely
    return eval(s, {'__builtins__':{}}, {})


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


# Key is the function name, value is a dictionary mapping argument positions to the
# type of the argument. The type is one of 'msgid', 'msgid_plural', or 'msgctxt'.
DEFAULTKEYWORDS = {
    '_': {0: 'msgid'},
    'gettext': {0: 'msgid'},
    'ngettext': {0: 'msgid', 1: 'msgid_plural'},
    'pgettext': {0: 'msgctxt', 1: 'msgid'},
    'npgettext': {0: 'msgctxt', 1: 'msgid', 2: 'msgid_plural'},
    'dgettext': {1: 'msgid'},
    'dngettext': {1: 'msgid', 2: 'msgid_plural'},
    'dpgettext': {1: 'msgctxt', 2: 'msgid'},
    'dnpgettext': {1: 'msgctxt', 2: 'msgid', 3: 'msgid_plural'},
}


def matches_spec(message, spec):
    """Check if a message has all the keys defined by the keyword spec."""
    return all(key in message for key in spec.values())


@dataclass(frozen=True)
class Location:
    filename: str
    lineno: int

    def __lt__(self, other):
        return (self.filename, self.lineno) < (other.filename, other.lineno)


@dataclass
class Message:
    msgid: str
    msgid_plural: str | None
    msgctxt: str | None
    locations: set[Location] = field(default_factory=set)
    is_docstring: bool = False

    def add_location(self, filename, lineno, msgid_plural=None, *, is_docstring=False):
        if self.msgid_plural is None:
            self.msgid_plural = msgid_plural
        self.locations.add(Location(filename, lineno))
        self.is_docstring |= is_docstring


def key_for(msgid, msgctxt=None):
    if msgctxt is not None:
        return (msgctxt, msgid)
    return msgid


class TokenEater:
    def __init__(self, options):
        self.__options = options
        self.__messages = {}
        self.__state = self.__waiting
        self.__data = defaultdict(str)
        self.__curr_arg = 0
        self.__curr_keyword = None
        self.__lineno = -1
        self.__freshmodule = 1
        self.__curfile = None
        self.__enclosurecount = 0

    def __call__(self, ttype, tstring, stup, etup, line):
        # dispatch
##        import token
##        print('ttype:', token.tok_name[ttype], 'tstring:', tstring,
##              file=sys.stderr)
        self.__state(ttype, tstring, stup[0])

    def __waiting(self, ttype, tstring, lineno):
        opts = self.__options
        # Do docstring extractions, if enabled
        if opts.docstrings and not opts.nodocstrings.get(self.__curfile):
            # module docstring?
            if self.__freshmodule:
                if ttype == tokenize.STRING and is_literal_string(tstring):
                    self.__addentry({'msgid': safe_eval(tstring)}, lineno, is_docstring=True)
                    self.__freshmodule = 0
                    return
                if ttype in (tokenize.COMMENT, tokenize.NL, tokenize.ENCODING):
                    return
                self.__freshmodule = 0
            # class or func/method docstring?
            if ttype == tokenize.NAME and tstring in ('class', 'def'):
                self.__state = self.__suiteseen
                return
        if ttype == tokenize.NAME and tstring in ('class', 'def'):
            self.__state = self.__ignorenext
            return
        if ttype == tokenize.NAME and tstring in opts.keywords:
            self.__state = self.__keywordseen
            self.__curr_keyword = tstring
            return
        if ttype == tokenize.STRING:
            maybe_fstring = ast.parse(tstring, mode='eval').body
            if not isinstance(maybe_fstring, ast.JoinedStr):
                return
            for value in filter(lambda node: isinstance(node, ast.FormattedValue),
                                maybe_fstring.values):
                for call in filter(lambda node: isinstance(node, ast.Call),
                                   ast.walk(value)):
                    func = call.func
                    if isinstance(func, ast.Name):
                        func_name = func.id
                    elif isinstance(func, ast.Attribute):
                        func_name = func.attr
                    else:
                        continue

                    if func_name not in opts.keywords:
                        continue
                    if len(call.args) != 1:
                        print(_(
                            '*** %(file)s:%(lineno)s: Seen unexpected amount of'
                            ' positional arguments in gettext call: %(source_segment)s'
                            ) % {
                            'source_segment': ast.get_source_segment(tstring, call) or tstring,
                            'file': self.__curfile,
                            'lineno': lineno
                            }, file=sys.stderr)
                        continue
                    if call.keywords:
                        print(_(
                            '*** %(file)s:%(lineno)s: Seen unexpected keyword arguments'
                            ' in gettext call: %(source_segment)s'
                            ) % {
                            'source_segment': ast.get_source_segment(tstring, call) or tstring,
                            'file': self.__curfile,
                            'lineno': lineno
                            }, file=sys.stderr)
                        continue
                    arg = call.args[0]
                    if not isinstance(arg, ast.Constant):
                        print(_(
                            '*** %(file)s:%(lineno)s: Seen unexpected argument type'
                            ' in gettext call: %(source_segment)s'
                            ) % {
                            'source_segment': ast.get_source_segment(tstring, call) or tstring,
                            'file': self.__curfile,
                            'lineno': lineno
                            }, file=sys.stderr)
                        continue
                    if isinstance(arg.value, str):
                        self.__curr_keyword = func_name
                        self.__addentry({'msgid': arg.value}, lineno)

    def __suiteseen(self, ttype, tstring, lineno):
        # skip over any enclosure pairs until we see the colon
        if ttype == tokenize.OP:
            if tstring == ':' and self.__enclosurecount == 0:
                # we see a colon and we're not in an enclosure: end of def
                self.__state = self.__suitedocstring
            elif tstring in '([{':
                self.__enclosurecount += 1
            elif tstring in ')]}':
                self.__enclosurecount -= 1

    def __suitedocstring(self, ttype, tstring, lineno):
        # ignore any intervening noise
        if ttype == tokenize.STRING and is_literal_string(tstring):
            self.__addentry({'msgid': safe_eval(tstring)}, lineno, is_docstring=True)
            self.__state = self.__waiting
        elif ttype not in (tokenize.NEWLINE, tokenize.INDENT,
                           tokenize.COMMENT):
            # there was no class docstring
            self.__state = self.__waiting

    def __keywordseen(self, ttype, tstring, lineno):
        if ttype == tokenize.OP and tstring == '(':
            self.__data.clear()
            self.__curr_arg = 0
            self.__enclosurecount = 0
            self.__lineno = lineno
            self.__state = self.__openseen
        else:
            self.__state = self.__waiting

    def __openseen(self, ttype, tstring, lineno):
        spec = self.__options.keywords[self.__curr_keyword]
        arg_type = spec.get(self.__curr_arg)
        expect_string_literal = arg_type is not None

        if ttype == tokenize.OP and self.__enclosurecount == 0:
            if tstring == ')':
                # We've seen the last of the translatable strings.  Record the
                # line number of the first line of the strings and update the list
                # of messages seen.  Reset state for the next batch.  If there
                # were no strings inside _(), then just ignore this entry.
                if self.__data:
                    self.__addentry(self.__data)
                self.__state = self.__waiting
                return
            elif tstring == ',':
                # Advance to the next argument
                self.__curr_arg += 1
                return

        if expect_string_literal:
            if ttype == tokenize.STRING and is_literal_string(tstring):
                self.__data[arg_type] += safe_eval(tstring)
            elif ttype not in (tokenize.COMMENT, tokenize.INDENT, tokenize.DEDENT,
                               tokenize.NEWLINE, tokenize.NL):
                # We are inside an argument which is a translatable string and
                # we encountered a token that is not a string.  This is an error.
                self.warn_unexpected_token(tstring)
                self.__enclosurecount = 0
                self.__state = self.__waiting
        elif ttype == tokenize.OP:
            if tstring in '([{':
                self.__enclosurecount += 1
            elif tstring in ')]}':
                self.__enclosurecount -= 1

    def __ignorenext(self, ttype, tstring, lineno):
        self.__state = self.__waiting

    def __addentry(self, msg, lineno=None, *, is_docstring=False):
        msgid = msg.get('msgid')
        if msgid in self.__options.toexclude:
            return
        if not is_docstring:
            spec = self.__options.keywords[self.__curr_keyword]
            if not matches_spec(msg, spec):
                return
        if lineno is None:
            lineno = self.__lineno
        msgctxt = msg.get('msgctxt')
        msgid_plural = msg.get('msgid_plural')
        key = key_for(msgid, msgctxt)
        if key in self.__messages:
            self.__messages[key].add_location(
                self.__curfile,
                lineno,
                msgid_plural,
                is_docstring=is_docstring,
            )
        else:
            self.__messages[key] = Message(
                msgid=msgid,
                msgid_plural=msgid_plural,
                msgctxt=msgctxt,
                locations={Location(self.__curfile, lineno)},
                is_docstring=is_docstring,
            )

    def warn_unexpected_token(self, token):
        print(_(
            '*** %(file)s:%(lineno)s: Seen unexpected token "%(token)s"'
            ) % {
            'token': token,
            'file': self.__curfile,
            'lineno': self.__lineno
            }, file=sys.stderr)

    def set_filename(self, filename):
        self.__curfile = filename
        self.__freshmodule = 1

    def write(self, fp):
        options = self.__options
        timestamp = time.strftime('%Y-%m-%d %H:%M%z')
        encoding = fp.encoding if fp.encoding else 'UTF-8'
        print(pot_header % {'time': timestamp, 'version': __version__,
                            'charset': encoding,
                            'encoding': '8bit'}, file=fp)

        # Sort locations within each message by filename and lineno
        sorted_keys = [
            (key, sorted(msg.locations))
            for key, msg in self.__messages.items()
        ]
        # Sort messages by locations
        # For example, a message with locations [('test.py', 1), ('test.py', 2)] will
        # appear before a message with locations [('test.py', 1), ('test.py', 3)]
        sorted_keys.sort(key=itemgetter(1))

        for key, locations in sorted_keys:
            msg = self.__messages[key]
            if options.writelocations:
                # location comments are different b/w Solaris and GNU:
                if options.locationstyle == options.SOLARIS:
                    for location in locations:
                        print(f'# File: {location.filename}, line: {location.lineno}', file=fp)
                elif options.locationstyle == options.GNU:
                    # fit as many locations on one line, as long as the
                    # resulting line length doesn't exceed 'options.width'
                    locline = '#:'
                    for location in locations:
                        s = f' {location.filename}:{location.lineno}'
                        if len(locline) + len(s) <= options.width:
                            locline = locline + s
                        else:
                            print(locline, file=fp)
                            locline = f'#:{s}'
                    if len(locline) > 2:
                        print(locline, file=fp)
            if msg.is_docstring:
                # If the entry was gleaned out of a docstring, then add a
                # comment stating so.  This is to aid translators who may wish
                # to skip translating some unimportant docstrings.
                print('#, docstring', file=fp)
            if msg.msgctxt is not None:
                print('msgctxt', normalize(msg.msgctxt, encoding), file=fp)
            print('msgid', normalize(msg.msgid, encoding), file=fp)
            if msg.msgid_plural is not None:
                print('msgid_plural', normalize(msg.msgid_plural, encoding), file=fp)
                print('msgstr[0] ""', file=fp)
                print('msgstr[1] ""\n', file=fp)
            else:
                print('msgstr ""\n', file=fp)


def main():
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            'ad:DEhk:Kno:p:S:Vvw:x:X:',
            ['extract-all', 'default-domain=', 'escape', 'help',
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
    no_default_keywords = False
    # parse options
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt in ('-a', '--extract-all'):
            options.extractall = 1
        elif opt in ('-d', '--default-domain'):
            options.outfile = arg + '.pot'
        elif opt in ('-E', '--escape'):
            options.escape = 1
        elif opt in ('-D', '--docstrings'):
            options.docstrings = 1
        elif opt in ('-k', '--keyword'):
            options.keywords.append(arg)
        elif opt in ('-K', '--no-default-keywords'):
            no_default_keywords = True
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
    options.keywords = {kw: {0: 'msgid'} for kw in options.keywords}
    if not no_default_keywords:
        options.keywords |= DEFAULTKEYWORDS

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
    eater = TokenEater(options)
    for filename in args:
        if filename == '-':
            if options.verbose:
                print(_('Reading standard input'))
            fp = sys.stdin.buffer
            closep = 0
        else:
            if options.verbose:
                print(_('Working on %s') % filename)
            fp = open(filename, 'rb')
            closep = 1
        try:
            eater.set_filename(filename)
            try:
                tokens = tokenize.tokenize(fp.readline)
                for _token in tokens:
                    eater(*_token)
            except tokenize.TokenError as e:
                print('%s: %s, line %d, column %d' % (
                    e.args[0], filename, e.args[1][0], e.args[1][1]),
                    file=sys.stderr)
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
        fp = open(options.outfile, 'w')
        closep = 1
    try:
        eater.write(fp)
    finally:
        if closep:
            fp.close()


if __name__ == '__main__':
    main()
    # some more test strings
    # this one creates a warning
    _('*** Seen unexpected token "%(token)s"') % {'token': 'test'}
    _('more' 'than' 'one' 'string')
