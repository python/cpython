import sqlite3

con = sqlite3.connect(":memory:")
cur = con.cursor()
cur.execute("create table lang (lang_name, lang_age)")

# This is the qmark style:
cur.execute("insert into lang values (?, ?)", ("C", 49))

# The qmark style used with executemany():
lang_list = [
    ("Fortran", 64),
    ("Python", 30),
    ("Go", 11),
]
cur.executemany("insert into lang values (?, ?)", lang_list)

# And this is the named style:
cur.execute("select * from lang where lang_name=:name and lang_age=:age",
            {"name": "C", "age": 49})
print(cur.fetchall())

con.close()
