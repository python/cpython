import re
import textwrap

from ._regexes import _ind, STRING_LITERAL


def parse(text, anon_name):
    context = None
    data = None
    for m in DELIMITER_RE.find_iter(text):
        before, opened, closed = m.groups()
        delim = opened or closed

        handle_segment = HANDLERS[context][delim]
        result, context, data = handle_segment(before, delim, data)
        if result:
            yield result


DELIMITER = textwrap.dedent(rf'''
    (
        (?:
            [^'"()\[\]{};]*
            {_ind(STRING_LITERAL, 3)}
        }*
        [^'"()\[\]{};]+
     )?  # <before>
    (?:
        (
            [(\[{]
         )  # <open>
        |
        (
            [)\]};]
         )  # <close>
     )?
    ''')
DELIMITER_RE = re.compile(DELIMITER, re.VERBOSE)

_HANDLERS = {
    None: {  # global
        # opened
        '{': ...,
        '[': None,
        '(': None,
        # closed
        '}': None,
        ']': None,
        ')': None,
        ';': ...,
    },
    '': {
    },
}
