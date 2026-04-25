#   Copyright 2010-2025 Antonio Cuni
#                       Daniel Hahler
#
#                        All Rights Reserved
"""Colorful tab completion for Python prompt"""
from _colorize import ANSIColors, get_colors, get_theme
import rlcompleter
import keyword
import types

class Completer(rlcompleter.Completer):
    """
    When doing something like a.b.<tab>, keep the full a.b.attr completion
    stem so readline-style completion can keep refining the menu as you type.

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
            # In GNU readline, this prevents escaping of ANSI control
            # characters in completion results. pyrepl's parse_and_bind()
            # is a no-op, but pyrepl handles ANSI sequences natively
            # via real_len()/stripcolor().
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

    def _callable_attr_postfix(self, val, word):
        return rlcompleter.Completer._callable_postfix(self, val, word)

    def global_matches(self, text):
        names = rlcompleter.Completer.global_matches(self, text)
        prefix = commonprefix(names)
        if prefix and prefix != text:
            return [prefix]

        names.sort()
        values = []
        for name in names:
            clean_name = name.rstrip(': ')
            if keyword.iskeyword(clean_name) or keyword.issoftkeyword(clean_name):
                values.append(None)
            else:
                try:
                    values.append(eval(name, self.namespace))
                except Exception:
                    values.append(None)
        if self.use_colors and names:
            return self.colorize_matches(names, values)
        return names

    def attr_matches(self, text):
        try:
            expr, attr, names, values = self._attr_matches(text)
        except ValueError:
            return []

        if not names:
            return []

        if len(names) == 1:
            # No coloring: when returning a single completion, readline
            # inserts it directly into the prompt, so ANSI codes would
            # appear as literal characters.
            return [self._callable_attr_postfix(values[0], f'{expr}.{names[0]}')]

        prefix = commonprefix(names)
        if prefix and prefix != attr:
            return [f'{expr}.{prefix}']  # autocomplete prefix

        names = [f'{expr}.{name}' for name in names]
        if self.use_colors:
            return self.colorize_matches(names, values)
        return names

    def _attr_matches(self, text):
        expr, attr = text.rsplit('.', 1)
        if '(' in expr or ')' in expr:  # don't call functions
            return expr, attr, [], []
        try:
            thisobject = eval(expr, self.namespace)
        except Exception:
            return expr, attr, [], []

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
                    # Mirror rlcompleter's safeguards so completion does not
                    # call properties or reify lazy module attributes.
                    if isinstance(getattr(type(thisobject), word, None), property):
                        value = None
                    elif (
                        isinstance(thisobject, types.ModuleType)
                        and isinstance(
                            thisobject.__dict__.get(word),
                            types.LazyImportType,
                        )
                    ):
                        value = thisobject.__dict__.get(word)
                    else:
                        value = getattr(thisobject, word, None)

                    names.append(word)
                    values.append(value)
            if names or not noprefix:
                break
            if noprefix == '_':
                noprefix = '__'
            else:
                noprefix = None

        return expr, attr, names, values

    def colorize_matches(self, names, values):
        return [
            self._color_for_obj(name, obj)
            for name, obj in zip(names, values)
        ]

    def _color_for_obj(self, name, value):
        t = type(value)
        color = self._color_by_type(t)
        return f"{color}{name}{ANSIColors.RESET}"

    def _color_by_type(self, t):
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
