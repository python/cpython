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
    # The C return type of the slot function.
    return_type: str
    # The name of the return converter which declares `return_type`.
    return_converter: str
    # Maps the converter classes of the parameters (excluding self) to
    # the C slot.
    signatures: dict[tuple[type[CConverter], ...], str]

    @property
    def error_value(self) -> str:
        """The value returned by the slot function on error."""
        return 'NULL' if self.return_type.endswith('*') else '-1'

    @property
    def takes_parameters(self) -> bool:
        return any(self.signatures)

    def c_slot(self, converters: list[CConverter]) -> str | None:
        """Return the C slot for the parameter converters, or None."""
        classes = tuple(type(c) for c in converters)
        return self.signatures.get(classes)

    def describe_signatures(self) -> str:
        return " or ".join(
            f"({', '.join(c.dsl_name() for c in sig) or 'no parameters'})"
            f" for {slot}"
            for sig, slot in self.signatures.items())


# Maps the name of a special method to its slot.  A name listed here must
# not be in dsl_parser.unsupported_special_methods.
SLOTS: Final[dict[str, SlotSpec]] = {
    '__next__': SlotSpec('__next__', 'PyObject *', 'object', {
        (): 'tp_iternext',
    }),
}
