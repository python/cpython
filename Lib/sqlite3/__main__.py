"""A simple SQLite CLI for the sqlite3 module.

Apart from using 'argparse' for the command-line interface,
this module implements the REPL as a thin wrapper around
the InteractiveConsole class from the 'code' stdlib module.
"""
import sqlite3
import sys

from argparse import ArgumentParser
from code import InteractiveConsole
from textwrap import dedent
from _colorize import get_theme, theme_no_color

from ._completer import completer


def execute(c, sql, suppress_errors=True, theme=theme_no_color):
    """Helper that wraps execution of SQL code.

    This is used both by the REPL and by direct execution from the CLI.

    'c' may be a cursor or a connection.
    'sql' is the SQL string to execute.
    """

    error = None
    if "\0" in sql:
        # A NUL makes c.execute() raise a broad ValueError; pre-check it.
        error = ValueError("embedded null character")
    else:
        try:
            for row in c.execute(sql):
                print(row)
        except (sqlite3.Error, UnicodeEncodeError) as e:  # or a lone surrogate
            error = e
    if error is not None:
        t = theme.traceback
        tp = type(error).__name__
        try:
            tp += f" ({error.sqlite_errorname})"
        except AttributeError:
            pass
        print(
            f"{t.type}{tp}{t.reset}: {t.message}{error}{t.reset}", file=sys.stderr
        )
        if not suppress_errors:
            sys.exit(1)


class SqliteInteractiveConsole(InteractiveConsole):
    """A simple SQLite REPL."""

    def __init__(self, connection, use_color=False):
        super().__init__()
        self._con = connection
        self._cur = connection.cursor()
        self._use_color = use_color

    def runsource(self, source, filename="<input>", symbol="single"):
        """Override runsource, the core of the InteractiveConsole REPL.

        Return True if more input is needed; buffering is done automatically.
        Return False if input is a complete statement ready for execution.
        """
        theme = get_theme(force_no_color=not self._use_color)

        if not source or source.isspace():
            return False
        # Remember to update CLI_COMMANDS in _completer.py
        if source[0] == ".":
            match source[1:].strip():
                case "version":
                    print(sqlite3.sqlite_version)
                case "help":
                    t = theme.syntax
                    print(f"Enter SQL code or one of the below commands, and press enter.\n\n"
                          f"{t.builtin}.version{t.reset}    Print underlying SQLite library version\n"
                          f"{t.builtin}.help{t.reset}       Print this help message\n"
                          f"{t.builtin}.quit{t.reset}       Exit the CLI, equivalent to CTRL-D\n")
                case "quit":
                    sys.exit(0)
                case "":
                    pass
                case _ as unknown:
                    t = theme.traceback
                    self.write(f'{t.type}Error{t.reset}: {t.message}unknown '
                               f'command: "{unknown}"{t.reset}\n')
        else:
            error = None
            if "\0" in source:
                # A NUL makes complete_statement() raise a broad ValueError;
                # pre-check it so ValueError from other code still surfaces.
                error = ValueError("embedded null character")
            else:
                try:
                    complete = sqlite3.complete_statement(source)
                except UnicodeEncodeError as e:  # a lone surrogate
                    error = e
            if error is not None:
                t = theme.traceback
                self.write(f"{t.type}{type(error).__name__}{t.reset}: "
                           f"{t.message}{error}{t.reset}\n")
                return False
            if not complete:
                return True
            execute(self._cur, source, theme=theme)
        return False


def main(*args):
    parser = ArgumentParser(description="Python sqlite3 CLI")
    parser.add_argument(
        "filename", type=str, default=":memory:", nargs="?",
        help=(
            "SQLite database to open (defaults to `:memory:`). "
            "A new database is created if the file does not previously exist."
        ),
    )
    parser.add_argument(
        "sql", type=str, nargs="?",
        help=(
            "An SQL query to execute. "
            "Any returned rows are printed to `stdout`."
        ),
    )
    parser.add_argument(
        "-v", "--version", action="version",
        version=f"SQLite version {sqlite3.sqlite_version}",
        help="Print underlying SQLite library version",
    )
    args = parser.parse_args(*args)

    if args.filename == ":memory:":
        db_name = "a transient in-memory database"
    else:
        db_name = repr(args.filename)

    # Prepare REPL banner and prompts.
    if sys.platform == "win32" and "idlelib.run" not in sys.modules:
        eofkey = "CTRL-Z"
    else:
        eofkey = "CTRL-D"
    banner = dedent(f"""
        sqlite3 shell, running on SQLite version {sqlite3.sqlite_version}
        Connected to {db_name}

        Each command will be run using execute() on the cursor.
        Type ".help" for more information; type ".quit" or {eofkey} to quit.
    """).strip()

    theme = get_theme()
    s = theme.syntax

    # Use RL_PROMPT_START_IGNORE (\001) and RL_PROMPT_END_IGNORE (\002) to
    # bracket non-printing characters. This tells readline to ignore them
    # when calculating screen space for redisplay during history scrolling.
    sys.ps1 = f"\001{s.prompt}\002sqlite> \001{s.reset}\002"
    sys.ps2 = f"\001{s.prompt}\002    ... \001{s.reset}\002"

    con = sqlite3.connect(args.filename, isolation_level=None)
    try:
        if args.sql:
            # SQL statement provided on the command-line; execute it directly.
            execute(con, args.sql, suppress_errors=False, theme=theme)
        else:
            # No SQL provided; start the REPL.
            with completer(con):
                console = SqliteInteractiveConsole(con, use_color=True)
                console.interact(banner, exitmsg="")
    finally:
        con.close()

    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv[1:])
