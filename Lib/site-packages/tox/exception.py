import os
import pipes
import signal


def exit_code_str(exception_name, command, exit_code):
    """String representation for an InvocationError, with exit code

    NOTE: this might also be used by plugin tests (tox-venv at the time of writing),
    so some coordination is needed if this is ever moved or a different solution for this hack
    is found.

    NOTE: this is a separate function because pytest-mock `spy` does not work on Exceptions
    We can use neither a class method nor a static because of https://bugs.python.org/issue23078.
    Even a normal method failed with "TypeError: descriptor '__getattribute__' requires a
    'BaseException' object but received a 'type'".
    """
    str_ = "{} for command {}".format(exception_name, command)
    if exit_code is not None:
        if exit_code < 0 or (os.name == "posix" and exit_code > 128):
            signals = {
                number: name for name, number in vars(signal).items() if name.startswith("SIG")
            }
            if exit_code < 0:
                # Signal reported via subprocess.Popen.
                sig_name = signals.get(-exit_code)
                str_ += " (exited with code {:d} ({}))".format(exit_code, sig_name)
            else:
                str_ += " (exited with code {:d})".format(exit_code)
                number = exit_code - 128
                name = signals.get(number)
                if name:
                    str_ += (
                        ")\nNote: this might indicate a fatal error signal "
                        "({:d} - 128 = {:d}: {})".format(exit_code, number, name)
                    )
        str_ += " (exited with code {:d})".format(exit_code)
    return str_


class Error(Exception):
    def __str__(self):
        return "{}: {}".format(self.__class__.__name__, self.args[0])


class MissingSubstitution(Error):
    FLAG = "TOX_MISSING_SUBSTITUTION"
    """placeholder for debugging configurations"""

    def __init__(self, name):
        self.name = name


class ConfigError(Error):
    """Error in tox configuration."""


class UnsupportedInterpreter(Error):
    """Signals an unsupported Interpreter."""


class InterpreterNotFound(Error):
    """Signals that an interpreter could not be found."""


class InvocationError(Error):
    """An error while invoking a script."""

    def __init__(self, command, exit_code=None, out=None):
        super(Error, self).__init__(command, exit_code)
        self.command = command
        self.exit_code = exit_code
        self.out = out

    def __str__(self):
        return exit_code_str(self.__class__.__name__, self.command, self.exit_code)


class MissingDirectory(Error):
    """A directory did not exist."""


class MissingDependency(Error):
    """A dependency could not be found or determined."""


class MissingRequirement(Error):
    """A requirement defined in :config:`require` is not met."""

    def __init__(self, config):
        self.config = config

    def __str__(self):
        return " ".join(pipes.quote(i) for i in self.config.requires)


class BadRequirement(Error):
    """A requirement defined in :config:`require` cannot be parsed."""
