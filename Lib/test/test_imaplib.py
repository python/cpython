import imaplib
import time

# We can check only that it successfully produces a result,
# not the correctness of the result itself, since the result
# depends on the timezone the machine is in.

timevalues = [2000000000, 2000000000.0, time.localtime(2000000000),
              '"18-May-2033 05:33:20 +0200"']

for t in timevalues:
    imaplib.Time2Internaldate(t)
