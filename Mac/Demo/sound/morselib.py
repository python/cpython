"""Translate text strings to Morse code"""

FRAMERATE = 22050
SAMPWIDTH = 2

BASEFREQ = 441
OCTAVE = 2

DOT = 30
DAH = 80

morsetab = {
        'a': '.-',
        'b': '-...',
        'c': '-.-.',
        'd': '-..',
        'e': '.',
        'f': '..-.',
        'g': '--.',
        'h': '....',
        'i': '..',
        'j': '.---',
        'k': '-.-',
        'l': '.-..',
        'm': '--',
        'n': '-.',
        'o': '---',
        'p': '.--.',
        'q': '--.-',
        'r': '.-.',
        's': '...',
        't': '-',
        'u': '..-',
        'v': '...-',
        'w': '.--',
        'x': '-..-',
        'y': '-.--',
        'z': '--..',
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
        ')': '-.--.-', # XXX same as code for '(' ???
        '_': '..--.-',
        ' ': ' '
}

def morsecode(s):
    from string import lower
    m = ''
    for c in s:
        c = lower(c)
        if morsetab.has_key(c):
            c = morsetab[c] + ' '
        else:
            c = '? '
        m = m + c
    return m


class BaseMorse:
    "base class for morse transmissions"

    def __init__(self):
        "constructor"
        self.dots = DOT
        self.dahs = DAH

    def noise(self, duration):
        "beep for given duration"
        pass

    def pause(self, duration):
        "pause for given duration"
        pass

    def dot(self):
        "short beep"
        self.noise(self.dots)

    def dah(self):
        "long beep"
        self.noise(self.dahs)

    def pdot(self):
        "pause as long as a dot"
        self.pause(self.dots)

    def pdah(self):
        "pause as long as a dah"
        self.pause(self.dahs)

    def sendmorse(self, s):
        for c in s:
            if c == '.': self.dot()
            elif c == '-': self.dah()
            else: self.pdah()
            self.pdot()

    def sendascii(self, s):
        self.sendmorse(morsecode(s))

    def send(self, s):
        self.sendascii(s)


import Audio_mac
class MyAudio(Audio_mac.Play_Audio_mac):
    def _callback(self, *args):
        if hasattr(self, 'usercallback'): self.usercallback()
        apply(Audio_mac.Play_Audio_mac._callback, (self,) + args)


class MacMorse(BaseMorse):
    "Mac specific class to play Morse code"

    def __init__(self):
        BaseMorse.__init__(self)
        self.dev = MyAudio()
        self.dev.setoutrate(FRAMERATE)
        self.dev.setsampwidth(SAMPWIDTH)
        self.dev.setnchannels(1)
        self.dev.usercallback = self.usercallback
        sinewave = ''
        n = int(FRAMERATE / BASEFREQ)
        octave = OCTAVE
        from math import sin, pi
        for i in range(n):
            val = int(sin(2 * pi * i * octave / n) * 0x7fff)
            sample = chr((val >> 8) & 255) + chr(val & 255)
            sinewave = sinewave + sample[:SAMPWIDTH]
        self.sinewave = sinewave
        self.silence = '\0' * (n*SAMPWIDTH)
        self.morsequeue = ''

    def __del__(self):
        self.close()

    def close(self):
        self.dev = None

    def pause(self, duration):
        self.dev.writeframes(self.silence * duration)

    def noise(self, duration):
        self.dev.writeframes(self.sinewave * duration)

    def sendmorse(self, s):
        self.morsequeue = self.morsequeue + s
        self.dev.usercallback()
        self.dev.usercallback()
        self.dev.usercallback()

    def usercallback(self):
        if self.morsequeue:
            c, self.morsequeue = self.morsequeue[0], self.morsequeue[1:]
            if c == '.': self.dot()
            elif c == '-': self.dah()
            else: self.pdah()
            self.pdot()


def test():
    m = MacMorse()
    while 1:
        try:
            line = raw_input('Morse line: ')
        except (EOFError, KeyboardInterrupt):
            break
        m.send(line)
        while m.morsequeue: pass

test()
