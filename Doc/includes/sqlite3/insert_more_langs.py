import sqlite3

con = sqlite3.connect("mydb")

cur = con.cursor()

languages = (
    ("Smalltalk", 1972),
    ("Swift", 2014),
)

for lang in languages:
    cur.execute("insert into lang (name, first_appeared) values (?, ?)", lang)

# The changes will not be saved unless the transaction is committed explicitly:
con.commit()

con.close()
