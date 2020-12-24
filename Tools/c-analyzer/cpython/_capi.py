from collections import namedtuple
import os
import os.path
import re
import textwrap

from c_common.tables import build_table, resolve_columns
from c_parser.parser._regexes import _ind
from ._files import iter_header_files, resolve_filename
from . import REPO_ROOT


INCLUDE_ROOT = os.path.join(REPO_ROOT, 'Include')
INCLUDE_CPYTHON = os.path.join(INCLUDE_ROOT, 'cpython')
INCLUDE_INTERNAL = os.path.join(INCLUDE_ROOT, 'internal')

_MAYBE_NESTED_PARENS = textwrap.dedent(r'''
    (?:
        (?: [^(]* [(] [^()]* [)] )* [^(]*
    )
''')

CAPI_FUNC = textwrap.dedent(rf'''
    (?:
        ^
        \s*
        PyAPI_FUNC \s*
        [(]
        {_ind(_MAYBE_NESTED_PARENS, 2)}
        [)] \s*
        (\w+)  # <func>
        \s* [(]
    )
''')
CAPI_DATA = textwrap.dedent(rf'''
    (?:
        ^
        \s*
        PyAPI_DATA \s*
        [(]
        {_ind(_MAYBE_NESTED_PARENS, 2)}
        [)] \s*
        (\w+)  # <data>
        \b [^(]
    )
''')
CAPI_INLINE = textwrap.dedent(r'''
    (?:
        ^
        \s*
        static \s+ inline \s+
        .*?
        \s+
        ( \w+ )  # <inline>
        \s* [(]
    )
''')
CAPI_MACRO = textwrap.dedent(r'''
    (?:
        (\w+)  # <macro>
        [(]
    )
''')
CAPI_CONSTANT = textwrap.dedent(r'''
    (?:
        (\w+)  # <constant>
        \s+ [^(]
    )
''')
CAPI_DEFINE = textwrap.dedent(rf'''
    (?:
        ^
        \s* [#] \s* define \s+
        (?:
            {_ind(CAPI_MACRO, 3)}
            |
            {_ind(CAPI_CONSTANT, 3)}
            |
            (?:
                # ignored
                \w+   # <defined_name>
                \s*
                $
            )
        )
    )
''')
CAPI_RE = re.compile(textwrap.dedent(rf'''
    (?:
        {_ind(CAPI_FUNC, 2)}
        |
        {_ind(CAPI_DATA, 2)}
        |
        {_ind(CAPI_INLINE, 2)}
        |
        {_ind(CAPI_DEFINE, 2)}
    )
'''), re.VERBOSE)

KINDS = [
    'func',
    'data',
    'inline',
    'macro',
    'constant',
]


def _parse_line(line, prev=None):
    last = line
    if prev:
        if not prev.endswith(os.linesep):
            prev += os.linesep
        line = prev + line
    m = CAPI_RE.match(line)
    if not m:
        if not prev and line.startswith('static inline '):
            return line  # the new "prev"
        #if 'PyAPI_' in line or '#define ' in line or ' define ' in line:
        #    print(line)
        return None
    results = zip(KINDS, m.groups())
    for kind, name in results:
        if name:
            clean = last.split('//')[0].strip()
            if clean.endswith('*/'):
                clean = clean.split('/*')[0].rstrip()
            if kind == 'macro' or kind == 'constant':
                if clean.endswith('\\'):
                    return line  # the new "prev"
            elif kind == 'inline':
                if not prev:
                    if not clean.endswith('}'):
                        return line  # the new "prev"
                elif clean != '}':
                    return line  # the new "prev"
            elif not clean.endswith(';'):
                return line  # the new "prev"
            return name, kind
    # It was a plain #define.
    return None


LEVELS = {
    'stable',
    'cpython',
    'private',
    'internal',
}

def _get_level(filename, name, *,
               _cpython=INCLUDE_CPYTHON + os.path.sep,
               _internal=INCLUDE_INTERNAL + os.path.sep,
               ):
    if filename.startswith(_internal):
        return 'internal'
    elif name.startswith('_'):
        return 'private'
    elif os.path.dirname(filename) == INCLUDE_ROOT:
        return 'stable'
    elif filename.startswith(_cpython):
        return 'cpython'
    else:
        raise NotImplementedError
    #return '???'


class CAPIItem(namedtuple('CAPIItem', 'file lno name kind level')):

    @classmethod
    def from_line(cls, line, filename, lno, prev=None):
        parsed = _parse_line(line, prev)
        if not parsed:
            return None, None
        if isinstance(parsed, str):
            # incomplete
            return None, parsed
        name, kind = parsed
        level = _get_level(filename, name)
        self = cls(filename, lno, name, kind, level)
        if prev:
            self._text = (prev + line).rstrip().splitlines()
        else:
            self._text = [line.rstrip()]
        return self, None

    @property
    def relfile(self):
        return self.file[len(REPO_ROOT) + 1:]

    @property
    def text(self):
        try:
            return self._text
        except AttributeError:
            # XXX Actually ready the text from disk?.
            self._text = []
            if self.kind == 'data':
                self._text = [
                    f'PyAPI_DATA(...) {self.name}',
                ]
            elif self.kind == 'func':
                self._text = [
                    f'PyAPI_FUNC(...) {self.name}(...);',
                ]
            elif self.kind == 'inline':
                self._text = [
                    f'static inline {self.name}(...);',
                ]
            elif self.kind == 'macro':
                self._text = [
                    f'#define {self.name}(...) \\',
                    f'    ...',
                ]
            elif self.kind == 'constant':
                self._text = [
                    f'#define {self.name} ...',
                ]
            else:
                raise NotImplementedError

            return self._text


def _parse_groupby(raw):
    if not raw:
        raw = 'kind'

    if isinstance(raw, str):
        groupby = raw.replace(',', ' ').strip().split()
    else:
        raise NotImplementedError

    if not all(v in ('kind', 'level') for v in groupby):
        raise ValueError(f'invalid groupby value {raw!r}')
    return groupby


def summarize(items, *, groupby='kind'):
    summary = {}

    groupby = _parse_groupby(groupby)[0]
    if groupby == 'kind':
        outers = KINDS
        inners = LEVELS
        def increment(item):
            summary[item.kind][item.level] += 1
    elif groupby == 'level':
        outers = LEVELS
        inners = KINDS
        def increment(item):
            summary[item.level][item.kind] += 1
    else:
        raise NotImplementedError

    for outer in outers:
        summary[outer] = _outer = {}
        for inner in inners:
            _outer[inner] = 0
    for item in items:
        increment(item)

    return summary


def _parse_capi(lines, filename):
    if isinstance(lines, str):
        lines = lines.splitlines()
    prev = None
    for lno, line in enumerate(lines, 1):
        parsed, prev = CAPIItem.from_line(line, filename, lno, prev)
        if parsed:
            yield parsed
    if prev:
        parsed, prev = CAPIItem.from_line('', filename, lno, prev)
        if parsed:
            yield parsed
        if prev:
            print('incomplete match:')
            print(filename)
            print(prev)
            raise Exception


def iter_capi(filenames=None):
    for filename in iter_header_files(filenames):
        with open(filename) as infile:
            for item in _parse_capi(infile, filename):
                yield item


def _collate(items, groupby):
    groupby = _parse_groupby(groupby)[0]
    maxfilename = maxname = maxkind = maxlevel = 0
    collated = {}
    for item in items:
        key = getattr(item, groupby)
        if key in collated:
            collated[key].append(item)
        else:
            collated[key] = [item]
        maxfilename = max(len(item.relfile), maxfilename)
        maxname = max(len(item.name), maxname)
        maxkind = max(len(item.kind), maxkind)
        maxlevel = max(len(item.level), maxlevel)
    maxextra = {
        'kind': maxkind,
        'level': maxlevel,
    }
    return collated, groupby, maxfilename, maxname, maxextra


##################################
# CLI rendering

_LEVEL_MARKERS = {
    'S': 'stable',
    'C': 'cpython',
    'P': 'private',
    'I': 'internal',
}
_KIND_MARKERS = {
    'F': 'func',
    'D': 'data',
    'I': 'inline',
    'M': 'macro',
    'C': 'constant',
}


def resolve_format(format):
    if not format:
        return 'brief'
    elif isinstance(format, str) and format in _FORMATS:
        return format
    else:
        return resolve_columns(format)


def get_renderer(format):
    format = resolve_format(format)
    if isinstance(format, str):
        try:
            return _FORMATS[format]
        except KeyError:
            raise ValueError(f'unsupported format {format!r}')
    else:
        def render(items, **kwargs):
            return render_table(items, columns=format, **kwargs)
        return render


def render_table(items, *, columns=None, groupby='kind', verbose=False):
    if groupby:
        collated, groupby, maxfilename, maxname, maxextra = _collate(items, groupby)
        if groupby == 'kind':
            groups = KINDS
            extras = ['level']
            markers = {'level': _LEVEL_MARKERS}
        elif groupby == 'level':
            groups = LEVELS
            extras = ['kind']
            markers = {'kind': _KIND_MARKERS}
        else:
            raise NotImplementedError
    else:
        # XXX Support no grouping?
        raise NotImplementedError

    if columns:
        def get_extra(item):
            return {extra: getattr(item, extra)
                    for extra in ('kind', 'level')}
    else:
        if verbose:
            maxextra['kind'] = max(len(kind) for kind in KINDS)
            maxextra['level'] = max(len(level) for level in LEVELS)
            extracols = [f'{extra}:{maxextra[extra]}'
                         for extra in extras]
            def get_extra(item):
                return {extra: getattr(item, extra)
                        for extra in extras}
        elif len(extras) == 1:
            extra, = extras
            extracols = [f'{m}:1' for m in markers[extra]]
            def get_extra(item):
                return {m: m if getattr(item, extra) == markers[extra][m] else ''
                        for m in markers[extra]}
        else:
            raise NotImplementedError
            #extracols = [[f'{m}:1' for m in markers[extra]]
            #             for extra in extras]
            #def get_extra(item):
            #    values = {}
            #    for extra in extras:
            #        cur = markers[extra]
            #        for m in cur:
            #            values[m] = m if getattr(item, m) == cur[m] else ''
            #    return values
        columns = [
            f'filename:{maxfilename}',
            f'name:{maxname}',
            *extracols,
        ]
    header, div, fmt = build_table(columns)

    total = 0
    for group in groups:
        if group not in collated:
            continue
        yield ''
        yield f' === {group} ==='
        yield ''
        yield header
        yield div
        for item in collated[group]:
            yield fmt.format(
                filename=item.relfile,
                name=item.name,
                **get_extra(item),
            )
        yield div
        subtotal = len(collated[group])
        yield f'  sub-total: {subtotal}'
        total += subtotal
    yield ''
    yield f'total: {total}'


def render_full(items, *, groupby=None, verbose=False):
    if groupby:
        collated, groupby, _, _, _ = _collate(items, groupby)
        for group, grouped in collated.items():
            yield '#' * 25
            yield f'# {group} ({len(grouped)})'
            yield '#' * 25
            yield ''
            if not grouped:
                continue
            for item in grouped:
                yield from _render_item_full(item, groupby, verbose)
                yield ''
    else:
        for item in items:
            yield from _render_item_full(item, None, verbose)
            yield ''


def _render_item_full(item, groupby, verbose):
    yield item.name
    yield f'  {"filename:":10} {item.relfile}'
    for extra in ('kind', 'level'):
        #if groupby != extra:
            yield f'  {extra+":":10} {getattr(item, extra)}'
    if verbose:
        print('  ---------------------------------------')
        for lno, line in enumerate(item.text, item.lno):
            print(f'  | {lno:3} {line}')
        print('  ---------------------------------------')


def render_summary(items, *, groupby='kind', verbose=False):
    total = 0
    summary = summarize(items, groupby=groupby)
    # XXX Stablize the sorting to match KINDS/LEVELS.
    for outer, counts in summary.items():
        subtotal = sum(c for _, c in counts.items())
        yield f'{outer + ":":20} ({subtotal})'
        for inner, count in counts.items():
            yield f'   {inner + ":":9} {count}'
        total += subtotal
    yield f'{"total:":20} ({total})'


_FORMATS = {
    'brief': render_table,
    'full': render_full,
    'summary': render_summary,
}
