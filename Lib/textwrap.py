"""
Utilities for wrapping text strings and filling text paragraphs.
"""

__revision__ = "$Id$"

import string, re


# XXX is this going to be implemented properly somewhere in 2.3?
def islower (c):
    return c in string.lowercase


class TextWrapper:
    """
    Object for wrapping/filling text.  The public interface consists of
    the wrap() and fill() methods; the other methods are just there for
    subclasses to override in order to tweak the default behaviour.
    If you want to completely replace the main wrapping algorithm,
    you'll probably have to override _wrap_chunks().

    Several instance attributes control various aspects of
    wrapping:
      expand_tabs
        if true (default), tabs in input text will be expanded
        to spaces before further processing.  Each tab will
        become 1 .. 8 spaces, depending on its position in its line.
        If false, each tab is treated as a single character.
      replace_whitespace
        if true (default), all whitespace characters in the input
        text are replaced by spaces after tab expansion.  Note
        that expand_tabs is false and replace_whitespace is true,
        every tab will be converted to a single space!
      break_long_words
        if true (default), words longer than the line width constraint
        will be broken.  If false, those words will not be broken,
        and some lines might be longer than the width constraint.
    """

    whitespace_trans = string.maketrans(string.whitespace,
                                        ' ' * len(string.whitespace))

    # This funky little regex is just the trick for splitting 
    # text up into word-wrappable chunks.  E.g.
    #   "Hello there -- you goof-ball, use the -b option!"
    # splits into
    #   Hello/ /there/ /--/ /you/ /goof-/ball,/ /use/ /the/ /-b/ /option!
    # (after stripping out empty strings).
    wordsep_re = re.compile(r'(\s+|'                  # any whitespace
                            r'\w{2,}-(?=\w{2,})|'     # hyphenated words
                            r'(?<=\w)-{2,}(?=\w))')   # em-dash


    def __init__ (self):
        self.expand_tabs = 1
        self.replace_whitespace = 1
        self.break_long_words = 1
        

    # -- Private methods -----------------------------------------------
    # (possibly useful for subclasses to override)

    def _munge_whitespace (self, text):
        """_munge_whitespace(text : string) -> string

        Munge whitespace in text: expand tabs and convert all other
        whitespace characters to spaces.  Eg. " foo\tbar\n\nbaz"
        becomes " foo    bar  baz".
        """
        if self.expand_tabs:
            text = text.expandtabs()
        if self.replace_whitespace:
            text = text.translate(self.whitespace_trans)
        return text


    def _split (self, text):
        """_split(text : string) -> [string]

        Split the text to wrap into indivisible chunks.  Chunks are
        not quite the same as words; see wrap_chunks() for full
        details.  As an example, the text
          Look, goof-ball -- use the -b option!
        breaks into the following chunks:
          'Look,', ' ', 'goof-', 'ball', ' ', '--', ' ',
          'use', ' ', 'the', ' ', '-b', ' ', 'option!'
        """
        chunks = self.wordsep_re.split(text)
        chunks = filter(None, chunks)
        return chunks

    def _fix_sentence_endings (self, chunks):
        """_fix_sentence_endings(chunks : [string])

        Correct for sentence endings buried in 'chunks'.  Eg. when the
        original text contains "... foo.\nBar ...", munge_whitespace()
        and split() will convert that to [..., "foo.", " ", "Bar", ...]
        which has one too few spaces; this method simply changes the one
        space to two.
        """
        i = 0
        while i < len(chunks)-1:
            # chunks[i] looks like the last word of a sentence,
            # and it's followed by a single space.
            if (chunks[i][-1] == "." and
                  chunks[i+1] == " " and
                  islower(chunks[i][-2])):
                chunks[i+1] = "  "
                i += 2
            else:
                i += 1

    def _handle_long_word (self, chunks, cur_line, cur_len, width):
        """_handle_long_word(chunks : [string],
                             cur_line : [string],
                             cur_len : int, width : int)

        Handle a chunk of text (most likely a word, not whitespace) that
        is too long to fit in any line.
        """
        space_left = width - cur_len

        # If we're allowed to break long words, then do so: put as much
        # of the next chunk onto the current line as will fit.
        if self.break_long_words:
            cur_line.append(chunks[0][0:space_left])
            chunks[0] = chunks[0][space_left:]

        # Otherwise, we have to preserve the long word intact.  Only add
        # it to the current line if there's nothing already there --
        # that minimizes how much we violate the width constraint.
        elif not cur_line:
            cur_line.append(chunks.pop(0))

        # If we're not allowed to break long words, and there's already
        # text on the current line, do nothing.  Next time through the
        # main loop of _wrap_chunks(), we'll wind up here again, but
        # cur_len will be zero, so the next line will be entirely
        # devoted to the long word that we can't handle right now.

    def _wrap_chunks (self, chunks, width):
        """_wrap_chunks(chunks : [string], width : int) -> [string]

        Wrap a sequence of text chunks and return a list of lines of
        length 'width' or less.  (If 'break_long_words' is false, some
        lines may be longer than 'width'.)  Chunks correspond roughly to
        words and the whitespace between them: each chunk is indivisible
        (modulo 'break_long_words'), but a line break can come between
        any two chunks.  Chunks should not have internal whitespace;
        ie. a chunk is either all whitespace or a "word".  Whitespace
        chunks will be removed from the beginning and end of lines, but
        apart from that whitespace is preserved.
        """
        lines = []

        while chunks:

            cur_line = []                   # list of chunks (to-be-joined)
            cur_len = 0                     # length of current line

            # First chunk on line is whitespace -- drop it.
            if chunks[0].strip() == '':
                del chunks[0]

            while chunks:
                l = len(chunks[0])

                # Can at least squeeze this chunk onto the current line.
                if cur_len + l <= width:
                    cur_line.append(chunks.pop(0))
                    cur_len += l

                # Nope, this line is full.
                else:
                    break

            # The current line is full, and the next chunk is too big to
            # fit on *any* line (not just this one).  
            if chunks and len(chunks[0]) > width:
                self._handle_long_word(chunks, cur_line, cur_len, width)

            # If the last chunk on this line is all whitespace, drop it.
            if cur_line and cur_line[-1].strip() == '':
                del cur_line[-1]

            # Convert current line back to a string and store it in list
            # of all lines (return value).
            if cur_line:
                lines.append(''.join(cur_line))

        return lines


    # -- Public interface ----------------------------------------------

    def wrap (self, text, width):
        """wrap(text : string, width : int) -> [string]

        Split 'text' into multiple lines of no more than 'width'
        characters each, and return the list of strings that results.
        Tabs in 'text' are expanded with string.expandtabs(), and all
        other whitespace characters (including newline) are converted to
        space.
        """
        text = self._munge_whitespace(text)
        if len(text) <= width:
            return [text]
        chunks = self._split(text)
        self._fix_sentence_endings(chunks)
        return self._wrap_chunks(chunks, width)

    def fill (self, text, width, initial_tab="", subsequent_tab=""):
        """fill(text : string,
                width : int,
                initial_tab : string = "",
                subsequent_tab : string = "")
           -> string

        Reformat the paragraph in 'text' to fit in lines of no more than
        'width' columns.  The first line is prefixed with 'initial_tab',
        and subsequent lines are prefixed with 'subsequent_tab'; the
        lengths of the tab strings are accounted for when wrapping lines
        to fit in 'width' columns.
        """
        lines = self.wrap(text, width)
        sep = "\n" + subsequent_tab
        return initial_tab + sep.join(lines)


# Convenience interface

_wrapper = TextWrapper()

def wrap (text, width):
    return _wrapper.wrap(text, width)

def fill (text, width, initial_tab="", subsequent_tab=""):
    return _wrapper.fill(text, width, initial_tab, subsequent_tab)
