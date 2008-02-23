import sys, math

DOT = 30
DAH = 80
OCTAVE = 2                              # 1 == 441 Hz, 2 == 882 Hz, ...
SAMPWIDTH = 2
FRAMERATE = 44100
BASEFREQ = 441
QSIZE = 20000

morsetab = {
        'A': '.-',              'a': '.-',
        'B': '-...',            'b': '-...',
        'C': '-.-.',            'c': '-.-.',
        'D': '-..',             'd': '-..',
        'E': '.',               'e': '.',
        'F': '..-.',            'f': '..-.',
        'G': '--.',             'g': '--.',
        'H': '....',            'h': '....',
        'I': '..',              'i': '..',
        'J': '.---',            'j': '.---',
        'K': '-.-',             'k': '-.-',
        'L': '.-..',            'l': '.-..',
        'M': '--',              'm': '--',
        'N': '-.',              'n': '-.',
        'O': '---',             'o': '---',
        'P': '.--.',            'p': '.--.',
        'Q': '--.-',            'q': '--.-',
        'R': '.-.',             'r': '.-.',
        'S': '...',             's': '...',
        'T': '-',               't': '-',
        'U': '..-',             'u': '..-',
        'V': '...-',            'v': '...-',
        'W': '.--',             'w': '.--',
        'X': '-..-',            'x': '-..-',
        'Y': '-.--',            'y': '-.--',
        'Z': '--..',            'z': '--..',
        '0': '-----',
        '1': '.----',
        '2': '..---',
        '3': '...--',
        '4': '....-',
        '5': '.....',
        '6': '-....',
        '7': '--...',
        '8': '---..',
        '9': '----.',
        ',': '--..--',
        '.': '.-.-.-',
        '?': '..--..',
        ';': '-.-.-.',
        ':': '---...',
        "'": '.----.',
        '-': '-....-',
        '/': '-..-.',
        '(': '-.--.-',
        ')': '-.--.-',
        '_': '..--.-',
        ' ': ' '
}

# If we play at 44.1 kHz (which we do), then if we produce one sine
# wave in 100 samples, we get a tone of 441 Hz.  If we produce two
# sine waves in these 100 samples, we get a tone of 882 Hz.  882 Hz
# appears to be a nice one for playing morse code.
def mkwave(octave):
    global sinewave, nowave
    sinewave = ''
    n = int(FRAMERATE / BASEFREQ)
    for i in range(n):
        val = int(math.sin(2 * math.pi * i * octave / n) * 0x7fff)
        sample = chr((val >> 8) & 255) + chr(val & 255)
        sinewave = sinewave + sample[:SAMPWIDTH]
    nowave = '\0' * (n*SAMPWIDTH)

mkwave(OCTAVE)

class BufferedAudioDev:
    def __init__(self, *args):
        import audiodev
        self._base = apply(audiodev.AudioDev, args)
        self._buffer = []
        self._filled = 0
        self._addmethods(self._base, self._base.__class__)
    def _addmethods(self, inst, cls):
        for name in cls.__dict__.keys():
            if not hasattr(self, name):
                try:
                    setattr(self, name, getattr(inst, name))
                except:
                    pass
        for basecls in cls.__bases__:
            self._addmethods(self, inst, basecls)
    def writeframesraw(self, frames):
        self._buffer.append(frames)
        self._filled = self._filled + len(frames)
        if self._filled >= QSIZE:
            self.flush()
    def wait(self):
        self.flush()
        self._base.wait()
    def flush(self):
        print 'flush: %d blocks, %d bytes' % (len(self._buffer), self._filled)
        if self._buffer:
            import string
            self._base.writeframes(string.joinfields(self._buffer, ''))
            self._buffer = []
            self._filled = 0

def main(args = sys.argv[1:]):
    import getopt, string
    try:
        opts, args = getopt.getopt(args, 'o:p:')
    except getopt.error:
        sys.stderr.write('Usage ' + sys.argv[0] +
                         ' [ -o outfile ] [ args ] ...\n')
        sys.exit(1)
    dev = None
    for o, a in opts:
        if o == '-o':
            import aifc
            dev = aifc.open(a, 'w')
            dev.setframerate(FRAMERATE)
            dev.setsampwidth(SAMPWIDTH)
            dev.setnchannels(1)
        if o == '-p':
            mkwave(string.atoi(a))
    if not dev:
        dev = BufferedAudioDev()
        dev.setoutrate(FRAMERATE)
        dev.setsampwidth(SAMPWIDTH)
        dev.setnchannels(1)
        dev.close = dev.stop
    if args:
        line = string.join(args)
    else:
        line = sys.stdin.readline()
    while line:
        print line
        mline = morse(line)
        print mline
        play(mline, dev)
        if hasattr(dev, 'wait'):
            dev.wait()
        if not args:
            line = sys.stdin.readline()
        else:
            line = ''
    dev.close()

# Convert a string to morse code with \001 between the characters in
# the string.
def morse(line):
    res = ''
    for c in line:
        try:
            res = res + morsetab[c] + '\001'
        except KeyError:
            pass
    return res

# Play a line of morse code.
def play(line, dev):
    for c in line:
        if c == '.':
            sine(dev, DOT)
        elif c == '-':
            sine(dev, DAH)
        else:
            pause(dev, DAH)
        pause(dev, DOT)

def sine(dev, length):
    dev.writeframesraw(sinewave*length)

def pause(dev, length):
    dev.writeframesraw(nowave*length)

if __name__ == '__main__' or sys.argv[0] == __name__:
    main()
