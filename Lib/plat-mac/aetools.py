"""Tools for use in AppleEvent clients and servers.

pack(x) converts a Python object to an AEDesc object
unpack(desc) does the reverse

packevent(event, parameters, attributes) sets params and attrs in an AEAppleEvent record
unpackevent(event) returns the parameters and attributes from an AEAppleEvent record

Plus...  Lots of classes and routines that help representing AE objects,
ranges, conditionals, logicals, etc., so you can write, e.g.:

    x = Character(1, Document("foobar"))

and pack(x) will create an AE object reference equivalent to AppleScript's

    character 1 of document "foobar"

Some of the stuff that appears to be exported from this module comes from other
files: the pack stuff from aepack, the objects from aetypes.

"""


from warnings import warnpy3k
warnpy3k("In 3.x, the aetools module is removed.", stacklevel=2)

from types import *
from Carbon import AE
from Carbon import Evt
from Carbon import AppleEvents
import MacOS
import sys
import time

from aetypes import *
from aepack import packkey, pack, unpack, coerce, AEDescType

Error = 'aetools.Error'

# Amount of time to wait for program to be launched
LAUNCH_MAX_WAIT_TIME=10

# Special code to unpack an AppleEvent (which is *not* a disguised record!)
# Note by Jack: No??!? If I read the docs correctly it *is*....

aekeywords = [
    'tran',
    'rtid',
    'evcl',
    'evid',
    'addr',
    'optk',
    'timo',
    'inte', # this attribute is read only - will be set in AESend
    'esrc', # this attribute is read only
    'miss', # this attribute is read only
    'from'  # new in 1.0.1
]

def missed(ae):
    try:
        desc = ae.AEGetAttributeDesc('miss', 'keyw')
    except AE.Error, msg:
        return None
    return desc.data

def unpackevent(ae, formodulename=""):
    parameters = {}
    try:
        dirobj = ae.AEGetParamDesc('----', '****')
    except AE.Error:
        pass
    else:
        parameters['----'] = unpack(dirobj, formodulename)
        del dirobj
    # Workaround for what I feel is a bug in OSX 10.2: 'errn' won't show up in missed...
    try:
        dirobj = ae.AEGetParamDesc('errn', '****')
    except AE.Error:
        pass
    else:
        parameters['errn'] = unpack(dirobj, formodulename)
        del dirobj
    while 1:
        key = missed(ae)
        if not key: break
        parameters[key] = unpack(ae.AEGetParamDesc(key, '****'), formodulename)
    attributes = {}
    for key in aekeywords:
        try:
            desc = ae.AEGetAttributeDesc(key, '****')
        except (AE.Error, MacOS.Error), msg:
            if msg[0] != -1701 and msg[0] != -1704:
                raise
            continue
        attributes[key] = unpack(desc, formodulename)
    return parameters, attributes

def packevent(ae, parameters = {}, attributes = {}):
    for key, value in parameters.items():
        packkey(ae, key, value)
    for key, value in attributes.items():
        ae.AEPutAttributeDesc(key, pack(value))

#
# Support routine for automatically generated Suite interfaces
# These routines are also useable for the reverse function.
#
def keysubst(arguments, keydict):
    """Replace long name keys by their 4-char counterparts, and check"""
    ok = keydict.values()
    for k in arguments.keys():
        if k in keydict:
            v = arguments[k]
            del arguments[k]
            arguments[keydict[k]] = v
        elif k != '----' and k not in ok:
            raise TypeError, 'Unknown keyword argument: %s'%k

def enumsubst(arguments, key, edict):
    """Substitute a single enum keyword argument, if it occurs"""
    if key not in arguments or edict is None:
        return
    v = arguments[key]
    ok = edict.values()
    if v in edict:
        arguments[key] = Enum(edict[v])
    elif not v in ok:
        raise TypeError, 'Unknown enumerator: %s'%v

def decodeerror(arguments):
    """Create the 'best' argument for a raise MacOS.Error"""
    errn = arguments['errn']
    err_a1 = errn
    if 'errs' in arguments:
        err_a2 = arguments['errs']
    else:
        err_a2 = MacOS.GetErrorString(errn)
    if 'erob' in arguments:
        err_a3 = arguments['erob']
    else:
        err_a3 = None

    return (err_a1, err_a2, err_a3)

class TalkTo:
    """An AE connection to an application"""
    _signature = None   # Can be overridden by subclasses
    _moduleName = None  # Can be overridden by subclasses
    _elemdict = {}      # Can be overridden by subclasses
    _propdict = {}      # Can be overridden by subclasses

    __eventloop_initialized = 0
    def __ensure_WMAvailable(klass):
        if klass.__eventloop_initialized: return 1
        if not MacOS.WMAvailable(): return 0
        # Workaround for a but in MacOSX 10.2: we must have an event
        # loop before we can call AESend.
        Evt.WaitNextEvent(0,0)
        return 1
    __ensure_WMAvailable = classmethod(__ensure_WMAvailable)

    def __init__(self, signature=None, start=0, timeout=0):
        """Create a communication channel with a particular application.

        Addressing the application is done by specifying either a
        4-byte signature, an AEDesc or an object that will __aepack__
        to an AEDesc.
        """
        self.target_signature = None
        if signature is None:
            signature = self._signature
        if type(signature) == AEDescType:
            self.target = signature
        elif type(signature) == InstanceType and hasattr(signature, '__aepack__'):
            self.target = signature.__aepack__()
        elif type(signature) == StringType and len(signature) == 4:
            self.target = AE.AECreateDesc(AppleEvents.typeApplSignature, signature)
            self.target_signature = signature
        else:
            raise TypeError, "signature should be 4-char string or AEDesc"
        self.send_flags = AppleEvents.kAEWaitReply
        self.send_priority = AppleEvents.kAENormalPriority
        if timeout:
            self.send_timeout = timeout
        else:
            self.send_timeout = AppleEvents.kAEDefaultTimeout
        if start:
            self._start()

    def _start(self):
        """Start the application, if it is not running yet"""
        try:
            self.send('ascr', 'noop')
        except AE.Error:
            _launch(self.target_signature)
            for i in range(LAUNCH_MAX_WAIT_TIME):
                try:
                    self.send('ascr', 'noop')
                except AE.Error:
                    pass
                else:
                    break
                time.sleep(1)

    def start(self):
        """Deprecated, used _start()"""
        self._start()

    def newevent(self, code, subcode, parameters = {}, attributes = {}):
        """Create a complete structure for an apple event"""

        event = AE.AECreateAppleEvent(code, subcode, self.target,
                  AppleEvents.kAutoGenerateReturnID, AppleEvents.kAnyTransactionID)
        packevent(event, parameters, attributes)
        return event

    def sendevent(self, event):
        """Send a pre-created appleevent, await the reply and unpack it"""
        if not self.__ensure_WMAvailable():
            raise RuntimeError, "No window manager access, cannot send AppleEvent"
        reply = event.AESend(self.send_flags, self.send_priority,
                                  self.send_timeout)
        parameters, attributes = unpackevent(reply, self._moduleName)
        return reply, parameters, attributes

    def send(self, code, subcode, parameters = {}, attributes = {}):
        """Send an appleevent given code/subcode/pars/attrs and unpack the reply"""
        return self.sendevent(self.newevent(code, subcode, parameters, attributes))

    #
    # The following events are somehow "standard" and don't seem to appear in any
    # suite...
    #
    def activate(self):
        """Send 'activate' command"""
        self.send('misc', 'actv')

    def _get(self, _object, asfile=None, _attributes={}):
        """_get: get data from an object
        Required argument: the object
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the data
        """
        _code = 'core'
        _subcode = 'getd'

        _arguments = {'----':_object}
        if asfile:
            _arguments['rtyp'] = mktype(asfile)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if 'errn' in _arguments:
            raise Error, decodeerror(_arguments)

        if '----' in _arguments:
            return _arguments['----']
            if asfile:
                item.__class__ = asfile
            return item

    get = _get

    _argmap_set = {
        'to' : 'data',
    }

    def _set(self, _object, _attributes={}, **_arguments):
        """set: Set an object's data.
        Required argument: the object for the command
        Keyword argument to: The new value.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'setd'

        keysubst(_arguments, self._argmap_set)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise Error, decodeerror(_arguments)
        # XXXX Optionally decode result
        if '----' in _arguments:
            return _arguments['----']

    set = _set

    # Magic glue to allow suite-generated classes to function somewhat
    # like the "application" class in OSA.

    def __getattr__(self, name):
        if name in self._elemdict:
            cls = self._elemdict[name]
            return DelayedComponentItem(cls, None)
        if name in self._propdict:
            cls = self._propdict[name]
            return cls()
        raise AttributeError, name

# Tiny Finder class, for local use only

class _miniFinder(TalkTo):
    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: list of objects to open
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'odoc'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if 'errn' in _arguments:
            raise Error, decodeerror(_arguments)
        # XXXX Optionally decode result
        if '----' in _arguments:
            return _arguments['----']
#pass

_finder = _miniFinder('MACS')

def _launch(appfile):
    """Open a file thru the finder. Specify file by name or fsspec"""
    _finder.open(_application_file(('ID  ', appfile)))


class _application_file(ComponentItem):
    """application file - An application's file on disk"""
    want = 'appf'

_application_file._propdict = {
}
_application_file._elemdict = {
}

# Test program
# XXXX Should test more, really...

def test():
    target = AE.AECreateDesc('sign', 'quil')
    ae = AE.AECreateAppleEvent('aevt', 'oapp', target, -1, 0)
    print unpackevent(ae)
    raw_input(":")
    ae = AE.AECreateAppleEvent('core', 'getd', target, -1, 0)
    obj = Character(2, Word(1, Document(1)))
    print obj
    print repr(obj)
    packevent(ae, {'----': obj})
    params, attrs = unpackevent(ae)
    print params['----']
    raw_input(":")

if __name__ == '__main__':
    test()
    sys.exit(1)
