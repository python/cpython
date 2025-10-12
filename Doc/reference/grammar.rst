.. _full-grammar-specification:

Full Grammar specification
==========================

This is the full Python grammar, derived directly from the grammar
used to generate the CPython parser (see :source:`Grammar/python.gram`).
The version here omits details related to code generation and
error recovery.

The notation used here is the same as in the preceding docs,
and is described in the :ref:`notation <notation>` section,
except for an extra complication:

* ``~`` ("cut"): commit to the current alternative and fail the rule
  even if this fails to parse

.. literalinclude:: ../../Grammar/python.gram
  :language: peg
