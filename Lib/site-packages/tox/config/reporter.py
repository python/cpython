from __future__ import absolute_import, unicode_literals


def add_verbosity_commands(parser):
    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        dest="verbose_level",
        default=0,
        help="increase verbosity of reporting output."
        "-vv mode turns off output redirection for package installation, "
        "above level two verbosity flags are passed through to pip (with two less level)",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="count",
        dest="quiet_level",
        default=0,
        help="progressively silence reporting output.",
    )
