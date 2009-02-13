"""
The compiler (>= 2.5) recurses happily.
"""

compile('()'*9**5, '?', 'exec')
