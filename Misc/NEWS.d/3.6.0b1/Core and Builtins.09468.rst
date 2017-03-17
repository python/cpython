Issue #25856: The __module__ attribute of extension classes and functions
now is interned. This leads to more compact pickle data with protocol 4.