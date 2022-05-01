import re
import itertools
import functools

from datetime import date, datetime, time, timedelta, timezone
from typing import Any


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
        >>> formatter.format(dt)
        "2022-05-19T12"
        >>> formatter.truncate(dt)
        datetime.datetime(2022, 5, 19, 12, 0)
    """

    _TZ_RE = re.compile(r"\[TZ:(?P<fmt>[^\]]+)\]$")
    _FLOAT_RE = re.compile(r"%\(f(?P<prec>\d+)\)$")

    # Create instances of these unit values for convenience and performance.
    _MICROSECOND = timedelta(microseconds=1)
    _SECOND = timedelta(seconds=1)
    _MINUTE = timedelta(minutes=1)
    _HOUR = timedelta(hours=1)
    _ZERO = timedelta(0)

    def __init__(self, format_str):
        self._format_str = format_str

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

    @functools.singledispatchmethod
    def format(self, dt: Any) -> str:
        raise NotImplementedError()

    @format.register
    def _(self, dt: datetime) -> str:
        """Apply the specified ISO8601 format to a datetime."""
        return (
            f"{format(dt, self._date_str)}{self._sep}"
            + f"{self._time_formatter(dt)}{self._tz_formatter(dt)}"
        )

    @format.register
    def _(self, dt: date) -> str:
        return f"{format(dt, self._date_str)}"

    @format.register
    def _(self, dt: time) -> str:
        return f"{self._time_formatter(dt)}{self._tz_formatter(dt)}"

    def truncate(self, dt):
        """Truncate a datetime to the precision level of the format."""
        truncator = {}
        if "W" in self._date_str and "%u" not in self._date_str:
            iso_year, week, weekday = dt.isocalendar()
            if weekday != 1:
                truncated_dt = datetime.fromisocalendar(iso_year, week, 1)
                for comp in ("year", "month", "day"):
                    if getattr(dt, comp) != (
                        new_comp := getattr(truncated_dt, comp)
                    ):
                        truncator[comp] = new_comp

        if isinstance(dt, (datetime, time)):
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
            self._time_truncator = self._make_time_truncator(timedelta(days=1))

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

            truncation = timedelta(microseconds=10 ** (6 - min(6, precision)))

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

    _ARBITRARY_DT = datetime(2000, 1, 1)

    def _make_tz_formatter(self, base_formatter):
        def tz_formatter(dt, *, _self=self, _base_formatter=base_formatter):
            if dt.tzinfo is None:
                return ""
            utcoffset = dt.utcoffset()

            t = self._ARBITRARY_DT + abs(utcoffset)

            sign = "+" if utcoffset >= _self._ZERO else "-"

            return sign + _base_formatter(t)

        return tz_formatter

    def _make_time_truncator(self, truncation):
        if truncation is None:

            def time_truncator(dt):
                return {}

        else:

            def time_truncator(dt, *, _time_truncation=truncation):
                time_as_td = timedelta(
                    hours=dt.hour,
                    minutes=dt.minute,
                    seconds=dt.second,
                    microseconds=dt.microsecond,
                )
                truncated = _time_truncation * (time_as_td // _time_truncation)

                if truncated == time_as_td:
                    return {}

                td_as_datetime = datetime(1970, 1, 1) + truncated
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
                new_tzinfo = timezone(sign * new_offset)
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

        if utcoffset == timedelta(0):
            return "Z"

        hours, rem = divmod(utcoffset, timedelta(hours=1))

        rv = f"{hours:+03d}"
        if not rem:
            return rv

        minutes, rem = divmod(rem, timedelta(minutes=1))
        rv += f":{rem.total_seconds():02f}"
        if not rem:
            return rv

        microseconds = rem // timedelta(microseconds=1)
        rv += f".{microseconds:06d}"
        return rv

    @staticmethod
    def _all_in(string, substrings):
        for substring in substrings:
            if substring not in string:
                return False
        return True


