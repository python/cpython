# Remote nusers client interface

import rpc
from rpc import Packer, Unpacker, TCPClient, UDPClient


class RnusersPacker(Packer):
	def pack_utmp(self, ui):
		ut_line, ut_name, ut_host, ut_time = utmp
		self.pack_string(ut_line)
		self.pack_string(ut_name)
		self.pack_string(ut_host)
		self.pack_int(ut_time)
	def pack_utmpidle(self, ui):
		ui_itmp, ui_idle = ui
		self.pack_utmp(ui_utmp)
		self.pack_uint(ui_idle)
	def pack_utmpidlearr(self, list):
		self.pack_array(list, self.pack_itmpidle)


class RnusersUnpacker(Unpacker):
	def unpack_utmp(self):
		ut_line = self.unpack_string()
		ut_name = self.unpack_string()
		ut_host = self.unpack_string()
		ut_time = self.unpack_int()
		return ut_line, ut_name, ut_host, ut_time
	def unpack_utmpidle(self):
		ui_utmp = self.unpack_utmp()
		ui_idle = self.unpack_uint()
		return ui_utmp, ui_idle
	def unpack_utmpidlearr(self):
		return self.unpack_array(self.unpack_utmpidle)


class RnusersClient(UDPClient):

	def addpackers(self):
		self.packer = RnusersPacker().init()
		self.unpacker = RnusersUnpacker().init('')

	def init(self, host):
		return UDPClient.init(self, host, 100002, 2)

	def Num(self):
		self.start_call(1)
		self.do_call()
		n = self.unpacker.unpack_int()
		self.end_call()
		return n

	def Names(self):
		self.start_call(2)
		self.do_call()
		list = self.unpacker.unpack_utmpidlearr()
		self.end_call()
		return list

	def Allnames(self):
		self.start_call(3)
		self.do_call()
		list = self.unpacker.unpack_utmpidlearr()
		self.end_call()
		return list


def test():
	import sys
	host = ''
	if sys.argv[1:]: host = sys.argv[1]
	c = RnusersClient().init(host)
	list = c.Names()
	def strip0(s):
		while s and s[-1] == '\0': s = s[:-1]
		return s
	for (line, name, host, time), idle in list:
		line = strip0(line)
		name = strip0(name)
		host = strip0(host)
		print name, host, line, time, idle
