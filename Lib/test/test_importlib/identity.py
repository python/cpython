"""
Provides ``identities_strategy``, a Hypothesis strategy for generating an
underlying list of identity ``Entries`` with corresponding structured and
unstructured identity metadata.
"""

import re
import types
import typing
import itertools
import dataclasses
import collections.abc

from test.support.hypothesis_helper import hypothesis

st = hypothesis.strategies


class OrderedSet(collections.abc.MutableSet):
    """
    Represent an ordered set using a dictionary attribute ``data``.
    """

    def __init__(self, values=[]):
        self.data = {value: None for value in values}

    def __contains__(self, value):
        return value in self.data

    def __iter__(self):
        return iter(self.data)

    def __len__(self):
        return len(self.data)

    def add(self, value):
        self.data[value] = None

    def discard(self, value):
        del self.data[value]

    def __repr__(self):
        data = ", ".join(repr(value) for value in self)
        return f"{type(self).__name__}([{data}])"

    def __eq__(self, other):
        try:
            return tuple(self) == tuple(other)
        except TypeError:
            return NotImplemented


@dataclasses.dataclass
class Token:
    """
    Represents a generated identity component. See ``Entry`` for more
    information.
    """

    category: typing.Any
    value: str
    delim: bool = False
    ignore: bool = False


class Entry(list):
    """
    Represent an identity of the form ``(name, email)`` as a list of
    ``Token`` instances. Delimiter (``delim``) tokens separate adjacent
    entries or name-email components. Tokens with ``delim`` or ``ignore``
    have their value omitted from the structured representation.
    """

    @classmethod
    def from_empty(cls, length, category, value=""):
        """
        Generate an empty identity with ``length`` components - usually
        two, representing name and email - and ``length - 1`` interleaved
        delimiters.
        """
        tokens = []
        for index in range(0, length):
            if index > 0:
                tokens.append(Token(category, value, True))
            tokens.append(Token(category, value, False))
        return cls(tokens)

    def __repr__(self):
        return type(self).__name__ + "(" + super().__repr__() + ")"

    @property
    def as_text(self):
        """
        Convert to an unstructured textual format - a string. No values
        are omitted.
        """
        return "".join(token.value for token in self)

    @property
    def as_data(self):
        """
        Convert to a structured format - a tuple, split on groups of
        ``delim`` tokens. Values from ``delim`` and ``ignore`` tokens
        are omitted. Empty components are converted to ``None``.
        """

        def keyfunc(token):
            return token.delim

        data = []
        for key, group in itertools.groupby(self, key=keyfunc):
            if key:
                continue
            text = "".join(token.value for token in group if not token.ignore)
            data.append(text if text else None)
        return tuple(data)


def replace(seq, index, item):
    """Return a new sequence such that ``seq[index] == item``."""
    if not 0 <= index < len(seq):
        raise IndexError
    return seq[:index] + item + seq[index + 1 :]


def characters_exclude(exclude_chars, simple_chars=False):
    """
    A characters strategy built on the baseline required by
    ``identities_strategy``. Excludes ``exclude_chars``, and if
    ``simple_chars``, all "Other" unicode code points.
    """
    return st.characters(
        # if simple_chars, smaller search space and easier visualization
        exclude_categories=("C",) if simple_chars else ("Cs", "Co", "Cn"),
        exclude_characters=exclude_chars +
        # universal newlines: not compatible with toolchain
        "\n\r\v\f\x1c\x1d\x1e\x85\u2028\u2029"
        # backspace: side-effects
        "\x08",
    )


def text_exclude(exclude_chars="", min_size=0, max_size=None, simple_chars=False):
    """A text strategy that uses ``characters_exclude`` as alphabet."""
    return st.text(
        alphabet=characters_exclude(exclude_chars, simple_chars=simple_chars),
        min_size=min_size,
        max_size=max_size,
    )


def generate_draw_function(draw, strategy):
    return lambda _: draw(strategy)


@st.composite
def merge_sub(draw, state, sample, category, sub_func=None, **kwargs):
    """
    Given a ``sample`` list of tokens, for adjacent tokens of the
    given ``category`` merge and then apply the substitution function
    ``sub_func`` with ``kwargs``.
    """

    def keyfunc(token):
        return dataclasses.replace(token, value=None)

    merged = []
    for key, group in itertools.groupby(sample, key=keyfunc):
        if key.category is category:
            key.value = "".join(token.value for token in group)
            merged.append(key)
        merged.extend(group)

    if not sub_func:
        return merged

    for i, token in enumerate(merged):
        opener = i == 0
        closer = i == len(merged) - 1
        if token.category is not category:
            continue
        token.value = draw(sub_func(state, token.value, opener, closer, **kwargs))

    return merged


@st.composite
def sub_entry(draw, state, text, opener, closer):
    """An entry cannot contain ", "."""
    repl_chars = characters_exclude("\"', ", simple_chars=state.simple_chars)
    repl_func = generate_draw_function(draw, repl_chars)
    text = re.sub(r"(?<=,) ", repl_func, text)
    return text


@st.composite
def sub_name(draw, state, text, opener, closer, name_only):
    """
    A name entry cannot:

    * contain [^ ]@.
    * contain ", "
    * end with "," or " "
    """
    repl_chars = characters_exclude("\"', @", simple_chars=state.simple_chars)
    repl_func = generate_draw_function(draw, repl_chars)
    # "@" is legal if:
    # * nothing or space before
    # -- or --
    # * nothing after
    #
    # Inversely (a or b -> -a and -b):
    # 1. not space before
    # -- and --
    # 2. anything after
    #
    # The inverse equivalent:
    # [^ ]@.
    escape_at = re.compile(
        rf"""
        (?: [^ ] {"| ^" if not opener else ""})
        (?= @. {"| @$" if not closer or not name_only else ""})
        """,
        re.VERBOSE,
    )
    text = escape_at.sub(" ", text)
    # ", t" -> replace " "
    text = re.sub(r"(?<=,) (?!@)", repl_func, text)
    # ", @" -> replace ","
    text = re.sub(r",(?= @)", repl_func, text)
    if closer and not name_only:
        text = re.sub(r"(?<=[, ]$)", repl_func, text)
    return text


def get_name_qchar_idxs(text, succeeded):
    """
    Given any legal input (ie, not matching "[^ ]@."), yield the
    character indexes for which the text remains valid when the
    index is substituted with a quote character.
    """
    replace_at = re.compile(
        rf"""
        # Cannot replace " " before "@" if "@" succeeded by anything.
        (?! $ | [ ]@. {"| [ ]@$" if succeeded else ""})
        """,
        re.VERBOSE,
    )
    return (m.start() for m in replace_at.finditer(text))


@st.composite
def identities_strategy(draw, simple_chars=True, debug_entries=False):
    """
    For each identity field of the core metadata specification ("Author",
    "Maintainer", "Author-email" and "Maintainer-email"), generate a list
    of ``Entry`` instances, each corresponding to a possibly-empty
    identity. The "-email" fields have stricter requirements and generate
    from ``ident_entry`` whereas the non-email fields use ``name_entry``.
    Empty lists are dropped. Process and combine each non-empty list into
    an unstructured string conforming to the core metadata specification
    as well as into a more succinct structured format - two lists of
    (name, email) tuples, one for authors and the other for maintainers.

    If ``simple_chars``, restrict the list of allowed characters to
    reduce the Hypothesis search space and permit easier visualization.
    If ``debug_entries``, return the underlying lists of ``Entry``
    instances in addition to the unstructured string and structured list
    representations.

    This strategy will generate the most permissible identities possible:
    unbalanced quote characters, unescaped special characters, omitted
    identity information, empty identities, and so forth.
    """
    text = ""
    authors = OrderedSet()
    maintainers = OrderedSet()
    entry_dict = dict()
    state = types.SimpleNamespace(simple_chars=simple_chars)

    combinations = zip(
        ((i, j) for i in (name_entry, ident_entry) for j in (authors, maintainers)),
        ("Author", "Maintainer", "Author-email", "Maintainer-email"),
    )

    for (entry_type, entry_target), entry_prefix in combinations:
        entries, data_set, data_str = draw(identity_entries(state, entry_type))
        entry_dict[entry_prefix] = entries
        if not entries:
            continue
        text += entry_prefix + ": " + data_str + "\n"
        entry_target |= data_set

    result = (text, authors, maintainers)
    if debug_entries:
        result = (entry_dict, *result)
    return result


@st.composite
def identity_entries(draw, state, entry_strategy):
    """
    Generate a list of processed identity entries using
    ``entry_strategy``.
    """
    sample = draw(st.lists(entry_strategy(state), min_size=0, max_size=10))
    for entry in sample[:-1]:
        entry.append(Token(identity_entries, ", ", True))
    return process(lstrip(draw(unbalance(sample))))


def process(entries):
    """
    Process a list of entries into structured and unstructured
    formats. Filter empty (``None``) entries from the structured
    format. Convert token categories to strings for simpler
    visualization.
    """
    text = ""
    data = OrderedSet()

    for entry in entries:
        entry_text = entry.as_text
        entry_data = entry.as_data
        for token in entry:
            token.category = token.category.__name__
        text += entry_text
        if all(elem is None for elem in entry_data):
            continue
        data.add(entry_data)

    return (entries, data, text)


def lstrip(entries):
    """
    The PackageMetadata implementation removes leading whitespace
    from the first entry, so we do the same. This should not alter
    the parsing of the entry. Return possibly-mutated ``entries``.
    """
    entry = entries[0] if entries else []
    for token in entry:
        token.value = token.value.lstrip()
        if token.value:
            break
    return entries


def unbalance_indexes(entries):
    """Return a list of valid replacement indices for quote characters."""
    index_candidates = []
    type_candidates = [
        ident_addr_domain_other,
        ident_addr_local_other,
        ident_name_other,
        name_value_other,
    ]
    stop = False
    i = len(entries) - 1
    while i >= 0 and not stop:
        j = len(entries[i]) - 1
        while j >= 0 and not (stop := entries[i][j].category is quote):
            token = entries[i][j]
            if token.category not in type_candidates:
                repls = ()
            elif token.category is ident_name_other:
                final = entries[i][j + 1] is ident_name_only
                repls = get_name_qchar_idxs(token.value, not final)
            else:
                repls = range(len(token.value))
            for k in repls:
                index_candidates.append((i, j, k))
            j -= 1
        i -= 1
    return index_candidates


@st.composite
def unbalance(draw, entries):
    """
    Unbalance quote characters in the list of entries without altering
    the parsing of the list. Mutate if possible, returning ``entries``.
    """
    qchrs = draw(st.sets(st.sampled_from("\"'")))
    index_candidates = unbalance_indexes(entries)
    if not qchrs or not index_candidates:
        return entries
    for qchr in sorted(qchrs):
        idx_entry, idx_token, idx_char = draw(st.sampled_from(index_candidates))
        token = entries[idx_entry][idx_token]
        token.value = replace(token.value, idx_char, qchr)
    return entries


# Non-leaf rules for "Author" and "Maintainer" fields:


def name_entry(state):
    return name_empty() | name_value(state)


def name_empty():
    return st.builds(Entry.from_empty, st.just(2), st.just(name_empty))


@st.composite
def name_value(draw, state):
    choice = name_value_other(state) | quote(state)
    sample = draw(st.lists(choice, min_size=1, max_size=10))
    entry = Entry.from_empty(2, name_value)
    entry[:1] = draw(merge_sub(state, sample, name_value_other, sub_entry))
    return entry


# Non-leaf rules for "Author-email" and "Maintainer-email" fields:


def ident_entry(state):
    return (
        ident_empty()
        | ident_name_only(state)
        | ident_addr_only(state)
        | ident_name_addr(state)
    )


def ident_empty():
    return st.builds(Entry.from_empty, st.just(2), st.just(ident_empty))


@st.composite
def ident_name_only(draw, state):
    entry = Entry.from_empty(2, ident_name_only)
    entry[:1] = draw(ident_name(state, name_only=True))
    return entry


@st.composite
def ident_addr_only(draw, state):
    entry = Entry.from_empty(2, ident_addr_only)
    addr = draw(ident_addr(state))
    if draw(st.booleans()):
        entry[1:] = [draw(ident_name_addr_delim("")), *addr]
    else:
        entry[2:] = addr
    return entry


@st.composite
def ident_name_addr(draw, state):
    name = draw(ident_name(state, name_only=False))
    delim = draw(ident_name_addr_delim(name[-1].value))
    addr = draw(ident_addr(state))
    return Entry((*name, delim, *addr))


@st.composite
def ident_name_addr_delim(draw, name):
    min_size = 0 if name.endswith("@") else 1
    delim = draw(st.text(alphabet=" ", min_size=min_size, max_size=10))
    return Token(ident_name_addr_delim, delim, True)


@st.composite
def ident_name(draw, state, name_only):
    choice = ident_name_other(state) | quote(state)
    sample = draw(st.lists(choice, min_size=1, max_size=10))
    return draw(
        merge_sub(state, sample, ident_name_other, sub_name, name_only=name_only)
    )


@st.composite
def ident_addr(draw, state):
    local = draw(ident_addr_local(state))
    domain = draw(ident_addr_domain(state))
    # affix possibly duplicate leading/trailing angle brackets
    if draw(st.booleans()):
        local = [Token(ident_addr, "<"), *local]
    if draw(st.booleans()):
        domain = [*domain, Token(ident_addr, ">")]
    # split any coupled leading/trailing angle brackets
    value = local[0].value
    if value.startswith("<") and len(value) > 1:
        split = dataclasses.replace(local[0], value="<", category=ident_addr)
        local[0].value = value[1:]
        local = [split, *local]
    value = domain[-1].value
    if value.endswith(">") and len(value) > 1:
        split = dataclasses.replace(domain[-1], value=">", category=ident_addr)
        domain[-1].value = value[:-1]
        domain = [*domain, split]
    # if not solitary, ignore single leading/trailing angle brackets
    if len(local) > 1 and local[0].value == "<":
        local[0].ignore = True
    if len(domain) > 1 and domain[-1].value == ">":
        domain[-1].ignore = True
    # combine the results
    result = [*local, Token(ident_addr, "@"), *domain]
    return result


@st.composite
def ident_addr_local(draw, state):
    choice = ident_addr_local_other(state) | quote(state)
    sample = draw(st.lists(choice, min_size=1, max_size=10))
    return draw(merge_sub(state, sample, ident_addr_local_other))


@st.composite
def ident_addr_domain(draw, state):
    choice = ident_addr_domain_other(state) | quote(state)
    sample = draw(st.lists(choice, min_size=1, max_size=10))
    return draw(merge_sub(state, sample, ident_addr_domain_other, sub_entry))


# Leaf rules:


@st.composite
def quote(draw, state):
    qchr = draw(st.sampled_from("'\""))
    escape_trail = re.compile(r"(?<!\\)((?:\\.)*)(\\)$")
    escape_qchar = re.compile(rf"(?<!\\)((?:\\.)*)({qchr})")
    text = draw(text_exclude(min_size=0, max_size=20, simple_chars=state.simple_chars))
    text = escape_trail.sub(r"\1\\\2", text)
    text = escape_qchar.sub(r"\1\\\2", text)
    text = qchr + text + qchr
    return Token(quote, text)


def ident_name_other(state):
    text = text_exclude("\"'", min_size=1, max_size=20, simple_chars=state.simple_chars)
    return st.builds(Token, st.just(ident_name_other), text)


def ident_addr_local_other(state):
    text = text_exclude(
        "\"' @", min_size=1, max_size=20, simple_chars=state.simple_chars
    )
    return st.builds(Token, st.just(ident_addr_local_other), text)


def ident_addr_domain_other(state):
    text = text_exclude("\"'", min_size=1, max_size=20, simple_chars=state.simple_chars)
    return st.builds(Token, st.just(ident_addr_domain_other), text)


def name_value_other(state):
    text = text_exclude("\"'", min_size=1, max_size=20, simple_chars=state.simple_chars)
    return st.builds(Token, st.just(name_value_other), text)
