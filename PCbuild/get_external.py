#!/usr/bin/env python3

import argparse
import os
import zipfile
from urllib.request import urlretrieve


def fetch_zip(commit_hash, zip_dir, *, org='python', binary=False, verbose):
    repo = f'cpython-{"bin" if binary else "source"}-deps'
    url = f'https://github.com/{org}/{repo}/archive/{commit_hash}.zip'
    reporthook = None
    if verbose:
        reporthook = print
    os.makedirs(zip_dir, exist_ok=True)
    filename, headers = urlretrieve(
        url,
        os.path.join(zip_dir, f'{commit_hash}.zip'),
        reporthook=reporthook,
    )
    return filename


def extract_zip(externals_dir, zip_path):
    with zipfile.ZipFile(zip_path) as zf:
        zf.extractall(externals_dir)
        return os.path.join(externals_dir, zf.namelist()[0].split('/')[0])


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('-v', '--verbose', action='store_true')
    p.add_argument('-b', '--binary', action='store_true',
                   help='Is the dependency in the binary repo?')
    p.add_argument('-O', '--organization',
                   help='Organization owning the deps repos', default='python')
    p.add_argument('-e', '--externals-dir',
                   help='Directory in which to store dependencies',
                   default=os.path.join(
                       os.path.dirname(                 # <src root>
                           os.path.dirname(             # '-> PCbuild
                               os.path.abspath(__file__)    # '-> here
                            )
                        ),
                        'externals'))
    p.add_argument('tag',
                   help='tag of the dependency')
    return p.parse_args()


def main():
    args = parse_args()
    zip_path = fetch_zip(
        args.tag,
        os.path.join(args.externals_dir, 'zips'),
        org=args.organization,
        binary=args.binary,
        verbose=args.verbose,
    )
    final_name = os.path.join(args.externals_dir, args.tag)
    os.rename(extract_zip(args.externals_dir, zip_path), final_name)


if __name__ == '__main__':
    main()
