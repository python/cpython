"""
Logging support for make_layout.
"""

__author__ = "Jeremy Paige <jeremyp@activestate.com>"
__version__ = "2.7"

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
    handler.setFormatter(logging.Formatter("%(levelname)s %(message)s"))
    handler.setLevel(s_level)
    LOG.addHandler(handler)

    if ns.log:
        handler = logging.FileHandler(ns.log, encoding="utf-8", delay=True)
        handler.setFormatter(
            logging.Formatter("[%(asctime)s]%(levelname)s: %(message)s")
        )
        handler.setLevel(f_level)
        LOG.addHandler(handler)


@public
def log_debug(msg, *args, **kwargs):
    return LOG.debug(msg, *args, **kwargs)


@public
def log_info(msg, *args, **kwargs):
    return LOG.info(msg, *args, **kwargs)


@public
def log_warning(msg, *args, **kwargs):
    return LOG.warning(msg, *args, **kwargs)


@public
def log_error(msg, *args, **kwargs):
    global HAS_ERROR
    HAS_ERROR = True
    return LOG.error(msg, *args, **kwargs)


@public
def log_exception(msg, *args, **kwargs):
    global HAS_ERROR
    HAS_ERROR = True
    return LOG.exception(msg, *args, **kwargs)


@public
def error_was_logged():
    return HAS_ERROR
