import sqlite3

FIELD_MAX_WIDTH = 20
TABLE_NAME = 'people'
SELECT = 'select * from %s order by age, name_last' % TABLE_NAME

con = sqlite3.connect("mydb")

cur = con.cursor()
cur.execute(SELECT)

# Print a header.
for fieldDesc in cur.description:
    print fieldDesc[0].ljust(FIELD_MAX_WIDTH) ,
print # Finish the header with a newline.
print '-' * 78

# For each row, print the value of each field left-justified within
# the maximum possible width of that field.
fieldIndices = range(len(cur.description))
for row in cur:
    for fieldIndex in fieldIndices:
        fieldValue = str(row[fieldIndex])
        print fieldValue.ljust(FIELD_MAX_WIDTH) ,

    print # Finish the row with a newline.
