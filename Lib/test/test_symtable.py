from test_support import verify

import _symtable

symbols, scopes = _symtable.symtable("def f(x): return x", "?", "exec")

verify(symbols.has_key(0))
verify(scopes.has_key(0))
