#   Copyright 2010-2025 Antonio Cuni
#                       Daniel Hahler
#
#                        All Rights Reserved
"""
Colorful tab completion for Python prompt
"""
from _pyrepl import readline
from _colorize import ANSIColors, get_colors
import rlcompleter
import types


class DefaultConfig:

    consider_getitems = True
    use_colors = 'auto'

    color_by_type = {
        types.BuiltinMethodType: ANSIColors.BOLD_CYAN,
        types.MethodType: ANSIColors.BOLD_CYAN,
        type((42).__add__): ANSIColors.BOLD_CYAN,
        type(int.__add__): ANSIColors.BOLD_CYAN,
        type(str.replace): ANSIColors.BOLD_CYAN,

        types.FunctionType: ANSIColors.BOLD_BLUE,
        types.BuiltinFunctionType: ANSIColors.BOLD_BLUE,

        type: ANSIColors.BOLD_MAGENTA,

        types.ModuleType: ANSIColors.CYAN,
        types.NoneType: ANSIColors.GREY,
        str: ANSIColors.BOLD_GREEN,
        bytes: ANSIColors.BOLD_GREEN,
        int: ANSIColors.BOLD_YELLOW,
        float: ANSIColors.BOLD_YELLOW,
        complex: ANSIColors.BOLD_YELLOW,
        bool: ANSIColors.BOLD_YELLOW,
    }

    def setup(self):
        if self.use_colors == 'auto':
            colors = get_colors()
            self.use_colors = colors.RED != ""


class Completer(rlcompleter.Completer):
    """
    When doing someting like a.b.<tab>, display only the attributes of
    b instead of the full a.b.attr string.

    Optionally, display the various completions in different colors
    depending on the type.
    """
    def __init__(self, namespace=None, Config=DefaultConfig):
        rlcompleter.Completer.__init__(self, namespace)
        self.config = Config()
        self.config.setup()

        # XXX: double check what happens in this case once fancycompleter works
        if False and hasattr(readline, '_setup'):
            # This is needed to offer PyREPL a better chance to patch
            # raw_input. Usually, it does at import time, but if we are under
            # pytest with output captured, at import time we don't have a
            # terminal and thus the raw_input hook is not installed.
            readline._setup()

        if self.config.use_colors:
            readline.parse_and_bind('set dont-escape-ctrl-chars on')
        if self.config.consider_getitems:
            delims = readline.get_completer_delims()
            delims = delims.replace('[', '')
            delims = delims.replace(']', '')
            readline.set_completer_delims(delims)

    def complete(self, text, state):
        """
        stolen from:
        http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/496812
        """
        if text == "":
            return ('\t', None)[state]
        else:
            return rlcompleter.Completer.complete(self, text, state)

    def _callable_postfix(self, val, word):
        # disable automatic insertion of '(' for global callables:
        # this method exists only in Python 2.6+
        return word

    def global_matches(self, text):
        import keyword
        names = rlcompleter.Completer.global_matches(self, text)
        prefix = commonprefix(names)
        if prefix and prefix != text:
            return [prefix]

        names.sort()
        values = []
        for name in names:
            clean_name = name.rstrip(': ')
            if clean_name in keyword.kwlist:
                values.append(None)
            else:
                try:
                    values.append(eval(name, self.namespace))
                except Exception as exc:
                    values.append(None)
        if self.config.use_colors and names:
            return self.colorize_matches(names, values)
        return names

    def attr_matches(self, text):
        expr, attr = text.rsplit('.', 1)
        if '(' in expr or ')' in expr:  # don't call functions
            return []
        try:
            thisobject = eval(expr, self.namespace)
        except Exception:
            return []

        # get the content of the object, except __builtins__
        words = set(dir(thisobject)) - {'__builtins__'}

        if hasattr(thisobject, '__class__'):
            words.add('__class__')
            words.update(rlcompleter.get_class_members(thisobject.__class__))
        names = []
        values = []
        n = len(attr)
        if attr == '':
            noprefix = '_'
        elif attr == '_':
            noprefix = '__'
        else:
            noprefix = None
        words = sorted(words)
        while True:
            for word in words:
                if (
                    word[:n] == attr
                    and not (noprefix and word[:n+1] == noprefix)
                ):
                    try:
                        val = getattr(thisobject, word)
                    except Exception:
                        val = None  # Include even if attribute not set

                    names.append(word)
                    values.append(val)
            if names or not noprefix:
                break
            if noprefix == '_':
                noprefix = '__'
            else:
                noprefix = None

        if not names:
            return []

        if len(names) == 1:
            return [f'{expr}.{names[0]}']  # only option, no coloring.

        prefix = commonprefix(names)
        if prefix and prefix != attr:
            return [f'{expr}.{prefix}']  # autocomplete prefix

        if self.config.use_colors:
            return self.colorize_matches(names, values)

        if prefix:
            names += [' ']
        return names

    def colorize_matches(self, names, values):
        matches = [self.color_for_obj(i, name, obj)
                   for i, (name, obj)
                   in enumerate(zip(names, values))]
        # We add a space at the end to prevent the automatic completion of the
        # common prefix, which is the ANSI escape sequence.
        return matches + [' ']

    def color_for_obj(self, i, name, value):
        t = type(value)
        color = self.config.color_by_type.get(t, ANSIColors.RESET)
        # hack: prepend an (increasing) fake escape sequence,
        # so that readline can sort the matches correctly.
        N = f"\x1b[{i:03d};00m"
        return f"{N}{color}{name}{ANSIColors.RESET}"


def commonprefix(names, base=''):
    """Return the common prefix of all 'names' starting with 'base'"""
    if base:
        names = [x for x in names if x.startswith(base)]
    if not names:
        return ''
    s1 = min(names)
    s2 = max(names)
    for i, c in enumerate(s1):
        if c != s2[i]:
            return s1[:i]
    return s1
