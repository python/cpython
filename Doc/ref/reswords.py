"""Spit out the Python reserved words table."""

import keyword

ncols = 5

def main():
    words = keyword.kwlist[:]
    words.sort()
    colwidth = 1 + max(map(len, words))
    nwords = len(words)
    nrows = (nwords + ncols - 1) / ncols
    for irow in range(nrows):
        for icol in range(ncols):
            i = irow + icol * nrows
            if 0 <= i < nwords:
                word = words[i]
            else:
                word = ""
            print "%-*s" % (colwidth, word),
        print

main()
