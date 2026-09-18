"""When called with a single argument, simulated fgrep with a single
argument and no options."""

import sys

if __name__ == "__main__":
    pattern = sys.argv[1]
    for line in sys.stdin:
        if pattern in line:
            sys.stdout.write(line)
