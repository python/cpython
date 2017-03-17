- Issue #22558: Add remaining doc links to source code for Python-coded modules.
  Patch by Yoni Lavi.

- Issue #12067: Rewrite Comparisons section in the Expressions chapter of the
  language reference. Some of the details of comparing mixed types were
  incorrect or ambiguous. NotImplemented is only relevant at a lower level
  than the Expressions chapter. Added details of comparing range() objects,
  and default behaviour and consistency suggestions for user-defined classes.
  Patch from Andy Maier.

- Issue #24952: Clarify the default size argument of stack_size() in
  the "threading" and "_thread" modules. Patch from Mattip.

- Issue #23725: Overhaul tempfile docs. Note deprecated status of mktemp.
  Patch from Zbigniew Jędrzejewski-Szmek.

- Issue #24808: Update the types of some PyTypeObject fields.  Patch by
  Joseph Weston.

- Issue #22812: Fix unittest discovery examples.
  Patch from Pam McA'Nulty.

