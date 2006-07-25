"""
Broken bytecode objects can easily crash the interpreter.

This is not going to be fixed.  It is generally agreed that there is no
point in writing a bytecode verifier and putting it in CPython just for
this.  Moreover, a verifier is bound to accept only a subset of all safe
bytecodes, so it could lead to unnecessary breakage.

For security purposes, "restricted" interpreters are not going to let
the user build or load random bytecodes anyway.  Otherwise, this is a
"won't fix" case.

"""

import types

co = types.CodeType(0, 0, 0, 0, '\x04\x71\x00\x00', (),
                    (), (), '', '', 1, '')
exec co
