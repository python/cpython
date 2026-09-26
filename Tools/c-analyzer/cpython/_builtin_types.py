from collections import namedtuple
import os.path
import re
import textwrap

from c_common import tables
from . import REPO_ROOT
from ._files import iter_header_files, iter_filenames


CAPI_PREFIX = os.path.join('Include', '')
INTERNAL_PREFIX = os.path.join('Include', 'internal', '')

REGEX = re.compile(textwrap.dedent(rf'''
    (?:
        ^
        (?:
            (?:
                (?:
                    (?:
                        (?:
                            ( static )  # <static>
                            \s+
                            |
                            ( extern )  # <extern>
                            \s+
                         )?
                        PyTypeObject \s+
                     )
                    |
                    (?:
                        ( PyAPI_DATA )  # <capi>
                        \s* [(] \s* PyTypeObject \s* [)] \s*
                     )
                 )
                (\w+)  # <name>
                \s*
                (?:
                    (?:
                        ( = \s* {{ )  # <def>
                        $
                     )
                    |
                    ( ; )  # <decl>
                 )
             )
            |
            (?:
                # These are specific to Objects/exceptions.c:
                (?:
                    SimpleExtendsException
                    |
                    MiddlingExtendsException
                    |
                    ComplexExtendsException
                 )
                \( \w+ \s* , \s*
                ( \w+ )  # <excname>
                \s* ,
             )
         )
    )
'''), re.VERBOSE)


def _parse_line(line):
    m = re.match(REGEX, line)
    if not m:
        return None
    (static, extern, capi,
     name,
     def_, decl,
     excname,
     ) = m.groups()
    if def_:
        isdecl = False
        if extern or capi:
            raise NotImplementedError(line)
        kind = 'static' if static else None
    elif excname:
        name = f'_PyExc_{excname}'
        isdecl = False
        kind = 'static'
    else:
        isdecl = True
        if static:
            kind = 'static'
        elif extern:
            kind = 'extern'
        elif capi:
            kind = 'capi'
        else:
            kind = None
    return name, isdecl, kind


class BuiltinTypeDecl(namedtuple('BuiltinTypeDecl', 'file lno name kind')):

    KINDS = {
        'static',
        'extern',
        'capi',
        'forward',
    }

    @classmethod
    def from_line(cls, line, filename, lno):
        # This is similar to ._capi.CAPIItem.from_line().
        parsed = _parse_line(line)
        if not parsed:
            return None
        name, isdecl, kind = parsed
        if not isdecl:
            return None
        return cls.from_parsed(name, kind, filename, lno)

    @classmethod
    def from_parsed(cls, name, kind, filename, lno):
        if not kind:
            kind = 'forward'
        return cls.from_values(filename, lno, name, kind)

    @classmethod
    def from_values(cls, filename, lno, name, kind):
        if kind not in cls.KINDS:
            raise ValueError(f'unsupported kind {kind!r}')
        self = cls(filename, lno, name, kind)
        if self.kind not in ('extern', 'capi') and self.api:
            raise NotImplementedError(self)
        elif self.kind == 'capi' and not self.api:
            raise NotImplementedError(self)
        return self

    @property
    def relfile(self):
        return self.file[len(REPO_ROOT) + 1:]

    @property
    def api(self):
        return self.relfile.startswith(CAPI_PREFIX)

    @property
    def internal(self):
        return self.relfile.startswith(INTERNAL_PREFIX)

    @property
    def private(self):
        if not self.name.startswith('_'):
            return False
        return self.api and not self.internal

    @property
    def public(self):
        if self.kind != 'capi':
            return False
        return not self.internal and not self.private


class BuiltinTypeInfo(namedtuple('BuiltinTypeInfo', 'file lno name static decl')):

    @classmethod
    def from_line(cls, line, filename, lno, *, decls=None):
        parsed = _parse_line(line)
        if not parsed:
            return None
        name, isdecl, kind = parsed
        if isdecl:
            return None
        return cls.from_parsed(name, kind, filename, lno, decls=decls)

    @classmethod
    def from_parsed(cls, name, kind, filename, lno, *, decls=None):
        if not kind:
            static = False
        elif kind == 'static':
            static = True
        else:
            raise NotImplementedError((filename, line, kind))
        decl = decls.get(name) if decls else None
        return cls(filename, lno, name, static, decl)

    @property
    def relfile(self):
        return self.file[len(REPO_ROOT) + 1:]

    @property
    def exported(self):
        return not self.static

    @property
    def api(self):
        if not self.decl:
            return False
        return self.decl.api

    @property
    def internal(self):
        if not self.decl:
            return False
        return self.decl.internal

    @property
    def private(self):
        if not self.decl:
            return False
        return self.decl.private

    @property
    def public(self):
        if not self.decl:
            return False
        return self.decl.public

    @property
    def inmodule(self):
        return self.relfile.startswith('Modules' + os.path.sep)

    def render_rowvalues(self, kinds):
        row = {
            'name': self.name,
            **{k: '' for k in kinds},
            'filename': f'{self.relfile}:{self.lno}',
        }
        if self.static:
            kind = 'static'
        else:
            if self.internal:
                kind = 'internal'
            elif self.private:
                kind = 'private'
            elif self.public:
                kind = 'public'
            else:
                kind = 'global'
        row['kind'] = kind
        row[kind] = kind
        return row


def _ensure_decl(decl, decls):
    prev = decls.get(decl.name)
    if prev:
        if decl.kind == 'forward':
            return None
        if prev.kind != 'forward':
            if decl.kind == prev.kind and decl.file == prev.file:
                assert decl.lno != prev.lno, (decl, prev)
                return None
            raise NotImplementedError(f'duplicate {decl} (was {prev}')
    decls[decl.name] = decl


def iter_builtin_types(filenames=None):
    decls = {}
    seen = set()
    for filename in iter_header_files():
        seen.add(filename)
        with open(filename) as infile:
            for lno, line in enumerate(infile, 1):
                decl = BuiltinTypeDecl.from_line(line, filename, lno)
                if not decl:
                    continue
                _ensure_decl(decl, decls)
    srcfiles = []
    for filename in iter_filenames():
        if filename.endswith('.c'):
            srcfiles.append(filename)
            continue
        if filename in seen:
            continue
        with open(filename) as infile:
            for lno, line in enumerate(infile, 1):
                decl = BuiltinTypeDecl.from_line(line, filename, lno)
                if not decl:
                    continue
                _ensure_decl(decl, decls)

    for filename in srcfiles:
        with open(filename) as infile:
            localdecls = {}
            for lno, line in enumerate(infile, 1):
                parsed = _parse_line(line)
                if not parsed:
                    continue
                name, isdecl, kind = parsed
                if isdecl:
                    decl = BuiltinTypeDecl.from_parsed(name, kind, filename, lno)
                    if not decl:
                        raise NotImplementedError((filename, line))
                    _ensure_decl(decl, localdecls)
                else:
                    builtin = BuiltinTypeInfo.from_parsed(
                            name, kind, filename, lno,
                            decls=decls if name in decls else localdecls)
                    if not builtin:
                        raise NotImplementedError((filename, line))
                    yield builtin


def resolve_matcher(showmodules=False):
    def match(info, *, log=None):
        if not info.inmodule:
            return True
        if log is not None:
            log(f'ignored {info.name!r}')
        return False
    return match


##################################
# CLI rendering

def resolve_format(fmt):
    if not fmt:
        return 'table'
    elif isinstance(fmt, str) and fmt in _FORMATS:
        return fmt
    else:
        raise NotImplementedError(fmt)


def get_renderer(fmt):
    fmt = resolve_format(fmt)
    if isinstance(fmt, str):
        try:
            return _FORMATS[fmt]
        except KeyError:
            raise ValueError(f'unsupported format {fmt!r}')
    else:
        raise NotImplementedError(fmt)


def render_table(types):
    types = sorted(types, key=(lambda t: t.name))
    colspecs = tables.resolve_columns(
            'name:<33 static:^ global:^ internal:^ private:^ public:^ filename:<30')
    header, div, rowfmt = tables.build_table(colspecs)
    leader = ' ' * sum(c.width+2 for c in colspecs[:3]) + '   '
    yield leader + f'{"API":^29}'
    yield leader + '-' * 29
    yield header
    yield div
    kinds = [c[0] for c in colspecs[1:-1]]
    counts = {k: 0 for k in kinds}
    base = {k: '' for k in kinds}
    for t in types:
        row = t.render_rowvalues(kinds)
        kind = row['kind']
        yield rowfmt.format(**row)
        counts[kind] += 1
    yield ''
    yield f'total: {sum(counts.values()):>3}'
    for kind in kinds:
        yield f'  {kind:>10}: {counts[kind]:>3}'


def render_repr(types):
    for t in types:
        yield repr(t)


_FORMATS = {
    'table': render_table,
    'repr': render_repr,
}
