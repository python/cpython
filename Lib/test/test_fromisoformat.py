import functools
import itertools
import re
import sys
import unittest
import zoneinfo
from datetime import date, datetime, time, timedelta, timezone
from test.support.hypothesis_helper import hypothesis
from test.support.import_helper import import_fresh_module
from typing import Any


def _get_modules():
    import datetime as c_datetime
    import zoneinfo as c_zoneinfo

    py_datetime = import_fresh_module(
        "datetime", fresh=["datetime", "_strptime"], blocked=["_datetime"]
    )

    return c_datetime, py_datetime


(c_datetime, py_datetime) = _get_modules()


@functools.lru_cache
def make_timedelta(module, *args, **kwargs):
    return module.timedelta(*args, **kwargs)


@functools.lru_cache
def make_cached_datetime(module, *args, **kwargs):
    return module.datetime(*args, **kwargs)


class IsoFormatter:
    """Helper class to make it possible to round-trip a given ISO 8601 format.

    The main problem this solves is that many ISO 8601 formats are lossy, e.g.::

        >>> datetime(2022, 5, 19, 12, 30, 15).isoformat(timespec="hours")
        2022-05-19T12

    This prevents us from easily writing tests that take arbitrary input
    datetimes, serializes them to an arbitrary ISO 8601 format and ensures that
    the same thing comes back when we try and parse it.

    This class allows you to specify an ISO 8601 format and generate both the
    ISO 8601 string and the truncated datetime, like so:

        >>> formatter = IsoFormatter("%Y-%m-%dT%H")
        >>> dt = datetime(2022, 5, 19, 12, 30, 15)
        >>> formatter.format_datetime(dt)
        "2022-05-19T12"
        >>> formatter.truncate(dt)
        datetime.datetime(2022, 5, 19, 12, 0)
    """

    _TZ_RE = re.compile(r"\[TZ:(?P<fmt>[^\]]+)\]$")
    _FLOAT_RE = re.compile(r"%\(f(?P<prec>\d+)\)$")

    def __init__(self, format_str, datetime_module=c_datetime):
        self._format_str = format_str
        self._module = datetime_module

        # Create instances of these unit values for convenience and performance.
        self._MICROSECOND = make_timedelta(self._module, microseconds=1)
        self._SECOND = make_timedelta(self._module, seconds=1)
        self._MINUTE = make_timedelta(self._module, minutes=1)
        self._HOUR = make_timedelta(self._module, hours=1)
        self._ZERO = make_timedelta(self._module, 0)
        self._ARBITRARY_DT = make_cached_datetime(self._module, 2000, 1, 1)

        if (m := self._TZ_RE.search(format_str)) is not None:
            self._tz_str = m.group("fmt")
            format_str = format_str[: m.start()]
        else:
            self._tz_str = None

        try:
            time_str_start = format_str.index("%H")
        except ValueError:
            time_str_start = None

        if time_str_start is not None:
            self._time_str = format_str[time_str_start:]
            self._sep = format_str[time_str_start - 1]
            self._date_str = format_str[: time_str_start - 1]
        else:
            self._time_str = None
            self._sep = ""
            self._date_str = format_str

        self._date_str = self._date_str.replace("%Y", "%4Y").replace(
            "%G", "%4G"
        )

        self._populate_time()
        self._populate_tz()

        if "W" in self._date_str:
            expected_components = ("%4G", "%V")
        else:
            expected_components = ("%4Y", "%m", "%d")

    def __repr__(self):
        return f"{self.__class__.__name__}('{self._format_str}')"

    def with_module(self, module):
        if self._module is module:
            return self
        return self.__class__(self._format_str, datetime_module=module)

    def format_datetime(self, dt) -> str:
        """Apply the specified ISO8601 format to a datetime."""
        return (
            f"{format(dt, self._date_str)}{self._sep}"
            + f"{self._time_formatter(dt)}{self._tz_formatter(dt)}"
        )

    def format_date(self, dt) -> str:
        return f"{format(dt, self._date_str)}"

    def format_time(self, dt) -> str:
        return f"{self._time_formatter(dt)}{self._tz_formatter(dt)}"

    def truncate(self, dt):
        """Truncate a datetime to the precision level of the format."""
        truncator = {}
        if "W" in self._date_str and "%u" not in self._date_str:
            iso_year, week, weekday = dt.isocalendar()
            if weekday != 1:
                truncated_dt = self._module.datetime.fromisocalendar(
                    iso_year, week, 1
                )
                for comp in ("year", "month", "day"):
                    if getattr(dt, comp) != (
                        new_comp := getattr(truncated_dt, comp)
                    ):
                        truncator[comp] = new_comp

        if hasattr(dt, "tzinfo"):
            truncator.update(self._time_truncator(dt))
            truncator.update(self._tz_truncator(dt))

        if truncator:
            return dt.replace(**truncator)
        else:
            return dt

    def _populate_time(self):
        if self._time_str is not None:
            time_formatter, time_truncation = self._make_timelike_formatter(
                self._time_str
            )
            self._time_formatter = time_formatter
            self._time_truncator = self._make_time_truncator(time_truncation)
        else:
            self._time_formatter = self._null_formatter
            self._time_truncator = self._make_time_truncator(
                self._module.timedelta(days=1)
            )

    def _populate_tz(self):
        if self._tz_str is not None:
            if self._tz_str == "Z":
                self._tz_formatter = self._tz_z_formatter
                self._tz_truncator = self._make_tz_truncator(None)
            else:
                base_formatter, tz_truncation = self._make_timelike_formatter(
                    self._tz_str
                )

                self._tz_formatter = self._make_tz_formatter(base_formatter)
                self._tz_truncator = self._make_tz_truncator(tz_truncation)
        else:
            self._tz_formatter = self._null_formatter
            self._tz_truncator = self._remove_tzinfo_truncator

    def _make_timelike_formatter(self, time_str):
        time_elements = ("%(f", "%S", "%M", "%H")
        truncation_elements = (None, self._SECOND, self._MINUTE, self._HOUR)

        truncation = None
        for i, elem in enumerate(time_elements):
            if elem in time_str:
                assert self._all_in(
                    time_str, time_elements[(i + 1) :]
                ), f"Invalid time str: {time_str}"
                truncation = truncation_elements[i]
                break
        else:
            assert False, f"Invalid time str: {time_str}"

        if (m := self._FLOAT_RE.search(time_str)) is not None:
            time_str = time_str[: m.start()]

            precision = int(m.group("prec"))
            assert precision > 0, "0 and negative precision is not supported"

            truncation = self._module.timedelta(
                microseconds=10 ** (6 - min(6, precision))
            )

            def format_time(dt, *, time_str=time_str, precision=precision):
                if precision < 7:
                    return (
                        format(dt, time_str)
                        + f"{dt.microsecond:06d}"[0:precision]
                    )
                else:
                    return (
                        format(dt, time_str)
                        + f"{dt.microsecond:06d}"
                        + "0" * (precision - 6)
                    )

        else:

            def format_time(dt, *, time_str=time_str):
                return format(dt, time_str)

        return format_time, truncation

    def _make_tz_formatter(self, base_formatter):
        def tz_formatter(dt, *, _self=self, _base_formatter=base_formatter):
            if dt.tzinfo is None:
                return ""
            utcoffset = dt.utcoffset()

            t = self._ARBITRARY_DT + abs(utcoffset)

            sign = "+" if utcoffset >= self._ZERO else "-"

            return sign + _base_formatter(t)

        return tz_formatter

    def _make_time_truncator(self, truncation):
        if truncation is None:

            def time_truncator(dt):
                return {}

        else:

            def time_truncator(dt, *, _time_truncation=truncation):
                time_as_td = self._module.timedelta(
                    hours=dt.hour,
                    minutes=dt.minute,
                    seconds=dt.second,
                    microseconds=dt.microsecond,
                )
                truncated = _time_truncation * (time_as_td // _time_truncation)

                if truncated == time_as_td:
                    return {}

                td_as_datetime = self._ARBITRARY_DT + truncated
                return {
                    component: getattr(td_as_datetime, component)
                    for component in ("hour", "minute", "second", "microsecond")
                }

        return time_truncator

    def _make_tz_truncator(self, truncation):
        if truncation is None:

            def tz_truncator(dt):
                return {}

        else:

            def tz_truncator(dt, *, _tz_truncation=truncation):
                if dt.tzinfo is None:
                    return {}

                offset = dt.utcoffset()
                sign = -1 if offset < self._ZERO else 1

                tmp, remainder = divmod(abs(offset), _tz_truncation)
                if not remainder:
                    return {}

                new_offset = tmp * _tz_truncation
                new_tzinfo = self._module.timezone(sign * new_offset)
                return {"tzinfo": new_tzinfo}

        return tz_truncator

    def _null_formatter(self, dt):
        return ""

    def _remove_tzinfo_truncator(self, dt):
        if dt.tzinfo is not None:
            return {"tzinfo": None}
        return {}

    def _tz_z_formatter(self, dt):
        if dt.tzinfo is None:
            return ""

        utcoffset = dt.utcoffset()

        if utcoffset == self._ZERO:
            return "Z"

        if utcoffset < self._ZERO:
            rv = "-"
        else:
            rv = "+"

        utcoffset = abs(utcoffset)

        hours, rem = divmod(utcoffset, self._HOUR)

        rv += f"{hours:02d}"
        if not rem:
            return rv

        minutes, rem = divmod(rem, self._MINUTE)
        rv += f":{minutes:02d}"
        if not rem:
            return rv

        seconds, rem = divmod(rem, self._SECOND)
        rv += f":{seconds:02d}"
        if not rem:
            return rv

        microseconds = rem // self._MICROSECOND
        rv += f".{microseconds:06d}"
        return rv

    @staticmethod
    def _all_in(string, substrings):
        for substring in substrings:
            if substring not in string:
                return False
        return True


def _cross_product_examples(**kwargs):
    """Adds the cross-product of multiple hypothesis examples.

    This is a helper function to make specifying a bunch of examples less
    complicated. By example:

        @_cross_product_examples(a=[1, 2], b=["a", "b"])
        def test_x(a, b):
            ...

    Is equivalent to this (order not guaranteed):

        @hypothesis.example(a=1, b="a")
        @hypothesis.example(a=2, b="a")
        @hypothesis.example(a=1, b="b")
        @hypothesis.example(a=2, b="b")
        def test_x(a, b):
            ...
    """
    params, values = zip(*kwargs.items())

    def inner(f):
        out = f
        for value_set in itertools.product(*values):
            out = hypothesis.example(**dict(zip(params, value_set)))(out)
        return out

    return inner


################
# Hypothesis strategies
def _valid_date_formats():
    return ("%Y-%m-%d", "%Y%m%d", "%G-W%V", "%GW%V", "%G-W%V-%u", "%GW%V%u")


def _valid_time_formats(max_precision=9):
    subsecond_format_tuples = itertools.product(
        ("%H:%M:%S", "%H%M%S"),
        (f"%(f{prec})" for prec in range(1, max_precision)),
    )
    subsecond_formats = (
        (".".join(comps), ",".join(comps)) for comps in subsecond_format_tuples
    )
    time_formats = ("%H", "%H:%M", "%H:%M:%S", "%H%M", "%H%M%S") + tuple(
        itertools.chain.from_iterable(subsecond_formats)
    )

    tz_formats = ("",) + tuple(
        (f"[TZ:{tz_fmt}]" for tz_fmt in time_formats + ("Z",))
    )

    return tuple(map("".join, itertools.product(time_formats, tz_formats)))


def _make_isoformatter_strategy():
    time_format = hypothesis.strategies.one_of(
        hypothesis.strategies.just(()),  # No time format
        hypothesis.strategies.tuples(
            hypothesis.strategies.one_of(
                hypothesis.strategies.just("T"),  # Shrink towards T and space
                hypothesis.strategies.just(" "),
                hypothesis.strategies.characters(),
            ),
            hypothesis.strategies.sampled_from(VALID_TIME_FORMATS),
        ),
    )

    return hypothesis.strategies.tuples(
        hypothesis.strategies.sampled_from(VALID_DATE_FORMATS), time_format
    ).map(lambda x: IsoFormatter("".join((x[0],) + x[1])))


VALID_DATE_FORMATS = _valid_date_formats()
VALID_TIME_FORMATS = _valid_time_formats()

DATE_ISOFORMATTERS = hypothesis.strategies.sampled_from(VALID_DATE_FORMATS).map(
    IsoFormatter
)
TIME_ISOFORMATTERS = hypothesis.strategies.sampled_from(VALID_TIME_FORMATS).map(
    IsoFormatter
)
ISOFORMATTERS = _make_isoformatter_strategy()
FIXED_TIMEZONES = hypothesis.strategies.timedeltas(
    min_value=timedelta(hours=-23, minutes=59, seconds=59, microseconds=999999),
    max_value=timedelta(hours=23, minutes=59, seconds=59, microseconds=999999),
).map(timezone)
TIMEZONES = hypothesis.strategies.one_of(
    hypothesis.strategies.none(),
    FIXED_TIMEZONES,
    hypothesis.strategies.timezones(),
)

################
# Constants
DEFAULT_DT = datetime(2025, 1, 2, 3, 4, 5, 678901)
AWARE_UTC_DT = datetime(2025, 1, 2, 3, 4, 5, 678901, tzinfo=timezone.utc)
AWARE_POS_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(timedelta(hours=3))
)
AWARE_NEG_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(-timedelta(hours=3))
)


################
# Tests


class FromIsoformatDateTest_Base(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.klass = cls.module.date


class FromIsoformatDateTest_Fast(FromIsoformatDateTest_Base):
    module = c_datetime

    @hypothesis.given(
        d=hypothesis.strategies.dates(),
        iso_formatter=DATE_ISOFORMATTERS,
    )
    @_cross_product_examples(
        d=[
            date(2025, 1, 2),
            date(2000, 1, 1),
            date(1, 1, 1),
            date(9999, 12, 31),
        ],
        iso_formatter=map(IsoFormatter, ["%Y-%m-%d", "%Y%m%d"]),
    )
    @_cross_product_examples(
        d=[date(2025, 1, 2), date(2025, 12, 31), date(2023, 1, 1),
           date(2020, 12, 29), date(2021, 1, 1), date(2015, 12, 31)],
        iso_formatter=map(
            IsoFormatter, ["%G-W%V", "%GW%V", "%G-W%V-%u", "%GW%V%u"]
        ),
    )
    def test_fromisoformat_dates(self, d, iso_formatter):
        iso_formatter = iso_formatter.with_module(self.module)

        if type(d) != self.klass:
            d = self.klass(d.year, d.month, d.day)

        input_str = iso_formatter.format_date(d)
        actual = self.klass.fromisoformat(input_str)
        expected = iso_formatter.truncate(d)

        self.assertEqual(
            actual,
            expected,
            f"\n{actual} != {expected}\n"
            + f"actual = {actual!r}\n"
            + f"expected = {expected!r}\n"
            + f"input_str = {input_str}\n"
            + f"formatter = {iso_formatter!r}",
        )


class FromIsoformatDateTest_Pure(FromIsoformatDateTest_Fast):
    module = py_datetime


class FromIsoformatDateTimeTest_Fast(FromIsoformatDateTest_Fast):
    module = c_datetime

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.klass = cls.module.datetime

    @hypothesis.given(
        dt=hypothesis.strategies.datetimes(timezones=TIMEZONES),
        iso_formatter=ISOFORMATTERS,
    )
    # fmt: off
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%d"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y%m%d"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y%m%dT%H"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y%m%dT%H"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y%m%dT%H:%M"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H%M"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H%M%S"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H%M"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H%M%S"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S.%(f1)"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S,%(f1)"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:Z]"))
    @hypothesis.example(dt=AWARE_UTC_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:Z]"))
    @hypothesis.example(dt=AWARE_POS_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:Z]"))
    @hypothesis.example(dt=AWARE_UTC_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H]"))
    @hypothesis.example(dt=AWARE_NEG_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H]"))
    @hypothesis.example(dt=AWARE_POS_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H]"))
    @hypothesis.example(dt=AWARE_UTC_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H%M]"))
    @hypothesis.example(dt=AWARE_NEG_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H%M]"))
    @hypothesis.example(dt=AWARE_POS_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S[TZ:%H%M]"))
    @hypothesis.example(dt=datetime(2000, 1, 1,
                                    tzinfo=timezone(-timedelta(hours=-22, microseconds=1))),
                        iso_formatter=IsoFormatter("%Y-%m-%dT%H[TZ:%H]"))
    @hypothesis.example(dt=AWARE_UTC_DT,
                        iso_formatter=IsoFormatter("%Y-%m-%d0%H:%M:%S,%(f1)[TZ:%H:%M:%S.%(f2)]"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%G-W%V"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%G-W%V-%u"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%GW%V:%H"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%GW%V5%H"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%GW%V%u5%H"))
    @hypothesis.example(dt=AWARE_UTC_DT, iso_formatter=IsoFormatter("%G-W%V0%H[TZ:%H]"))
    @hypothesis.example(dt=DEFAULT_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S,%(f3)[TZ:Z]"))
    @hypothesis.example(dt=DEFAULT_DT.replace(tzinfo=timezone(timedelta(seconds=10))), iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S,%(f3)[TZ:Z]"))
    @hypothesis.example(dt=AWARE_UTC_DT, iso_formatter=IsoFormatter("%Y-%m-%dT%H:%M:%S,%(f1)[TZ:%H%M%S.%(f2)]"))
    @_cross_product_examples(
        dt=[
            datetime(2020, 1, 1, 3, 5, 7, 123457, tzinfo=zoneinfo.ZoneInfo("America/New_York")),
            datetime(2020, 6, 1, 4, 5, 6, 111111, tzinfo=zoneinfo.ZoneInfo("America/New_York")),
            datetime(2021, 10, 31, 1, 30, tzinfo=zoneinfo.ZoneInfo("Europe/London")),
        ],
        iso_formatter=[
            IsoFormatter("%Y-%m-%dT%H:%M:%S.%(f6)[TZ:%H:%M]"),
            IsoFormatter("%Y-%m-%dT%H:%M:%S.%(f6)[TZ:%H%M]"),
        ])
    # fmt: on
    def test_fromisoformat(self, dt, iso_formatter):
        iso_formatter = iso_formatter.with_module(self.module)

        if dt.tzinfo is None or isinstance(dt.tzinfo, self.module.timezone):
            new_tzinfo = dt.tzinfo
        else:
            new_offset = self.module.timedelta(
                seconds=dt.utcoffset().total_seconds()
            )
            new_tzinfo = self.module.timezone(new_offset, dt.tzname())

        if not isinstance(dt, self.module.datetime):
            dt = self.klass(
                dt.year,
                dt.month,
                dt.day,
                dt.hour,
                dt.minute,
                dt.second,
                dt.microsecond,
                tzinfo=new_tzinfo,
                fold=dt.fold,
            )
        elif dt.tzinfo is not new_tzinfo:
            dt = dt.astimezone(new_tzinfo)

        if "%G" in iso_formatter._format_str:
            if (
                iso_formatter._format_str.startswith("%G-W%V-%u")
                and len(iso_formatter._format_str) > 9
            ):
                hypothesis.assume(not iso_formatter._format_str[9].isdigit())

        input_str = iso_formatter.format_datetime(dt)
        actual = self.klass.fromisoformat(input_str)
        expected = iso_formatter.truncate(dt)

        self.assertEqual(
            actual,
            expected,
            f"\n{actual} != {expected}\n"
            + f"actual = {actual!r}\n"
            + f"expected = {expected!r} \n"
            + f"input_str = {input_str}",
        )


class FromIsoformatDateTimeTest_Pure(FromIsoformatDateTimeTest_Fast):
    module = py_datetime


class FromIsoformatTimeTest_Base(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.klass = cls.module.time


class FromIsoformatTimeTest_Fast(FromIsoformatTimeTest_Base):
    module = c_datetime

    @hypothesis.given(
        t=hypothesis.strategies.times(
            timezones=FIXED_TIMEZONES | hypothesis.strategies.none()
        ),
        iso_formatter=TIME_ISOFORMATTERS,
    )
    @_cross_product_examples(
        t=[
            time(0, 0),
            time(12, 0),
            time(23, 59, 59, 999999),
            time(12, 0, tzinfo=timezone.utc),
            time(12, 0, tzinfo=timezone(timedelta(hours=-5))),
        ],
        iso_formatter=map(
            IsoFormatter,
            [
                "%H:%M",
                "T%H:%M",
                "%H%M",
                "%H:%M:%S",
                "%H%M%S",
                "%H:%M:%S.%(f6)",
                "%H%M%S.%(f6)",
                "%H:%M:%S.%(f3)",
                "%H%M%S.%(f1)",
                "%H%M%S.%(f3)",
                "%H:%M:%S[TZ:%H:%M]",
                "%H:%M:%S[TZ:%H%M]",
                "T%H:%M:%S",
                "T%H%M%S",
            ],
        ),
    )
    @hypothesis.example(
        t=time(0, 0, tzinfo=timezone.utc),
        iso_formatter=IsoFormatter("%H:%M:%S[TZ:Z]"),
    )
    @_cross_product_examples(
        t=[
            time(0, 0, tzinfo=timezone(timedelta(hours=5, minutes=30))),
        ],
        iso_formatter=map(
            IsoFormatter, ("%H:%M:%S[TZ:%H]", "%H:%M:%S[TZ:%H:%M]")
        ),
    )
    @hypothesis.example(
        t=time(
            0,
            0,
            tzinfo=timezone(
                -timedelta(
                    hours=23, minutes=59, seconds=59, microseconds=999999
                )
            ),
        ),
        iso_formatter=IsoFormatter("%H:%M:%S,%(f3)[TZ:Z]"),
    )
    def test_fromisoformat_times(self, t, iso_formatter):
        iso_formatter = iso_formatter.with_module(self.module)

        if t.tzinfo is None or isinstance(t.tzinfo, self.module.timezone):
            new_tzinfo = t.tzinfo
        else:
            new_offset = self.module.timedelta(
                seconds=t.utcoffset().total_seconds()
            )
            new_tzinfo = self.module.timezone(new_offset, t.tzname())

        if not isinstance(t, self.module.time):
            t = self.klass(
                hour=t.hour,
                minute=t.minute,
                second=t.second,
                microsecond=t.microsecond,
                tzinfo=new_tzinfo,
                fold=t.fold,
            )
        elif t.tzinfo is not new_tzinfo:
            t = t.replace(tzinfo=new_tzinfo)

        input_str = iso_formatter.format_time(t)
        actual = self.klass.fromisoformat(input_str)
        expected = iso_formatter.truncate(t)

        self.assertEqual(
            actual,
            expected,
            f"\n{actual} != {expected}\n"
            + f"actual = {actual!r}\n"
            + f"expected = {expected!r} \n"
            + f"input_str = {input_str}\n"
            + f"formatter = {iso_formatter!r}",
        )


class FromIsoformatTimeTest_Pure(FromIsoformatTimeTest_Fast):
    module = py_datetime
