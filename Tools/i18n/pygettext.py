#! /usr/bin/env python
# Originally written by Barry Warsaw <bwarsaw@python.org>

"""pygettext -- Python equivalent of xgettext(1)

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
where ever possible.

Usage: pygettext [options] filename ...

Options:

    -a
    --extract-all
        Extract all strings

    -d default-domain
    --default-domain=default-domain
        Rename the default output file from messages.pot to default-domain.pot 

    -k [word]
    --keyword[=word]
        Additional keywords to look for.  Without `word' means not to use the
        default keywords.  The default keywords, which are always looked for
        if not explicitly disabled: _

        The default keyword list is different than GNU xgettext. You can have
        multiple -k flags on the command line.

    --no-location
        Do not write filename/lineno location comments

    -n [style]
    --add-location[=style]
        Write filename/lineno location comments indicating where each
        extracted string is found in the source.  These lines appear before
        each msgid.  Two styles are supported:

        Solaris  # File: filename, line: line-number
        Gnu      #: filename:line

        If style is omitted, Gnu is used.  The style name is case
        insensitive.  By default, locations are included.

    -v
    --verbose
        Print the names of the files being processed.

    --help
    -h
        print this help message and exit

"""

import os
import sys
import string
import time
import getopt
import tokenize

__version__ = '0.2'



# for selftesting
def _(s): return s


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
for i in range(256):
    if i < 32 or i > 127:
        escapes.append("\\%03o" % i)
    else:
        escapes.append(chr(i))

escapes[ord('\\')] = '\\\\'
escapes[ord('\t')] = '\\t'
escapes[ord('\r')] = '\\r'
escapes[ord('\n')] = '\\n'
escapes[ord('\"')] = '\\"'

def escape(s):
    s = list(s)
    for i in range(len(s)):
        s[i] = escapes[ord(s[i])]
    return string.join(s, '')


def safe_eval(s):
    # unwrap quotes, safely
    return eval(s, {'__builtins__':{}}, {})


def normalize(s):
    # This converts the various Python string types into a format that is
    # appropriate for .po files, namely much closer to C style.
    lines = string.split(s, '\n')
    if len(lines) == 1:
        s = '"' + escape(s) + '"'
    else:
        if not lines[-1]:
            del lines[-1]
            lines[-1] = lines[-1] + '\n'
        for i in range(len(lines)):
            lines[i] = escape(lines[i])
        s = '""\n"' + string.join(lines, '\\n"\n"') + '"'
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
                msg = string.join(self.__data, '')
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
            print pot_header % {'time': timestamp, 'version':__version__}
            for k, v in self.__messages.items():
                for filename, lineno in v:
                    # location comments are different b/w Solaris and GNU
                    d = {'filename': filename,
                         'lineno': lineno}
                    if options.location == options.SOLARIS:
                        print _('# File: %(filename)s, line: %(lineno)d') % d
                    elif options.location == options.GNU:
                        print _('#: %(filename)s:%(lineno)d') % d
                # TBD: sorting, normalizing
                print 'msgid', normalize(k)
                print 'msgstr ""'
                print
        finally:
            sys.stdout = sys.__stdout__


def main():
    default_keywords = ['_']
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            'k:d:n:hv',
            ['keyword', 'default-domain', 'help',
             'add-location=', 'no-location', 'verbose'])
    except getopt.error, msg:
        usage(1, msg)

    # for holding option values
    class Options:
        # constants
        GNU = 1
        SOLARIS = 2
        # defaults
        keywords = []
        outfile = 'messages.pot'
        location = GNU
        verbose = 0

    options = Options()
    locations = {'gnu' : options.GNU,
                 'solaris' : options.SOLARIS,
                 }

    # parse options
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt in ('-k', '--keyword'):
            if arg is None:
                default_keywords = []
            options.keywords.append(arg)
        elif opt in ('-d', '--default-domain'):
            options.outfile = arg + '.pot'
        elif opt in ('-n', '--add-location'):
            if arg is None:
                arg = 'gnu'
            try:
                options.location = locations[string.lower(arg)]
            except KeyError:
                d = {'arg':arg}
                usage(1, _('Invalid value for --add-location: %(arg)s') % d)
        elif opt in ('--no-location',):
            options.location = 0
        elif opt in ('-v', '--verbose'):
            options.verbose = 1

    # calculate all keywords
    options.keywords.extend(default_keywords)

    # slurp through all the files
    eater = TokenEater(options)
    for filename in args:
        if options.verbose:
            print _('Working on %(filename)s') % {'filename':filename}
        fp = open(filename)
        eater.set_filename(filename)
        tokenize.tokenize(fp.readline, eater)
        fp.close()

    fp = open(options.outfile, 'w')
    eater.write(fp)
    fp.close()



if __name__ == '__main__':
    main()
