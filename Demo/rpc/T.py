import sys, os, time

def TSTART():
	global t0, t1
	u, s, cu, cs = os.times()
	t0 = u+cu, s+cs, time.millitimer()

def TSTOP(*label):
	global t0, t1
	u, s, cu, cs = os.times()
	t1 = u+cu, s+cs, time.millitimer()
	tt = []
	for i in range(3):
		tt.append(t1[i] - t0[i])
	[u, s, r] = tt
	msg = ''
	for x in label: msg = msg + (x + ' ')
	msg = msg + `u` + ' user, ' + `s` + ' sys, ' + `r*0.001` + ' real\n'
	sys.stderr.write(msg)
