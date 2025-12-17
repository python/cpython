from cpython.__main__ import parse_args, main, configure_logger


cmd, cmd_kwargs, verbosity, traceback_cm = parse_args()
configure_logger(verbosity)
with traceback_cm:
    main(cmd, cmd_kwargs)
