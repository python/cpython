#! /usr/bin/env python

# Print prime numbers in a given range

def primes(min, max):
    if max >= 2 >= min:
        print 2
    primes = [2]
    i = 3
    while i <= max:
        for p in primes:
            if i % p == 0 or p*p > i:
                break
        if i % p != 0:
            primes.append(i)
            if i >= min:
                print i
        i += 2

def main():
    import sys
    min, max = 2, 0x7fffffff
    if sys.argv[1:]:
        min = int(sys.argv[1])
        if sys.argv[2:]:
            max = int(sys.argv[2])
    primes(min, max)

if __name__ == "__main__":
    main()
