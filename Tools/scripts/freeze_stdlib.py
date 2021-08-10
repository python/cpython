import os
import os.path
import subprocess


SCRIPTS_DIR = os.path.dirname(__file__)
TOOLS_DIR = os.path.dirname(SCRIPTS_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)
STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
MODULES_DIR = os.path.join(ROOT_DIR, 'Python')
#MODULES_DIR = os.path.join(ROOT_DIR, 'Modules')
TOOL = os.path.join(ROOT_DIR, 'Programs', '_freeze_module')

STARTUP_MODULES_WITHOUT_SITE = [
]
STARTUP_MODULES_WITH_SITE = [
]


def freeze_module(modname, destdir):
    if modname.startswith('<'):
        pkgname = modname[1:-1]
        pkgname, sep, match= pkgname.partition('.')
        pyfile, _ = _freeze_module(pkgname, MODULES_DIR, ispkg=True)

        if sep:
            pkgdir = os.path.dirname(pyfile)
            for modname, ispkg in _iter_submodules(pkgname, pkgdir, match):
                _freeze_module(modname, MODULES_DIR, ispkg)
    else:
        _freeze_module(modname, MODULES_DIR)


def _freeze_module(modname, destdir, ispkg=False):
    if ispkg:
        pyfile = os.path.join(STDLIB_DIR, *modname.split('.'), '__init__.py')
    else:
        pyfile = os.path.join(STDLIB_DIR, *modname.split('.')) + '.py'
    destfile = os.path.join(destdir, 'frozen_' + modname.replace('.', '_')) + '.h'
    tmpfile = destfile + '.new'

    argv = [TOOL, modname, pyfile, tmpfile]
    print('#', ' '.join(argv))
    subprocess.run(argv, check=True)

    os.replace(tmpfile, destfile)

    return pyfile, destfile


def _iter_submodules(pkgname, pkgdir, match='*'):
    if not match:
        match = '*'
    if match != '*':
        raise NotImplementedError(match)

    for entry in os.scandir(pkgdir):
        modname = f'{pkgname}.{entry.name}'
        if modname.endswith('.py'):
            yield modname[:-3], False
        elif entry.is_dir():
            if os.path.exists(os.path.join(entry.path, '__init__.py')):
                yield modname, True
                yield from _iter_submodules(modname, entry.path)


def main():
    for modname in STARTUP_MODULES_WITHOUT_SITE:
        freeze_module(modname, MODULES_DIR)
    for modname in STARTUP_MODULES_WITH_SITE:
        freeze_module(modname, MODULES_DIR)


if __name__ == '__main__':
    main()
