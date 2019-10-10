import collections
import timeit
import random
import sqlite3
from named_row import NamedRow


FIELD_MAP = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
FIELD_NAME_LENGTH = 12
TESTS = (
    ("Native", "index"),
    ("Row", "index"),
    ("Row", "dict"),
    ("NamedRow", "index"),
    ("NamedRow", "dict"),
    ("NamedRow", "attr"),
)


connection = sqlite3.Connection("timing.db")
connection.executescript(
    """CREATE TABLE IF NOT EXISTS t (
    title char(15),
    fields int,
    timeit real
);
CREATE UNIQUE INDEX IF NOT EXISTS onlyone ON t (title, fields);"""
)


def make_field(idx: int) -> str:
    return FIELD_MAP[idx] * random.randrange(4, 15)


for number_of_fields in range(2, 41):
    # Each round will have identical fields to the proceeding. Show off the
    # speed of NamedRow over Row
    random.seed(101038)
    fields = [make_field(i) for i in range(number_of_fields)]
    select = ", ".join([f"{i} AS {fields[i]}" for i in range(number_of_fields)])
    print(f"Fields {number_of_fields}")

    # Process all tests for the same number of fields. Does this help with
    # fluctuations in background processes?
    for factory, test_method in TESTS:
        factory_row = ""
        if factory != "Native":
            factory_row = f"conn.row_factory=sqlite3.{factory}"

        setup = "\n".join(
            [
                "import sqlite3",
                "conn = sqlite3.connect(':memory:')",
                factory_row,
                "cursor = conn.cursor()",
            ]
        )
        string, formatter = {
            "index": ("row[{0}]", int),
            "dict": ("row['{0}']", lambda i: fields[i]),
            "attr": ("row.{0}", lambda i: fields[i]),
        }[test_method]

        timing = [f'row = cursor.execute("SELECT {select}").fetchone()']
        timing.extend([string.format(formatter(i)) for i in range(number_of_fields)])
        timing = "\n".join(timing)

        title = f"{factory} {test_method}"
        try:
            v = timeit.timeit(timing, setup=setup, number=10000)
        except AttributeError:
            print("AttributeError", title)
            continue

        connection.execute(
            "INSERT INTO t (title, fields, timeit) "
            "VALUES(?,?,?) ON CONFLICT (title, fields) "
            "DO UPDATE SET timeit=excluded.timeit "
            "WHERE excluded.timeit < t.timeit;",
            (title, number_of_fields, v),
        )
    connection.commit()
connection.close()
