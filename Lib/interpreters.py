"""Interpreters module providing a high-level API to Python interpreters."""

import _interpreters


__all__ = ['enumerate', 'current', 'main', 'create', 'Interpreter']


def enumerate():
    """Return a list of all existing interpreters."""
    return [Interpreter(id)
            for id in _interpreters.enumerate()]


def current():
    """Return the currently running interpreter."""
    id = _interpreters.get_current()
    return Interpreter(id)


def main():
    """Return the main interpreter."""
    id = _interpreters.get_main()
    return Interpreter(id)


def create():
    """Return a new Interpreter.

    The interpreter is created in the current OS thread.  It will remain
    idle until its run() method is called.
    """
    id = _interpreters.create()
    return Interpreter(id)


class Interpreter:
    """A single Python interpreter in the current runtime."""

    def __init__(self, id):
        self._id = id

    def __repr__(self):
        return '{}(id={!r})'.format(type(self).__name__, self._id)

    def __eq__(self, other):
        try:
            other_id = other.id
        except AttributeError:
            return False
        return self._id == other_id

    @property
    def id(self):
        """The interpreter's ID."""
        return self._id

    def is_running(self):
        """Return whether or not the interpreter is currently running."""
        return _interpreters.is_running(self._id)

    def destroy(self):
        """Finalize and destroy the interpreter.
        
        After calling destroy(), all operations on this interpreter
        will fail.
        """
        _interpreters.destroy(self._id)

    def run(self, code):
        """Run the provided Python code in this interpreter.

        If the code is a string then it will be run as it would by
        calling exec().  Other kinds of code are not supported.
        """
        if isinstance(code, str):
            _interpreters.run_string(self._id, code)
        else:
            raise NotImplementedError
