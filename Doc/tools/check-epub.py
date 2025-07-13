import sys
from pathlib import Path


def main() -> int:
    if not Path("Doc").exists() or not Path("Doc").is_dir():
        raise RuntimeError("Must run this script from the repo root")

    with Path("Doc/epubcheck.txt").open(encoding="UTF-8") as f:
        warnings = [warning.split(" - ") for warning in f.read().splitlines()]

    fatal_errors = [warning for warning in warnings if warning[0] == "FATAL"]

    if fatal_errors:
        print("\nError: must not contain fatal errors:\n")
        for error in fatal_errors:
            print(" - ".join(error))

    return len(fatal_errors)


if __name__ == "__main__":
    sys.exit(main())
