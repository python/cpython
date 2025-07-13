import sys
from pathlib import Path


def main() -> int:
    wrong_directory_msg = "Must run this script from the repo root"
    if not Path("Doc").exists() or not Path("Doc").is_dir():
        raise RuntimeError(wrong_directory_msg)

    with Path("Doc/epubcheck.txt").open(encoding="UTF-8") as f:
        warnings = [warning.split(" - ") for warning in f.read().splitlines()]

    fatal_errors = [warning for warning in warnings if warning[0] == "FATAL"]

    if fatal_errors:
        print("\nError: must not contain fatal errors:\n")
        for warning in fatal_errors:
            print(" - ".join(warning))

    return len(fatal_errors)


if __name__ == "__main__":
    sys.exit(main())
