""" TeXcheck.py -- rough syntax checking on Python style LaTeX documents.

   Written by Raymond D. Hettinger <python at rcn.com>
   Copyright (c) 2003 Python Software Foundation.  All rights reserved.

Designed to catch common markup errors including:
* Unbalanced or mismatched parenthesis, brackets, and braces.
* Unbalanced or mismatched \\begin and \\end blocks.
* Misspelled or invalid LaTeX commands.
* Use of forward slashes instead of backslashes for commands.
* Table line size mismatches.

Sample command line usage:
    python texcheck.py -k chapterheading -m lib/librandomtex *.tex

Options:
    -m          Munge parenthesis and brackets. [0,n) would normally mismatch.
    -k keyword: Keyword is a valid LaTeX command. Do not include the backslash.
    -d:         Delimiter check only (useful for non-LaTeX files).
    -h:         Help
    -s lineno:  Start at lineno (useful for skipping complex sections).
    -v:         Verbose.  Trace the matching of //begin and //end blocks.
"""

import re
import sys
import getopt
from itertools import izip, count, islice
import glob

cmdstr = r"""
    \section \module \declaremodule \modulesynopsis \moduleauthor
    \sectionauthor \versionadded \code \class \method \begin
    \optional \var \ref \end \subsection \lineiii \hline \label
    \indexii \textrm \ldots \keyword \stindex \index \item \note
    \withsubitem \ttindex \footnote \citetitle \samp \opindex
    \noindent \exception \strong \dfn \ctype \obindex \character
    \indexiii \function \bifuncindex \refmodule \refbimodindex
    \subsubsection \nodename \member \chapter \emph \ASCII \UNIX
    \regexp \program \production \token \productioncont \term
    \grammartoken \lineii \seemodule \file \EOF \documentclass
    \usepackage \title \input \maketitle \ifhtml \fi \url \Cpp
    \tableofcontents \kbd \programopt \envvar \refstmodindex
    \cfunction \constant \NULL \moreargs \cfuncline \cdata
    \textasciicircum \n \ABC \setindexsubitem \versionchanged
    \deprecated \seetext \newcommand \POSIX \pep \warning \rfc
    \verbatiminput \methodline \textgreater \seetitle \lineiv
    \funclineni \ulink \manpage \funcline \dataline \unspecified
    \textbackslash \mimetype \mailheader \seepep \textunderscore
    \longprogramopt \infinity \plusminus \shortversion \version
    \refmodindex \seerfc \makeindex \makemodindex \renewcommand
    \indexname \appendix \protect \indexiv \mbox \textasciitilde
    \platform \seeurl \leftmargin \labelwidth \localmoduletable
    \LaTeX \copyright \memberline \backslash \pi \centerline
    \caption \vspace \textwidth \menuselection \textless
    \makevar \csimplemacro \menuselection \bfcode \sub \release
    \email \kwindex \refexmodindex \filenq \e \menuselection
    \exindex \linev \newsgroup \verbatim \setshortversion
    \author \authoraddress \paragraph \subparagraph \cmemberline
    \textbar \C \seelink
"""

def matchclose(c_lineno, c_symbol, openers, pairmap):
    "Verify that closing delimiter matches most recent opening delimiter"
    try:
        o_lineno, o_symbol = openers.pop()
    except IndexError:
        print "\nDelimiter mismatch.  On line %d, encountered closing '%s' without corresponding open" % (c_lineno, c_symbol)
        return
    if o_symbol in pairmap.get(c_symbol, [c_symbol]): return
    print "\nOpener '%s' on line %d was not closed before encountering '%s' on line %d" % (o_symbol, o_lineno, c_symbol, c_lineno)
    return

def checkit(source, opts, morecmds=[]):
    """Check the LaTeX formatting in a sequence of lines.

    Opts is a mapping of options to option values if any:
        -m          munge parenthesis and brackets
        -d          delimiters only checking
        -v          verbose trace of delimiter matching
        -s lineno:  linenumber to start scan (default is 1).

    Morecmds is a sequence of LaTeX commands (without backslashes) that
    are to be considered valid in the scan.
    """

    texcmd = re.compile(r'\\[A-Za-z]+')
    falsetexcmd = re.compile(r'\/([A-Za-z]+)') # Mismarked with forward slash

    validcmds = set(cmdstr.split())
    for cmd in morecmds:
        validcmds.add('\\' + cmd)

    if '-m' in opts:
        pairmap = {']':'[(', ')':'(['}      # Munged openers
    else:
        pairmap = {']':'[', ')':'('}        # Normal opener for a given closer
    openpunct = set('([')                   # Set of valid openers

    delimiters = re.compile(r'\\(begin|end){([_a-zA-Z]+)}|([()\[\]])')
    braces = re.compile(r'({)|(})')
    doubledwords = re.compile(r'(\b[A-za-z]+\b) \b\1\b')
    spacingmarkup = re.compile(r'\\(ABC|ASCII|C|Cpp|EOF|infinity|NULL|plusminus|POSIX|UNIX)\s')

    openers = []                            # Stack of pending open delimiters
    bracestack = []                         # Stack of pending open braces

    tablestart = re.compile(r'\\begin{(?:long)?table([iv]+)}')
    tableline = re.compile(r'\\line([iv]+){')
    tableend = re.compile(r'\\end{(?:long)?table([iv]+)}')
    tablelevel = ''
    tablestartline = 0

    startline = int(opts.get('-s', '1'))
    lineno = 0

    for lineno, line in izip(count(startline), islice(source, startline-1, None)):
        line = line.rstrip()

        # Check balancing of open/close parenthesis, brackets, and begin/end blocks
        for begend, name, punct in delimiters.findall(line):
            if '-v' in opts:
                print lineno, '|', begend, name, punct,
            if begend == 'begin' and '-d' not in opts:
                openers.append((lineno, name))
            elif punct in openpunct:
                openers.append((lineno, punct))
            elif begend == 'end' and '-d' not in opts:
                matchclose(lineno, name, openers, pairmap)
            elif punct in pairmap:
                matchclose(lineno, punct, openers, pairmap)
            if '-v' in opts:
                print '   --> ', openers

        # Balance opening and closing braces
        for open, close in braces.findall(line):
            if open == '{':
                bracestack.append(lineno)
            if close == '}':
                try:
                    bracestack.pop()
                except IndexError:
                    print r'Warning, unmatched } on line %s.' % (lineno,)

        # Optionally, skip LaTeX specific checks
        if '-d' in opts:
            continue

        # Warn whenever forward slashes encountered with a LaTeX command
        for cmd in falsetexcmd.findall(line):
            if '822' in line or '.html' in line:
                continue    # Ignore false positives for urls and for /rfc822
            if '\\' + cmd in validcmds:
                print 'Warning, forward slash used on line %d with cmd: /%s' % (lineno, cmd)

        # Check for markup requiring {} for correct spacing
        for cmd in spacingmarkup.findall(line):
            print r'Warning, \%s should be written as \%s{} on line %d' % (cmd, cmd, lineno)

        # Validate commands
        nc = line.find(r'\newcommand')
        if nc != -1:
            start = line.find('{', nc)
            end = line.find('}', start)
            validcmds.add(line[start+1:end])
        for cmd in texcmd.findall(line):
            if cmd not in validcmds:
                print r'Warning, unknown tex cmd on line %d: \%s' % (lineno, cmd)

        # Check table levels (make sure lineii only inside tableii)
        m = tablestart.search(line)
        if m:
            tablelevel = m.group(1)
            tablestartline = lineno
        m = tableline.search(line)
        if m and m.group(1) != tablelevel:
            print r'Warning, \line%s on line %d does not match \table%s on line %d' % (m.group(1), lineno, tablelevel, tablestartline)
        if tableend.search(line):
            tablelevel = ''

        # Style guide warnings
        if 'e.g.' in line or 'i.e.' in line:
            print r'Style warning, avoid use of i.e or e.g. on line %d' % (lineno,)

        for dw in doubledwords.findall(line):
            print r'Doubled word warning.  "%s" on line %d' % (dw, lineno)

    lastline = lineno
    for lineno, symbol in openers:
        print "Unmatched open delimiter '%s' on line %d" % (symbol, lineno)
    for lineno in bracestack:
        print "Unmatched { on line %d" % (lineno,)
    print 'Done checking %d lines.' % (lastline,)
    return 0

def main(args=None):
    if args is None:
        args = sys.argv[1:]
    optitems, arglist = getopt.getopt(args, "k:mdhs:v")
    opts = dict(optitems)
    if '-h' in opts or args==[]:
        print __doc__
        return 0

    if len(arglist) < 1:
        print 'Please specify a file to be checked'
        return 1

    for i, filespec in enumerate(arglist):
        if '*' in filespec or '?' in filespec:
            arglist[i:i+1] = glob.glob(filespec)

    morecmds = [v for k,v in optitems if k=='-k']
    err = []

    for filename in arglist:
        print '=' * 30
        print "Checking", filename
        try:
            f = open(filename)
        except IOError:
            print 'Cannot open file %s.' % arglist[0]
            return 2

        try:
            err.append(checkit(f, opts, morecmds))
        finally:
            f.close()

    return max(err)

if __name__ == '__main__':
    sys.exit(main())
