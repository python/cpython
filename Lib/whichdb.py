"""Guess which db package to use to open a db file."""

import struct

def whichdb(filename):
    """Guess which db package to use to open a db file.

    Return values:

    - None if the database file can't be read;
    - empty string if the file can be read but can't be recognized
    - the module name (e.g. "dbm" or "gdbm") if recognized.

    Importing the given module may still fail, and opening the
    database using that module may still fail.
    """

    # Check for dbm first -- this has a .pag and a .dir file
    try:
        f = open(filename + ".pag", "rb")
        f.close()
        f = open(filename + ".dir", "rb")
        f.close()
        return "dbm"
    except IOError:
        pass

    # Check for dumbdbm next -- this has a .dir and and a .dat file
    try:
        f = open(filename + ".dat", "rb")
        f.close()
        f = open(filename + ".dir", "rb")
        try:
            if f.read(1) in ["'", '"']:
                return "dumbdbm"
        finally:
            f.close()
    except IOError:
        pass

    # See if the file exists, return None if not
    try:
        f = open(filename, "rb")
    except IOError:
        return None

    # Read the start of the file -- the magic number
    s16 = f.read(16)
    f.close()
    s = s16[0:4]

    # Return "" if not at least 4 bytes
    if len(s) != 4:
        return ""

    # Convert to 4-byte int in native byte order -- return "" if impossible
    try:
        (magic,) = struct.unpack("=l", s)
    except struct.error:
        return ""

    # Check for GNU dbm
    if magic == 0x13579ace:
        return "gdbm"

    # Check for BSD hash
    if magic in (0x00061561, 0x61150600):
        return "dbhash"

    # BSD hash v2 has a 12-byte NULL pad in front of the file type
    try:
        (magic,) = struct.unpack("=l", s16[-4:])
    except struct.error:
        return ""

    # Check for BSD hash
    if magic in (0x00061561, 0x61150600):
        return "dbhash"

    # Unknown
    return ""
