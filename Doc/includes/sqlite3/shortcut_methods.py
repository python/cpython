import sqlite3

langs = [
    ("C++", 1985),
    ("Objective-C", 1984),
]

con = sqlite3.connect(":memory:")

# Create the table
con.execute("create table lang(name, first_appeared)")

# Fill the table
con.executemany("insert into lang(name, first_appeared) values (?, ?)", langs)

# Print the table contents
for row in con.execute("select name, first_appeared from lang"):
    print(row)

print("I just deleted", con.execute("delete from lang").rowcount, "rows")

# close is not a shortcut method and it's not called automatically,
# so the connection object should be closed manually
con.close()
