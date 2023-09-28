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

# The formatting of the identity fields ("Author", "Maintainer", "Author-email"
# and "Maintainer-email") in the core metadata specification and related
# 'pyproject.toml' specification is inspired by RFC5233 but not precisely
# defined. In practice conflicting definitions are used by many packages and
# even examples from the specification. For a permissive parser the key
# takeaway from RFC5233 is that special characters such as "," and "@" must be
# quoted when used as text.


def _entries_findall(string):
    """
    Return a list of entries given an RFC5233-inspired string. Entries are
    separated by ", " and contents of quoted strings are ignored. Each
    entry will be a non-empty string.

    >>> _entries_findall('a, b ,  c, "d, e, f"')
    ['a', 'b ', ' c', '"d, e, f"']

    >>> _entries_findall('a')
    ['a']

    >>> _entries_findall('')
    []

    >>> _entries_findall(", ")
    []
    """

    # Split an RFC5233-ish list:
    # 1. Require a list separator, or beginning-of-string.
    # 2. Alt 1: match single or double quotes and handle escape characters.
    # 3. Alt 2: match anything except ',' followed by a space. If quote
    #    characters are unbalanced, they will be matched here.
    # 4. Match the alternatives at least once, in any order...
    # 5. ... and capture them.
    # Result:
    #   group 1 (list entry): None or non-empty string.
    _entries = re.compile(
        r"""
        (?: (?<=,\ ) | (?<=^) )                 # 1
        ( (?: (["']) (?:(?!\2|\\).|\\.)* \2     # 2
          |   (?!,\ ).                          # 3
          )+                                    # 4
        )                                       # 5
        """,
        re.VERBOSE,
    )

    return [entry[0] for entry in _entries.findall(string)]


def _name_email_split(string):
    """
    Split an RFC5233-inspired entry into a name and email address tuple. Each
    component will be either None or a non-empty string. Split the form "name
    local@domain" on the first unquoted "@" such that:

    * local may not be empty and may not contain any unquoted spaces
    * domain may not be empty
    * spaces between name and address are consumed
    * space between name and address is optional if name ends in "@"
    * first opening "<" of local is consumed only if local remains non-empty
    * last closing ">" of domain is consumed only if domain remains non-empty

    >>> _name_email_split("name local@domain")
    ('name', 'local@domain')

    >>> _name_email_split('@"unlocal@undomain" @    <loc"al@dom\\'ain')
    ('@"unlocal@undomain" @', 'loc"al@dom\\'ain')

    >>> _name_email_split('@@ local@domain')
    ('@@', 'local@domain')

    >>> _name_email_split('@nameonly@')
    ('@nameonly@', None)

    >>> _name_email_split('@domain@ ')
    ('@', 'domain@ ')

    >>> _name_email_split('   domain@only')
    (None, 'domain@only')

    >>> _name_email_split('   ')
    ('   ', None)

    >>> _name_email_split('')
    (None, None)
    """

    # Split an RFC5233-inspired name-address entry:
    # 01. Start at the beginning.
    # 02. Capture at least one name component, but optionally so the result
    #     will be 'None' rather than an empty string.
    # 03. Stop matching against name components if the lookahead matches an
    #     address. An address can be preceded by spaces, which are optional if
    #     the name is missing.
    # 04. Simulate a possessive quantifier for Python < 3.11 given the
    #     equivalence between "(...)++" and "(?=( (...)+ ))\1". The contained
    #     alternatives are not exclusive and the possessive quantifier prevents
    #     the second alternative from stealing quoted components during
    #     backtracking.
    # 05. Alt 1.1: Match single-quoted or double-quoted components and handle
    #     escape characters.
    # 06. Alt 1.2: Match any character except the local component delimiters
    #     " " or "@". If quote characters are unbalanced, they will be matched
    #     here.
    # 07. Match the alternatives at least once - the local part of the address
    #     cannot be empty.
    # 08. (See 04)
    # 09. Match "@" followed by something - the domain cannot be empty either.
    # 10. (See 03)
    # 11. Alt 2.1: Match a quoted component...
    # 12. Alt 2.2: ... or match a single character.
    # ...
    # 14. (See 02)
    # 15. (See 02)
    # 16. If the name portion is missing or ends with an "@", there may or may
    #     not be whitespace before the address. The opening angle bracket is
    #     always optional.
    # ...
    # 20. Match everything after "@" with a non-greedy quantifier to allow for
    #     the optional closing angle bracket.
    # 21. Allow for no address component.
    # 22. Match the optional closing angle bracket.
    # 23. Finish at the end.
    # Summary:
    #   ^ ( ( not: space* (quote | not:space-or-at)++ @ anything
    #         quote | anything
    #       )+
    #     )?
    #   space* <? ( (quote | not:space-or-at)+ @ anything+? )? >? $
    # Result:
    #   group 1 (name):  None or non-empty string.
    #   group 5 (email): None or non-empty string.
    _name_email = re.compile(
        r"""
        ^                                                       # 01
          ( (?:                                                 # 02
                (?! \ *                                         # 03
                    (?=(                                        # 04
                       (?: (["']) (?:(?!\3|\\).|\\.)* \3        # 05
                       |   [^ @]                                # 06
                       )+                                       # 07
                    ))\2                                        # 08
                    @ .                                         # 09
                )                                               # 10
                (?: (["']) (?:(?!\4|\\).|\\.)* \4               # 11
                |   .                                           # 12
                )                                               # 13
            )+                                                  # 14
          )?                                                    # 15
          \ * <?                                                # 16
          ( (?: (["']) (?:(?!\6|\\).|\\.)* \6                   # 17
            |   [^ @]                                           # 18
            )+                                                  # 19
            @ .+?                                               # 20
          )?                                                    # 21
          >?                                                    # 22
        $                                                       # 23
        """,
        re.VERBOSE,
    )

    # Equivalent, simpler, version using possessive quantifiers, for
    # Python >= 3.11.
    # _name_email = re.compile(
    #     r"""
    #     ^ ( (?: (?! \ *
    #                 (?: (["']) (?:(?!\2|\\).|\\.)* \2
    #                 |   [^ @]
    #                 )++
    #                 @ .
    #             )
    #             (?: (["']) (?:(?!\3|\\).|\\.)* \3
    #             |   .
    #             )
    #         )+
    #       )?
    #       \ * <?
    #       ( (?: (["']) (?:(?!\5|\\).|\\.)* \5
    #         |   [^ @]
    #         )+
    #         @ .+?
    #       )?
    #       >?
    #     $
    #     """,
    #     re.VERBOSE,
    # )

    return _name_email.match(string).groups()[::4]


def _uniq(values):
    """
    Return a list omitting duplicate values.

    >>> _uniq([1, 2, 1, 2, 3, 1, 4])
    [1, 2, 3, 4]

    >>> _uniq(())
    []
    """
    unique = set()
    result = []
    for value in values:
        if value in unique:
            continue
        unique.add(value)
        result.append(value)
    return result


@dataclasses.dataclass(eq=True, frozen=True)
class Ident:
    """
    A container for identity attributes, used by the author or
    maintainer fields.
    """

    name: str | None
    email: str | None

    def __iter__(self):
        return (getattr(self, field.name) for field in dataclasses.fields(self))


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

    def _parse_idents(self, string):
        entries = (_name_email_split(entry) for entry in _entries_findall(string))
        return _uniq(Ident(*entry) for entry in entries if entry != (None, None))

    def _parse_names(self, string):
        return _uniq(Ident(entry, None) for entry in _entries_findall(string))

    def _parse_names_idents(self, names_field, idents_field):
        names = self._parse_names(self.get(names_field, ""))
        idents = self._parse_idents(self.get(idents_field, ""))
        return _uniq((*names, *idents))

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
