import imaplib
import time

timevalues = [2000000000, 2000000000.0, time.localtime(2000000000),
              '"18-May-2033 05:33:20 +0200"', '"18-May-2033 13:33:20 +1000"']

check = timevalues[2]

for t in timevalues:
    if check <> imaplib.Internaldate2tuple('INTERNALDATE ' + imaplib.Time2Internaldate(t)):
        print 'incorrect result when converting', `t`
