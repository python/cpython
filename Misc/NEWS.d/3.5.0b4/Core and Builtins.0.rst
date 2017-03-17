- Issue #23573: Restored optimization of bytes.rfind() and bytearray.rfind()
  for single-byte argument on Linux.

- Issue #24569: Make PEP 448 dictionary evaluation more consistent.

- Issue #24583: Fix crash when set is mutated while being updated.

- Issue #24407: Fix crash when dict is mutated while being updated.

- Issue #24619: New approach for tokenizing async/await. As a consequence,
  it is now possible to have one-line 'async def foo(): await ..' functions.

- Issue #24687: Plug refleak on SyntaxError in function parameters
  annotations.

- Issue #15944: memoryview: Allow arbitrary formats when casting to bytes.
  Patch by Martin Panter.

