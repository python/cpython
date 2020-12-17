#!/usr/bin/env python3
"""Script that generates the ctype.h-replacement in stringobject.c."""

NAMES = ("LOWER", "UPPER", "ALPHA", "DIGIT", "XDIGIT", "ALNUM", "SPACE")

print("""
#define FLAG_LOWER  0x01
#define FLAG_UPPER  0x02
#define FLAG_ALPHA  (FLAG_LOWER|FLAG_UPPER)
#define FLAG_DIGIT  0x04
#define FLAG_ALNUM  (FLAG_ALPHA|FLAG_DIGIT)
#define FLAG_SPACE  0x08
#define FLAG_XDIGIT 0x10

static unsigned int ctype_table[256] = {""")

for i in range(128):
    c = chr(i)
    flags = []
    for name in NAMES:
        if name in ("ALPHA", "ALNUM"):
            continue
        if name == "XDIGIT":
            method = lambda: c.isdigit() or c.upper() in "ABCDEF"
        else:
            method = getattr(c, "is" + name.lower())
        if method():
            flags.append("FLAG_" + name)
    rc = repr(c)
    if c == '\v':
        rc = "'\\v'"
    elif c == '\f':
        rc = "'\\f'"
    if not flags:
        print("    0, /* 0x%x %s */" % (i, rc))
    else:
        print("    %s, /* 0x%x %s */" % ("|".join(flags), i, rc))

for i in range(128, 256, 16):
    print("    %s," % ", ".join(16*["0"]))

print("};")
print("")

for name in NAMES:
    print("#define IS%s(c) (ctype_table[Py_CHARMASK(c)] & FLAG_%s)" %
          (name, name))

print("")

for name in NAMES:
    name = "is" + name.lower()
    print("#undef %s" % name)
    print("#define %s(c) undefined_%s(c)" % (name, name))

print("""
static unsigned char ctype_tolower[256] = {""")

for i in range(0, 256, 8):
    values = []
    for i in range(i, i+8):
        if i < 128:
            c = chr(i)
            if c.isupper():
                i = ord(c.lower())
        values.append("0x%02x" % i)
    print("    %s," % ", ".join(values))

print("};")

print("""
static unsigned char ctype_toupper[256] = {""")

for i in range(0, 256, 8):
    values = []
    for i in range(i, i+8):
        if i < 128:
            c = chr(i)
            if c.islower():
                i = ord(c.upper())
        values.append("0x%02x" % i)
    print("    %s," % ", ".join(values))

print("};")

print("""
#define TOLOWER(c) (ctype_tolower[Py_CHARMASK(c)])
#define TOUPPER(c) (ctype_toupper[Py_CHARMASK(c)])

#undef tolower
#define tolower(c) undefined_tolower(c)
#undef toupper
#define toupper(c) undefined_toupper(c)
""")
