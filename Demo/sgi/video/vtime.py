#
# Module vtime - Keep virtual time between two nodes.
#
# We try for synchronised clocks by sending a packet of the for
# (1,mytime,0) to the other side, and waiting (at most) a second for
# a reply. This reply has the form (2,mytime,histime), and we can
# estimate the time difference by defining histime to be exactly half-way
# between the time we sent our message and got our reply. We send a
# final (3,mynewtime,histime) message to allow the other side to do the
# same computations.
#
# Note that the protocol suffers heavily from the 2-army problem.
# It'll have to do until I can read up on time-sync protocols, though.
#
from socket import *
import time

MSGSIZE = 100
MSGTIMEOUT = 1000

recv_timeout = 'receive timeout'
bad_connect = 'Bad connection'

def timeavg(a,b):
    return int((long(a)+b)/2L)
def tryrecv(s):
    cnt = 0
    while 1:
	if s.avail():
	    return s.recvfrom(MSGSIZE)
	time.millisleep(100)
	cnt = cnt + 100
	if cnt > MSGTIMEOUT:
	    raise recv_timeout

class VTime():
    def init(self,(client,host,port)):
	s = socket(AF_INET, SOCK_DGRAM)
	host = gethostbyname(host)
	localhost = gethostbyname(gethostname())
	raddr = (host,port)
	s.bind((localhost,port))
	if client:
	    #
	    # We loop here because we want the *second* measurement
	    # for accuracy
	    for loopct in (0,2):
		curtijd = time.millitimer()
		check = `(loopct,curtijd,0)`
		s.sendto(check,raddr)
		while 1:
		    try:
			if loopct:
			    data, other = s.recvfrom(MSGSIZE)
			else:
			    data, other = tryrecv(s)
			newtijd = time.millitimer()
			if other <> raddr:
			    print 'Someone else syncing to us: ', other
			    raise bad_connect
			data = eval(data)
			if data[:2] = (loopct+1,curtijd):
			    break
			if data[0] <> 2:
			    print 'Illegal sync reply: ', data
			    raise bad_connect
		    except recv_timeout:
			curtijd = time.millitimer()
			check = `(loopct,curtijd,0)`
			s.sendto(check,raddr)
	    histime = data[2]
	    s.sendto(`(4,newtijd,histime)`,raddr)
	    mytime = timeavg(curtijd,newtijd)
	    #mytime = curtijd
	    self.timediff = histime - mytime
	else:
	    while 1:
		data,other = s.recvfrom(MSGSIZE)
		if other <> raddr:
		    print 'Someone else syncing to us: ', other, ' Wanted ', raddr
		    raise bad_connect
		data = eval(data)
		if data[0] in (0,2):
		    curtijd = time.millitimer()
		    s.sendto(`(data[0]+1,data[1],curtijd)`,raddr)
		elif data[0] = 4:
		    newtijd = time.millitimer()
		    histime = data[1]
		    mytime = timeavg(curtijd,newtijd)
		    #mytime = curtijd
		    self.timediff = histime-mytime
		    break
		else:
		    print 'Funny data: ', data
		    raise bad_connect
	return self
	#
    def his2mine(self,tijd):
	return tijd - self.timediff
    #
    def mine2his(self, tijd):
	return tijd + self.timediff

def test(clt, host, port):
    xx = VTime().init(clt,host,port)
    print 'Time diff: ', xx.his2mine(0)
