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
import _colorize as colorize

def _clr(color, use_color):
    if use_color:
        return color
    return ''

def execute(c, sql, suppress_errors=True, use_color=False):
    """Helper that wraps execution of SQL code.

    This is used both by the REPL and by direct execution from the CLI.

    'c' may be a cursor or a connection.
    'sql' is the SQL string to execute.
    """

    try:
        for row in c.execute(sql):
            print(row)
    except sqlite3.Error as e:
        theme = colorize.get_theme(force_color=True).traceback
        tp = type(e).__name__
        try:
            print(f"{_clr(theme.type, use_color)}{tp} ({e.sqlite_errorname})"
                  f"{_clr(theme.reset, use_color)}: "
                  f"{_clr(theme.message, use_color)}{e}{_clr(theme.RESET, use_color)}", file=sys.stderr)
        except AttributeError:
            print(f"{_clr(theme.type, use_color)}{tp}{_clr(theme.RESET, use_color)}: "
                  f"{_clr(theme.message, use_color)}{e}{_clr(theme.RESET, use_color)}", file=sys.stderr)
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
        match source:
            case ".version":
                print(f"{sqlite3.sqlite_version}")
            case ".help":
                print("Enter SQL code and press enter.")
            case ".quit":
                sys.exit(0)
            case _:
                if not sqlite3.complete_statement(source):
                    return True
                execute(self._cur, source, use_color=self._use_color)
        return False


def main(*args):
    parser = ArgumentParser(
        description="Python sqlite3 CLI",
        color=True,
    )
    parser.add_argument(
        "filename", type=str, default=":memory:", nargs="?",
        help=(
            "SQLite database to open (defaults to ':memory:'). "
            "A new database is created if the file does not previously exist."
        ),
    )
    parser.add_argument(
        "sql", type=str, nargs="?",
        help=(
            "An SQL query to execute. "
            "Any returned rows are printed to stdout."
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

    use_color = colorize.can_colorize()
    theme = colorize.get_theme(force_color=True).syntax

    sys.ps1 = f"{_clr(theme.prompt, use_color)}sqlite> {_clr(theme.RESET, use_color)}"
    sys.ps2 = f"{_clr(theme.prompt, use_color)}    ... {_clr(theme.RESET, use_color)}"

    con = sqlite3.connect(args.filename, isolation_level=None)
    try:
        if args.sql:
            # SQL statement provided on the command-line; execute it directly.
            execute(con, args.sql, suppress_errors=False)
        else:
            # No SQL provided; start the REPL.
            console = SqliteInteractiveConsole(con, use_color)
            try:
                import readline  # noqa: F401
            except ImportError:
                pass
            console.interact(banner, exitmsg="")
    finally:
        con.close()

    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv[1:])
