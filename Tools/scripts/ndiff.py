#! /usr/bin/env python

# Module ndiff version 1.6.0
# Released to the public domain 08-Dec-2000,
# by Tim Peters (tim.one@home.com).

# Provided as-is; use at your own risk; no warranty; no promises; enjoy!

"""ndiff [-q] file1 file2
    or
ndiff (-r1 | -r2) < ndiff_output > file1_or_file2

Print a human-friendly file difference report to stdout.  Both inter-
and intra-line differences are noted.  In the second form, recreate file1
(-r1) or file2 (-r2) on stdout, from an ndiff report on stdin.

In the first form, if -q ("quiet") is not specified, the first two lines
of output are

-: file1
+: file2

Each remaining line begins with a two-letter code:

    "- "    line unique to file1
    "+ "    line unique to file2
    "  "    line common to both files
    "? "    line not present in either input file

Lines beginning with "? " attempt to guide the eye to intraline
differences, and were not present in either input file.  These lines can be
confusing if the source files contain tab characters.

The first file can be recovered by retaining only lines that begin with
"  " or "- ", and deleting those 2-character prefixes; use ndiff with -r1.

The second file can be recovered similarly, but by retaining only "  " and
"+ " lines; use ndiff with -r2; or, on Unix, the second file can be
recovered by piping the output through

    sed -n '/^[+ ] /s/^..//p'

See module comments for details and programmatic interface.
"""

__version__ = 1, 5, 0

# SequenceMatcher tries to compute a "human-friendly diff" between
# two sequences (chiefly picturing a file as a sequence of lines,
# and a line as a sequence of characters, here).  Unlike e.g. UNIX(tm)
# diff, the fundamental notion is the longest *contiguous* & junk-free
# matching subsequence.  That's what catches peoples' eyes.  The
# Windows(tm) windiff has another interesting notion, pairing up elements
# that appear uniquely in each sequence.  That, and the method here,
# appear to yield more intuitive difference reports than does diff.  This
# method appears to be the least vulnerable to synching up on blocks
# of "junk lines", though (like blank lines in ordinary text files,
# or maybe "<P>" lines in HTML files).  That may be because this is
# the only method of the 3 that has a *concept* of "junk" <wink>.
#
# Note that ndiff makes no claim to produce a *minimal* diff.  To the
# contrary, minimal diffs are often counter-intuitive, because they
# synch up anywhere possible, sometimes accidental matches 100 pages
# apart.  Restricting synch points to contiguous matches preserves some
# notion of locality, at the occasional cost of producing a longer diff.
#
# With respect to junk, an earlier version of ndiff simply refused to
# *start* a match with a junk element.  The result was cases like this:
#     before: private Thread currentThread;
#     after:  private volatile Thread currentThread;
# If you consider whitespace to be junk, the longest contiguous match
# not starting with junk is "e Thread currentThread".  So ndiff reported
# that "e volatil" was inserted between the 't' and the 'e' in "private".
# While an accurate view, to people that's absurd.  The current version
# looks for matching blocks that are entirely junk-free, then extends the
# longest one of those as far as possible but only with matching junk.
# So now "currentThread" is matched, then extended to suck up the
# preceding blank; then "private" is matched, and extended to suck up the
# following blank; then "Thread" is matched; and finally ndiff reports
# that "volatile " was inserted before "Thread".  The only quibble
# remaining is that perhaps it was really the case that " volatile"
# was inserted after "private".  I can live with that <wink>.
#
# NOTE on junk:  the module-level names
#    IS_LINE_JUNK
#    IS_CHARACTER_JUNK
# can be set to any functions you like.  The first one should accept
# a single string argument, and return true iff the string is junk.
# The default is whether the regexp r"\s*#?\s*$" matches (i.e., a
# line without visible characters, except for at most one splat).
# The second should accept a string of length 1 etc.  The default is
# whether the character is a blank or tab (note: bad idea to include
# newline in this!).
#
# After setting those, you can call fcompare(f1name, f2name) with the
# names of the files you want to compare.  The difference report
# is sent to stdout.  Or you can call main(args), passing what would
# have been in sys.argv[1:] had the cmd-line form been used.

from difflib import SequenceMatcher

import string
TRACE = 0

# define what "junk" means
import re

def IS_LINE_JUNK(line, pat=re.compile(r"\s*#?\s*$").match):
    return pat(line) is not None

def IS_CHARACTER_JUNK(ch, ws=" \t"):
    return ch in ws

del re

# meant for dumping lines
def dump(tag, x, lo, hi):
    for i in xrange(lo, hi):
        print tag, x[i],

def plain_replace(a, alo, ahi, b, blo, bhi):
    assert alo < ahi and blo < bhi
    # dump the shorter block first -- reduces the burden on short-term
    # memory if the blocks are of very different sizes
    if bhi - blo < ahi - alo:
        dump('+', b, blo, bhi)
        dump('-', a, alo, ahi)
    else:
        dump('-', a, alo, ahi)
        dump('+', b, blo, bhi)

# When replacing one block of lines with another, this guy searches
# the blocks for *similar* lines; the best-matching pair (if any) is
# used as a synch point, and intraline difference marking is done on
# the similar pair.  Lots of work, but often worth it.

def fancy_replace(a, alo, ahi, b, blo, bhi):
    if TRACE:
        print '*** fancy_replace', alo, ahi, blo, bhi
        dump('>', a, alo, ahi)
        dump('<', b, blo, bhi)

    # don't synch up unless the lines have a similarity score of at
    # least cutoff; best_ratio tracks the best score seen so far
    best_ratio, cutoff = 0.74, 0.75
    cruncher = SequenceMatcher(IS_CHARACTER_JUNK)
    eqi, eqj = None, None   # 1st indices of equal lines (if any)

    # search for the pair that matches best without being identical
    # (identical lines must be junk lines, & we don't want to synch up
    # on junk -- unless we have to)
    for j in xrange(blo, bhi):
        bj = b[j]
        cruncher.set_seq2(bj)
        for i in xrange(alo, ahi):
            ai = a[i]
            if ai == bj:
                if eqi is None:
                    eqi, eqj = i, j
                continue
            cruncher.set_seq1(ai)
            # computing similarity is expensive, so use the quick
            # upper bounds first -- have seen this speed up messy
            # compares by a factor of 3.
            # note that ratio() is only expensive to compute the first
            # time it's called on a sequence pair; the expensive part
            # of the computation is cached by cruncher
            if cruncher.real_quick_ratio() > best_ratio and \
                  cruncher.quick_ratio() > best_ratio and \
                  cruncher.ratio() > best_ratio:
                best_ratio, best_i, best_j = cruncher.ratio(), i, j
    if best_ratio < cutoff:
        # no non-identical "pretty close" pair
        if eqi is None:
            # no identical pair either -- treat it as a straight replace
            plain_replace(a, alo, ahi, b, blo, bhi)
            return
        # no close pair, but an identical pair -- synch up on that
        best_i, best_j, best_ratio = eqi, eqj, 1.0
    else:
        # there's a close pair, so forget the identical pair (if any)
        eqi = None

    # a[best_i] very similar to b[best_j]; eqi is None iff they're not
    # identical
    if TRACE:
        print '*** best_ratio', best_ratio, best_i, best_j
        dump('>', a, best_i, best_i+1)
        dump('<', b, best_j, best_j+1)

    # pump out diffs from before the synch point
    fancy_helper(a, alo, best_i, b, blo, best_j)

    # do intraline marking on the synch pair
    aelt, belt = a[best_i], b[best_j]
    if eqi is None:
        # pump out a '-', '?', '+', '?' quad for the synched lines
        atags = btags = ""
        cruncher.set_seqs(aelt, belt)
        for tag, ai1, ai2, bj1, bj2 in cruncher.get_opcodes():
            la, lb = ai2 - ai1, bj2 - bj1
            if tag == 'replace':
                atags += '^' * la
                btags += '^' * lb
            elif tag == 'delete':
                atags += '-' * la
            elif tag == 'insert':
                btags += '+' * lb
            elif tag == 'equal':
                atags += ' ' * la
                btags += ' ' * lb
            else:
                raise ValueError, 'unknown tag ' + `tag`
        printq(aelt, belt, atags, btags)
    else:
        # the synch pair is identical
        print ' ', aelt,

    # pump out diffs from after the synch point
    fancy_helper(a, best_i+1, ahi, b, best_j+1, bhi)

def fancy_helper(a, alo, ahi, b, blo, bhi):
    if alo < ahi:
        if blo < bhi:
            fancy_replace(a, alo, ahi, b, blo, bhi)
        else:
            dump('-', a, alo, ahi)
    elif blo < bhi:
        dump('+', b, blo, bhi)

# Crap to deal with leading tabs in "?" output.  Can hurt, but will
# probably help most of the time.

def printq(aline, bline, atags, btags):
    common = min(count_leading(aline, "\t"),
                 count_leading(bline, "\t"))
    common = min(common, count_leading(atags[:common], " "))
    print "-", aline,
    if count_leading(atags, " ") < len(atags):
        print "?", "\t" * common + atags[common:]
    print "+", bline,
    if count_leading(btags, " ") < len(btags):
        print "?", "\t" * common + btags[common:]

def count_leading(line, ch):
    i, n = 0, len(line)
    while i < n and line[i] == ch:
        i += 1
    return i

def fail(msg):
    import sys
    out = sys.stderr.write
    out(msg + "\n\n")
    out(__doc__)
    return 0

# open a file & return the file object; gripe and return 0 if it
# couldn't be opened
def fopen(fname):
    try:
        return open(fname, 'r')
    except IOError, detail:
        return fail("couldn't open " + fname + ": " + str(detail))

# open two files & spray the diff to stdout; return false iff a problem
def fcompare(f1name, f2name):
    f1 = fopen(f1name)
    f2 = fopen(f2name)
    if not f1 or not f2:
        return 0

    a = f1.readlines(); f1.close()
    b = f2.readlines(); f2.close()

    cruncher = SequenceMatcher(IS_LINE_JUNK, a, b)
    for tag, alo, ahi, blo, bhi in cruncher.get_opcodes():
        if tag == 'replace':
            fancy_replace(a, alo, ahi, b, blo, bhi)
        elif tag == 'delete':
            dump('-', a, alo, ahi)
        elif tag == 'insert':
            dump('+', b, blo, bhi)
        elif tag == 'equal':
            dump(' ', a, alo, ahi)
        else:
            raise ValueError, 'unknown tag ' + `tag`

    return 1

# crack args (sys.argv[1:] is normal) & compare;
# return false iff a problem

def main(args):
    import getopt
    try:
        opts, args = getopt.getopt(args, "qr:")
    except getopt.error, detail:
        return fail(str(detail))
    noisy = 1
    qseen = rseen = 0
    for opt, val in opts:
        if opt == "-q":
            qseen = 1
            noisy = 0
        elif opt == "-r":
            rseen = 1
            whichfile = val
    if qseen and rseen:
        return fail("can't specify both -q and -r")
    if rseen:
        if args:
            return fail("no args allowed with -r option")
        if whichfile in "12":
            restore(whichfile)
            return 1
        return fail("-r value must be 1 or 2")
    if len(args) != 2:
        return fail("need 2 filename args")
    f1name, f2name = args
    if noisy:
        print '-:', f1name
        print '+:', f2name
    return fcompare(f1name, f2name)

def restore(which):
    import sys
    tag = {"1": "- ", "2": "+ "}[which]
    prefixes = ("  ", tag)
    for line in sys.stdin.readlines():
        if line[:2] in prefixes:
            print line[2:],

if __name__ == '__main__':
    import sys
    args = sys.argv[1:]
    if "-profile" in args:
        import profile, pstats
        args.remove("-profile")
        statf = "ndiff.pro"
        profile.run("main(args)", statf)
        stats = pstats.Stats(statf)
        stats.strip_dirs().sort_stats('time').print_stats()
    else:
        main(args)
