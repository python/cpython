
"""
    A __main__ for the Logging module for general logging of
    python scripts, either ones designed with logging in mind or not.

    Solves issue https://github.com/python/cpython/issues/93557
    -- May Draskovics
"""

from contextlib import contextmanager   ## contextmanager -> log_exception
import argparse                         ## argparse.ArgumentParser
import logging                          ## logging.basicConfig, all other logging functions
import sys                              ## sys.stdout
import os                               ## os.path.isfile & os.remove

@contextmanager
def log_exception(logger:logging.Logger) -> None:
    """
        log_exception(logging.Logger) -> None
            - context manager for running a script with logging
                and catching exceptions for logging

        :params:
            logger (logging.Logger) = Current logger for our script
    """
    try:
        yield
    except Exception as e:
        logger.exception(e)
        logger.critical("Script ended on an unhandled exception!")

def log_file(script:str) -> None:
    """
        log_file(str) -> None
            - Runs a python script with logging and enable

        :params:
            script (str) = The script that we want to log
    """


    logging.info("Logger initialized!")

    """
        Even with logging we should kill the process
        if we come into an unhandled exception otherwise
        we could have some unwanted behavior
    """

    with log_exception(logging.getLogger()):
        if script:
            with open(script, "r") as f:
                exec(f.read())
        else:
            exec(input(">> "))
        logging.info(f"Script \"{script}\" has ran successfully!")
        sys.exit(0)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Python Logging Module")
    parser.add_argument("-t", "--time_stamp", default=False, action="store_true", help="Include time stamp in output.")
    parser.add_argument("-p", "--remap_print", default=False, action="store_true", help="Remap the \"print\" function to \"logging.info\".")
    parser.add_argument("-d", "--double_print", default=False, action="store_true", help="Remap \"print\" to a lambda printing and logging all print statements.")
    parser.add_argument("-f", "--file", default="", help="Output log to file.")
    parser.add_argument("-l", "--level", default=0, type=int, help="Log level [1-5] (1=debug, 2=info, 3=warning, 4=error, 5=critical)")
    parser.add_argument("-s", "--string_format", default="DEFAULT", help="Format for logs.")
    ## these args aren't used on first run, but are used on subsequent runs
    parser.add_argument("-r", "--reuse_logger", default=False, action="store_true", help="Reuse the same logger for multiple scripts.")
    parser.add_argument("-c", "--clear_logfile", default=False, action="store_true", help="Clear the log file before writing to it.")
    ## the last argument is the file being executed
    parser.add_argument("script", default=None, nargs="?", help="Python script to be logged.")

    args = parser.parse_args()

    assert args.script is not None, "No script specified, please specify a script to be logged."

    args.level = args.level * 10

    if args.remap_print and args.level != 0 :
        print = logging.info
    elif args.double_print and args.level != 0:
        ## write to stdout and log file
        print = lambda *args, **kwargs: sys.stdout.write(f"{args[0]}\n") and logging.info(*args, **kwargs)

    ## set the output format
    outputFormat = args.string_format
    if outputFormat == "DEFAULT":
        outputFormat = "%(asctime)s [%(threadName)-12.12s] [%(levelname)-6s] %(message)s" if args.time_stamp else "[%(threadName)-12.12s] [%(levelname)-6s] %(message)s"

    ## setup logging
    if args.file:
        if os.path.exists(args.file):
            ## just to make life easier if we want to reuse the same file
            if args.clear_logfile:
                os.remove(args.file)
            ## this is to avoid large files and to avoid cluttering up the log file
            assert args.reuse_logger, "Log file already exists, please use the --reuse_logger flag to reuse the same logger."
        logging.basicConfig(filename=args.file, format=outputFormat, level=args.level)
    else:
        logging.basicConfig(format=outputFormat, level=args.level, handlers=[logging.StreamHandler(sys.stdout), logging.StreamHandler(sys.stderr)])

    log_file(args.script)
