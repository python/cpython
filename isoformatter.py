import functools
import itertools

from datetime import date, datetime, time, timedelta, timezone

# import hypothesis
from test.support.hypothesis_helper import hypothesis

from test.isoformat_helper import IsoFormatter

import unittest


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
    ).map(lambda x: IsoFormatter("".join((x[0],) + x[1])))


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


DEFAULT_D = date(2025, 1, 2)
DEFAULT_DT = datetime(2025, 1, 2, 3, 4, 5, 678901)
AWARE_UTC_DT = datetime(2025, 1, 2, 3, 4, 5, 678901, tzinfo=timezone.utc)
AWARE_POS_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(timedelta(hours=3))
)
AWARE_NEG_DT = datetime(
    2025, 1, 2, 3, 5, 6, 678901, tzinfo=timezone(-timedelta(hours=3))
)


def _cross_product_examples(**kwargs):
    params, values = zip(*kwargs.items())

    example_stack = []
    for value_set in itertools.product(*values):
        example_stack.append(hypothesis.example(**dict(zip(params, value_set))))

    return functools.reduce(lambda a, b: a(b), example_stack)


class IsoFormatTest(unittest.TestCase):
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
        d=[date(2025, 1, 2), date(2025, 12, 31), date(2023, 1, 1)],
        iso_formatter=map(
            IsoFormatter, ["%G-W%V", "%GW%V", "%G-W%V-%u", "%GW%V%u"]
        ),
    )
    def test_dates(self, d, iso_formatter):
        input_str = iso_formatter.format(d)
        actual = type(d).fromisoformat(input_str)
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
                "%H:%M:%S",
                "%H%M%S",
                "%H:%M:%S.%f",
                "%H%M%S.%f",
                "%H:%M:%S[TZ:%H:%M]",
                "%H:%M:%S[TZ:%H%M]",
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
    def test_times(self, t, iso_formatter):
        input_str = iso_formatter.format(t)
        actual = type(t).fromisoformat(input_str)
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

    @unittest.skip("Broken atm")
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
