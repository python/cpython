import logging
import sys


VERBOSITY = 3


# The root logger for the whole top-level package:
_logger = logging.getLogger(__name__.rpartition('.')[0])


def configure_logger(logger, verbosity=VERBOSITY, *,
                     logfile=None,
                     maxlevel=logging.CRITICAL,
                     ):
    level = max(1,  # 0 disables it, so we use the next lowest.
                min(maxlevel,
                    maxlevel - verbosity * 10))
    logger.setLevel(level)
    #logger.propagate = False

    if not logger.handlers:
        if logfile:
            handler = logging.FileHandler(logfile)
        else:
            handler = logging.StreamHandler(sys.stdout)
        handler.setLevel(level)
        #handler.setFormatter(logging.Formatter())
        logger.addHandler(handler)

    # In case the provided logger is in a sub-package...
    if logger is not _logger:
        configure_logger(
            _logger,
            verbosity,
            logfile=logfile,
            maxlevel=maxlevel,
        )


def hide_emit_errors():
    """Ignore errors while emitting log entries.

    Rather than printing a message desribing the error, we show nothing.
    """
    # For now we simply ignore all exceptions.  If we wanted to ignore
    # specific ones (e.g. BrokenPipeError) then we would need to use
    # a Handler subclass with a custom handleError() method.
    orig = logging.raiseExceptions
    logging.raiseExceptions = False
    def restore():
        logging.raiseExceptions = orig
    return restore


class Printer:
    def __init__(self, verbosity=VERBOSITY):
        self.verbosity = verbosity

    def info(self, *args, **kwargs):
        if self.verbosity < 3:
            return
        print(*args, **kwargs)
