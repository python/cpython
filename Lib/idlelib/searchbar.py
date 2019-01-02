"""SearchBar.py - An IDLE extension for searching for text in windows.

The interface is a small bar which appears on the bottom of the window. The
search bar is closed when the user hits Escape or when the bar loses focus,
e.g. the user clicks elsewhere. (Tab traversal outside the search bar is
disabled.)

This extension implements the usual search options, regular expressions, and
incremental searching. Also, while searching, all matches are highlighted.

Incremental searching can be toggled on/off in the extensions configuration.

"""

# TODO:
# * With incremental search, add a small delay after the search expression has
#   changed before searching the entire text, so that if one wants to search
#   for "self" it doesn't search for "s", then "se", then "sel" and finally
#   "self".

import re
import string

from tkinter import Button, Checkbutton, Entry, Frame, Label, StringVar
from tkinter import TOP, BOTTOM, LEFT, RIGHT, X, NONE

from idlelib.config import idleConf
from idlelib.delegator import Delegator
import idlelib.searchengine as searchengine
import idlelib.windowsearchengine as windowsearchengine


class TkTextWidgetChangeTracer(Delegator):
    """Traces insert and delete operations on a Tkinter Text widget.

    To trace a Text widget, create an instance of this class and insert it
    as a filter into a Percolator to which the Text widget's methods are
    redirected. (Such redirection is done using the WidgetRedirector class;
    see EditorWindow.py for an example.)

    The constructor accepts a single argument: a callback which will be
    called whenever the Text widget's contents change.

    Additionally, an instance can be deactivated and rectivated. When
    inactive, an instance will not call its callback.

    """
    def __init__(self, callback, delegate=None):
        self.callback = callback
        self.is_active = True
        Delegator.__init__(self, delegate)

    def activate(self):
        self.is_active = True

    def deactivate(self):
        self.is_active = False

    def insert(self, *args, **kwargs):
        if self.delegate is not None:
            self.delegate.insert(*args, **kwargs)
        if self.is_active:
            self.callback()

    def delete(self, *args, **kwargs):
        if self.delegate is not None:
            self.delegate.delete(*args, **kwargs)
        if self.is_active:
            self.callback()


class SearchBar(object):
    is_incremental = True

    @classmethod
    def reload(cls):
        cls.is_incremental = idleConf.GetOption("main", "Search",
                                                "is-incremental", type="bool",
                                                default=False)

    def __init__(self, editwin):
        self.fb = FindBar(editwin, editwin.status_bar)
        self.rb = ReplaceBar(editwin, editwin.status_bar)

    def search_event(self, event=None):
        return self.fb.show_event(event)

    def search_again_event(self, event=None):
        return self.fb.search_again_event(event)

    def search_selection_event(self, event=None):
        return self.fb.search_selection_event(event)

    def replace_event(self, event=None):
        return self.rb.show_event(event)


def FindBar(editwin, pack_after):
    return SearchBarWidget(editwin, pack_after, is_replace=False)


def ReplaceBar(editwin, pack_after):
    return SearchBarWidget(editwin, pack_after, is_replace=True)


class SearchBarWidget:
    def __init__(self, editwin, pack_after, is_replace=False):
        self.editwin = editwin
        self.is_replace = is_replace
        self.pack_after = pack_after

        self.text = editwin.text
        self.root = self.text._root()
        self.engine = searchengine.get(self.root)
        self.window_engine = windowsearchengine.get(editwin)

        self.top = editwin.top

        self._are_widgets_built = False
        self.is_shown = False
        self.prev_text_takefocus_value = None

        self.find_var = StringVar(self.root)
        self.find_var.trace("w", self._search_expression_changed_callback)

        self.is_safe = False
        self.is_safe_tracer = TkTextWidgetChangeTracer(self._make_unsafe)

        # The text widget's selection isn't shown when it doesn't have the
        # focus. The "findsel" tag looks like the selection, and will be used
        # while the search bar is visible. When the search bar is hidden, the
        # original selection is updated as required.
        #
        # This also allows saving the original selection. If the search bar is
        # hidden with no selected search hit, the original selection is
        # restored.
        self.text.tag_configure(
            "findsel",
            background=self.text.tag_cget("sel", "background"),
            foreground=self.text.tag_cget("sel", "foreground"),
        )

        self._hide()

    def _show(self):
        if not self._are_widgets_built:
            self._build_widgets()
            
        if not self.is_shown:
            self.orig_insert = self.text.index("insert")
            orig_geom = self.top.wm_geometry()
            self.bar_frame.pack(side=BOTTOM, fill=X, expand=1, pady=1,
                                after=self.pack_after)
            # Reset the window's size only if it is in 'normal' state.
            # On Windows, if this is done when the window is maximized
            # ('zoomed' state), then the window will not return to its
            # original size when it is unmaximized.
            if self.top.wm_state() == 'normal':
                self.top.wm_geometry(orig_geom)
                # Ensure that the insertion point is still visible
                self.top.update()
                self.text.see("insert")

            self.window_engine.show_find_marks()
            self.is_shown = True # must be _before_ reset_selection()!
            # Add the "findsel" tag, which looks like the selection
            self._reset_selection()            

    def _hide(self):
        if self._are_widgets_built and self.is_shown:
            self.bar_frame.pack_forget()
            #self.window_engine.reset()
            self.window_engine.hide_find_marks()

            sel = self._get_selection()
            self.is_shown = False # must be _after_ get_selection()!
            if sel:
                self._set_selection(sel[0], sel[1])
                self.text.mark_set("insert", sel[0])
            else:
                self._reset_selection()
                self.text.mark_set("insert", self.orig_insert)
            self.text.see("insert")
            self.editwin.set_line_and_column()

        self.text.tag_remove("findsel", "1.0", "end")

    def _search_expression_changed_callback(self, *args):
        if self.is_shown and SearchBar.is_incremental:
            if self.engine.isre():
                self.window_engine.reset()
                self._clear_selection()
                self.text.see("insert")
            elif self._set_search_expression():
                self._search_event(start=self.text.index("insert"))
            else:
                self._clear_selection()
                self.text.see("insert")

    def _make_safe(self):
        if not self.is_safe:
            self.is_safe = True
            self.editwin.per.insertfilter(self.is_safe_tracer)

    def _make_unsafe(self):
        if self.is_safe:
            self.is_safe = False
            self.editwin.per.removefilter(self.is_safe_tracer)

    def _build_widgets(self):
        if self._are_widgets_built:
            return

        def _make_entry(parent, label_text, var):
            """Utility function for creating Entry widgets"""
            label = Label(parent, text=label_text)
            label.pack(side=LEFT, fill=NONE, expand=0)
            entry = Entry(parent, textvariable=var, exportselection=0,
                          border=1)
            entry.pack(side=LEFT, fill=X, expand=1)
            entry.bind("<Escape>", self.hide_event)
            return entry

        def _make_checkbutton(parent, label_text, var):
            """Utility function for creating Checkbutton widgets"""
            btn = Checkbutton(parent, anchor="w", text=label_text,
                              variable=var)
            btn.pack(side=LEFT, fill=NONE, expand=0)
            btn.bind("<Escape>", self.hide_event)
            return btn

        def _make_button(parent, label_text, command):
            """Utility function for creating Button widgets"""
            btn = Button(parent, text=label_text, command=command)
            btn.pack(side=LEFT, fill=NONE, expand=0)
            btn.bind("<Escape>", self.hide_event)
            return btn

        entries = []

        # Frame for the entire bar
        self.bar_frame = Frame(self.top, border=1, relief="flat")

        # Frame for the 'Find:' / 'Replace:' entry + search options
        self.find_frame = Frame(self.bar_frame, border=0)

        # 'Find:' / 'Replace:' entry
        label = "Find:" if not self.is_replace else "Replace:"
        self.find_ent = _make_entry(self.find_frame,
                                    label, self.find_var)
        self.find_ent.bind("<Return>", self._search_event)
        entries.append(self.find_ent)
        del label

        # Match case checkbutton
        case_btn = _make_checkbutton(self.find_frame,
                                     "Match case", self.engine.casevar)
        if self.engine.iscase():
            case_btn.select()
        case_btn.configure(command=self._search_expression_changed_callback)

        # Regular expression checkbutton
        reg_btn = _make_checkbutton(self.find_frame,
                                    "Reg-Exp", self.engine.revar)
        if self.engine.isre():
            reg_btn.select()
        reg_btn.configure(command=self._search_expression_changed_callback)

        # Whole word checkbutton
        word_btn = _make_checkbutton(self.find_frame,
                                     "Whole word", self.engine.wordvar)
        if self.engine.isword():
            word_btn.select()
        word_btn.configure(command=self._search_expression_changed_callback)

        # Wrap checkbutton
        wrap_btn = _make_checkbutton(self.find_frame,
                                     "Wrap around", self.engine.wrapvar)
        if self.engine.iswrap():
            wrap_btn.select()

        # Direction checkbutton
        direction_txt_var = StringVar(self.root)
        def _update_direction_button():
            direction_txt_var.set("Down" if self.engine.isback() else "Up")
        direction_btn = Checkbutton(self.find_frame,
                                    textvariable=direction_txt_var,
                                    variable=self.engine.backvar,
                                    command=_update_direction_button,
                                    indicatoron=0,
                                    width=5)
        direction_btn.config(selectcolor=direction_btn.cget("bg"))
        direction_btn.pack(side=RIGHT, fill=NONE, expand=0)
        Label(self.find_frame, text="Direction:").pack(side=RIGHT,
                                                       fill=NONE,
                                                       expand=0)
        # TODO: remove?
        if self.engine.isback():
            direction_btn.select()
        else:
            direction_btn.deselect()
        _update_direction_button()
        direction_btn.bind("<Escape>", self.hide_event)

        self.find_frame.pack(side=TOP, fill=X, expand=1)

        if self.is_replace:
            # Frame for the 'With:' entry + replace options
            self.replace_frame = Frame(self.bar_frame, border=0)

            self.replace_with_var = StringVar(self.root)
            self.replace_ent = _make_entry(self.replace_frame, "With:",
                                           self.replace_with_var)
            self.replace_ent.bind("<Return>", self._replace_event)
            self.replace_ent.bind("<Shift-Return>", self._search_event)
            entries.append(self.replace_ent)

            _make_button(self.replace_frame, "Find",
                         self._search_event)
            _make_button(self.replace_frame, "Replace",
                         self._replace_event)
            _make_button(self.replace_frame, "Replace All",
                         self._replace_all_event)

            self.replace_frame.pack(side=TOP, fill=X, expand=1)

        self._are_widgets_built = True


        def _make_toggle_event(button):
            def event(event, button=button):
                button.invoke()
                return "break"
            return event

        for entry in entries:
            entry.bind("<Control-Key-f>", self._search_event)
            entry.bind("<Control-Key-g>", self._search_event)
            entry.bind("<Control-Key-R>", _make_toggle_event(reg_btn))
            entry.bind("<Control-Key-C>", _make_toggle_event(case_btn))
            entry.bind("<Control-Key-W>", _make_toggle_event(wrap_btn))
            entry.bind("<Control-Key-D>", _make_toggle_event(direction_btn))

            expander = EntryExpander(entry, self.text)
            expander.bind("<Alt-Key-slash>")

    # end of _build_widgets

    def _destroy_widgets(self):
        if self._are_widgets_built:
            self.bar_frame.destroy()
            self._are_widgets_built = False

    def __del__(self):
        self._destroy_widgets()

    def show_event(self, event):
        # Put the current selection in the "Find:" entry
        sel = self._get_selection()
        if sel:
            self.find_var.set(self.text.get(sel[0], sel[1]))

        # Now show the FindBar in all it's glory!
        self._show()

        # Select all of the text in the "Find:"/"Replace:" entry
        self.find_ent.selection_range(0, "end")

        # Hide the findbar if the focus is lost
        self.bar_frame.bind("<FocusOut>", self.hide_event)

        # Focus traversal (Tab or Shift-Tab) shouldn't return focus to
        # the text widget
        self.prev_text_takefocus_value = self.text.cget("takefocus")
        self.text.config(takefocus=0)

        # Set the focus to the "Find:"/"Replace:" entry
        self.find_ent.focus()
        return "break"

    def hide_event(self, event=None):
        self._hide()
        self.text.config(takefocus=self.prev_text_takefocus_value)
        self.text.focus()
        return "break"

    def search_again_event(self, event):
        if self.engine.getpat():
            return self._search_event(event)
        else:
            return self.show_event(event)

    def search_selection_event(self, event):
        # Get the current selection.
        sel = self._get_selection()
        if not sel:
            # No selection; beep and leave.
            self.text.bell()
            return "break"

        # Set the window's search engine's pattern to the current selection.
        self.find_var.set(self.text.get(sel[0], sel[1]))

        # Search, temporarily overriding all search flags.
        flags = {"re": False,
                 "case": True,
                 "word": False,
                 "wrap": True,
                 "back": False}
        vars = {flag: getattr(self.engine, flag + 'var') for flag in flags}
        orig_values = {flag: vars[flag].get() for flag in flags}
        for flag, value in flags.items():
            orig_values[flag] = vars[flag].get()
            vars[flag].set(value)
        try:
            return self._search_event(event)
        finally:
            for flag in flags:
                vars[flag].set(orig_values[flag])

    def _search_text(self, start):
        if not self._set_search_expression():
            return None

        direction = not self.engine.isback()
        wrap = self.engine.iswrap()
        sel = self._get_selection()

        # The 'search_start' variable will be used instead of 'start' when
        # searching, to allow starting the search from one character forward
        # when a hit is already selected, thus avoiding repeatedly returning
        # the same hit.
        if start is None:
            if sel:
                search_start = start = sel[0]
                if direction and \
                   self.window_engine.match_range(sel[0], sel[1]):
                    search_start += "+1c"
            else:
                search_start = start = self.text.index("insert")
        else:
            search_start = start

        hit = self.window_engine.findnext(search_start, direction, wrap)

        # Ring the bell if the selection was found again.
        if (
            hit and sel and start == sel[0] and
            self.text.compare(hit[0], '==', sel[0]) and
            self.text.compare(hit[1], '==', sel[1])
        ):
            self.text.bell()

        return hit

    def _search_event(self, event=None, start=None):
        res = self._search_text(start)
        if res:
            first, last = res
            self._set_selection(first, last)
            self.text.see(last)
            self.text.see(first)
            self.text.mark_set("insert", first)
            self.editwin.set_line_and_column()
        else:
            # Don't clear the selection for "Find Again" and "Find Selection".
            if self.is_shown:
                self._clear_selection()
            self.text.bell()
        return "break"

    def _replace_event(self, event=None):
        if not self._set_search_expression():
            return "break"
            
        # Replace if appropriate.
        sel = self._get_selection()
        if sel and self.window_engine.match_range(sel[0], sel[1]):
            replace_with = self.replace_with_var.get()
            self.is_safe_tracer.deactivate()
            self.editwin.undo.undo_block_start()
            if sel[0] != sel[1]:
                self.text.delete(sel[0], sel[1])
            if replace_with:
                self.text.insert(sel[0], replace_with)
            self.editwin.undo.undo_block_stop()
            self.is_safe_tracer.activate()
            self.text.mark_set("insert", sel[0] + '+%dc' % len(replace_with))

        # Now search for the next appearance.
        return self._search_event(event)

    def _replace_all_event(self, event=None):
        if not self._set_search_expression():
            self.text.bell()
            return "break"

        replace_with = self.replace_with_var.get()
        self.is_safe_tracer.deactivate()
        self.editwin.undo.undo_block_start()
        n_replaced = self.window_engine.replace_all(replace_with)
        self.editwin.undo.undo_block_stop()
        self.is_safe_tracer.activate()
        if n_replaced == 0:
            self.text.bell()
        return "break"

    def _set_search_expression(self):
        self.engine.setpat(self.find_var.get())

        search_exp = self._get_search_expression()
        if search_exp is None:
            self.window_engine.reset()
            self._make_unsafe()
            return False

        if not (self.is_safe and
                search_exp == self.window_engine.search_expression):
            self.window_engine.set_search_expression(search_exp)
            self._make_safe()
        return True

    def _get_regexp(self):
        if not self.engine.getpat():
            # If the search expression is empty, return None.
            # (otherwise self.engine.getprog() pops up an error message)
            return None

        return self.engine.getprog()

    def _get_search_expression(self):
        if not self.engine.getpat():
            # If the search expression is empty, return None.
            # (otherwise self.engine.getprog() pops up an error message)
            return None

        if not (self.engine.isre() or self.engine.isword()):
            return (self.engine.getpat(), self.engine.iscase())
        else:
            return self.engine.getprog()

    ### Selection related methods
    seltagname = property(lambda self: "findsel" if self.is_shown else "sel")

    def _clear_selection(self):
        self.text.tag_remove(self.seltagname, "1.0", "end")

    def _set_selection(self, start, end):
        self._clear_selection()
        self.text.tag_add(self.seltagname, start, end)

    def _get_selection(self):
        return self.text.tag_nextrange(self.seltagname, '1.0', 'end')

    def _reset_selection(self):
        if self.is_shown:
            sel = self.text.tag_nextrange("sel", '1.0', 'end')
            if sel:
                self._set_selection(sel[0], sel[1])
            else:
                self._clear_selection()


class EntryExpander(object):
    """Expands words in an entry, taking possible words from a text widget."""
    def __init__(self, entry, text):
        self.text = text
        self.entry = entry
        self._reset()

        self.entry.bind('<Map>', self._reset)

    def _reset(self, event=None):
        self._state = None

    def bind(self, event_string):
        """Bind the given event to the word expansion."""
        self.entry.bind(event_string, self._expand_word_event)

    def _expand_word_event(self, event=None):
        """Cycle through possible expansions for the current word."""
        # load the existing state or get all possible expansions
        curinsert = self.entry.index("insert")
        curline = self.entry.get()
        if not self._state:
            words = self._get_word_expansions()
            index = 0
        else:
            words, index, insert, line = self._state
            if insert != curinsert or line != curline:
                words = self._get_word_expansions()
                index = 0

        if not words: # no possible expansions
            self.text.bell()
            return "break"

        # get the next possible expansion
        newword = words[index]
        index = (index + 1) % len(words)
        if index == 0:
            self.text.bell() # Warn the user that we cycled around

        # remove the previous expansion and insert the new one
        curword = self._get_current_word()
        insert_index = int(self.entry.index("insert"))
        self.entry.delete(str(insert_index - len(curword)), str(insert_index))
        self.entry.insert("insert", newword)

        # remember the current state for next time
        curinsert = self.entry.index("insert")
        curline = self.entry.get()        
        self._state = words, index, curinsert, curline
        return "break"
        
    def _get_word_expansions(self):
        """Get a list of unique expansion suggestions.

        The returned list will be sorted by distance of the expansions from the
        current index in the text widget.

        The final item in the list will be the word currently in the entry
        widget.

        """
        curword = self._get_current_word()
        if not curword:
            return []

        # search for possible expansions in the text widget, sorting them by
        # their distance from the current index
        expansion_regexp = re.compile(r"\b" + curword + r"\w+\b")
        before_text = self.text.get("1.0", "insert wordstart")
        insert_index = len(before_text)
        words = sorted(
            (abs(match.start() - insert_index), match.group())
            for match in expansion_regexp.finditer(
                self.text.get("1.0", "end-1c")
            )
        )

        # remove duplicate appearances of words, keeping the first,
        # while keeping the current word for last
        words_set = {curword}
        def unique_words_iter():
            for match in words:
                word = match[1]
                if word not in words_set:
                    words_set.add(word)
                    yield word
        words = list(unique_words_iter())
        words.append(curword)
        return words

    _wordchars = string.ascii_letters + string.digits + "_"
    def _get_current_word(self):
        """Get the last word in the entry, ending at the 'insert' index."""
        line = self.entry.get()
        i = j = self.entry.index("insert")
        while i > 0 and line[i-1] in self._wordchars:
            i -= 1
        return line[i:j]
