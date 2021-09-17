import io
import logging
import os
import os.path
import re
import sys

from c_common import fsutil
from c_common.logging import VERBOSITY, Printer
from c_common.scriptutil import (
    add_verbosity_cli,
    add_traceback_cli,
    add_sepval_cli,
    add_progress_cli,
    add_files_cli,
    add_commands_cli,
    process_args_by_key,
    configure_logger,
    get_prog,
    filter_filenames,
    iter_marks,
)
from c_parser.info import KIND
from c_parser.match import is_type_decl
from .match import filter_forward
from . import (
    analyze as _analyze,
    datafiles as _datafiles,
    check_all as _check_all,
)


KINDS = [
    KIND.TYPEDEF,
    KIND.STRUCT,
    KIND.UNION,
    KIND.ENUM,
    KIND.FUNCTION,
    KIND.VARIABLE,
    KIND.STATEMENT,
]

logger = logging.getLogger(__name__)


#######################################
# table helpers

TABLE_SECTIONS = {
    'types': (
        ['kind', 'name', 'data', 'file'],
        KIND.is_type_decl,
        (lambda v: (v.kind.value, v.filename or '', v.name)),
    ),
    'typedefs': 'types',
    'structs': 'types',
    'unions': 'types',
    'enums': 'types',
    'functions': (
        ['name', 'data', 'file'],
        (lambda kind: kind is KIND.FUNCTION),
        (lambda v: (v.filename or '', v.name)),
    ),
    'variables': (
        ['name', 'parent', 'data', 'file'],
        (lambda kind: kind is KIND.VARIABLE),
        (lambda v: (v.filename or '', str(v.parent) if v.parent else '', v.name)),
    ),
    'statements': (
        ['file', 'parent', 'data'],
        (lambda kind: kind is KIND.STATEMENT),
        (lambda v: (v.filename or '', str(v.parent) if v.parent else '', v.name)),
    ),
    KIND.TYPEDEF: 'typedefs',
    KIND.STRUCT: 'structs',
    KIND.UNION: 'unions',
    KIND.ENUM: 'enums',
    KIND.FUNCTION: 'functions',
    KIND.VARIABLE: 'variables',
    KIND.STATEMENT: 'statements',
}


def _render_table(items, columns, relroot=None):
    # XXX improve this
    header = '\t'.join(columns)
    div = '--------------------'
    yield header
    yield div
    total = 0
    for item in items:
        rowdata = item.render_rowdata(columns)
        row = [rowdata[c] for c in columns]
        if relroot and 'file' in columns:
            index = columns.index('file')
            row[index] = os.path.relpath(row[index], relroot)
        yield '\t'.join(row)
        total += 1
    yield div
    yield f'total: {total}'


def build_section(name, groupitems, *, relroot=None):
    info = TABLE_SECTIONS[name]
    while type(info) is not tuple:
        if name in KINDS:
            name = info
        info = TABLE_SECTIONS[info]

    columns, match_kind, sortkey = info
    items = (v for v in groupitems if match_kind(v.kind))
    items = sorted(items, key=sortkey)
    def render():
        yield ''
        yield f'{name}:'
        yield ''
        for line in _render_table(items, columns, relroot):
            yield line
    return items, render


#######################################
# the checks

CHECKS = {
    #'globals': _check_globals,
}


def add_checks_cli(parser, checks=None, *, add_flags=None):
    default = False
    if not checks:
        checks = list(CHECKS)
        default = True
    elif isinstance(checks, str):
        checks = [checks]
    if (add_flags is None and len(checks) > 1) or default:
        add_flags = True

    process_checks = add_sepval_cli(parser, '--check', 'checks', checks)
    if add_flags:
        for check in checks:
            parser.add_argument(f'--{check}', dest='checks',
                                action='append_const', const=check)
    return [
        process_checks,
    ]


def _get_check_handlers(fmt, printer, verbosity=VERBOSITY):
    div = None
    def handle_after():
        pass
    if not fmt:
        div = ''
        def handle_failure(failure, data):
            data = repr(data)
            if verbosity >= 3:
                logger.info(f'failure: {failure}')
                logger.info(f'data:    {data}')
            else:
                logger.warn(f'failure: {failure} (data: {data})')
    elif fmt == 'raw':
        def handle_failure(failure, data):
            print(f'{failure!r} {data!r}')
    elif fmt == 'brief':
        def handle_failure(failure, data):
            parent = data.parent or ''
            funcname = parent if isinstance(parent, str) else parent.name
            name = f'({funcname}).{data.name}' if funcname else data.name
            failure = failure.split('\t')[0]
            print(f'{data.filename}:{name} - {failure}')
    elif fmt == 'summary':
        def handle_failure(failure, data):
            print(_fmt_one_summary(data, failure))
    elif fmt == 'full':
        div = ''
        def handle_failure(failure, data):
            name = data.shortkey if data.kind is KIND.VARIABLE else data.name
            parent = data.parent or ''
            funcname = parent if isinstance(parent, str) else parent.name
            known = 'yes' if data.is_known else '*** NO ***'
            print(f'{data.kind.value} {name!r} failed ({failure})')
            print(f'  file:         {data.filename}')
            print(f'  func:         {funcname or "-"}')
            print(f'  name:         {data.name}')
            print(f'  data:         ...')
            print(f'  type unknown: {known}')
    else:
        if fmt in FORMATS:
            raise NotImplementedError(fmt)
        raise ValueError(f'unsupported fmt {fmt!r}')
    return handle_failure, handle_after, div


#######################################
# the formats

def fmt_raw(analysis):
    for item in analysis:
        yield from item.render('raw')


def fmt_brief(analysis):
    # XXX Support sorting.
    items = sorted(analysis)
    for kind in KINDS:
        if kind is KIND.STATEMENT:
            continue
        for item in items:
            if item.kind is not kind:
                continue
            yield from item.render('brief')
    yield f'  total: {len(items)}'


def fmt_summary(analysis):
    # XXX Support sorting and grouping.
    items = list(analysis)
    total = len(items)

    def section(name):
        _, render = build_section(name, items)
        yield from render()

    yield from section('types')
    yield from section('functions')
    yield from section('variables')
    yield from section('statements')

    yield ''
#    yield f'grand total: {len(supported) + len(unsupported)}'
    yield f'grand total: {total}'


def _fmt_one_summary(item, extra=None):
    parent = item.parent or ''
    funcname = parent if isinstance(parent, str) else parent.name
    if extra:
        return f'{item.filename:35}\t{funcname or "-":35}\t{item.name:40}\t{extra}'
    else:
        return f'{item.filename:35}\t{funcname or "-":35}\t{item.name}'


def fmt_full(analysis):
    # XXX Support sorting.
    items = sorted(analysis, key=lambda v: v.key)
    yield ''
    for item in items:
        yield from item.render('full')
        yield ''
    yield f'total: {len(items)}'


FORMATS = {
    'raw': fmt_raw,
    'brief': fmt_brief,
    'summary': fmt_summary,
    'full': fmt_full,
}


def add_output_cli(parser, *, default='summary'):
    parser.add_argument('--format', dest='fmt', default=default, choices=tuple(FORMATS))

    def process_args(args, *, argv=None):
        pass
    return process_args


#######################################
# the commands

def _cli_check(parser, checks=None, **kwargs):
    if isinstance(checks, str):
        checks = [checks]
    if checks is False:
        process_checks = None
    elif checks is None:
        process_checks = add_checks_cli(parser)
    elif len(checks) == 1 and type(checks) is not dict and re.match(r'^<.*>$', checks[0]):
        check = checks[0][1:-1]
        def process_checks(args, *, argv=None):
            args.checks = [check]
    else:
        process_checks = add_checks_cli(parser, checks=checks)
    process_progress = add_progress_cli(parser)
    process_output = add_output_cli(parser, default=None)
    process_files = add_files_cli(parser, **kwargs)
    return [
        process_checks,
        process_progress,
        process_output,
        process_files,
    ]


def cmd_check(filenames, *,
              checks=None,
              ignored=None,
              fmt=None,
              failfast=False,
              iter_filenames=None,
              relroot=fsutil.USE_CWD,
              track_progress=None,
              verbosity=VERBOSITY,
              _analyze=_analyze,
              _CHECKS=CHECKS,
              **kwargs
              ):
    if not checks:
        checks = _CHECKS
    elif isinstance(checks, str):
        checks = [checks]
    checks = [_CHECKS[c] if isinstance(c, str) else c
              for c in checks]
    printer = Printer(verbosity)
    (handle_failure, handle_after, div
     ) = _get_check_handlers(fmt, printer, verbosity)

    filenames, relroot = fsutil.fix_filenames(filenames, relroot=relroot)
    filenames = filter_filenames(filenames, iter_filenames, relroot)
    if track_progress:
        filenames = track_progress(filenames)

    logger.info('analyzing files...')
    analyzed = _analyze(filenames, **kwargs)
    analyzed.fix_filenames(relroot, normalize=False)
    decls = filter_forward(analyzed, markpublic=True)

    logger.info('checking analysis results...')
    failed = []
    for data, failure in _check_all(decls, checks, failfast=failfast):
        if data is None:
            printer.info('stopping after one failure')
            break
        if div is not None and len(failed) > 0:
            printer.info(div)
        failed.append(data)
        handle_failure(failure, data)
    handle_after()

    printer.info('-------------------------')
    logger.info(f'total failures: {len(failed)}')
    logger.info('done checking')

    if fmt == 'summary':
        print('Categorized by storage:')
        print()
        from .match import group_by_storage
        grouped = group_by_storage(failed, ignore_non_match=False)
        for group, decls in grouped.items():
            print()
            print(group)
            for decl in decls:
                print(' ', _fmt_one_summary(decl))
            print(f'subtotal: {len(decls)}')

    if len(failed) > 0:
        sys.exit(len(failed))


def _cli_analyze(parser, **kwargs):
    process_progress = add_progress_cli(parser)
    process_output = add_output_cli(parser)
    process_files = add_files_cli(parser, **kwargs)
    return [
        process_progress,
        process_output,
        process_files,
    ]


# XXX Support filtering by kind.
def cmd_analyze(filenames, *,
                fmt=None,
                iter_filenames=None,
                relroot=fsutil.USE_CWD,
                track_progress=None,
                verbosity=None,
                _analyze=_analyze,
                formats=FORMATS,
                **kwargs
                ):
    verbosity = verbosity if verbosity is not None else 3

    try:
        do_fmt = formats[fmt]
    except KeyError:
        raise ValueError(f'unsupported fmt {fmt!r}')

    filenames, relroot = fsutil.fix_filenames(filenames, relroot=relroot)
    filenames = filter_filenames(filenames, iter_filenames, relroot)
    if track_progress:
        filenames = track_progress(filenames)

    logger.info('analyzing files...')
    analyzed = _analyze(filenames, **kwargs)
    analyzed.fix_filenames(relroot, normalize=False)
    decls = filter_forward(analyzed, markpublic=True)

    for line in do_fmt(decls):
        print(line)


def _cli_data(parser, filenames=None, known=None):
    ArgumentParser = type(parser)
    common = ArgumentParser(add_help=False)
    # These flags will get processed by the top-level parse_args().
    add_verbosity_cli(common)
    add_traceback_cli(common)

    subs = parser.add_subparsers(dest='datacmd')

    sub = subs.add_parser('show', parents=[common])
    if known is None:
        sub.add_argument('--known', required=True)
    if filenames is None:
        sub.add_argument('filenames', metavar='FILE', nargs='+')

    sub = subs.add_parser('dump', parents=[common])
    if known is None:
        sub.add_argument('--known')
    sub.add_argument('--show', action='store_true')
    process_progress = add_progress_cli(sub)

    sub = subs.add_parser('check', parents=[common])
    if known is None:
        sub.add_argument('--known', required=True)

    def process_args(args, *, argv):
        if args.datacmd == 'dump':
            process_progress(args, argv)
    return process_args


def cmd_data(datacmd, filenames, known=None, *,
             _analyze=_analyze,
             formats=FORMATS,
             extracolumns=None,
             relroot=fsutil.USE_CWD,
             track_progress=None,
             **kwargs
             ):
    kwargs.pop('verbosity', None)
    usestdout = kwargs.pop('show', None)
    if datacmd == 'show':
        do_fmt = formats['summary']
        if isinstance(known, str):
            known, _ = _datafiles.get_known(known, extracolumns, relroot)
        for line in do_fmt(known):
            print(line)
    elif datacmd == 'dump':
        filenames, relroot = fsutil.fix_filenames(filenames, relroot=relroot)
        if track_progress:
            filenames = track_progress(filenames)
        analyzed = _analyze(filenames, **kwargs)
        analyzed.fix_filenames(relroot, normalize=False)
        if known is None or usestdout:
            outfile = io.StringIO()
            _datafiles.write_known(analyzed, outfile, extracolumns,
                                   relroot=relroot)
            print(outfile.getvalue())
        else:
            _datafiles.write_known(analyzed, known, extracolumns,
                                   relroot=relroot)
    elif datacmd == 'check':
        raise NotImplementedError(datacmd)
    else:
        raise ValueError(f'unsupported data command {datacmd!r}')


COMMANDS = {
    'check': (
        'analyze and fail if the given C source/header files have any problems',
        [_cli_check],
        cmd_check,
    ),
    'analyze': (
        'report on the state of the given C source/header files',
        [_cli_analyze],
        cmd_analyze,
    ),
    'data': (
        'check/manage local data (e.g. knwon types, ignored vars, caches)',
        [_cli_data],
        cmd_data,
    ),
}


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0], *, subset=None):
    import argparse
    parser = argparse.ArgumentParser(
        prog=prog or get_prog(),
    )

    processors = add_commands_cli(
        parser,
        commands={k: v[1] for k, v in COMMANDS.items()},
        commonspecs=[
            add_verbosity_cli,
            add_traceback_cli,
        ],
        subset=subset,
    )

    args = parser.parse_args(argv)
    ns = vars(args)

    cmd = ns.pop('cmd')

    verbosity, traceback_cm = process_args_by_key(
        args,
        argv,
        processors[cmd],
        ['verbosity', 'traceback_cm'],
    )
    # "verbosity" is sent to the commands, so we put it back.
    args.verbosity = verbosity

    return cmd, ns, verbosity, traceback_cm


def main(cmd, cmd_kwargs):
    try:
        run_cmd = COMMANDS[cmd][0]
    except KeyError:
        raise ValueError(f'unsupported cmd {cmd!r}')
    run_cmd(**cmd_kwargs)


if __name__ == '__main__':
    cmd, cmd_kwargs, verbosity, traceback_cm = parse_args()
    configure_logger(verbosity)
    with traceback_cm:
        main(cmd, cmd_kwargs)
