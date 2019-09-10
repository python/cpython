"""Sub-interpreters High Level Module."""

__all__ = ['create', 'list_all', 'get_current', 'get_main',
            'run_string', 'destroy']


import _interpreters

# Rename so that "from interpreters import *" is safe
_list_all = _interpreters.list_all
_get_current = _interpreters.get_current
_get_main = _interpreters.get_main
_run_string = _interpreters.run_string

# Global API functions

def create():
    """ create() -> Interpreter

    Create a new interpreter and return a unique generated ID.
    """
    return _interpreters.create()

def list_all():
    """list_all() -> [Interpreter]

    Return a list containing the ID of every existing interpreter.
    """
    return _list_all()

def get_current():
    """get_current() -> Interpreter

    Return the ID of the currently running interpreter.
    """
    return _get_current()

def get_main():
    """get_main() -> ID

    Return the ID of the main interpreter.
    """
    return _get_main()

def destroy(id):
    """destroy(id) -> None

    Destroy the identified interpreter.
    """
    return _interpreters.destroy(id)

def run_string(id, script, shared):
    """run_string(id, script, shared) -> None

    Execute the provided string in the identified interpreter.
    See PyRun_SimpleStrings.
    """
    return _run_string(id, script, shared)
