#
# Class for profiling python code.
# Author: Sjoerd Mullender
# Hacked somewhat by: Guido van Rossum
#
# See the accompanying document profile.doc for more information.

import sys
import codehack
import os
import string
import fpformat
import marshal

class Profile:

	def __init__(self):
		self.timings = {}
		self.debug = None
		self.call_level = 0
		self.profile_func = None
		self.profiling = 0

	def profile(self, funcname):
		if not self.profile_func:
			self.profile_func = {}
		self.profile_func[funcname] = 1

	def trace_dispatch(self, frame, event, arg):
		if event == 'call':
			funcname = codehack.getcodename(frame.f_code)
			if self.profile_func and not self.profiling:
				if self.profile_func.has_key(funcname):
					return
				self.profiling = 1
			t = os.times()
			t = t[0] + t[1]
			if frame.f_locals.has_key('__key'):
				key = frame.f_locals['__key']
			else:
				lineno = codehack.getlineno(frame.f_code)
				filename = frame.f_code.co_filename
				key = filename + ':' + `lineno` + '(' + funcname + ')'
				frame.f_locals['__key'] = key
			self.call_level = depth(frame)
			self.cur_frame = frame
			pframe = frame.f_back
			if self.debug:
				s0 = 'call: ' + key + ' depth: ' + `self.call_level` + ' time: ' + `t`
			if pframe:
				if pframe.f_locals.has_key('__key'):
					pkey = pframe.f_locals['__key']
				else:
					pkey = pframe.f_code.co_filename + \
						  ':' + \
						  `codehack.getlineno(pframe.f_code)` \
						  + '(' + \
						  codehack.getcodename(pframe.f_code) \
						  + ')'
					pframe.f_locals['__key'] = pkey
				if self.debug:
					s1 = 'parent: ' + pkey
				if pframe.f_locals.has_key('__start_time'):
					st = pframe.f_locals['__start_time']
					nc, tt, ct, callers, callees = \
						self.timings[pkey]
					if self.debug:
						s1 = s1+' before: st='+`st`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct`
					if callers.has_key(key):
						callers[key] = callers[key] + 1
					else:
						callers[key] = 1
					if self.debug:
						s1 = s1+' after: st='+`st`+' nc='+`nc`+' tt='+`tt+(t-st)`+' ct='+`ct`
					self.timings[pkey] = nc, tt + (t - st), ct, callers, callees
			if self.timings.has_key(key):
				nc, tt, ct, callers, callees = self.timings[key]
			else:
				nc, tt, ct, callers, callees = 0, 0, 0, {}, {}
			if self.debug:
				s0 = s0+' before: nc='+`nc`+' tt='+`tt`+' ct='+`ct`
				s0 = s0+' after: nc='+`nc+1`+' tt='+`tt`+' ct='+`ct`
			if pframe:
				if callees.has_key(pkey):
					callees[pkey] = callees[pkey] + 1
				else:
					callees[pkey] = 1
			self.timings[key] = nc + 1, tt, ct, callers, callees
			frame.f_locals['__start_time'] = t
			if self.debug:
				print s0
				print s1
			return
		if event == 'return':
			if self.profile_func:
				if not self.profiling:
					return
				if self.profile_func.has_key( \
					codehack.getcodename(frame.f_code)):
					self.profiling = 0
			self.call_level = depth(frame)
			self.cur_frame = frame
			pframe = frame.f_back
			if self.debug:
				s0 = 'return: '
			else:
				s0 = None
			self.handle_return(pframe, frame, s0)
			return
		if event == 'exception':
			if self.profile_func and not self.profiling:
				return
			call_level = depth(frame)
			if call_level < self.call_level:
				if call_level <> self.call_level - 1:
					print 'heh!',call_level,self.call_level
				if self.debug:
					s0 = 'exception: '
				else:
					s0 = None
				self.handle_return(self.cur_frame, frame, s0)
			self.call_level = call_level
			self.cur_frame = frame
			return
		print 'profile.Profile.dispatch: unknown debugging event:',
		print `event`
		return

	def handle_return(self, pframe, frame, s0):
		t = os.times()
		t = t[0] + t[1]
		if frame.f_locals.has_key('__key'):
			key = frame.f_locals['__key']
		else:
			funcname = codehack.getcodename(frame.f_code)
			lineno = codehack.getlineno(frame.f_code)
			filename = frame.f_code.co_filename
			key = filename + ':' + `lineno` + '(' + funcname + ')'
			frame.f_locals['__key'] = key
		if self.debug:
			s0 = s0 + key + ' depth: ' + `self.call_level` + ' time: ' + `t`
		if pframe:
			if pframe.f_locals.has_key('__key'):
				pkey = pframe.f_locals['__key']
			else:
				funcname = codehack.getcodename(frame.f_code)
				lineno = codehack.getlineno(frame.f_code)
				filename = frame.f_code.co_filename
				pkey = filename + ':' + `lineno` + '(' + funcname + ')'
				pframe.f_locals['__key'] = pkey
			if self.debug:
				s1 = 'parent: '+pkey
			if pframe.f_locals.has_key('__start_time') and \
				  self.timings.has_key(pkey):
				st = pframe.f_locals['__start_time']
				nc, tt, ct, callers, callees = \
					self.timings[pkey]
				if self.debug:
					s1 = s1+' before: st='+`st`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct`
					s1 = s1+' after: st='+`t`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct+(t-st)`
				self.timings[pkey] = \
					nc, tt, ct + (t - st), callers, callees
				pframe.f_locals['__start_time'] = t
		if self.timings.has_key(key):
			nc, tt, ct, callers, callees = self.timings[key]
		else:
			nc, tt, ct, callers, callees = 0, 0, 0, {}, {}
		if frame.f_locals.has_key('__start_time'):
			st = frame.f_locals['__start_time']
		else:
			st = t
		if self.debug:
			s0 = s0+' before: st='+`st`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct`
			s0 = s0+' after: nc='+`nc`+' tt='+`tt+(t-st)`+' ct='+`ct+(t-st)`
			print s0
			print s1
		self.timings[key] = \
			nc, tt + (t - st), ct + (t - st), callers, callees

	def print_stats(self):
		# Print in reverse order by ct
		print_title()
		list = []
		for key in self.timings.keys():
			nc, tt, ct, callers, callees = self.timings[key]
			if nc == 0:
				continue
			list.append(ct, tt, nc, key)
		list.sort()
		list.reverse()
		for ct, tt, nc, key in list:
			print_line(nc, tt, ct, os.path.basename(key))

	def dump_stats(self, file):
		f = open(file, 'w')
		marshal.dump(self.timings, f)
		f.close()

	# The following two methods can be called by clients to use
	# a profiler to profile a statement, given as a string.
	
	def run(self, cmd):
		import __main__
		dict = __main__.__dict__
		self.runctx(cmd, dict, dict)
	
	def runctx(self, cmd, globals, locals):
		sys.setprofile(self.trace_dispatch)
		try:
			exec(cmd + '\n', globals, locals)
		finally:
			sys.setprofile(None)

	# This method is more useful to profile a single function call.

	def runcall(self, func, *args):
		sys.setprofile(self.trace_dispatch)
		try:
			apply(func, args)
		finally:
			sys.setprofile(None)


def depth(frame):
	d = 0
	while frame:
		d = d + 1
		frame = frame.f_back
	return d

class Stats:
	def __init__(self, file):
		f = open(file, 'r')
		self.stats = marshal.load(f)
		f.close()
		self.stats_list = None

	def print_stats(self):
		print_title()
		if self.stats_list:
			for i in range(len(self.stats_list)):
				nc, tt, ct, callers, callees, key = \
					self.stats_list[i]
				print_line(nc, tt, ct, key)
		else:
			for key in self.stats.keys():
				nc, tt, ct, callers, callees = self.stats[key]
				print_line(nc, tt, ct, key)

	def print_callers(self):
		if self.stats_list:
			for i in range(len(self.stats_list)):
				nc, tt, ct, callers, callees, key = \
					self.stats_list[i]
				print key,
				for func in callers.keys():
					print func+'('+`callers[func]`+')',
				print
		else:
			for key in self.stats.keys():
				nc, tt, ct, callers, callees = self.stats[key]
				print key,
				for func in callers.keys():
					print func+'('+`callers[func]`+')',
				print

	def print_callees(self):
		if self.stats_list:
			for i in range(len(self.stats_list)):
				nc, tt, ct, callers, callees, key = \
					self.stats_list[i]
				print key,
				for func in callees.keys():
					print func+'('+`callees[func]`+')',
				print
		else:
			for key in self.stats.keys():
				nc, tt, ct, callers, callees = self.stats[key]
				print key,
				for func in callees.keys():
					print func+'('+`callees[func]`+')',
				print

	def sort_stats(self, field):
		stats_list = []
		for key in self.stats.keys():
			t = self.stats[key]
			nt = ()
			for i in range(len(t)):
				if i == field:
					nt = (t[i],) + nt[:]
				else:
					nt = nt[:] + (t[i],)
			if field == -1:
				nt = (key,) + nt
			else:
				nt = nt + (key,)
			stats_list.append(nt)
		stats_list.sort()
		self.stats_list = []
		for i in range(len(stats_list)):
			t = stats_list[i]
			if field == -1:
				nt = t[1:] + t[0:1]
			else:
				nt = t[1:]
				nt = nt[:field] + t[0:1] + nt[field:]
			self.stats_list.append(nt)

	def reverse_order(self):
		self.stats_list.reverse()

	def strip_dirs(self):
		newstats = {}
		for key in self.stats.keys():
			nc, tt, ct, callers, callees = self.stats[key]
			newkey = os.path.basename(key)
			newcallers = {}
			for c in callers.keys():
				newcallers[os.path.basename(c)] = callers[c]
			newcallees = {}
			for c in callees.keys():
				newcallees[os.path.basename(c)] = callees[c]
			newstats[newkey] = nc, tt, ct, newcallers, newcallees
		self.stats = newstats
		self.stats_list = None

def print_title():
	print string.rjust('ncalls', 8),
	print string.rjust('tottime', 8),
	print string.rjust('percall', 8),
	print string.rjust('cumtime', 8),
	print string.rjust('percall', 8),
	print 'filename:lineno(function)'

def print_line(nc, tt, ct, key):
	print string.rjust(`nc`, 8),
	print f8(tt),
	if nc == 0:
		print ' '*8,
	else:
		print f8(tt/nc),
	print f8(ct),
	if nc == 0:
		print ' '*8,
	else:
		print f8(ct/nc),
	print key

def f8(x):
	return string.rjust(fpformat.fix(x, 3), 8)

# simplified user interface
def run(statement, *args):
	prof = Profile()
	try:
		prof.run(statement)
	except SystemExit:
		pass
	if len(args) == 0:
		prof.print_stats()
	else:
		prof.dump_stats(args[0])

# test command with debugging
def debug():
	prof = Profile()
	prof.debug = 1
	try:
		prof.run('import x; x.main()')
	except SystemExit:
		pass
	prof.print_stats()

# test command
def test():
	run('import x; x.main()')

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
