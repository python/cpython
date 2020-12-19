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
