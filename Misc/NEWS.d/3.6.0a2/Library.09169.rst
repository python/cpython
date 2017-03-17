Issue #27030: Unknown escapes consisting of ``'\'`` and an ASCII letter in
regular expressions now are errors.  The re.LOCALE flag now can be used
only with bytes patterns.