import sqlite3

class CountCursorsConnection(sqlite3.Connection):
    def __init__(self, *args, **kwargs):
        sqlite3.Connection.__init__(self, *args, **kwargs)
        self.numcursors = 0

    def cursor(self, *args, **kwargs):
        self.numcursors += 1
        return sqlite3.Connection.cursor(self, *args, **kwargs)

con = sqlite3.connect(":memory:", factory=CountCursorsConnection)
cur1 = con.cursor()
cur2 = con.cursor()
print(con.numcursors)
