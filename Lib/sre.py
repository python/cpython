#
# Secret Labs' Regular Expression Engine
#
# re-compatible interface for the sre matching engine
#
# Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

import sre_compile
import sre_parse

# flags
I = IGNORECASE = sre_compile.SRE_FLAG_IGNORECASE
L = LOCALE = sre_compile.SRE_FLAG_LOCALE
M = MULTILINE = sre_compile.SRE_FLAG_MULTILINE
S = DOTALL = sre_compile.SRE_FLAG_DOTALL
X = VERBOSE = sre_compile.SRE_FLAG_VERBOSE

# sre extensions (may or may not be in 2.0 final)
T = TEMPLATE = sre_compile.SRE_FLAG_TEMPLATE
U = UNICODE = sre_compile.SRE_FLAG_UNICODE

# sre exception
error = sre_compile.error

# --------------------------------------------------------------------
# public interface

# FIXME: add docstrings

def match(pattern, string, flags=0):
    return _compile(pattern, flags).match(string)

def search(pattern, string, flags=0):
    return _compile(pattern, flags).search(string)

def sub(pattern, repl, string, count=0):
    return _compile(pattern).sub(repl, string, count)

def subn(pattern, repl, string, count=0):
    return _compile(pattern).subn(repl, string, count)

def split(pattern, string, maxsplit=0):
    return _compile(pattern).split(string, maxsplit)

def findall(pattern, string, maxsplit=0):
    return _compile(pattern).findall(string, maxsplit)

def compile(pattern, flags=0):
    return _compile(pattern, flags)

def template(pattern, flags=0):
    return _compile(pattern, flags|T)

def escape(pattern):
    s = list(pattern)
    for i in range(len(pattern)):
        c = pattern[i]
        if not ("a" <= c <= "z" or "A" <= c <= "Z" or "0" <= c <= "9"):
            if c == "\000":
                s[i] = "\\000"
            else:
                s[i] = "\\" + c
    return pattern[:0].join(s)

# --------------------------------------------------------------------
# internals

_cache = {}
_MAXCACHE = 100

def _compile(pattern, flags=0):
    # internal: compile pattern
    tp = type(pattern)
    if tp not in (type(""), type(u"")):
        return pattern
    key = (tp, pattern, flags)
    try:
        return _cache[key]
    except KeyError:
        pass
    p = sre_compile.compile(pattern, flags)
    if len(_cache) >= _MAXCACHE:
        _cache.clear()
    _cache[key] = p
    return p

def purge():
    # clear pattern cache
    _cache.clear()

def _sub(pattern, template, string, count=0):
    # internal: pattern.sub implementation hook
    return _subn(pattern, template, string, count)[0]

def _subn(pattern, template, string, count=0):
    # internal: pattern.subn implementation hook
    if callable(template):
        filter = template
    else:
        template = sre_parse.parse_template(template, pattern)
        def filter(match, template=template):
            return sre_parse.expand_template(template, match)
    n = i = 0
    s = []
    append = s.append
    c = pattern.scanner(string)
    while not count or n < count:
        m = c.search()
        if not m:
            break
        b, e = m.span()
        if i < b:
            append(string[i:b])
        append(filter(m))
        i = e
        n = n + 1
    append(string[i:])
    return string[:0].join(s), n

def _split(pattern, string, maxsplit=0):
    # internal: pattern.split implementation hook
    n = i = 0
    s = []
    append = s.append
    extend = s.extend
    c = pattern.scanner(string)
    g = pattern.groups
    while not maxsplit or n < maxsplit:
        m = c.search()
        if not m:
            break
        b, e = m.span()
        if b == e:
            if i >= len(string):
                break
            continue
        append(string[i:b])
        if g and b != e:
            extend(m.groups())
        i = e
        n = n + 1
    append(string[i:])
    return s

# register myself for pickling

import copy_reg

def _pickle(p):
    return _compile, (p.pattern, p.flags)

copy_reg.pickle(type(_compile("")), _pickle, _compile)
