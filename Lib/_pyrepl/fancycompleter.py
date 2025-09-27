#   Copyright 2010-2025 Antonio Cuni
#                       Daniel Hahler
#
#                        All Rights Reserved
"""Colorful tab completion for Python prompt"""
from _colorize import ANSIColors, get_colors, get_theme
import rlcompleter
import types
import keyword

class Completer(rlcompleter.Completer):
    """
    When doing someting like a.b.<tab>, display only the attributes of
    b instead of the full a.b.attr string.

    Optionally, display the various completions in different colors
    depending on the type.
    """
    def __init__(
        self,
        namespace=None,
        *,
        use_colors='auto',
        consider_getitems=True,
    ):
        from _pyrepl import readline
        rlcompleter.Completer.__init__(self, namespace)
        if use_colors == 'auto':
            # use colors only if we can
            use_colors = get_colors().RED != ""
        self.use_colors = use_colors
        self.consider_getitems = consider_getitems

        if self.use_colors:
            readline.parse_and_bind('set dont-escape-ctrl-chars on')
            self.theme = get_theme()
        else:
            self.theme = None

        if self.consider_getitems:
            delims = readline.get_completer_delims()
            delims = delims.replace('[', '')
            delims = delims.replace(']', '')
            readline.set_completer_delims(delims)

    def complete(self, text, state):
        # if you press <tab> at the beginning of a line, insert an actual
        # \t. Else, trigger completion.
        if text == "":
            return ('\t', None)[state]
        else:
            return rlcompleter.Completer.complete(self, text, state)

    def _callable_postfix(self, val, word):
        # disable automatic insertion of '(' for global callables
        return word

    def global_matches(self, text):
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
        if self.use_colors and names:
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

        # sort the words now to make sure to return completions in
        # alphabetical order. It's easier to do it now, else we would need to
        # sort 'names' later but make sure that 'values' in kept in sync,
        # which is annoying.
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

        if self.use_colors:
            return self.colorize_matches(names, values)

        if prefix:
            names.append(' ')
        return names

    def colorize_matches(self, names, values):
        matches = [self.color_for_obj(i, name, obj)
                   for i, (name, obj)
                   in enumerate(zip(names, values))]
        # We add a space at the end to prevent the automatic completion of the
        # common prefix, which is the ANSI escape sequence.
        matches.append(' ')
        return matches

    def color_for_obj(self, i, name, value):
        t = type(value)
        color = self.color_by_type(t)
        # hack: prepend an (increasing) fake escape sequence,
        # so that readline can sort the matches correctly.
        N = f"\x1b[{i:03d};00m"
        return f"{N}{color}{name}{ANSIColors.RESET}"

    def color_by_type(self, t):
        typename = t.__name__
        # this is needed e.g. to turn method-wrapper into method_wrapper,
        # because if we want _colorize.FancyCompleter to be "dataclassable"
        # our keys need to be valid identifiers.
        typename = typename.replace('-', '_').replace('.', '_')
        return getattr(self.theme.fancycompleter, typename, ANSIColors.RESET)


def commonprefix(names):
    """Return the common prefix of all 'names'"""
    if not names:
        return ''
    s1 = min(names)
    s2 = max(names)
    for i, c in enumerate(s1):
        if c != s2[i]:
            return s1[:i]
    return s1
