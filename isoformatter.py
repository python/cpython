import re
import itertools
import functools

from datetime import date, datetime, time, timedelta, timezone

# import hypothesis
from test.support.hypothesis_helper import hypothesis

import unittest


def _valid_date_formats():
    return ('%Y-%m-%d', '%Y%m%d', '%G-W%V', '%GW%V', '%G-W%V-%u', '%GW%V%u')


def _valid_time_formats(max_precision=9):
    subsecond_format_tuples = itertools.product(
        ('%H:%M:%S', '%H%M%S'),
        (f'%(f{prec})' for prec in range(1, max_precision)),
    )
    subsecond_formats = (
        ('.'.join(comps), ','.join(comps)) for comps in subsecond_format_tuples
    )
    time_formats = ('%H', '%H:%M', '%H:%M:%S', '%H%M', '%H%M%S') + tuple(
        itertools.chain.from_iterable(subsecond_formats)
    )

    tz_formats = ('',) + tuple(
        (f'[TZ:{tz_fmt}]' for tz_fmt in time_formats + ('Z',))
    )

    return tuple(map(''.join, itertools.product(time_formats, tz_formats)))


VALID_DATE_FORMATS = _valid_date_formats()
VALID_TIME_FORMATS = _valid_time_formats()


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
    ).map(lambda x: IsoFormatter(''.join((x[0],) + x[1])))


ISOFORMATTERS = _make_isoformatter_strategy()
TIMEZONES = hypothesis.strategies.one_of(
    hypothesis.strategies.none(),
    hypothesis.strategies.timedeltas(
        min_value=timedelta(
            hours=-23, minutes=59, seconds=59, microseconds=999999
        ),
        max_value=timedelta(
            hours=23, minutes=59, seconds=59, microseconds=999999
        ),
    ).map(timezone),
    hypothesis.strategies.timezones(),
)


class IsoFormatter:
    _TZ_RE = re.compile(r'\[TZ:(?P<fmt>[^\]]+)\]$')
    _FLOAT_RE = re.compile(r'%\(f(?P<prec>\d+)\)$')

    _MICROSECOND = timedelta(microseconds=1)
    _SECOND = timedelta(seconds=1)
    _MINUTE = timedelta(minutes=1)
    _HOUR = timedelta(hours=1)
    _ZERO = timedelta(0)

    def __init__(self, format_str):
        self._format_str = format_str

        if (m := self._TZ_RE.search(format_str)) is not None:
            self._tz_str = m.group('fmt')
            format_str = format_str[: m.start()]
        else:
            self._tz_str = None

        try:
            time_str_start = format_str.index('%H')
        except ValueError:
            time_str_start = None

        if time_str_start is not None:
            self._time_str = format_str[time_str_start:]
            self._sep = format_str[time_str_start - 1]
            self._date_str = format_str[: time_str_start - 1]
        else:
            self._time_str = None
            self._sep = ''
            self._date_str = format_str

        self._date_str = self._date_str.replace("%Y", "%4Y").replace(
            "%G", "%4G"
        )

        self._populate_time()
        self._populate_tz()

        if 'W' in self._date_str:
            expected_components = ('%4G', '%V')
        else:
            expected_components = ('%4Y', '%m', '%d')
        assert self._all_in(
            self._date_str, expected_components
        ), f'Must specify all date components: {self._format_str}'

    def __repr__(self):
        return f'{self.__class__.__name__}(\'{self._format_str}\')'

    @functools.singledispatchmethod
    def format(self, dt : datetime) -> str:
        """Apply the specified ISO8601 format to a datetime."""
        return (
            f'{format(dt, self._date_str)}{self._sep}'
            + f'{self._time_formatter(dt)}{self._tz_formatter(dt)}'
        )

    @format.register
    def _(self, dt: date) -> str:
        return f'{format(dt, self._date_str)}'

    @format.register
    def _(self, dt: time) -> str:
        return f'{self._time_formatter(dt)}

    def truncate(self, dt):
        """Truncate a datetime to the precision level of the format."""
        truncator = {}
        if 'W' in self._date_str and '%u' not in self._date_str:
            iso_year, week, weekday = dt.isocalendar()
            if weekday != 1:
                truncated_dt = datetime.fromisocalendar(iso_year, week, 1)
                for comp in ('year', 'month', 'day'):
                    if getattr(dt, comp) != (
                        new_comp := getattr(truncated_dt, comp)
                    ):
                        truncator[comp] = new_comp

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
            if self._tz_str == 'Z':
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
        time_elements = ('%(f', '%S', '%M', '%H')
        truncation_elements = (None, self._SECOND, self._MINUTE, self._HOUR)

        truncation = None
        for i, elem in enumerate(time_elements):
            if elem in time_str:
                assert self._all_in(
                    time_str, time_elements[(i + 1) :]
                ), f'Invalid time str: {time_str}'
                truncation = truncation_elements[i]
                break
        else:
            assert False, f'Invalid time str: {time_str}'

        if (m := self._FLOAT_RE.search(time_str)) is not None:
            time_str = time_str[: m.start()]

            precision = int(m.group('prec'))
            assert precision > 0, '0 and negative precision is not supported'

            truncation = timedelta(microseconds=10 ** (6 - min(6, precision)))

            def format_time(dt, *, time_str=time_str, precision=precision):
                if precision < 7:
                    return (
                        format(dt, time_str)
                        + f'{dt.microsecond:06d}'[0:precision]
                    )
                else:
                    return (
                        format(dt, time_str)
                        + f'{dt.microsecond:06d}'
                        + '0' * (precision - 6)
                    )

        else:

            def format_time(dt, *, time_str=time_str):
                return format(dt, time_str)

        return format_time, truncation

    _ARBITRARY_DT = datetime(2000, 1, 1)

    def _make_tz_formatter(self, base_formatter):
        def tz_formatter(dt, *, _self=self, _base_formatter=base_formatter):
            if dt.tzinfo is None:
                return ''
            utcoffset = dt.utcoffset()

            t = self._ARBITRARY_DT + abs(utcoffset)

            sign = '+' if utcoffset >= _self._ZERO else '-'

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
                    for component in ('hour', 'minute', 'second', 'microsecond')
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
                return {'tzinfo': new_tzinfo}

        return tz_truncator

    def _null_formatter(self, dt):
        return ''

    def _remove_tzinfo_truncator(self, dt):
        if dt.tzinfo is not None:
            return {'tzinfo': None}
        return {}

    def _tz_z_formatter(self, dt):
        if dt.tzinfo is None:
            return ''

        utcoffset = dt.utcoffset()

        if utcoffset == timedelta(0):
            return 'Z'

        hours, rem = divmod(utcoffset, timedelta(hours=1))

        rv = f'{hours:+03d}'
        if not rem:
            return rv

        minutes, rem = divmod(rem, timedelta(minutes=1))
        rv += f':{rem.total_seconds():02f}'
        if not rem:
            return rv

        microseconds = rem // timedelta(microseconds=1)
        rv += f'.{microseconds:06d}'
        return rv

    @staticmethod
    def _all_in(string, substrings):
        for substring in substrings:
            if substring not in string:
                return False
        return True


DEFAULT_DT = datetime(2025, 1, 2, 3, 4, 5, 678901)
AWARE_UTC_DT = datetime(2025, 1, 2, 3, 4, 5, 678901, tzinfo=timezone.utc)
AWARE_POS_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(timedelta(hours=3))
)
AWARE_NEG_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(-timedelta(hours=3))
)


class IsoFormatTest(unittest.TestCase):
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
    # fmt: on
    def test_fromisoformat(self, dt, iso_formatter):

        if "%G" in iso_formatter._format_str:
            if (
                iso_formatter._format_str.startswith("%G-W%V-%u")
                and len(iso_formatter._format_str) > 9
            ):
                hypothesis.assume(not iso_formatter._format_str[9].isdigit())

        input_str = iso_formatter.format(dt)
        actual = datetime.fromisoformat(input_str)
        expected = iso_formatter.truncate(dt)

        self.assertEqual(
            actual,
            expected,
            f"\n{actual} != {expected}\n"
            + f"actual = {actual!r}\n"
            + f"expected = {expected!r} \n"
            + f"input_str = {input_str}",
        )
