import sys


OS = sys.platform


def _as_tuple(items):
    if isinstance(items, str):
        return tuple(items.strip().replace(',', ' ').split())
    elif items:
        return tuple(items)
    else:
        return ()


class PreprocessorError(Exception):
    """Something preprocessor-related went wrong."""

    @classmethod
    def _msg(cls, filename, reason, **ignored):
        msg = 'failure while preprocessing'
        if reason:
            msg = f'{msg} ({reason})'
        return msg

    def __init__(self, filename, preprocessor=None, reason=None):
        if isinstance(reason, str):
            reason = reason.strip()

        self.filename = filename
        self.preprocessor = preprocessor or None
        self.reason = str(reason) if reason else None

        msg = self._msg(**vars(self))
        msg = f'({filename}) {msg}'
        if preprocessor:
            msg = f'[{preprocessor}] {msg}'
        super().__init__(msg)


class PreprocessorFailure(PreprocessorError):
    """The preprocessor command failed."""

    @classmethod
    def _msg(cls, error, **ignored):
        msg = 'preprocessor command failed'
        if error:
            msg = f'{msg} {error}'
        return msg

    def __init__(self, filename, argv, error=None, preprocessor=None):
        exitcode = -1
        if isinstance(error, tuple):
            if len(error) == 2:
                error, exitcode = error
            else:
                error = str(error)
        if isinstance(error, str):
            error = error.strip()

        self.argv = _as_tuple(argv) or None
        self.error = error if error else None
        self.exitcode = exitcode

        reason = str(self.error)
        super().__init__(filename, preprocessor, reason)


class ErrorDirectiveError(PreprocessorFailure):
    """The file hit a #error directive."""

    @classmethod
    def _msg(cls, error, **ignored):
        return f'#error directive hit ({error})'

    def __init__(self, filename, argv, error, *args, **kwargs):
        super().__init__(filename, argv, error, *args, **kwargs)


class MissingDependenciesError(PreprocessorFailure):
    """The preprocessor did not have access to all the target's dependencies."""

    @classmethod
    def _msg(cls, missing, **ignored):
        msg = 'preprocessing failed due to missing dependencies'
        if missing:
            msg = f'{msg} ({", ".join(missing)})'
        return msg

    def __init__(self, filename, missing=None, *args, **kwargs):
        self.missing = _as_tuple(missing) or None

        super().__init__(filename, *args, **kwargs)


class OSMismatchError(MissingDependenciesError):
    """The target is not compatible with the host OS."""

    @classmethod
    def _msg(cls, expected, **ignored):
        return f'OS is {OS} but expected {expected or "???"}'

    def __init__(self, filename, expected=None, *args, **kwargs):
        if isinstance(expected, str):
            expected = expected.strip()

        self.actual = OS
        self.expected = expected if expected else None

        super().__init__(filename, None, *args, **kwargs)
