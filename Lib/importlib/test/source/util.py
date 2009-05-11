from .. import util
import contextlib
import functools
import imp
import os
import os.path
import sys
import tempfile
from test import support


def writes_bytecode_files(fxn):
    """Decorator to protect sys.dont_write_bytecode from mutation and to skip
    tests that require it to be set to False."""
    if sys.dont_write_bytecode:
        return lambda *args, **kwargs: None
    @functools.wraps(fxn)
    def wrapper(*args, **kwargs):
        original = sys.dont_write_bytecode
        sys.dont_write_bytecode = False
        try:
            to_return = fxn(*args, **kwargs)
        finally:
            sys.dont_write_bytecode = original
        return to_return
    return wrapper


def bytecode_path(source_path):
    for suffix, _, type_ in imp.get_suffixes():
        if type_ == imp.PY_COMPILED:
            bc_suffix = suffix
            break
    else:
        raise ValueError("no bytecode suffix is defined")
    return os.path.splitext(source_path)[0] + bc_suffix


@contextlib.contextmanager
def create_modules(*names):
    """Temporarily create each named module with an attribute (named 'attr')
    that contains the name passed into the context manager that caused the
    creation of the module.

    All files are created in a temporary directory specified by
    tempfile.gettempdir(). This directory is inserted at the beginning of
    sys.path. When the context manager exits all created files (source and
    bytecode) are explicitly deleted.

    No magic is performed when creating packages! This means that if you create
    a module within a package you must also create the package's __init__ as
    well.

    """
    source = 'attr = {0!r}'
    created_paths = []
    mapping = {}
    try:
        temp_dir = tempfile.gettempdir()
        mapping['.root'] = temp_dir
        import_names = set()
        for name in names:
            if not name.endswith('__init__'):
                import_name = name
            else:
                import_name = name[:-len('.__init__')]
            import_names.add(import_name)
            if import_name in sys.modules:
                del sys.modules[import_name]
            name_parts = name.split('.')
            file_path = temp_dir
            for directory in name_parts[:-1]:
                file_path = os.path.join(file_path, directory)
                if not os.path.exists(file_path):
                    os.mkdir(file_path)
                    created_paths.append(file_path)
            file_path = os.path.join(file_path, name_parts[-1] + '.py')
            with open(file_path, 'w') as file:
                file.write(source.format(name))
            created_paths.append(file_path)
            mapping[name] = file_path
        uncache_manager = util.uncache(*import_names)
        uncache_manager.__enter__()
        state_manager = util.import_state(path=[temp_dir])
        state_manager.__enter__()
        yield mapping
    finally:
        state_manager.__exit__(None, None, None)
        uncache_manager.__exit__(None, None, None)
        # Reverse the order for path removal to unroll directory creation.
        for path in reversed(created_paths):
            if file_path.endswith('.py'):
                support.unlink(path)
                support.unlink(path + 'c')
                support.unlink(path + 'o')
            else:
                os.rmdir(path)
