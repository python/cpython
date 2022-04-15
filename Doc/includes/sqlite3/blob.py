import sqlite3

con = sqlite3.connect(":memory:")
con.execute("create table test(blob_col blob)")
con.execute("insert into test(blob_col) values (zeroblob(10))")

# Write to our blob, using two write operations:
with con.blobopen("test", "blob_col", 1) as blob:
    blob.write(b"Hello")
    blob.write(b"World")

# Read the contents of our blob
with con.blobopen("test", "blob_col", 1) as blob:
    greeting = blob.read()

print(greeting)  # outputs "b'HelloWorld'"
