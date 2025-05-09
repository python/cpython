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


def execute(c, sql, suppress_errors=True, theme=theme_no_color):
    """Helper that wraps execution of SQL code.

    This is used both by the REPL and by direct execution from the CLI.

    'c' may be a cursor or a connection.
    'sql' is the SQL string to execute.
    """

    try:
        for row in c.execute(sql):
            print(row)
    except sqlite3.Error as e:
        t = theme.traceback
        tp = type(e).__name__
        try:
            tp += f" ({e.sqlite_errorname})"
        except AttributeError:
            pass
        print(
            f"{t.type}{tp}{t.reset}: {t.message}{e}{t.reset}", file=sys.stderr
        )
        if not suppress_errors:
            sys.exit(1)


class SqliteInteractiveConsole(InteractiveConsole):
    """A simple SQLite REPL."""
    PS1 = "sqlite> "
    PS2 = "    ... "
    ruler = "="
    doc_header = "Documented commands (type .help <command>):"
    undoc_header = "Undocumented commands:"

    def __init__(self, connection, use_color=False):
        super().__init__()
        self._con = connection
        self._cur = connection.cursor()
        self._use_color = use_color
        self._theme = get_theme(force_no_color=not use_color)

        s = self._theme.syntax
        sys.ps1 = f"{s.prompt}{self.PS1}{s.reset}"
        sys.ps2 = f"{s.prompt}{self.PS2}{s.reset}"

    def do_version(self, _):
        """.version

        Show version of the runtime SQLite library.
        """
        print(sqlite3.sqlite_version)

    def do_help(self, arg):
        """.help [-all] [command]

        Without argument, print the list of available commands.
        With a command name as argument, print help about that command.
        With more command names as arguments, only the first one is used.
        """
        if not arg:
            cmds = sorted(name[3:] for name in dir(self.__class__)
                          if name.startswith("do_"))
            cmds_doc = []
            cmds_undoc = []
            for cmd in cmds:
                if getattr(self, f"do_{cmd}").__doc__:
                    cmds_doc.append(cmd)
                else:
                    cmds_undoc.append(cmd)
            self._print_commands(self.doc_header, cmds_doc, 80)
            self._print_commands(self.undoc_header, cmds_undoc, 80)
        else:
            arg = arg.split()[0]
            if arg in ("-all", "--all"):
                names = sorted(name for name in dir(self.__class__)
                               if name.startswith("do_"))
                print(self._help_message_from_method_names(names))
            else:
                if (method := getattr(self, "do_" + arg, None)) is not None:
                    print(self._help_message_from_doc(method.__doc__))
                else:
                    self._error(f"No help for '{arg}'")

    def do_quit(self, _):
        """.q(uit)

        Exit this program.
        """
        sys.exit(0)

    do_q = do_quit

    def _help_message_from_doc(self, doc):
        # copied from Lib/pdb.py#L2544
        lines = [line.strip() for line in doc.rstrip().splitlines()]
        if not lines:
            return "No help message found."
        if "" in lines:
            usage_end = lines.index("")
        else:
            usage_end = 1
        formatted = []
        indent = " " * len(self.PS1)
        for i, line in enumerate(lines):
            if i == 0:
                prefix = "Usage: "
            elif i < usage_end:
                prefix = "       "
            else:
                prefix = ""
            formatted.append(indent + prefix + line)
        return "\n".join(formatted)

    def _help_message_from_method_names(self, names):
        formatted = []
        indent = " " * len(self.PS1)
        for name in names:
            if not (doc := getattr(self, name).__doc__):
                formatted.append(f".{name[3:]}")
                continue
            lines = [line.strip() for line in doc.rstrip().splitlines()]
            if "" in lines:
                usage_end = lines.index("")
            else:
                usage_end = 1
            for i, line in enumerate(lines):
                # skip method aliases, e.g. do_q for do_quit
                if i == 0 and line in formatted:
                    break
                elif i < usage_end:
                    formatted.append(line)
                elif not line and i == usage_end:
                    continue
                else:
                    formatted.append(indent + line)
        return "\n".join(formatted)

    def _error(self, msg):
        t = self._theme.traceback
        self.write(f"{t.message}{msg}{t.reset}\n")

    def _print_commands(self, header, cmds, maxcol):
        # copied and modified from Lib/cmd.py#L351
        if cmds:
            print(header)
            if self.ruler:
                print(self.ruler * len(header))
            self._columnize(cmds, maxcol-1)
            print()

    def _columnize(self, strings, displaywidth=80):
        """Display a list of strings as a compact set of columns.

        Each column is only as wide as necessary.
        Columns are separated by two spaces (one was not legible enough).
        """
        # copied and modified from Lib/cmd.py#L359
        if not strings:
            print("<empty>")
            return

        size = len(strings)
        if size == 1:
            print(strings[0])
            return
        # Try every row count from 1 upwards
        for nrows in range(1, size):
            ncols = (size+nrows-1) // nrows
            colwidths = []
            totwidth = -2
            for col in range(ncols):
                colwidth = 0
                for row in range(nrows):
                    i = row + nrows*col
                    if i >= size:
                        break
                    x = strings[i]
                    colwidth = max(colwidth, len(x))
                colwidths.append(colwidth)
                totwidth += colwidth + 2
                if totwidth > displaywidth:
                    break
            if totwidth <= displaywidth:
                break
        else:
            nrows = size
            ncols = 1
            colwidths = [0]
        for row in range(nrows):
            texts = []
            for col in range(ncols):
                i = row + nrows*col
                if i >= size:
                    x = ""
                else:
                    x = strings[i]
                texts.append(x)
            while texts and not texts[-1]:
                del texts[-1]
            for col in range(len(texts)):
                texts[col] = texts[col].ljust(colwidths[col])
            print("  ".join(texts))

    def runsource(self, source, filename="<input>", symbol="single"):
        """Override runsource, the core of the InteractiveConsole REPL.

        Return True if more input is needed; buffering is done automatically.
        Return False if input is a complete statement ready for execution.
        """
        if not source or source.isspace():
            return False
        if source[0] == ".":
            if line := source[1:].strip():
                try:
                    cmd, arg = line.split(maxsplit=1)
                except ValueError:
                    cmd, arg = line, None
                if (func := getattr(self, "do_" + cmd, None)) is not None:
                    func(arg)
                else:
                    t = self._theme.traceback
                    self.write(f'{t.type}Error{t.reset}:{t.message} unknown'
                               f' command or invalid arguments:  "{line}".'
                               f' Enter ".help" for help{t.reset}\n')
        else:
            if not sqlite3.complete_statement(source):
                return True
            execute(self._cur, source, theme=self._theme)
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

    theme = get_theme()

    con = sqlite3.connect(args.filename, isolation_level=None)
    try:
        if args.sql:
            # SQL statement provided on the command-line; execute it directly.
            execute(con, args.sql, suppress_errors=False, theme=theme)
        else:
            # No SQL provided; start the REPL.
            console = SqliteInteractiveConsole(con, use_color=True)
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
