"""argvemulator - create sys.argv from OSA events. Used by applets that
want unix-style arguments.
"""

import sys
import traceback
from Carbon import AE
from Carbon.AppleEvents import *
from Carbon import Evt
from Carbon import File
from Carbon.Events import *
import aetools

class ArgvCollector:

    """A minimal FrameWork.Application-like class"""

    def __init__(self):
        self.quitting = 0
        # Remove the funny -psn_xxx_xxx argument
        if len(sys.argv) > 1 and sys.argv[1][:4] == '-psn':
            del sys.argv[1]

        AE.AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, self.__runapp)
        AE.AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, self.__openfiles)

    def close(self):
        AE.AERemoveEventHandler(kCoreEventClass, kAEOpenApplication)
        AE.AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments)

    def mainloop(self, mask = highLevelEventMask, timeout = 1*60):
        # Note: this is not the right way to run an event loop in OSX or even
        # "recent" versions of MacOS9. This is however code that has proven
        # itself.
        stoptime = Evt.TickCount() + timeout
        while not self.quitting and Evt.TickCount() < stoptime:
            self._dooneevent(mask, timeout)

        if not self.quitting:
            print "argvemulator: timeout waiting for arguments"

        self.close()

    def _dooneevent(self, mask = highLevelEventMask, timeout = 1*60):
        got, event = Evt.WaitNextEvent(mask, timeout)
        if got:
            self._lowlevelhandler(event)

    def _lowlevelhandler(self, event):
        what, message, when, where, modifiers = event
        h, v = where
        if what == kHighLevelEvent:
            try:
                AE.AEProcessAppleEvent(event)
            except AE.Error, err:
                msg = "High Level Event: %r %r" % (hex(message), hex(h | (v<<16)))
                print 'AE error: ', err
                print 'in', msg
                traceback.print_exc()
            return
        else:
            print "Unhandled event:", event


    def _quit(self):
        self.quitting = 1

    def __runapp(self, requestevent, replyevent):
        self._quit()

    def __openfiles(self, requestevent, replyevent):
        try:
            listdesc = requestevent.AEGetParamDesc(keyDirectObject, typeAEList)
            for i in range(listdesc.AECountItems()):
                aliasdesc = listdesc.AEGetNthDesc(i+1, typeAlias)[1]
                alias = File.Alias(rawdata=aliasdesc.data)
                fsref = alias.FSResolveAlias(None)[0]
                pathname = fsref.as_pathname()
                sys.argv.append(pathname)
        except  Exception, e:
            print "argvemulator.py warning: can't unpack an open document event"
            import traceback
            traceback.print_exc()

        self._quit()

if __name__ == '__main__':
    ArgvCollector().mainloop()
    print "sys.argv=", sys.argv
