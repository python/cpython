import sqlite3

from code import InteractiveConsole


class SqliteInteractiveConsole(InteractiveConsole):

    def __init__(self, database):
        super().__init__()
        self._con = sqlite3.connect(database, isolation_level=None)
        self._cur = self._con.cursor()

    def runsql(self, sql):
        if sqlite3.complete_statement(sql):
            try:
                self._cur.execute(sql)
                rows = self._cur.fetchall()
                for row in rows:
                    print(row)
            except sqlite3.Error as e:
                print(f"{e.sqlite_errorname}: {e}")
            return False
        return True

    def runpy(self, source, filename="<input>", symbol="single"):
        code = self.compile(source, filename, symbol)
        self.runcode(code)
        return False

    def printhelp(self, ignored):
        print("Enter SQL code and press enter.")

    def runsource(self, source, filename="<input>", symbol="single"):
        keywords = {
            "version": lambda x: print(f"{sqlite3.sqlite_version}"),
            "help": self.printhelp,
            "copyright": self.runpy,
            "credits": self.runpy,
            "license": self.runpy,
            "license()": self.runpy,
            "quit()": self.runpy,
            "quit": self.runpy,
        }
        return keywords.get(source, self.runsql)(source)


if __name__ == "__main__":
    import sys
    from argparse import ArgumentParser
    from textwrap import dedent

    parser = ArgumentParser(
        description="Python sqlite3 REPL",
        prog="python -m sqlite3",
    )
    parser.add_argument(
        "-f", "--filename",
        type=str, dest="database", action="store", default=":memory:",
        help="Database to open (default in-memory database)",
    )
    args = parser.parse_args()

    if args.database == ":memory:":
        db_name = "a transient in-memory database"
    else:
        db_name = f"'{args.database}'"

    banner = dedent(f"""
        sqlite3 shell, running on SQLite version {sqlite3.sqlite_version}
        Connected to {db_name}

        Each command will be run using execute() on the cursor.
        Type "help" for more information; type "quit" or CTRL-D to quit.
    """).strip()
    sys.ps1 = "sqlite> "
    sys.ps2 = "    ... "

    console = SqliteInteractiveConsole(args.database)
    console.interact(banner, exitmsg="")
