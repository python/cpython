# Replacements for getevent() and pollevent(),
# and new functions ungetevent() and sync().


# Every library module should ideally use this instead of
# stdwin.{get,poll}event(), so applications can use the services
# of ungetevent() and sync().


import stdwin


# Events read ahead are stored in this queue.
#
queue = []


# Replacement for getevent().
#
def getevent():
	if queue:
		event = queue[0]
		del queue[0]
		return event
	else:
		return stdwin.getevent()


# Replacement for pollevent().
#
def pollevent():
	if queue:
		return getevent()
	else:
		return stdwin.pollevent()


# Push an event back in the queue.
#
def ungetevent(event):
	queue.insert(0, event)


# Synchronize the display.  It turns out that this is the way to
# force STDWIN to call XSync(), which some (esoteric) applications need.
# (This is stronger than just flushing -- it actually waits for a
# positive response from the X server on the last command issued.)
#
def sync():
	while 1:
		event = stdwin.pollevent()
		if not event: break
		queue.append(event)
