'''A module to test if line numbers are calculated for doctests
This test will be located properly because it in on module level.
>>> # not unique
>>> x = 7
'''

__test__ = {
'q': '''
>>> # not unique
>>> x = 7
>>> x/0
''',
'r': '''
>>> # this one will be found because there is a unique line in it
>>> x = 8
>>> x/0
''',
}

if __name__ == '__main__':
    import doctest
    doctest.testmod()
