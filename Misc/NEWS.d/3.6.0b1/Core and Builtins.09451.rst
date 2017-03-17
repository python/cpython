Issue #27214: In long_invert, be more careful about modifying object
returned by long_add, and remove an unnecessary check for small longs.
Thanks Oren Milman for analysis and patch.