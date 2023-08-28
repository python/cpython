import functools
import warnings
import re
import textwrap
import email.message
import dataclasses

from ._text import FoldedCase


# Do not remove prior to 2024-01-01 or Python 3.14
_warn = functools.partial(
    warnings.warn,
    "Implicit None on return values is deprecated and will raise KeyErrors.",
    DeprecationWarning,
    stacklevel=2,
)

# It looks like RFC5322 but it's much much worse. The only takeaway from
# RFC5233 is that special characters such as "," and "<" must be quoted
# when used as text.

# Split an RFC5233-ish list:
# 1. Alt 1: match single or double quotes and handle escape characters.
# 2. Alt 2: match anything except ',' followed by a space. If quote
#    characters are unbalanced, they will be matched here.
# 3. Match the alternatives at least once, in any order...
# 4. ... and capture them.
# 5. Match the list separator, or end-of-string.
# Result:
#   group 1 (list entry): None or non-empty string.

_entries = re.compile(r"""
( (?: (["']) (?:(?!\2|\\).|\\.)* \2     # 1
  |   (?!,\ ).                          # 2
  )+                                    # 3
)                                       # 4
(?:,\ |$)                               # 5
""", re.VERBOSE)

# Split an RFC5233-ish name-email entry:
# 01. Start at the beginning.
# 02. If it starts with '<', skip this name-capturing regex.
# 03. Alt 1: match single or double quotes and handle escape characters.
# 04. Alt 2: match anything except one or more spaces followed by '<'. If
#     quote characters are unbalanced, they will be matched here.
# 05. Match the alternatives at least once, in any order...
# 06. ... but optionally so the result will be 'None' rather than an empty
#     string.
# 07. If the name portion is missing there may not be whitespace before
#     '<'.
# 08. Capture everything after '<' with a non-greedy quantifier to allow #
#     for the next regex. Use '+','?' to force an empty string to become
#     'None'.
# 09. Strip the final '>', if it exists.
# 10. Allow for missing email section.
# 11. Finish at the end.
# Result:
#   group 1 (name):  None or non-empty string.
#   group 3 (email): None or non-empty string.

_name_email = re.compile(r"""
^                                           # 01
  ( (?!<)                                   # 02
    (?: (["']) (?:(?!\2|\\).|\\.)* \2       # 03
    |   (?!\ +<).                           # 04
    )+                                      # 05
  )?                                        # 06
  (?: \ *<                                  # 07
      (.+?)?                                # 08
      >?                                    # 09
  )?                                        # 10
$                                           # 11
""", re.VERBOSE)


@dataclasses.dataclass(eq=True, frozen=True)
class Ident:
    """
    A container for identity attributes, used by the author or
    maintainer fields.
    """
    name: str|None
    email: str|None

    def __iter__(self):
        return (getattr(self, field.name) for
                field in dataclasses.fields(self))


class Message(email.message.Message):
    multiple_use_keys = set(
        map(
            FoldedCase,
            [
                'Classifier',
                'Obsoletes-Dist',
                'Platform',
                'Project-URL',
                'Provides-Dist',
                'Provides-Extra',
                'Requires-Dist',
                'Requires-External',
                'Supported-Platform',
                'Dynamic',
            ],
        )
    )
    """
    Keys that may be indicated multiple times per PEP 566.
    """

    def __new__(cls, orig: email.message.Message):
        res = super().__new__(cls)
        vars(res).update(vars(orig))
        return res

    def __init__(self, *args, **kwargs):
        self._headers = self._repair_headers()

    # suppress spurious error from mypy
    def __iter__(self):
        return super().__iter__()

    def __getitem__(self, item):
        """
        Warn users that a ``KeyError`` can be expected when a
        mising key is supplied. Ref python/importlib_metadata#371.
        """
        res = super().__getitem__(item)
        if res is None:
            _warn()
        return res

    def _repair_headers(self):
        def redent(value):
            "Correct for RFC822 indentation"
            if not value or '\n' not in value:
                return value
            return textwrap.dedent(' ' * 8 + value)

        headers = [(key, redent(value)) for key, value in vars(self)['_headers']]
        if self._payload:
            headers.append(('Description', self.get_payload()))
        return headers

    @property
    def json(self):
        """
        Convert PackageMetadata to a JSON-compatible format
        per PEP 0566.
        """

        def transform(key):
            value = self.get_all(key) if key in self.multiple_use_keys else self[key]
            if key == 'Keywords':
                value = re.split(r'\s+', value)
            tk = key.lower().replace('-', '_')
            return tk, value

        return dict(map(transform, map(FoldedCase, self)))

    def _parse_idents(self, s):
        es = (i[0] for i in _entries.findall(s))
        es = (_name_email.match(i)[::2] for i in es)
        es = {Ident(*i) for i in es if i != (None, None)}
        return es

    def _parse_names(self, s):
        es = (i[0] for i in _entries.findall(s))
        es = {Ident(i, None) for i in es}
        return es

    def _parse_names_idents(self, fn, fi):
        sn = self.get(fn, "")
        si = self.get(fi, "")
        return self._parse_names(sn) | self._parse_idents(si)

    @property
    def authors(self):
        """
        Minimal parsing for "Author" and "Author-email" fields.
        """
        return self._parse_names_idents("Author", "Author-email")

    @property
    def maintainers(self):
        """
        Minimal parsing for "Maintainer" and "Maintainer-email" fields.
        """
        return self._parse_names_idents("Maintainer", "Maintainer-email")
