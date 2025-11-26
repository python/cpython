The subscript operator (``obj[key]``) now falls back to an attribute-based
lookup of ``__getitem__`` when the type does not define a mapping or
sequence slot. This lookup follows normal attribute resolution rules,
including ``__getattribute__`` and ``__getattr__``. This allows classes
that provide ``__getitem__`` purely as an attribute or through dynamic
attribute handlers to support subscripting.
