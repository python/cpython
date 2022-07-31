import sqlite3
import sys

from argparse import ArgumentParser
from code import InteractiveConsole
from textwrap import dedent


def execute(c, sql):
    try:
        for row in c.execute(sql):
            print(row)
    except sqlite3.Error as e:
        tp = type(e).__name__
        try:
            print(f"{tp} ({e.sqlite_errorname}): {e}")
        except:
            print(f"{tp}: {e}")


class SqliteInteractiveConsole(InteractiveConsole):

    def __init__(self, connection):
        super().__init__()
        self._con = connection
        self._cur = connection.cursor()

    def runsql(self, sql):
        if not sqlite3.complete_statement(sql):
            return True
        execute(self._cur, sql)
        return False

    def runsource(self, source, filename="<input>", symbol="single"):
        keywords = {
            ".version": lambda: print(f"{sqlite3.sqlite_version}"),
            ".help": lambda: print("Enter SQL code and press enter."),
            ".quit": lambda: sys.exit(0),
        }
        if source in keywords:
            keywords[source]()
        else:
            self.runsql(source)


def main():
    parser = ArgumentParser(
        description="Python sqlite3 REPL",
        prog="python -m sqlite3",
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
    args = parser.parse_args()

    if args.filename == ":memory:":
        db_name = "a transient in-memory database"
    else:
        db_name = repr(args.filename)

    banner = dedent(f"""
        sqlite3 shell, running on SQLite version {sqlite3.sqlite_version}
        Connected to {db_name}

        Each command will be run using execute() on the cursor.
        Type ".help" for more information; type ".quit" or CTRL-D to quit.
    """).strip()
    sys.ps1 = "sqlite> "
    sys.ps2 = "    ... "

    con = sqlite3.connect(args.filename, isolation_level=None)
    try:
        if args.sql:
            execute(con, args.sql)
        else:
            console = SqliteInteractiveConsole(con)
            console.interact(banner, exitmsg="")
    finally:
        con.close()


main()
