Issue #27419: Standard __import__() no longer look up "__import__" in globals
or builtins for importing submodules or "from import".  Fixed a crash if
raise a warning about unabling to resolve package from __spec__ or
__package__.