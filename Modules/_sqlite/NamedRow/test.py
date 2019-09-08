import sqlite3
import pytest
from named_row import NamedRow


def create_database():
    """Database ('a', '-dash')
    Filled with a 1000 of (1, 2)
    """
    conn = sqlite3.connect(":memory:")
    conn.execute('CREATE TABLE t(a, "-dash")')
    for i in range(1000):
        conn.execute("INSERT INTO t VALUES(?,?)", (1, 2))
    conn.row_factory = NamedRow
    return conn


@pytest.fixture
def db(scope="session"):
    yield create_database()


@pytest.fixture
def row(db):
    """An instance of NamedRow."""
    cursor = db.execute("SELECT * FROM t LIMIT 1")
    row = cursor.fetchone()
    yield row


def test_01_database(db):
    assert "NamedRow" in repr(db.row_factory)
    cursor = db.execute("SELECT * FROM t LIMIT 1")
    assert ("a", "-dash") == tuple(x[0] for x in cursor.description)


def test_02_is_NamedRow(row):
    assert "NamedRow" in row.__class__.__name__
    assert "a" in row
    repr_str = repr(row)
    assert "NamedRow" in repr_str
    assert "-dash" in repr_str


def test_11_iterable_keyValue_pairs(row):
    assert [("a", 1), ("-dash", 2)] == [x for x in row]


def test_12_iterable_into_dict(row):
    assert dict((("a", 1), ("-dash", 2))) == dict(row)


def test_30_index_by_number(row):
    assert row[0] == 1
    assert row[1] == 2


def test_31_index_by_string(row):
    assert row["a"] == 1
    assert row["-dash"] == 2


def test_32_access_by_attribute(row):
    assert row.a == 1


def test_50_IndexError(row):
    try:
        row[10]
        raise RuntimeError("Expected IndexError")
    except IndexError:
        pass


def test_51_ValueError(row):
    try:
        row["alphabet"]
        raise RuntimeError("Expected ValueError")
    except ValueError:
        pass


def test_52_AttributeError(row):
    try:
        row.alphabet
        raise RuntimeError("Expected AttributeError")
    except AttributeError:
        pass


def test_53_not_assignable(row):
    try:
        row["a"] = 100
        raise RuntimeError("Expecting TypeError object does no support item assignment")
    except TypeError:
        pass


if __name__ == "__main__":
    pass
