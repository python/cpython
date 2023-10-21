"""
Module doctest

>>> raise_with_note()
Traceback (most recent call last):
  ...
ValueError: Text
Note
"""

def raise_with_note():
    err = ValueError('Text')
    err.add_note('Note')
    raise err
