"""Rudimentary parser for C struct definitions."""

import re

PyObject_HEAD = "PyObject_HEAD"
PyObject_VAR_HEAD = "PyObject_VAR_HEAD"

rx_name = re.compile("} (\w+);")

class Struct:
    def __init__(self, name, head, members):
        self.name = name
        self.head = head
        self.members = members

    def get_type(self, name):
        for _name, type in self.members:
            if name == _name:
                return type
        raise ValueError, "no member named %s" % name

def parse(s):
    """Parse a C struct definition.

    The parser is very restricted in what it will accept.
    """

    lines = filter(None, s.split("\n")) # get non-empty lines
    assert lines[0].strip() == "typedef struct {"
    pyhead = lines[1].strip()
    assert (pyhead.startswith("PyObject") and
            pyhead.endswith("HEAD"))
    members = []
    for line in lines[2:]:
        line = line.strip()
        if line.startswith("}"):
            break

        assert line.endswith(";")
        line = line[:-1]
        words = line.split()
        name = words[-1]
        type = " ".join(words[:-1])
        if name[0] == "*":
            name = name[1:]
            type += " *"
        members.append((name, type))
    name = None
    mo = rx_name.search(line)
    assert mo is not None
    name = mo.group(1)
    return Struct(name, pyhead, members)
