"boolcheck - import this module to ensure True, False, bool() builtins exist."
try:
    True
except NameError:
    import __builtin__
    __builtin__.True = 1
    __builtin__.False = 0
    from operator import truth
    __builtin__.bool = truth
