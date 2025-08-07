#!/usr/bin/env python3

import argparse
import os
import pathlib
import sys
import time
import urllib.error
import urllib.request
import zipfile


def retrieve_with_retries(download_location, output_path, reporthook,
                          max_retries=7):
    """Download a file with exponential backoff retry and save to disk."""
    for attempt in range(max_retries + 1):
        try:
            resp = urllib.request.urlretrieve(
                download_location,
                output_path,
                reporthook=reporthook,
            )
        except (urllib.error.URLError, ConnectionError) as ex:
            if attempt == max_retries:
                msg = f"Download from {download_location} failed."
                raise OSError(msg) from ex
            time.sleep(2.25**attempt)
        else:
            return resp


def fetch_zip(commit_hash, zip_dir, *, org='python', binary=False, verbose):
    repo = f'cpython-{"bin" if binary else "source"}-deps'
    url = f'https://github.com/{org}/{repo}/archive/{commit_hash}.zip'
    reporthook = None
    if verbose:
        reporthook = print
    zip_dir.mkdir(parents=True, exist_ok=True)
    filename, _headers = retrieve_with_retries(
        url,
        zip_dir / f'{commit_hash}.zip',
        reporthook
    )
    return filename


def extract_zip(externals_dir, zip_path):
    with zipfile.ZipFile(os.fspath(zip_path)) as zf:
        zf.extractall(os.fspath(externals_dir))
        return externals_dir / zf.namelist()[0].split('/')[0]


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('-v', '--verbose', action='store_true')
    p.add_argument('-b', '--binary', action='store_true',
                   help='Is the dependency in the binary repo?')
    p.add_argument('-O', '--organization',
                   help='Organization owning the deps repos', default='python')
    p.add_argument('-e', '--externals-dir', type=pathlib.Path,
                   help='Directory in which to store dependencies',
                   default=pathlib.Path(__file__).parent.parent / 'externals')
    p.add_argument('tag',
                   help='tag of the dependency')
    return p.parse_args()


def main():
    args = parse_args()
    zip_path = fetch_zip(
        args.tag,
        args.externals_dir / 'zips',
        org=args.organization,
        binary=args.binary,
        verbose=args.verbose,
    )
    final_name = args.externals_dir / args.tag
    extracted = extract_zip(args.externals_dir, zip_path)
    for wait in [1, 2, 3, 5, 8, 0]:
        try:
            extracted.replace(final_name)
            break
        except PermissionError as ex:
            retry = f" Retrying in {wait}s..." if wait else ""
            print(f"Encountered permission error '{ex}'.{retry}", file=sys.stderr)
            time.sleep(wait)
    else:
        print(
            f"ERROR: Failed to extract {final_name}.",
            "You may need to restart your build",
            file=sys.stderr,
        )
        sys.exit(1)


if __name__ == '__main__':
    main()
