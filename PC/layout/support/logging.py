"""
Logging support for make_layout.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import logging
import sys

__all__ = []

LOG = None
HAS_ERROR = False


def public(f):
    __all__.append(f.__name__)
    return f


@public
def configure_logger(ns):
    global LOG
    if LOG:
        return

    LOG = logging.getLogger("make_layout")
    LOG.level = logging.DEBUG

    if ns.v:
        s_level = max(logging.ERROR - ns.v * 10, logging.DEBUG)
        f_level = max(logging.WARNING - ns.v * 10, logging.DEBUG)
    else:
        s_level = logging.ERROR
        f_level = logging.INFO

    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(logging.Formatter("{levelname:8s} {message}", style="{"))
    handler.setLevel(s_level)
    LOG.addHandler(handler)

    if ns.log:
        handler = logging.FileHandler(ns.log, encoding="utf-8", delay=True)
        handler.setFormatter(
            logging.Formatter("[{asctime}]{levelname:8s}: {message}", style="{")
        )
        handler.setLevel(f_level)
        LOG.addHandler(handler)


class BraceMessage:
    def __init__(self, fmt, *args, **kwargs):
        self.fmt = fmt
        self.args = args
        self.kwargs = kwargs

    def __str__(self):
        return self.fmt.format(*self.args, **self.kwargs)


@public
def log_debug(msg, *args, **kwargs):
    return LOG.debug(BraceMessage(msg, *args, **kwargs))


@public
def log_info(msg, *args, **kwargs):
    return LOG.info(BraceMessage(msg, *args, **kwargs))


@public
def log_warning(msg, *args, **kwargs):
    return LOG.warning(BraceMessage(msg, *args, **kwargs))


@public
def log_error(msg, *args, **kwargs):
    global HAS_ERROR
    HAS_ERROR = True
    return LOG.error(BraceMessage(msg, *args, **kwargs))


@public
def log_exception(msg, *args, **kwargs):
    global HAS_ERROR
    HAS_ERROR = True
    return LOG.exception(BraceMessage(msg, *args, **kwargs))


@public
def error_was_logged():
    return HAS_ERROR
