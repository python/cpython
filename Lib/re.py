# change this to "pre" if your regexps stopped working.  don't
# forget to send a bug report to <some suitable address>

engine = "sre"

if engine == "sre":
    # new 2.0 engine
    from sre import *
else:
    # old 1.5.2 engine.  will be removed in 2.0 final.
    from pre import *
