"""Freeze modules and regen related files (e.g. Python/frozen.c).

See the notes at the top of Python/frozen.c for more info.
"""

from collections import namedtuple
import hashlib
import os
import ntpath
import posixpath
import argparse
from update_file import updating_file_with_tmpfile


ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
ROOT_DIR = os.path.abspath(ROOT_DIR)
FROZEN_ONLY = os.path.join(ROOT_DIR, 'Tools', 'freeze', 'flag.py')

STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
# If FROZEN_MODULES_DIR or DEEPFROZEN_MODULES_DIR is changed then the
# .gitattributes and .gitignore files needs to be updated.
FROZEN_MODULES_DIR = os.path.join(ROOT_DIR, 'Python', 'frozen_modules')
DEEPFROZEN_MODULES_DIR = os.path.join(ROOT_DIR, 'Python', 'deepfreeze')
DEEPFREEZE_MAPPING_FNAME = 'deepfreeze_mappings.txt'

FROZEN_FILE = os.path.join(ROOT_DIR, 'Python', 'frozen.c')
MAKEFILE = os.path.join(ROOT_DIR, 'Makefile.pre.in')
PCBUILD_PROJECT = os.path.join(ROOT_DIR, 'PCbuild', '_freeze_module.vcxproj')
PCBUILD_FILTERS = os.path.join(ROOT_DIR, 'PCbuild', '_freeze_module.vcxproj.filters')
PCBUILD_PYTHONCORE = os.path.join(ROOT_DIR, 'PCbuild', 'pythoncore.vcxproj')


OS_PATH = 'ntpath' if os.name == 'nt' else 'posixpath'

# These are modules that get frozen.
# If you're debugging new bytecode instructions,
# you can delete all sections except 'import system'.
# This also speeds up building somewhat.
TESTS_SECTION = 'Test module'
FROZEN = [
    # See parse_frozen_spec() for the format.
    # In cases where the frozenid is duplicated, the first one is re-used.
    ('import system', [
        # These frozen modules are necessary for bootstrapping
        # the import system.
        'importlib._bootstrap : _frozen_importlib',
        'importlib._bootstrap_external : _frozen_importlib_external',
        # This module is important because some Python builds rely
        # on a builtin zip file instead of a filesystem.
        'zipimport',
        ]),
    # (You can delete entries from here down to the end of the list.)
    ('stdlib - startup, without site (python -S)', [
        'abc',
        'codecs',
        # For now we do not freeze the encodings, due # to the noise all
        # those extra modules add to the text printed during the build.
        # (See https://github.com/python/cpython/pull/28398#pullrequestreview-756856469.)
        #'<encodings.*>',
        'io',
        ]),
    ('stdlib - startup, with site', [
        '_collections_abc',
        '_sitebuiltins',
        'genericpath',
        'ntpath',
        'posixpath',
        # We must explicitly mark os.path as a frozen module
        # even though it will never be imported.
        f'{OS_PATH} : os.path',
        'os',
        'site',
        'stat',
        ]),
    ('runpy - run module with -m', [
        "importlib.util",
        "importlib.machinery",
        "runpy",
    ]),
    (TESTS_SECTION, [
        '__hello__',
        '__hello__ : __hello_alias__',
        '__hello__ : <__phello_alias__>',
        '__hello__ : __phello_alias__.spam',
        '<__phello__.**.*>',
        f'frozen_only : __hello_only__ = {FROZEN_ONLY}',
        ]),
    # (End of stuff you could delete.)
]
BOOTSTRAP = {
    'importlib._bootstrap',
    'importlib._bootstrap_external',
    'zipimport',
}


#######################################
# platform-specific helpers

if os.path is posixpath:
    relpath_for_posix_display = os.path.relpath

    def relpath_for_windows_display(path, base):
        return ntpath.relpath(
            ntpath.join(*path.split(os.path.sep)),
            ntpath.join(*base.split(os.path.sep)),
        )

else:
    relpath_for_windows_display = ntpath.relpath

    def relpath_for_posix_display(path, base):
        return posixpath.relpath(
            posixpath.join(*path.split(os.path.sep)),
            posixpath.join(*base.split(os.path.sep)),
        )


#######################################
# specs

def parse_frozen_specs():
    seen = {}
    for section, specs in FROZEN:
        parsed = _parse_specs(specs, section, seen)
        for item in parsed:
            frozenid, pyfile, modname, ispkg, section = item
            try:
                source = seen[frozenid]
            except KeyError:
                source = FrozenSource.from_id(frozenid, pyfile)
                seen[frozenid] = source
            else:
                assert not pyfile or pyfile == source.pyfile, item
            yield FrozenModule(modname, ispkg, section, source)


def _parse_specs(specs, section, seen):
    for spec in specs:
        info, subs = _parse_spec(spec, seen, section)
        yield info
        for info in subs or ():
            yield info


def _parse_spec(spec, knownids=None, section=None):
    """Yield an info tuple for each module corresponding to the given spec.

    The info consists of: (frozenid, pyfile, modname, ispkg, section).

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
    frozenid, _, remainder = spec.partition(':')
    modname, _, pyfile = remainder.partition('=')
    frozenid = frozenid.strip()
    modname = modname.strip()
    pyfile = pyfile.strip()

    submodules = None
    if modname.startswith('<') and modname.endswith('>'):
        assert check_modname(frozenid), spec
        modname = modname[1:-1]
        assert check_modname(modname), spec
        if frozenid in knownids:
            pass
        elif pyfile:
            assert not os.path.isdir(pyfile), spec
        else:
            pyfile = _resolve_module(frozenid, ispkg=False)
        ispkg = True
    elif pyfile:
        assert check_modname(frozenid), spec
        assert not knownids or frozenid not in knownids, spec
        assert check_modname(modname), spec
        assert not os.path.isdir(pyfile), spec
        ispkg = False
    elif knownids and frozenid in knownids:
        assert check_modname(frozenid), spec
        assert check_modname(modname), spec
        ispkg = False
    else:
        assert not modname or check_modname(modname), spec
        resolved = iter(resolve_modules(frozenid))
        frozenid, pyfile, ispkg = next(resolved)
        if not modname:
            modname = frozenid
        if ispkg:
            pkgid = frozenid
            pkgname = modname
            pkgfiles = {pyfile: pkgid}
            def iter_subs():
                for frozenid, pyfile, ispkg in resolved:
                    if pkgname:
                        modname = frozenid.replace(pkgid, pkgname, 1)
                    else:
                        modname = frozenid
                    if pyfile:
                        if pyfile in pkgfiles:
                            frozenid = pkgfiles[pyfile]
                            pyfile = None
                        elif ispkg:
                            pkgfiles[pyfile] = frozenid
                    yield frozenid, pyfile, modname, ispkg, section
            submodules = iter_subs()

    info = (frozenid, pyfile or None, modname, ispkg, section)
    return info, submodules


#######################################
# frozen source files

class FrozenSource(namedtuple('FrozenSource', 'id pyfile frozenfile deepfreezefile')):

    @classmethod
    def from_id(cls, frozenid, pyfile=None):
        if not pyfile:
            pyfile = os.path.join(STDLIB_DIR, *frozenid.split('.')) + '.py'
            #assert os.path.exists(pyfile), (frozenid, pyfile)
        frozenfile = resolve_frozen_file(frozenid, FROZEN_MODULES_DIR)
        deepfreezefile = resolve_frozen_file(frozenid, DEEPFROZEN_MODULES_DIR)
        return cls(frozenid, pyfile, frozenfile, deepfreezefile)

    @property
    def frozenid(self):
        return self.id

    @property
    def modname(self):
        if self.pyfile.startswith(STDLIB_DIR):
            return self.id
        return None

    @property
    def symbol(self):
        # This matches what we do in Programs/_freeze_module.c:
        name = self.frozenid.replace('.', '_')
        return '_Py_M__' + name

    @property
    def ispkg(self):
        if not self.pyfile:
            return False
        elif self.frozenid.endswith('.__init__'):
            return False
        else:
            return os.path.basename(self.pyfile) == '__init__.py'

    @property
    def isbootstrap(self):
        return self.id in BOOTSTRAP


def resolve_frozen_file(frozenid, destdir):
    """Return the filename corresponding to the given frozen ID.

    For stdlib modules the ID will always be the full name
    of the source module.
    """
    if not isinstance(frozenid, str):
        try:
            frozenid = frozenid.frozenid
        except AttributeError:
            raise ValueError(f'unsupported frozenid {frozenid!r}')
    # We use a consistent naming convention for all frozen modules.
    frozenfile = f'{frozenid}.h'
    if not destdir:
        return frozenfile
    return os.path.join(destdir, frozenfile)


#######################################
# frozen modules

class FrozenModule(namedtuple('FrozenModule', 'name ispkg section source')):

    def __getattr__(self, name):
        return getattr(self.source, name)

    @property
    def modname(self):
        return self.name

    @property
    def orig(self):
        return self.source.modname

    @property
    def isalias(self):
        orig = self.source.modname
        if not orig:
            return True
        return self.name != orig

    def summarize(self):
        source = self.source.modname
        if source:
            source = f'<{source}>'
        else:
            source = relpath_for_posix_display(self.pyfile, ROOT_DIR)
        return {
            'module': self.name,
            'ispkg': self.ispkg,
            'source': source,
            'frozen': os.path.basename(self.frozenfile),
            'checksum': _get_checksum(self.frozenfile),
        }


def _iter_sources(modules):
    seen = set()
    for mod in modules:
        if mod.source not in seen:
            yield mod.source
            seen.add(mod.source)


#######################################
# generic helpers

def _get_checksum(filename):
    with open(filename, "rb") as infile:
        contents = infile.read()
    m = hashlib.sha256()
    m.update(contents)
    return m.hexdigest()


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
    replacements = [line.rstrip() + '\n' for line in replacements]
    return lines[:start_pos + 1] + replacements + lines[end_pos:]


class UniqueList(list):
    def __init__(self):
        self._seen = set()

    def append(self, item):
        if item in self._seen:
            return
        super().append(item)
        self._seen.add(item)


def regen_frozen(modules):
    headerlines = []
    parentdir = os.path.dirname(FROZEN_FILE)
    for src in _iter_sources(modules):
        # Adding a comment to separate sections here doesn't add much,
        # so we don't.
        header = relpath_for_posix_display(src.frozenfile, parentdir)
        headerlines.append(f'#include "{header}"')

    externlines = UniqueList()
    bootstraplines = []
    stdliblines = []
    testlines = []
    aliaslines = []
    indent = '    '
    lastsection = None
    for mod in modules:
        if mod.isbootstrap:
            lines = bootstraplines
        elif mod.section == TESTS_SECTION:
            lines = testlines
        else:
            lines = stdliblines
            if mod.section != lastsection:
                if lastsection is not None:
                    lines.append('')
                lines.append(f'/* {mod.section} */')
            lastsection = mod.section

        # Also add a extern declaration for the corresponding
        # deepfreeze-generated function.
        orig_name = mod.source.id
        code_name = orig_name.replace(".", "_")
        get_code_name = "_Py_get_%s_toplevel" % code_name
        externlines.append("extern PyObject *%s(void);" % get_code_name)

        pkg = 'true' if mod.ispkg else 'false'
        size = f"(int)sizeof({mod.symbol})"
        line = f'{{"{mod.name}", {mod.symbol}, {size}, {pkg}}},'
        lines.append(line)

        if mod.isalias:
            if not mod.orig:
                entry = '{"%s", NULL},' % (mod.name,)
            elif mod.source.ispkg:
                entry = '{"%s", "<%s"},' % (mod.name, mod.orig)
            else:
                entry = '{"%s", "%s"},' % (mod.name, mod.orig)
            aliaslines.append(indent + entry)

    for lines in (bootstraplines, stdliblines, testlines):
        # TODO: Is this necessary any more?
        if lines and not lines[0]:
            del lines[0]
        for i, line in enumerate(lines):
            if line:
                lines[i] = indent + line

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
            "/* Start extern declarations */",
            "/* End extern declarations */",
            externlines,
            FROZEN_FILE,
        )
        lines = replace_block(
            lines,
            "static const struct _frozen bootstrap_modules[] =",
            "/* bootstrap sentinel */",
            bootstraplines,
            FROZEN_FILE,
        )
        lines = replace_block(
            lines,
            "static const struct _frozen stdlib_modules[] =",
            "/* stdlib sentinel */",
            stdliblines,
            FROZEN_FILE,
        )
        lines = replace_block(
            lines,
            "static const struct _frozen test_modules[] =",
            "/* test sentinel */",
            testlines,
            FROZEN_FILE,
        )
        lines = replace_block(
            lines,
            "const struct _module_alias aliases[] =",
            "/* aliases sentinel */",
            aliaslines,
            FROZEN_FILE,
        )
        outfile.writelines(lines)


def regen_makefile(modules):
    pyfiles = []
    frozenfiles = []
    rules = ['']
    deepfreezerules = ["$(DEEPFREEZE_C): $(DEEPFREEZE_DEPS)",
                       "\t$(PYTHON_FOR_FREEZE) $(srcdir)/Tools/build/deepfreeze.py \\"]
    for src in _iter_sources(modules):
        frozen_header = relpath_for_posix_display(src.frozenfile, ROOT_DIR)
        frozenfiles.append(f'\t\t{frozen_header} \\')

        pyfile = relpath_for_posix_display(src.pyfile, ROOT_DIR)
        pyfiles.append(f'\t\t{pyfile} \\')

        if src.isbootstrap:
            freezecmd = '$(FREEZE_MODULE_BOOTSTRAP)'
            freezedep = '$(FREEZE_MODULE_BOOTSTRAP_DEPS)'
        else:
            freezecmd = '$(FREEZE_MODULE)'
            freezedep = '$(FREEZE_MODULE_DEPS)'

        freeze = (f'{freezecmd} {src.frozenid} '
                    f'$(srcdir)/{pyfile} {frozen_header}')
        rules.extend([
            f'{frozen_header}: {pyfile} {freezedep}',
            f'\t{freeze}',
            '',
        ])
        deepfreezerules.append(f"\t{frozen_header}:{src.frozenid} \\")
    deepfreezerules.append('\t-o Python/deepfreeze/deepfreeze.c')
    pyfiles[-1] = pyfiles[-1].rstrip(" \\")
    frozenfiles[-1] = frozenfiles[-1].rstrip(" \\")

    print(f'# Updating {os.path.relpath(MAKEFILE)}')
    with updating_file_with_tmpfile(MAKEFILE) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            "FROZEN_FILES_IN =",
            "# End FROZEN_FILES_IN",
            pyfiles,
            MAKEFILE,
        )
        lines = replace_block(
            lines,
            "FROZEN_FILES_OUT =",
            "# End FROZEN_FILES_OUT",
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
        lines = replace_block(
            lines,
            "# BEGIN: deepfreeze modules",
            "# END: deepfreeze modules",
            deepfreezerules,
            MAKEFILE,
        )
        outfile.writelines(lines)


def regen_pcbuild(modules):
    projlines = []
    filterlines = []
    corelines = []
    deepfreezemappingsfile = f'$(IntDir)\\{DEEPFREEZE_MAPPING_FNAME}'
    deepfreezerules = [f'    <Exec Command=\'$(PythonForBuild) "$(PySourcePath)Tools\\build\\deepfreeze.py" -f "{deepfreezemappingsfile}" -o "$(GeneratedFrozenModulesDir)deepfreeze.c"\' />']
    deepfreezemappings = []
    for src in _iter_sources(modules):
        pyfile = relpath_for_windows_display(src.pyfile, ROOT_DIR)
        header = relpath_for_windows_display(src.frozenfile, ROOT_DIR)
        intfile = ntpath.splitext(ntpath.basename(header))[0] + '.g.h'
        projlines.append(f'    <None Include="..\\{pyfile}">')
        projlines.append(f'      <ModName>{src.frozenid}</ModName>')
        projlines.append(f'      <IntFile>$(IntDir){intfile}</IntFile>')
        projlines.append(f'      <OutFile>$(GeneratedFrozenModulesDir){header}</OutFile>')
        projlines.append(f'    </None>')

        filterlines.append(f'    <None Include="..\\{pyfile}">')
        filterlines.append('      <Filter>Python Files</Filter>')
        filterlines.append('    </None>')
        deepfreezemappings.append(f'    <FrozenModule Include="$(GeneratedFrozenModulesDir){header}" FrozenId="{src.frozenid}" />\n')

    corelines.append(f'    <ClCompile Include="$(GeneratedFrozenModulesDir)deepfreeze.c" />')

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
    with updating_file_with_tmpfile(PCBUILD_PROJECT) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN freeze mappings -->',
            '<!-- END freeze mappings -->',
            deepfreezemappings,
            PCBUILD_PROJECT,
        )
        outfile.writelines(lines)
    with updating_file_with_tmpfile(PCBUILD_PROJECT) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN freeze mapping file -->',
            '<!-- END freeze mapping file -->',
            [deepfreezemappingsfile, ],
            PCBUILD_PROJECT,
        )
        outfile.writelines(lines)
    with updating_file_with_tmpfile(PCBUILD_PROJECT) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN deepfreeze rule -->',
            '<!-- END deepfreeze rule -->',
            deepfreezerules,
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
    print(f'# Updating {os.path.relpath(PCBUILD_PYTHONCORE)}')
    with updating_file_with_tmpfile(PCBUILD_PYTHONCORE) as (infile, outfile):
        lines = infile.readlines()
        lines = replace_block(
            lines,
            '<!-- BEGIN deepfreeze -->',
            '<!-- END deepfreeze -->',
            corelines,
            PCBUILD_FILTERS,
        )
        outfile.writelines(lines)


#######################################
# the script

def main():
    # Expand the raw specs, preserving order.
    modules = list(parse_frozen_specs())

    # Regen build-related files.
    regen_makefile(modules)
    regen_pcbuild(modules)
    regen_frozen(modules)


if __name__ == '__main__':
    main()
