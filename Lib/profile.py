#
# Class for profiling python code.
# Author: Sjoerd Mullender
#
# See the accompanying document profile.doc for more information.

import sys
import codehack
import posix

class Profile():

	def init(self):
		self.timings = {}
		self.debug = None
		self.call_level = 0
		self.profile_func = None
		self.profiling = 0
		return self

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
			t = posix.times()
			t = t[0] + t[1]
			lineno = codehack.getlineno(frame.f_code)
			filename = frame.f_code.co_filename
			key = filename + ':' + `lineno` + '(' + funcname + ')'
			self.call_level = depth(frame)
			self.cur_frame = frame
			pframe = frame.f_back
			if self.debug:
				s0 = 'call: ' + key + ' depth: ' + `self.call_level` + ' time: ' + `t`
			if pframe:
				pkey = pframe.f_code.co_filename + ':' + \
					  `codehack.getlineno(pframe.f_code)` + '(' + \
					  codehack.getcodename(pframe.f_code) + ')'
				if self.debug:
					s1 = 'parent: ' + pkey
				if pframe.f_locals.has_key('__start_time'):
					st = pframe.f_locals['__start_time']
					nc, tt, ct, callers, callees = self.timings[pkey]
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
				if self.profile_func.has_key(codehack.getcodename(frame.f_code)):
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
		print 'profile.Profile.dispatch: unknown debugging event:', `event`
		return

	def handle_return(self, pframe, frame, s0):
		t = posix.times()
		t = t[0] + t[1]
		funcname = codehack.getcodename(frame.f_code)
		lineno = codehack.getlineno(frame.f_code)
		filename = frame.f_code.co_filename
		key = filename + ':' + `lineno` + '(' + funcname + ')'
		if self.debug:
			s0 = s0 + key + ' depth: ' + `self.call_level` + ' time: ' + `t`
		if pframe:
			funcname = codehack.getcodename(frame.f_code)
			lineno = codehack.getlineno(frame.f_code)
			filename = frame.f_code.co_filename
			pkey = filename + ':' + `lineno` + '(' + funcname + ')'
			if self.debug:
				s1 = 'parent: '+pkey
			if pframe.f_locals.has_key('__start_time') and \
				  self.timings.has_key(pkey):
				st = pframe.f_locals['__start_time']
				nc, tt, ct, callers, callees = self.timings[pkey]
				if self.debug:
					s1 = s1+' before: st='+`st`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct`
					s1 = s1+' after: st='+`t`+' nc='+`nc`+' tt='+`tt`+' ct='+`ct+(t-st)`
				self.timings[pkey] = nc, tt, ct + (t - st), callers, callees
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
		self.timings[key] = nc, tt + (t - st), ct + (t - st), callers, callees

	def print_stats(self):
		import string
		s = string.rjust('# calls', 8)
		s = s + ' ' + string.rjust('tot time', 8)
		s = s + ' ' + string.rjust('per call', 8)
		s = s + ' ' + string.rjust('cum time', 8)
		s = s + ' ' + string.rjust('per call', 8)
		print s + ' filename(function)'
		for key in self.timings.keys():
			nc, tt, ct, callers, callees = self.timings[key]
			if nc == 0:
				continue
			s = string.rjust(`nc`, 8)
			s = s + ' ' + string.rjust(`tt`, 8)
			s = s + ' ' + string.rjust(`tt/nc`, 8)
			s = s + ' ' + string.rjust(`ct`, 8)
			s = s + ' ' + string.rjust(`ct/nc`, 8)
			print s + ' ' + key

	def dump_stats(self, file):
		import marshal
		f = open(file, 'w')
		marshal.dump(self.timings, f)
		f.close()

	# The following two functions can be called by clients to use
	# a debugger to debug a statement, given as a string.
	
	def run(self, cmd):
		import __main__
		dict = __main__.__dict__
		self.runctx(cmd, dict, dict)
	
	def runctx(self, cmd, globals, locals):
##		self.reset()
		sys.setprofile(self.trace_dispatch)
		try:
##			try:
				exec(cmd + '\n', globals, locals)
##			except ProfQuit:
##				pass
		finally:
##			self.quitting = 1
			sys.setprofile(None)
		# XXX What to do if the command finishes normally?

def depth(frame):
	d = 0
	while frame:
		d = d + 1
		frame = frame.f_back
	return d

def run(statement, *args):
	prof = Profile().init()
	try:
		prof.run(statement)
	except SystemExit:
		pass
	if len(args) == 0:
		prof.print_stats()
	else:
		prof.dump_stats(args[0])

def cv_float(val, width):
	import string
	s = `val`
	try:
		e = string.index(s, 'e')
		exp = s[e+1:]
		s = s[:e]
		width = width - len(exp) - 1
	except string.index_error:
		exp = ''
	try:
		d = string.index(s, '.')
		frac = s[d+1:]
		s = s[:d]
		width = width - len(s) - 1
	except string.index_error:
		if exp <> '':
			return s + 'e' + exp
		else:
			return s
	if width < 0:
		width = 0
	while width < len(frac):
		c = frac[width]
		frac = frac[:width]
		if ord(c) >= ord('5'):
			carry = 1
			for i in range(width-1, -1, -1):
				if frac[i] == '9':
					frac = frac[:i]
					# keep if trailing zeroes are wanted
					# + '0' + frac[i+1:width]
				else:
					frac = frac[:i] + chr(ord(frac[i])+1) + frac[i+1:width]
					carry = 0
					break
			if carry:
				for i in range(len(s)-1, -1, -1):
					if s[i] == '9':
						s = s[:i] + '0' + s[i+1:]
						if i == 0:
							# gets one wider, so
							# should shorten
							# fraction by one
							s = '1' + s
							if width > 0:
								width = width - 1
					else:
						s = s[:i] + chr(ord(s[i])+1) + s[i+1:]
						break
	# delete trailing zeroes
	for i in range(len(frac)-1, -1, -1):
		if frac[i] == '0':
			frac = frac[:i]
		else:
			break
	# build up the number
	if width > 0 and len(frac) > 0:
		s = s + '.' + frac[:width]
	if exp <> '':
		s = s + 'e' + exp
	return s

def print_line(nc, tt, ct, callers, callees, key):
	import string
	s = string.rjust(cv_float(nc,8), 8)
	s = s + ' ' + string.rjust(cv_float(tt,8), 8)
	if nc == 0:
		s = s + ' '*9
	else:
		s = s + ' ' + string.rjust(cv_float(tt/nc,8), 8)
	s = s + ' ' + string.rjust(cv_float(ct,8), 8)
	if nc == 0:
		s = s + ' '*9
	else:
		s = s + ' ' + string.rjust(cv_float(ct/nc,8), 8)
	print s + ' ' + key

class Stats():
	def init(self, file):
		import marshal
		f = open(file, 'r')
		self.stats = marshal.load(f)
		f.close()
		self.stats_list = None
		return self

	def print_stats(self):
		import string
		s = string.rjust('# calls', 8)
		s = s + ' ' + string.rjust('tot time', 8)
		s = s + ' ' + string.rjust('per call', 8)
		s = s + ' ' + string.rjust('cum time', 8)
		s = s + ' ' + string.rjust('per call', 8)
		print s + ' filename(function)'
		if self.stats_list:
			for i in range(len(self.stats_list)):
				nc, tt, ct, callers, callees, key = self.stats_list[i]
				print_line(nc, tt, ct, callers, callees, key)
		else:
			for key in self.stats.keys():
				nc, tt, ct, callers, callees = self.stats[key]
				print_line(nc, tt, ct, callers, callees, key)

	def print_callers(self):
		if self.stats_list:
			for i in range(len(self.stats_list)):
				nc, tt, ct, callers, callees, key = self.stats_list[i]
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
				nc, tt, ct, callers, callees, key = self.stats_list[i]
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

	def strip_dirs(self):
		import os
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

# test command
def debug():
	prof = Profile().init()
	prof.debug = 1
	try:
		prof.run('import x; x.main()')
	except SystemExit:
		pass
	prof.print_stats()

def test():
	run('import x; x.main()')
