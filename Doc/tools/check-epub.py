import sys
from pathlib import Path


def main():
    wrong_directory_msg = "Must run this script from the repo root"
    if not Path("Doc").exists() or not Path("Doc").is_dir():
        raise RuntimeError(wrong_directory_msg)

    with Path("Doc/epubcheck.txt").open(encoding="UTF-8") as f:
        warnings = f.read().splitlines()

    warnings = [warning.split(" - ") for warning in warnings]

    fatals = [warning for warning in warnings if warning[0] == "FATAL"]

    if fatals:
        print("\nError: must not contain fatal errors:\n")
        for fatal in fatals:
            print(" - ".join(fatal))

    return len(fatals)


if __name__ == "__main__":
    sys.exit(main())
