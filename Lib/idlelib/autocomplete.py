"""Complete either attribute names or file names.

Either on demand or after a user-selected delay after a key character,
pop up a list of candidates.
"""
import __main__
import os
import string
import sys

# These constants represent the two different types of completions.
# They must be defined here so autocomple_w can import them.
COMPLETE_ATTRIBUTES, COMPLETE_FILES = range(1, 2+1)

from idlelib import autocomplete_w
from idlelib.config import idleConf
from idlelib.hyperparser import HyperParser

# This string includes all chars that may be in an identifier.
# TODO Update this here and elsewhere.
ID_CHARS = string.ascii_letters + string.digits + "_"

SEPS = os.sep
if os.altsep:  # e.g. '/' on Windows...
    SEPS += os.altsep


class AutoComplete:

    def __init__(self, editwin=None):
        self.editwin = editwin
        if editwin is not None:   # not in subprocess or test
            self.text = editwin.text
            self.autocompletewindow = None
            # id of delayed call, and the index of the text insert when
            # the delayed call was issued. If _delayed_completion_id is
            # None, there is no delayed call.
            self._delayed_completion_id = None
            self._delayed_completion_index = None

    @classmethod
    def reload(cls):
        cls.popupwait = idleConf.GetOption(
            "extensions", "AutoComplete", "popupwait", type="int", default=0)

    def _make_autocomplete_window(self):
        return autocomplete_w.AutoCompleteWindow(self.text)

    def _remove_autocomplete_window(self, event=None):
        if self.autocompletewindow:
            self.autocompletewindow.hide_window()
            self.autocompletewindow = None

    def force_open_completions_event(self, event):
        """Happens when the user really wants to open a completion list, even
        if a function call is needed.
        """
        self.open_completions(True, False, True)
        return "break"

    def try_open_completions_event(self, event):
        """Happens when it would be nice to open a completion list, but not
        really necessary, for example after a dot, so function
        calls won't be made.
        """
        lastchar = self.text.get("insert-1c")
        if lastchar == ".":
            self._open_completions_later(False, False, False,
                                         COMPLETE_ATTRIBUTES)
        elif lastchar in SEPS:
            self._open_completions_later(False, False, False,
                                         COMPLETE_FILES)

    def autocomplete_event(self, event):
        """Happens when the user wants to complete his word, and if necessary,
        open a completion list after that (if there is more than one
        completion)
        """
        if hasattr(event, "mc_state") and event.mc_state or\
                not self.text.get("insert linestart", "insert").strip():
            # A modifier was pressed along with the tab or
            # there is only previous whitespace on this line, so tab.
            return None
        if self.autocompletewindow and self.autocompletewindow.is_active():
            self.autocompletewindow.complete()
            return "break"
        else:
            opened = self.open_completions(False, True, True)
            return "break" if opened else None

    def _open_completions_later(self, *args):
        self._delayed_completion_index = self.text.index("insert")
        if self._delayed_completion_id is not None:
            self.text.after_cancel(self._delayed_completion_id)
        self._delayed_completion_id = \
            self.text.after(self.popupwait, self._delayed_open_completions,
                            *args)

    def _delayed_open_completions(self, *args):
        self._delayed_completion_id = None
        if self.text.index("insert") == self._delayed_completion_index:
            self.open_completions(*args)

    def open_completions(self, evalfuncs, complete, userWantsWin, mode=None):
        """Find the completions and create the AutoCompleteWindow.
        Return True if successful (no syntax error or so found).
        If complete is True, then if there's nothing to complete and no
        start of completion, won't open completions and return False.
        If mode is given, will open a completion list only in this mode.

        Action  Function                Eval   Complete WantWin Mode
        ^space  force_open_completions  True,  False,   True    no
        . or /  try_open_completions    False, False,   False   yes
        tab     autocomplete            False, True,    True    no
        """
        # Cancel another delayed call, if it exists.
        if self._delayed_completion_id is not None:
            self.text.after_cancel(self._delayed_completion_id)
            self._delayed_completion_id = None

        hp = HyperParser(self.editwin, "insert")
        curline = self.text.get("insert linestart", "insert")
        i = j = len(curline)
        if hp.is_in_string() and (not mode or mode==COMPLETE_FILES):
            # Find the beginning of the string.
            # fetch_completions will look at the file system to determine
            # whether the string value constitutes an actual file name
            # XXX could consider raw strings here and unescape the string
            # value if it's not raw.
            self._remove_autocomplete_window()
            mode = COMPLETE_FILES
            # Find last separator or string start
            while i and curline[i-1] not in "'\"" + SEPS:
                i -= 1
            comp_start = curline[i:j]
            j = i
            # Find string start
            while i and curline[i-1] not in "'\"":
                i -= 1
            comp_what = curline[i:j]
        elif hp.is_in_code() and (not mode or mode==COMPLETE_ATTRIBUTES):
            self._remove_autocomplete_window()
            mode = COMPLETE_ATTRIBUTES
            while i and (curline[i-1] in ID_CHARS or ord(curline[i-1]) > 127):
                i -= 1
            comp_start = curline[i:j]
            if i and curline[i-1] == '.':
                hp.set_index("insert-%dc" % (len(curline)-(i-1)))
                comp_what = hp.get_expression()
                if not comp_what or \
                   (not evalfuncs and comp_what.find('(') != -1):
                    return None
            else:
                comp_what = ""
        else:
            return None

        if complete and not comp_what and not comp_start:
            return None
        comp_lists = self.fetch_completions(comp_what, mode)
        if not comp_lists[0]:
            return None
        self.autocompletewindow = self._make_autocomplete_window()
        return not self.autocompletewindow.show_window(
                comp_lists, "insert-%dc" % len(comp_start),
                complete, mode, userWantsWin)

    def fetch_completions(self, what, mode):
        """Return a pair of lists of completions for something. The first list
        is a sublist of the second. Both are sorted.

        If there is a Python subprocess, get the comp. list there.  Otherwise,
        either fetch_completions() is running in the subprocess itself or it
        was called in an IDLE EditorWindow before any script had been run.

        The subprocess environment is that of the most recently run script.  If
        two unrelated modules are being edited some calltips in the current
        module may be inoperative if the module was not the last to run.
        """
        try:
            rpcclt = self.editwin.flist.pyshell.interp.rpcclt
        except:
            rpcclt = None
        if rpcclt:
            return rpcclt.remotecall("exec", "get_the_completion_list",
                                     (what, mode), {})
        else:
            if mode == COMPLETE_ATTRIBUTES:
                if what == "":
                    namespace = {**__main__.__builtins__.__dict__,
                                 **__main__.__dict__}
                    bigl = eval("dir()", namespace)
                    bigl.sort()
                    if "__all__" in bigl:
                        smalll = sorted(eval("__all__", namespace))
                    else:
                        smalll = [s for s in bigl if s[:1] != '_']
                else:
                    try:
                        entity = self.get_entity(what)
                        bigl = dir(entity)
                        bigl.sort()
                        if "__all__" in bigl:
                            smalll = sorted(entity.__all__)
                        else:
                            smalll = [s for s in bigl if s[:1] != '_']
                    except:
                        return [], []

            elif mode == COMPLETE_FILES:
                if what == "":
                    what = "."
                try:
                    expandedpath = os.path.expanduser(what)
                    bigl = os.listdir(expandedpath)
                    bigl.sort()
                    smalll = [s for s in bigl if s[:1] != '.']
                except OSError:
                    return [], []

            if not smalll:
                smalll = bigl
            return smalll, bigl

    def get_entity(self, name):
        "Lookup name in a namespace spanning sys.modules and __main.dict__."
        return eval(name, {**sys.modules, **__main__.__dict__})


AutoComplete.reload()

if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_autocomplete', verbosity=2)
