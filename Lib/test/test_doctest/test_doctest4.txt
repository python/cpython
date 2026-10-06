This is a sample doctest in a text file that contains non-ASCII characters.
This file is encoded using UTF-8.

In order to get this test to pass, we have to manually specify the
encoding.

  >>> 'föö'
  'f\xf6\xf6'

  >>> 'bąr'
  'b\u0105r'
