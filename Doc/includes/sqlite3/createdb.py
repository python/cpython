# Not referenced from the documentation, but builds the database file the other
# code snippets expect.

import sqlite3
import os

DB_FILE = "mydb"

if os.path.exists(DB_FILE):
    os.remove(DB_FILE)

con = sqlite3.connect(DB_FILE)
cur = con.cursor()
cur.execute("""
        create table lang
        (
          name           varchar(20),
          first_appeared integer
        )
        """)

cur.execute("insert into lang (name, first_appeared) values ('Forth', 1970)")
cur.execute("insert into lang (name, first_appeared) values ('Ada', 1980)")

con.commit()

cur.close()
con.close()
