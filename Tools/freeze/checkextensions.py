# Check for a module in a set of extension directories.
# An extension directory should contain a Setup file
# and one or more .o files or a lib.a file.

import os
import parsesetup
import re

def checkextensions(unknown, extensions):
    files = []
    modules = []
    edict = {}
    for e in extensions:
        setup = os.path.join(e, 'Setup')
        liba = os.path.join(e, 'lib.a')
        if not os.path.isfile(liba):
            liba = None
        edict[e] = parsesetup.getsetupinfo(setup), liba
    for mod in unknown:
        for e in extensions:
            (mods, vars), liba = edict[e]
            if mod not in mods:
                continue
            modules.append(mod)
            if liba:
                # If we find a lib.a, use it, ignore the
                # .o files, and use *all* libraries for
                # *all* modules in the Setup file
                if liba in files:
                    break
                files.append(liba)
                for m in list(mods.keys()):
                    files = files + select(e, mods, vars,
                                           m, 1)
                break
            files = files + select(e, mods, vars, mod, 0)
            break
    return files, modules

def select(e, mods, vars, mod, skipofiles):
    files = []
    for w in mods[mod]:
        w = treatword(w)
        if not w:
            continue
        w = expandvars(w, vars)
        for w in w.split():
            if skipofiles and w[-2:] == '.o':
                continue
            # Assume $var expands to absolute pathname
            if w[0] not in ('-', '$') and w[-2:] in ('.o', '.a'):
                w = os.path.join(e, w)
            if w[:2] in ('-L', '-R') and w[2:3] != '$':
                w = w[:2] + os.path.join(e, w[2:])
            files.append(w)
    return files

cc_flags = ['-I', '-D', '-U']
cc_exts = ['.c', '.C', '.cc', '.c++']

def treatword(w):
    if w[:2] in cc_flags:
        return None
    if w[:1] == '-':
        return w # Assume loader flag
    head, tail = os.path.split(w)
    base, ext = os.path.splitext(tail)
    if ext in cc_exts:
        tail = base + '.o'
        w = os.path.join(head, tail)
    return w

def expandvars(str, vars):
    def repl(m):
        name = m[1]
        if name[0] == '(':
            if name[-1] != ')':
                return m[0]
            name = name[1:-1]

        if name in resolved:
            result = resolved[name]
            if result is not None:
                return result
        elif name in vars:
            resolved[name] = None
            result = resolved[name] = expandvars(vars[name])
            return result
        return m[0]

    def expandvars(str):
        return re.sub(r'(?m)\$(\([^)]*\)?|.)', repl, str)

    resolved = {}
    return expandvars(str)
