import sqlite3

con = sqlite3.connect(":memory:")

con.execute("create table test(blob_col blob)")
con.execute("insert into test(blob_col) values (zeroblob(10))")

with con.blobopen("test", "blob_col", 1) as blob:
    blob.write(b"Hello")
    blob.write(b"World")
    blob.seek(0)
    print(blob.read())  # will print b"HelloWorld"
