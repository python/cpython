from collections import namedtuple
import logging
import os
import os.path
import re
import textwrap

from c_common.tables import build_table, resolve_columns
from c_parser.parser._regexes import _ind
from ._files import iter_header_files, resolve_filename
from . import REPO_ROOT


logger = logging.getLogger(__name__)


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
            clean = last.split('//')[0].rstrip()
            if clean.endswith('*/'):
                clean = clean.split('/*')[0].rstrip()

            if kind == 'macro' or kind == 'constant':
                if not clean.endswith('\\'):
                    return name, kind
            elif kind == 'inline':
                if clean.endswith('}'):
                    if not prev or clean == '}':
                        return name, kind
            elif kind == 'func' or kind == 'data':
                if clean.endswith(';'):
                    return name, kind
            else:
                # This should not be reached.
                raise NotImplementedError
            return line  # the new "prev"
    # It was a plain #define.
    return None


LEVELS = [
    'stable',
    'cpython',
    'private',
    'internal',
]

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


GROUPINGS = {
    'kind': KINDS,
    'level': LEVELS,
}


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

    if not all(v in GROUPINGS for v in groupby):
        raise ValueError(f'invalid groupby value {raw!r}')
    return groupby


def _resolve_full_groupby(groupby):
    if isinstance(groupby, str):
        groupby = [groupby]
    groupings = []
    for grouping in groupby + list(GROUPINGS):
        if grouping not in groupings:
            groupings.append(grouping)
    return groupings


def summarize(items, *, groupby='kind', includeempty=True, minimize=None):
    if minimize is None:
        if includeempty is None:
            minimize = True
            includeempty = False
        else:
            minimize = includeempty
    elif includeempty is None:
        includeempty = minimize
    elif minimize and includeempty:
        raise ValueError(f'cannot minimize and includeempty at the same time')

    groupby = _parse_groupby(groupby)[0]
    _outer, _inner = _resolve_full_groupby(groupby)
    outers = GROUPINGS[_outer]
    inners = GROUPINGS[_inner]

    summary = {
        'totals': {
            'all': 0,
            'subs': {o: 0 for o in outers},
            'bygroup': {o: {i: 0 for i in inners}
                        for o in outers},
        },
    }

    for item in items:
        outer = getattr(item, _outer)
        inner = getattr(item, _inner)
        # Update totals.
        summary['totals']['all'] += 1
        summary['totals']['subs'][outer] += 1
        summary['totals']['bygroup'][outer][inner] += 1

    if not includeempty:
        subtotals = summary['totals']['subs']
        bygroup = summary['totals']['bygroup']
        for outer in outers:
            if subtotals[outer] == 0:
                del subtotals[outer]
                del bygroup[outer]
                continue

            for inner in inners:
                if bygroup[outer][inner] == 0:
                    del bygroup[outer][inner]
            if minimize:
                if len(bygroup[outer]) == 1:
                    del bygroup[outer]

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


def resolve_filter(ignored):
    if not ignored:
        return None
    ignored = set(_resolve_ignored(ignored))
    def filter(item, *, log=None):
        if item.name not in ignored:
            return True
        if log is not None:
            log(f'ignored {item.name!r}')
        return False
    return filter


def _resolve_ignored(ignored):
    if isinstance(ignored, str):
        ignored = [ignored]
    for raw in ignored:
        if isinstance(raw, str):
            if raw.startswith('|'):
                yield raw[1:]
            elif raw.startswith('<') and raw.endswith('>'):
                filename = raw[1:-1]
                try:
                    infile = open(filename)
                except Exception as exc:
                    logger.error(f'ignore file failed: {exc}')
                    continue
                logger.log(1, f'reading ignored names from {filename!r}')
                with infile:
                    for line in infile:
                        if not line:
                            continue
                        if line[0].isspace():
                            continue
                        line = line.partition('#')[0].rstrip()
                        if line:
                            # XXX Recurse?
                            yield line
            else:
                raw = raw.strip()
                if raw:
                    yield raw
        else:
            raise NotImplementedError


def _collate(items, groupby, includeempty):
    groupby = _parse_groupby(groupby)[0]
    maxfilename = maxname = maxkind = maxlevel = 0

    collated = {}
    groups = GROUPINGS[groupby]
    for group in groups:
        collated[group] = []

    for item in items:
        key = getattr(item, groupby)
        collated[key].append(item)
        maxfilename = max(len(item.relfile), maxfilename)
        maxname = max(len(item.name), maxname)
        maxkind = max(len(item.kind), maxkind)
        maxlevel = max(len(item.level), maxlevel)
    if not includeempty:
        for group in groups:
            if not collated[group]:
                del collated[group]
    maxextra = {
        'kind': maxkind,
        'level': maxlevel,
    }
    return collated, groupby, maxfilename, maxname, maxextra


def _get_sortkey(sort, _groupby, _columns):
    if sort is True or sort is None:
        # For now:
        def sortkey(item):
            return (
                item.level == 'private',
                LEVELS.index(item.level),
                KINDS.index(item.kind),
                os.path.dirname(item.file),
                os.path.basename(item.file),
                item.name,
            )
        return sortkey

        sortfields = 'not-private level kind dirname basename name'.split()
    elif isinstance(sort, str):
        sortfields = sort.replace(',', ' ').strip().split()
    elif callable(sort):
        return sort
    else:
        raise NotImplementedError

    # XXX Build a sortkey func from sortfields.
    raise NotImplementedError


##################################
# CLI rendering

_MARKERS = {
    'level': {
        'S': 'stable',
        'C': 'cpython',
        'P': 'private',
        'I': 'internal',
    },
    'kind': {
        'F': 'func',
        'D': 'data',
        'I': 'inline',
        'M': 'macro',
        'C': 'constant',
    },
}


def resolve_format(format):
    if not format:
        return 'table'
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


def render_table(items, *,
                 columns=None,
                 groupby='kind',
                 sort=True,
                 showempty=False,
                 verbose=False,
                 ):
    if groupby is None:
        groupby = 'kind'
    if showempty is None:
        showempty = False

    if groupby:
        (collated, groupby, maxfilename, maxname, maxextra,
         ) = _collate(items, groupby, showempty)
        for grouping in GROUPINGS:
            maxextra[grouping] = max(len(g) for g in GROUPINGS[grouping])

        _, extra = _resolve_full_groupby(groupby)
        extras = [extra]
        markers = {extra: _MARKERS[extra]}

        groups = GROUPINGS[groupby]
    else:
        # XXX Support no grouping?
        raise NotImplementedError

    if columns:
        def get_extra(item):
            return {extra: getattr(item, extra)
                    for extra in ('kind', 'level')}
    else:
        if verbose:
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

    if sort:
        sortkey = _get_sortkey(sort, groupby, columns)

    total = 0
    for group, grouped in collated.items():
        if not showempty and group not in collated:
            continue
        yield ''
        yield f' === {group} ==='
        yield ''
        yield header
        yield div
        if grouped:
            if sort:
                grouped = sorted(grouped, key=sortkey)
            for item in grouped:
                yield fmt.format(
                    filename=item.relfile,
                    name=item.name,
                    **get_extra(item),
                )
        yield div
        subtotal = len(grouped)
        yield f'  sub-total: {subtotal}'
        total += subtotal
    yield ''
    yield f'total: {total}'


def render_full(items, *,
                groupby='kind',
                sort=None,
                showempty=None,
                verbose=False,
                ):
    if groupby is None:
        groupby = 'kind'
    if showempty is None:
        showempty = False

    if sort:
        sortkey = _get_sortkey(sort, groupby, None)

    if groupby:
        collated, groupby, _, _, _ = _collate(items, groupby, showempty)
        for group, grouped in collated.items():
            yield '#' * 25
            yield f'# {group} ({len(grouped)})'
            yield '#' * 25
            yield ''
            if not grouped:
                continue
            if sort:
                grouped = sorted(grouped, key=sortkey)
            for item in grouped:
                yield from _render_item_full(item, groupby, verbose)
                yield ''
    else:
        if sort:
            items = sorted(items, key=sortkey)
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


def render_summary(items, *,
                   groupby='kind',
                   sort=None,
                   showempty=None,
                   verbose=False,
                   ):
    if groupby is None:
        groupby = 'kind'
    summary = summarize(
        items,
        groupby=groupby,
        includeempty=showempty,
        minimize=None if showempty else not verbose,
    )

    subtotals = summary['totals']['subs']
    bygroup = summary['totals']['bygroup']
    lastempty = False
    for outer, subtotal in subtotals.items():
        if bygroup:
            subtotal = f'({subtotal})'
            yield f'{outer + ":":20} {subtotal:>8}'
        else:
            yield f'{outer + ":":10} {subtotal:>8}'
        if outer in bygroup:
            for inner, count in bygroup[outer].items():
                yield f'   {inner + ":":9} {count}'
            lastempty = False
        else:
            lastempty = True

    total = f'*{summary["totals"]["all"]}*'
    label = '*total*:'
    if bygroup:
        yield f'{label:20} {total:>8}'
    else:
        yield f'{label:10} {total:>9}'


_FORMATS = {
    'table': render_table,
    'full': render_full,
    'summary': render_summary,
}
