#!/bin/bash

# https://bugs.python.org/issue13299
# https://bugs.python.org/file37673/sqlite_namedtuplerow.patch
# https://github.com/python/cpython/tree/master/Modules/_sqlite


IMPORTS="import sqlite3, collections, named_row; con=sqlite3.connect(':memory:')"
CREATE="con.execute('CREATE TABLE t(a,b)')"
POPULATE="for i in range(1000): con.execute('INSERT INTO t VALUES (?,?)', (i, 10000-i))"
TEST="con.execute('SELECT * FROM t').fetchall()"

echo "tuple"
python -m timeit -s "$IMPORTS" -s "$CREATE" -s "$POPULATE" -- "for x in $TEST: x[0]"

factory="con.row_factory=sqlite3.Row"
echo -e "\nsqlite3.row # $factory"
python -m timeit -s "$IMPORTS" -s "$factory" -s"$CREATE" -s "$POPULATE" -- "for x in $TEST: x['a']"

factory="con.row_factory=collections.namedtuple('Row', ['a','b'])"
echo -e "\nnamedtuple  # $factory"
python -m timeit -s "$IMPORTS" -s "$factory" -s"$CREATE" -s "$POPULATE" -- "for x in $TEST: x.a"

factory="con.row_factory=named_row.NamedRow"
echo -e "\nNamedRow    # $factory [0]"
python -m timeit -s "$IMPORTS" -s "$factory" -s"$CREATE" -s "$POPULATE" -- "for x in $TEST: x[0]"

factory="con.row_factory=named_row.NamedRow"
echo -e "\nNamedRow    # $factory"
python -m timeit -s "$IMPORTS" -s "$factory" -s"$CREATE" -s "$POPULATE" -- "for x in $TEST: x.a"

# tuple         804  540
# sqlite3.row   983  673
# namedtuple   1.38  923
# NamedRow[0]  1.34  1.03
# NamedRow.a   2.33  1.5
