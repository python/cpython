#! /usr/bin/env python3

class Markov:
    def __init__(self, histsize, choice):
        self.histsize = histsize
        self.choice = choice
        self.trans = {}

    def add(self, state, next):
        self.trans.setdefault(state, []).append(next)

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
        while True:
            subseq = seq[max(0, len(seq)-n):]
            options = trans[subseq]
            next = choice(options)
            if not next:
                break
            seq += next
        return seq


def test():
    import sys, random, getopt
    args = sys.argv[1:]
    try:
        opts, args = getopt.getopt(args, '0123456789cdwq')
    except getopt.error:
        print('Usage: %s [-#] [-cddqw] [file] ...' % sys.argv[0])
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
        sys.exit(2)
    histsize = 2
    do_words = False
    debug = 1
    for o, a in opts:
        if '-0' <= o <= '-9': histsize = int(o[1:])
        if o == '-c': do_words = False
        if o == '-d': debug += 1
        if o == '-q': debug = 0
        if o == '-w': do_words = True
    if not args:
        args = ['-']

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
            paralist = text.split('\n\n')
            for para in paralist:
                if debug > 1: print('feeding ...')
                words = para.split()
                if words:
                    if do_words:
                        data = tuple(words)
                    else:
                        data = ' '.join(words)
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
    while True:
        data = m.get()
        if do_words:
            words = data
        else:
            words = data.split()
        n = 0
        limit = 72
        for w in words:
            if n + len(w) > limit:
                print()
                n = 0
            print(w, end=' ')
            n += len(w) + 1
        print()
        print()

if __name__ == "__main__":
    test()
