import argparse
import os
import sys

from . import open as dbm_open, whichdb, error


def _whichdb_command(filenames):
    exit_code = 0

    for filename in filenames:
        if os.path.exists(filename):
            db_type = whichdb(filename)
            print(f"{db_type or 'UNKNOWN'} {filename}")
        else:
            print(f"Error: File '{filename}' not found", file=sys.stderr)
            exit_code = 1

    return exit_code


def _dump_command(filename):
    try:
        with dbm_open(filename, "r") as db:
            for key in db:
                print(f"{key!r}: {db[key]!r}")
        return 0
    except error:
        print(f"Error: Database '{filename}' not found", file=sys.stderr)
        return 1


def _reorganize_command(filename):
    try:
        with dbm_open(filename, "c") as db:
            if db.hasattr("reorganize"):
                db.reorganize()
                print(f"Reorganized database: '{filename}'", file=sys.stderr)
            else:
                print("Database type doesn't support reorganize method",
                      file=sys.stderr)
                return 1
        return 0
    except error:
        print(f"Error: Database '{filename}' not found or cannot be opened",
              file=sys.stderr)
        return 1


def main():
    parser = argparse.ArgumentParser(
        prog="python -m dbm", description="DBM toolkit"
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--whichdb",
        nargs="+",
        metavar="file",
        help="Identify database type for one or more files",
    )
    group.add_argument(
        "--dump", metavar="file", help="Display database contents"
    )
    group.add_argument(
        "--reorganize",
        metavar="file",
        help="Reorganize the database",
    )
    options = parser.parse_args()

    try:
        if options.whichdb:
            return _whichdb_command(options.whichdb)
        elif options.dump:
            return _dump_command(options.dump)
        elif options.reorganize:
            return _reorganize_command(options.reorganize)
    except KeyboardInterrupt:
        return 1


if __name__ == "__main__":
    sys.exit(main())
