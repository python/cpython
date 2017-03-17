Issue #27792: The modulo operation applied to ``bool`` and other
``int`` subclasses now always returns an ``int``. Previously
the return type depended on the input values. Patch by Xiang Zhang.