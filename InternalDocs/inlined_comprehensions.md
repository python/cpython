Inlined comprehensions
======================

Since [PEP 709](https://peps.python.org/pep-0709/), list, set, and dict
comprehensions are compiled into the enclosing compilation unit instead of
creating a nested function and calling it. Generator expressions are not
inlined; they still become their own code object.

The resulting bytecode lives in the enclosing unit, but the comprehension
still has its own locals: iteration variables must not leak into, or
overwrite, names in the enclosing scope. The symbol table models that as a
nested lexical scope; codegen then emits the comprehension in place.

Which comprehensions are inlined
--------------------------------

`symtable_handle_comprehension()` in
[`Python/symtable.c`](../Python/symtable.c) inlines a comprehension when it
is not a generator expression and the current block cannot see class scope
(`!ste_can_see_class_scope`). Annotation scopes that can see a class keep
the historical nested-function compilation so class-local names are not
treated as comprehension locals.

The outermost iterator expression is always evaluated in the enclosing
scope. The rest of the comprehension (targets, `if` clauses, inner
generators, and the element/value expressions) is visited in the
comprehension's own block.

Symbol table
------------

An inlined comprehension gets an `InlinedComprehensionBlock` entry
([`pycore_symtable.h`](../Include/internal/pycore_symtable.h)). That entry
is a child of the enclosing block, with `ste_parent` pointing at the
enclosing `PySTEntryObject`. It is a lexical subscope, not a compilation
unit: there is no separate code object, `co_consts` entry, or compiler
scope for it.

Uses and bindings inside the comprehension are recorded on that child
table. Because those loads are in the enclosing compilation unit, a
`FREE` use is also marked `USE` on the parent table. For
`def inner(): return [x for y in ()]`, both `inner.lookup("x")` and the
inlined child's lookup are `FREE|USE`.

### Analysis

`analyze_block()` records this block's declarations, analyzes children,
then classifies uses. `finalize_inlined_comprehension()` copies
`USE` from an inlined child onto the parent before that second pass, and
drops inlined-only frees so `analyze_cells()` does not promote those
names to cells.

A name that is `FREE` in the comprehension and bound in the parent is
dropped from the parent's free set unless:

* a real nested unit (function, lambda, or genexp) still needs it as a
  cell, or
* a sibling nested scope already marked it free.

That keeps iteration variables as fast locals when they are only used by
nested inlined comprehensions. A nested lambda that captures the name
still forces a cell on the binding comprehension.

Class-closure names (`__class__` and friends) loaded from an inlined
comprehension do not require a class cell unless a nested function,
lambda, or genexp captures them. Compile treats the inlined loads as
implicit globals. `is_free_in_any_child()` walks through inlined children
and only counts `FREE` on non-inlined descendants.

Compiler
--------

Codegen stays in the enclosing compiler unit. Around the inlined region,
`_PyCompile_EnterInlinedComprehensionScope()` /
`_PyCompile_ExitInlinedComprehensionScope()` in
[`Python/compile.c`](../Python/compile.c) swap `c->u->u_ste` so name
lookup uses the comprehension's symbol table. The saved `u_ste` is
restored on both success and error.

`_PyCompile_ResolveNameop()` calls `compiler_resolve_inlined_free()`,
which walks `ste_parent` while the current table is inlined and the name
is `FREE` or missing (scope `0`). Missing names include loads synthesized
by codegen, such as the implicit receiver for zero-argument `super()`.
The walk stops at a class: nested scopes do not see class locals.
Class-closure names that would otherwise be free through a class become
`GLOBAL_IMPLICIT`.

### Isolating iteration variables

`codegen_push_inlined_comprehension_locals()` in
[`Python/codegen.c`](../Python/codegen.c) isolates names bound in the
comprehension:

* `LOAD_FAST_AND_CLEAR` saves the enclosing value (possibly `NULL`) and
  clears the slot.
* `MAKE_CELL` runs if the name is a cell for this comprehension.
* In module and class units the name is added to `u_fasthidden` so
  assemble can set `CO_FAST_HIDDEN`.

A `SETUP_FINALLY` / `COMPILE_FBLOCK_INLINED_COMPREHENSION` handler
restores those slots if the comprehension raises, so an enclosing `except`
or `finally` sees the original values. 

Runtime
-------

An inlined comprehension cell can share a localsplus name with an
enclosing free variable (for example `[lambda: x for x in x]` inside a
nested function). `FrameLocalsProxy` keys, values, items, and `len`
keep the first slot of each name so they agree with `getitem`.

Source
------

* [`Python/symtable.c`](../Python/symtable.c):
  `symtable_handle_comprehension()`, `analyze_block()`,
  `finalize_inlined_comprehension()`, `is_free_in_any_child()`
* [`Python/compile.c`](../Python/compile.c):
  `compiler_resolve_inlined_free()`,
  `_PyCompile_EnterInlinedComprehensionScope()`,
  `compiler_cellvars()`
* [`Python/codegen.c`](../Python/codegen.c):
  `codegen_comprehension()`,
  `push_inlined_comprehension_state()`,
  `codegen_push_inlined_comprehension_locals()`
* [`Include/internal/pycore_symtable.h`](../Include/internal/pycore_symtable.h):
  `InlinedComprehensionBlock`
* [`Include/internal/pycore_compile.h`](../Include/internal/pycore_compile.h):
  `_PyCompile_InlinedComprehensionState`
* [`Objects/frameobject.c`](../Objects/frameobject.c):
  `FrameLocalsProxy` duplicate-name handling
