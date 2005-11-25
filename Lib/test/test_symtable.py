from test.test_support import vereq, TestFailed

import symtable

symbols = symtable.symtable("def f(x): return x", "?", "exec")

## XXX
## Test disabled because symtable module needs to be rewritten for new compiler

##vereq(symbols[0].name, "global")
##vereq(len([ste for ste in symbols.values() if ste.name == "f"]), 1)

### Bug tickler: SyntaxError file name correct whether error raised
### while parsing or building symbol table.
##def checkfilename(brokencode):
##    try:
##        _symtable.symtable(brokencode, "spam", "exec")
##    except SyntaxError, e:
##        vereq(e.filename, "spam")
##    else:
##        raise TestFailed("no SyntaxError for %r" % (brokencode,))
##checkfilename("def f(x): foo)(")  # parse-time
##checkfilename("def f(x): global x")  # symtable-build-time
