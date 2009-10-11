#! /usr/bin/env python

# DAH should be three DOTs.
# Space between DOTs and DAHs should be one DOT.
# Space between two letters should be one DAH.
# Space between two words should be DOT DAH DAH.

import sys, math, audiodev

DOT = 30
DAH = 3 * DOT
OCTAVE = 2                              # 1 == 441 Hz, 2 == 882 Hz, ...

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
        '0': '-----',           ',': '--..--',
        '1': '.----',           '.': '.-.-.-',
        '2': '..---',           '?': '..--..',
        '3': '...--',           ';': '-.-.-.',
        '4': '....-',           ':': '---...',
        '5': '.....',           "'": '.----.',
        '6': '-....',           '-': '-....-',
        '7': '--...',           '/': '-..-.',
        '8': '---..',           '(': '-.--.-',
        '9': '----.',           ')': '-.--.-',
        ' ': ' ',               '_': '..--.-',
}

nowave = '\0' * 200

# If we play at 44.1 kHz (which we do), then if we produce one sine
# wave in 100 samples, we get a tone of 441 Hz.  If we produce two
# sine waves in these 100 samples, we get a tone of 882 Hz.  882 Hz
# appears to be a nice one for playing morse code.
def mkwave(octave):
    sinewave = ''
    for i in range(100):
        val = int(math.sin(math.pi * i * octave / 50.0) * 30000)
        sinewave += chr((val >> 8) & 255) + chr(val & 255)
    return sinewave

defaultwave = mkwave(OCTAVE)

def main():
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'o:p:')
    except getopt.error:
        sys.stderr.write('Usage ' + sys.argv[0] +
                         ' [ -o outfile ] [ -p octave ] [ words ] ...\n')
        sys.exit(1)
    dev = None
    wave = defaultwave
    for o, a in opts:
        if o == '-o':
            import aifc
            dev = aifc.open(a, 'w')
            dev.setframerate(44100)
            dev.setsampwidth(2)
            dev.setnchannels(1)
        if o == '-p':
            wave = mkwave(int(a))
    if not dev:
        import audiodev
        dev = audiodev.AudioDev()
        dev.setoutrate(44100)
        dev.setsampwidth(2)
        dev.setnchannels(1)
        dev.close = dev.stop
        dev.writeframesraw = dev.writeframes
    if args:
        source = [' '.join(args)]
    else:
        source = iter(sys.stdin.readline, '')
    for line in source:
        mline = morse(line)
        play(mline, dev, wave)
        if hasattr(dev, 'wait'):
            dev.wait()
    dev.close()

# Convert a string to morse code with \001 between the characters in
# the string.
def morse(line):
    res = ''
    for c in line:
        try:
            res += morsetab[c] + '\001'
        except KeyError:
            pass
    return res

# Play a line of morse code.
def play(line, dev, wave):
    for c in line:
        if c == '.':
            sine(dev, DOT, wave)
        elif c == '-':
            sine(dev, DAH, wave)
        else:                   # space
            pause(dev, DAH + DOT)
        pause(dev, DOT)

def sine(dev, length, wave):
    for i in range(length):
        dev.writeframesraw(wave)

def pause(dev, length):
    for i in range(length):
        dev.writeframesraw(nowave)

if __name__ == '__main__':
    main()
