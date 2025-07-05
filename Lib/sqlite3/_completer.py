from _sqlite3 import OperationalError
from contextlib import contextmanager

try:
    from _sqlite3 import SQLITE_KEYWORDS
except ImportError:
    SQLITE_KEYWORDS = ()

_completion_matches = []


def _complete(con, text, state):
    global _completion_matches

    if state == 0:
        text_upper = text.upper()
        text_lower = text.lower()
        _completion_matches = [c + " " for c in SQLITE_KEYWORDS if c.startswith(text_upper)]
        cursor = con.cursor()
        schemata = tuple(row[1] for row
                         in cursor.execute("PRAGMA database_list"))
        # tables, indexes, triggers, and views
        select_clauses = (f"SELECT name FROM \"{schema}\".sqlite_master"
                          for schema in schemata)
        tables = (row[0] for row
                  in cursor.execute(" UNION ".join(select_clauses)))
        _completion_matches.extend(c + " " for c in tables
                                   if c.lower().startswith(text_lower))
        # columns
        try:
            select_clauses = (f"""\
                SELECT pti.name FROM "{schema}".sqlite_master AS sm
                JOIN pragma_table_xinfo(sm.name,'{schema}') AS pti
                WHERE sm.type='table'""" for schema in schemata)
            columns = (row[0] for row
                       in cursor.execute(" UNION ".join(select_clauses)))
            _completion_matches.extend(c + " " for c in columns
                                       if c.lower().startswith(text_lower))
        except OperationalError:
            # skip on SQLite<3.16.0 where pragma table-valued function is not
            # supported yet
            pass
        # functions
        try:
            funcs = (row[0] for row in cursor.execute("""\
                    SELECT DISTINCT UPPER(name) FROM pragma_function_list()
                    WHERE name NOT IN ('->', '->>')"""))
            _completion_matches.extend(c + "(" for c in funcs
                                       if c.startswith(text_upper))
        except OperationalError:
            # skip on SQLite<3.30.0 where function_list is not supported yet
            pass
        # schemata
        _completion_matches.extend(c for c in schemata
                                   if c.lower().startswith(text_lower))
        _completion_matches = sorted(set(_completion_matches))
    try:
        return _completion_matches[state]
    except IndexError:
        return None


@contextmanager
def completer(con):
    try:
        import readline
    except ImportError:
        yield
        return

    old_completer = readline.get_completer()
    def complete(text, state):
        return _complete(con, text, state)
    try:
        readline.set_completer(complete)
        if readline.backend == "editline":
            # libedit uses "^I" instead of "tab"
            command_string = "bind ^I rl_complete"
        else:
            command_string = "tab: complete"
        readline.parse_and_bind(command_string)
        yield
    finally:
        readline.set_completer(old_completer)
