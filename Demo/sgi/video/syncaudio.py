import AL
import al
import sys
import vtime
import socket
import time


SLEEPTIME = 500		# 500 ms sleeps
SAMPLEFREQ = 16000	# 16Khz samples
SAMPLERATE = AL.RATE_16000
NEEDBUFFERED = SAMPLEFREQ	# Buffer 1 second of sound
BUFFERSIZE = NEEDBUFFERED*4	# setqueuesize() par for 2 second sound

AVSYNCPORT = 10000	# Port for time syncing
AVCTLPORT = 10001	# Port for record start/stop

def main():
    if len(sys.argv) <> 3:
	print 'Usage: ', sys.argv[0], 'videohostname soundfile'
	sys.exit(1)
    #
    ofile = open(sys.argv[2], 'w')
    #
    globaltime = vtime.VTime().init(0,sys.argv[1],AVSYNCPORT)
    #
    ctl = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    ctl.bind((socket.gethostname(),AVCTLPORT))
    #
    inp = openmic()
    #
    out = 0		# Open aiff file
    #
    while 1:
	if mainloop(None, ctl, inp, out, globaltime):
	    break
	if mainloop(ofile, ctl, inp, out, globaltime):
	    break
    pass	# Close aiff file
    sys.exit(0)
#
def openmic():
    conf = al.newconfig()
    conf.setqueuesize(BUFFERSIZE)
    conf.setwidth(AL.SAMPLE_16)
    conf.setchannels(AL.MONO)
    return al.openport('micr','r',conf)
#
def mainloop(ofile, ctl, inp, out, globaltime):
    #
    # Wait for sync packet, keeping 1-2 seconds of sound in the
    # buffer
    #
    totsamps = 0
    totbytes = 0
    starttime = time.millitimer()
    while 1:
	time.millisleep(SLEEPTIME)
	if ctl.avail():
	    break
	nsamples = inp.getfilled()-NEEDBUFFERED
	if nsamples>0:
	    data = inp.readsamps(nsamples)
	    totsamps = totsamps + nsamples
	    totbytes = totbytes + len(data)
	    if ofile <> None:
		ofile.write(data)
    #
    # Compute his starttime and the timestamp of the first byte in the
    # buffer. Discard all buffered data upto his starttime
    #
    startstop,histime = eval(ctl.recv(100))
    if (ofile == None and startstop == 0) or \
			   (ofile <> None and startstop == 1):
	print 'Sync error: saving=',save,' request=',startstop
	sys.exit(1)
    filllevel = inp.getfilled()
    filltime = time.millitimer()
    filltime = filltime - filllevel / (SAMPLEFREQ/1000)
    starttime = globaltime.his2mine(histime)
    nsamples = starttime - filltime
    if nsamples < 0:
	print 'Start/stop signal came too late'
	sys.exit(1)
    nsamples = nsamples * (SAMPLEFREQ / 1000)
    data = inp.readsamps(nsamples)
    totsamps = totsamps + nsamples
    totbytes = totbytes + len(data)
    print 'Time: ', time.millitimer()-starttime, ', Bytes: ', totbytes, ', Samples: ', totsamps
    if ofile <> None:
	ofile.write(data)
    return (startstop == 2)

main()
