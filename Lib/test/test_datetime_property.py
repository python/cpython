import datetime
import sys
import time
import unittest
from test import support
from test.support import import_helper
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
                        OneFormatFrom(("%Y", "%y"), must_use=True),
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
                                # Monday-based week number and weekday
                                EachFormatFrom(
                                    (
                                        OneFormatFrom(("%W",)),
                                        OneFormatFrom(("%w",)),
                                    ),
                                ),
                                # Sunday-based week number and weekday
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
        return f"{self.__class__.__name__}(format_codes={self.codes!r}, format_str={self.fmt!r})"

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
                new_children.append(new_sub_node)

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
                # gh-124531: Some strftime implementations truncate at NUL.
                interstitial_text = draw(st.text(
                    alphabet=st.characters(exclude_characters="\x00"),
                    max_size=10,
                )).replace("%", "%%")
            else:
                interstitial_text = ""
            return interstitial_text

        for component in selected_formats:
            components.append(_make_interstitial())
            components.append(component)
        components.append(_make_interstitial())

        # Variable-width numeric directives can consume digits belonging to
        # adjacent fields. Round trips require unambiguous field boundaries.
        format_str = "|".join(components)
        return DatetimeFormat(
            format_codes=frozenset(selected_formats), format_str=format_str
        )

    return _strftime_strategy


def datetime_strftimes(*args, **kwargs):
    return _make_strftime_strategy(DATETIME_FORMAT_CODES, *args, **kwargs)()


def strptime_inputs():
    # isoformat pads small years consistently across platforms, so these also
    # cover the years that the platform strftime can't always round-trip.
    return st.datetimes().map(lambda dt: (
        dt.isoformat(" "),
        "%Y-%m-%d %H:%M:%S" + (".%f" if dt.microsecond else ""),
        dt,
    ))


class DateTimeTest(unittest.TestCase):
    datetime_module = datetime
    theclass = datetime.datetime

    @hypothesis.settings(max_examples=50)
    @hypothesis.given(
        dt=st.datetimes(),
        mode=st.sampled_from(("full", "date", "time", "year", "month", "minute",
                              "ordinal", "iso", "redundant")),
    )
    @hypothesis.example(dt=datetime.datetime.min, mode="full")
    @hypothesis.example(dt=datetime.datetime.max, mode="full")
    @hypothesis.example(dt=datetime.datetime(2000, 2, 29, 12, 34, 56, 789), mode="date")
    @hypothesis.example(dt=datetime.datetime(2000, 2, 29, 12, 34, 56, 789), mode="time")
    @hypothesis.example(dt=datetime.datetime(1969, 7, 20, 20, 17), mode="year")
    @hypothesis.example(dt=datetime.datetime(2000, 2, 29), mode="month")
    @hypothesis.example(dt=datetime.datetime(2000, 1, 1, 23, 59), mode="minute")
    @hypothesis.example(dt=datetime.datetime(2000, 12, 31), mode="ordinal")
    @hypothesis.example(dt=datetime.datetime(2016, 1, 1), mode="iso")
    @hypothesis.example(dt=datetime.datetime(2024, 2, 29), mode="redundant")
    def test_public_entry_points(self, dt: datetime.datetime, mode: str) -> None:
        expected = dt
        clock = f"{dt.hour:02}:{dt.minute:02}:{dt.second:02}.{dt.microsecond:06}"
        ordinal = dt.toordinal() - datetime.date(dt.year, 1, 1).toordinal() + 1
        if mode == "full":
            value = f"{dt.date().isoformat()} {clock}+00:00"
            fmt = "%Y-%m-%d %H:%M:%S.%f%:z"
        elif mode == "date":
            value, fmt = dt.date().isoformat(), "%Y-%m-%d"
            expected = datetime.datetime(dt.year, dt.month, dt.day)
        elif mode == "time":
            value, fmt = clock, "%H:%M:%S.%f"
            expected = dt.replace(year=1900, month=1, day=1)
        elif mode == "year":
            value, fmt = f"{dt.year:04}", "%Y"
            expected = datetime.datetime(dt.year, 1, 1)
        elif mode == "month":
            value, fmt = str(dt.month), "%m"
            expected = datetime.datetime(1900, dt.month, 1)
        elif mode == "minute":
            value, fmt = str(dt.minute), "%M"
            expected = datetime.datetime(1900, 1, 1, minute=dt.minute)
        elif mode == "ordinal":
            value, fmt = f"{dt.year:04} {ordinal} {clock}", "%Y %j %H:%M:%S.%f"
        elif mode == "iso":
            year, week, weekday = dt.isocalendar()
            value = f"{year:04} {week} {weekday} {clock}"
            fmt = "%G %V %u %H:%M:%S.%f"
        else:
            value = f"{dt.date().isoformat()} {ordinal} {dt.isoweekday()} {clock}"
            fmt = "%Y-%m-%d %j %u %H:%M:%S.%f"

        parsed = self.theclass.strptime(value, fmt)
        date_result = self.datetime_module.date.strptime(value, fmt)
        time_result = self.datetime_module.time.strptime(value, fmt)
        struct_result = time.strptime(value, fmt)
        self.assertEqual(parsed.replace(tzinfo=None).isoformat(), expected.isoformat())
        self.assertEqual(date_result.isoformat(), expected.date().isoformat())
        self.assertEqual(time_result.replace(tzinfo=None).isoformat(),
                         expected.time().isoformat())
        self.assertEqual(struct_result[:9], expected.timetuple()[:9])
        for result in (parsed, time_result):
            if mode == "full":
                self.assertIsNotNone(result.tzinfo)
                self.assertEqual(result.utcoffset().total_seconds(), 0)
            else:
                self.assertIsNone(result.tzinfo)

    @support.run_with_locale("LC_TIME", "C")
    @hypothesis.settings(max_examples=50)
    @hypothesis.given(
        day=st.dates(), hour=st.integers(min_value=0, max_value=23),
        abbreviated=st.booleans(), casing=st.sampled_from(("upper", "lower", "title")),
        literal=st.text(max_size=8),
        whitespace=st.sampled_from((" ", "\t", "\n", "\r\n", "\u2003")),
    )
    @hypothesis.example(day=datetime.date(2000, 2, 29), hour=0,
                        abbreviated=False, casing="upper", literal="\x00%[]",
                        whitespace="\u2003")
    @hypothesis.example(day=datetime.date.min, hour=12,
                        abbreviated=True, casing="lower", literal="\\.^$*+?",
                        whitespace="\r\n")
    @hypothesis.example(day=datetime.date.max, hour=23,
                        abbreviated=False, casing="title", literal="'\ud800",
                        whitespace="\t")
    def test_names_and_literals(
        self, day: datetime.date, hour: int, abbreviated: bool, casing: str,
        literal: str, whitespace: str,
    ) -> None:
        # Fixed C-locale tables avoid using strftime to supply the expected
        # names. Arbitrary literals, including NUL, need only work in strptime.
        months = ("January", "February", "March", "April", "May", "June", "July",
                  "August", "September", "October", "November", "December")
        weekdays = ("Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
                    "Saturday", "Sunday")
        month = months[day.month - 1]
        weekday = weekdays[day.weekday()]
        if abbreviated:
            month, weekday = month[:3], weekday[:3]
        month = getattr(month, casing)()
        weekday = getattr(weekday, casing)()
        ampm = getattr("AM" if hour < 12 else "PM", casing)()
        value = (f"{literal}|{day.year:04} {month} {day.day} {weekday}"
                 f"{whitespace}{hour % 12 or 12}:07 {ampm}")
        fmt = literal.replace("%", "%%") + "|%Y "
        fmt += "%b %d %a" if abbreviated else "%B %d %A"
        fmt += " %I:%M %p"
        actual = self.theclass.strptime(value, fmt)
        self.assertEqual(actual.isoformat(), f"{day.isoformat()}T{hour:02}:07:00")

    @hypothesis.settings(max_examples=50)
    @hypothesis.given(
        day=st.dates(),
        field=st.sampled_from(("month", "day", "leap_day", "hour", "minute",
                               "second", "offset", "fraction", "iso_year",
                               "iso_week", "iso_weekday", "trailing", "year")),
        excess=st.integers(min_value=0, max_value=99),
    )
    @hypothesis.example(day=datetime.date(1900, 2, 28), field="leap_day", excess=0)
    @hypothesis.example(day=datetime.date(2000, 2, 29), field="month", excess=0)
    @hypothesis.example(day=datetime.date(2000, 2, 29), field="month", excess=1)
    @hypothesis.example(day=datetime.date.min, field="day", excess=0)
    @hypothesis.example(day=datetime.date.min, field="day", excess=1)
    @hypothesis.example(day=datetime.date.max, field="hour", excess=0)
    @hypothesis.example(day=datetime.date.min, field="minute", excess=0)
    @hypothesis.example(day=datetime.date.min, field="second", excess=0)
    @hypothesis.example(day=datetime.date.min, field="offset", excess=0)
    @hypothesis.example(day=datetime.date.min, field="fraction", excess=0)
    @hypothesis.example(day=datetime.date.min, field="fraction", excess=1)
    @hypothesis.example(day=datetime.date.min, field="iso_year", excess=0)
    @hypothesis.example(day=datetime.date.min, field="iso_week", excess=0)
    @hypothesis.example(day=datetime.date.min, field="iso_weekday", excess=0)
    @hypothesis.example(day=datetime.date.min, field="trailing", excess=0)
    @hypothesis.example(day=datetime.date.min, field="year", excess=0)
    def test_invalid_fields(
        self, day: datetime.date, field: str, excess: int,
    ) -> None:
        value = day.isoformat()
        fmt = "%Y-%m-%d"
        if field == "month":
            month = 12 + excess if excess else 0
            value = f"{day.year:04}-{month:02}-{day.day:02}"
        elif field == "day":
            invalid_day = 31 + excess if excess else 0
            value = f"{day.year:04}-{day.month:02}-{invalid_day:02}"
        elif field == "leap_day":
            year = day.year
            if year % 4 == 0 and (year % 100 != 0 or year % 400 == 0):
                year -= 1
            value = f"{year:04}-02-29"
        elif field in ("hour", "minute", "second"):
            directive, limit = {
                "hour": ("%H", 24), "minute": ("%M", 60), "second": ("%S", 60),
            }[field]
            value += f" {limit + excess}"
            fmt += " " + directive
        elif field == "offset":
            value += f" +{24 + excess:02}:00"
            fmt += " %:z"
        elif field == "fraction":
            value += "." + ("1234567" if excess % 2 else "")
            fmt += ".%f"
        elif field == "iso_year":
            value, fmt = f"{day.year:04} 01", "%G %V"
        elif field == "iso_week":
            value, fmt = f"{day.year:04} {54 + excess} 1", "%G %V %u"
        elif field == "iso_weekday":
            value, fmt = f"{day.year:04} 01 {8 + excess}", "%G %V %u"
        elif field == "trailing":
            value += "!" * (excess + 1)
        else:
            value = f"0000-{day.month:02}-{day.day:02}"
        with self.assertRaises(ValueError):
            self.theclass.strptime(value, fmt)

    @hypothesis.settings(max_examples=50)
    @hypothesis.given(
        day=st.dates(),
        fmt=st.sampled_from(("%Y-%m-%d", "%y-%m-%d", "%Y %j",
                             "%G %V %u", "%Y %U %w", "%Y %W %w")),
        padded=st.booleans(),
    )
    @hypothesis.example(day=datetime.date.min, fmt="%Y %U %w", padded=False)
    @hypothesis.example(day=datetime.date.max, fmt="%G %V %u", padded=True)
    @hypothesis.example(day=datetime.date(2000, 2, 29), fmt="%Y %j", padded=False)
    @hypothesis.example(day=datetime.date(1900, 3, 1), fmt="%Y %j", padded=True)
    @hypothesis.example(day=datetime.date(2016, 1, 1), fmt="%G %V %u", padded=False)
    @hypothesis.example(day=datetime.date(1968, 2, 29), fmt="%y-%m-%d", padded=True)
    @hypothesis.example(day=datetime.date(2069, 1, 1), fmt="%y-%m-%d", padded=False)
    @hypothesis.example(day=datetime.date(2023, 1, 1), fmt="%Y %W %w", padded=True)
    @hypothesis.example(day=datetime.date(1, 1, 1), fmt="%Y-%m-%d", padded=False)
    def test_calendar_representations(
        self, day: datetime.date, fmt: str, padded: bool,
    ) -> None:
        # Build the input without strftime: libc year padding and calendar
        # calculations must not also be the oracle for strptime.
        width = 2 if padded else 1
        expected = day
        if fmt in ("%Y-%m-%d", "%y-%m-%d"):
            if fmt.startswith("%y"):
                year = day.year % 100
                value = f"{year:02}"
                expected = day.replace(year=year + (2000 if year <= 68 else 1900))
            else:
                value = f"{day.year:04}"
            value += f"-{day.month:0{width}}-{day.day:0{width}}"
        elif fmt == "%G %V %u":
            year, week, weekday = day.isocalendar()
            value = f"{year:04} {week:0{width}} {weekday}"
            expected = datetime.date.fromisocalendar(year, week, weekday)
        else:
            ordinal = day.toordinal() - datetime.date(day.year, 1, 1).toordinal()
            if fmt == "%Y %j":
                value = f"{day.year:04} {ordinal + 1:0{3 if padded else 1}}"
            else:
                # Count complete weeks ending on this date. Week zero ends
                # immediately before the first Sunday (%U) or Monday (%W).
                weekday = (day.weekday() + 1) % 7
                week_start = weekday if "%U" in fmt else day.weekday()
                week = (ordinal + 7 - week_start) // 7
                value = f"{day.year:04} {week:0{width}} {weekday}"
        actual = self.theclass.strptime(value, fmt)
        self.assertEqual(actual.isoformat(), f"{expected.isoformat()}T00:00:00")

    @hypothesis.settings(max_examples=50)
    @hypothesis.given(
        fraction=st.text(alphabet="0123456789", min_size=1, max_size=6),
        offset=st.integers(min_value=-86_399_999_999, max_value=86_399_999_999),
        directive=st.sampled_from(("%z", "%:z")),
        style=st.sampled_from(("minutes", "seconds", "fraction", "Z", "naive")),
        colon=st.booleans(),
    )
    @hypothesis.example(fraction="1", offset=-1, directive="%z",
                        style="fraction", colon=False)
    @hypothesis.example(fraction="000001", offset=86_399_999_999, directive="%:z",
                        style="fraction", colon=True)
    @hypothesis.example(fraction="999999", offset=-86_399_999_999, directive="%z",
                        style="fraction", colon=True)
    @hypothesis.example(fraction="01", offset=0, directive="%:z",
                        style="Z", colon=True)
    @hypothesis.example(fraction="001", offset=0, directive="%z",
                        style="naive", colon=False)
    @hypothesis.example(fraction="1234", offset=-1, directive="%z",
                        style="minutes", colon=False)
    @hypothesis.example(fraction="12345", offset=3_661_000_000, directive="%z",
                        style="seconds", colon=True)
    def test_fractions_and_offsets(
        self, fraction: str, offset: int, directive: str, style: str, colon: bool,
    ) -> None:
        sign = "-" if offset < 0 else "+"
        magnitude = abs(offset)
        if style == "minutes":
            magnitude = magnitude // 60_000_000 * 60_000_000
        elif style == "seconds":
            magnitude = magnitude // 1_000_000 * 1_000_000
        hours, remainder = divmod(magnitude, 3_600_000_000)
        minutes, remainder = divmod(remainder, 60_000_000)
        seconds, micros = divmod(remainder, 1_000_000)
        separator = ":" if colon or directive == "%:z" else ""
        zone = f"{sign}{hours:02}{separator}{minutes:02}"
        if style in ("seconds", "fraction"):
            zone += f"{separator}{seconds:02}"
        if style == "fraction":
            # Include every allowed fractional precision, including leading
            # zeros. Removing trailing zeros doesn't change the value.
            zone += "." + (f"{micros:06}".rstrip("0") or "0")
        expected_offset = datetime.timedelta(
            microseconds=-magnitude if sign == "-" else magnitude,
        )
        if style == "Z":
            zone = "Z"
            expected_offset = datetime.timedelta(0)
        elif style == "naive":
            zone = ""
            expected_offset = None
        actual = self.theclass.strptime(
            f"2000-02-29 23:59:58.{fraction}|{zone}",
            "%Y-%m-%d %H:%M:%S.%f|" + directive,
        )
        expected_fraction = int(fraction) * 10 ** (6 - len(fraction))
        self.assertEqual(
            (actual.year, actual.month, actual.day, actual.hour, actual.minute,
             actual.second, actual.microsecond),
            (2000, 2, 29, 23, 59, 58, expected_fraction),
        )
        actual_offset = actual.utcoffset()
        if expected_offset is None:
            self.assertIsNone(actual.tzinfo)
        else:
            # C and Python timedelta objects aren't directly comparable.
            self.assertIsNotNone(actual_offset)
            self.assertEqual(
                (actual_offset.days, actual_offset.seconds, actual_offset.microseconds),
                (expected_offset.days, expected_offset.seconds,
                 expected_offset.microseconds),
            )

    @support.run_with_locale("LC_TIME", "C")
    @hypothesis.given(case=strptime_inputs())
    @hypothesis.example(case=(
        "0001-01-01 00:00:00", "%Y-%m-%d %H:%M:%S", datetime.datetime.min,
    ))
    @hypothesis.example(case=(
        "+000000000001AM00", "%z%M%f%p%H",
        datetime.datetime(1900, 1, 1, microsecond=100,
                          tzinfo=datetime.timezone.utc),
    ))
    @hypothesis.example(case=(
        "+000000000100", "%z%f%S",
        datetime.datetime(1900, 1, 1, microsecond=100,
                          tzinfo=datetime.timezone.utc),
    ))
    def test_strptime_fields(self, case):
        value, fmt, expected = case
        actual = self.theclass.strptime(value, fmt)
        # Comparing components also works across the C and Python classes.
        self.assertEqual(actual.isoformat(), expected.isoformat())

    # Locale composites may include fields such as %Z that aren't invertible.
    # Keep this property in the C locale, where their expansions are known.
    @support.run_with_locale("LC_TIME", "C")
    @hypothesis.given(
        # Some strftime implementations don't pad %Y or %G to four digits.
        dt=st.datetimes(min_value=datetime.datetime(1000, 1, 8),
                        timezones=st.timezones()),
        # gh-66571: Arbitrary IANA zone names cannot be parsed with %Z.
        fmt=datetime_strftimes(exclude={"%Z"}).filter(lambda x: x),
    )
    @hypothesis.example(
        dt=datetime.datetime(2000, 1, 1, tzinfo=datetime.timezone.utc),
        fmt=DatetimeFormat(frozenset({"%z", "%H", "%d", "%m"}), "%z%H%d%m"),
    )
    @hypothesis.example(
        dt=datetime.datetime(2000, 1, 3, tzinfo=datetime.timezone.utc),
        fmt=DatetimeFormat(frozenset({"%W", "%Y"}), "%W%Y"),
    )
    @hypothesis.example(
        dt=datetime.datetime(1968, 1, 1, tzinfo=datetime.timezone.utc),
        fmt=DatetimeFormat(frozenset({"%A", "%j", "%y"}), "%A%j%y"),
    )
    @hypothesis.example(
        dt=datetime.datetime(2000, 1, 1, tzinfo=datetime.timezone.utc),
        fmt=DatetimeFormat(frozenset({"%c"}), "|%c|"),
    )
    def test_strftime_strptime_property(self, dt, fmt):
        fmt_code = fmt.fmt
        # This first step can be lossy so without more extensive logic, we
        # cannot directly make assertions about what this does.
        dt_str = dt.strftime(fmt_code)

        # Day-of-month parsing without a year is no longer supported.
        if "%d" in fmt.codes and not ({"%Y", "%y"} & fmt.codes):
            with self.assertRaises(ValueError):
                self.theclass.strptime(dt_str, fmt_code)
            return

        # From here on out strptime/strftime rounds should be idempotent
        dt_rt = self.theclass.strptime(dt_str, fmt_code)

        dt_rt_str = dt_rt.strftime(fmt_code)
        if (
            (
                not ({"%a", "%A", "%w", "%u"} & fmt.codes)
                or (
                    ("%Y" in fmt.codes or
                     ("%y" in fmt.codes and 1969 <= dt.year <= 2068))
                    and ("%j" in fmt.codes or
                         ("%d" in fmt.codes and {"%m", "%b", "%B"} & fmt.codes))
                )
                or ("%G" in fmt.codes)
            )
            and not ("%p" in fmt.codes and not ({"%H", "%I"} & fmt.codes))
            # Week numbers are ignored unless a weekday is also supplied.
            and not (
                {"%U", "%W"} & fmt.codes
                and not {"%a", "%A", "%w", "%u"} & fmt.codes
            )
        ):
            self.assertEqual(dt_rt_str, dt_str)

        # Normally we would need to worry about whether or not one of these
        # is ambiguous, but strptime can only generate code with fixed offsets.
        dt_rt_2 = self.theclass.strptime(dt_rt_str, fmt_code)
        self.assertEqual(dt_rt_2, dt_rt)


class PureDateTimeTest(DateTimeTest):
    datetime_module = import_helper.import_fresh_module(
        "datetime", fresh=["_pydatetime"], blocked=["_datetime"],
    )
    theclass = datetime_module.datetime

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        # _strptime must construct timezones from the same implementation as
        # the datetime class. Restore the module cache after the pure tests.
        with support.swap_item(sys.modules, "datetime", cls.datetime_module):
            strptime = import_helper.import_fresh_module("_strptime")
        cls.enterClassContext(support.swap_item(sys.modules, "_strptime", strptime))


if __name__ == "__main__":
    unittest.main()
