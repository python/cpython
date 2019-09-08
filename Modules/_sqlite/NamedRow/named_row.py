import sqlite3


class NamedRow:
    """Sqlite3.NamedRow as a namedtuple like object.
    """

    def __init__(self, cursor, values):
        """Create a named row.

        The title_tuple comes from tuple(x[0] for x in defination. However,
        there is no need to keep creating it anew.
        """
        self._datum = dict(zip(("a", "b"), values))

    #        self._titles = ("a", "b")
    # 1 msec for creating the tuple
    # self._titles = tuple(x[0] for x in cursor.description)
    #        self._values = values

    def __len__(self):
        return len(self._values)

    def __iter__(self):
        """Dict.items() equivalent."""
        # print("__iter__")
        return iter(zip(self._titles, self._values))

    def __getitem__(self, key):
        """Sequence or Mapping lookup."""
        # print(f"__getitem__[{repr(key)}]")
        # return self._values[self._titles.index(key)]
        return self._datum[key]

    def __getattr__(self, name):
        try:
            # print(f"__getattr__[{repr(name)}]")
            # return self._values[self._titles.index(name)]
            return self._datum[name]
        except ValueError:
            # print(f"__getattr__[{repr(name)}] raise AttributeError")
            raise AttributeError(name)

    def __contains__(self, key):
        """True if Row has the specified key, else False."""
        return key in self._titles

    def __repr__(self):
        return "<NamedRow {0} {1}>".format(repr(self._titles), id(self))
