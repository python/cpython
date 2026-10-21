"""Type slots ("tp slots") for which Argument Clinic generates the slot function.

A slot function does not parse arguments: Argument Clinic generates a thin
wrapper with the C signature of the slot, which forwards its arguments to
the implementation function.  The wrapper is the place where
``@critical_section`` acquires the lock.

The parameters declared in the DSL only give the C signature of the slot;
they must be positional-only, without a default value.
"""
from __future__ import annotations
import dataclasses as dc
from typing import Final

from libclinic.return_converters import CReturnConverter, return_converters


@dc.dataclass(frozen=True, slots=True, kw_only=True)
class SlotSpec:
    # The name of the return converter of the implementation function,
    # which declares the C return type of the slot function.
    return_converter: str

    @property
    def return_type(self) -> str:
        """The C return type of the slot function."""
        return self.make_return_converter().type

    @property
    def error_value(self) -> str:
        """The value returned by the slot function on error."""
        return 'NULL' if self.return_type.endswith('*') else '-1'

    def make_return_converter(self) -> CReturnConverter:
        return return_converters[self.return_converter]()


# Maps the name of a special method to its slot.  A name listed here must
# not be in dsl_parser.unsupported_special_methods.
SLOTS: Final[dict[str, SlotSpec]] = {
    '__next__': SlotSpec(return_converter='object'),
    '__len__': SlotSpec(return_converter='Py_ssize_t'),
    '__bool__': SlotSpec(return_converter='int'),
}
