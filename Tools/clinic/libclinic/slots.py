"""Type slots ("tp slots") for which Argument Clinic generates the slot function.

A slot function does not parse arguments: Argument Clinic generates a thin
wrapper with the C signature of the slot, which forwards its arguments to
the implementation function.  The wrapper is the place where
``@critical_section`` acquires the lock.
"""
from __future__ import annotations
import dataclasses as dc
from typing import Final


@dc.dataclass(frozen=True, slots=True)
class SlotSpec:
    # The special method name, e.g. '__next__'.
    name: str
    # The C slot which the function implements, e.g. 'tp_iternext'.
    c_slot: str
    # The C return type of the slot function.
    return_type: str
    # The name of the return converter which declares `return_type`.
    return_converter: str


# Maps the name of a special method to its slot.  Only slots without
# parameters other than `self` are supported for now.
SLOTS: Final[dict[str, SlotSpec]] = {
    '__next__': SlotSpec('__next__', 'tp_iternext', 'PyObject *', 'object'),
}
