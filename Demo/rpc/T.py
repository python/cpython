# Simple interface to report execution times of program fragments.
# Call TSTART() to reset the timer, TSTOP(...) to report times.

import sys, os, time

def TSTART():
    global t0, t1
    u, s, cu, cs = os.times()
    t0 = u+cu, s+cs, time.time()

def TSTOP(*label):
    global t0, t1
    u, s, cu, cs = os.times()
    t1 = u+cu, s+cs, time.time()
    tt = []
    for i in range(3):
        tt.append(t1[i] - t0[i])
    [u, s, r] = tt
    msg = ''
    for x in label: msg = msg + (x + ' ')
    msg = msg + '%r user, %r sys, %r real\n' % (u, s, r)
    sys.stderr.write(msg)
