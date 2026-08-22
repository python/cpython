import datetime
import unittest
from test.support.hypothesis_helper import hypothesis

st = hypothesis.strategies


class FormatSet:
    def __init__(self, children, must_use=False):
        if children is None:
            self.children = ()
        else:
            if must_use:
                self.children = tuple(children)
            else:
                self.children = (*children, NoComponent)

    def __repr__(self):
        import textwrap

        children = textwrap.indent(str(self.children), prefix="    ")
        return f"{self.__class__.__name__}(\n{children}\n)"

    def replace_children(self, children):
        return type(self)(children, must_use=True)


class OneFormatFrom(FormatSet):
    pass


class EachFormatFrom(FormatSet):
    pass


class NoComponentClass:
    def __repr__(self):
        return "NoComponent()"


NoComponent = NoComponentClass()

# Composite dates
DATE_FORMAT_CODES = OneFormatFrom(
    (
        EachFormatFrom(
            (
                EachFormatFrom(
                    (
                        OneFormatFrom(("%Y", "%y")),
                        OneFormatFrom(
                            (
                                # Month and Day
                                EachFormatFrom(
                                    (
                                        OneFormatFrom(("%m", "%B", "%b")),
                                        OneFormatFrom(("%d",)),
                                    )
                                ),
                                # Julian day of year
                                OneFormatFrom(("%j",)),
                                # Week number and day of week (Sunday = 0)
                                EachFormatFrom(
                                    (
                                        OneFormatFrom(("%W",)),
                                        OneFormatFrom(("%w",)),
                                    ),
                                ),
                                # Week number and day of week (Monday = 0)
                                EachFormatFrom(
                                    (
                                        OneFormatFrom(("%U",)),
                                        OneFormatFrom(("%u",)),
                                    ),
                                ),
                            ),
                        ),
                    ),
                ),
                OneFormatFrom(("%A", "%a")),
            ),
        ),
        # Full spec
        OneFormatFrom(("%x",)),
        # ISO 8601
        EachFormatFrom(
            (
                OneFormatFrom(("%G",), must_use=True),
                OneFormatFrom(("%V",), must_use=True),
                OneFormatFrom(("%u", "%a", "%A"), must_use=True),
            )
        ),
    )
)


TIME_FORMAT_CODES = EachFormatFrom(
    (
        OneFormatFrom(
            (
                EachFormatFrom(
                    (
                        OneFormatFrom(("%H", "%I")),
                        OneFormatFrom(("%p",)),
                        OneFormatFrom(("%M",)),
                        OneFormatFrom(("%S",)),
                        OneFormatFrom(("%f",)),
                    )
                ),
                OneFormatFrom(("%X",), must_use=True),
            )
        ),
        EachFormatFrom(
            (
                OneFormatFrom(("%z", "%:z")),
                OneFormatFrom(("%Z",)),
            ),
        ),
    )
)

DATETIME_FORMAT_CODES = OneFormatFrom(
    (
        EachFormatFrom(
            (
                DATE_FORMAT_CODES,
                TIME_FORMAT_CODES,
            ),
        ),
        OneFormatFrom(("%c",)),
    ),
)


class DatetimeFormat:
    def __init__(self, format_codes, format_str):
        self.codes = format_codes
        self.fmt = format_str

    def __repr__(self):
        return f"{self.__class__.__name__}(format_codes={self.codes}, format_str={self.fmt})"

    def __bool__(self):
        return bool(self.fmt)


def _make_strftime_strategy(format_codes, exclude=frozenset()):
    """Generates a strategy that generates valid strftime strings."""

    def _exclude_codes(code_set):
        new_children = []
        for child in code_set.children:
            if child is NoComponent:
                new_children.append(child)
            elif isinstance(child, str):
                if child in exclude:
                    continue
                new_children.append(child)
            else:
                new_sub_node = _exclude_codes(child)
                if new_sub_node is None:
                    continue
                new_children.append(_exclude_codes(child))

        if (
            not new_children
            or len(new_children) == 1
            and new_children[0] is NoComponent
        ):
            return None

        return code_set.replace_children(children=new_children)

    def select_formats(draw, code_set):
        stack = [code_set]
        output = []
        while stack:
            node = stack.pop()
            if node is NoComponent:
                continue

            if isinstance(node, str):
                output.append(node)
            elif isinstance(node, EachFormatFrom):
                for child in node.children:
                    stack.append(child)
            elif isinstance(node, OneFormatFrom):
                stack.append(draw(st.sampled_from(node.children)))
            else:
                raise TypeError(f"Unknown node type: {type(node)}")

        return output

    format_codes = _exclude_codes(format_codes)

    @st.composite
    def _strftime_strategy(draw):
        # Randomly select one format code from each date component category
        selected_formats = select_formats(draw, format_codes)

        # Choose a random order
        selected_formats = draw(st.permutations(selected_formats))

        # Add interstitial components
        components = []

        def _make_interstitial():
            if draw(st.booleans()):
                interstitial_text = draw(st.text()).replace("%", "%%")
            else:
                interstitial_text = ""
            return interstitial_text

        for component in selected_formats:
            components.append(_make_interstitial())
            components.append(component)
        components.append(_make_interstitial())

        format_str = "".join(components)
        return DatetimeFormat(
            format_codes=frozenset(selected_formats), format_str=format_str
        )

    return _strftime_strategy


def datetime_strftimes(*args, **kwargs):
    return _make_strftime_strategy(DATETIME_FORMAT_CODES, *args, **kwargs)()


all_timezones = st.one_of(
    st.timezones(),
    st.none(),
    st.tuples(
        st.timedeltas(
            min_value=-datetime.timedelta(hours=24),
            max_value=datetime.timedelta(hours=24),
        ),
        st.one_of(st.none(), st.text()),
    ).map(lambda x: datetime.timezone(*x)),
)


class DateTimeTest(unittest.TestCase):
    theclass = datetime.datetime

    @hypothesis.given(
        dt=st.datetimes(timezones=st.timezones()),
        # gh-12137: strptime does not accept "%:z"
        fmt=datetime_strftimes(exclude={"%:z"}).filter(lambda x: x),
    )
    def test_strftime_strptime_property(self, dt, fmt):
        fmt_code = fmt.fmt
        # gh-124531: \x00 terminates format strings
        hypothesis.assume("\x00" not in fmt_code)

        # This first step can be lossy so without more extensive logic, we
        # cannot directly make assertions about what this does.
        dt_str = dt.strftime(fmt_code)

        # gh-124529: %c does not work for years < 1000
        hypothesis.assume(not ("%c" in fmt.codes and dt.year < 1000))

        # gh-66571: These can only be parsed back in specific situations
        hypothesis.assume(not "%Z" in fmt.codes)

        # From here on out strptime/strftime rounds should be idempotent
        dt_rt = self.theclass.strptime(dt_str, fmt_code)

        dt_rt_str = dt_rt.strftime(fmt_code)
        if (
            (
                not ({"%a", "%A", "%w", "%u"} & fmt.codes)
                or (
                    ("%Y" in fmt.codes or "%y" in fmt.codes)
                    and ("%j" in fmt.codes or ("%m" in fmt.codes and "%b" in fmt.codes))
                )
                or ("%G" in fmt.codes)
            )
            and not ("%p" in fmt.codes and not ({"%H", "%I"} & fmt.codes))
            and not any(
                x in fmt_code
                for x in ("%z%j", "%z%H", "%z%I", "%z%f", "%z%d", "%z%m", "%z%S")
            )
        ):
            self.assertEqual(dt_rt_str, dt_str)

        # Normally we would need to worry about whether or not one of these
        # is ambiguous, but strptime can only generate code with fixed offsets.
        dt_rt_2 = self.theclass.strptime(dt_rt_str, fmt_code)
        self.assertEqual(dt_rt_2, dt_rt)
