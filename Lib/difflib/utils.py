import re
from collections import namedtuple as _namedtuple
from heapq import nlargest as _nlargest

Match = _namedtuple("Match", "a b size")

def _calculate_ratio(matches, length):
    if length:
        return 2.0 * matches / length
    return 1.0





def _keep_original_ws(s, tag_s):
    """Replace whitespace with the original whitespace characters in `s`"""
    return "".join(
        c if tag_c == " " and c.isspace() else tag_c for c, tag_c in zip(s, tag_s)
    )


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


def IS_LINE_JUNK(line, pat=re.compile(r"\s*(?:#\s*)?$").match):
    r"""
    Return True for ignorable line: iff `line` is blank or contains a single '#'.

    Examples:

    >>> IS_LINE_JUNK('\n')
    True
    >>> IS_LINE_JUNK('  #   \n')
    True
    >>> IS_LINE_JUNK('hello\n')
    False
    """

    return pat(line) is not None


def IS_CHARACTER_JUNK(ch, ws=" \t"):
    r"""
    Return True for ignorable character: iff `ch` is a space or tab.

    Examples:

    >>> IS_CHARACTER_JUNK(' ')
    True
    >>> IS_CHARACTER_JUNK('\t')
    True
    >>> IS_CHARACTER_JUNK('\n')
    False
    >>> IS_CHARACTER_JUNK('x')
    False
    """

    return ch in ws


########################################################################
###  Unified Diff
########################################################################


def _format_range_unified(start, stop):
    'Convert range to the "ed" format'
    # Per the diff spec at http://www.unix.org/single_unix_specification/
    beginning = start + 1  # lines start numbering with one
    length = stop - start
    if length == 1:
        return "{}".format(beginning)
    if not length:
        beginning -= 1  # empty ranges begin at line just before the range
    return "{},{}".format(beginning, length)





########################################################################
###  Context Diff
########################################################################


def _format_range_context(start, stop):
    'Convert range to the "ed" format'
    # Per the diff spec at http://www.unix.org/single_unix_specification/
    beginning = start + 1  # lines start numbering with one
    length = stop - start
    if not length:
        beginning -= 1  # empty ranges begin at line just before the range
    if length <= 1:
        return "{}".format(beginning)
    return "{},{}".format(beginning, beginning + length - 1)




def _check_types(a, b, *args):
    # Checking types is weird, but the alternative is garbled output when
    # someone passes mixed bytes and str to {unified,context}_diff(). E.g.
    # without this check, passing filenames as bytes results in output like
    #   --- b'oldfile.txt'
    #   +++ b'newfile.txt'
    # because of how str.format() incorporates bytes objects.
    if a and not isinstance(a[0], str):
        raise TypeError(
            "lines to compare must be str, not %s (%r)" % (type(a[0]).__name__, a[0])
        )
    if b and not isinstance(b[0], str):
        raise TypeError(
            "lines to compare must be str, not %s (%r)" % (type(b[0]).__name__, b[0])
        )
    if isinstance(a, str):
        raise TypeError(
            "input must be a sequence of strings, not %s" % type(a).__name__
        )
    if isinstance(b, str):
        raise TypeError(
            "input must be a sequence of strings, not %s" % type(b).__name__
        )
    for arg in args:
        if not isinstance(arg, str):
            raise TypeError("all arguments must be str, not: %r" % (arg,))


def diff_bytes(
    dfunc,
    a,
    b,
    fromfile=b"",
    tofile=b"",
    fromfiledate=b"",
    tofiledate=b"",
    n=3,
    lineterm=b"\n",
):
    r"""
    Compare `a` and `b`, two sequences of lines represented as bytes rather
    than str. This is a wrapper for `dfunc`, which is typically either
    unified_diff() or context_diff(). Inputs are losslessly converted to
    strings so that `dfunc` only has to worry about strings, and encoded
    back to bytes on return. This is necessary to compare files with
    unknown or inconsistent encoding. All other inputs (except `n`) must be
    bytes rather than str.
    """

    def decode(s):
        try:
            return s.decode("ascii", "surrogateescape")
        except AttributeError as err:
            msg = "all arguments must be bytes, not %s (%r)" % (type(s).__name__, s)
            raise TypeError(msg) from err

    a = list(map(decode, a))
    b = list(map(decode, b))
    fromfile = decode(fromfile)
    tofile = decode(tofile)
    fromfiledate = decode(fromfiledate)
    tofiledate = decode(tofiledate)
    lineterm = decode(lineterm)

    lines = dfunc(a, b, fromfile, tofile, fromfiledate, tofiledate, n, lineterm)
    for line in lines:
        yield line.encode("ascii", "surrogateescape")



def _mdiff(fromlines, tolines, context=None, linejunk=None, charjunk=IS_CHARACTER_JUNK):
    r"""Returns generator yielding marked up from/to side by side differences.

    Arguments:
    fromlines -- list of text lines to compared to tolines
    tolines -- list of text lines to be compared to fromlines
    context -- number of context lines to display on each side of difference,
               if None, all from/to text lines will be generated.
    linejunk -- passed on to ndiff (see ndiff documentation)
    charjunk -- passed on to ndiff (see ndiff documentation)

    This function returns an iterator which returns a tuple:
    (from line tuple, to line tuple, boolean flag)

    from/to line tuple -- (line num, line text)
        line num -- integer or None (to indicate a context separation)
        line text -- original line text with following markers inserted:
            '\0+' -- marks start of added text
            '\0-' -- marks start of deleted text
            '\0^' -- marks start of changed text
            '\1' -- marks end of added/deleted/changed text

    boolean flag -- None indicates context separation, True indicates
        either "from" or "to" line contains a change, otherwise False.

    This function/iterator was originally developed to generate side by side
    file difference for making HTML pages (see HtmlDiff class for example
    usage).

    Note, this function utilizes the ndiff function to generate the side by
    side difference markup.  Optional ndiff arguments may be passed to this
    function and they in turn will be passed to ndiff.
    """

    # regular expression for finding intraline change indices
    change_re = re.compile(r"(\++|\-+|\^+)")

    # create the difference iterator to generate the differences
    diff_lines_iterator = ndiff(fromlines, tolines, linejunk, charjunk)

    def _make_line(lines, format_key, side, num_lines=[0, 0]):
        """Returns line of text with user's change markup and line formatting.

        lines -- list of lines from the ndiff generator to produce a line of
                 text from.  When producing the line of text to return, the
                 lines used are removed from this list.
        format_key -- '+' return first line in list with "add" markup around
                          the entire line.
                      '-' return first line in list with "delete" markup around
                          the entire line.
                      '?' return first line in list with add/delete/change
                          intraline markup (indices obtained from second line)
                      None return first line in list with no markup
        side -- indice into the num_lines list (0=from,1=to)
        num_lines -- from/to current line number.  This is NOT intended to be a
                     passed parameter.  It is present as a keyword argument to
                     maintain memory of the current line numbers between calls
                     of this function.

        Note, this function is purposefully not defined at the module scope so
        that data it needs from its parent function (within whose context it
        is defined) does not need to be of module scope.
        """
        num_lines[side] += 1
        # Handle case where no user markup is to be added, just return line of
        # text with user's line format to allow for usage of the line number.
        if format_key is None:
            return (num_lines[side], lines.pop(0)[2:])
        # Handle case of intraline changes
        if format_key == "?":
            text, markers = lines.pop(0), lines.pop(0)
            # find intraline changes (store change type and indices in tuples)
            sub_info = []

            def record_sub_info(match_object, sub_info=sub_info):
                sub_info.append([match_object.group(1)[0], match_object.span()])
                return match_object.group(1)

            change_re.sub(record_sub_info, markers)
            # process each tuple inserting our special marks that won't be
            # noticed by an xml/html escaper.
            for key, (begin, end) in reversed(sub_info):
                text = text[0:begin] + "\0" + key + text[begin:end] + "\1" + text[end:]
            text = text[2:]
        # Handle case of add/delete entire line
        else:
            text = lines.pop(0)[2:]
            # if line of text is just a newline, insert a space so there is
            # something for the user to highlight and see.
            if not text:
                text = " "
            # insert marks that won't be noticed by an xml/html escaper.
            text = "\0" + format_key + text + "\1"
        # Return line of text, first allow user's line formatter to do its
        # thing (such as adding the line number) then replace the special
        # marks with what the user's change markup.
        return (num_lines[side], text)

    def _line_iterator():
        """Yields from/to lines of text with a change indication.

        This function is an iterator.  It itself pulls lines from a
        differencing iterator, processes them and yields them.  When it can
        it yields both a "from" and a "to" line, otherwise it will yield one
        or the other.  In addition to yielding the lines of from/to text, a
        boolean flag is yielded to indicate if the text line(s) have
        differences in them.

        Note, this function is purposefully not defined at the module scope so
        that data it needs from its parent function (within whose context it
        is defined) does not need to be of module scope.
        """
        lines = []
        num_blanks_pending, num_blanks_to_yield = 0, 0
        while True:
            # Load up next 4 lines so we can look ahead, create strings which
            # are a concatenation of the first character of each of the 4 lines
            # so we can do some very readable comparisons.
            while len(lines) < 4:
                lines.append(next(diff_lines_iterator, "X"))
            s = "".join([line[0] for line in lines])
            if s.startswith("X"):
                # When no more lines, pump out any remaining blank lines so the
                # corresponding add/delete lines get a matching blank line so
                # all line pairs get yielded at the next level.
                num_blanks_to_yield = num_blanks_pending
            elif s.startswith("-?+?"):
                # simple intraline change
                yield _make_line(lines, "?", 0), _make_line(lines, "?", 1), True
                continue
            elif s.startswith("--++"):
                # in delete block, add block coming: we do NOT want to get
                # caught up on blank lines yet, just process the delete line
                num_blanks_pending -= 1
                yield _make_line(lines, "-", 0), None, True
                continue
            elif s.startswith(("--?+", "--+", "- ")):
                # in delete block and see an intraline change or unchanged line
                # coming: yield the delete line and then blanks
                from_line, to_line = _make_line(lines, "-", 0), None
                num_blanks_to_yield, num_blanks_pending = num_blanks_pending - 1, 0
            elif s.startswith("-+?"):
                # intraline change
                yield _make_line(lines, None, 0), _make_line(lines, "?", 1), True
                continue
            elif s.startswith("-?+"):
                # intraline change
                yield _make_line(lines, "?", 0), _make_line(lines, None, 1), True
                continue
            elif s.startswith("-"):
                # delete FROM line
                num_blanks_pending -= 1
                yield _make_line(lines, "-", 0), None, True
                continue
            elif s.startswith("+--"):
                # in add block, delete block coming: we do NOT want to get
                # caught up on blank lines yet, just process the add line
                num_blanks_pending += 1
                yield None, _make_line(lines, "+", 1), True
                continue
            elif s.startswith(("+ ", "+-")):
                # will be leaving an add block: yield blanks then add line
                from_line, to_line = None, _make_line(lines, "+", 1)
                num_blanks_to_yield, num_blanks_pending = num_blanks_pending + 1, 0
            elif s.startswith("+"):
                # inside an add block, yield the add line
                num_blanks_pending += 1
                yield None, _make_line(lines, "+", 1), True
                continue
            elif s.startswith(" "):
                # unchanged text, yield it to both sides
                yield _make_line(lines[:], None, 0), _make_line(lines, None, 1), False
                continue
            # Catch up on the blank lines so when we yield the next from/to
            # pair, they are lined up.
            while num_blanks_to_yield < 0:
                num_blanks_to_yield += 1
                yield None, ("", "\n"), True
            while num_blanks_to_yield > 0:
                num_blanks_to_yield -= 1
                yield ("", "\n"), None, True
            if s.startswith("X"):
                return
            else:
                yield from_line, to_line, True

    def _line_pair_iterator():
        """Yields from/to lines of text with a change indication.

        This function is an iterator.  It itself pulls lines from the line
        iterator.  Its difference from that iterator is that this function
        always yields a pair of from/to text lines (with the change
        indication).  If necessary it will collect single from/to lines
        until it has a matching pair from/to pair to yield.

        Note, this function is purposefully not defined at the module scope so
        that data it needs from its parent function (within whose context it
        is defined) does not need to be of module scope.
        """
        line_iterator = _line_iterator()
        fromlines, tolines = [], []
        while True:
            # Collecting lines of text until we have a from/to pair
            while len(fromlines) == 0 or len(tolines) == 0:
                try:
                    from_line, to_line, found_diff = next(line_iterator)
                except StopIteration:
                    return
                if from_line is not None:
                    fromlines.append((from_line, found_diff))
                if to_line is not None:
                    tolines.append((to_line, found_diff))
            # Once we have a pair, remove them from the collection and yield it
            from_line, fromDiff = fromlines.pop(0)
            to_line, to_diff = tolines.pop(0)
            yield (from_line, to_line, fromDiff or to_diff)

    # Handle case where user does not want context differencing, just yield
    # them up without doing anything else with them.
    line_pair_iterator = _line_pair_iterator()
    if context is None:
        yield from line_pair_iterator
    # Handle case where user wants context differencing.  We must do some
    # storage of lines until we know for sure that they are to be yielded.
    else:
        context += 1
        lines_to_write = 0
        while True:
            # Store lines up until we find a difference, note use of a
            # circular queue because we only need to keep around what
            # we need for context.
            index, contextLines = 0, [None] * (context)
            found_diff = False
            while found_diff is False:
                try:
                    from_line, to_line, found_diff = next(line_pair_iterator)
                except StopIteration:
                    return
                i = index % context
                contextLines[i] = (from_line, to_line, found_diff)
                index += 1
            # Yield lines that we have collected so far, but first yield
            # the user's separator.
            if index > context:
                yield None, None, None
                lines_to_write = context
            else:
                lines_to_write = index
                index = 0
            while lines_to_write:
                i = index % context
                index += 1
                yield contextLines[i]
                lines_to_write -= 1
            # Now yield the context lines after the change
            lines_to_write = context - 1
            try:
                while lines_to_write:
                    from_line, to_line, found_diff = next(line_pair_iterator)
                    # If another change within the context, extend the context
                    if found_diff:
                        lines_to_write = context - 1
                    else:
                        lines_to_write -= 1
                    yield from_line, to_line, found_diff
            except StopIteration:
                # Catch exception from next() and return normally
                return


del re


def restore(delta, which):
    r"""
    Generate one of the two sequences that generated a delta.

    Given a `delta` produced by `Differ.compare()` or `ndiff()`, extract
    lines originating from file 1 or 2 (parameter `which`), stripping off line
    prefixes.

    Examples:

    >>> diff = ndiff('one\ntwo\nthree\n'.splitlines(keepends=True),
    ...              'ore\ntree\nemu\n'.splitlines(keepends=True))
    >>> diff = list(diff)
    >>> print(''.join(restore(diff, 1)), end="")
    one
    two
    three
    >>> print(''.join(restore(diff, 2)), end="")
    ore
    tree
    emu
    """
    try:
        tag = {1: "- ", 2: "+ "}[int(which)]
    except KeyError:
        raise ValueError("unknown delta choice (must be 1 or 2): %r" % which) from None
    prefixes = ("  ", tag)
    for line in delta:
        if line[:2] in prefixes:
            yield line[2:]
