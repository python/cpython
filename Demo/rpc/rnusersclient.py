# Remote nusers client interface

import rpc
from rpc import Packer, Unpacker, UDPClient, BroadcastUDPClient


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


class PartialRnusersClient:

    def addpackers(self):
        self.packer = RnusersPacker()
        self.unpacker = RnusersUnpacker('')

    def Num(self):
        return self.make_call(1, None, None, self.unpacker.unpack_int)

    def Names(self):
        return self.make_call(2, None, \
                None, self.unpacker.unpack_utmpidlearr)

    def Allnames(self):
        return self.make_call(3, None, \
                None, self.unpacker.unpack_utmpidlearr)


class RnusersClient(PartialRnusersClient, UDPClient):

    def __init__(self, host):
        UDPClient.__init__(self, host, 100002, 2)


class BroadcastRnusersClient(PartialRnusersClient, BroadcastUDPClient):

    def __init__(self, bcastaddr):
        BroadcastUDPClient.__init__(self, bcastaddr, 100002, 2)


def test():
    import sys
    if not sys.argv[1:]:
        testbcast()
        return
    else:
        host = sys.argv[1]
    c = RnusersClient(host)
    list = c.Names()
    for (line, name, host, time), idle in list:
        line = strip0(line)
        name = strip0(name)
        host = strip0(host)
        print "%r %r %r %s %s" % (name, host, line, time, idle)

def testbcast():
    c = BroadcastRnusersClient('<broadcast>')
    def listit(list, fromaddr):
        host, port = fromaddr
        print host + '\t:',
        for (line, name, host, time), idle in list:
            print strip0(name),
        print
    c.set_reply_handler(listit)
    all = c.Names()
    print 'Total Count:', len(all)

def strip0(s):
    while s and s[-1] == '\0': s = s[:-1]
    return s

test()
