import os
import os.path
import subprocess
import sys


SCRIPTS_DIR = os.path.abspath(os.path.dirname(__file__))
TOOLS_DIR = os.path.dirname(SCRIPTS_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)
STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
MODULES_DIR = os.path.join(ROOT_DIR, 'Python')
#MODULES_DIR = os.path.join(ROOT_DIR, 'Modules')
TOOL = os.path.join(ROOT_DIR, 'Programs', '_freeze_module')

# These are modules that get frozen.
FROZEN = {
    # frozen_id: (pyfile, frozenfile)
    # frozen_id: pyfile

    # importlib
    'importlib._bootstrap': (None, 'importlib.h'),
    'importlib._bootstrap_external': (None, 'importlib_external.h'),
    'zipimport': (None, 'importlib_zipimport.h'),
    # stdlib
    # ...
    # test
    'hello': os.path.join(TOOLS_DIR, 'freeze', 'flag.py'),
}
FROZEN_GROUPS = {
    'importlib': [
        'importlib._bootstrap',
        'importlib._bootstrap_external',
        'zipimport',
    ],
    'stdlib': [
        # without site (python -S):

        # with site:

    ],
    'test': [
        'hello',
    ],
}


def expand_frozen(destdir=MODULES_DIR):
    # First, expand FROZEN.
    frozen = {}
    packages = {}
    for frozenid, info in FROZEN.items():
        if frozenid.startswith('<'):
            assert info is None, (frozenid, info)
            modids = packages[frozenid] = []
            pkgname = frozenid[1:-1]
            _pkgname, sep, match = pkgname.rpartition('.')
            if not sep or match.isidentifier():
                frozen[pkgname] = (None, None, True)
                modids.append(pkgname)
            else:
                pkgname = _pkgname
                frozen[pkgname] = (None, None, True)
                modids.append(okgname)
                for subname, ispkg in _iter_submodules(pkgname, match=match):
                    frozen[subname] = (None, None, ispkg)
                    modids.append(subname)
        else:
            if info is None or isinstance(info, str):
                info = (info, None, False)
            else:
                pyfile, frozenfile = info
                info = (pyfile, frozenfile, False)
            frozen[frozenid] = info
    for frozenid, info in frozen.items():
        pyfile, frozenfile, ispkg = info

        if not pyfile:
            pyfile = _resolve_module(frozenid, ispkg)
            info = (pyfile, frozenfile, ispkg)

        if not frozenfile:
            frozenfile = _resolve_frozen(frozenid, destdir)
#            frozenfile = f'frozen_{frozenid}'
#            frozenfile = os.path.join(destdir, frozenfile)
            info = (pyfile, frozenfile, ispkg)
        elif not os.path.isabs(frozenfile):
            frozenfile = os.path.join(destdir, frozenfile)
            info = (pyfile, frozenfile, ispkg)

        frozen[frozenid] = info

    # Then, expand FROZEN_GROUPS.
    groups = {}
    for groupname, frozenids in FROZEN_GROUPS.items():
        group = groups[groupname] = []
        for frozenid in frozenids:
            if frozenid in packages:
                group.extend(packages[frozenid])
            else:
                group.append(frozenid)

    return groups, frozen


#def freeze_stdlib_module(modname, destdir=MODULES_DIR):
#    if modname.startswith('<'):
#        pkgname = modname[1:-1]
#        pkgname, sep, match = pkgname.rpartition('.')
#        if not sep or match.isidentifier():
#            pyfile = _resolve_module(pkgname, ispkg=True)
#            frozenfile = _resolve_frozen(pkgname, destdir)
#            _freeze_module(pkgname, pyfile, frozenfile)
#        else:
#            pkgdir = os.path.dirname(pyfile)
#            for subname, ispkg in _iter_submodules(pkgname, pkgdir, match):
#                pyfile = _resolve_module(subname, ispkg)
#                frozenfile = _resolve_frozen(subname, destdir)
#                _freeze_module(subname, pyfile, frozenfile)
#    else:
#        pyfile = _resolve_module(modname, ispkg=False)
#        frozenfile = _resolve_frozen(modname, destdir)
#        _freeze_module(modname, pyfile, frozenfile)


def _freeze_module(frozenid, pyfile, frozenfile):
    tmpfile = frozenfile + '.new'

    argv = [TOOL, frozenid, pyfile, tmpfile]
    print('#', ' '.join(argv))
    subprocess.run(argv, check=True)

    os.replace(tmpfile, frozenfile)


def _resolve_module(modname, ispkg=False):
    if ispkg:
        return os.path.join(STDLIB_DIR, *modname.split('.'), '__init__.py')
    return os.path.join(STDLIB_DIR, *modname.split('.')) + '.py'


def _resolve_frozen(modname, destdir):
    return os.path.join(destdir, 'frozen_' + modname.replace('.', '_')) + '.h'


def _iter_submodules(pkgname, pkgdir=None, match='*'):
    if not pkgdir:
        pkgdir = os.path.join(STDLIB_DIR, *pkgname.split('.'))
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


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    KINDS = ['all', *sorted(FROZEN_GROUPS)]
    usage = f'usage: {prog} [-h] [{"|".join(KINDS)}] ...'
    if '-h' in argv or '--help' in argv:
        print(usage)
        sys.exit(0)
    else:
        kinds = set()
        for arg in argv:
            if arg not in KINDS:
                print(usage, file=sys.stderr)
                sys.exit(f'ERROR: unsupported kind {arg!r}')
            kinds.add(arg)
    return {'kinds': kinds if kinds and 'all' not in kinds else None}


def main(kinds=None):
    groups, frozen = expand_frozen(MODULES_DIR)

    # First, freeze the modules.
    # (We use a consistent order:)
    ordered = ['importlib', 'stdlib', 'test']
    assert (set(ordered) == set(groups))
    for kind in ordered:
        if not kinds or kind in kinds:
            for frozenid in groups[kind]:
                #freeze_module(frozenid, frozen)
                pyfile, frozenfile, _ = frozen[frozenid]
                _freeze_module(frozenid, pyfile, frozenfile)


if __name__ == '__main__':
    kwargs = parse_args()
    main(**kwargs)
