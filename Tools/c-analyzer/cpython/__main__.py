import logging
import sys

from c_common.fsutil import expand_filenames, iter_files_by_suffix
from c_common.scriptutil import (
    add_verbosity_cli,
    add_traceback_cli,
    add_commands_cli,
    add_kind_filtering_cli,
    add_files_cli,
    process_args_by_key,
    configure_logger,
    get_prog,
)
from c_parser.info import KIND
import c_parser.__main__ as c_parser
import c_analyzer.__main__ as c_analyzer
import c_analyzer as _c_analyzer
from c_analyzer.info import UNKNOWN
from . import _analyzer, _parser, REPO_ROOT


logger = logging.getLogger(__name__)


def _resolve_filenames(filenames):
    if filenames:
        resolved = (_parser.resolve_filename(f) for f in filenames)
    else:
        resolved = _parser.iter_filenames()
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
    c_parser.cmd_parse(filenames, **kwargs)


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
