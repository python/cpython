#! /usr/bin/env python
#
# Class for profiling python code. rev 1.0  6/2/94
#
# Based on prior profile module by Sjoerd Mullender...
#   which was hacked somewhat by: Guido van Rossum
#
# See profile.doc for more information

"""Class for profiling Python code."""

# Copyright 1994, by InfoSeek Corporation, all rights reserved.
# Written by James Roskind
# 
# Permission to use, copy, modify, and distribute this Python software
# and its associated documentation for any purpose (subject to the
# restriction in the following sentence) without fee is hereby granted,
# provided that the above copyright notice appears in all copies, and
# that both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of InfoSeek not be used in
# advertising or publicity pertaining to distribution of the software
# without specific, written prior permission.  This permission is
# explicitly restricted to the copying and modification of the software
# to remain in Python, compiled Python, or other languages (such as C)
# wherein the modified or derived code is exclusively imported into a
# Python module.
# 
# INFOSEEK CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS. IN NO EVENT SHALL INFOSEEK CORPORATION BE LIABLE FOR ANY
# SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.



import sys
import os
import time
import marshal


# Sample timer for use with 
#i_count = 0
#def integer_timer():
#	global i_count
#	i_count = i_count + 1
#	return i_count
#itimes = integer_timer # replace with C coded timer returning integers

#**************************************************************************
# The following are the static member functions for the profiler class
# Note that an instance of Profile() is *not* needed to call them.
#**************************************************************************


# simplified user interface
def run(statement, *args):
	prof = Profile()
	try:
		prof = prof.run(statement)
	except SystemExit:
		pass
	if args:
		prof.dump_stats(args[0])
	else:
		return prof.print_stats()

# print help
def help():
	for dirname in sys.path:
		fullname = os.path.join(dirname, 'profile.doc')
		if os.path.exists(fullname):
			sts = os.system('${PAGER-more} '+fullname)
			if sts: print '*** Pager exit status:', sts
			break
	else:
		print 'Sorry, can\'t find the help file "profile.doc"',
		print 'along the Python search path'


class Profile:
	"""Profiler class.
	
	self.cur is always a tuple.  Each such tuple corresponds to a stack
	frame that is currently active (self.cur[-2]).  The following are the
	definitions of its members.  We use this external "parallel stack" to
	avoid contaminating the program that we are profiling. (old profiler
	used to write into the frames local dictionary!!) Derived classes
	can change the definition of some entries, as long as they leave
	[-2:] intact.

	[ 0] = Time that needs to be charged to the parent frame's function.
	       It is used so that a function call will not have to access the
	       timing data for the parent frame.
	[ 1] = Total time spent in this frame's function, excluding time in
	       subfunctions
	[ 2] = Cumulative time spent in this frame's function, including time in
	       all subfunctions to this frame.
	[-3] = Name of the function that corresponds to this frame.  
	[-2] = Actual frame that we correspond to (used to sync exception handling)
	[-1] = Our parent 6-tuple (corresponds to frame.f_back)

	Timing data for each function is stored as a 5-tuple in the dictionary
	self.timings[].  The index is always the name stored in self.cur[4].
	The following are the definitions of the members:

	[0] = The number of times this function was called, not counting direct
	      or indirect recursion,
	[1] = Number of times this function appears on the stack, minus one
	[2] = Total time spent internal to this function
	[3] = Cumulative time that this function was present on the stack.  In
	      non-recursive functions, this is the total execution time from start
	      to finish of each invocation of a function, including time spent in
	      all subfunctions.
	[5] = A dictionary indicating for each function name, the number of times
	      it was called by us.
	"""

	def __init__(self, timer=None):
		self.timings = {}
		self.cur = None
		self.cmd = ""

		self.dispatch = {  \
			  'call'     : self.trace_dispatch_call, \
			  'return'   : self.trace_dispatch_return, \
			  'exception': self.trace_dispatch_exception, \
			  }

		if not timer:
			if os.name == 'mac':
				import MacOS
				self.timer = MacOS.GetTicks
				self.dispatcher = self.trace_dispatch_mac
				self.get_time = self.get_time_mac
			elif hasattr(time, 'clock'):
				self.timer = time.clock
				self.dispatcher = self.trace_dispatch_i
			elif hasattr(os, 'times'):
				self.timer = os.times
				self.dispatcher = self.trace_dispatch
			else:
				self.timer = time.time
				self.dispatcher = self.trace_dispatch_i
		else:
			self.timer = timer
			t = self.timer() # test out timer function
			try:
				if len(t) == 2:
					self.dispatcher = self.trace_dispatch
				else:
					self.dispatcher = self.trace_dispatch_l
			except TypeError:
				self.dispatcher = self.trace_dispatch_i
		self.t = self.get_time()
		self.simulate_call('profiler')


	def get_time(self): # slow simulation of method to acquire time
		t = self.timer()
		if type(t) == type(()) or type(t) == type([]):
			t = reduce(lambda x,y: x+y, t, 0)
		return t
		
	def get_time_mac(self):
		return self.timer()/60.0

	# Heavily optimized dispatch routine for os.times() timer

	def trace_dispatch(self, frame, event, arg):
		t = self.timer()
		t = t[0] + t[1] - self.t        # No Calibration constant
		# t = t[0] + t[1] - self.t - .00053 # Calibration constant

		if self.dispatch[event](frame,t):
			t = self.timer()
			self.t = t[0] + t[1]
		else:
			r = self.timer()
			self.t = r[0] + r[1] - t # put back unrecorded delta
		return



	# Dispatch routine for best timer program (return = scalar integer)

	def trace_dispatch_i(self, frame, event, arg):
		t = self.timer() - self.t # - 1 # Integer calibration constant
		if self.dispatch[event](frame,t):
			self.t = self.timer()
		else:
			self.t = self.timer() - t  # put back unrecorded delta
		return
	
	# Dispatch routine for macintosh (timer returns time in ticks of 1/60th second)

	def trace_dispatch_mac(self, frame, event, arg):
		t = self.timer()/60.0 - self.t # - 1 # Integer calibration constant
		if self.dispatch[event](frame,t):
			self.t = self.timer()/60.0
		else:
			self.t = self.timer()/60.0 - t  # put back unrecorded delta
		return


	# SLOW generic dispatch routine for timer returning lists of numbers

	def trace_dispatch_l(self, frame, event, arg):
		t = self.get_time() - self.t

		if self.dispatch[event](frame,t):
			self.t = self.get_time()
		else:
			self.t = self.get_time()-t # put back unrecorded delta
		return


	def trace_dispatch_exception(self, frame, t):
		rt, rtt, rct, rfn, rframe, rcur = self.cur
		if (not rframe is frame) and rcur:
			return self.trace_dispatch_return(rframe, t)
		return 0


	def trace_dispatch_call(self, frame, t):
		fcode = frame.f_code
		fn = (fcode.co_filename, fcode.co_firstlineno, fcode.co_name)
		self.cur = (t, 0, 0, fn, frame, self.cur)
		if self.timings.has_key(fn):
			cc, ns, tt, ct, callers = self.timings[fn]
			self.timings[fn] = cc, ns + 1, tt, ct, callers
		else:
			self.timings[fn] = 0, 0, 0, 0, {}
		return 1

	def trace_dispatch_return(self, frame, t):
		# if not frame is self.cur[-2]: raise "Bad return", self.cur[3]

		# Prefix "r" means part of the Returning or exiting frame
		# Prefix "p" means part of the Previous or older frame

		rt, rtt, rct, rfn, frame, rcur = self.cur
		rtt = rtt + t
		sft = rtt + rct

		pt, ptt, pct, pfn, pframe, pcur = rcur
		self.cur = pt, ptt+rt, pct+sft, pfn, pframe, pcur

		cc, ns, tt, ct, callers = self.timings[rfn]
		if not ns:
			ct = ct + sft
			cc = cc + 1
		if callers.has_key(pfn):
			callers[pfn] = callers[pfn] + 1  # hack: gather more
			# stats such as the amount of time added to ct courtesy
			# of this specific call, and the contribution to cc
			# courtesy of this call.
		else:
			callers[pfn] = 1
		self.timings[rfn] = cc, ns - 1, tt+rtt, ct, callers

		return 1

	# The next few function play with self.cmd. By carefully preloading
	# our parallel stack, we can force the profiled result to include
	# an arbitrary string as the name of the calling function.
	# We use self.cmd as that string, and the resulting stats look
	# very nice :-).

	def set_cmd(self, cmd):
		if self.cur[-1]: return   # already set
		self.cmd = cmd
		self.simulate_call(cmd)

	class fake_code:
		def __init__(self, filename, line, name):
			self.co_filename = filename
			self.co_line = line
			self.co_name = name
			self.co_firstlineno = 0

		def __repr__(self):
			return repr((self.co_filename, self.co_line, self.co_name))

	class fake_frame:
		def __init__(self, code, prior):
			self.f_code = code
			self.f_back = prior
			
	def simulate_call(self, name):
		code = self.fake_code('profile', 0, name)
		if self.cur:
			pframe = self.cur[-2]
		else:
			pframe = None
		frame = self.fake_frame(code, pframe)
		a = self.dispatch['call'](frame, 0)
		return

	# collect stats from pending stack, including getting final
	# timings for self.cmd frame.
	
	def simulate_cmd_complete(self):   
		t = self.get_time() - self.t
		while self.cur[-1]:
			# We *can* cause assertion errors here if
			# dispatch_trace_return checks for a frame match!
			a = self.dispatch['return'](self.cur[-2], t)
			t = 0
		self.t = self.get_time() - t

	
	def print_stats(self):
		import pstats
		pstats.Stats(self).strip_dirs().sort_stats(-1). \
			  print_stats()

	def dump_stats(self, file):
		f = open(file, 'wb')
		self.create_stats()
		marshal.dump(self.stats, f)
		f.close()

	def create_stats(self):
		self.simulate_cmd_complete()
		self.snapshot_stats()

	def snapshot_stats(self):
		self.stats = {}
		for func in self.timings.keys():
			cc, ns, tt, ct, callers = self.timings[func]
			callers = callers.copy()
			nc = 0
			for func_caller in callers.keys():
				nc = nc + callers[func_caller]
			self.stats[func] = cc, nc, tt, ct, callers


	# The following two methods can be called by clients to use
	# a profiler to profile a statement, given as a string.
	
	def run(self, cmd):
		import __main__
		dict = __main__.__dict__
		return self.runctx(cmd, dict, dict)
	
	def runctx(self, cmd, globals, locals):
		self.set_cmd(cmd)
		sys.setprofile(self.dispatcher)
		try:
			exec cmd in globals, locals
		finally:
			sys.setprofile(None)
		return self

	# This method is more useful to profile a single function call.
	def runcall(self, func, *args):
		self.set_cmd(`func`)
		sys.setprofile(self.dispatcher)
		try:
			return apply(func, args)
		finally:
			sys.setprofile(None)


	#******************************************************************
	# The following calculates the overhead for using a profiler.  The
	# problem is that it takes a fair amount of time for the profiler
	# to stop the stopwatch (from the time it receives an event).
	# Similarly, there is a delay from the time that the profiler
	# re-starts the stopwatch before the user's code really gets to
	# continue.  The following code tries to measure the difference on
	# a per-event basis. The result can the be placed in the
	# Profile.dispatch_event() routine for the given platform.  Note
	# that this difference is only significant if there are a lot of
	# events, and relatively little user code per event.  For example,
	# code with small functions will typically benefit from having the
	# profiler calibrated for the current platform.  This *could* be
	# done on the fly during init() time, but it is not worth the
	# effort.  Also note that if too large a value specified, then
	# execution time on some functions will actually appear as a
	# negative number.  It is *normal* for some functions (with very
	# low call counts) to have such negative stats, even if the
	# calibration figure is "correct." 
	#
	# One alternative to profile-time calibration adjustments (i.e.,
	# adding in the magic little delta during each event) is to track
	# more carefully the number of events (and cumulatively, the number
	# of events during sub functions) that are seen.  If this were
	# done, then the arithmetic could be done after the fact (i.e., at
	# display time).  Currently, we track only call/return events.
	# These values can be deduced by examining the callees and callers
	# vectors for each functions.  Hence we *can* almost correct the
	# internal time figure at print time (note that we currently don't
	# track exception event processing counts).  Unfortunately, there
	# is currently no similar information for cumulative sub-function
	# time.  It would not be hard to "get all this info" at profiler
	# time.  Specifically, we would have to extend the tuples to keep
	# counts of this in each frame, and then extend the defs of timing
	# tuples to include the significant two figures. I'm a bit fearful
	# that this additional feature will slow the heavily optimized
	# event/time ratio (i.e., the profiler would run slower, fur a very
	# low "value added" feature.) 
	#
	# Plugging in the calibration constant doesn't slow down the
	# profiler very much, and the accuracy goes way up.
	#**************************************************************
	
	def calibrate(self, m):
		# Modified by Tim Peters
		n = m
		s = self.get_time()
		while n:
			self.simple()
			n = n - 1
		f = self.get_time()
		my_simple = f - s
		#print "Simple =", my_simple,

		n = m
		s = self.get_time()
		while n:
			self.instrumented()
			n = n - 1
		f = self.get_time()
		my_inst = f - s
		# print "Instrumented =", my_inst
		avg_cost = (my_inst - my_simple)/m
		#print "Delta/call =", avg_cost, "(profiler fixup constant)"
		return avg_cost

	# simulate a program with no profiler activity
	def simple(self):
		a = 1
		pass

	# simulate a program with call/return event processing
	def instrumented(self):
		a = 1
		self.profiler_simulation(a, a, a)

	# simulate an event processing activity (from user's perspective)
	def profiler_simulation(self, x, y, z):  
		t = self.timer()
		## t = t[0] + t[1]
		self.ut = t



class OldProfile(Profile):
	"""A derived profiler that simulates the old style profile, providing
	errant results on recursive functions. The reason for the usefulness of
	this profiler is that it runs faster (i.e., less overhead).  It still
	creates all the caller stats, and is quite useful when there is *no*
	recursion in the user's code.
	
	This code also shows how easy it is to create a modified profiler.
	"""

	def trace_dispatch_exception(self, frame, t):
		rt, rtt, rct, rfn, rframe, rcur = self.cur
		if rcur and not rframe is frame:
			return self.trace_dispatch_return(rframe, t)
		return 0

	def trace_dispatch_call(self, frame, t):
		fn = `frame.f_code`
		
		self.cur = (t, 0, 0, fn, frame, self.cur)
		if self.timings.has_key(fn):
			tt, ct, callers = self.timings[fn]
			self.timings[fn] = tt, ct, callers
		else:
			self.timings[fn] = 0, 0, {}
		return 1

	def trace_dispatch_return(self, frame, t):
		rt, rtt, rct, rfn, frame, rcur = self.cur
		rtt = rtt + t
		sft = rtt + rct

		pt, ptt, pct, pfn, pframe, pcur = rcur
		self.cur = pt, ptt+rt, pct+sft, pfn, pframe, pcur

		tt, ct, callers = self.timings[rfn]
		if callers.has_key(pfn):
			callers[pfn] = callers[pfn] + 1
		else:
			callers[pfn] = 1
		self.timings[rfn] = tt+rtt, ct + sft, callers

		return 1


	def snapshot_stats(self):
		self.stats = {}
		for func in self.timings.keys():
			tt, ct, callers = self.timings[func]
			callers = callers.copy()
			nc = 0
			for func_caller in callers.keys():
				nc = nc + callers[func_caller]
			self.stats[func] = nc, nc, tt, ct, callers

		

class HotProfile(Profile):
	"""The fastest derived profile example.  It does not calculate
	caller-callee relationships, and does not calculate cumulative
	time under a function.  It only calculates time spent in a
	function, so it runs very quickly due to its very low overhead.
	"""

	def trace_dispatch_exception(self, frame, t):
		rt, rtt, rfn, rframe, rcur = self.cur
		if rcur and not rframe is frame:
			return self.trace_dispatch_return(rframe, t)
		return 0

	def trace_dispatch_call(self, frame, t):
		self.cur = (t, 0, frame, self.cur)
		return 1

	def trace_dispatch_return(self, frame, t):
		rt, rtt, frame, rcur = self.cur

		rfn = `frame.f_code`

		pt, ptt, pframe, pcur = rcur
		self.cur = pt, ptt+rt, pframe, pcur

		if self.timings.has_key(rfn):
			nc, tt = self.timings[rfn]
			self.timings[rfn] = nc + 1, rt + rtt + tt
		else:
			self.timings[rfn] =      1, rt + rtt

		return 1


	def snapshot_stats(self):
		self.stats = {}
		for func in self.timings.keys():
			nc, tt = self.timings[func]
			self.stats[func] = nc, nc, tt, 0, {}

		

#****************************************************************************
def Stats(*args):
	print 'Report generating functions are in the "pstats" module\a'


# When invoked as main program, invoke the profiler on a script
if __name__ == '__main__':
	import sys
	import os
	if not sys.argv[1:]:
		print "usage: profile.py scriptfile [arg] ..."
		sys.exit(2)

	filename = sys.argv[1]	# Get script filename

	del sys.argv[0]		# Hide "profile.py" from argument list

	# Insert script directory in front of module search path
	sys.path.insert(0, os.path.dirname(filename))

	run('execfile(' + `filename` + ')')
