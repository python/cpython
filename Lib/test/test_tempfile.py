# SF bug #476138: tempfile behavior across platforms
# Ensure that a temp file can be closed any number of times without error.

import tempfile

f = tempfile.TemporaryFile("w+b")
f.write('abc\n')
f.close()
f.close()
f.close()
