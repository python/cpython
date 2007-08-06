#! /usr/bin/env python

class Markov:
    def __init__(self, histsize, choice):
        self.histsize = histsize
        self.choice = choice
        self.trans = {}
    def add(self, state, next):
        if state not in self.trans:
            self.trans[state] = [next]
        else:
            self.trans[state].append(next)
    def put(self, seq):
        n = self.histsize
        add = self.add
        add(None, seq[:0])
        for i in range(len(seq)):
            add(seq[max(0, i-n):i], seq[i:i+1])
        add(seq[len(seq)-n:], None)
    def get(self):
        choice = self.choice
        trans = self.trans
        n = self.histsize
        seq = choice(trans[None])
        while 1:
            subseq = seq[max(0, len(seq)-n):]
            options = trans[subseq]
            next = choice(options)
            if not next: break
            seq = seq + next
        return seq

def test():
    import sys, string, random, getopt
    args = sys.argv[1:]
    try:
        opts, args = getopt.getopt(args, '0123456789cdw')
    except getopt.error:
        print('Usage: markov [-#] [-cddqw] [file] ...')
        print('Options:')
        print('-#: 1-digit history size (default 2)')
        print('-c: characters (default)')
        print('-w: words')
        print('-d: more debugging output')
        print('-q: no debugging output')
        print('Input files (default stdin) are split in paragraphs')
        print('separated blank lines and each paragraph is split')
        print('in words by whitespace, then reconcatenated with')
        print('exactly one space separating words.')
        print('Output consists of paragraphs separated by blank')
        print('lines, where lines are no longer than 72 characters.')
    histsize = 2
    do_words = 0
    debug = 1
    for o, a in opts:
        if '-0' <= o <= '-9': histsize = eval(o[1:])
        if o == '-c': do_words = 0
        if o == '-d': debug = debug + 1
        if o == '-q': debug = 0
        if o == '-w': do_words = 1
    if not args: args = ['-']
    m = Markov(histsize, random.choice)
    try:
        for filename in args:
            if filename == '-':
                f = sys.stdin
                if f.isatty():
                    print('Sorry, need stdin from file')
                    continue
            else:
                f = open(filename, 'r')
            if debug: print('processing', filename, '...')
            text = f.read()
            f.close()
            paralist = string.splitfields(text, '\n\n')
            for para in paralist:
                if debug > 1: print('feeding ...')
                words = string.split(para)
                if words:
                    if do_words: data = tuple(words)
                    else: data = string.joinfields(words, ' ')
                    m.put(data)
    except KeyboardInterrupt:
        print('Interrupted -- continue with data read so far')
    if not m.trans:
        print('No valid input files')
        return
    if debug: print('done.')
    if debug > 1:
        for key in m.trans.keys():
            if key is None or len(key) < histsize:
                print(repr(key), m.trans[key])
        if histsize == 0: print(repr(''), m.trans[''])
        print()
    while 1:
        data = m.get()
        if do_words: words = data
        else: words = string.split(data)
        n = 0
        limit = 72
        for w in words:
            if n + len(w) > limit:
                print()
                n = 0
            print(w, end=' ')
            n = n + len(w) + 1
        print()
        print()

def tuple(list):
    if len(list) == 0: return ()
    if len(list) == 1: return (list[0],)
    i = len(list)/2
    return tuple(list[:i]) + tuple(list[i:])

if __name__ == "__main__":
    test()
