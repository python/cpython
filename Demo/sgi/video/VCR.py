import fcntl
import IOCTL
from IOCTL import *
import sys
import struct
import select
import posix
import time

DEVICE='/dev/ttyd2'

class UnixFile:
	def open(self, name, mode):
		self.fd = posix.open(name, mode)
		return self

	def read(self, len):
		return posix.read(self.fd, len)

	def write(self, data):
		dummy = posix.write(self.fd, data)

	def flush(self):
		pass

	def fileno(self):
		return self.fd

	def close(self):
		dummy = posix.close(self.fd)

def packttyargs(*args):
	if type(args) <> type(()):
		raise 'Incorrect argtype for packttyargs'
	if type(args[0]) == type(1):
		iflag, oflag, cflag, lflag, line, chars = args
	elif type(args[0]) == type(()):
		if len(args) <> 1:
			raise 'Only 1 argument expected'
		iflag, oflag, cflag, lflag, line, chars = args[0]
	elif type(args[0]) == type([]):
		if len(args) <> 1:
			raise 'Only 1 argument expected'
		[iflag, oflag, cflag, lflag, line, chars] = args[0]
	str = struct.pack('hhhhb', iflag, oflag, cflag, lflag, line)
	for c in chars:
		str = str + c
	return str

def nullttyargs():
	chars = ['\0']*IOCTL.NCCS
	return packttyargs(0, 0, 0, 0, 0, chars)

def unpackttyargs(str):
	args = str[:-IOCTL.NCCS]
	rawchars = str[-IOCTL.NCCS:]
	chars = []
	for c in rawchars:
		chars.append(c)
	iflag, oflag, cflag, lflag, line = struct.unpack('hhhhb', args)
	return (iflag, oflag, cflag, lflag, line, chars)

def initline(name):
	fp = UnixFile().open(name, 2)
	ofp = fp
	fd = fp.fileno()
	rv = fcntl.ioctl(fd, IOCTL.TCGETA, nullttyargs())
	iflag, oflag, cflag, lflag, line, chars = unpackttyargs(rv)
	iflag = iflag & ~(INPCK|ISTRIP|INLCR|IUCLC|IXON|IXOFF)
	oflag = oflag & ~OPOST
	cflag = B9600|CS8|CREAD|CLOCAL
	lflag = lflag & ~(ISIG|ICANON|ECHO|TOSTOP)
	chars[VMIN] = chr(1)
	chars[VTIME] = chr(0)
	arg = packttyargs(iflag, oflag, cflag, lflag, line, chars)
	dummy = fcntl.ioctl(fd, IOCTL.TCSETA, arg)
	return fp, ofp

#
#
error = 'VCR.error'

# Commands/replies:
COMPLETION = '\x01'
ACK  ='\x0a'
NAK  ='\x0b'

NUMBER_N = 0x30
ENTER  = '\x40'

EXP_7= '\xde'
EXP_8= '\xdf'

CL   ='\x56'
CTRL_ENABLE = EXP_8 + '\xc6'
SEARCH_DATA = EXP_8 + '\x93'
ADDR_SENSE = '\x60'

PLAY ='\x3a'
STOP ='\x3f'
EJECT='\x2a'
FF   ='\xab'
REW  ='\xac'
STILL='\x4f'
STEP_FWD ='\x2b'    # Was: '\xad'
FM_SELECT=EXP_8 + '\xc8'
FM_STILL=EXP_8 + '\xcd'
PREVIEW=EXP_7 + '\x9d'
REVIEW=EXP_7 + '\x9e'
DM_OFF=EXP_8 + '\xc9'
DM_SET=EXP_8 + '\xc4'
FWD_SHUTTLE='\xb5'
REV_SHUTTLE='\xb6'
EM_SELECT=EXP_8 + '\xc0'
N_FRAME_REC=EXP_8 + '\x92'
SEARCH_PREROLL=EXP_8 + '\x90'
EDIT_PB_STANDBY=EXP_8 + '\x96'
EDIT_PLAY=EXP_8 + '\x98'
AUTO_EDIT=EXP_7 + '\x9c'

IN_ENTRY=EXP_7 + '\x90'
IN_ENTRY_RESET=EXP_7 + '\x91'
IN_ENTRY_SET=EXP_7 + '\x98'
IN_ENTRY_INC=EXP_7 + '\x94'
IN_ENTRY_DEC=EXP_7 + '\x95'
IN_ENTRY_SENSE=EXP_7 + '\x9a'

OUT_ENTRY=EXP_7 + '\x92'
OUT_ENTRY_RESET=EXP_7 + '\x93'
OUT_ENTRY_SET=EXP_7 + '\x99'
OUT_ENTRY_INC=EXP_7 + '\x96'
OUT_ENTRY_DEC=EXP_7 + '\x98'
OUT_ENTRY_SENSE=EXP_7 + '\x9b'

MUTE_AUDIO = '\x24'
MUTE_AUDIO_OFF = '\x25'
MUTE_VIDEO = '\x26'
MUTE_VIDEO_OFF = '\x27'
MUTE_AV = EXP_7 + '\xc6'
MUTE_AV_OFF = EXP_7 + '\xc7'

DEBUG=0

class VCR:
	def __init__(self):
		self.ifp, self.ofp = initline(DEVICE)
		self.busy_cmd = None
		self.async = 0
		self.cb = None
		self.cb_arg = None

	def _check(self):
		if self.busy_cmd:
			raise error, 'Another command active: '+self.busy_cmd

	def _endlongcmd(self, name):
		if not self.async:
			self.waitready()
			return 1
		self.busy_cmd = name
		return 'VCR BUSY'

	def fileno(self):
		return self.ifp.fileno()

	def setasync(self, async):
		self.async = async

	def setcallback(self, cb, arg):
		self.setasync(1)
		self.cb = cb
		self.cb_arg = arg

	def poll(self):
		if not self.async:
			raise error, 'Can only call poll() in async mode'
		if not self.busy_cmd:
			return
		if self.testready():
			if self.cb:
				apply(self.cb, (self.cb_arg,))

	def _cmd(self, cmd):
		if DEBUG:
			print '>>>',`cmd`
		self.ofp.write(cmd)
		self.ofp.flush()

	def _waitdata(self, len, tout):
		rep = ''
		while len > 0:
			if tout == None:
				ready, d1, d2 = select.select( \
					  [self.ifp], [], [])
			else:
				ready, d1, d2 = select.select( \
					  [self.ifp], [], [], tout)
			if ready == []:
##				if rep:
##					print 'FLUSHED:', `rep`
				return None
			data = self.ifp.read(1)
			if DEBUG:
				print '<<<',`data`
			if data == NAK:
				return NAK
			rep = rep + data
			len = len - 1
		return rep

	def _reply(self, len):
		data = self._waitdata(len, 10)
		if data == None:
			raise error, 'Lost contact with VCR'
		return data

	def _getnumber(self, len):
		data = self._reply(len)
		number = 0
		for c in data:
			digit = ord(c) - NUMBER_N
			if digit < 0 or digit > 9:
				raise error, 'Non-digit in number'+`c`
			number = number*10 + digit
		return number

	def _iflush(self):
		dummy = self._waitdata(10000, 0)
##		if dummy:
##			print 'IFLUSH:', dummy

	def simplecmd(self,cmd):
		self._iflush()
		for ch in cmd:
			self._cmd(ch)
			rep = self._reply(1)
			if rep == NAK:
				return 0
			elif rep <> ACK:
				raise error, 'Unexpected reply:' + `rep`
		return 1

	def replycmd(self, cmd):
		if not self.simplecmd(cmd[:-1]):
			return 0
		self._cmd(cmd[-1])

	def _number(self, number, digits):
		if number < 0:
			raise error, 'Unexpected negative number:'+ `number`
		maxnum = pow(10, digits)
		if number > maxnum:
			raise error, 'Number too big'
		while maxnum > 1:
			number = number % maxnum
			maxnum = maxnum / 10
			digit = number / maxnum
			ok = self.simplecmd(chr(NUMBER_N + digit))
			if not ok:
				raise error, 'Error while transmitting number'

	def initvcr(self, *optarg):
		timeout = None
		if optarg <> ():
			timeout = optarg[0]
			starttime = time.time()
		self.busy_cmd = None
		self._iflush()
		while 1:
##			print 'SENDCL'
			self._cmd(CL)
			rep = self._waitdata(1, 2)
##			print `rep`
			if rep in ( None, CL, NAK ):
				if timeout:
					if time.time() - starttime > timeout:
						raise error, \
							  'No reply from VCR'
				continue
			break
		if rep <> ACK:
			raise error, 'Unexpected reply:' + `rep`
		dummy = self.simplecmd(CTRL_ENABLE)

	def waitready(self):
		rep = self._waitdata(1, None)
		if rep == None:
			raise error, 'Unexpected None reply from waitdata'
		if rep not in  (COMPLETION, ACK):
			self._iflush()
			raise error, 'Unexpected waitready reply:' + `rep`
		self.busy_cmd = None

	def testready(self):
		rep = self._waitdata(1, 0)
		if rep == None:
			return 0
		if rep not in  (COMPLETION, ACK):
			self._iflush()
			raise error, 'Unexpected waitready reply:' + `rep`
		self.busy_cmd = None
		return 1

	def play(self): return self.simplecmd(PLAY)
	def stop(self): return self.simplecmd(STOP)
	def ff(self):   return self.simplecmd(FF)
	def rew(self):  return self.simplecmd(REW)
	def eject(self):return self.simplecmd(EJECT)
	def still(self):return self.simplecmd(STILL)
	def step(self): return self.simplecmd(STEP_FWD)

	def goto(self, (h, m, s, f)):
		if not self.simplecmd(SEARCH_DATA):
			return 0
		self._number(h, 2)
		self._number(m, 2)
		self._number(s, 2)
		self._number(f, 2)
		if not self.simplecmd(ENTER):
			return 0
		return self._endlongcmd('goto')

	# XXXX TC_SENSE doesn't seem to work
	def faulty_where(self):
		self._check()
		self._cmd(TC_SENSE)
		h = self._getnumber(2)
		m = self._getnumber(2)
		s = self._getnumber(2)
		f = self._getnumber(2)
		return (h, m, s, f)

	def where(self):
		return self.addr2tc(self.sense())

	def sense(self):
		self._check()
		self._cmd(ADDR_SENSE)
		num = self._getnumber(5)
		return num

	def addr2tc(self, num):
		f = num % 25
		num = num / 25
		s = num % 60
		num = num / 60
		m = num % 60
		h = num / 60
		return (h, m, s, f)

	def tc2addr(self, (h, m, s, f)):
		return ((h*60 + m)*60 + s)*25 + f

	def fmmode(self, mode):
		self._check()
		if mode == 'off':
			arg = 0
		elif mode == 'buffer':
			arg = 1
		elif mode == 'dnr':
			arg = 2
		else:
			raise error, 'fmmode arg should be off, buffer or dnr'
		if not self.simplecmd(FM_SELECT):
			return 0
		self._number(arg, 1)
		if not self.simplecmd(ENTER):
			return 0
		return 1

	def mute(self, mode, value):
		self._check()
		if mode == 'audio':
			cmds = (MUTE_AUDIO_OFF, MUTE_AUDIO)
		elif mode == 'video':
			cmds = (MUTE_VIDEO_OFF, MUTE_VIDEO)
		elif mode == 'av':
			cmds = (MUTE_AV_OFF, MUTE_AV)
		else:
			raise error, 'mute type should be audio, video or av'
		cmd = cmds[value]
		return self.simplecmd(cmd)

	def editmode(self, mode):
		self._check()
		if mode == 'off':
			a0 = a1 = a2 = 0
		elif mode == 'format':
			a0 = 4
			a1 = 7
			a2 = 4
		elif mode == 'asmbl':
			a0 = 1
			a1 = 7
			a2 = 4
		elif mode == 'insert-video':
			a0 = 2
			a1 = 4
			a2 = 0
		else:
			raise 'editmode should be off,format,asmbl or insert-video'
		if not self.simplecmd(EM_SELECT):
			return 0
		self._number(a0, 1)
		self._number(a1, 1)
		self._number(a2, 1)
		if not self.simplecmd(ENTER):
			return 0
		return 1

	def autoedit(self):
		self._check()
		return self._endlongcmd(AUTO_EDIT)

	def nframerec(self, num):
		if not self.simplecmd(N_FRAME_REC):
			return 0
		self._number(num, 4)
		if not self.simplecmd(ENTER):
			return 0
		return self._endlongcmd('nframerec')

	def fmstill(self):
		if not self.simplecmd(FM_STILL):
			return 0
		return self._endlongcmd('fmstill')

	def preview(self):
		if not self.simplecmd(PREVIEW):
			return 0
		return self._endlongcmd('preview')

	def review(self):
		if not self.simplecmd(REVIEW):
			return 0
		return self._endlongcmd('review')

	def search_preroll(self):
		if not self.simplecmd(SEARCH_PREROLL):
			return 0
		return self._endlongcmd('search_preroll')

	def edit_pb_standby(self):
		if not self.simplecmd(EDIT_PB_STANDBY):
			return 0
		return self._endlongcmd('edit_pb_standby')

	def edit_play(self):
		if not self.simplecmd(EDIT_PLAY):
			return 0
		return self._endlongcmd('edit_play')

	def dmcontrol(self, mode):
		self._check()
		if mode == 'off':
			return self.simplecmd(DM_OFF)
		if mode == 'multi freeze':
			num = 1000
		elif mode == 'zoom freeze':
			num = 2000
		elif mode == 'digital slow':
			num = 3000
		elif mode == 'freeze':
			num = 4011
		else:
			raise error, 'unknown dmcontrol argument: ' + `mode`
		if not self.simplecmd(DM_SET):
			return 0
		self._number(num, 4)
		if not self.simplecmd(ENTER):
			return 0
		return 1

	def fwdshuttle(self, num):
		if not self.simplecmd(FWD_SHUTTLE):
			return 0
		self._number(num, 1)
		return 1

	def revshuttle(self, num):
		if not self.simplecmd(REV_SHUTTLE):
			return 0
		self._number(num, 1)
		return 1

	def getentry(self, which):
		self._check()
		if which == 'in':
			cmd = IN_ENTRY_SENSE
		elif which == 'out':
			cmd = OUT_ENTRY_SENSE
		self.replycmd(cmd)
		h = self._getnumber(2)
		m = self._getnumber(2)
		s = self._getnumber(2)
		f = self._getnumber(2)
		return (h, m, s, f)

	def inentry(self, arg):
		return self.ioentry(arg, (IN_ENTRY, IN_ENTRY_RESET, \
			  IN_ENTRY_SET, IN_ENTRY_INC, IN_ENTRY_DEC))

	def outentry(self, arg):
		return self.ioentry(arg, (OUT_ENTRY, OUT_ENTRY_RESET, \
			  OUT_ENTRY_SET, OUT_ENTRY_INC, OUT_ENTRY_DEC))

	def ioentry(self, arg, (Load, Clear, Set, Inc, Dec)):
		self._check()
		if type(arg) == type(()):
			h, m, s, f = arg
			if not self.simplecmd(Set):
				return 0
			self._number(h,2)
			self._number(m,2)
			self._number(s,2)
			self._number(f,2)
			if not self.simplecmd(ENTER):
				return 0
			return 1
		elif arg == 'reset':
			cmd = Clear
		elif arg == 'load':
			cmd = Load
		elif arg == '+':
			cmd = Inc
		elif arg == '-':
			cmd = Dec
		else:
			raise error, 'Arg should be +,-,reset,load or (h,m,s,f)'
		return self.simplecmd(cmd)

	def cancel(self):
		d = self.simplecmd(CL)
		self.busy_cmd = None
