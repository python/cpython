# Example taken from https://www.sqlite.org/windowfunctions.html#udfwinfunc
import sqlite3


class WindowSumInt:
    def __init__(self):
        self.count = 0

    def step(self, value):
        """Adds a row to the current window."""
        self.count += value

    def value(self):
        """Returns the current value of the aggregate."""
        return self.count

    def inverse(self, value):
        """Removes a row from the current window."""
        self.count -= value

    def finalize(self):
        """Returns the final value of the aggregate.

        Any clean-up actions should be placed here.
        """
        return self.count


con = sqlite3.connect(":memory:")
cur = con.execute("create table test(x, y)")
values = [
    ("a", 4),
    ("b", 5),
    ("c", 3),
    ("d", 8),
    ("e", 1),
]
cur.executemany("insert into test values(?, ?)", values)
con.create_window_function("sumint", 1, WindowSumInt)
cur.execute("""
    select x, sumint(y) over (
        order by x rows between 1 preceding and 1 following
    ) as sum_y
    from test order by x
""")
print(cur.fetchall())
