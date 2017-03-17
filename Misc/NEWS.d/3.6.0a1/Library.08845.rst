Issue #25406: Fixed a bug in C implementation of OrderedDict.move_to_end()
that caused segmentation fault or hang in iterating after moving several
items to the start of ordered dict.