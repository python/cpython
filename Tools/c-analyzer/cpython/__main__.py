import logging
import sys

from c_common.fsutil import expand_filenames, iter_files_by_suffix
from c_common.scriptutil import (
    VERBOSITY,
    add_verbosity_cli,
    add_traceback_cli,
    add_commands_cli,
    add_kind_filtering_cli,
    add_files_cli,
    add_progress_cli,
    main_for_filenames,
    process_args_by_key,
    configure_logger,
    get_prog,
)
from c_parser.info import KIND
import c_parser.__main__ as c_parser
import c_analyzer.__main__ as c_analyzer
import c_analyzer as _c_analyzer
from c_analyzer.info import UNKNOWN
from . import _analyzer, _capi, _files, _parser, REPO_ROOT


logger = logging.getLogger(__name__)


def _resolve_filenames(filenames):
    if filenames:
        resolved = (_files.resolve_filename(f) for f in filenames)
    else:
        resolved = _files.iter_filenames()
    return resolved


#######################################
# the formats

def fmt_summary(analysis):
    # XXX Support sorting and grouping.
    supported = []
    unsupported = []
    for item in analysis:
        if item.supported:
            supported.append(item)
        else:
            unsupported.append(item)
    total = 0

    def section(name, groupitems):
        nonlocal total
        items, render = c_analyzer.build_section(name, groupitems,
                                                 relroot=REPO_ROOT)
        yield from render()
        total += len(items)

    yield ''
    yield '===================='
    yield 'supported'
    yield '===================='

    yield from section('types', supported)
    yield from section('variables', supported)

    yield ''
    yield '===================='
    yield 'unsupported'
    yield '===================='

    yield from section('types', unsupported)
    yield from section('variables', unsupported)

    yield ''
    yield f'grand total: {total}'


#######################################
# the checks

CHECKS = dict(c_analyzer.CHECKS, **{
    'globals': _analyzer.check_globals,
})

#######################################
# the commands

FILES_KWARGS = dict(excluded=_parser.EXCLUDED, nargs='*')


def _cli_parse(parser):
    process_output = c_parser.add_output_cli(parser)
    process_kind = add_kind_filtering_cli(parser)
    process_preprocessor = c_parser.add_preprocessor_cli(
        parser,
        get_preprocessor=_parser.get_preprocessor,
    )
    process_files = add_files_cli(parser, **FILES_KWARGS)
    return [
        process_output,
        process_kind,
        process_preprocessor,
        process_files,
    ]


def cmd_parse(filenames=None, **kwargs):
    filenames = _resolve_filenames(filenames)
    if 'get_file_preprocessor' not in kwargs:
        kwargs['get_file_preprocessor'] = _parser.get_preprocessor()
    c_parser.cmd_parse(
        filenames,
        relroot=REPO_ROOT,
        **kwargs
    )


def _cli_check(parser, **kwargs):
    return c_analyzer._cli_check(parser, CHECKS, **kwargs, **FILES_KWARGS)


def cmd_check(filenames=None, **kwargs):
    filenames = _resolve_filenames(filenames)
    kwargs['get_file_preprocessor'] = _parser.get_preprocessor(log_err=print)
    c_analyzer.cmd_check(
        filenames,
        relroot=REPO_ROOT,
        _analyze=_analyzer.analyze,
        _CHECKS=CHECKS,
        **kwargs
    )


def cmd_analyze(filenames=None, **kwargs):
    formats = dict(c_analyzer.FORMATS)
    formats['summary'] = fmt_summary
    filenames = _resolve_filenames(filenames)
    kwargs['get_file_preprocessor'] = _parser.get_preprocessor(log_err=print)
    c_analyzer.cmd_analyze(
        filenames,
        relroot=REPO_ROOT,
        _analyze=_analyzer.analyze,
        formats=formats,
        **kwargs
    )


def _cli_data(parser):
    filenames = False
    known = True
    return c_analyzer._cli_data(parser, filenames, known)


def cmd_data(datacmd, **kwargs):
    formats = dict(c_analyzer.FORMATS)
    formats['summary'] = fmt_summary
    filenames = (file
                 for file in _resolve_filenames(None)
                 if file not in _parser.EXCLUDED)
    kwargs['get_file_preprocessor'] = _parser.get_preprocessor(log_err=print)
    if datacmd == 'show':
        types = _analyzer.read_known()
        results = []
        for decl, info in types.items():
            if info is UNKNOWN:
                if decl.kind in (KIND.STRUCT, KIND.UNION):
                    extra = {'unsupported': ['type unknown'] * len(decl.members)}
                else:
                    extra = {'unsupported': ['type unknown']}
                info = (info, extra)
            results.append((decl, info))
            if decl.shortkey == 'struct _object':
                tempinfo = info
        known = _analyzer.Analysis.from_results(results)
        analyze = None
    elif datacmd == 'dump':
        known = _analyzer.KNOWN_FILE
        def analyze(files, **kwargs):
            decls = []
            for decl in _analyzer.iter_decls(files, **kwargs):
                if not KIND.is_type_decl(decl.kind):
                    continue
                if not decl.filename.endswith('.h'):
                    if decl.shortkey not in _analyzer.KNOWN_IN_DOT_C:
                        continue
                decls.append(decl)
            results = _c_analyzer.analyze_decls(
                decls,
                known={},
                analyze_resolved=_analyzer.analyze_resolved,
            )
            return _analyzer.Analysis.from_results(results)
    else:  # check
        known = _analyzer.read_known()
        def analyze(files, **kwargs):
            return _analyzer.iter_decls(files, **kwargs)
    extracolumns = None
    c_analyzer.cmd_data(
        datacmd,
        filenames,
        known,
        _analyze=analyze,
        formats=formats,
        extracolumns=extracolumns,
        relroot=REPO_ROOT,
        **kwargs
    )


def _cli_capi(parser):
    parser.add_argument('--levels', action='append', metavar='LEVEL[,...]')
    parser.add_argument(f'--public', dest='levels',
                        action='append_const', const='public')
    parser.add_argument(f'--no-public', dest='levels',
                        action='append_const', const='no-public')
    for level in _capi.LEVELS:
        parser.add_argument(f'--{level}', dest='levels',
                            action='append_const', const=level)
    def process_levels(args, *, argv=None):
        levels = []
        for raw in args.levels or ():
            for level in raw.replace(',', ' ').strip().split():
                if level == 'public':
                    levels.append('stable')
                    levels.append('cpython')
                elif level == 'no-public':
                    levels.append('private')
                    levels.append('internal')
                elif level in _capi.LEVELS:
                    levels.append(level)
                else:
                    parser.error(f'expected LEVEL to be one of {sorted(_capi.LEVELS)}, got {level!r}')
        args.levels = set(levels)

    parser.add_argument('--kinds', action='append', metavar='KIND[,...]')
    for kind in _capi.KINDS:
        parser.add_argument(f'--{kind}', dest='kinds',
                            action='append_const', const=kind)
    def process_kinds(args, *, argv=None):
        kinds = []
        for raw in args.kinds or ():
            for kind in raw.replace(',', ' ').strip().split():
                if kind in _capi.KINDS:
                    kinds.append(kind)
                else:
                    parser.error(f'expected KIND to be one of {sorted(_capi.KINDS)}, got {kind!r}')
        args.kinds = set(kinds)

    parser.add_argument('--group-by', dest='groupby',
                        choices=['level', 'kind'])

    parser.add_argument('--format', default='table')
    parser.add_argument('--summary', dest='format',
                        action='store_const', const='summary')
    def process_format(args, *, argv=None):
        orig = args.format
        args.format = _capi.resolve_format(args.format)
        if isinstance(args.format, str):
            if args.format not in _capi._FORMATS:
                parser.error(f'unsupported format {orig!r}')

    parser.add_argument('--show-empty', dest='showempty', action='store_true')
    parser.add_argument('--no-show-empty', dest='showempty', action='store_false')
    parser.set_defaults(showempty=None)

    # XXX Add --sort-by, --sort and --no-sort.

    parser.add_argument('--ignore', dest='ignored', action='append')
    def process_ignored(args, *, argv=None):
        ignored = []
        for raw in args.ignored or ():
            ignored.extend(raw.replace(',', ' ').strip().split())
        args.ignored = ignored or None

    parser.add_argument('filenames', nargs='*', metavar='FILENAME')
    process_progress = add_progress_cli(parser)

    return [
        process_levels,
        process_kinds,
        process_format,
        process_ignored,
        process_progress,
    ]


def cmd_capi(filenames=None, *,
             levels=None,
             kinds=None,
             groupby='kind',
             format='table',
             showempty=None,
             ignored=None,
             track_progress=None,
             verbosity=VERBOSITY,
             **kwargs
             ):
    render = _capi.get_renderer(format)

    filenames = _files.iter_header_files(filenames, levels=levels)
    #filenames = (file for file, _ in main_for_filenames(filenames))
    if track_progress:
        filenames = track_progress(filenames)
    items = _capi.iter_capi(filenames)
    if levels:
        items = (item for item in items if item.level in levels)
    if kinds:
        items = (item for item in items if item.kind in kinds)

    filter = _capi.resolve_filter(ignored)
    if filter:
        items = (item for item in items if filter(item, log=lambda msg: logger.log(1, msg)))

    lines = render(
        items,
        groupby=groupby,
        showempty=showempty,
        verbose=verbosity > VERBOSITY,
    )
    print()
    for line in lines:
        print(line)


# We do not define any other cmd_*() handlers here,
# favoring those defined elsewhere.

COMMANDS = {
    'check': (
        'analyze and fail if the CPython source code has any problems',
        [_cli_check],
        cmd_check,
    ),
    'analyze': (
        'report on the state of the CPython source code',
        [(lambda p: c_analyzer._cli_analyze(p, **FILES_KWARGS))],
        cmd_analyze,
    ),
    'parse': (
        'parse the CPython source files',
        [_cli_parse],
        cmd_parse,
    ),
    'data': (
        'check/manage local data (e.g. knwon types, ignored vars, caches)',
        [_cli_data],
        cmd_data,
    ),
    'capi': (
        'inspect the C-API',
        [_cli_capi],
        cmd_capi,
    ),
}


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=None, *, subset=None):
    import argparse
    parser = argparse.ArgumentParser(
        prog=prog or get_prog(),
    )

#    if subset == 'check' or subset == ['check']:
#        if checks is not None:
#            commands = dict(COMMANDS)
#            commands['check'] = list(commands['check'])
#            cli = commands['check'][1][0]
#            commands['check'][1][0] = (lambda p: cli(p, checks=checks))
    processors = add_commands_cli(
        parser,
        commands=COMMANDS,
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
    if cmd != 'parse':
        # "verbosity" is sent to the commands, so we put it back.
        args.verbosity = verbosity

    return cmd, ns, verbosity, traceback_cm


def main(cmd, cmd_kwargs):
    try:
        run_cmd = COMMANDS[cmd][-1]
    except KeyError:
        raise ValueError(f'unsupported cmd {cmd!r}')
    run_cmd(**cmd_kwargs)


if __name__ == '__main__':
    cmd, cmd_kwargs, verbosity, traceback_cm = parse_args()
    configure_logger(verbosity)
    with traceback_cm:
        main(cmd, cmd_kwargs)
