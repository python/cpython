#
# mactty module - Use a terminal line as communications channel.
#
# Note that this module is not very complete or well-designed, but it
# will have to serve until I have time to write something better. A unix
# module with the same API is available too, contact me (jack@cwi.nl)
# if you need it.
#
# Usage:
# t = Tty(toolname)
# t.raw()		Set in raw/no-echo mode
# t.baudrate(rate)	Set baud rate
# t.reset()		Back to normal
# t.read(len)		Read up to 'len' bytes (but often reads less)
# t.readall(len)	Read 'len' bytes
# t.timedread(len,tout)	Read upto 'len' bytes, or until 'tout' seconds idle
# t.getmode()		Get parameters as a string
# t.setmode(args)	Set parameters
#
# Jack Jansen, CWI, January 1997
#
import ctb
DEBUG=1

class Tty:
	def __init__(self, name=None):
		self.connection = ctb.CMNew('Serial Tool', (10000, 10000, 0, 0, 0, 0))
		#self.orig_data = self.connection.GetConfig()
		#if name:
		#	self.connection.SetConfig(self.orig_data + ' Port "%s"'%name)
		self.connection.Open(10)
		sizes, status = self.connection.Status()

	def getmode(self):
		return self.connection.GetConfig()

	def setmode(self, mode):
		length = self.connection.SetConfig(mode)
		if length != 0 and length != len(mode):
			raise 'SetConfig Error', (mode[:length], mode[length:])
		
	def raw(self):
		pass
		
	def baudrate(self, rate):
		self.setmode(self.getmode()+' baud %d'%rate)

	def reset(self):
		self.setmode(self.orig_data)

	def readall(self, length):
		data = ''
		while len(data) < length:
			data = data + self.read(length-len(data))
		return data

	def timedread(self,length, timeout):
		self.connection.Idle()
		data, eom = self.connection.Read(length, ctb.cmData, timeout*10)
		if DEBUG:
			print 'Timedread(%d, %d)->%s'%(length, timeout, `data`)
		return data
		
	def read(self, length):
		return self.timedread(length, 0x3fffffff)
		
	def write(self, data):
		if DEBUG:
			print 'Write(%s)'%`data`
		while data:
			self.connection.Idle()
			done = self.connection.Write(data, ctb.cmData, 0x3fffffff, 0)
			self.connection.Idle()
			data = data[done:]
