import sys


def _fix_import_path():
    """This will remove the local inport path ''

    This will remove the local import path '', now using at idlelib/pyshell.py
    and idlelib/run.py. When running with pyshell.main(), it will add back the
    local import path when dealing with args. So don't need to worry about local
    import problem.
    """
    if '' in sys.path:
        sys.path.remove('')
