import sqlite3

con = sqlite3.connect(":memory:")
# creating the table
con.execute("create table test(id integer primary key, blob_col blob)")
con.execute("insert into test(blob_col) values (zeroblob(10))")
# opening blob handle
blob = con.open_blob("test", "blob_col", 1)
blob.write(b"Hello")
blob.write(b"World")
blob.seek(0)
print(blob.read()) # will print b"HelloWorld"
blob.close()
