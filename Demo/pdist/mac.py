import sys
import string
import rcvs

def main():
    while 1:
        try:
            line = raw_input('$ ')
        except EOFError:
            break
        words = string.split(line)
        if not words:
            continue
        if words[0] != 'rcvs':
            words.insert(0, 'rcvs')
        sys.argv = words
        rcvs.main()

main()
