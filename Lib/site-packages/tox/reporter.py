"""A progress reporter inspired from the logging modules"""
from __future__ import absolute_import, unicode_literals

import os
import time
from contextlib import contextmanager
from datetime import datetime

import py


class Verbosity(object):
    DEBUG = 2
    INFO = 1
    DEFAULT = 0
    QUIET = -1
    EXTRA_QUIET = -2


REPORTER_TIMESTAMP_ON_ENV = str("TOX_REPORTER_TIMESTAMP")
REPORTER_TIMESTAMP_ON = os.environ.get(REPORTER_TIMESTAMP_ON_ENV, False) == "1"
START = datetime.now()


class Reporter(object):
    def __init__(self, verbose_level=None, quiet_level=None):
        kwargs = {}
        if verbose_level is not None:
            kwargs["verbose_level"] = verbose_level
        if quiet_level is not None:
            kwargs["quiet_level"] = quiet_level
        self._reset(**kwargs)

    def _reset(self, verbose_level=0, quiet_level=0):
        self.verbose_level = verbose_level
        self.quiet_level = quiet_level
        self.reported_lines = []
        self.tw = py.io.TerminalWriter()

    @property
    def verbosity(self):
        return self.verbose_level - self.quiet_level

    def log_popen(self, cwd, outpath, cmd_args_shell, pid):
        """ log information about the action.popen() created process. """
        msg = "[{}] {}$ {}".format(pid, cwd, cmd_args_shell)
        if outpath:
            if outpath.common(cwd) is not None:
                outpath = cwd.bestrelpath(outpath)
            msg = "{} >{}".format(msg, outpath)
        self.verbosity1(msg, of="logpopen")

    @property
    def messages(self):
        return [i for _, i in self.reported_lines]

    @contextmanager
    def timed_operation(self, name, msg):
        self.verbosity2("{} start: {}".format(name, msg), bold=True)
        start = time.time()
        yield
        duration = time.time() - start
        self.verbosity2(
            "{} finish: {} after {:.2f} seconds".format(name, msg, duration),
            bold=True,
        )

    def separator(self, of, msg, level):
        if self.verbosity >= level:
            self.reported_lines.append(("separator", "- summary -"))
            self.tw.sep(of, msg)

    def logline_if(self, level, of, msg, key=None, **kwargs):
        if self.verbosity >= level:
            message = str(msg) if key is None else "{}{}".format(key, msg)
            self.logline(of, message, **kwargs)

    def logline(self, of, msg, **opts):
        self.reported_lines.append((of, msg))
        timestamp = ""
        if REPORTER_TIMESTAMP_ON:
            timestamp = "{} ".format(datetime.now() - START)
        line_msg = "{}{}\n".format(timestamp, msg)
        self.tw.write(line_msg, **opts)

    def keyvalue(self, name, value):
        if name.endswith(":"):
            name += " "
        self.tw.write(name, bold=True)
        self.tw.write(value)
        self.tw.line()

    def line(self, msg, **opts):
        self.logline("line", msg, **opts)

    def info(self, msg):
        self.logline_if(Verbosity.DEBUG, "info", msg)

    def using(self, msg):
        self.logline_if(Verbosity.INFO, "using", msg, "using ", bold=True)

    def good(self, msg):
        self.logline_if(Verbosity.QUIET, "good", msg, green=True)

    def warning(self, msg):
        self.logline_if(Verbosity.QUIET, "warning", msg, "WARNING: ", red=True)

    def error(self, msg):
        self.logline_if(Verbosity.QUIET, "error", msg, "ERROR: ", red=True)

    def skip(self, msg):
        self.logline_if(Verbosity.QUIET, "skip", msg, "SKIPPED: ", yellow=True)

    def verbosity0(self, msg, **opts):
        self.logline_if(Verbosity.DEFAULT, "verbosity0", msg, **opts)

    def verbosity1(self, msg, of="verbosity1", **opts):
        self.logline_if(Verbosity.INFO, of, msg, **opts)

    def verbosity2(self, msg, **opts):
        self.logline_if(Verbosity.DEBUG, "verbosity2", msg, **opts)

    def quiet(self, msg):
        self.logline_if(Verbosity.QUIET, "quiet", msg)


_INSTANCE = Reporter()


def update_default_reporter(quiet_level, verbose_level):
    _INSTANCE.quiet_level = quiet_level
    _INSTANCE.verbose_level = verbose_level


def has_level(of):
    return _INSTANCE.verbosity > of


def verbosity():
    return _INSTANCE.verbosity


verbosity0 = _INSTANCE.verbosity0
verbosity1 = _INSTANCE.verbosity1
verbosity2 = _INSTANCE.verbosity2
error = _INSTANCE.error
warning = _INSTANCE.warning
good = _INSTANCE.good
using = _INSTANCE.using
skip = _INSTANCE.skip
info = _INSTANCE.info
line = _INSTANCE.line
separator = _INSTANCE.separator
keyvalue = _INSTANCE.keyvalue
quiet = _INSTANCE.quiet
timed_operation = _INSTANCE.timed_operation
log_popen = _INSTANCE.log_popen
