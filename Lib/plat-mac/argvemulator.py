"""argvemulator - create sys.argv from OSA events. Used by applets that
want unix-style arguments.
"""

import sys
import traceback
from Carbon import AE
from Carbon.AppleEvents import *
from Carbon import Evt
from Carbon.Events import *
import aetools

class ArgvCollector:

    """A minimal FrameWork.Application-like class"""

    def __init__(self):
        self.quitting = 0
        self.ae_handlers = {}
        # Remove the funny -psn_xxx_xxx argument
        if len(sys.argv) > 1 and sys.argv[1][:4] == '-psn':
            del sys.argv[1]
        self.installaehandler('aevt', 'oapp', self.open_app)
        self.installaehandler('aevt', 'odoc', self.open_file)

    def installaehandler(self, classe, type, callback):
        AE.AEInstallEventHandler(classe, type, self.callback_wrapper)
        self.ae_handlers[(classe, type)] = callback

    def close(self):
        for classe, type in self.ae_handlers.keys():
            AE.AERemoveEventHandler(classe, type)

    def mainloop(self, mask = highLevelEventMask, timeout = 1*60):
        stoptime = Evt.TickCount() + timeout
        while not self.quitting and Evt.TickCount() < stoptime:
            self.dooneevent(mask, timeout)
        self.close()

    def _quit(self):
        self.quitting = 1

    def dooneevent(self, mask = highLevelEventMask, timeout = 1*60):
        got, event = Evt.WaitNextEvent(mask, timeout)
        if got:
            self.lowlevelhandler(event)

    def lowlevelhandler(self, event):
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

    def callback_wrapper(self, _request, _reply):
        _parameters, _attributes = aetools.unpackevent(_request)
        _class = _attributes['evcl'].type
        _type = _attributes['evid'].type

        if self.ae_handlers.has_key((_class, _type)):
            _function = self.ae_handlers[(_class, _type)]
        elif self.ae_handlers.has_key((_class, '****')):
            _function = self.ae_handlers[(_class, '****')]
        elif self.ae_handlers.has_key(('****', '****')):
            _function = self.ae_handlers[('****', '****')]
        else:
            raise 'Cannot happen: AE callback without handler', (_class, _type)

        # XXXX Do key-to-name mapping here

        _parameters['_attributes'] = _attributes
        _parameters['_class'] = _class
        _parameters['_type'] = _type
        if _parameters.has_key('----'):
            _object = _parameters['----']
            del _parameters['----']
            # The try/except that used to be here can mask programmer errors.
            # Let the program crash, the programmer can always add a **args
            # to the formal parameter list.
            rv = _function(_object, **_parameters)
        else:
            #Same try/except comment as above
            rv = _function(**_parameters)

        if rv == None:
            aetools.packevent(_reply, {})
        else:
            aetools.packevent(_reply, {'----':rv})

    def open_app(self, **args):
        self._quit()

    def open_file(self, _object=None, **args):
        for alias in _object:
            fsr = alias.FSResolveAlias(None)[0]
            pathname = fsr.as_pathname()
            sys.argv.append(pathname)
        self._quit()

    def other(self, _object=None, _class=None, _type=None, **args):
        print 'Ignore AppleEvent', (_class, _type), 'for', _object, 'Other args:', args

if __name__ == '__main__':
    ArgvCollector().mainloop()
    print "sys.argv=", sys.argv
