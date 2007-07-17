import sys
import rcvs

def raw_input(prompt):
    sys.stdout.write(prompt)
    sys.stdout.flush()
    return sys.stdin.readline()

def main():
    while 1:
        try:
            line = input('$ ')
        except EOFError:
            break
        words = line.split()
        if not words:
            continue
        if words[0] != 'rcvs':
            words.insert(0, 'rcvs')
        sys.argv = words
        rcvs.main()

if __name__ == '__main__':
    main()
