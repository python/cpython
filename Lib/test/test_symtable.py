from test import test_support

import symtable
import unittest


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

class SymtableTest(unittest.TestCase):
    def test_invalid_args(self):
        self.assertRaises(TypeError, symtable.symtable, "42")
        self.assertRaises(ValueError, symtable.symtable, "42", "?", "")

    def test_eval(self):
        symbols = symtable.symtable("42", "?", "eval")

    def test_single(self):
        symbols = symtable.symtable("42", "?", "single")

    def test_exec(self):
        symbols = symtable.symtable("def f(x): return x", "?", "exec")


def test_main():
    test_support.run_unittest(SymtableTest)

if __name__ == '__main__':
    test_main()
