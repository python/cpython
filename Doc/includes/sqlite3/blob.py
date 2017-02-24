import sqlite3

con = sqlite3.connect(":memory:")
# creating the table
con.execute("create table test(id integer primary key, blob_col blob)")
con.execute("insert into test(blob_col) values (zeroblob(10))")
# opening blob handle
blob = con.open_blob("test", "blob_col", 1, 1)
blob.write(b"a" * 5)
blob.write(b"b" * 5)
blob.seek(0)
print(blob.read()) # will print b"aaaaabbbbb"
blob.close()
