"""
Utilities for wrapping text strings and filling text paragraphs.
"""

# Copyright (C) 2001 Gregory P. Ward.
# Copyright (C) 2002 Python Software Foundation.
# Written by Greg Ward <gward@python.net>

__revision__ = "$Id$"

import string, re

class TextWrapper:
    """
    Object for wrapping/filling text.  The public interface consists of
    the wrap() and fill() methods; the other methods are just there for
    subclasses to override in order to tweak the default behaviour.
    If you want to completely replace the main wrapping algorithm,
    you'll probably have to override _wrap_chunks().

    Several instance attributes control various aspects of wrapping:
      width (default: 70)
        the maximum width of wrapped lines (unless break_long_words
        is false)
      expand_tabs (default: true)
        Expand tabs in input text to spaces before further processing.
        Each tab will become 1 .. 8 spaces, depending on its position in
        its line.  If false, each tab is treated as a single character.
      replace_whitespace (default: true)
        Replace all whitespace characters in the input text by spaces
        after tab expansion.  Note that if expand_tabs is false and
        replace_whitespace is true, every tab will be converted to a
        single space!
      fix_sentence_endings (default: false)
        Ensure that sentence-ending punctuation is always followed
        by two spaces.  Off by default becaus the algorithm is
        (unavoidably) imperfect.
      break_long_words (default: true)
        Break words longer than 'width'.  If false, those words will not
        be broken, and some lines might be longer than 'width'.
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

    # XXX will there be a locale-or-charset-aware version of
    # string.lowercase in 2.3?
    sentence_end_re = re.compile(r'[%s]'              # lowercase letter
                                 r'[\.\!\?]'          # sentence-ending punct.
                                 r'[\"\']?'           # optional end-of-quote
                                 % string.lowercase)


    def __init__ (self,
                  width=70,
                  expand_tabs=True,
                  replace_whitespace=True,
                  fix_sentence_endings=False,
                  break_long_words=True):
        self.width = width
        self.expand_tabs = expand_tabs
        self.replace_whitespace = replace_whitespace
        self.fix_sentence_endings = fix_sentence_endings
        self.break_long_words = break_long_words
        

    # -- Private methods -----------------------------------------------
    # (possibly useful for subclasses to override)

    def _munge_whitespace(self, text):
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


    def _split(self, text):
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

    def _fix_sentence_endings(self, chunks):
        """_fix_sentence_endings(chunks : [string])

        Correct for sentence endings buried in 'chunks'.  Eg. when the
        original text contains "... foo.\nBar ...", munge_whitespace()
        and split() will convert that to [..., "foo.", " ", "Bar", ...]
        which has one too few spaces; this method simply changes the one
        space to two.
        """
        i = 0
        pat = self.sentence_end_re
        while i < len(chunks)-1:
            if chunks[i+1] == " " and pat.search(chunks[i]):
                chunks[i+1] = "  "
                i += 2
            else:
                i += 1

    def _handle_long_word(self, chunks, cur_line, cur_len):
        """_handle_long_word(chunks : [string],
                             cur_line : [string],
                             cur_len : int)

        Handle a chunk of text (most likely a word, not whitespace) that
        is too long to fit in any line.
        """
        space_left = self.width - cur_len

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

    def _wrap_chunks(self, chunks):
        """_wrap_chunks(chunks : [string]) -> [string]

        Wrap a sequence of text chunks and return a list of lines of
        length 'self.width' or less.  (If 'break_long_words' is false,
        some lines may be longer than this.)  Chunks correspond roughly
        to words and the whitespace between them: each chunk is
        indivisible (modulo 'break_long_words'), but a line break can
        come between any two chunks.  Chunks should not have internal
        whitespace; ie. a chunk is either all whitespace or a "word".
        Whitespace chunks will be removed from the beginning and end of
        lines, but apart from that whitespace is preserved.
        """
        lines = []
        width = self.width

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
                self._handle_long_word(chunks, cur_line, cur_len)

            # If the last chunk on this line is all whitespace, drop it.
            if cur_line and cur_line[-1].strip() == '':
                del cur_line[-1]

            # Convert current line back to a string and store it in list
            # of all lines (return value).
            if cur_line:
                lines.append(''.join(cur_line))

        return lines


    # -- Public interface ----------------------------------------------

    def wrap(self, text):
        """wrap(text : string) -> [string]

        Split 'text' into multiple lines of no more than 'self.width'
        characters each, and return the list of strings that results.
        Tabs in 'text' are expanded with string.expandtabs(), and all
        other whitespace characters (including newline) are converted to
        space.
        """
        text = self._munge_whitespace(text)
        if len(text) <= self.width:
            return [text]
        chunks = self._split(text)
        if self.fix_sentence_endings:
            self._fix_sentence_endings(chunks)
        return self._wrap_chunks(chunks)

    def fill(self, text, initial_tab="", subsequent_tab=""):
        """fill(text : string,
                initial_tab : string = "",
                subsequent_tab : string = "")
           -> string

        Reformat the paragraph in 'text' to fit in lines of no more than
        'width' columns.  The first line is prefixed with 'initial_tab',
        and subsequent lines are prefixed with 'subsequent_tab'; the
        lengths of the tab strings are accounted for when wrapping lines
        to fit in 'width' columns.
        """
        lines = self.wrap(text)
        sep = "\n" + subsequent_tab
        return initial_tab + sep.join(lines)


# Convenience interface

def wrap(text, width):
    return TextWrapper(width=width).wrap(text)

def fill(text, width, initial_tab="", subsequent_tab=""):
    return TextWrapper(width=width).fill(text, initial_tab, subsequent_tab)
