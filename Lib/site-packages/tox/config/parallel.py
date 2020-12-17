from __future__ import absolute_import, unicode_literals

from argparse import ArgumentTypeError

ENV_VAR_KEY_PUBLIC = "TOX_PARALLEL_ENV"
ENV_VAR_KEY_PRIVATE = "_TOX_PARALLEL_ENV"
OFF_VALUE = 0
DEFAULT_PARALLEL = OFF_VALUE


def auto_detect_cpus():
    try:
        from os import sched_getaffinity  # python 3 only

        def cpu_count():
            return len(sched_getaffinity(0))

    except ImportError:
        # python 2 options
        try:
            from os import cpu_count
        except ImportError:
            from multiprocessing import cpu_count

    try:
        n = cpu_count()
    except NotImplementedError:  # pragma: no cov
        n = None  # pragma: no cov
    return n if n else 1


def parse_num_processes(s):
    if s == "all":
        return None
    if s == "auto":
        return auto_detect_cpus()
    else:
        value = int(s)
        if value < 0:
            raise ArgumentTypeError("value must be positive")
        return value


def add_parallel_flags(parser):
    parser.add_argument(
        "-p",
        "--parallel",
        nargs="?",
        const="auto",
        dest="parallel",
        help="run tox environments in parallel, the argument controls limit: all,"
        " auto or missing argument - cpu count, some positive number, 0 to turn off",
        action="store",
        type=parse_num_processes,
        default=DEFAULT_PARALLEL,
        metavar="VAL",
    )
    parser.add_argument(
        "-o",
        "--parallel-live",
        action="store_true",
        dest="parallel_live",
        help="connect to stdout while running environments",
    )


def add_parallel_config(parser):
    parser.add_testenv_attribute(
        "depends",
        type="env-list",
        help="tox environments that this environment depends on (must be run after those)",
    )

    parser.add_testenv_attribute(
        "parallel_show_output",
        type="bool",
        default=False,
        help="if set to True the content of the output will always be shown "
        "when running in parallel mode",
    )
