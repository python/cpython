# Test the signal module

import signal
import os


pid = os.getpid()

# Shell script that will send us asynchronous signals
script = """
(
	set -x
	sleep 2
	kill -5 %(pid)d
	sleep 2
	kill -2 %(pid)d
	sleep 2
	kill -3 %(pid)d
) &
""" % vars()

def handlerA(*args):
	print "handlerA", args

HandlerBCalled = "HandlerBCalled"	# Exception

def handlerB(*args):
	print "handlerB", args
	raise HandlerBCalled, args

signal.alarm(20)			# Entire test lasts at most 20 sec.
signal.signal(5, handlerA)
signal.signal(2, handlerB)
signal.signal(3, signal.SIG_IGN)
signal.signal(signal.SIGALRM, signal.default_int_handler)

os.system(script)

print "starting pause() loop..."

try:
	while 1:
		print "call pause()..."
		try:
			signal.pause()
			print "pause() returned"
		except HandlerBCalled:
			print "HandlerBCalled exception caught"
except KeyboardInterrupt:
	print "KeyboardInterrupt (assume the alarm() went off)"
