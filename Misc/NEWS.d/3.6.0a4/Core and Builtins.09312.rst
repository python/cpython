Issue #27704: Optimized creating bytes and bytearray from byte-like objects
and iterables.  Speed up to 3 times for short objects.  Original patch by
Naoki Inada.