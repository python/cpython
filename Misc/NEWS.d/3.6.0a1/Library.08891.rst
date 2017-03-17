Issue #25873: Optimized iterating ElementTree.  Iterating elements
Element.iter() is now 40% faster, iterating text Element.itertext()
is now up to 2.5 times faster.