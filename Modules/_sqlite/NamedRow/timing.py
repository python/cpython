import collections
import timeit
import sqlite3
from named_row import NamedRow


SETUP = """\
import sqlite3, collections
from named_row import NamedRow
conn = sqlite3.connect(":memory:")
conn.execute('CREATE TABLE t(a, b)')
for i in range(1000):
    conn.execute("INSERT INTO t VALUES(?,?)", (i, i))
cursor = conn.cursor()
"""
SQLITE3_ROW = SETUP + "cursor.row_factory=sqlite3.Row"
SQLITE3_NAMED = SETUP + "cursor.row_factory=sqlite3.NamedRow"
NAMED_TUPLE = SETUP + "cursor.row_factory=collections.namedtuple('Row', ['a','b'])"
NAMED_ROW = SETUP + "cursor.row_factory=NamedRow"

NUMBER = 1000
INDEX = """for x in cursor.execute("SELECT * FROM t").fetchall(): x[0]"""
DICTS = """for x in cursor.execute("SELECT * FROM t").fetchall(): x['a']"""
ATTRS = """for x in cursor.execute("SELECT * FROM t").fetchall(): x.a"""

tests = (
    (SETUP, INDEX, "sqlite3 indx"),
    (SQLITE3_ROW, INDEX, "sqlite3.Row indx"),
    (SQLITE3_ROW, DICTS, "sqlite3.Row dict"),
    (SQLITE3_NAMED, INDEX, "sqlite3.NamedRow indx"),
    (SQLITE3_NAMED, DICTS, "sqlite3.NamedRow dict"),
    (SQLITE3_NAMED, ATTRS, "sqlite3.NamedRow attr"),
    (NAMED_TUPLE, INDEX, "namedtuple indx"),
    (NAMED_TUPLE, ATTRS, "namedtuple attr"),
    (NAMED_ROW, DICTS, "NamedRow dict"),
    (NAMED_ROW, ATTRS, "NamedRow attr"),
)

base = 0

for setup, test, title in tests:
    try:
        v = timeit.timeit(test, setup=setup, number=1000)
    except AttributeError:
        print("AttributeError", title)
        continue

    if setup == SETUP:
        base = v
    percent = int(100 * ((v / base)) - 100)
    print(f"+{percent}%", round(1000 * v), "usec", title)
