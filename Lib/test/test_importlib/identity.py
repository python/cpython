import re
import typing
import itertools
import dataclasses

from test.support.hypothesis_helper import hypothesis


@dataclasses.dataclass
class Token:
    category: typing.Any
    value: str
    delim: bool = False
    ignore: bool = False


class Entry(list):
    @classmethod
    def from_empty(cls, length, category, value=""):
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
        return "".join(token.value for token in self)

    @property
    def as_data(self):
        data = []
        keyfunc = lambda token: token.delim
        for key, group in itertools.groupby(self, key=keyfunc):
            if key:
                continue
            text = "".join\
                (token.value for token in group if not token.ignore)
            data.append(text if text else None)
        return tuple(data)


@hypothesis.strategies.composite
def identities_strategy(draw, debug=False):
    strategies = hypothesis.strategies
    random = draw(strategies.randoms())
    char_opts =\
        { "blacklist_categories":
            # if debug, smaller search space and easier visualization
            ("C",) if debug else ("Cs", "Co", "Cn")
        , "blacklist_characters":
            # universal newlines: not compatible with toolchain
            "\n\r\v\f\x1c\x1d\x1e\x85\u2028\u2029"
            # backspace: side-effects
            "\x08"
        }

    def merge_char_opts(opts1, opts2):
        def resolve_sum(iter1, iter2, apply):
            return apply(itertools.chain(iter1 or (), iter2 or ())) or None

        def resolve_key(arg1, arg2, apply):
            return apply(arg1, arg2, key=lambda i: 0 if i is None else i)

        conflicts =\
            [ ("min_codepoint", resolve_key, max)
            , ("max_codepoint", resolve_key, min)
            , ("whitelist_categories", resolve_sum, list)
            , ("blacklist_categories", resolve_sum, list)
            , ("whitelist_characters", resolve_sum, "".join)
            , ("blacklist_characters", resolve_sum, "".join)
            ]

        opts = opts1 | opts2
        for key, resolver, *args in conflicts:
            if key not in opts1 or key not in opts2:
                continue
            opts[key] = resolver(opts1[key], opts2[key], *args)

        return opts

    def merge_char_kwargs(opts1, **kwargs):
        return merge_char_opts(opts1, kwargs)

    def choices_bernoulli(population, weights, failure_weight, min_size=0):
        # We'd have to sample the negative binomial distribution to
        # determine 'k' from the probability 'failure_weight', so
        # instead we simulate a bernoulli process which stops at the
        # first failure.
        fail = object()
        results = []
        while True:
            choice = random.choices\
                ((*population, fail), (*weights, failure_weight), k=1)[0]
            if choice is fail and len(results) < min_size:
                continue
            if choice is fail:
                break
            results.append(choice)
        return results

    def choose_bool(true=50, false=50):
        return random.choices\
            ((True, False), (true, false))[0]

    def replace(seq, index, item):
        return seq[:index] + item + seq[index+1:]

    def generate_blacklist_chars(blacklist_chars):
        return strategies.characters(**merge_char_kwargs
            (char_opts, blacklist_characters=blacklist_chars))

    def generate_repl_func(strategy):
        return lambda _: draw(strategy)

    def generate_text(blacklist_chars="", min_size=0, max_size=None):
        text_chars = generate_blacklist_chars(blacklist_chars)
        text = draw(strategies.text
            (alphabet=text_chars, min_size=min_size, max_size=max_size))
        return text

    def apply_merge(functions, category, sub_func=None, **kwargs):
        applied = (function() for function in functions)
        keyfunc = lambda token: dataclasses.replace(token, value=None)
        merged = []
        for key, group in itertools.groupby(applied, key=keyfunc):
            if key.category is category:
                key.value = "".join(token.value for token in group)
                merged.append(key)
            merged.extend(group)

        if not sub_func:
            return merged

        for i, token in enumerate(merged):
            opener = (i == 0)
            closer = (i == len(merged) - 1)
            if token.category is not category:
                continue
            token.value = sub_func(token.value, opener, closer, **kwargs)

        return merged

    def sub_entry(text, opener, closer):
        repl_chars = generate_blacklist_chars("\"', ")
        repl_func = generate_repl_func(repl_chars)
        text = re.sub(r"(?<=,) ", repl_func, text)
        return text

    def sub_name(text, opener, closer, name_only):
        repl_chars = generate_blacklist_chars("\"', @")
        repl_func = generate_repl_func(repl_chars)
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
        escape_at = re.compile(rf"""
            (?: [^ ] {"| ^" if not opener else ""})
            (?= @. {"| @$" if not closer or not name_only else ""})
            """, re.VERBOSE)
        text = escape_at.sub(" ", text)
        # ", t" -> replace " "
        text = re.sub(r"(?<=,) (?!@)", repl_func, text)
        # ", @" -> replace ","
        text = re.sub(r",(?= @)", repl_func, text)
        if closer and not name_only:
            text = re.sub(r"(?<=[, ]$)", repl_func, text)
        return text

    def get_name_qchar_idxs(text, succeeded):
        # Given any legal input (ie, not matching "[^ ]@."), yield the
        # character indexes for which the text remains valid when the
        # index is substituted with a quote character.
        replace_at = re.compile(rf"""
            # Cannot replace " " before "@" if "@" succeeded by anything.
            (?! $ | [ ]@. {"| [ ]@$" if succeeded else ""})
            """, re.VERBOSE)
        return (m.start() for m in replace_at.finditer(text))

    def entries_combined(debug=False):
        text = ""
        authors = set()
        maintainers = set()
        entry_dict = dict()

        entry_types = (name_entry, ident_entry)
        entry_targets = (authors, maintainers)
        entry_prefixes =\
            ( "Author", "Maintainer"
            , "Author-email", "Maintainer-email"
            )
        combinations = zip\
            ( ((i,j) for i in entry_types for j in entry_targets)
            , entry_prefixes
            )

        for (entry_type, entry_target), entry_prefix in combinations:
            entry_list, data_set, data_str = entries(entry_type)
            entry_dict[entry_prefix] = entry_list
            if not entry_list:
                continue
            text += entry_prefix + ": " + data_str + "\n"
            entry_target |= data_set

        result = (text, authors, maintainers)
        if debug:
            result = (entry_dict, *result)
        return result

    def entries(entry_function):
        count = draw(strategies.integers(min_value=0, max_value=20))
        entry_list = []
        for i in range(count):
            entry = entry_function()
            if i < count - 1:
                entry.append(Token(entries, ", ", True))
            entry_list.append(entry)
        unbalance(entry_list)
        lstrip(entry_list)
        return process(entry_list)

    def process(entries):
        text = ""
        data = set()

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
        # The PackageMetadata implementation removes leading
        # whitespace from the first entry, so we do the same.
        # This should not alter the parsing of the entry.
        entry = entries[0] if entries else []
        strip = False
        for token in entry:
            value = token.value.lstrip()
            strip = strip or token.value != value
            token.value = value
            if token.value:
                break
        return strip

    def unbalance(entries):
        index_candidates = []
        type_candidates =\
            [ ident_addr_domain_other
            , ident_addr_local_other
            , ident_name_other
            , name_value_other
            ]
        stop = False
        i = len(entries) - 1
        while i >= 0 and not stop:
            j = len(entries[i]) - 1
            while j >= 0 and not (stop:=entries[i][j].category is quote):
                token = entries[i][j]
                if token.category not in type_candidates:
                    repls = ()
                elif token.category is ident_name_other:
                    final = entries[i][j+1] is ident_name_only
                    repls = get_name_qchar_idxs(token.value, not final)
                else:
                    repls = range(len(token.value))
                for k in repls:
                    index_candidates.append((i,j,k))
                j -= 1
            i -= 1
        if not index_candidates:
            return False
        for qchr in "\"'":
            if choose_bool():
                continue
            idx_entry, idx_token, idx_char =\
                random.choice(index_candidates)
            token = entries[idx_entry][idx_token]
            token.value = replace(token.value, idx_char, qchr)
        return True

    def name_entry():
        return random.choices\
            ( (name_empty, name_value), (10, 90)
            )[0]()

    def name_empty():
        return Entry.from_empty(2, name_empty)

    def name_value():
        functions = choices_bernoulli\
            ( (quote, name_value_other)
            , (25, 75), 10, min_size=1
            )
        entry = Entry.from_empty(2, name_value)
        entry[:1] = apply_merge(functions, name_value_other, sub_entry)
        return entry

    def ident_entry():
        return random.choices\
            ( ( ident_empty
              , ident_name_only
              , ident_addr_only
              , ident_name_addr
              )
            , (10, 20, 20, 50)
            )[0]()

    def ident_empty():
        return Entry.from_empty(2, ident_empty)

    def ident_name_only():
        entry = Entry.from_empty(2, ident_name_only)
        entry[:1] = ident_name(name_only=True)
        return entry

    def ident_addr_only():
        entry = Entry.from_empty(2, ident_addr_only)
        addr = ident_addr()
        if choose_bool(true=10, false=90):
            delim = ident_name_addr_delim("")
            addr = [delim, *addr]
        entry[2:] = addr
        return entry

    def ident_name_addr():
        name = ident_name(name_only=False)
        delim = ident_name_addr_delim(name[-1].value)
        addr = ident_addr()
        return Entry((*name, delim, *addr))

    def ident_name_addr_delim(name):
        min_size = 0 if name.endswith("@") else 1
        delim = draw(strategies.text(alphabet=" ", min_size=min_size))
        delim = Token(ident_name_addr_delim, delim, True)
        return delim

    def ident_name(name_only):
        functions = choices_bernoulli\
            ( (quote, ident_name_other)
            , (25, 75), 10, min_size=1
            )
        return apply_merge\
            (functions, ident_name_other, sub_name, name_only=name_only)

    def ident_addr():
        local = ident_addr_local()
        domain = ident_addr_domain()
        # affix possibly duplicate leading/trailing angle brackets
        true_false = [20, 80]
        if choose_bool(*true_false):
            local = [Token(ident_addr, "<"), *local]
            true_false = [40, 60]
        if choose_bool(*true_false):
            domain = [*domain, Token(ident_addr, ">")]
        # split any coupled leading/trailing angle brackets
        value = local[0].value
        if value.startswith("<") and len(value) > 1:
            split = dataclasses.replace\
                (local[0], value="<", category=ident_addr)
            local[0].value = value[1:]
            local = [split, *local]
        value = domain[-1].value
        if value.endswith(">") and len(value) > 1:
            split = dataclasses.replace\
                (domain[-1], value=">", category=ident_addr)
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

    def ident_addr_local():
        functions = choices_bernoulli\
            ( (quote, ident_addr_local_other)
            , (25, 75), 10, min_size=1
            )
        return apply_merge(functions, ident_addr_local_other)

    def ident_addr_domain():
        functions = choices_bernoulli\
            ( (quote, ident_addr_domain_other)
            , (25, 75), 10, min_size=1
            )
        return apply_merge(functions, ident_addr_domain_other, sub_entry)

    def quote():
        qchr = draw(strategies.sampled_from("'\""))
        escape_trail = re.compile(r"(?<!\\)((?:\\.)*)(\\)$")
        escape_qchar = re.compile(rf"(?<!\\)((?:\\.)*)({qchr})")
        text_chars = strategies.characters(**char_opts)
        text = draw(strategies.text(alphabet=text_chars, min_size=0))
        text = escape_trail.sub(r"\1\\\2", text)
        text = escape_qchar.sub(r"\1\\\2", text)
        text = qchr + text + qchr
        return Token(quote, text)

    def ident_name_other():
        text = generate_text("\"'", min_size=1, max_size=20)
        return Token(ident_name_other, text)

    def ident_addr_local_other():
        text = generate_text("\"' @", min_size=1, max_size=20)
        return Token(ident_addr_local_other, text)

    def ident_addr_domain_other():
        text = generate_text("\"'", min_size=1, max_size=20)
        return Token(ident_addr_domain_other, text)

    def name_value_other():
        text = generate_text("\"'", min_size=1, max_size=20)
        return Token(name_value_other, text)

    return entries_combined(debug=debug)
