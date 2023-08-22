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

from test.support import SuppressCrashReport

def func():
    pass

invalid_code = b'\x04\x00\x71\x00'
func.__code__ = func.__code__.replace(co_code=invalid_code)

with SuppressCrashReport():
    func()
