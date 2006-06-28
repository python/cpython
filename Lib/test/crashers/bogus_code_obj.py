"""
Broken bytecode objects can easily crash the interpreter.
"""

import types

co = types.CodeType(0, 0, 0, 0, '\x04\x71\x00\x00', (),
                    (), (), '', '', 1, '')
exec co
