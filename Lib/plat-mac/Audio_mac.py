QSIZE = 100000
error='Audio_mac.error'

from warnings import warnpy3k
warnpy3k("In 3.x, the Play_Audio_mac module is removed.", stacklevel=2)

class Play_Audio_mac:

    def __init__(self, qsize=QSIZE):
        self._chan = None
        self._qsize = qsize
        self._outrate = 22254
        self._sampwidth = 1
        self._nchannels = 1
        self._gc = []
        self._usercallback = None

    def __del__(self):
        self.stop()
        self._usercallback = None

    def wait(self):
        import time
        while self.getfilled():
            time.sleep(0.1)
        self._chan = None
        self._gc = []

    def stop(self, quietNow = 1):
        ##chan = self._chan
        self._chan = None
        ##chan.SndDisposeChannel(1)
        self._gc = []

    def setoutrate(self, outrate):
        self._outrate = outrate

    def setsampwidth(self, sampwidth):
        self._sampwidth = sampwidth

    def setnchannels(self, nchannels):
        self._nchannels = nchannels

    def writeframes(self, data):
        import time
        from Carbon.Sound import bufferCmd, callBackCmd, extSH
        import struct
        import MacOS
        if not self._chan:
            from Carbon import Snd
            self._chan = Snd.SndNewChannel(5, 0, self._callback)
        nframes = len(data) / self._nchannels / self._sampwidth
        if len(data) != nframes * self._nchannels * self._sampwidth:
            raise error, 'data is not a whole number of frames'
        while self._gc and \
              self.getfilled() + nframes > \
                self._qsize / self._nchannels / self._sampwidth:
            time.sleep(0.1)
        if self._sampwidth == 1:
            import audioop
            data = audioop.add(data, '\x80'*len(data), 1)
        h1 = struct.pack('llHhllbbl',
            id(data)+MacOS.string_id_to_buffer,
            self._nchannels,
            self._outrate, 0,
            0,
            0,
            extSH,
            60,
            nframes)
        h2 = 22*'\0'
        h3 = struct.pack('hhlll',
            self._sampwidth*8,
            0,
            0,
            0,
            0)
        header = h1+h2+h3
        self._gc.append((header, data))
        self._chan.SndDoCommand((bufferCmd, 0, header), 0)
        self._chan.SndDoCommand((callBackCmd, 0, 0), 0)

    def _callback(self, *args):
        del self._gc[0]
        if self._usercallback:
            self._usercallback()

    def setcallback(self, callback):
        self._usercallback = callback

    def getfilled(self):
        filled = 0
        for header, data in self._gc:
            filled = filled + len(data)
        return filled / self._nchannels / self._sampwidth

    def getfillable(self):
        return (self._qsize / self._nchannels / self._sampwidth) - self.getfilled()

    def ulaw2lin(self, data):
        import audioop
        return audioop.ulaw2lin(data, 2)

def test():
    import aifc
    import EasyDialogs
    fn = EasyDialogs.AskFileForOpen(message="Select an AIFF soundfile", typeList=("AIFF",))
    if not fn: return
    af = aifc.open(fn, 'r')
    print af.getparams()
    p = Play_Audio_mac()
    p.setoutrate(af.getframerate())
    p.setsampwidth(af.getsampwidth())
    p.setnchannels(af.getnchannels())
    BUFSIZ = 10000
    while 1:
        data = af.readframes(BUFSIZ)
        if not data: break
        p.writeframes(data)
        print 'wrote', len(data), 'space', p.getfillable()
    p.wait()

if __name__ == '__main__':
    test()
