"""Freeze modules and regen related files (e.g. Python/frozen.c).

See the notes at the top of Python/frozen.c for more info.
"""

import os
import os.path
import subprocess
import sys
import textwrap

from update_file import updating_file_with_tmpfile


SCRIPTS_DIR = os.path.abspath(os.path.dirname(__file__))
TOOLS_DIR = os.path.dirname(SCRIPTS_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)

STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
# If MODULES_DIR is changed then the .gitattributes file needs to be updated.
MODULES_DIR = os.path.join(ROOT_DIR, 'Python/frozen_modules')
TOOL = os.path.join(ROOT_DIR, 'Programs', '_freeze_module')

FROZEN_FILE = os.path.join(ROOT_DIR, 'Python', 'frozen.c')
MAKEFILE = os.path.join(ROOT_DIR, 'Makefile.pre.in')
PCBUILD_PROJECT = os.path.join(ROOT_DIR, 'PCbuild', '_freeze_module.vcxproj')
PCBUILD_FILTERS = os.path.join(ROOT_DIR, 'PCbuild', '_freeze_module.vcxproj.filters')

# These are modules that get frozen.
FROZEN = [
    # See parse_frozen_spec() for the format.
    # In cases where the frozenid is duplicated, the first one is re-used.
    ('importlib', [
        'importlib._bootstrap : _frozen_importlib',
        'importlib._bootstrap_external : _frozen_importlib_external',
        'zipimport',
        ]),
    ('Test module', [
        'hello : __hello__ = ' + os.path.join(TOOLS_DIR, 'freeze', 'flag.py'),
        'hello : <__phello__>',
        'hello : __phello__.spam',
        ]),
]


#######################################
# specs

def parse_frozen_spec(rawspec, knownids=None, section=None):
    """Yield (frozenid, pyfile, modname, ispkg) for the corresponding modules.

    Supported formats:

      frozenid
      frozenid : modname
      frozenid : modname = pyfile

    "frozenid" and "modname" must be valid module names (dot-separated
    identifiers).  If "modname" is not provided then "frozenid" is used.
    If "pyfile" is not provided then the filename of the module
    corresponding to "frozenid" is used.

    Angle brackets around a frozenid (e.g. '<encodings>") indicate
    it is a package.  This also means it must be an actual module
    (i.e. "pyfile" cannot have been provided).  Such values can have
    patterns to expand submodules:

      <encodings.*>    - also freeze all direct submodules
      <encodings.**.*> - also freeze the full submodule tree

    As with "frozenid", angle brackets around "modname" indicate
    it is a package.  However, in this case "pyfile" should not
    have been provided and patterns in "modname" are not supported.
    Also, if "modname" has brackets then "frozenid" should not,
    and "pyfile" should have been provided..
    """
    frozenid, _, remainder = rawspec.partition(':')
    modname, _, pyfile = remainder.partition('=')
    frozenid = frozenid.strip()
    modname = modname.strip()
    pyfile = pyfile.strip()

    submodules = None
    if modname.startswith('<') and modname.endswith('>'):
        assert check_modname(frozenid), rawspec
        modname = modname[1:-1]
        assert check_modname(modname), rawspec
        if frozenid in knownids:
            pass
        elif pyfile:
            assert not os.path.isdir(pyfile), rawspec
        else:
            pyfile = _resolve_module(frozenid, ispkg=False)
        ispkg = True
    elif pyfile:
        assert check_modname(frozenid), rawspec
        assert not knownids or frozenid not in knownids, rawspec
        assert check_modname(modname), rawspec
        assert not os.path.isdir(pyfile), rawspec
        ispkg = False
    elif knownids and frozenid in knownids:
        assert check_modname(frozenid), rawspec
        assert check_modname(modname), rawspec
        ispkg = False
    else:
        assert not modname or check_modname(modname), rawspec
        resolved = iter(resolve_modules(frozenid))
        frozenid, pyfile, ispkg = next(resolved)
        if not modname:
            modname = frozenid
        if ispkg:
            pkgid = frozenid
            pkgname = modname
            def iter_subs():
                for frozenid, pyfile, ispkg in resolved:
                    assert not knownids or frozenid not in knownids, (frozenid, rawspec)
                    if pkgname:
                        modname = frozenid.replace(pkgid, pkgname, 1)
                    else:
                        modname = frozenid
                    yield frozenid, pyfile, modname, ispkg, section
            submodules = iter_subs()

    spec = (frozenid, pyfile or None, modname, ispkg, section)
    return spec, submodules


def parse_frozen_specs(rawspecs=FROZEN):
    seen = set()
    for section, _specs in rawspecs:
        for spec in _parse_frozen_specs(_specs, section, seen):
            frozenid = spec[0]
            yield spec
            seen.add(frozenid)


def _parse_frozen_specs(rawspecs, section, seen):
    for rawspec in rawspecs:
        spec, subs = parse_frozen_spec(rawspec, seen, section)
        yield spec
        for spec in subs or ():
            yield spec


def resolve_frozen_file(spec, destdir=MODULES_DIR):
    if isinstance(spec, str):
        modname = spec
    else:
        _, frozenid, _, _, _= spec
        modname = frozenid
    # We use a consistent naming convention for all frozen modules.
    return os.path.join(destdir, modname.replace('.', '_')) + '.h'


def resolve_frozen_files(specs, destdir=MODULES_DIR):
    frozen = {}
    frozenids = []
    lastsection = None
    for spec in specs:
        frozenid, pyfile, *_, section = spec
        if frozenid in frozen:
            if section is None:
                lastsection = None
            else:
                assert section == lastsection
            continue
        lastsection = section
        frozenfile = resolve_frozen_file(frozenid, destdir)
        frozen[frozenid] = (pyfile, frozenfile)
        frozenids.append(frozenid)
    return frozen, frozenids


#######################################
# generic helpers

def resolve_modules(modname, pyfile=None):
    if modname.startswith('<') and modname.endswith('>'):
        if pyfile:
            assert os.path.isdir(pyfile) or os.path.basename(pyfile) == '__init__.py', pyfile
        ispkg = True
        modname = modname[1:-1]
        rawname = modname
        # For now, we only expect match patterns at the end of the name.
        _modname, sep, match = modname.rpartition('.')
        if sep:
            if _modname.endswith('.**'):
                modname = _modname[:-3]
                match = f'**.{match}'
            elif match and not match.isidentifier():
                modname = _modname
            # Otherwise it's a plain name so we leave it alone.
        else:
            match = None
    else:
        ispkg = False
        rawname = modname
        match = None

    if not check_modname(modname):
        raise ValueError(f'not a valid module name ({rawname})')

    if not pyfile:
        pyfile = _resolve_module(modname, ispkg=ispkg)
    elif os.path.isdir(pyfile):
        pyfile = _resolve_module(modname, pyfile, ispkg)
    yield modname, pyfile, ispkg

    if match:
        pkgdir = os.path.dirname(pyfile)
        yield from iter_submodules(modname, pkgdir, match)


def check_modname(modname):
    return all(n.isidentifier() for n in modname.split('.'))


def iter_submodules(pkgname, pkgdir=None, match='*'):
    if not pkgdir:
        pkgdir = os.path.join(STDLIB_DIR, *pkgname.split('.'))
    if not match:
        match = '**.*'
    match_modname = _resolve_modname_matcher(match, pkgdir)

    def _iter_submodules(pkgname, pkgdir):
        for entry in sorted(os.scandir(pkgdir), key=lambda e: e.name):
            matched, recursive = match_modname(entry.name)
            if not matched:
                continue
            modname = f'{pkgname}.{entry.name}'
            if modname.endswith('.py'):
                yield modname[:-3], entry.path, False
            elif entry.is_dir():
                pyfile = os.path.join(entry.path, '__init__.py')
                # We ignore namespace packages.
                if os.path.exists(pyfile):
                    yield modname, pyfile, True
                    if recursive:
                        yield from _iter_submodules(modname, entry.path)

    return _iter_submodules(pkgname, pkgdir)


def _resolve_modname_matcher(match, rootdir=None):
    if isinstance(match, str):
        if match.startswith('**.'):
            recursive = True
            pat = match[3:]
            assert match
        else:
            recursive = False
            pat = match

        if pat == '*':
            def match_modname(modname):
                return True, recursive
        else:
            raise NotImplementedError(match)
    elif callable(match):
        match_modname = match(rootdir)
    else:
        raise ValueError(f'unsupported matcher {match!r}')
    return match_modname


def _resolve_module(modname, pathentry=STDLIB_DIR, ispkg=False):
    assert pathentry, pathentry
    pathentry = os.path.normpath(pathentry)
    assert os.path.isabs(pathentry)
    if ispkg:
        return os.path.join(pathentry, *modname.split('.'), '__init__.py')
    return os.path.join(pathentry, *modname.split('.')) + '.py'


#######################################
# regenerating dependent files

def find_marker(lines, marker, file):
    for pos, line in enumerate(lines):
        if marker in line:
            return pos
    raise Exception(f"Can't find {marker!r} in file {file}")


def replace_block(lines, start_marker, end_marker, replacements, file):
    start_pos = find_marker(lines, start_marker, file)
    end_pos = find_marker(lines, end_marker, file)
    if end_pos <= start_pos:
        raise Exception(f"End marker {end_marker!r} "
                        f"occurs before start marker {start_marker!r} "
                        f"in file {file}")
    replacements = [line.rstrip() + os.linesep for line in replacements]
    return lines[:start_pos + 1] + replacements + lines[end_pos:]


def regen_frozen(specs, dest=MODULES_DIR):
    if isinstance(dest, str):
        frozen, frozenids = resolve_frozen_files(specs, destdir)
    else:
        frozenids, frozen = dest

    headerlines = []
    parentdir = os.path.dirname(FROZEN_FILE)
    for frozenid in frozenids:
        # Adding a comment to separate sections here doesn't add much,
        # so we don't.
        _, frozenfile = frozen[frozenid]
        header = os.path.relpath(frozenfile, parentdir)
        headerlines.append(f'#include "{header}"')

    deflines = []
    indent = '    '
    lastsection = None
    for spec in specs:
        frozenid, _, modname, ispkg, section = spec
        if section != lastsection:
            if lastsection is not None:
                deflines.append('')
            deflines.append(f'/* {section} */')
        lastsection = section

        # This matches what we do in Programs/_freeze_module.c:
        name = frozenid.replace('.', '_')
        symbol = '_Py_M__' + name
        pkg = '-' if ispkg else ''
        line = ('{"%s", %s, %s(int)sizeof(%s)},'
                % (modname, symbol, pkg, symbol))
        # TODO: Consider not folding lines
        if len(line) < 80:
            deflines.append(line)
        else:
            line1, _, line2 = line.rpartition(' ')
            deflines.append(line1)
            deflines.append(indent + line2)

    if not deflines[0]:
        del deflines[0]
    for i, line in enumerate(deflines):
        if line:
            deflines[i] = indent + line

    print(f'# Updating {os.path.relpath(FROZEN_FILE)}')
    with updating_file_with_tmpfile(FROZEN_FILE) as (infile, outfile):
        lines = infile.readlines()
        # TODO: Use more obvious markers, e.g.
        # $START GENERATED FOOBAR$ / $END GENERATED FOOBAR$
        lines = replace_block(
            lines,
            "/* Includes for frozen modules: */",
            "/* End includes */",
            headerlines,
            FROZEN_FILE,
        )
        lines = replace_block(
            lines,
            "static const struct _frozen _PyImport_FrozenModules[] =",
            "/* sentinel */",
            deflines,
            FROZEN_FILE,
        )
        outfile.writelines(lines)


def regen_makefile(frozenids, frozen):
    frozenfiles = []
    rules = ['']
    for frozenid in frozenids:
        pyfile, frozenfile = frozen[frozenid]
        header = os.path.relpath(frozenfile, ROOT_DIR)
        relfile = header.replace('\\', '/')
        frozenfiles.append(f'\t\t$(srcdir)/{relfile} \\')

        _pyfile = os.path.relpath(pyfile, ROOT_DIR)
        tmpfile = f'{header}.new'
        # Note that we freeze the module to the target .h file
        # instead of going through an intermediate file like we used to.
        rules.append(f'{header}: $(srcdir)/Programs/_freeze_module $(srcdir)/{_pyfile}')
        rules.append(f'\t$(srcdir)/Programs/_freeze_module {frozenid} \\')
        rules.append(f'\t\t$(srcdir)/{_pyfile} \\')
        rules.append(f'\t\t$(srcdir)/{header}')
        rules.append('')

    frozenfiles[-1] = frozenfiles[-1].rstrip(" \\")

    print(f'# Updating {os.path.relpath(MAKEFILE)}')
    with updating_file_with_tmpfile(MAKEFILE) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            "FROZEN_FILES =",
            "# End FROZEN_FILES",
            frozenfiles,
            MAKEFILE,
        )
        lines = replace_block(
            lines,
            "# BEGIN: freezing modules",
            "# END: freezing modules",
            rules,
            MAKEFILE,
        )
        outfile.writelines(lines)


def regen_pcbuild(frozenids, frozen):
    projlines = []
    filterlines = []
    for frozenid in frozenids:
        pyfile, frozenfile = frozen[frozenid]

        _pyfile = os.path.relpath(pyfile, ROOT_DIR).replace('/', '\\')
        header = os.path.relpath(frozenfile, ROOT_DIR).replace('/', '\\')
        intfile = header.split('\\')[-1].strip('.h') + '.g.h'
        projlines.append(f'    <None Include="..\\{_pyfile}">')
        projlines.append(f'      <ModName>{frozenid}</ModName>')
        projlines.append(f'      <IntFile>$(IntDir){intfile}</IntFile>')
        projlines.append(f'      <OutFile>$(PySourcePath){header}</OutFile>')
        projlines.append(f'    </None>')

        filterlines.append(f'    <None Include="..\\{_pyfile}">')
        filterlines.append('      <Filter>Python Files</Filter>')
        filterlines.append('    </None>')

    print(f'# Updating {os.path.relpath(PCBUILD_PROJECT)}')
    with updating_file_with_tmpfile(PCBUILD_PROJECT) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN frozen modules -->',
            '<!-- END frozen modules -->',
            projlines,
            PCBUILD_PROJECT,
        )
        outfile.writelines(lines)
    print(f'# Updating {os.path.relpath(PCBUILD_FILTERS)}')
    with updating_file_with_tmpfile(PCBUILD_FILTERS) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN frozen modules -->',
            '<!-- END frozen modules -->',
            filterlines,
            PCBUILD_FILTERS,
        )
        outfile.writelines(lines)


#######################################
# freezing modules

def freeze_module(modname, pyfile=None, destdir=MODULES_DIR):
    """Generate the frozen module .h file for the given module."""
    for modname, pyfile, ispkg in resolve_modules(modname, pyfile):
        frozenfile = _resolve_frozen(modname, destdir)
        _freeze_module(modname, pyfile, frozenfile)


def _freeze_module(frozenid, pyfile, frozenfile):
    tmpfile = frozenfile + '.new'

    argv = [TOOL, frozenid, pyfile, tmpfile]
    print('#', '  '.join(os.path.relpath(a) for a in argv))
    try:
        subprocess.run(argv, check=True)
    except subprocess.CalledProcessError:
        if not os.path.exists(TOOL):
            sys.exit(f'ERROR: missing {TOOL}; you need to run "make regen-frozen"')
        raise  # re-raise

    os.replace(tmpfile, frozenfile)


#######################################
# the script

def main():
    # Expand the raw specs, preserving order.
    specs = list(parse_frozen_specs())
    frozen, frozenids = resolve_frozen_files(specs, MODULES_DIR)

    # Regen build-related files.
    regen_frozen(specs, (frozenids, frozen))
    regen_makefile(frozenids, frozen)
    regen_pcbuild(frozenids, frozen)

    # Freeze the target modules.
    for frozenid in frozenids:
        pyfile, frozenfile = frozen[frozenid]
        _freeze_module(frozenid, pyfile, frozenfile)


if __name__ == '__main__':
    argv = sys.argv[1:]
    if argv:
        sys.exit('ERROR: got unexpected args {argv}')
    main()
