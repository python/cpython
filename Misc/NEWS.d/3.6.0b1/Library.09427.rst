Issue #22493: Inline flags now should be used only at the start of the
regular expression.  Deprecation warning is emitted if uses them in the
middle of the regular expression.