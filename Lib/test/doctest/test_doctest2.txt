This is a sample doctest in a text file.

In this example, we'll rely on some silly setup:

  >>> import test.test_doctest
  >>> test.test_doctest.sillySetup
  True

This test also has some (random) encoded (utf-8) unicode text:

  ï»¿ÐÐÐÐÐ

This doesn't cause a problem in the tect surrounding the examples, but
we include it here (in this test text file) to make sure. :)
