#
# Start of posixfile.py
#

#
# Extended file operations
#
# f = posixfile.open(filename, mode)
#
# f.file()
#	will return the original builtin file object
#
# f.dup()
#	will return a new file object based on a new filedescriptor
#
# f.dup2(fd)
#	will return a new file object based on the given filedescriptor
#
# f.flags(mode)
#	will turn on the associated flag (merge)
#	mode can contain the following characters:
#
#   (character representing a flag)
#	a	append only flag
#	c	close on exec flag
#	n	no delay flag
#	s	synchronization flag
#   (modifiers)
#	!	turn flags 'off' instead of default 'on'
#	=	copy flags 'as is' instead of default 'merge'
#	?	return a string in which the characters represent the flags
#		that are set
#
#	note: - the '!' and '=' modifiers are mutually exclusive.
#	      - the '?' modifier will return the status of the flags after they
#		have been changed by other characters in the mode string
#
# f.lock(mode [, len [, start [, whence]]])
#	will (un)lock a region
#	mode can contain the following characters:
#
#   (character representing type of lock)
#	u	unlock
#	r	read lock
#	w	write lock
#   (modifiers)
#	|	wait until the lock can be granted
#	?	return the first lock conflicting with the requested lock
#		or 'None' if there is no conflict. The lock returned is in the
#		format (mode, len, start, whence, pid) where mode is a
#		character representing the type of lock ('r' or 'w')
#
#	note: - the '?' modifier prevents a region from being locked; it is
#		query only
#

class _posixfile_:
    #
    # Internal routines
    #
    def __repr__(self):
	return repr(self._file_)

    def __del__(self):
	self._file_.close()

    #
    # Initialization routines
    #
    def open(self, name, mode):
	import __builtin__

	self._name_ = name
	self._mode_ = mode
	self._file_ = __builtin__.open(name, mode)
	# Copy basic file methods
	for method in self._file_.__methods__:
	    setattr(self, method, getattr(self._file_, method))
	return self

    #
    # New methods
    #
    def file(self):
	return self._file_

    def dup(self):
	import posix

	try: ignore = posix.fdopen
	except: raise AttributeError, 'dup() method unavailable'

	return posix.fdopen(posix.dup(self._file_.fileno()), self._mode_)

    def dup2(self, fd):
	import posix

	try: ignore = posix.fdopen
	except: raise AttributeError, 'dup() method unavailable'

	posix.dup2(self._file_.fileno(), fd)
	return posix.fdopen(fd, self._mode_)

    def flags(self, which):
	import fcntl, FCNTL

	l_flags = 0
	if 'n' in which: l_flags = l_flags | FCNTL.O_NDELAY
	if 'a' in which: l_flags = l_flags | FCNTL.O_APPEND
	if 's' in which: l_flags = l_flags | FCNTL.O_SYNC

	if '=' not in which:
	    cur_fl = fcntl.fcntl(self._file_.fileno(), FCNTL.F_GETFL, 0)
	    if '!' in which: l_flags = cur_fl & ~ l_flags
	    else: l_flags = cur_fl | l_flags

	l_flags = fcntl.fcntl(self._file_.fileno(), FCNTL.F_SETFL, l_flags)

	if 'c' in which:
	    arg = ('!' not in which)	# 1 is close
	    l_flags = fcntl.fcntl(self._file_.fileno(), FCNTL.F_SETFD, arg)

	if '?' in which:
	    which = ''
	    l_flags = fcntl.fcntl(self._file_.fileno(), FCNTL.F_GETFL, 0)
	    if FCNTL.O_APPEND & l_flags: which = which + 'a'
	    if fcntl.fcntl(self._file_.fileno(), FCNTL.F_GETFD, 0) & 1:
		which = which + 'c'
	    if FCNTL.O_NDELAY & l_flags: which = which + 'n'
	    if FCNTL.O_SYNC & l_flags: which = which + 's'
	    return which
	
    def lock(self, how, *args):
	import struct, fcntl, FCNTL

	if 'w' in how: l_type = FCNTL.F_WRLCK
	elif 'r' in how: l_type = FCNTL.F_WRLCK
	elif 'u' in how: l_type = FCNTL.F_UNLCK
	else: raise TypeError, 'no type of lock specified'

	if '|' in how: cmd = FCNTL.F_SETLKW
	elif '?' in how: cmd = FCNTL.F_GETLK
	else: cmd = FCNTL.F_SETLK

	l_whence = 0
	l_start = 0
	l_len = 0

	if len(args) == 1:
	    l_len = args[0]
	elif len(args) == 2:
	    l_len, l_start = args
	elif len(args) == 3:
	    l_len, l_start, l_whence = args
	elif len(args) > 3:
	    raise TypeError, 'too many arguments'

	flock = struct.pack('hhllhh', l_type, l_whence, l_start, l_len, 0, 0)
	flock = fcntl.fcntl(self._file_.fileno(), cmd, flock)

	if '?' in how:
	    l_type, l_whence, l_start, l_len, l_sysid, l_pid = \
		struct.unpack('hhllhh', flock)
	    if l_type != FCNTL.F_UNLCK:
		if l_type == FCNTL.F_RDLCK:
		    return 'r', l_len, l_start, l_whence, l_pid
		else:
		    return 'w', l_len, l_start, l_whence, l_pid

#
# Public routine to obtain a posixfile object
#
def open(name, mode):
    return _posixfile_().open(name, mode)

#
# End of posixfile.py
#
