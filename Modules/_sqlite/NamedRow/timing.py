import collections
import timeit
import random
import sqlite3
from named_row import NamedRow


FIELD_MAP = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
TESTS = (
    ("Native", "index"),
    ("Row", "index"),
    ("Row", "dict"),
    ("NamedRow", "index"),
    ("NamedRow", "dict"),
    ("NamedRow", "attr"),
)


def make_field(idx: int) -> str:
    return FIELD_MAP[idx] * random.randrange(4, 15)


def setup_test_instantiation(factory, method, fields):
    """
    factory: 'Native', 'Row', 'NamedRow'
    method: 'index', 'dict', or 'attr'
    fields: all possible fields
    """

    select = ", ".join([f"{i} AS {field}" for i, field in enumerate(fields)])
    factory_row = "" if factory == "Native" else f"cursor.row_factory=sqlite3.{factory}"
    setup = [
        "import sqlite3",
        "cursor = sqlite3.Connection(':memory:').cursor()",
        f"data = tuple(1 for x in range({len(fields)}))",
        factory_row,
        f'cursor.execute("SELECT {select}")',
    ]

    string, formatter = {
        "index": ("row[{0}]", int),
        "dict": ("row['{0}']", lambda i: fields[i]),
        "attr": ("row.{0}", lambda i: fields[i]),
    }[method]

    test = []
    if factory == "Native":
        test.append("row = data")
    else:
        test.append("row = cursor.row_factory(cursor, data)")
    test.extend([string.format(formatter(i)) for i in range(len(fields))])

    return "\n".join(setup), "\n".join(test)


def setup_test_fetchone(factory, method, fields):
    """
    factory: 'Native', 'Row', 'NamedRow'
    method: 'index', 'dict', or 'attr'
    fields: all possible fields
    """
    select = ", ".join([f"{i} AS {field}" for i, field in enumerate(fields)])

    factory_row = ""
    if factory != "Native":
        factory_row = f"conn.row_factory=sqlite3.{factory}"

    timing_setup = [
        "import sqlite3",
        "conn = sqlite3.connect(':memory:')",
        factory_row,
        "cursor = conn.cursor()",
    ]

    string, formatter = {
        "index": ("row[{0}]", int),
        "dict": ("row['{0}']", lambda i: fields[i]),
        "attr": ("row.{0}", lambda i: fields[i]),
    }[method]

    test = [f'row = cursor.execute("SELECT {select}").fetchone()']
    test.extend([string.format(formatter(i)) for i in range(len(fields))])

    return "\n".join(timing_setup), "\n".join(test)


def run_test(db_name, function):
    connection = sqlite3.Connection(db_name)
    connection.executescript(
        """CREATE TABLE IF NOT EXISTS t (
        title char(15),
        fields int,
        timeit real
    );
    CREATE UNIQUE INDEX IF NOT EXISTS onlyone ON t (title, fields);"""
    )
    for number_of_fields in range(2, 41):

        # Each round will have identical fields to the proceeding. Show off the
        # speed of NamedRow over Row
        random.seed(101038)
        fields = [make_field(i) for i in range(number_of_fields)]

        print(f"Fields {number_of_fields}")

        # Process all tests for the same number of fields. Does this help with

        for factory, test_method in TESTS:
            title = f"{factory} {test_method}"
            setup, test = function(factory, test_method, fields)

            # print(setup)
            # print(test)
            try:
                v = timeit.timeit(test, setup=setup, number=100000)
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


if __name__ == "__main__":
    # run_test("fetchone.db", setup_test_fetchone)
    run_test("instance.db", setup_test_instantiation)
