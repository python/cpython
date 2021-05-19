import sqlite3

con = sqlite3.connect("mydb")

cur = con.cursor()
SELECT = "select name, first_appeared from people order by first_appeared, name"

# 1. Iterate over the rows available from the cursor, unpacking the
# resulting sequences to yield their elements (name, first_appeared):
cur.execute(SELECT)
for name, first_appeared in cur:
    print(f"The {name} programming language appeared in {first_appeared}.")

# 2. Equivalently:
cur.execute(SELECT)
for row in cur:
    print(f"The {row[0]} programming language appeared in {row[1]}.")

con.close()
