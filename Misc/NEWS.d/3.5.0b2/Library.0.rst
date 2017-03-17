- Issue #23247: Fix a crash in the StreamWriter.reset() of CJK codecs.

- Issue #24270: Add math.isclose() and cmath.isclose() functions as per PEP 485.
  Contributed by Chris Barker and Tal Einat.

- Issue #5633: Fixed timeit when the statement is a string and the setup is not.

- Issue #24326: Fixed audioop.ratecv() with non-default weightB argument.
  Original patch by David Moore.

- Issue #16991: Add a C implementation of OrderedDict.

- Issue #23934: Fix inspect.signature to fail correctly for builtin types
  lacking signature information.  Initial patch by James Powell.

