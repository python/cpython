Issue #24903: Fix regression in number of arguments compileall accepts when
'-d' is specified.  The check on the number of arguments has been dropped
completely as it never worked correctly anyway.