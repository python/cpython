from datetime import date, datetime, time, timedelta, timezone
import itertools

from test.support.hypothesis_helper import hypothesis

from .isoformatter import IsoFormatter

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
