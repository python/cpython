from datetime import tzinfo

class UTC(tzinfo):
    """UTC"""

    def utcoffset(self, dt):
        return 0

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return 0

class FixedOffset(tzinfo):
    """Fixed offset in minutes east from UTC."""

    def __init__(self, offset, name):
        self.__offset = offset
        self.__name = name

    def utcoffset(self, dt):
        return self.__offset

    def tzname(self, dt):
        return self.__name

    def dst(self, dt):
        # It depends on more than we know in an example.
        return None # Indicate we don't know

import time

class LocalTime(tzinfo):
    """Local time as defined by the operating system."""

    def _isdst(self, dt):
        t = (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
             -1, -1, -1)
        # XXX This may fail for years < 1970 or >= 2038
        t = time.localtime(time.mktime(t))
        return t.tm_isdst > 0

    def utcoffset(self, dt):
        if self._isdst(dt):
            return -time.timezone/60
        else:
            return -time.altzone/60

    def tzname(self, dt):
        return time.tzname[self._isdst(dt)]
