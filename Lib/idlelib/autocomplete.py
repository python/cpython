"""Complete either attribute names or file names.

Either on demand or after a user-selected delay after a key character,
pop up a list of candidates.
"""
import __main__
import itertools
import os
import re
import string
import sys

# Two types of completions; defined here for autocomplete_w import below.
ATTRS, FILES, DICTKEYS = 0, 1, 2
from idlelib import autocomplete_w
from idlelib.config import idleConf
from idlelib.hyperparser import HyperParser

# Tuples passed to open_completions.
#       EvalFunc, Complete, WantWin, Mode
FORCE = True,     False,    True,    None   # Control-Space.
TAB   = False,    True,     True,    None   # Tab.
TRY_A = False,    False,    False,   ATTRS  # '.' for attributes.
TRY_F = False,    False,    False,   FILES  # '/' in quotes for file name.

# This string includes all chars that may be in an identifier.
# TODO Update this here and elsewhere.
ID_CHARS = string.ascii_letters + string.digits + "_"

SEPS = f"{os.sep}{os.altsep if os.altsep else ''}"
TRIGGERS = f".{SEPS}"

class AutoComplete:

    def __init__(self, editwin=None):
        self.editwin = editwin
        if editwin is not None:   # not in subprocess or no-gui test
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

    def _make_autocomplete_window(self):  # Makes mocking easier.
        return autocomplete_w.AutoCompleteWindow(self.text)

    def _remove_autocomplete_window(self, event=None):
        if self.autocompletewindow:
            self.autocompletewindow.hide_window()
            self.autocompletewindow = None

    def force_open_completions_event(self, event):
        "(^space) Open completion list, even if a function call is needed."
        self.open_completions(FORCE)
        return "break"

    def autocomplete_event(self, event):
        "(tab) Complete word or open list if multiple options."
        if hasattr(event, "mc_state") and event.mc_state or\
                not self.text.get("insert linestart", "insert").strip():
            # A modifier was pressed along with the tab or
            # there is only previous whitespace on this line, so tab.
            return None
        if self.autocompletewindow and self.autocompletewindow.is_active():
            self.autocompletewindow.complete()
            return "break"
        else:
            opened = self.open_completions(TAB)
            return "break" if opened else None

    def try_open_completions_event(self, event=None):
        "(./) Open completion list after pause with no movement."
        lastchar = self.text.get("insert-1c")
        if lastchar in TRIGGERS:
            args = TRY_A if lastchar == "." else TRY_F
            self._delayed_completion_index = self.text.index("insert")
            if self._delayed_completion_id is not None:
                self.text.after_cancel(self._delayed_completion_id)
            self._delayed_completion_id = self.text.after(
                self.popupwait, self._delayed_open_completions, args)

    def _delayed_open_completions(self, args):
        "Call open_completions if index unchanged."
        self._delayed_completion_id = None
        if self.text.index("insert") == self._delayed_completion_index:
            self.open_completions(args)

    _dict_str_key_prefix_re = re.compile(r'\s*(br|rb|fr|rf|b|f|r|u|)', re.I)

    @classmethod
    def is_completing_dict_key(cls, hp, curline):
        if hp.is_in_string():
            opening_bracket_idx = hp.bracketing[hp.indexbracket - 1][0]
            if hp.rawtext[opening_bracket_idx] != '[':
                return None
            opening_quote_idx = hp.bracketing[hp.indexbracket][0]
            m = cls._dict_str_key_prefix_re.fullmatch(
                hp.rawtext[opening_bracket_idx+1:opening_quote_idx])
            if m is None:
                return None
            comp_start = m.group(1) + hp.rawtext[opening_quote_idx:hp.indexinrawtext]
        elif hp.is_in_code():
            if not hp.isopener[hp.indexbracket]:
                return None
            opening_bracket_idx = hp.bracketing[hp.indexbracket][0]
            if hp.rawtext[opening_bracket_idx] != '[':
                return None
            m = cls._dict_str_key_prefix_re.fullmatch(
                hp.rawtext[opening_bracket_idx + 1:hp.indexinrawtext])
            if m is None:
                return None
            comp_start = m.group(1)
        else:
            return None

        hp.set_index("insert-%dc" % (len(hp.rawtext) - opening_bracket_idx))
        comp_what = hp.get_expression()
        return comp_what, comp_start

    @classmethod
    def dict_str_key_reprs(cls, comp_start, comp_list):
        # is_bytes = None
        is_raw = False
        quote_type = '"'
        prefix_match = cls._dict_str_key_prefix_re.match(comp_start)
        if prefix_match is not None:
            prefix = prefix_match.group(1)
            post_prefix = comp_start[prefix_match.end():]
            lowercase_prefix = prefix.lower()

            # if 'f' in lowercase_prefix and '{' in post_prefix:
            #     pass
            # else:
            # is_bytes = 'b' in lowercase_prefix
            # if (
            #         not is_bytes and
            #         not post_prefix and
            #         lowercase_prefix in ('', 'r')
            # ):
            #     is_bytes = None
            is_raw = 'r' in lowercase_prefix
            if post_prefix:
                quote_type_candidate = post_prefix[0]
                if quote_type_candidate not in "'\"":
                    pass
                elif post_prefix[:3] == quote_type_candidate * 3:
                    quote_type = quote_type_candidate * 3
                else:
                    quote_type = quote_type_candidate
        else:
            prefix = lowercase_prefix = ''

        prefix = prefix.replace('b', '').replace('B', '')
        bytes_prefix = (prefix + 'b') if lowercase_prefix in ('', 'r') else 'b'

        escape_re = re.compile(r'[\0-\15]')
        escapes = {chr(x): f'\\{x}' for x in range(8)}
        escapes.update({
            7: '\\a',
            8: '\\b',
            9: '\\t',
            10: '\\n',
            11: '\\v',
            12: '\\f',
            13: '\\r',
        })
        high_byte_escape_re = re.compile(r'[\x80-\xff]')
        def high_byte_sub(char):
            return f'\\x{ord(char):x}'

        str_comps = (c for c in comp_list if type(c) == str)
        bytes_comps = (c for c in comp_list if type(c) == bytes)

        str_repr_template = f"{prefix}{quote_type}{{}}{quote_type}"
        str_comps = [x.replace(quote_type, '\\'+quote_type) for x in str_comps]
        if not is_raw:
            str_comps = [escape_re.sub(escapes.__getitem__, x) for x in str_comps]
            str_comps = [str_repr_template.format(x) for x in str_comps]
        else:
            str_comps = [
                str_repr_template.format(x)
                if escape_re.search(x) is None
                else f"{prefix.replace('r', '').replace('R', '')}{quote_type}{escape_re.sub(escapes.__getitem__, x)}{quote_type}"
                for x in str_comps
            ]

        bytes_repr_template = f"{bytes_prefix}{quote_type}{{}}{quote_type}"
        bytes_comps = [x.decode('latin-1') for x in bytes_comps]
        bytes_comps = [x.replace(quote_type, '\\'+quote_type) for x in bytes_comps]
        if not is_raw:
            bytes_comps = [escape_re.sub(escapes.__getitem__, x) for x in bytes_comps]
            bytes_comps = [high_byte_escape_re.sub(high_byte_sub, x) for x in bytes_comps]
            bytes_comps = [bytes_repr_template.format(x) for x in bytes_comps]
        else:
            bytes_comps = [
                bytes_repr_template.format(x)
                if (
                    escape_re.search(x) is None and
                    high_byte_escape_re.search(x) is None
                )
                else f"b{quote_type}{high_byte_escape_re.sub(high_byte_sub, escape_re.sub(escapes.__getitem__, x))}{quote_type}"
                for x in bytes_comps
            ]

        return itertools.chain(str_comps, bytes_comps)

    def open_completions(self, args):
        """Find the completions and create the AutoCompleteWindow.
        Return True if successful (no syntax error or so found).
        If complete is True, then if there's nothing to complete and no
        start of completion, won't open completions and return False.
        If mode is given, will open a completion list only in this mode.
        """
        evalfuncs, complete, wantwin, mode = args
        # Cancel another delayed call, if it exists.
        if self._delayed_completion_id is not None:
            self.text.after_cancel(self._delayed_completion_id)
            self._delayed_completion_id = None

        hp = HyperParser(self.editwin, "insert")
        curline = self.text.get("insert linestart", "insert")
        i = j = len(curline)

        if mode in (None, DICTKEYS):
            comp_what_and_start = self.is_completing_dict_key(hp, curline)
            if comp_what_and_start is not None:
                comp_what, comp_start = comp_what_and_start
                if comp_what and (evalfuncs or '(' not in comp_what):
                    self._remove_autocomplete_window()
                    mode = DICTKEYS
        if mode in (None, FILES) and hp.is_in_string():
            # Find the beginning of the string.
            # fetch_completions will look at the file system to determine
            # whether the string value constitutes an actual file name
            # XXX could consider raw strings here and unescape the string
            # value if it's not raw.
            self._remove_autocomplete_window()
            mode = FILES
            # Find last separator or string start
            while i and curline[i-1] not in "'\"" + SEPS:
                i -= 1
            comp_start = curline[i:j]
            j = i
            # Find string start
            while i and curline[i-1] not in "'\"":
                i -= 1
            comp_what = curline[i:j]
        elif mode in (None, ATTRS) and hp.is_in_code():
            self._remove_autocomplete_window()
            mode = ATTRS
            while i and (curline[i-1] in ID_CHARS or ord(curline[i-1]) > 127):
                i -= 1
            comp_start = curline[i:j]
            if i and curline[i-1] == '.':  # Need object with attributes.
                hp.set_index("insert-%dc" % (len(curline)-(i-1)))
                comp_what = hp.get_expression()
                if (not comp_what or
                   (not evalfuncs and comp_what.find('(') != -1)):
                    return None
            else:
                comp_what = ""
        elif mode == DICTKEYS:
            pass
        else:
            return None

        if complete and not comp_what and not comp_start:
            return None
        comp_lists = self.fetch_completions(comp_what, comp_start, mode)
        if mode == DICTKEYS:
            # if hp.is_in_string():
            #     sep_index = max(0, *(comp_start.rfind(sep) for sep in SEPS))
            #     file_comp_what = comp_start[sep_index:]
            #     extra_comp_lists = self.fetch_completions(file_comp_what, FILES)
            # elif hp.is_in_code():
            if hp.is_in_code():
                extra_comp_lists = self.fetch_completions("", "", ATTRS)
                if comp_lists[0] is comp_lists[1]:
                    comp_lists = comp_lists[0], list(comp_lists[1])
                comp_lists[0].extend(extra_comp_lists[0])
                comp_lists[0].sort()
                comp_lists[1].extend(extra_comp_lists[1])
                comp_lists[1].sort()
        if not comp_lists[0]:
            return None
        self.autocompletewindow = self._make_autocomplete_window()
        return not self.autocompletewindow.show_window(
                comp_lists, "insert-%dc" % len(comp_start),
                complete, mode, wantwin)

    def fetch_completions(self, what, start, mode):
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
                                     (what, start, mode), {})
        else:
            if mode == ATTRS:
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

            elif mode == FILES:
                if what == "":
                    what = "."
                try:
                    expandedpath = os.path.expanduser(what)
                    bigl = os.listdir(expandedpath)
                    bigl.sort()
                    smalll = [s for s in bigl if s[:1] != '.']
                except OSError:
                    return [], []

            elif mode == DICTKEYS:
                try:
                    entity = self.get_entity(what)
                    keys = entity.keys() if isinstance(entity, dict) else []
                except:
                    return [], []
                str_keys = [k for k in keys if type(k) in (bytes, str)]
                smalll = bigl = sorted(self.dict_str_key_reprs(start, str_keys))

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
