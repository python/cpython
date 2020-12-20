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
        (\w+)  # <func_name>
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
        (\w+)  # <data_name>
        \b [^(]
    )
''')
CAPI_MACRO = textwrap.dedent(r'''
    (?:
        (\w+)  # <macro_name>
        \s* [(]
    )
''')
CAPI_CONSTANT = textwrap.dedent(r'''
    (?:
        (\w+)  # <constant_name>
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
        {_ind(CAPI_DEFINE, 2)}
    )
'''), re.VERBOSE)

KINDS = [
    'func',
    'data',
    'macro',
    'constant',
]


def _parse_line(line):
    m = CAPI_RE.match(line)
    if not m:
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
    def from_line(cls, line, filename, lno):
        parsed = _parse_line(line)
        if not parsed:
            return None
        name, kind = parsed
        level = _get_level(filename, name)
        return cls(filename, lno, name, kind, level)

    @property
    def relfile(self):
        return self.file[len(REPO_ROOT) + 1:]


def _parse_capi(lines, filename):
    if isinstance(lines, str):
        lines = lines.splitlines()
    for lno, line in enumerate(lines, 1):
        yield CAPIItem.from_line(line, filename, lno)


def iter_capi(filenames=None):
    for filename in iter_header_files(filenames):
        with open(filename) as infile:
            for item in _parse_capi(infile, filename):
                if item is not None:
                    yield item


def _collate_by_kind(items):
    maxfilename = maxname = maxlevel = 0
    collated = {}
    for item in items:
        if item.kind in collated:
            collated[item.kind].append(item)
        else:
            collated[item.kind] = [item]
        maxfilename = max(len(item.relfile), maxfilename)
        maxname = max(len(item.name), maxname)
        maxlevel = max(len(item.name), maxlevel)
    return collated, maxfilename, maxname, maxlevel


def render_table(items, *, verbose=False):
    collated, maxfilename, maxname, maxlevel = _collate_by_kind(items)
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


def render_summary(items, *, bykind=True):
    total = 0
    summary = summarize(items, bykind=bykind)
    for outer, counts in summary.items():
        subtotal = sum(c for _, c in counts.items())
        yield f'{outer + ":":20} ({subtotal})'
        for inner, count in counts.items():
            yield f'   {inner + ":":9} {count}'
        total += subtotal
    yield f'{"total:":20} ({total})'


def summarize(items, *, bykind=True):
    if bykind:
        summary = {kind: {l: 0 for l in LEVELS}
                   for kind in KINDS}
        for item in items:
            summary[item.kind][item.level] += 1
    else:
        summary = {level: {k: 0 for k in KINDS}
                   for level in LEVELS}
        for item in items:
            summary[item.level][item.kind] += 1
    return summary
