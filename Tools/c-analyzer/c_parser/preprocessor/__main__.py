import logging
import sys

from c_common.scriptutil import (
    CLIArgSpec as Arg,
    add_verbosity_cli,
    add_traceback_cli,
    add_kind_filtering_cli,
    add_files_cli,
    add_failure_filtering_cli,
    add_commands_cli,
    process_args_by_key,
    configure_logger,
    get_prog,
    main_for_filenames,
)
from . import (
    errors as _errors,
    get_preprocessor as _get_preprocessor,
)


FAIL = {
    'err': _errors.ErrorDirectiveError,
    'deps': _errors.MissingDependenciesError,
    'os': _errors.OSMismatchError,
}
FAIL_DEFAULT = tuple(v for v in FAIL if v != 'os')


logger = logging.getLogger(__name__)


##################################
# CLI helpers

def add_common_cli(parser, *, get_preprocessor=_get_preprocessor):
    parser.add_argument('--macros', action='append')
    parser.add_argument('--incldirs', action='append')
    parser.add_argument('--same', action='append')
    process_fail_arg = add_failure_filtering_cli(parser, FAIL)

    def process_args(args, *, argv):
        ns = vars(args)

        process_fail_arg(args, argv=argv)
        ignore_exc = ns.pop('ignore_exc')
        # We later pass ignore_exc to _get_preprocessor().

        args.get_file_preprocessor = get_preprocessor(
            file_macros=ns.pop('macros'),
            file_incldirs=ns.pop('incldirs'),
            file_same=ns.pop('same'),
            ignore_exc=ignore_exc,
            log_err=print,
        )
    return process_args


def _iter_preprocessed(filename, *,
                       get_preprocessor,
                       match_kind=None,
                       pure=False,
                       ):
    preprocess = get_preprocessor(filename)
    for line in preprocess(tool=not pure) or ():
        if match_kind is not None and not match_kind(line.kind):
            continue
        yield line


#######################################
# the commands

def _cli_preprocess(parser, excluded=None, **prepr_kwargs):
    parser.add_argument('--pure', action='store_true')
    parser.add_argument('--no-pure', dest='pure', action='store_const', const=False)
    process_kinds = add_kind_filtering_cli(parser)
    process_common = add_common_cli(parser, **prepr_kwargs)
    parser.add_argument('--raw', action='store_true')
    process_files = add_files_cli(parser, excluded=excluded)

    return [
        process_kinds,
        process_common,
        process_files,
    ]


def cmd_preprocess(filenames, *,
                   raw=False,
                   iter_filenames=None,
                   **kwargs
                   ):
    if 'get_file_preprocessor' not in kwargs:
        kwargs['get_file_preprocessor'] = _get_preprocessor()
    if raw:
        def show_file(filename, lines):
            for line in lines:
                print(line)
                #print(line.raw)
    else:
        def show_file(filename, lines):
            for line in lines:
                linefile = ''
                if line.filename != filename:
                    linefile = f' ({line.filename})'
                text = line.data
                if line.kind == 'comment':
                    text = '/* ' + line.data.splitlines()[0]
                    text += ' */' if '\n' in line.data else r'\n... */'
                print(f' {line.lno:>4} {line.kind:10} | {text}')

    filenames = main_for_filenames(filenames, iter_filenames)
    for filename in filenames:
        lines = _iter_preprocessed(filename, **kwargs)
        show_file(filename, lines)


def _cli_data(parser):
    ...

    return None


def cmd_data(filenames,
             **kwargs
             ):
    # XXX
    raise NotImplementedError


COMMANDS = {
    'preprocess': (
        'preprocess the given C source & header files',
        [_cli_preprocess],
        cmd_preprocess,
    ),
    'data': (
        'check/manage local data (e.g. excludes, macros)',
        [_cli_data],
        cmd_data,
    ),
}


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0], *,
               subset='preprocess',
               excluded=None,
               **prepr_kwargs
               ):
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
