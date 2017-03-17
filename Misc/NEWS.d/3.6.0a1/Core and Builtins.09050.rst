Issue #24726: Fixed a crash and leaking NULL in repr() of OrderedDict that
was mutated by direct calls of dict methods.