Issue #27319: Methods selection_set(), selection_add(), selection_remove()
and selection_toggle() of ttk.TreeView now allow passing multiple items as
multiple arguments instead of passing them as a tuple.  Deprecated
undocumented ability of calling the selection() method with arguments.