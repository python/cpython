"""
The compiler (>= 2.5) recurses happily until it blows the stack.

Recorded on the tracker as http://bugs.python.org/issue11383
"""

# The variant below blows up in compiler_call, but there are assorted
# other variations that blow up in other functions
# e.g. '1*'*10**5+'1' will die in compiler_visit_expr

# The exact limit to destroy the stack will vary by platform
# but 10M should do the trick even with huge stack allocations
compile('()'*10**7, '?', 'exec')
