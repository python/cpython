#! /usr/bin/env python
# Originally written by Barry Warsaw <bwarsaw@python.org>
#
# minimally patched to make it even more xgettext compatible 
# by Peter Funk <pf@artcom-gmbh.de>

# for selftesting
try:
    import fintl
    _ = fintl.gettext
except ImportError:
    def _(s): return s


__doc__ = _("""pygettext -- Python equivalent of xgettext(1)

Many systems (Solaris, Linux, Gnu) provide extensive tools that ease the
internationalization of C programs.  Most of these tools are independent of
the programming language and can be used from within Python programs.  Martin
von Loewis' work[1] helps considerably in this regard.

There's one problem though; xgettext is the program that scans source code
looking for message strings, but it groks only C (or C++).  Python introduces
a few wrinkles, such as dual quoting characters, triple quoted strings, and
raw strings.  xgettext understands none of this.

Enter pygettext, which uses Python's standard tokenize module to scan Python
source code, generating .pot files identical to what GNU xgettext[2] generates
for C and C++ code.  From there, the standard GNU tools can be used.

A word about marking Python strings as candidates for translation.  GNU
xgettext recognizes the following keywords: gettext, dgettext, dcgettext, and
gettext_noop.  But those can be a lot of text to include all over your code.
C and C++ have a trick: they use the C preprocessor.  Most internationalized C
source includes a #define for gettext() to _() so that what has to be written
in the source is much less.  Thus these are both translatable strings:

    gettext("Translatable String")
    _("Translatable String")

Python of course has no preprocessor so this doesn't work so well.  Thus,
pygettext searches only for _() by default, but see the -k/--keyword flag
below for how to augment this.

 [1] http://www.python.org/workshops/1997-10/proceedings/loewis.html
 [2] http://www.gnu.org/software/gettext/gettext.html

NOTE: pygettext attempts to be option and feature compatible with GNU xgettext
where ever possible.  However some options are still missing or are not fully
implemented.  Also, xgettext's use of command line switches with option
arguments is broken, and in these cases, pygettext just defines additional
switches.

Usage: pygettext [options] inputfile ...

Options:

    -a
    --extract-all
        Extract all strings

    -d name
    --default-domain=name
        Rename the default output file from messages.pot to name.pot 

    -E
    --escape
        replace non-ASCII characters with octal escape sequences.

    -h
    --help
        print this help message and exit

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

    -S stylename
    --style stylename
        Specify which style to use for location comments.  Two styles are
        supported:

        Solaris  # File: filename, line: line-number
        GNU      #: filename:line

        The style name is case insensitive.  GNU style is the default.

    -o filename
    --output=filename
        Rename the default output file from messages.pot to filename.  If
        filename is `-' then the output is sent to standard out.

    -p dir
    --output-dir=dir
        Output files will be placed in directory dir.

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

If `inputfile' is -, standard input is read.

""")

import os
import sys
import time
import getopt
import tokenize

__version__ = '1.1'

default_keywords = ['_']
DEFAULTKEYWORDS = ', '.join(default_keywords)

EMPTYSTRING = ''



# The normal pot-file header. msgmerge and EMACS' po-mode work better if
# it's there.
pot_header = _('''\
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR ORGANIZATION
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\\n"
"PO-Revision-Date: %(time)s\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=CHARSET\\n"
"Content-Transfer-Encoding: ENCODING\\n"
"Generated-By: pygettext.py %(version)s\\n"

''')


def usage(code, msg=''):
    print __doc__ % globals()
    if msg:
        print msg
    sys.exit(code)



escapes = []

def make_escapes(pass_iso8859):
    global escapes
    if pass_iso8859:
        # Allow iso-8859 characters to pass through so that e.g. 'msgid
        # "Höhe"' would result not result in 'msgid "H\366he"'.  Otherwise we
        # escape any character outside the 32..126 range.
        mod = 128
    else:
        mod = 256
    for i in range(256):
        if 32 <= (i % mod) <= 126:
            escapes.append(chr(i))
        else:
            escapes.append("\\%03o" % i)
    escapes[ord('\\')] = '\\\\'
    escapes[ord('\t')] = '\\t'
    escapes[ord('\r')] = '\\r'
    escapes[ord('\n')] = '\\n'
    escapes[ord('\"')] = '\\"'


def escape(s):
    global escapes
    s = list(s)
    for i in range(len(s)):
        s[i] = escapes[ord(s[i])]
    return EMPTYSTRING.join(s)


def safe_eval(s):
    # unwrap quotes, safely
    return eval(s, {'__builtins__':{}}, {})


def normalize(s):
    # This converts the various Python string types into a format that is
    # appropriate for .po files, namely much closer to C style.
    lines = s.split('\n')
    if len(lines) == 1:
        s = '"' + escape(s) + '"'
    else:
        if not lines[-1]:
            del lines[-1]
            lines[-1] = lines[-1] + '\n'
        for i in range(len(lines)):
            lines[i] = escape(lines[i])
        lineterm = '\\n"\n"'
        s = '""\n"' + lineterm.join(lines) + '"'
    return s



class TokenEater:
    def __init__(self, options):
        self.__options = options
        self.__messages = {}
        self.__state = self.__waiting
        self.__data = []
        self.__lineno = -1

    def __call__(self, ttype, tstring, stup, etup, line):
        # dispatch
        self.__state(ttype, tstring, stup[0])

    def __waiting(self, ttype, tstring, lineno):
        if ttype == tokenize.NAME and tstring in self.__options.keywords:
            self.__state = self.__keywordseen

    def __keywordseen(self, ttype, tstring, lineno):
        if ttype == tokenize.OP and tstring == '(':
            self.__data = []
            self.__lineno = lineno
            self.__state = self.__openseen
        else:
            self.__state = self.__waiting

    def __openseen(self, ttype, tstring, lineno):
        if ttype == tokenize.OP and tstring == ')':
            # We've seen the last of the translatable strings.  Record the
            # line number of the first line of the strings and update the list 
            # of messages seen.  Reset state for the next batch.  If there
            # were no strings inside _(), then just ignore this entry.
            if self.__data:
                msg = EMPTYSTRING.join(self.__data)
                if not msg in self.__options.toexclude:
                    entry = (self.__curfile, self.__lineno)
                    linenos = self.__messages.get(msg)
                    if linenos is None:
                        self.__messages[msg] = [entry]
                    else:
                        linenos.append(entry)
            self.__state = self.__waiting
        elif ttype == tokenize.STRING:
            self.__data.append(safe_eval(tstring))
        # TBD: should we warn if we seen anything else?

    def set_filename(self, filename):
        self.__curfile = filename

    def write(self, fp):
        options = self.__options
        timestamp = time.ctime(time.time())
        # common header
        try:
            sys.stdout = fp
            # The time stamp in the header doesn't have the same format
            # as that generated by xgettext...
            print pot_header % {'time': timestamp, 'version': __version__}
            for k, v in self.__messages.items():
                if not options.writelocations:
                    pass
                # location comments are different b/w Solaris and GNU:
                elif options.locationstyle == options.SOLARIS:
                    for filename, lineno in v:
                        d = {'filename': filename, 'lineno': lineno}
                        print _('# File: %(filename)s, line: %(lineno)d') % d
                elif options.locationstyle == options.GNU:
                    # fit as many locations on one line, as long as the
                    # resulting line length doesn't exceeds 'options.width'
                    locline = '#:'
                    for filename, lineno in v:
                        d = {'filename': filename, 'lineno': lineno}
                        s = _(' %(filename)s:%(lineno)d') % d
                        if len(locline) + len(s) <= options.width:
                            locline = locline + s
                        else:
                            print locline
                            locline = "#:" + s
                    if len(locline) > 2:
                        print locline
                # TBD: sorting, normalizing
                print 'msgid', normalize(k)
                print 'msgstr ""\n'
        finally:
            sys.stdout = sys.__stdout__


def main():
    global default_keywords
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            'ad:Ehk:Kno:p:S:Vvw:x:',
            ['extract-all', 'default-domain', 'escape', 'help',
             'keyword=', 'no-default-keywords',
             'add-location', 'no-location', 'output=', 'output-dir=',
             'style=', 'verbose', 'version', 'width=', 'exclude-file=',
             ])
    except getopt.error, msg:
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
        elif opt in ('-E', '--escape'):
            options.escape = 1
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
            print _('pygettext.py (xgettext for Python) %s') % __version__
            sys.exit(0)
        elif opt in ('-w', '--width'):
            try:
                options.width = int(arg)
            except ValueError:
                usage(1, _('--width argument must be an integer: %s') % arg)
        elif opt in ('-x', '--exclude-file'):
            options.excludefilename = arg

    # calculate escapes
    make_escapes(options.escape)

    # calculate all keywords
    options.keywords.extend(default_keywords)

    # initialize list of strings to exclude
    if options.excludefilename:
        try:
            fp = open(options.excludefilename)
            options.toexclude = fp.readlines()
            fp.close()
        except IOError:
            sys.stderr.write(_("Can't read --exclude-file: %s") %
                             options.excludefilename)
            sys.exit(1)
    else:
        options.toexclude = []

    # slurp through all the files
    eater = TokenEater(options)
    for filename in args:
        if filename == '-':
            if options.verbose:
                print _('Reading standard input')
            fp = sys.stdin
            closep = 0
        else:
            if options.verbose:
                print _('Working on %s') % filename
            fp = open(filename)
            closep = 1
        try:
            eater.set_filename(filename)
            tokenize.tokenize(fp.readline, eater)
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
    _(u'a unicode string')
