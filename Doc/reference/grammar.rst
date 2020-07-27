Full Grammar specification
==========================

This is the full Python grammar, derived directly from the grammar
used to generate the CPython parser (see :source:`Grammar/python.gram`).
The version here omits details related to code generation and
error recovery.

The notation is a mixture of `EBNF
<https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form>`_
and `PEG <https://en.wikipedia.org/wiki/Parsing_expression_grammar>`_.
In particular, ``&`` followed by a symbol, token or parenthesized
group indicates a positive lookahead (i.e., is required to match but
not consumed), while ``!`` indicates a negative lookahead (i.e., is
required _not_ to match).  We use the ``|`` separator to mean PEG's
"ordered choice" (written as ``/`` in traditional PEG grammars).

.. literalinclude:: ../../Grammar/python.gram
  :language: peg
