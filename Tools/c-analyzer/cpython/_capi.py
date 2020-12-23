from collections import namedtuple
import os.path
import re
import textwrap

from c_common.tables import build_table
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
    if prev:
        line = prev + ' ' + line
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
            return None, parsed
        name, kind = parsed
        level = _get_level(filename, name)
        return cls(filename, lno, name, kind, level), None

    @property
    def relfile(self):
        return self.file[len(REPO_ROOT) + 1:]


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


def iter_capi(filenames=None):
    for filename in iter_header_files(filenames):
        with open(filename) as infile:
            for item in _parse_capi(infile, filename):
                yield item


def _collate(items, groupby):
    groupby = _parse_groupby(groupby)[0]
    maxfilename = maxname = maxlevel = 0
    collated = {}
    for item in items:
        key = getattr(item, groupby)
        if key in collated:
            collated[key].append(item)
        else:
            collated[key] = [item]
        maxfilename = max(len(item.relfile), maxfilename)
        maxname = max(len(item.name), maxname)
        maxlevel = max(len(item.name), maxlevel)
    return collated, maxfilename, maxname, maxlevel


def render_table(items, *, groupby='kind', verbose=False):
    groupby = groupby or 'kind'
    if groupby != 'kind':
        raise NotImplementedError
    collated, maxfilename, maxname, maxlevel = _collate(items, groupby)
    maxlevel = max(len(level) for level in LEVELS)
    header, div, fmt = build_table([
        f'filename:{maxfilename}',
        f'name:{maxname}',
        *([f'level:{maxlevel}']
          if verbose
          else [
            f'S:1',
            f'C:1',
            f'P:1',
            f'I:1',
          ]
        ),
    ])
    total = 0
    for kind in KINDS:
        if kind not in collated:
            continue
        yield ''
        yield f' === {kind} ==='
        yield ''
        yield header
        yield div
        for item in collated[kind]:
            yield fmt.format(
                filename=item.relfile,
                name=item.name,
                **(dict(level=item.level)
                   if verbose
                   else dict(
                       S='S' if item.level == 'stable' else '',
                       C='C' if item.level == 'cpython' else '',
                       P='P' if item.level == 'private' else '',
                       I='I' if item.level == 'internal' else '',
                   )
                )

            )
        yield div
        subtotal = len(collated[kind])
        yield f'  sub-total: {subtotal}'
        total += subtotal
    yield ''
    yield f'total: {total}'


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


FORMATS = {
    'brief': render_table,
    'summary': render_summary,
}
