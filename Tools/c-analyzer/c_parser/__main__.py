import logging
import os.path
import sys

from c_common import fsutil
from c_common.scriptutil import (
    CLIArgSpec as Arg,
    add_verbosity_cli,
    add_traceback_cli,
    add_kind_filtering_cli,
    add_files_cli,
    add_commands_cli,
    process_args_by_key,
    configure_logger,
    get_prog,
    main_for_filenames,
)
from .preprocessor import get_preprocessor
from .preprocessor.__main__ import (
    add_common_cli as add_preprocessor_cli,
)
from .info import KIND
from . import parse_file as _iter_parsed


logger = logging.getLogger(__name__)


def _format_vartype(vartype):
    if isinstance(vartype, str):
        return vartype

    data = vartype
    try:
        vartype = data['vartype']
    except KeyError:
        storage, typequal, typespec, abstract = vartype.values()
    else:
        storage = data.get('storage')
        if storage:
            _, typequal, typespec, abstract = vartype.values()
        else:
            storage, typequal, typespec, abstract = vartype.values()

    vartype = f'{typespec} {abstract}'
    if typequal:
        vartype = f'{typequal} {vartype}'
    if storage:
        vartype = f'{storage} {vartype}'
    return vartype


def _get_preprocessor(filename, **kwargs):
    return get_processor(filename,
                         log_err=print,
                         **kwargs
                         )


#######################################
# the formats

def fmt_raw(filename, item, *, showfwd=None):
    yield str(tuple(item))


def fmt_summary(filename, item, *, showfwd=None):
    if item.filename != filename:
        yield f'> {item.filename}'

    if showfwd is None:
        LINE = ' {lno:>5} {kind:10} {funcname:40} {fwd:1} {name:40} {data}'
    else:
        LINE = ' {lno:>5} {kind:10} {funcname:40} {name:40} {data}'
    lno = kind = funcname = fwd = name = data = ''
    MIN_LINE = len(LINE.format(**locals()))

    fileinfo, kind, funcname, name, data = item
    lno = fileinfo.lno if fileinfo and fileinfo.lno >= 0 else ''
    funcname = funcname or ' --'
    name = name or ' --'
    isforward = False
    if kind is KIND.FUNCTION:
        storage, inline, params, returntype, isforward = data.values()
        returntype = _format_vartype(returntype)
        data = returntype + params
        if inline:
            data = f'inline {data}'
        if storage:
            data = f'{storage} {data}'
    elif kind is KIND.VARIABLE:
        data = _format_vartype(data)
    elif kind is KIND.STRUCT or kind is KIND.UNION:
        if data is None:
            isforward = True
        else:
            fields = data
            data = f'({len(data)}) {{ '
            indent = ',\n' + ' ' * (MIN_LINE + len(data))
            data += ', '.join(f.name for f in fields[:5])
            fields = fields[5:]
            while fields:
                data = f'{data}{indent}{", ".join(f.name for f in fields[:5])}'
                fields = fields[5:]
            data += ' }'
    elif kind is KIND.ENUM:
        if data is None:
            isforward = True
        else:
            names = [d if isinstance(d, str) else d.name
                     for d in data]
            data = f'({len(data)}) {{ '
            indent = ',\n' + ' ' * (MIN_LINE + len(data))
            data += ', '.join(names[:5])
            names = names[5:]
            while names:
                data = f'{data}{indent}{", ".join(names[:5])}'
                names = names[5:]
            data += ' }'
    elif kind is KIND.TYPEDEF:
        data = f'typedef {data}'
    elif kind == KIND.STATEMENT:
        pass
    else:
        raise NotImplementedError(item)
    if isforward:
        fwd = '*'
        if not showfwd and showfwd is not None:
            return
    elif showfwd:
        return
    kind = kind.value
    yield LINE.format(**locals())


def fmt_full(filename, item, *, showfwd=None):
    raise NotImplementedError


FORMATS = {
    'raw': fmt_raw,
    'summary': fmt_summary,
    'full': fmt_full,
}


def add_output_cli(parser):
    parser.add_argument('--format', dest='fmt', default='summary', choices=tuple(FORMATS))
    parser.add_argument('--showfwd', action='store_true', default=None)
    parser.add_argument('--no-showfwd', dest='showfwd', action='store_false', default=None)

    def process_args(args):
        pass
    return process_args


#######################################
# the commands

def _cli_parse(parser, excluded=None, **prepr_kwargs):
    process_output = add_output_cli(parser)
    process_kinds = add_kind_filtering_cli(parser)
    process_preprocessor = add_preprocessor_cli(parser, **prepr_kwargs)
    process_files = add_files_cli(parser, excluded=excluded)
    return [
        process_output,
        process_kinds,
        process_preprocessor,
        process_files,
    ]


def cmd_parse(filenames, *,
              fmt='summary',
              showfwd=None,
              iter_filenames=None,
              relroot=None,
              **kwargs
              ):
    if 'get_file_preprocessor' not in kwargs:
        kwargs['get_file_preprocessor'] = _get_preprocessor()
    try:
        do_fmt = FORMATS[fmt]
    except KeyError:
        raise ValueError(f'unsupported fmt {fmt!r}')
    for filename, relfile in main_for_filenames(filenames, iter_filenames, relroot):
        for item in _iter_parsed(filename, **kwargs):
            item = item.fix_filename(relroot, fixroot=False, normalize=False)
            for line in do_fmt(relfile, item, showfwd=showfwd):
                print(line)


def _cli_data(parser):
    ...

    return []


def cmd_data(filenames,
             **kwargs
             ):
    # XXX
    raise NotImplementedError


COMMANDS = {
    'parse': (
        'parse the given C source & header files',
        [_cli_parse],
        cmd_parse,
    ),
    'data': (
        'check/manage local data (e.g. excludes, macros)',
        [_cli_data],
        cmd_data,
    ),
}


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0], *, subset='parse'):
    import argparse
    parser = argparse.ArgumentParser(
        prog=prog or get_prog,
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
