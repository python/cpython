import dataclasses as dc


TemplateDict = dict[str, str]


class CRenderData:
    def __init__(self) -> None:

        # The C statements to declare variables.
        # Should be full lines with \n eol characters.
        self.declarations: list[str] = []

        # The C statements required to initialize the variables before the parse call.
        # Should be full lines with \n eol characters.
        self.initializers: list[str] = []

        # The C statements needed to dynamically modify the values
        # parsed by the parse call, before calling the impl.
        self.modifications: list[str] = []

        # The entries for the "keywords" array for PyArg_ParseTuple.
        # Should be individual strings representing the names.
        self.keywords: list[str] = []

        # The "format units" for PyArg_ParseTuple.
        # Should be individual strings that will get
        self.format_units: list[str] = []

        # The varargs arguments for PyArg_ParseTuple.
        self.parse_arguments: list[str] = []

        # The parameter declarations for the impl function.
        self.impl_parameters: list[str] = []

        # The arguments to the impl function at the time it's called.
        self.impl_arguments: list[str] = []

        # For return converters: the name of the variable that
        # should receive the value returned by the impl.
        self.return_value = "return_value"

        # For return converters: the code to convert the return
        # value from the parse function.  This is also where
        # you should check the _return_value for errors, and
        # "goto exit" if there are any.
        self.return_conversion: list[str] = []
        self.converter_retval = "_return_value"

        # The C statements required to do some operations
        # after the end of parsing but before cleaning up.
        # These operations may be, for example, memory deallocations which
        # can only be done without any error happening during argument parsing.
        self.post_parsing: list[str] = []

        # The C statements required to clean up after the impl call.
        self.cleanup: list[str] = []

        # The C statements to generate critical sections (per-object locking).
        self.lock: list[str] = []
        self.unlock: list[str] = []


@dc.dataclass(slots=True, frozen=True)
class Include:
    """
    An include like: #include "pycore_long.h"   // _Py_ID()
    """
    # Example: "pycore_long.h".
    filename: str

    # Example: "_Py_ID()".
    reason: str

    # None means unconditional include.
    # Example: "#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)".
    condition: str | None

    def sort_key(self) -> tuple[str, str]:
        # order: '#if' comes before 'NO_CONDITION'
        return (self.condition or 'NO_CONDITION', self.filename)
