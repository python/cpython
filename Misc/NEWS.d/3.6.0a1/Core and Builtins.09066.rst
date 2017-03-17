Issue #22995: Instances of extension types with a state that aren't
subclasses of list or dict and haven't implemented any pickle-related
methods (__reduce__, __reduce_ex__, __getnewargs__, __getnewargs_ex__,
or __getstate__), can no longer be pickled.  Including memoryview.