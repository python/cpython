#
# Secret Labs' Regular Expression Engine
# $Id$
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

# flags
I = IGNORECASE = sre_compile.SRE_FLAG_IGNORECASE
L = LOCALE = sre_compile.SRE_FLAG_LOCALE
M = MULTILINE = sre_compile.SRE_FLAG_MULTILINE
S = DOTALL = sre_compile.SRE_FLAG_DOTALL
X = VERBOSE = sre_compile.SRE_FLAG_VERBOSE

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

def _sub(pattern, template, string, count=0):
    # internal: pattern.sub implementation hook
    return _subn(pattern, template, string, count)[0]

def _expand(match, template):
    # internal: expand template
    return template # FIXME

def _subn(pattern, template, string, count=0):
    # internal: pattern.subn implementation hook
    if callable(template):
        filter = template
    else:
        # FIXME: prepare template
        def filter(match, template=template):
            return _expand(match, template)
    n = i = 0
    s = []
    append = s.append
    c = pattern.cursor(string)
    while not count or n < count:
        m = c.search()
        if not m:
            break
        j = m.start()
        if j > i:
            append(string[i:j])
        append(filter(m))
        i = m.end()
        n = n + 1
    if i < len(string):
        append(string[i:])
    return string[:0].join(s), n

def _split(pattern, string, maxsplit=0):
    # internal: pattern.split implementation hook
    n = i = 0
    s = []
    append = s.append
    c = pattern.cursor(string)
    while not maxsplit or n < maxsplit:
        m = c.search()
        if not m:
            break
        j = m.start()
        append(string[i:j])
        i = m.end()
        n = n + 1
    if i < len(string):
        append(string[i:])
    return s
