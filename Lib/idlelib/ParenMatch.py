"""ParenMatch -- An IDLE extension for parenthesis matching.

When you hit a right paren, the cursor should move briefly to the left
paren.  Paren here is used generically; the matching applies to
parentheses, square brackets, and curly braces.

WARNING: This extension will fight with the CallTips extension,
because they both are interested in the KeyRelease-parenright event.
We'll have to fix IDLE to do something reasonable when two or more
extensions what to capture the same event.
"""

import string

import PyParse
from AutoIndent import AutoIndent, index2line
from IdleConf import idleconf

class ParenMatch:
    """Highlight matching parentheses

    There are three supported style of paren matching, based loosely
    on the Emacs options.  The style is select based on the 
    HILITE_STYLE attribute; it can be changed used the set_style
    method.

    The supported styles are:

    default -- When a right paren is typed, highlight the matching
        left paren for 1/2 sec.

    expression -- When a right paren is typed, highlight the entire
        expression from the left paren to the right paren.

    TODO:
        - fix interaction with CallTips
        - extend IDLE with configuration dialog to change options
        - implement rest of Emacs highlight styles (see below)
        - print mismatch warning in IDLE status window

    Note: In Emacs, there are several styles of highlight where the
    matching paren is highlighted whenever the cursor is immediately
    to the right of a right paren.  I don't know how to do that in Tk,
    so I haven't bothered.
    """
    
    menudefs = []
    
    keydefs = {
        '<<flash-open-paren>>' : ('<KeyRelease-parenright>',
                                  '<KeyRelease-bracketright>',
                                  '<KeyRelease-braceright>'),
        '<<check-restore>>' : ('<KeyPress>',),
    }

    windows_keydefs = {}
    unix_keydefs = {}

    iconf = idleconf.getsection('ParenMatch')
    STYLE = iconf.getdef('style', 'default')
    FLASH_DELAY = iconf.getint('flash-delay')
    HILITE_CONFIG = iconf.getcolor('hilite')
    BELL = iconf.getboolean('bell')
    del iconf

    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.finder = LastOpenBracketFinder(editwin)
        self.counter = 0
        self._restore = None
        self.set_style(self.STYLE)

    def set_style(self, style):
        self.STYLE = style
        if style == "default":
            self.create_tag = self.create_tag_default
            self.set_timeout = self.set_timeout_last
        elif style == "expression":
            self.create_tag = self.create_tag_expression
            self.set_timeout = self.set_timeout_none

    def flash_open_paren_event(self, event):
        index = self.finder.find(keysym_type(event.keysym))
        if index is None:
            self.warn_mismatched()
            return
        self._restore = 1
        self.create_tag(index)
        self.set_timeout()

    def check_restore_event(self, event=None):
        if self._restore:
            self.text.tag_delete("paren")
            self._restore = None

    def handle_restore_timer(self, timer_count):
        if timer_count + 1 == self.counter:
            self.check_restore_event()

    def warn_mismatched(self):
        if self.BELL:
            self.text.bell()

    # any one of the create_tag_XXX methods can be used depending on
    # the style

    def create_tag_default(self, index):
        """Highlight the single paren that matches"""
        self.text.tag_add("paren", index)
        self.text.tag_config("paren", self.HILITE_CONFIG)

    def create_tag_expression(self, index):
        """Highlight the entire expression"""
        self.text.tag_add("paren", index, "insert")
        self.text.tag_config("paren", self.HILITE_CONFIG)

    # any one of the set_timeout_XXX methods can be used depending on
    # the style

    def set_timeout_none(self):
        """Highlight will remain until user input turns it off"""
        pass

    def set_timeout_last(self):
        """The last highlight created will be removed after .5 sec"""
        # associate a counter with an event; only disable the "paren"
        # tag if the event is for the most recent timer.
        self.editwin.text_frame.after(self.FLASH_DELAY,
                                      lambda self=self, c=self.counter: \
                                      self.handle_restore_timer(c))
        self.counter = self.counter + 1

def keysym_type(ks):
    # Not all possible chars or keysyms are checked because of the
    # limited context in which the function is used.
    if ks == "parenright" or ks == "(":
        return "paren"
    if ks == "bracketright" or ks == "[":
        return "bracket"
    if ks == "braceright" or ks == "{":
        return "brace"

class LastOpenBracketFinder:
    num_context_lines = AutoIndent.num_context_lines
    indentwidth = AutoIndent.indentwidth
    tabwidth = AutoIndent.tabwidth
    context_use_ps1 = AutoIndent.context_use_ps1
    
    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text

    def _find_offset_in_buf(self, lno):
        y = PyParse.Parser(self.indentwidth, self.tabwidth)
        for context in self.num_context_lines:
            startat = max(lno - context, 1)
            startatindex = `startat` + ".0"
            # rawtext needs to contain everything up to the last
            # character, which was the close paren.  the parser also
	    # requires that the last line ends with "\n"
            rawtext = self.text.get(startatindex, "insert")[:-1] + "\n"
            y.set_str(rawtext)
            bod = y.find_good_parse_start(
                        self.context_use_ps1,
                        self._build_char_in_string_func(startatindex))
            if bod is not None or startat == 1:
                break
        y.set_lo(bod or 0)
        i = y.get_last_open_bracket_pos()
        return i, y.str

    def find(self, right_keysym_type):
        """Return the location of the last open paren"""
        lno = index2line(self.text.index("insert"))
        i, buf = self._find_offset_in_buf(lno)
        if i is None \
	   or keysym_type(buf[i]) != right_keysym_type:
            return None
        lines_back = string.count(buf[i:], "\n") - 1
        # subtract one for the "\n" added to please the parser
        upto_open = buf[:i]
        j = string.rfind(upto_open, "\n") + 1 # offset of column 0 of line
        offset = i - j
        return "%d.%d" % (lno - lines_back, offset)

    def _build_char_in_string_func(self, startindex):
        def inner(offset, startindex=startindex,
                  icis=self.editwin.is_char_in_string):
            return icis(startindex + "%dc" % offset)
        return inner

