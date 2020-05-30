import os


# Filename used for testing
if os.name == 'java':
    # Jython disallows @ in module names
    TESTFN = '$test'
else:
    TESTFN = '@test'

# Disambiguate TESTFN for parallel testing, while letting it remain a valid
# module name.
TESTFN = "{}_{}_tmp".format(TESTFN, os.getpid())
