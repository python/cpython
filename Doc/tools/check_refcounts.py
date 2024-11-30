import os
import sys
import argparse

def read_refcounts_file(file_path):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")
    with open(file_path, 'r') as file:
        lines = file.readlines()
    return [line.strip() for line in lines if line.strip()]

def read_stable_abi_file(file_path):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")
    with open(file_path, 'r') as file:
        lines = file.readlines()
    return [line.strip() for line in lines if line.strip()]

def is_sorted(file_lines):
    return file_lines == sorted(file_lines)

def check_refcounts(refcounts_file, stable_abi_file, auto_sort=False):
    refcounts = read_refcounts_file(refcounts_file)
    stable_abi = read_stable_abi_file(stable_abi_file)

    missing_entries = [entry for entry in stable_abi if entry not in refcounts]
    if missing_entries:
        print("Missing entries in refcounts.dat:")
        for entry in missing_entries:
            print(entry)
        print(f"Total entries in stable_abi.dat: {len(stable_abi)}")
        print(f"Total entries in refcounts.dat: {len(refcounts)}")
        print(f"Missing entries: {len(missing_entries)}")
        return False

    if not is_sorted(refcounts):
        print("refcounts.dat is not sorted alphabetically.")
        if auto_sort:
            with open(refcounts_file, 'w') as file:
                file.writelines('\n'.join(sorted(refcounts)) + '\n')
            print("refcounts.dat has been sorted.")
        return False

    print("refcounts.dat is up-to-date and sorted.")
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Validate refcounts.dat.")
    parser.add_argument("--refcounts", default='../data/refcounts.dat', help="Path to refcounts.dat")
    parser.add_argument("--stable_abi", default='../data/stable_abi.dat', help="Path to stable_abi.dat")
    parser.add_argument("--auto_sort", action='store_true', help="Auto-sort refcounts.dat if not sorted")
    args = parser.parse_args()

    if not check_refcounts(args.refcounts, args.stable_abi, args.auto_sort):
        sys.exit(1)
