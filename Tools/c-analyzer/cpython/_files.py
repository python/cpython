import os.path

from c_common.fsutil import expand_filenames, iter_files_by_suffix
from . import REPO_ROOT, INCLUDE_DIRS, SOURCE_DIRS


GLOBS = [
    'Include/*.h',
    'Include/internal/*.h',
    'Modules/**/*.h',
    'Modules/**/*.c',
    'Objects/**/*.h',
    'Objects/**/*.c',
    'Python/**/*.h',
    'Parser/**/*.c',
    'Python/**/*.h',
    'Parser/**/*.c',
]
LEVEL_GLOBS = {
    'stable': 'Include/*.h',
    'cpython': 'Include/cpython/*.h',
    'internal': 'Include/internal/*.h',
}


def resolve_filename(filename):
    orig = filename
    filename = os.path.normcase(os.path.normpath(filename))
    if os.path.isabs(filename):
        if os.path.relpath(filename, REPO_ROOT).startswith('.'):
            raise Exception(f'{orig!r} is outside the repo ({REPO_ROOT})')
        return filename
    else:
        return os.path.join(REPO_ROOT, filename)


def iter_filenames(*, search=False):
    if search:
        yield from iter_files_by_suffix(INCLUDE_DIRS, ('.h',))
        yield from iter_files_by_suffix(SOURCE_DIRS, ('.c',))
    else:
        globs = (os.path.join(REPO_ROOT, file) for file in GLOBS)
        yield from expand_filenames(globs)


def iter_header_files(filenames=None, *, levels=None):
    if not filenames:
        if levels:
            levels = set(levels)
            if 'private' in levels:
                levels.add('stable')
                levels.add('cpython')
            for level, glob in LEVEL_GLOBS.items():
                if level in levels:
                    yield from expand_filenames([glob])
        else:
            yield from iter_files_by_suffix(INCLUDE_DIRS, ('.h',))
        return

    for filename in filenames:
        orig = filename
        filename = resolve_filename(filename)
        if filename.endswith(os.path.sep):
            yield from iter_files_by_suffix(INCLUDE_DIRS, ('.h',))
        elif filename.endswith('.h'):
            yield filename
        else:
            # XXX Log it and continue instead?
            raise ValueError(f'expected .h file, got {orig!r}')
