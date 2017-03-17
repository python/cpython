Issue #27832: Make ``_normalize`` parameter to ``Fraction`` constuctor
keyword-only, so that ``Fraction(2, 3, 4)`` now raises ``TypeError``.