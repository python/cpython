from contextlib import contextmanager


KEYWORDS = ("ABORT", "ACTION", "ADD", "AFTER", "ALL", "ALTER", "ALWAYS",
            "ANALYZE", "AND", "AS", "ASC", "ATTACH", "AUTOINCREMENT",
            "BEFORE", "BEGIN", "BETWEEN", "BY", "CASCADE", "CASE", "CAST",
            "CHECK", "COLLATE", "COLUMN", "COMMIT", "CONFLICT",
            "CONSTRAINT", "CREATE", "CROSS", "CURRENT", "CURRENT_DATE",
            "CURRENT_TIME", "CURRENT_TIMESTAMP", "DATABASE", "DEFAULT",
            "DEFERRABLE", "DEFERRED", "DELETE", "DESC", "DETACH",
            "DISTINCT", "DO", "DROP", "EACH", "ELSE", "END", "ESCAPE",
            "EXCEPT", "EXCLUDE", "EXCLUSIVE", "EXISTS", "EXPLAIN", "FAIL",
            "FILTER", "FIRST", "FOLLOWING", "FOR", "FOREIGN", "FROM",
            "FULL", "GENERATED", "GLOB", "GROUP", "GROUPS", "HAVING", "IF",
            "IGNORE", "IMMEDIATE", "IN", "INDEX", "INDEXED", "INITIALLY",
            "INNER", "INSERT", "INSTEAD", "INTERSECT", "INTO", "IS",
            "ISNULL", "JOIN", "KEY", "LAST", "LEFT", "LIKE", "LIMIT",
            "MATCH", "MATERIALIZED", "NATURAL", "NO", "NOT", "NOTHING",
            "NOTNULL", "NULL", "NULLS", "OF", "OFFSET", "ON", "OR",
            "ORDER", "OTHERS", "OUTER", "OVER", "PARTITION", "PLAN",
            "PRAGMA", "PRECEDING", "PRIMARY", "QUERY", "RAISE", "RANGE",
            "RECURSIVE", "REFERENCES", "REGEXP", "REINDEX", "RELEASE",
            "RENAME", "REPLACE", "RESTRICT", "RETURNING", "RIGHT",
            "ROLLBACK", "ROW", "ROWS", "SAVEPOINT", "SELECT", "SET",
            "TABLE", "TEMP", "TEMPORARY", "THEN", "TIES", "TO",
            "TRANSACTION", "TRIGGER", "UNBOUNDED", "UNION", "UNIQUE",
            "UPDATE", "USING", "VACUUM", "VALUES", "VIEW", "VIRTUAL",
            "WHEN", "WHERE", "WINDOW", "WITH", "WITHOUT")

_completion_matches = []

def _complete(text, state):
    global _completion_matches
    if state == 0:
        text_upper = text.upper()
        _completion_matches = [c for c in KEYWORDS if c.startswith(text_upper)]
    try:
        return _completion_matches[state] + " "
    except IndexError:
        return None


@contextmanager
def enable_completer():
    try:
        import readline
    except ImportError:
        yield
        return

    old_completer = readline.get_completer()
    try:
        readline.set_completer(_complete)
        if readline.backend == "editline":
            # libedit uses "^I" instead of "tab"
            command_string = "bind ^I rl_complete"
        else:
            command_string = "tab: complete"
        readline.parse_and_bind(command_string)
        yield
    finally:
        readline.set_completer(old_completer)
