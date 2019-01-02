# TODO:
# * Allow overlapping search hits in RegexpSearchEngine (?)
import re

from idlelib.config import idleConf


def get(editwin):
    """Return the WindowSearchEngine instance for the editor window.

    The single WindowSearchEngine saves state for the given window
    while tracking changes to its contents, allowing to reuse
    already computed search results when possible.

    If there is not a WindowSearchEngine already, make one.

    """
    if not hasattr(editwin, "_window_search_engine"):
        editwin._window_search_engine = WindowSearchEngine(editwin.text)
    return editwin._window_search_engine


class StringSearchEngine(object):
    """A class for searching a Text widget for a given string.

    Provides a findnext() method which finds the next occurrence of the string
    in the text relative to a given index and allows wrapping search, case
    sensitive/insensitive search and searching forward or backward.

    When instantiated, an instance first marks all appearances of the string in
    the text with a tag named "findmark".

    """
    def __init__(self, text_widget, string, case_sensitive=True):
        self.text_widget = text_widget
        self.string = string
        self.case_sensitive = case_sensitive
        self._mark_hits()

    def _search(self, start_index, direction=1, wrap=False):
        """Internal search function."""
        # Use the underlying Text widget's search function. Otherwise
        # we would have to copy at least some of the underlying text in order
        # to search through it.
        #
        # The Text widget's search function wraps by default. To disallow
        # wrapping we need to set stopindex appropriately, i.e. "end" for
        # forward search or "1.0" for backward search.
        return self.text_widget.search(
            self.string, start_index,
            backwards=not direction,
            stopindex=(not wrap) and ("end" if direction else "1.0"),
            nocase=not self.case_sensitive)

    def _mark_hits(self):
        """Search the text, marking all matches with the "findmark" tag."""
        add_string_len_str = "+%dc" % len(self.string)
        tag_add = self.text_widget.tag_add

        start_idx = self._search("1.0")
        while start_idx:
            tag_add("findmark", start_idx, start_idx + add_string_len_str)
            start_idx = self._search(start_idx + "+1c")

    def search_range(self, range_, backward=False):
        if not backward:
            start_index, stop_index = range_
        else:
            stop_index, start_index = range_        
        hit = self.text_widget.search(self.string, start_index,
                                      stopindex=stop_index,
                                      backwards=backward,
                                      nocase=(not self.case_sensitive))
        if hit:
            return hit, hit + "+%dc" % len(self.string)
        return None

    def findnext(self, start_index, direction=1, wrap=False):
        """Find the next text sequence which matches the given string."""
        index = self._search(start_index, direction, wrap)
        if index:
            return index, index + "+%dc" % len(self.string)
        return None  # no hit found


class RegexpSearchEngine(object):
    """A class for searching a Text widget for a given reg-exp.

    Provides a findnext() method which finds the next match for the reg-exp in
    the text relative to a given index and allows wrapping search and searching
    forward or backward.

    When instantiated, an instance first marks all matches for the reg-exp in
    the text with a tag named "findmark". Later searches are very quick since
    they just jump to the next occurrence of this tag.

    """
    def __init__(self, text, regexp):
        self.text_widget = text
        self.regexp = regexp
        self._mark_hits()

    def _mark_hits(self):
        """Search the text, marking all matches with the "findmark" tag."""
        # The search & tag algorithm has been optimized by counting the number
        # of line breaks before each hit. This allows using precise index
        # identifiers for the Text widget, instead of "1.0+<index in string>c",
        # which speeds up the setting of tags immensely.
        text = self.text_widget.get("1.0", "end-1c")
        prev = 0
        line = 1
        rfind = text.rfind
        tag_add = self.text_widget.tag_add
        for res in self.regexp.finditer(text):
            start, end = res.span()
            line += text[prev:start].count("\n")
            prev = start
            start_idx = "%d.%d" % (line,
                                   start - (rfind("\n", 0, start) + 1))
            tag_add("findmark", start_idx, start_idx + "+%dc" % (end - start))

    def search_range(self, range_, backward=False):
        """Check whether the regular expression matches the text in the given
        range of indices in the text widget.

        """
        start, end = range_

        # For regexp starting and/or ending with \b, we need to look at one
        # additional character before and/or after the given range. This is
        # required in order to support "whole word" searches, where \b is added
        # before and after the given search expression.
        add_left = (self.regexp.pattern.startswith(r"\b") and
                    self.text_widget.compare(start, ">", "1.0"))
        if add_left:
            start = start + "-1c"
        add_right = (self.regexp.pattern.endswith(r"\b") and
                     self.text_widget.compare(end, "<", "end"))
        if add_right:
            end = end + "+1c"

        text = self.text_widget.get(start, end)

        matches = self.regexp.finditer(text)
        if backward:
            matches = reversed(list(matches))
        for match in matches:
            # Skip matches not in the given range. This would be caused by the
            # addition of one character before and/or after the given range
            # (see above).
            if (
                (add_left and match.start() == 0) or
                (add_right and match.end() == len(text))
            ):
                continue
            return (start + "+%dc" % match.start(),
                    start + "+%dc" % match.end())

        return None  # no valid match found

    def findnext(self, start, direction=1, wrap=True):
        """Find the next text sequence which matches the given regexp.

        The 'next' sequence is the one after the selection or the insert
        cursor, or before if the direction is up instead of down.

        """
        text_widget = self.text_widget # we're going to be using this often...

        index = start

        # Check if the current index is still inside a "findmark" tag.
        # If it is, tag_nextrange will skip it, so we use tag_prevrange instead.
        prev_range = text_widget.tag_prevrange("findmark",
                                               start, start + ' linestart')
        if prev_range and text_widget.compare(start, "<", prev_range[1]):
            # We're still inside a findmark tag! This is a rare case, probably
            # caused by adjacent hits. We'll play it safe: search this tag
            # range for another match.
            if direction: # searching forward
                search_start, search_end = start, prev_range[1]
            else: # searching backward
                search_start, search_end = prev_range[0], start
                # When searching backward we change the current index to the
                # beginning of the range, so that if we continue searching (via
                # tag_prevrange) we won't find this tag range again.
                index = search_start
            hit = self.search_range((search_start, search_end), backward=True)
            if hit:
                return hit

        # The current index isn't in a "findmark" tag range, or if it is then
        # there are no subsequent matches in the range. Search for the next
        # "findmark" tag range.
        #
        # Whenever we find a "findmark" tag range, we want to be sure to return
        # a single precise match. Therefore search for regexp matches in the
        # tag range, and if there are matches then return the first one.

        if direction:  # searching forward
            end = "end"
            while True:
                tag_range = text_widget.tag_nextrange("findmark", index, end)
                if tag_range:
                    hit = self.search_range(tag_range)
                    if hit:
                        return hit
                    index = tag_range[1]
                else:
                    if not wrap:
                        break
                    index, end = "1.0", start
                    wrap = False
        else:  # searching backward
            end = "1.0"
            while True:
                tag_range = text_widget.tag_prevrange("findmark", index, end)
                if tag_range:
                    hit = self.search_range(tag_range, backward=True)
                    if hit:
                        return hit
                    index = tag_range[0]
                else:
                    if not wrap:
                        break
                    index, end = "end", start
                    wrap = False

        return None  # no hit found
        

class WindowSearchEngine(object):
    """A class for searching a Text widget.

    This can search for either plain strings or regular expressions, using
    either StringSearchEngine or RegexpSearchEngine as appropriate.

    Provides a findnext() method which finds the next occurence of the string
    in the text relative to a given index and allows wrapping search, case
    sensitive/insensitive search and searching forward or backward.

    """
    def __init__(self, text_widget):
        self.text_widget = text_widget

        self.hide_find_marks()  # initialize "findmark" tag
        self.reset()

    def __del__(self):
        self.text_widget.tag_delete("findmark")

    def show_find_marks(self):
        # Get the highlight colors for "hit"
        # Do this here (and not in __init__) so that config changes take
        # effect immediately
        currentTheme = idleConf.CurrentTheme()
        highlight_dict = idleConf.GetHighlight(currentTheme, "hit")
        self.text_widget.tag_configure("findmark",
                                       foreground=highlight_dict["foreground"],
                                       background=highlight_dict["background"])

    def hide_find_marks(self):
        self.text_widget.tag_configure("findmark",
                                       foreground="",
                                       background="")

    def reset(self):
        self.text_widget.tag_remove("findmark", "1.0", "end")
        self.search_expression = None
        self.search_engine = None

    def set_search_expression(self, search_exp):
        self.reset()
        self.search_expression = search_exp
        if isinstance(search_exp, tuple):
            string, case_sensitive = search_exp
            self.engine = StringSearchEngine(self.text_widget,
                                             string, case_sensitive)
        else:
            assert isinstance(search_exp, re.Pattern)
            self.engine = RegexpSearchEngine(self.text_widget, search_exp)

    def match_range(self, start, end):
        match = self.search_range(start, end)
        return (match and
                self.text_widget.compare(match[0], '==', start) and
                self.text_widget.compare(match[1], '==', end))


    def search_range(self, start, end, backward=False):
        return self.engine.search_range((start, end), backward)

    def findnext(self, start, direction=1, wrap=False):
        return self.engine.findnext(start, direction, wrap)
        
    def replace_all(self, replace_with):
        insert = self.text_widget.insert
        delete = self.text_widget.delete
        
        n_replaced = 0
        hit = self.findnext("1.0", direction=1, wrap=False)
        while hit:
            n_replaced += 1
            first, last = hit
            if first != last:
                delete(first, last)
            if replace_with:
                insert(first, replace_with)
            hit = self.findnext(first + "+%dc" % len(replace_with),
                                direction=1, wrap=False)
        return n_replaced


def test_string_search_engine():
    from tkinter import Text, Tk

    root = Tk()
    text_widget = Text(root)
    engine = StringSearchEngine(text_widget)

    test_data = [
        ("abcabcabca", "abc",
         [("1.0", "1.3"), ("1.3", "1.6"), ("1.6", "1.9")]),
        ("if and only if", " if",
         [("1.11", "1.14")]),
        ("'''", "'",
         [("1.0", "1.1"), ("1.1", "1.2"), ("1.2", "1.3")]),
        ]

    results = []
    for text, string, indices in test_data:
        text_widget.insert("1.0", text)
        idx = "1.0"
        i = 0
        while True:
            res = engine.findnext(string, idx, wrap=False)
            if not res:
                if i < len(indices):
                    results.append(((False, 'ran out early'),
                                    text, string, indices))
                else:
                    results.append((True, text, string, indices))
                break
            start, end = res
            if not (indices and
                    text_widget.compare(indices[i][0], '==', start) and
                    text_widget.compare(indices[i][1], '==', end)):
                results.append(((False, 'did not find'),
                                text, string, indices))
                break
            i += 1
            idx = start + "+1c"
        text_widget.delete("1.0", "end")
    return results

