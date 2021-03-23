import sqlite3

con = sqlite3.connect(":memory:")
cur = con.cursor()
cur.execute("create table people (name_last, age)")

who = "Yeltsin"
age = 72

# This is the qmark style:
cur.execute("insert into people values (?, ?)", (who, age))

# The qmark style used with executemany():
people = [
    ('Chirac', 70),
    ('Finnbogadóttir', 72),
    ('Schröder', 58),
]
cur.executemany("insert into people values (?, ?)", people)

# And this is the named style:
cur.execute("select * from people where name_last=:who and age=:age",
            {"who": who, "age": age})
print(cur.fetchall())

con.close()
