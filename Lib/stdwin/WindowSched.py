# Combine a real-time scheduling queue and stdwin event handling.
# Uses the millisecond timer.

import stdwin
from stdwinevents import WE_TIMER
import WindowParent
import sched
import time

# Delay function called by the scheduler when it has nothing to do.
# Return immediately when something is done, or when the delay is up.
#
def delayfunc(msecs):
	#
	# Check for immediate stdwin event
	#
	event = stdwin.pollevent()
	if event:
		WindowParent.Dispatch(event)
		return
	#
	# Use millisleep for very short delays or if there are no windows
	#
	if msecs < 100 or WindowParent.CountWindows() = 0:
		if msecs > 0:
			time.millisleep(msecs)
		return
	#
	# Post a timer event on an arbitrary window and wait for it
	#
	window = WindowParent.AnyWindow()
	window.settimer(msecs/100)
	event = stdwin.getevent()
	window.settimer(0)
	if event[0] <> WE_TIMER:
		WindowParent.Dispatch(event)

q = sched.scheduler().init(time.millitimer, delayfunc)

# Export functions enter, enterabs and cancel just like a scheduler
#
enter = q.enter
enterabs = q.enterabs
cancel = q.cancel

# Emptiness check must check both queues
#
def empty():
	return q.empty() and WindowParent.CountWindows() = 0

# Run until there is nothing left to do
#
def run():
	while not empty():
		if q.empty():
			WindowParent.Dispatch(stdwin.getevent())
		else:
			q.run()
