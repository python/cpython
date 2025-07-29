This is a sample doctest in a text file.

In this example, we'll rely on a global variable being set for us
already:

  >>> favorite_color
  'blue'

We can make this fail by disabling the blank-line feature.

  >>> if 1:
  ...    print('a')
  ...    print()
  ...    print('b')
  a
  <BLANKLINE>
  b
