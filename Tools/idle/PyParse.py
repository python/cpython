import string
import re
import sys

# Reason last stmt is continued (or C_NONE if it's not).
C_NONE, C_BACKSLASH, C_STRING, C_BRACKET = range(4)

if 0:   # for throwaway debugging output
    def dump(*stuff):
        import sys
        sys.__stdout__.write(string.join(map(str, stuff), " ") + "\n")

# find a def or class stmt
_defclassre = re.compile(r"""
    ^
    [ \t]*
    (?:
        def   [ \t]+ [a-zA-Z_]\w* [ \t]* \(
    |   class [ \t]+ [a-zA-Z_]\w* [ \t]*
        (?: \( .* \) )?
        [ \t]* :
    )
""", re.VERBOSE | re.MULTILINE).search

# match blank line or non-indenting comment line
_junkre = re.compile(r"""
    [ \t]*
    (?: \# [^ \t\n] .* )?
    \n
""", re.VERBOSE).match

# match any flavor of string; the terminating quote is optional
# so that we're robust in the face of incomplete program text
_match_stringre = re.compile(r"""
    \""" [^"\\]* (?:
                     (?: \\. | "(?!"") )
                     [^"\\]*
                 )*
    (?: \""" )?

|   " [^"\\\n]* (?: \\. [^"\\\n]* )* "?

|   ''' [^'\\]* (?:
                   (?: \\. | '(?!'') )
                   [^'\\]*
                )*
    (?: ''' )?

|   ' [^'\\\n]* (?: \\. [^'\\\n]* )* '?
""", re.VERBOSE | re.DOTALL).match

# match a line that doesn't start with something interesting;
# used to skip junk lines when searching for the first element
# of a bracket structure
_not_itemre = re.compile(r"""
    [ \t]*
    [#\n\\]
""", re.VERBOSE).match

# match start of stmts that should be followed by a dedent
_closere = re.compile(r"""
    \s*
    (?: return
    |   break
    |   continue
    |   raise
    |   pass
    )
    \b
""", re.VERBOSE).match

# Build translation table to map uninteresting chars to "x", open
# brackets to "(", and close brackets to ")".

_tran = ['x'] * 256
for ch in "({[":
    _tran[ord(ch)] = '('
for ch in ")}]":
    _tran[ord(ch)] = ')'
for ch in "\"'\\\n#":
    _tran[ord(ch)] = ch
_tran = string.join(_tran, '')
del ch

class Parser:

    def __init__(self, indentwidth, tabwidth):
        self.indentwidth = indentwidth
        self.tabwidth = tabwidth

    def set_str(self, str):
        assert len(str) == 0 or str[-1] == '\n'
        self.str = str
        self.study_level = 0

    # Return index of start of last (probable!) def or class stmt, or
    # None if none found.  It's only probable because we can't know
    # whether we're in a string without reparsing from the start of
    # the file -- and that's too slow to bear.
    #
    # Ack, hack: in the shell window this kills us, because there's
    # no way to tell the differences between output, >>> etc and
    # user input.  Indeed, IDLE's first output line makes the rest
    # look like it's in an unclosed paren!:
    # Python 1.5.2 (#0, Apr 13 1999, ...

    def find_last_def_or_class(self, _defclassre=_defclassre):
        str, pos = self.str, None
        i = 0
        while 1:
            m = _defclassre(str, i)
            if m:
                pos, i = m.span()
            else:
                break
        if pos is None:
            # hack for shell window
            ps1 = '\n' + sys.ps1
            i = string.rfind(str, ps1)
            if i >= 0:
                pos = i + len(ps1)
                self.str = str[:pos-1] + '\n' + str[pos:]
        return pos

    # Throw away the start of the string.  Intended to be called with
    # find_last_def_or_class's result.

    def set_lo(self, lo):
        assert lo == 0 or self.str[lo-1] == '\n'
        if lo > 0:
            self.str = self.str[lo:]

    # As quickly as humanly possible <wink>, find the line numbers (0-
    # based) of the non-continuation lines.
    # Creates self.{stmts, continuation}.

    def _study1(self, _replace=string.replace, _find=string.find):
        if self.study_level >= 1:
            return
        self.study_level = 1

        # Map all uninteresting characters to "x", all open brackets
        # to "(", all close brackets to ")", then collapse runs of
        # uninteresting characters.  This can cut the number of chars
        # by a factor of 10-40, and so greatly speed the following loop.
        str = self.str
        str = string.translate(str, _tran)
        str = _replace(str, 'xxxxxxxx', 'x')
        str = _replace(str, 'xxxx', 'x')
        str = _replace(str, 'xx', 'x')
        str = _replace(str, 'xx', 'x')
        str = _replace(str, '\nx', '\n')
        # note that replacing x\n with \n would be incorrect, because
        # x may be preceded by a backslash

        # March over the squashed version of the program, accumulating
        # the line numbers of non-continued stmts, and determining
        # whether & why the last stmt is a continuation.
        continuation = C_NONE
        level = lno = 0     # level is nesting level; lno is line number
        self.stmts = stmts = [0]
        push_stmt = stmts.append
        i, n = 0, len(str)
        while i < n:
            ch = str[i]
            # cases are checked in decreasing order of frequency

            if ch == 'x':
                i = i+1
                continue

            if ch == '\n':
                lno = lno + 1
                if level == 0:
                    push_stmt(lno)
                    # else we're in an unclosed bracket structure
                i = i+1
                continue

            if ch == '(':
                level = level + 1
                i = i+1
                continue

            if ch == ')':
                if level:
                    level = level - 1
                    # else the program is invalid, but we can't complain
                i = i+1
                continue

            if ch == '"' or ch == "'":
                # consume the string
                quote = ch
                if str[i:i+3] == quote * 3:
                    quote = quote * 3
                w = len(quote)
                i = i+w
                while i < n:
                    ch = str[i]
                    if ch == 'x':
                        i = i+1
                        continue

                    if str[i:i+w] == quote:
                        i = i+w
                        break

                    if ch == '\n':
                        lno = lno + 1
                        i = i+1
                        if w == 1:
                            # unterminated single-quoted string
                            if level == 0:
                                push_stmt(lno)
                            break
                        continue

                    if ch == '\\':
                        assert i+1 < n
                        if str[i+1] == '\n':
                            lno = lno + 1
                        i = i+2
                        continue

                    # else comment char or paren inside string
                    i = i+1

                else:
                    # didn't break out of the loop, so it's an
                    # unterminated triple-quoted string
                    assert w == 3
                    continuation = C_STRING
                continue

            if ch == '#':
                # consume the comment
                i = _find(str, '\n', i)
                assert i >= 0
                continue

            assert ch == '\\'
            assert i+1 < n
            if str[i+1] == '\n':
                lno = lno + 1
                if i+2 == n:
                    continuation = C_BACKSLASH
            i = i+2

        # Push the final line number as a sentinel value, regardless of
        # whether it's continued.
        if stmts[-1] != lno:
            push_stmt(lno)

        # The last stmt may be continued for all 3 reasons.
        # String continuation takes precedence over bracket
        # continuation, which beats backslash continuation.
        if continuation != C_STRING and level > 0:
            continuation = C_BRACKET
        self.continuation = continuation

    def get_continuation_type(self):
        self._study1()
        return self.continuation

    # study1 was sufficient to determine the continuation status,
    # but doing more requires looking at every character.  study2
    # does this for the last interesting statement in the block.
    # Creates:
    #     self.stmt_start, stmt_end
    #         slice indices of last interesting stmt
    #     self.lastch
    #         last non-whitespace character before optional trailing
    #         comment
    #     self.lastopenbracketpos
    #         if continuation is C_BRACKET, index of last open bracket

    def _study2(self, _rfind=string.rfind, _find=string.find,
                      _ws=string.whitespace):
        if self.study_level >= 2:
            return
        self._study1()
        self.study_level = 2

        self.lastch = ""

        # Set p and q to slice indices of last interesting stmt.
        str, stmts = self.str, self.stmts
        i = len(stmts) - 1
        p = len(str)    # index of newest line
        found = 0
        while i:
            assert p
            # p is the index of the stmt at line number stmts[i].
            # Move p back to the stmt at line number stmts[i-1].
            q = p
            for nothing in range(stmts[i-1], stmts[i]):
                # tricky: sets p to 0 if no preceding newline
                p = _rfind(str, '\n', 0, p-1) + 1
            # The stmt str[p:q] isn't a continuation, but may be blank
            # or a non-indenting comment line.
            if  _junkre(str, p):
                i = i-1
            else:
                found = 1
                break
        self.stmt_start, self.stmt_end = p, q

        # Analyze this stmt, to find the last open bracket (if any)
        # and last interesting character (if any).
        stack = []  # stack of open bracket indices
        push_stack = stack.append
        while p < q:
            ch = str[p]
            if ch == '"' or ch == "'":
                # consume string
                # Note that study1 did this with a Python loop, but
                # we use a regexp here; the reason is speed in both
                # cases; the string may be huge, but study1 pre-squashed
                # strings to a couple of characters per line.  study1
                # also needed to keep track of newlines, and we don't
                # have to.
                self.lastch = ch
                p = _match_stringre(str, p, q).end()
                continue

            if ch == '#':
                # consume comment and trailing newline
                p = _find(str, '\n', p, q) + 1
                assert p > 0
                continue

            if ch == '\\':
                assert p+1 < q
                if str[p+1] != '\n':
                    # the program is invalid, but can't complain
                    self.lastch = str[p:p+2]
                p = p+2
                continue

            if ch not in _ws:
                self.lastch = ch
                if ch in "([{":
                    push_stack(p)
                elif ch in ")]}" and stack:
                    del stack[-1]
            p = p+1

        # end while p < q:

        if stack:
            self.lastopenbracketpos = stack[-1]

    # Assuming continuation is C_BRACKET, return the number
    # of spaces the next line should be indented.

    def compute_bracket_indent(self, _find=string.find):
        self._study2()
        assert self.continuation == C_BRACKET
        j = self.lastopenbracketpos
        str = self.str
        n = len(str)
        origi = i = string.rfind(str, '\n', 0, j) + 1
        j = j+1
        # find first list item
        while _not_itemre(str, j):
            # this line is junk; advance to the next line
            i = _find(str, '\n', j)
            if i < 0:
                break
            j = i = i+1
        if i < 0 or j >= n:
            # nothing interesting follows the bracket;
            # reproduce the bracket line's indentation + a level
            j = i = origi
            extra = self.indentwidth
        else:
            # the first list item begins on this line; line up with
            # the first interesting character
            extra = 0
        while str[j] in " \t":
            j = j+1
        return len(string.expandtabs(str[i:j],
                                     self.tabwidth)) + extra

    # Return number of physical lines in last stmt (whether or not
    # it's an interesting stmt!  this is intended to be called when
    # continuation is C_BACKSLASH).

    def get_num_lines_in_stmt(self):
        self._study1()
        stmts = self.stmts
        return stmts[-1] - stmts[-2]

    # Assuming continuation is C_BACKSLASH, return the number of spaces
    # the next line should be indented.  Also assuming the new line is
    # the first one following the initial line of the stmt.

    def compute_backslash_indent(self):
        self._study2()
        assert self.continuation == C_BACKSLASH
        str = self.str
        i = self.stmt_start
        while str[i] in " \t":
            i = i+1
        startpos = i
        endpos = string.find(str, '\n', startpos) + 1
        found = level = 0
        while i < endpos:
            ch = str[i]
            if ch in "([{":
                level = level + 1
                i = i+1
            elif ch in ")]}":
                if level:
                    level = level - 1
                i = i+1
            elif ch == '"' or ch == "'":
                i = _match_stringre(str, i, endpos).end()
            elif ch == '#':
                break
            elif level == 0 and ch == '=' and \
                 (i == 0 or str[i-1] not in "=<>!") and \
                 str[i+1] != '=':
                found = 1
                break
            else:
                i = i+1

        if found:
            # found a legit =, but it may be the last interesting
            # thing on the line
            i = i+1     # move beyond the =
            found = re.match(r"\s*\\", str[i:endpos]) is None

        if not found:
            # oh well ... settle for moving beyond the first chunk
            # of non-whitespace chars
            i = startpos
            while str[i] not in " \t\n":
                i = i+1

        return len(string.expandtabs(str[self.stmt_start :
                                         i],
                                     self.tabwidth)) + 1

    # Return the leading whitespace on the initial line of the last
    # interesting stmt.

    def get_base_indent_string(self):
        self._study2()
        i, n = self.stmt_start, self.stmt_end
        assert i is not None
        j = i
        str = self.str
        while j < n and str[j] in " \t":
            j = j + 1
        return str[i:j]

    # Did the last interesting stmt open a block?

    def is_block_opener(self):
        self._study2()
        return self.lastch == ':'

    # Did the last interesting stmt close a block?

    def is_block_closer(self):
        self._study2()
        return _closere(self.str, self.stmt_start) is not None
