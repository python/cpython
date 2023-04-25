from collections import namedtuple
import enum
import re

from c_common import fsutil
from c_common.clsutil import classonly
import c_common.misc as _misc
import c_common.strutil as _strutil
import c_common.tables as _tables
from .parser._regexes import _STORAGE


FIXED_TYPE = _misc.Labeled('FIXED_TYPE')

STORAGE = frozenset(_STORAGE)


#############################
# kinds

@enum.unique
class KIND(enum.Enum):

    # XXX Use these in the raw parser code.
    TYPEDEF = 'typedef'
    STRUCT = 'struct'
    UNION = 'union'
    ENUM = 'enum'
    FUNCTION = 'function'
    VARIABLE = 'variable'
    STATEMENT = 'statement'

    @classonly
    def _from_raw(cls, raw):
        if raw is None:
            return None
        elif isinstance(raw, cls):
            return raw
        elif type(raw) is str:
            # We could use cls[raw] for the upper-case form,
            # but there's no need to go to the trouble.
            return cls(raw.lower())
        else:
            raise NotImplementedError(raw)

    @classonly
    def by_priority(cls, group=None):
        if group is None:
            return cls._ALL_BY_PRIORITY.copy()
        elif group == 'type':
            return cls._TYPE_DECLS_BY_PRIORITY.copy()
        elif group == 'decl':
            return cls._ALL_DECLS_BY_PRIORITY.copy()
        elif isinstance(group, str):
            raise NotImplementedError(group)
        else:
            # XXX Treat group as a set of kinds & return in priority order?
            raise NotImplementedError(group)

    @classonly
    def is_type_decl(cls, kind):
        if kind in cls.TYPES:
            return True
        if not isinstance(kind, cls):
            raise TypeError(f'expected KIND, got {kind!r}')
        return False

    @classonly
    def is_decl(cls, kind):
        if kind in cls.DECLS:
            return True
        if not isinstance(kind, cls):
            raise TypeError(f'expected KIND, got {kind!r}')
        return False

    @classonly
    def get_group(cls, kind, *, groups=None):
        if not isinstance(kind, cls):
            raise TypeError(f'expected KIND, got {kind!r}')
        if groups is None:
            groups = ['type']
        elif not groups:
            groups = ()
        elif isinstance(groups, str):
            group = groups
            if group not in cls._GROUPS:
                raise ValueError(f'unsupported group {group!r}')
            groups = [group]
        else:
            unsupported = [g for g in groups if g not in cls._GROUPS]
            if unsupported:
                raise ValueError(f'unsupported groups {", ".join(repr(unsupported))}')
        for group in groups:
            if kind in cls._GROUPS[group]:
                return group
        else:
            return kind.value

    @classonly
    def resolve_group(cls, group):
        if isinstance(group, cls):
            return {group}
        elif isinstance(group, str):
            try:
                return cls._GROUPS[group].copy()
            except KeyError:
                raise ValueError(f'unsupported group {group!r}')
        else:
            resolved = set()
            for gr in group:
                resolve.update(cls.resolve_group(gr))
            return resolved
            #return {*cls.resolve_group(g) for g in group}


KIND._TYPE_DECLS_BY_PRIORITY = [
    # These are in preferred order.
    KIND.TYPEDEF,
    KIND.STRUCT,
    KIND.UNION,
    KIND.ENUM,
]
KIND._ALL_DECLS_BY_PRIORITY = [
    # These are in preferred order.
    *KIND._TYPE_DECLS_BY_PRIORITY,
    KIND.FUNCTION,
    KIND.VARIABLE,
]
KIND._ALL_BY_PRIORITY = [
    # These are in preferred order.
    *KIND._ALL_DECLS_BY_PRIORITY,
    KIND.STATEMENT,
]

KIND.TYPES = frozenset(KIND._TYPE_DECLS_BY_PRIORITY)
KIND.DECLS = frozenset(KIND._ALL_DECLS_BY_PRIORITY)
KIND._GROUPS = {
    'type': KIND.TYPES,
    'decl': KIND.DECLS,
}
KIND._GROUPS.update((k.value, {k}) for k in KIND)


def get_kind_group(item):
    return KIND.get_group(item.kind)


#############################
# low-level

def _fix_filename(filename, relroot, *,
                  formatted=True,
                  **kwargs):
    if formatted:
        fix = fsutil.format_filename
    else:
        fix = fsutil.fix_filename
    return fix(filename, relroot=relroot, **kwargs)


class FileInfo(namedtuple('FileInfo', 'filename lno')):
    @classmethod
    def from_raw(cls, raw):
        if isinstance(raw, cls):
            return raw
        elif isinstance(raw, tuple):
            return cls(*raw)
        elif not raw:
            return None
        elif isinstance(raw, str):
            return cls(raw, -1)
        else:
            raise TypeError(f'unsupported "raw": {raw:!r}')

    def __str__(self):
        return self.filename

    def fix_filename(self, relroot=fsutil.USE_CWD, **kwargs):
        filename = _fix_filename(self.filename, relroot, **kwargs)
        if filename == self.filename:
            return self
        return self._replace(filename=filename)


class SourceLine(namedtuple('Line', 'file kind data conditions')):
    KINDS = (
        #'directive',  # data is ...
        'source',  # "data" is the line
        #'comment',  # "data" is the text, including comment markers
    )

    @property
    def filename(self):
        return self.file.filename

    @property
    def lno(self):
        return self.file.lno


class DeclID(namedtuple('DeclID', 'filename funcname name')):
    """The globally-unique identifier for a declaration."""

    @classmethod
    def from_row(cls, row, **markers):
        row = _tables.fix_row(row, **markers)
        return cls(*row)

    # We have to provde _make() becaose we implemented __new__().

    @classmethod
    def _make(cls, iterable):
        try:
            return cls(*iterable)
        except Exception:
            super()._make(iterable)
            raise  # re-raise

    def __new__(cls, filename, funcname, name):
        self = super().__new__(
            cls,
            filename=str(filename) if filename else None,
            funcname=str(funcname) if funcname else None,
            name=str(name) if name else None,
        )
        self._compare = tuple(v or '' for v in self)
        return self

    def __hash__(self):
        return super().__hash__()

    def __eq__(self, other):
        try:
            other = tuple(v or '' for v in other)
        except TypeError:
            return NotImplemented
        return self._compare == other

    def __gt__(self, other):
        try:
            other = tuple(v or '' for v in other)
        except TypeError:
            return NotImplemented
        return self._compare > other

    def fix_filename(self, relroot=fsutil.USE_CWD, **kwargs):
        filename = _fix_filename(self.filename, relroot, **kwargs)
        if filename == self.filename:
            return self
        return self._replace(filename=filename)


class ParsedItem(namedtuple('ParsedItem', 'file kind parent name data')):

    @classmethod
    def from_raw(cls, raw):
        if isinstance(raw, cls):
            return raw
        elif isinstance(raw, tuple):
            return cls(*raw)
        else:
            raise TypeError(f'unsupported "raw": {raw:!r}')

    @classmethod
    def from_row(cls, row, columns=None):
        if not columns:
            colnames = 'filename funcname name kind data'.split()
        else:
            colnames = list(columns)
            for i, column in enumerate(colnames):
                if column == 'file':
                    colnames[i] = 'filename'
                elif column == 'funcname':
                    colnames[i] = 'parent'
        if len(row) != len(set(colnames)):
            raise NotImplementedError(columns, row)
        kwargs = {}
        for column, value in zip(colnames, row):
            if column == 'filename':
                kwargs['file'] = FileInfo.from_raw(value)
            elif column == 'kind':
                kwargs['kind'] = KIND(value)
            elif column in cls._fields:
                kwargs[column] = value
            else:
                raise NotImplementedError(column)
        return cls(**kwargs)

    @property
    def id(self):
        try:
            return self._id
        except AttributeError:
            if self.kind is KIND.STATEMENT:
                self._id = None
            else:
                self._id = DeclID(str(self.file), self.funcname, self.name)
            return self._id

    @property
    def filename(self):
        if not self.file:
            return None
        return self.file.filename

    @property
    def lno(self):
        if not self.file:
            return -1
        return self.file.lno

    @property
    def funcname(self):
        if not self.parent:
            return None
        if type(self.parent) is str:
            return self.parent
        else:
            return self.parent.name

    def fix_filename(self, relroot=fsutil.USE_CWD, **kwargs):
        fixed = self.file.fix_filename(relroot, **kwargs)
        if fixed == self.file:
            return self
        return self._replace(file=fixed)

    def as_row(self, columns=None):
        if not columns:
            columns = self._fields
        row = []
        for column in columns:
            if column == 'file':
                value = self.filename
            elif column == 'kind':
                value = self.kind.value
            elif column == 'data':
                value = self._render_data()
            else:
                value = getattr(self, column)
            row.append(value)
        return row

    def _render_data(self):
        if not self.data:
            return None
        elif isinstance(self.data, str):
            return self.data
        else:
            # XXX
            raise NotImplementedError


def _get_vartype(data):
    try:
        vartype = dict(data['vartype'])
    except KeyError:
        vartype = dict(data)
        storage = data.get('storage')
    else:
        storage = data.get('storage') or vartype.get('storage')
    del vartype['storage']
    return storage, vartype


def get_parsed_vartype(decl):
    kind = getattr(decl, 'kind', None)
    if isinstance(decl, ParsedItem):
        storage, vartype = _get_vartype(decl.data)
        typequal = vartype['typequal']
        typespec = vartype['typespec']
        abstract = vartype['abstract']
    elif isinstance(decl, dict):
        kind = decl.get('kind')
        storage, vartype = _get_vartype(decl)
        typequal = vartype['typequal']
        typespec = vartype['typespec']
        abstract = vartype['abstract']
    elif isinstance(decl, VarType):
        storage = None
        typequal, typespec, abstract = decl
    elif isinstance(decl, TypeDef):
        storage = None
        typequal, typespec, abstract = decl.vartype
    elif isinstance(decl, Variable):
        storage = decl.storage
        typequal, typespec, abstract = decl.vartype
    elif isinstance(decl, Signature):
        storage = None
        typequal, typespec, abstract = decl.returntype
    elif isinstance(decl, Function):
        storage = decl.storage
        typequal, typespec, abstract = decl.signature.returntype
    elif isinstance(decl, str):
        vartype, storage = VarType.from_str(decl)
        typequal, typespec, abstract = vartype
    else:
        raise NotImplementedError(decl)
    return kind, storage, typequal, typespec, abstract


def get_default_storage(decl):
    if decl.kind not in (KIND.VARIABLE, KIND.FUNCTION):
        return None
    return 'extern' if decl.parent is None else 'auto'


def get_effective_storage(decl, *, default=None):
    # Note that "static" limits access to just that C module
    # and "extern" (the default for module-level) allows access
    # outside the C module.
    if default is None:
        default = get_default_storage(decl)
        if default is None:
            return None
    try:
        storage = decl.storage
    except AttributeError:
        storage, _ = _get_vartype(decl.data)
    return storage or default


#############################
# high-level

class HighlevelParsedItem:

    kind = None

    FIELDS = ('file', 'parent', 'name', 'data')

    @classmethod
    def from_parsed(cls, parsed):
        if parsed.kind is not cls.kind:
            raise TypeError(f'kind mismatch ({parsed.kind.value} != {cls.kind.value})')
        data, extra = cls._resolve_data(parsed.data)
        self = cls(
            cls._resolve_file(parsed),
            parsed.name,
            data,
            cls._resolve_parent(parsed) if parsed.parent else None,
            **extra or {}
        )
        self._parsed = parsed
        return self

    @classmethod
    def _resolve_file(cls, parsed):
        fileinfo = FileInfo.from_raw(parsed.file)
        if not fileinfo:
            raise NotImplementedError(parsed)
        return fileinfo

    @classmethod
    def _resolve_data(cls, data):
        return data, None

    @classmethod
    def _raw_data(cls, data, extra):
        if isinstance(data, str):
            return data
        else:
            raise NotImplementedError(data)

    @classmethod
    def _data_as_row(cls, data, extra, colnames):
        row = {}
        for colname in colnames:
            if colname in row:
                continue
            rendered = cls._render_data_row_item(colname, data, extra)
            if rendered is iter(rendered):
                rendered, = rendered
            row[colname] = rendered
        return row

    @classmethod
    def _render_data_row_item(cls, colname, data, extra):
        if colname == 'data':
            return str(data)
        else:
            return None

    @classmethod
    def _render_data_row(cls, fmt, data, extra, colnames):
        if fmt != 'row':
            raise NotImplementedError
        datarow = cls._data_as_row(data, extra, colnames)
        unresolved = [c for c, v in datarow.items() if v is None]
        if unresolved:
            raise NotImplementedError(unresolved)
        for colname, value in datarow.items():
            if type(value) != str:
                if colname == 'kind':
                    datarow[colname] = value.value
                else:
                    datarow[colname] = str(value)
        return datarow

    @classmethod
    def _render_data(cls, fmt, data, extra):
        row = cls._render_data_row(fmt, data, extra, ['data'])
        yield ' '.join(row.values())

    @classmethod
    def _resolve_parent(cls, parsed, *, _kind=None):
        fileinfo = FileInfo(parsed.file.filename, -1)
        if isinstance(parsed.parent, str):
            if parsed.parent.isidentifier():
                name = parsed.parent
            else:
                # XXX It could be something like "<kind> <name>".
                raise NotImplementedError(repr(parsed.parent))
            parent = ParsedItem(fileinfo, _kind, None, name, None)
        elif type(parsed.parent) is tuple:
            # XXX It could be something like (kind, name).
            raise NotImplementedError(repr(parsed.parent))
        else:
            return parsed.parent
        Parent = KIND_CLASSES.get(_kind, Declaration)
        return Parent.from_parsed(parent)

    @classmethod
    def _parse_columns(cls, columns):
        colnames = {}  # {requested -> actual}
        columns = list(columns or cls.FIELDS)
        datacolumns = []
        for i, colname in enumerate(columns):
            if colname == 'file':
                columns[i] = 'filename'
                colnames['file'] = 'filename'
            elif colname == 'lno':
                columns[i] = 'line'
                colnames['lno'] = 'line'
            elif colname in ('filename', 'line'):
                colnames[colname] = colname
            elif colname == 'data':
                datacolumns.append(colname)
                colnames[colname] = None
            elif colname in cls.FIELDS or colname == 'kind':
                colnames[colname] = colname
            else:
                datacolumns.append(colname)
                colnames[colname] = None
        return columns, datacolumns, colnames

    def __init__(self, file, name, data, parent=None, *,
                 _extra=None,
                 _shortkey=None,
                 _key=None,
                 ):
        self.file = file
        self.parent = parent or None
        self.name = name
        self.data = data
        self._extra = _extra or {}
        self._shortkey = _shortkey
        self._key = _key

    def __repr__(self):
        args = [f'{n}={getattr(self, n)!r}'
                for n in ['file', 'name', 'data', 'parent', *(self._extra or ())]]
        return f'{type(self).__name__}({", ".join(args)})'

    def __str__(self):
        try:
            return self._str
        except AttributeError:
            self._str = next(self.render())
            return self._str

    def __getattr__(self, name):
        try:
            return self._extra[name]
        except KeyError:
            raise AttributeError(name)

    def __hash__(self):
        return hash(self._key)

    def __eq__(self, other):
        if isinstance(other, HighlevelParsedItem):
            return self._key == other._key
        elif type(other) is tuple:
            return self._key == other
        else:
            return NotImplemented

    def __gt__(self, other):
        if isinstance(other, HighlevelParsedItem):
            return self._key > other._key
        elif type(other) is tuple:
            return self._key > other
        else:
            return NotImplemented

    @property
    def id(self):
        return self.parsed.id

    @property
    def shortkey(self):
        return self._shortkey

    @property
    def key(self):
        return self._key

    @property
    def filename(self):
        if not self.file:
            return None
        return self.file.filename

    @property
    def parsed(self):
        try:
            return self._parsed
        except AttributeError:
            parent = self.parent
            if parent is not None and not isinstance(parent, str):
                parent = parent.name
            self._parsed = ParsedItem(
                self.file,
                self.kind,
                parent,
                self.name,
                self._raw_data(),
            )
            return self._parsed

    def fix_filename(self, relroot=fsutil.USE_CWD, **kwargs):
        if self.file:
            self.file = self.file.fix_filename(relroot, **kwargs)
        return self

    def as_rowdata(self, columns=None):
        columns, datacolumns, colnames = self._parse_columns(columns)
        return self._as_row(colnames, datacolumns, self._data_as_row)

    def render_rowdata(self, columns=None):
        columns, datacolumns, colnames = self._parse_columns(columns)
        def data_as_row(data, ext, cols):
            return self._render_data_row('row', data, ext, cols)
        rowdata = self._as_row(colnames, datacolumns, data_as_row)
        for column, value in rowdata.items():
            colname = colnames.get(column)
            if not colname:
                continue
            if column == 'kind':
                value = value.value
            else:
                if column == 'parent':
                    if self.parent:
                        value = f'({self.parent.kind.value} {self.parent.name})'
                if not value:
                    value = '-'
                elif type(value) is VarType:
                    value = repr(str(value))
                else:
                    value = str(value)
            rowdata[column] = value
        return rowdata

    def _as_row(self, colnames, datacolumns, data_as_row):
        try:
            data = data_as_row(self.data, self._extra, datacolumns)
        except NotImplementedError:
            data = None
        row = data or {}
        for column, colname in colnames.items():
            if colname == 'filename':
                value = self.file.filename if self.file else None
            elif colname == 'line':
                value = self.file.lno if self.file else None
            elif colname is None:
                value = getattr(self, column, None)
            else:
                value = getattr(self, colname, None)
            row.setdefault(column, value)
        return row

    def render(self, fmt='line'):
        fmt = fmt or 'line'
        try:
            render = _FORMATS[fmt]
        except KeyError:
            raise TypeError(f'unsupported fmt {fmt!r}')
        try:
            data = self._render_data(fmt, self.data, self._extra)
        except NotImplementedError:
            data = '-'
        yield from render(self, data)


### formats ###

def _fmt_line(parsed, data=None):
    parts = [
        f'<{parsed.kind.value}>',
    ]
    parent = ''
    if parsed.parent:
        parent = parsed.parent
        if not isinstance(parent, str):
            if parent.kind is KIND.FUNCTION:
                parent = f'{parent.name}()'
            else:
                parent = parent.name
        name = f'<{parent}>.{parsed.name}'
    else:
        name = parsed.name
    if data is None:
        data = parsed.data
    elif data is iter(data):
        data, = data
    parts.extend([
        name,
        f'<{data}>' if data else '-',
        f'({str(parsed.file or "<unknown file>")})',
    ])
    yield '\t'.join(parts)


def _fmt_full(parsed, data=None):
    if parsed.kind is KIND.VARIABLE and parsed.parent:
        prefix = 'local '
        suffix = f' ({parsed.parent.name})'
    else:
        # XXX Show other prefixes (e.g. global, public)
        prefix = suffix = ''
    yield f'{prefix}{parsed.kind.value} {parsed.name!r}{suffix}'
    for column, info in parsed.render_rowdata().items():
        if column == 'kind':
            continue
        if column == 'name':
            continue
        if column == 'parent' and parsed.kind is not KIND.VARIABLE:
            continue
        if column == 'data':
            if parsed.kind in (KIND.STRUCT, KIND.UNION):
                column = 'members'
            elif parsed.kind is KIND.ENUM:
                column = 'enumerators'
            elif parsed.kind is KIND.STATEMENT:
                column = 'text'
                data, = data
            else:
                column = 'signature'
                data, = data
            if not data:
#                yield f'\t{column}:\t-'
                continue
            elif isinstance(data, str):
                yield f'\t{column}:\t{data!r}'
            else:
                yield f'\t{column}:'
                for line in data:
                    yield f'\t\t- {line}'
        else:
            yield f'\t{column}:\t{info}'


_FORMATS = {
    'raw': (lambda v, _d: [repr(v)]),
    'brief': _fmt_line,
    'line': _fmt_line,
    'full': _fmt_full,
}


### declarations ##

class Declaration(HighlevelParsedItem):

    @classmethod
    def from_row(cls, row, **markers):
        fixed = tuple(_tables.fix_row(row, **markers))
        if cls is Declaration:
            _, _, _, kind, _ = fixed
            sub = KIND_CLASSES.get(KIND(kind))
            if not sub or not issubclass(sub, Declaration):
                raise TypeError(f'unsupported kind, got {row!r}')
        else:
            sub = cls
        return sub._from_row(fixed)

    @classmethod
    def _from_row(cls, row):
        filename, funcname, name, kind, data = row
        kind = KIND._from_raw(kind)
        if kind is not cls.kind:
            raise TypeError(f'expected kind {cls.kind.value!r}, got {row!r}')
        fileinfo = FileInfo.from_raw(filename)
        extra = None
        if isinstance(data, str):
            data, extra = cls._parse_data(data, fmt='row')
        if extra:
            return cls(fileinfo, name, data, funcname, _extra=extra)
        else:
            return cls(fileinfo, name, data, funcname)

    @classmethod
    def _resolve_parent(cls, parsed, *, _kind=None):
        if _kind is None:
            raise TypeError(f'{cls.kind.value} declarations do not have parents ({parsed})')
        return super()._resolve_parent(parsed, _kind=_kind)

    @classmethod
    def _render_data(cls, fmt, data, extra):
        if not data:
            # XXX There should be some!  Forward?
            yield '???'
        else:
            yield from cls._format_data(fmt, data, extra)

    @classmethod
    def _render_data_row_item(cls, colname, data, extra):
        if colname == 'data':
            return cls._format_data('row', data, extra)
        else:
            return None

    @classmethod
    def _format_data(cls, fmt, data, extra):
        raise NotImplementedError(fmt)

    @classmethod
    def _parse_data(cls, datastr, fmt=None):
        """This is the reverse of _render_data."""
        if not datastr or datastr is _tables.UNKNOWN or datastr == '???':
            return None, None
        elif datastr is _tables.EMPTY or datastr == '-':
            # All the kinds have *something* even it is unknown.
            raise TypeError('all declarations have data of some sort, got none')
        else:
            return cls._unformat_data(datastr, fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        raise NotImplementedError(fmt)


class VarType(namedtuple('VarType', 'typequal typespec abstract')):

    @classmethod
    def from_str(cls, text):
        orig = text
        storage, sep, text = text.strip().partition(' ')
        if not sep:
            text = storage
            storage = None
        elif storage not in ('auto', 'register', 'static', 'extern'):
            text = orig
            storage = None
        return cls._from_str(text), storage

    @classmethod
    def _from_str(cls, text):
        orig = text
        if text.startswith(('const ', 'volatile ')):
            typequal, _, text = text.partition(' ')
        else:
            typequal = None

        # Extract a series of identifiers/keywords.
        m = re.match(r"^ *'?([a-zA-Z_]\w*(?:\s+[a-zA-Z_]\w*)*)\s*(.*?)'?\s*$", text)
        if not m:
            raise ValueError(f'invalid vartype text {orig!r}')
        typespec, abstract = m.groups()

        return cls(typequal, typespec, abstract or None)

    def __str__(self):
        parts = []
        if self.qualifier:
            parts.append(self.qualifier)
        parts.append(self.spec + (self.abstract or ''))
        return ' '.join(parts)

    @property
    def qualifier(self):
        return self.typequal

    @property
    def spec(self):
        return self.typespec


class Variable(Declaration):
    kind = KIND.VARIABLE

    @classmethod
    def _resolve_parent(cls, parsed):
        return super()._resolve_parent(parsed, _kind=KIND.FUNCTION)

    @classmethod
    def _resolve_data(cls, data):
        if not data:
            return None, None
        storage, vartype = _get_vartype(data)
        return VarType(**vartype), {'storage': storage}

    @classmethod
    def _raw_data(self, data, extra):
        vartype = data._asdict()
        return {
            'storage': extra['storage'],
            'vartype': vartype,
        }

    @classmethod
    def _format_data(cls, fmt, data, extra):
        storage = extra.get('storage')
        text = f'{storage} {data}' if storage else str(data)
        if fmt in ('line', 'brief'):
            yield text
        #elif fmt == 'full':
        elif fmt == 'row':
            yield text
        else:
            raise NotImplementedError(fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        if fmt in ('line', 'brief'):
            vartype, storage = VarType.from_str(datastr)
            return vartype, {'storage': storage}
        #elif fmt == 'full':
        elif fmt == 'row':
            vartype, storage = VarType.from_str(datastr)
            return vartype, {'storage': storage}
        else:
            raise NotImplementedError(fmt)

    def __init__(self, file, name, data, parent=None, storage=None):
        super().__init__(file, name, data, parent,
                         _extra={'storage': storage or None},
                         _shortkey=f'({parent.name}).{name}' if parent else name,
                         _key=(str(file),
                               # Tilde comes after all other ascii characters.
                               f'~{parent or ""}~',
                               name,
                               ),
                         )
        if storage:
            if storage not in STORAGE:
                # The parser must need an update.
                raise NotImplementedError(storage)
            # Otherwise we trust the compiler to have validated it.

    @property
    def vartype(self):
        return self.data


class Signature(namedtuple('Signature', 'params returntype inline isforward')):

    @classmethod
    def from_str(cls, text):
        orig = text
        storage, sep, text = text.strip().partition(' ')
        if not sep:
            text = storage
            storage = None
        elif storage not in ('auto', 'register', 'static', 'extern'):
            text = orig
            storage = None
        return cls._from_str(text), storage

    @classmethod
    def _from_str(cls, text):
        orig = text
        inline, sep, text = text.partition('|')
        if not sep:
            text = inline
            inline = None

        isforward = False
        if text.endswith(';'):
            text = text[:-1]
            isforward = True
        elif text.endswith('{}'):
            text = text[:-2]

        index = text.rindex('(')
        if index < 0:
            raise ValueError(f'bad signature text {orig!r}')
        params = text[index:]
        while params.count('(') <= params.count(')'):
            index = text.rindex('(', 0, index)
            if index < 0:
                raise ValueError(f'bad signature text {orig!r}')
            params = text[index:]
        text = text[:index]

        returntype = VarType._from_str(text.rstrip())

        return cls(params, returntype, inline, isforward)

    def __str__(self):
        parts = []
        if self.inline:
            parts.extend([
                self.inline,
                '|',
            ])
        parts.extend([
            str(self.returntype),
            self.params,
            ';' if self.isforward else '{}',
        ])
        return ' '.join(parts)

    @property
    def returns(self):
        return self.returntype

    @property
    def typequal(self):
        return self.returntype.typequal

    @property
    def typespec(self):
        return self.returntype.typespec

    @property
    def abstract(self):
        return self.returntype.abstract


class Function(Declaration):
    kind = KIND.FUNCTION

    @classmethod
    def _resolve_data(cls, data):
        if not data:
            return None, None
        kwargs = dict(data)
        returntype = dict(data['returntype'])
        del returntype['storage']
        kwargs['returntype'] = VarType(**returntype)
        storage = kwargs.pop('storage')
        return Signature(**kwargs), {'storage': storage}

    @classmethod
    def _raw_data(self, data):
        # XXX finish!
        return data

    @classmethod
    def _format_data(cls, fmt, data, extra):
        storage = extra.get('storage')
        text = f'{storage} {data}' if storage else str(data)
        if fmt in ('line', 'brief'):
            yield text
        #elif fmt == 'full':
        elif fmt == 'row':
            yield text
        else:
            raise NotImplementedError(fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        if fmt in ('line', 'brief'):
            sig, storage = Signature.from_str(sig)
            return sig, {'storage': storage}
        #elif fmt == 'full':
        elif fmt == 'row':
            sig, storage = Signature.from_str(sig)
            return sig, {'storage': storage}
        else:
            raise NotImplementedError(fmt)

    def __init__(self, file, name, data, parent=None, storage=None):
        super().__init__(file, name, data, parent, _extra={'storage': storage})
        self._shortkey = f'~{name}~ {self.data}'
        self._key = (
            str(file),
            self._shortkey,
        )

    @property
    def signature(self):
        return self.data


class TypeDeclaration(Declaration):

    def __init__(self, file, name, data, parent=None, *, _shortkey=None):
        if not _shortkey:
            _shortkey = f'{self.kind.value} {name}'
        super().__init__(file, name, data, parent,
                         _shortkey=_shortkey,
                         _key=(
                             str(file),
                             _shortkey,
                             ),
                         )


class POTSType(TypeDeclaration):

    def __init__(self, name):
        _file = _data = _parent = None
        super().__init__(_file, name, _data, _parent, _shortkey=name)


class FuncPtr(TypeDeclaration):

    def __init__(self, vartype):
        _file = _name = _parent = None
        data = vartype
        self.vartype = vartype
        super().__init__(_file, _name, data, _parent, _shortkey=f'<{vartype}>')


class TypeDef(TypeDeclaration):
    kind = KIND.TYPEDEF

    @classmethod
    def _resolve_data(cls, data):
        if not data:
            raise NotImplementedError(data)
        kwargs = dict(data)
        del kwargs['storage']
        if 'returntype' in kwargs:
            vartype = kwargs['returntype']
            del vartype['storage']
            kwargs['returntype'] = VarType(**vartype)
            datacls = Signature
        else:
            datacls = VarType
        return datacls(**kwargs), None

    @classmethod
    def _raw_data(self, data):
        # XXX finish!
        return data

    @classmethod
    def _format_data(cls, fmt, data, extra):
        text = str(data)
        if fmt in ('line', 'brief'):
            yield text
        elif fmt == 'full':
            yield text
        elif fmt == 'row':
            yield text
        else:
            raise NotImplementedError(fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        if fmt in ('line', 'brief'):
            vartype, _ = VarType.from_str(datastr)
            return vartype, None
        #elif fmt == 'full':
        elif fmt == 'row':
            vartype, _ = VarType.from_str(datastr)
            return vartype, None
        else:
            raise NotImplementedError(fmt)

    def __init__(self, file, name, data, parent=None):
        super().__init__(file, name, data, parent, _shortkey=name)

    @property
    def vartype(self):
        return self.data


class Member(namedtuple('Member', 'name vartype size')):

    @classmethod
    def from_data(cls, raw, index):
        name = raw.name if raw.name else index
        vartype = size = None
        if type(raw.data) is int:
            size = raw.data
        elif isinstance(raw.data, str):
            size = int(raw.data)
        elif raw.data:
            vartype = dict(raw.data)
            del vartype['storage']
            if 'size' in vartype:
                size = vartype.pop('size')
                if isinstance(size, str) and size.isdigit():
                    size = int(size)
            vartype = VarType(**vartype)
        return cls(name, vartype, size)

    @classmethod
    def from_str(cls, text):
        name, _, vartype = text.partition(': ')
        if name.startswith('#'):
            name = int(name[1:])
        if vartype.isdigit():
            size = int(vartype)
            vartype = None
        else:
            vartype, _ = VarType.from_str(vartype)
            size = None
        return cls(name, vartype, size)

    def __str__(self):
        name = self.name if isinstance(self.name, str) else f'#{self.name}'
        return f'{name}: {self.vartype or self.size}'


class _StructUnion(TypeDeclaration):

    @classmethod
    def _resolve_data(cls, data):
        if not data:
            # XXX There should be some!  Forward?
            return None, None
        return [Member.from_data(v, i) for i, v in enumerate(data)], None

    @classmethod
    def _raw_data(self, data):
        # XXX finish!
        return data

    @classmethod
    def _format_data(cls, fmt, data, extra):
        if fmt in ('line', 'brief'):
            members = ', '.join(f'<{m}>' for m in data)
            yield f'[{members}]'
        elif fmt == 'full':
            for member in data:
                yield f'{member}'
        elif fmt == 'row':
            members = ', '.join(f'<{m}>' for m in data)
            yield f'[{members}]'
        else:
            raise NotImplementedError(fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        if fmt in ('line', 'brief'):
            members = [Member.from_str(m[1:-1])
                       for m in datastr[1:-1].split(', ')]
            return members, None
        #elif fmt == 'full':
        elif fmt == 'row':
            members = [Member.from_str(m.rstrip('>').lstrip('<'))
                       for m in datastr[1:-1].split('>, <')]
            return members, None
        else:
            raise NotImplementedError(fmt)

    def __init__(self, file, name, data, parent=None):
        super().__init__(file, name, data, parent)

    @property
    def members(self):
        return self.data


class Struct(_StructUnion):
    kind = KIND.STRUCT


class Union(_StructUnion):
    kind = KIND.UNION


class Enum(TypeDeclaration):
    kind = KIND.ENUM

    @classmethod
    def _resolve_data(cls, data):
        if not data:
            # XXX There should be some!  Forward?
            return None, None
        enumerators = [e if isinstance(e, str) else e.name
                       for e in data]
        return enumerators, None

    @classmethod
    def _raw_data(self, data):
        # XXX finish!
        return data

    @classmethod
    def _format_data(cls, fmt, data, extra):
        if fmt in ('line', 'brief'):
            yield repr(data)
        elif fmt == 'full':
            for enumerator in data:
                yield f'{enumerator}'
        elif fmt == 'row':
            # XXX This won't work with CSV...
            yield ','.join(data)
        else:
            raise NotImplementedError(fmt)

    @classmethod
    def _unformat_data(cls, datastr, fmt=None):
        if fmt in ('line', 'brief'):
            return _strutil.unrepr(datastr), None
        #elif fmt == 'full':
        elif fmt == 'row':
            return datastr.split(','), None
        else:
            raise NotImplementedError(fmt)

    def __init__(self, file, name, data, parent=None):
        super().__init__(file, name, data, parent)

    @property
    def enumerators(self):
        return self.data


### statements ###

class Statement(HighlevelParsedItem):
    kind = KIND.STATEMENT

    @classmethod
    def _resolve_data(cls, data):
        # XXX finish!
        return data, None

    @classmethod
    def _raw_data(self, data):
        # XXX finish!
        return data

    @classmethod
    def _render_data(cls, fmt, data, extra):
        # XXX Handle other formats?
        return repr(data)

    @classmethod
    def _parse_data(self, datastr, fmt=None):
        # XXX Handle other formats?
        return _strutil.unrepr(datastr), None

    def __init__(self, file, name, data, parent=None):
        super().__init__(file, name, data, parent,
                         _shortkey=data or '',
                         _key=(
                             str(file),
                             file.lno,
                             # XXX Only one stmt per line?
                             ),
                         )

    @property
    def text(self):
        return self.data


###

KIND_CLASSES = {cls.kind: cls for cls in [
    Variable,
    Function,
    TypeDef,
    Struct,
    Union,
    Enum,
    Statement,
]}


def resolve_parsed(parsed):
    if isinstance(parsed, HighlevelParsedItem):
        return parsed
    try:
        cls = KIND_CLASSES[parsed.kind]
    except KeyError:
        raise ValueError(f'unsupported kind in {parsed!r}')
    return cls.from_parsed(parsed)


def set_flag(item, name, value):
    try:
        setattr(item, name, value)
    except AttributeError:
        object.__setattr__(item, name, value)


#############################
# composite

class Declarations:

    @classmethod
    def from_decls(cls, decls):
        return cls(decls)

    @classmethod
    def from_parsed(cls, items):
        decls = (resolve_parsed(item)
                 for item in items
                 if item.kind is not KIND.STATEMENT)
        return cls.from_decls(decls)

    @classmethod
    def _resolve_key(cls, raw):
        if isinstance(raw, str):
            raw = [raw]
        elif isinstance(raw, Declaration):
            raw = (
                raw.filename if cls._is_public(raw) else None,
                # `raw.parent` is always None for types and functions.
                raw.parent if raw.kind is KIND.VARIABLE else None,
                raw.name,
            )

        extra = None
        if len(raw) == 1:
            name, = raw
            if name:
                name = str(name)
                if name.endswith(('.c', '.h')):
                    # This is only legit as a query.
                    key = (name, None, None)
                else:
                    key = (None, None, name)
            else:
                key = (None, None, None)
        elif len(raw) == 2:
            parent, name = raw
            name = str(name)
            if isinstance(parent, Declaration):
                key = (None, parent.name, name)
            elif not parent:
                key = (None, None, name)
            else:
                parent = str(parent)
                if parent.endswith(('.c', '.h')):
                    key = (parent, None, name)
                else:
                    key = (None, parent, name)
        else:
            key, extra = raw[:3], raw[3:]
            filename, funcname, name = key
            filename = str(filename) if filename else None
            if isinstance(funcname, Declaration):
                funcname = funcname.name
            else:
                funcname = str(funcname) if funcname else None
            name = str(name) if name else None
            key = (filename, funcname, name)
        return key, extra

    @classmethod
    def _is_public(cls, decl):
        # For .c files don't we need info from .h files to make this decision?
        # XXX Check for "extern".
        # For now we treat all decls a "private" (have filename set).
        return False

    def __init__(self, decls):
        # (file, func, name) -> decl
        # "public":
        #   * (None, None, name)
        # "private", "global":
        #   * (file, None, name)
        # "private", "local":
        #   * (file, func, name)
        if hasattr(decls, 'items'):
            self._decls = decls
        else:
            self._decls = {}
            self._extend(decls)

        # XXX always validate?

    def validate(self):
        for key, decl in self._decls.items():
            if type(key) is not tuple or len(key) != 3:
                raise ValueError(f'expected 3-tuple key, got {key!r} (for decl {decl!r})')
            filename, funcname, name = key
            if not name:
                raise ValueError(f'expected name in key, got {key!r} (for decl {decl!r})')
            elif type(name) is not str:
                raise ValueError(f'expected name in key to be str, got {key!r} (for decl {decl!r})')
            # XXX Check filename type?
            # XXX Check funcname type?

            if decl.kind is KIND.STATEMENT:
                raise ValueError(f'expected a declaration, got {decl!r}')

    def __repr__(self):
        return f'{type(self).__name__}({list(self)})'

    def __len__(self):
        return len(self._decls)

    def __iter__(self):
        yield from self._decls

    def __getitem__(self, key):
        # XXX Be more exact for the 3-tuple case?
        if type(key) not in (str, tuple):
            raise KeyError(f'unsupported key {key!r}')
        resolved, extra = self._resolve_key(key)
        if extra:
            raise KeyError(f'key must have at most 3 parts, got {key!r}')
        if not resolved[2]:
            raise ValueError(f'expected name in key, got {key!r}')
        try:
            return self._decls[resolved]
        except KeyError:
            if type(key) is tuple and len(key) == 3:
                filename, funcname, name = key
            else:
                filename, funcname, name = resolved
            if filename and not filename.endswith(('.c', '.h')):
                raise KeyError(f'invalid filename in key {key!r}')
            elif funcname and funcname.endswith(('.c', '.h')):
                raise KeyError(f'invalid funcname in key {key!r}')
            elif name and name.endswith(('.c', '.h')):
                raise KeyError(f'invalid name in key {key!r}')
            else:
                raise  # re-raise

    @property
    def types(self):
        return self._find(kind=KIND.TYPES)

    @property
    def functions(self):
        return self._find(None, None, None, KIND.FUNCTION)

    @property
    def variables(self):
        return self._find(None, None, None, KIND.VARIABLE)

    def iter_all(self):
        yield from self._decls.values()

    def get(self, key, default=None):
        try:
            return self[key]
        except KeyError:
            return default

    #def add_decl(self, decl, key=None):
    #    decl = _resolve_parsed(decl)
    #    self._add_decl(decl, key)

    def find(self, *key, **explicit):
        if not key:
            if not explicit:
                return iter(self)
            return self._find(**explicit)

        resolved, extra = self._resolve_key(key)
        filename, funcname, name = resolved
        if not extra:
            kind = None
        elif len(extra) == 1:
            kind, = extra
        else:
            raise KeyError(f'key must have at most 4 parts, got {key!r}')

        implicit= {}
        if filename:
            implicit['filename'] = filename
        if funcname:
            implicit['funcname'] = funcname
        if name:
            implicit['name'] = name
        if kind:
            implicit['kind'] = kind
        return self._find(**implicit, **explicit)

    def _find(self, filename=None, funcname=None, name=None, kind=None):
        for decl in self._decls.values():
            if filename and decl.filename != filename:
                continue
            if funcname:
                if decl.kind is not KIND.VARIABLE:
                    continue
                if decl.parent.name != funcname:
                    continue
            if name and decl.name != name:
                continue
            if kind:
                kinds = KIND.resolve_group(kind)
                if decl.kind not in kinds:
                    continue
            yield decl

    def _add_decl(self, decl, key=None):
        if key:
            if type(key) not in (str, tuple):
                raise NotImplementedError((key, decl))
            # Any partial key will be turned into a full key, but that
            # same partial key will still match a key lookup.
            resolved, _ = self._resolve_key(key)
            if not resolved[2]:
                raise ValueError(f'expected name in key, got {key!r}')
            key = resolved
            # XXX Also add with the decl-derived key if not the same?
        else:
            key, _ = self._resolve_key(decl)
        self._decls[key] = decl

    def _extend(self, decls):
        decls = iter(decls)
        # Check only the first item.
        for decl in decls:
            if isinstance(decl, Declaration):
                self._add_decl(decl)
                # Add the rest without checking.
                for decl in decls:
                    self._add_decl(decl)
            elif isinstance(decl, HighlevelParsedItem):
                raise NotImplementedError(decl)
            else:
                try:
                    key, decl = decl
                except ValueError:
                    raise NotImplementedError(decl)
                if not isinstance(decl, Declaration):
                    raise NotImplementedError(decl)
                self._add_decl(decl, key)
                # Add the rest without checking.
                for key, decl in decls:
                    self._add_decl(decl, key)
            # The iterator will be exhausted at this point.
