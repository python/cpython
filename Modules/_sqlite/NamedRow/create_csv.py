import csv
import sqlite3

METHODS = [
    "Native index",
    "Row index",
    "Row dict",
    "NamedRow index",
    "NamedRow dict",
    "NamedRow attr",
]

if __name__ == "__main__":
    db_filename = "instance.db"
    connection = sqlite3.Connection(db_filename)
    cursor = connection.cursor()
    LENGTHS = [
        x[0]
        for x in cursor.execute(
            "SELECT distinct fields FROM t ORDER BY fields"
        ).fetchall()
    ]
    with open(db_filename.split(".")[0] + ".csv", "wt") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(["Method"] + LENGTHS)
        for method in METHODS:
            values = cursor.execute(
                "SELECT timeit FROM t WHERE title=? ORDER BY fields", (method,)
            ).fetchall()
            writer.writerow([method] + [x[0] for x in values])
