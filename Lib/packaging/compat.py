"""Support for build-time 2to3 conversion."""

from packaging import logger


# XXX Having two classes with the same name is not a good thing.
# XXX 2to3-related code should move from util to this module

try:
    from packaging.util import Mixin2to3 as _Mixin2to3
    _CONVERT = True
    _KLASS = _Mixin2to3
except ImportError:
    _CONVERT = False
    _KLASS = object

__all__ = ['Mixin2to3']


class Mixin2to3(_KLASS):
    """ The base class which can be used for refactoring. When run under
    Python 3.0, the run_2to3 method provided by Mixin2to3 is overridden.
    When run on Python 2.x, it merely creates a class which overrides run_2to3,
    yet does nothing in particular with it.
    """
    if _CONVERT:

        def _run_2to3(self, files=[], doctests=[], fixers=[]):
            """ Takes a list of files and doctests, and performs conversion
            on those.
              - First, the files which contain the code(`files`) are converted.
              - Second, the doctests in `files` are converted.
              - Thirdly, the doctests in `doctests` are converted.
            """
            if fixers:
                self.fixer_names = fixers

            if files:
                logger.info('converting Python code and doctests')
                _KLASS.run_2to3(self, files)
                _KLASS.run_2to3(self, files, doctests_only=True)

            if doctests:
                logger.info('converting doctests in text files')
                _KLASS.run_2to3(self, doctests, doctests_only=True)
    else:
        # If run on Python 2.x, there is nothing to do.

        def _run_2to3(self, files=[], doctests=[], fixers=[]):
            pass
