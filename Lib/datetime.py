try:
    from _datetime import *
    from _datetime import __doc__  # noqa
except ImportError:
    from _pydatetime import *
    from _pydatetime import __doc__  # noqa

__all__ = ("date", "datetime", "time", "timedelta", "timezone", "tzinfo",
           "MINYEAR", "MAXYEAR", "UTC")
