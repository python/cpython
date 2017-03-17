Issue #25945: Fixed a crash when unpickle the functools.partial object with
wrong state.  Fixed a leak in failed functools.partial constructor.
"args" and "keywords" attributes of functools.partial have now always types
tuple and dict correspondingly.