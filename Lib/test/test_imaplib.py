import imaplib
import time

# Can't use time.time() values, as they are O/S specific

timevalues = [(2033, 5, 18, 3, 33, 20, 2, 138, 0), '"18-May-2033 13:33:20 +1000"']

for t in timevalues:
    t1 = imaplib.Time2Internaldate(t)
    t2 = imaplib.Time2Internaldate(imaplib.Internaldate2tuple('INTERNALDATE ' + imaplib.Time2Internaldate(t)))
    if t1 <> t2:
        print 'incorrect result when converting', `t`
