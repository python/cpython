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
        modname = modname[1:-1]
        pkgdir = os.path.join(STDLIB_DIR, *modname.split('.'))
        pyfile = os.path.join(pkgdir, '__init__.py')
    else:
        pyfile = os.path.join(STDLIB_DIR, *modname.split('.')) + '.py'
    destfile = os.path.join(destdir, 'frozen_' + modname.replace('.', '_')) + '.h'
    tmpfile = destfile + '.new'

    argv = [TOOL, modname, pyfile, tmpfile]
    print('#', ' '.join(argv))
    subprocess.run(argv, check=True)

    os.replace(tmpfile, destfile)


def main():
    for modname in STARTUP_MODULES_WITHOUT_SITE:
        freeze_module(modname, MODULES_DIR)
    for modname in STARTUP_MODULES_WITH_SITE:
        freeze_module(modname, MODULES_DIR)


if __name__ == '__main__':
    main()
