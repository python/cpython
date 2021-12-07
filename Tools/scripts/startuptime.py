# Quick script to time startup for various binaries

import subprocess
import sys
import time

NREPS = 100


def main():
    binaries = sys.argv[1:]
    for bin in binaries:
        t0 = time.time()
        for _ in range(NREPS):
            result = subprocess.run([bin, "-c", "pass"])
            result.check_returncode()
        t1 = time.time()
        print(f"{(t1-t0)/NREPS:6.3f} {bin}")


if __name__ == "__main__":
    main()
