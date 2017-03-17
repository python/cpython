Issue #26530: Add C functions :c:func:`_PyTraceMalloc_Track` and
:c:func:`_PyTraceMalloc_Untrack` to track memory blocks using the
:mod:`tracemalloc` module. Add :c:func:`_PyTraceMalloc_GetTraceback` to get
the traceback of an object.