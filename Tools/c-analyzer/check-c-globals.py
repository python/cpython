import sys

from cpython.__main__ import main, configure_logger


def parse_args(argv=sys.argv[1:]):
    import argparse
    from c_common.scriptutil import (
        add_verbosity_cli,
        add_traceback_cli,
        process_args_by_key,
    )
    from cpython.__main__ import _cli_check
    parser = argparse.ArgumentParser()
    processors = [
        add_verbosity_cli(parser),
        add_traceback_cli(parser),
        #_cli_check(parser, checks='<globals>'),
        _cli_check(parser),
    ]

    args = parser.parse_args()
    ns = vars(args)

    cmd = 'check'
    verbosity, traceback_cm = process_args_by_key(
        args,
        argv,
        processors,
        ['verbosity', 'traceback_cm'],
    )

    return cmd, ns, verbosity, traceback_cm


(cmd, cmd_kwargs, verbosity, traceback_cm) = parse_args()
configure_logger(verbosity)
with traceback_cm:
    main(cmd, cmd_kwargs)
