"""Type slots ("tp slots") for which Argument Clinic generates the slot function.

A slot function does not parse arguments: Argument Clinic generates a thin
wrapper with the C signature of the slot, which forwards its arguments to
the implementation function.  The wrapper is the place where
``@critical_section`` acquires the lock.

The parameters declared in the DSL only select the C signature; they must be
positional-only, without a default value. When a special method maps to
several slots (``__getitem__`` is ``sq_item`` or ``mp_subscript``),
the converters of its parameters determine which one it implements.
"""
from __future__ import annotations
import dataclasses as dc
from typing import Final

from libclinic.converter import CConverter


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
    # The converter classes of the parameters, excluding self.
    parameters: tuple[type[CConverter], ...] = ()

    @property
    def error_value(self) -> str:
        """The value returned by the slot function on error."""
        return 'NULL' if self.return_type.endswith('*') else '-1'

    def matches(self, converters: list[CConverter]) -> bool:
        """Return whether the parameter converters are those of the slot."""
        return tuple(type(c) for c in converters) == self.parameters

    def describe(self) -> str:
        params = ', '.join(c.dsl_name() for c in self.parameters)
        return f"({params or 'no parameters'}) for {self.c_slot}"


# Maps the name of a special method to the slots it can implement.  A name
# listed here must not be in dsl_parser.unsupported_special_methods.
SLOTS: Final[dict[str, tuple[SlotSpec, ...]]] = {
    '__next__': (
        SlotSpec('__next__', 'tp_iternext', 'PyObject *', 'object'),
    ),
    '__len__': (
        SlotSpec('__len__', 'sq_length', 'Py_ssize_t', 'Py_ssize_t'),
    ),
}
