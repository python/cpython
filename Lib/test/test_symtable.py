from test_support import verify

import _symtable

symbols = _symtable.symtable("def f(x): return x", "?", "exec")

verify(symbols[0].name == "global")
verify(len([ste for ste in symbols.values() if ste.name == "f"]) == 1)
