"""Spit out the Python reserved words table."""

import string

raw_words = """
and       del       for       is        raise    
assert    elif      from      lambda    return   
break     else      global    not       try      
class     except    if        or        while    
continue  exec      import    pass               
def       finally   in        print              
"""

ncols = 5

def main():
    words = string.split(raw_words)
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
