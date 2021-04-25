import ast
import sys
import time

from pegen.testutil import print_memstats


def main() -> None:
    t0 = time.time()
    for filename in sys.argv[1:]:
        print(filename, end="\r")
        try:
            with open(filename) as file:
                source = file.read()
            tree = ast.parse(source, filename)
        except Exception as err:
            print(f"{filename}: {err.__class__.__name__}: {err}", file=sys.stderr)
    tok = None
    t1 = time.time()
    dt = t1 - t0
    print(f"Parsed in {dt:.3f} secs", file=sys.stderr)
    print_memstats()


if __name__ == "__main__":
    main()
