#! /usr/bin/env python

"""
Module difflib -- helpers for computing deltas between objects.

Function get_close_matches(word, possibilities, n=3, cutoff=0.6):

    Use SequenceMatcher to return list of the best "good enough" matches.

    word is a sequence for which close matches are desired (typically a
    string).

    possibilities is a list of sequences against which to match word
    (typically a list of strings).

    Optional arg n (default 3) is the maximum number of close matches to
    return.  n must be > 0.

    Optional arg cutoff (default 0.6) is a float in [0, 1].  Possibilities
    that don't score at least that similar to word are ignored.

    The best (no more than n) matches among the possibilities are returned
    in a list, sorted by similarity score, most similar first.

    >>> get_close_matches("appel", ["ape", "apple", "peach", "puppy"])
    ['apple', 'ape']
    >>> import keyword
    >>> get_close_matches("wheel", keyword.kwlist)
    ['while']
    >>> get_close_matches("apple", keyword.kwlist)
    []
    >>> get_close_matches("accept", keyword.kwlist)
    ['except']

Class SequenceMatcher

SequenceMatcher is a flexible class for comparing pairs of sequences of any
type, so long as the sequence elements are hashable.  The basic algorithm
predates, and is a little fancier than, an algorithm published in the late
1980's by Ratcliff and Obershelp under the hyperbolic name "gestalt pattern
matching".  The basic idea is to find the longest contiguous matching
subsequence that contains no "junk" elements (R-O doesn't address junk).
The same idea is then applied recursively to the pieces of the sequences to
the left and to the right of the matching subsequence.  This does not yield
minimal edit sequences, but does tend to yield matches that "look right"
to people.

Example, comparing two strings, and considering blanks to be "junk":

>>> s = SequenceMatcher(lambda x: x == " ",
...                     "private Thread currentThread;",
...                     "private volatile Thread currentThread;")
>>>

.ratio() returns a float in [0, 1], measuring the "similarity" of the
sequences.  As a rule of thumb, a .ratio() value over 0.6 means the
sequences are close matches:

>>> print round(s.ratio(), 3)
0.866
>>>

If you're only interested in where the sequences match,
.get_matching_blocks() is handy:

>>> for block in s.get_matching_blocks():
...     print "a[%d] and b[%d] match for %d elements" % block
a[0] and b[0] match for 8 elements
a[8] and b[17] match for 6 elements
a[14] and b[23] match for 15 elements
a[29] and b[38] match for 0 elements

Note that the last tuple returned by .get_matching_blocks() is always a
dummy, (len(a), len(b), 0), and this is the only case in which the last
tuple element (number of elements matched) is 0.

If you want to know how to change the first sequence into the second, use
.get_opcodes():

>>> for opcode in s.get_opcodes():
...     print "%6s a[%d:%d] b[%d:%d]" % opcode
 equal a[0:8] b[0:8]
insert a[8:8] b[8:17]
 equal a[8:14] b[17:23]
 equal a[14:29] b[23:38]

See Tools/scripts/ndiff.py for a fancy human-friendly file differencer,
which uses SequenceMatcher both to view files as sequences of lines, and
lines as sequences of characters.

See also function get_close_matches() in this module, which shows how
simple code building on SequenceMatcher can be used to do useful work.

Timing:  Basic R-O is cubic time worst case and quadratic time expected
case.  SequenceMatcher is quadratic time for the worst case and has
expected-case behavior dependent in a complicated way on how many
elements the sequences have in common; best case time is linear.

SequenceMatcher methods:

__init__(isjunk=None, a='', b='')
    Construct a SequenceMatcher.

    Optional arg isjunk is None (the default), or a one-argument function
    that takes a sequence element and returns true iff the element is junk.
    None is equivalent to passing "lambda x: 0", i.e. no elements are
    considered to be junk.  For example, pass
        lambda x: x in " \\t"
    if you're comparing lines as sequences of characters, and don't want to
    synch up on blanks or hard tabs.

    Optional arg a is the first of two sequences to be compared.  By
    default, an empty string.  The elements of a must be hashable.

    Optional arg b is the second of two sequences to be compared.  By
    default, an empty string.  The elements of b must be hashable.

set_seqs(a, b)
    Set the two sequences to be compared.

    >>> s = SequenceMatcher()
    >>> s.set_seqs("abcd", "bcde")
    >>> s.ratio()
    0.75

set_seq1(a)
    Set the first sequence to be compared.

    The second sequence to be compared is not changed.

    >>> s = SequenceMatcher(None, "abcd", "bcde")
    >>> s.ratio()
    0.75
    >>> s.set_seq1("bcde")
    >>> s.ratio()
    1.0
    >>>

    SequenceMatcher computes and caches detailed information about the
    second sequence, so if you want to compare one sequence S against many
    sequences, use .set_seq2(S) once and call .set_seq1(x) repeatedly for
    each of the other sequences.

    See also set_seqs() and set_seq2().

set_seq2(b)
    Set the second sequence to be compared.

    The first sequence to be compared is not changed.

    >>> s = SequenceMatcher(None, "abcd", "bcde")
    >>> s.ratio()
    0.75
    >>> s.set_seq2("abcd")
    >>> s.ratio()
    1.0
    >>>

    SequenceMatcher computes and caches detailed information about the
    second sequence, so if you want to compare one sequence S against many
    sequences, use .set_seq2(S) once and call .set_seq1(x) repeatedly for
    each of the other sequences.

    See also set_seqs() and set_seq1().

find_longest_match(alo, ahi, blo, bhi)
    Find longest matching block in a[alo:ahi] and b[blo:bhi].

    If isjunk is not defined:

    Return (i,j,k) such that a[i:i+k] is equal to b[j:j+k], where
        alo <= i <= i+k <= ahi
        blo <= j <= j+k <= bhi
    and for all (i',j',k') meeting those conditions,
        k >= k'
        i <= i'
        and if i == i', j <= j'

    In other words, of all maximal matching blocks, return one that starts
    earliest in a, and of all those maximal matching blocks that start
    earliest in a, return the one that starts earliest in b.

    >>> s = SequenceMatcher(None, " abcd", "abcd abcd")
    >>> s.find_longest_match(0, 5, 0, 9)
    (0, 4, 5)

    If isjunk is defined, first the longest matching block is determined as
    above, but with the additional restriction that no junk element appears
    in the block.  Then that block is extended as far as possible by
    matching (only) junk elements on both sides.  So the resulting block
    never matches on junk except as identical junk happens to be adjacent
    to an "interesting" match.

    Here's the same example as before, but considering blanks to be junk.
    That prevents " abcd" from matching the " abcd" at the tail end of the
    second sequence directly.  Instead only the "abcd" can match, and
    matches the leftmost "abcd" in the second sequence:

    >>> s = SequenceMatcher(lambda x: x==" ", " abcd", "abcd abcd")
    >>> s.find_longest_match(0, 5, 0, 9)
    (1, 0, 4)

    If no blocks match, return (alo, blo, 0).

    >>> s = SequenceMatcher(None, "ab", "c")
    >>> s.find_longest_match(0, 2, 0, 1)
    (0, 0, 0)

get_matching_blocks()
    Return list of triples describing matching subsequences.

    Each triple is of the form (i, j, n), and means that
    a[i:i+n] == b[j:j+n].  The triples are monotonically increasing in i
    and in j.

    The last triple is a dummy, (len(a), len(b), 0), and is the only triple
    with n==0.

    >>> s = SequenceMatcher(None, "abxcd", "abcd")
    >>> s.get_matching_blocks()
    [(0, 0, 2), (3, 2, 2), (5, 4, 0)]

get_opcodes()
    Return list of 5-tuples describing how to turn a into b.

    Each tuple is of the form (tag, i1, i2, j1, j2).  The first tuple has
    i1 == j1 == 0, and remaining tuples have i1 == the i2 from the tuple
    preceding it, and likewise for j1 == the previous j2.

    The tags are strings, with these meanings:

    'replace':  a[i1:i2] should be replaced by b[j1:j2]
    'delete':   a[i1:i2] should be deleted.
                Note that j1==j2 in this case.
    'insert':   b[j1:j2] should be inserted at a[i1:i1].
                Note that i1==i2 in this case.
    'equal':    a[i1:i2] == b[j1:j2]

    >>> a = "qabxcd"
    >>> b = "abycdf"
    >>> s = SequenceMatcher(None, a, b)
    >>> for tag, i1, i2, j1, j2 in s.get_opcodes():
    ...    print ("%7s a[%d:%d] (%s) b[%d:%d] (%s)" %
    ...           (tag, i1, i2, a[i1:i2], j1, j2, b[j1:j2]))
     delete a[0:1] (q) b[0:0] ()
      equal a[1:3] (ab) b[0:2] (ab)
    replace a[3:4] (x) b[2:3] (y)
      equal a[4:6] (cd) b[3:5] (cd)
     insert a[6:6] () b[5:6] (f)

ratio()
    Return a measure of the sequences' similarity (float in [0,1]).

    Where T is the total number of elements in both sequences, and M is the
    number of matches, this is 2,0*M / T. Note that this is 1 if the
    sequences are identical, and 0 if they have nothing in common.

    .ratio() is expensive to compute if you haven't already computed
    .get_matching_blocks() or .get_opcodes(), in which case you may want to
    try .quick_ratio() or .real_quick_ratio() first to get an upper bound.

    >>> s = SequenceMatcher(None, "abcd", "bcde")
    >>> s.ratio()
    0.75
    >>> s.quick_ratio()
    0.75
    >>> s.real_quick_ratio()
    1.0

quick_ratio()
    Return an upper bound on .ratio() relatively quickly.

    This isn't defined beyond that it is an upper bound on .ratio(), and
    is faster to compute.

real_quick_ratio():
    Return an upper bound on ratio() very quickly.

    This isn't defined beyond that it is an upper bound on .ratio(), and
    is faster to compute than either .ratio() or .quick_ratio().
"""

TRACE = 0

class SequenceMatcher:
    def __init__(self, isjunk=None, a='', b=''):
        """Construct a SequenceMatcher.

        Optional arg isjunk is None (the default), or a one-argument
        function that takes a sequence element and returns true iff the
        element is junk. None is equivalent to passing "lambda x: 0", i.e.
        no elements are considered to be junk.  For example, pass
            lambda x: x in " \\t"
        if you're comparing lines as sequences of characters, and don't
        want to synch up on blanks or hard tabs.

        Optional arg a is the first of two sequences to be compared.  By
        default, an empty string.  The elements of a must be hashable.  See
        also .set_seqs() and .set_seq1().

        Optional arg b is the second of two sequences to be compared.  By
        default, an empty string.  The elements of b must be hashable. See
        also .set_seqs() and .set_seq2().
        """

        # Members:
        # a
        #      first sequence
        # b
        #      second sequence; differences are computed as "what do
        #      we need to do to 'a' to change it into 'b'?"
        # b2j
        #      for x in b, b2j[x] is a list of the indices (into b)
        #      at which x appears; junk elements do not appear
        # b2jhas
        #      b2j.has_key
        # fullbcount
        #      for x in b, fullbcount[x] == the number of times x
        #      appears in b; only materialized if really needed (used
        #      only for computing quick_ratio())
        # matching_blocks
        #      a list of (i, j, k) triples, where a[i:i+k] == b[j:j+k];
        #      ascending & non-overlapping in i and in j; terminated by
        #      a dummy (len(a), len(b), 0) sentinel
        # opcodes
        #      a list of (tag, i1, i2, j1, j2) tuples, where tag is
        #      one of
        #          'replace'   a[i1:i2] should be replaced by b[j1:j2]
        #          'delete'    a[i1:i2] should be deleted
        #          'insert'    b[j1:j2] should be inserted
        #          'equal'     a[i1:i2] == b[j1:j2]
        # isjunk
        #      a user-supplied function taking a sequence element and
        #      returning true iff the element is "junk" -- this has
        #      subtle but helpful effects on the algorithm, which I'll
        #      get around to writing up someday <0.9 wink>.
        #      DON'T USE!  Only __chain_b uses this.  Use isbjunk.
        # isbjunk
        #      for x in b, isbjunk(x) == isjunk(x) but much faster;
        #      it's really the has_key method of a hidden dict.
        #      DOES NOT WORK for x in a!

        self.isjunk = isjunk
        self.a = self.b = None
        self.set_seqs(a, b)

    def set_seqs(self, a, b):
        """Set the two sequences to be compared.

        >>> s = SequenceMatcher()
        >>> s.set_seqs("abcd", "bcde")
        >>> s.ratio()
        0.75
        """

        self.set_seq1(a)
        self.set_seq2(b)

    def set_seq1(self, a):
        """Set the first sequence to be compared.

        The second sequence to be compared is not changed.

        >>> s = SequenceMatcher(None, "abcd", "bcde")
        >>> s.ratio()
        0.75
        >>> s.set_seq1("bcde")
        >>> s.ratio()
        1.0
        >>>

        SequenceMatcher computes and caches detailed information about the
        second sequence, so if you want to compare one sequence S against
        many sequences, use .set_seq2(S) once and call .set_seq1(x)
        repeatedly for each of the other sequences.

        See also set_seqs() and set_seq2().
        """

        if a is self.a:
            return
        self.a = a
        self.matching_blocks = self.opcodes = None

    def set_seq2(self, b):
        """Set the second sequence to be compared.

        The first sequence to be compared is not changed.

        >>> s = SequenceMatcher(None, "abcd", "bcde")
        >>> s.ratio()
        0.75
        >>> s.set_seq2("abcd")
        >>> s.ratio()
        1.0
        >>>

        SequenceMatcher computes and caches detailed information about the
        second sequence, so if you want to compare one sequence S against
        many sequences, use .set_seq2(S) once and call .set_seq1(x)
        repeatedly for each of the other sequences.

        See also set_seqs() and set_seq1().
        """

        if b is self.b:
            return
        self.b = b
        self.matching_blocks = self.opcodes = None
        self.fullbcount = None
        self.__chain_b()

    # For each element x in b, set b2j[x] to a list of the indices in
    # b where x appears; the indices are in increasing order; note that
    # the number of times x appears in b is len(b2j[x]) ...
    # when self.isjunk is defined, junk elements don't show up in this
    # map at all, which stops the central find_longest_match method
    # from starting any matching block at a junk element ...
    # also creates the fast isbjunk function ...
    # note that this is only called when b changes; so for cross-product
    # kinds of matches, it's best to call set_seq2 once, then set_seq1
    # repeatedly

    def __chain_b(self):
        # Because isjunk is a user-defined (not C) function, and we test
        # for junk a LOT, it's important to minimize the number of calls.
        # Before the tricks described here, __chain_b was by far the most
        # time-consuming routine in the whole module!  If anyone sees
        # Jim Roskind, thank him again for profile.py -- I never would
        # have guessed that.
        # The first trick is to build b2j ignoring the possibility
        # of junk.  I.e., we don't call isjunk at all yet.  Throwing
        # out the junk later is much cheaper than building b2j "right"
        # from the start.
        b = self.b
        self.b2j = b2j = {}
        self.b2jhas = b2jhas = b2j.has_key
        for i in xrange(len(b)):
            elt = b[i]
            if b2jhas(elt):
                b2j[elt].append(i)
            else:
                b2j[elt] = [i]

        # Now b2j.keys() contains elements uniquely, and especially when
        # the sequence is a string, that's usually a good deal smaller
        # than len(string).  The difference is the number of isjunk calls
        # saved.
        isjunk, junkdict = self.isjunk, {}
        if isjunk:
            for elt in b2j.keys():
                if isjunk(elt):
                    junkdict[elt] = 1   # value irrelevant; it's a set
                    del b2j[elt]

        # Now for x in b, isjunk(x) == junkdict.has_key(x), but the
        # latter is much faster.  Note too that while there may be a
        # lot of junk in the sequence, the number of *unique* junk
        # elements is probably small.  So the memory burden of keeping
        # this dict alive is likely trivial compared to the size of b2j.
        self.isbjunk = junkdict.has_key

    def find_longest_match(self, alo, ahi, blo, bhi):
        """Find longest matching block in a[alo:ahi] and b[blo:bhi].

        If isjunk is not defined:

        Return (i,j,k) such that a[i:i+k] is equal to b[j:j+k], where
            alo <= i <= i+k <= ahi
            blo <= j <= j+k <= bhi
        and for all (i',j',k') meeting those conditions,
            k >= k'
            i <= i'
            and if i == i', j <= j'

        In other words, of all maximal matching blocks, return one that
        starts earliest in a, and of all those maximal matching blocks that
        start earliest in a, return the one that starts earliest in b.

        >>> s = SequenceMatcher(None, " abcd", "abcd abcd")
        >>> s.find_longest_match(0, 5, 0, 9)
        (0, 4, 5)

        If isjunk is defined, first the longest matching block is
        determined as above, but with the additional restriction that no
        junk element appears in the block.  Then that block is extended as
        far as possible by matching (only) junk elements on both sides.  So
        the resulting block never matches on junk except as identical junk
        happens to be adjacent to an "interesting" match.

        Here's the same example as before, but considering blanks to be
        junk.  That prevents " abcd" from matching the " abcd" at the tail
        end of the second sequence directly.  Instead only the "abcd" can
        match, and matches the leftmost "abcd" in the second sequence:

        >>> s = SequenceMatcher(lambda x: x==" ", " abcd", "abcd abcd")
        >>> s.find_longest_match(0, 5, 0, 9)
        (1, 0, 4)

        If no blocks match, return (alo, blo, 0).

        >>> s = SequenceMatcher(None, "ab", "c")
        >>> s.find_longest_match(0, 2, 0, 1)
        (0, 0, 0)
        """

        # CAUTION:  stripping common prefix or suffix would be incorrect.
        # E.g.,
        #    ab
        #    acab
        # Longest matching block is "ab", but if common prefix is
        # stripped, it's "a" (tied with "b").  UNIX(tm) diff does so
        # strip, so ends up claiming that ab is changed to acab by
        # inserting "ca" in the middle.  That's minimal but unintuitive:
        # "it's obvious" that someone inserted "ac" at the front.
        # Windiff ends up at the same place as diff, but by pairing up
        # the unique 'b's and then matching the first two 'a's.

        a, b, b2j, isbjunk = self.a, self.b, self.b2j, self.isbjunk
        besti, bestj, bestsize = alo, blo, 0
        # find longest junk-free match
        # during an iteration of the loop, j2len[j] = length of longest
        # junk-free match ending with a[i-1] and b[j]
        j2len = {}
        nothing = []
        for i in xrange(alo, ahi):
            # look at all instances of a[i] in b; note that because
            # b2j has no junk keys, the loop is skipped if a[i] is junk
            j2lenget = j2len.get
            newj2len = {}
            for j in b2j.get(a[i], nothing):
                # a[i] matches b[j]
                if j < blo:
                    continue
                if j >= bhi:
                    break
                k = newj2len[j] = j2lenget(j-1, 0) + 1
                if k > bestsize:
                    besti, bestj, bestsize = i-k+1, j-k+1, k
            j2len = newj2len

        # Now that we have a wholly interesting match (albeit possibly
        # empty!), we may as well suck up the matching junk on each
        # side of it too.  Can't think of a good reason not to, and it
        # saves post-processing the (possibly considerable) expense of
        # figuring out what to do with it.  In the case of an empty
        # interesting match, this is clearly the right thing to do,
        # because no other kind of match is possible in the regions.
        while besti > alo and bestj > blo and \
              isbjunk(b[bestj-1]) and \
              a[besti-1] == b[bestj-1]:
            besti, bestj, bestsize = besti-1, bestj-1, bestsize+1
        while besti+bestsize < ahi and bestj+bestsize < bhi and \
              isbjunk(b[bestj+bestsize]) and \
              a[besti+bestsize] == b[bestj+bestsize]:
            bestsize = bestsize + 1

        if TRACE:
            print "get_matching_blocks", alo, ahi, blo, bhi
            print "    returns", besti, bestj, bestsize
        return besti, bestj, bestsize

    def get_matching_blocks(self):
        """Return list of triples describing matching subsequences.

        Each triple is of the form (i, j, n), and means that
        a[i:i+n] == b[j:j+n].  The triples are monotonically increasing in
        i and in j.

        The last triple is a dummy, (len(a), len(b), 0), and is the only
        triple with n==0.

        >>> s = SequenceMatcher(None, "abxcd", "abcd")
        >>> s.get_matching_blocks()
        [(0, 0, 2), (3, 2, 2), (5, 4, 0)]
        """

        if self.matching_blocks is not None:
            return self.matching_blocks
        self.matching_blocks = []
        la, lb = len(self.a), len(self.b)
        self.__helper(0, la, 0, lb, self.matching_blocks)
        self.matching_blocks.append( (la, lb, 0) )
        if TRACE:
            print '*** matching blocks', self.matching_blocks
        return self.matching_blocks

    # builds list of matching blocks covering a[alo:ahi] and
    # b[blo:bhi], appending them in increasing order to answer

    def __helper(self, alo, ahi, blo, bhi, answer):
        i, j, k = x = self.find_longest_match(alo, ahi, blo, bhi)
        # a[alo:i] vs b[blo:j] unknown
        # a[i:i+k] same as b[j:j+k]
        # a[i+k:ahi] vs b[j+k:bhi] unknown
        if k:
            if alo < i and blo < j:
                self.__helper(alo, i, blo, j, answer)
            answer.append(x)
            if i+k < ahi and j+k < bhi:
                self.__helper(i+k, ahi, j+k, bhi, answer)

    def get_opcodes(self):
        """Return list of 5-tuples describing how to turn a into b.

        Each tuple is of the form (tag, i1, i2, j1, j2).  The first tuple
        has i1 == j1 == 0, and remaining tuples have i1 == the i2 from the
        tuple preceding it, and likewise for j1 == the previous j2.

        The tags are strings, with these meanings:

        'replace':  a[i1:i2] should be replaced by b[j1:j2]
        'delete':   a[i1:i2] should be deleted.
                    Note that j1==j2 in this case.
        'insert':   b[j1:j2] should be inserted at a[i1:i1].
                    Note that i1==i2 in this case.
        'equal':    a[i1:i2] == b[j1:j2]

        >>> a = "qabxcd"
        >>> b = "abycdf"
        >>> s = SequenceMatcher(None, a, b)
        >>> for tag, i1, i2, j1, j2 in s.get_opcodes():
        ...    print ("%7s a[%d:%d] (%s) b[%d:%d] (%s)" %
        ...           (tag, i1, i2, a[i1:i2], j1, j2, b[j1:j2]))
         delete a[0:1] (q) b[0:0] ()
          equal a[1:3] (ab) b[0:2] (ab)
        replace a[3:4] (x) b[2:3] (y)
          equal a[4:6] (cd) b[3:5] (cd)
         insert a[6:6] () b[5:6] (f)
        """

        if self.opcodes is not None:
            return self.opcodes
        i = j = 0
        self.opcodes = answer = []
        for ai, bj, size in self.get_matching_blocks():
            # invariant:  we've pumped out correct diffs to change
            # a[:i] into b[:j], and the next matching block is
            # a[ai:ai+size] == b[bj:bj+size].  So we need to pump
            # out a diff to change a[i:ai] into b[j:bj], pump out
            # the matching block, and move (i,j) beyond the match
            tag = ''
            if i < ai and j < bj:
                tag = 'replace'
            elif i < ai:
                tag = 'delete'
            elif j < bj:
                tag = 'insert'
            if tag:
                answer.append( (tag, i, ai, j, bj) )
            i, j = ai+size, bj+size
            # the list of matching blocks is terminated by a
            # sentinel with size 0
            if size:
                answer.append( ('equal', ai, i, bj, j) )
        return answer

    def ratio(self):
        """Return a measure of the sequences' similarity (float in [0,1]).

        Where T is the total number of elements in both sequences, and
        M is the number of matches, this is 2,0*M / T.
        Note that this is 1 if the sequences are identical, and 0 if
        they have nothing in common.

        .ratio() is expensive to compute if you haven't already computed
        .get_matching_blocks() or .get_opcodes(), in which case you may
        want to try .quick_ratio() or .real_quick_ratio() first to get an
        upper bound.

        >>> s = SequenceMatcher(None, "abcd", "bcde")
        >>> s.ratio()
        0.75
        >>> s.quick_ratio()
        0.75
        >>> s.real_quick_ratio()
        1.0
        """

        matches = reduce(lambda sum, triple: sum + triple[-1],
                         self.get_matching_blocks(), 0)
        return 2.0 * matches / (len(self.a) + len(self.b))

    def quick_ratio(self):
        """Return an upper bound on ratio() relatively quickly.

        This isn't defined beyond that it is an upper bound on .ratio(), and
        is faster to compute.
        """

        # viewing a and b as multisets, set matches to the cardinality
        # of their intersection; this counts the number of matches
        # without regard to order, so is clearly an upper bound
        if self.fullbcount is None:
            self.fullbcount = fullbcount = {}
            for elt in self.b:
                fullbcount[elt] = fullbcount.get(elt, 0) + 1
        fullbcount = self.fullbcount
        # avail[x] is the number of times x appears in 'b' less the
        # number of times we've seen it in 'a' so far ... kinda
        avail = {}
        availhas, matches = avail.has_key, 0
        for elt in self.a:
            if availhas(elt):
                numb = avail[elt]
            else:
                numb = fullbcount.get(elt, 0)
            avail[elt] = numb - 1
            if numb > 0:
                matches = matches + 1
        return 2.0 * matches / (len(self.a) + len(self.b))

    def real_quick_ratio(self):
        """Return an upper bound on ratio() very quickly.

        This isn't defined beyond that it is an upper bound on .ratio(), and
        is faster to compute than either .ratio() or .quick_ratio().
        """

        la, lb = len(self.a), len(self.b)
        # can't have more matches than the number of elements in the
        # shorter sequence
        return 2.0 * min(la, lb) / (la + lb)

def get_close_matches(word, possibilities, n=3, cutoff=0.6):
    """Use SequenceMatcher to return list of the best "good enough" matches.

    word is a sequence for which close matches are desired (typically a
    string).

    possibilities is a list of sequences against which to match word
    (typically a list of strings).

    Optional arg n (default 3) is the maximum number of close matches to
    return.  n must be > 0.

    Optional arg cutoff (default 0.6) is a float in [0, 1].  Possibilities
    that don't score at least that similar to word are ignored.

    The best (no more than n) matches among the possibilities are returned
    in a list, sorted by similarity score, most similar first.

    >>> get_close_matches("appel", ["ape", "apple", "peach", "puppy"])
    ['apple', 'ape']
    >>> import keyword
    >>> get_close_matches("wheel", keyword.kwlist)
    ['while']
    >>> get_close_matches("apple", keyword.kwlist)
    []
    >>> get_close_matches("accept", keyword.kwlist)
    ['except']
    """

    if not n >  0:
        raise ValueError("n must be > 0: " + `n`)
    if not 0.0 <= cutoff <= 1.0:
        raise ValueError("cutoff must be in [0.0, 1.0]: " + `cutoff`)
    result = []
    s = SequenceMatcher()
    s.set_seq2(word)
    for x in possibilities:
        s.set_seq1(x)
        if s.real_quick_ratio() >= cutoff and \
           s.quick_ratio() >= cutoff and \
           s.ratio() >= cutoff:
            result.append((s.ratio(), x))
    # Sort by score.
    result.sort()
    # Retain only the best n.
    result = result[-n:]
    # Move best-scorer to head of list.
    result.reverse()
    # Strip scores.
    return [x for score, x in result]

def _test():
    import doctest, difflib
    return doctest.testmod(difflib)

if __name__ == "__main__":
    _test()
