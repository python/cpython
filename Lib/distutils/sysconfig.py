"""Prototype sysconfig module that loads information when run as a script,
but only defines constants when imported.

This should be run as a script as one of the last steps of the Python
installation process.

Written by:   Fred L. Drake, Jr.
Email:        <fdrake@acm.org>
Initial date: 17-Dec-1998
"""

__version__ = "$Revision$"


def _init_posix():
    import os
    import re
    import string
    import sys

    g = globals()

    version = sys.version[:3]
    config_dir = os.path.join(
        sys.exec_prefix, "lib", "python" + version, "config")

    # load the installed config.h:
    define_rx = re.compile("#define ([A-Z][A-Z0-9_]+) (.*)\n")
    undef_rx = re.compile("/[*] #undef ([A-Z][A-Z0-9_]+) [*]/\n")
    fp = open(os.path.join(config_dir, "config.h"))

    while 1:
        line = fp.readline()
        if not line:
            break
        m = define_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            try: v = string.atoi(v)
            except ValueError: pass
            g[n] = v
        else:
            m = undef_rx.match(line)
            if m:
                g[m.group(1)] = 0

    # load the installed Makefile.pre.in:
    variable_rx = re.compile("([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)\n")
    done = {}
    notdone = {}
    fp = open(os.path.join(config_dir, "Makefile"))

    while 1:
        line = fp.readline()
        if not line:
            break
        m = variable_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            v = string.strip(v)
            if "$" in v:
                notdone[n] = v
            else:
                try: v = string.atoi(v)
                except ValueError: pass
                done[n] = v

    # do variable interpolation here
    findvar1_rx = re.compile(r"\$\(([A-Za-z][A-Za-z0-9_]*)\)")
    findvar2_rx = re.compile(r"\${([A-Za-z][A-Za-z0-9_]*)}")
    while notdone:
        for name in notdone.keys():
            value = notdone[name]
            m = findvar1_rx.search(value)
            if not m:
                m = findvar2_rx.search(value)
            if m:
                n = m.group(1)
                if done.has_key(n):
                    after = value[m.end():]
                    value = value[:m.start()] + done[n] + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = string.atoi(value)
                        except ValueError: pass
                        done[name] = string.strip(value)
                        del notdone[name]
                elif notdone.has_key(n):
                    # get it on a subsequent round
                    pass
                else:
                    done[n] = ""
                    after = value[m.end():]
                    value = value[:m.start()] + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = string.atoi(value)
                        except ValueError: pass
                        done[name] = string.strip(value)
                        del notdone[name]
            else:
                # bogus variable reference; just drop it since we can't deal
                del notdone[name]

    # save the results in the global dictionary
    g.update(done)


import os
exec "_init_%s()" % os.name
del os
del _init_posix
