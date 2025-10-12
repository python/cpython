#!/usr/bin/env python3

import argparse
import contextlib
import hashlib
import io
import json
import os
import pathlib
import shutil
import sys
import time
import urllib.error
import urllib.request
import zipfile


# Mapping of binary dependency tag to GitHub release asset ID
TAG_TO_ASSET_ID = {
    'llvm-20.1.8.0': 301710576,
}


def request_with_retry(request_func, *args, max_retries=7,
                       err_msg='Request failed.', **kwargs):
    """Make a request using request_func with exponential backoff"""
    for attempt in range(max_retries + 1):
        try:
            resp = request_func(*args, **kwargs)
        except (urllib.error.URLError, ConnectionError) as ex:
            if attempt == max_retries:
                raise OSError(err_msg) from ex
            time.sleep(2.25**attempt)
        else:
            return resp


def retrieve_with_retries(download_location, output_path, reporthook):
    """Download a file with retries."""
    return request_with_retry(
        urllib.request.urlretrieve,
        download_location,
        output_path,
        reporthook,
        err_msg=f'Download from {download_location} failed.',
    )


def get_with_retries(url, headers):
    req = urllib.request.Request(url=url, headers=headers, method='GET')
    return request_with_retry(
        urllib.request.urlopen,
        req,
        err_msg=f'Request to {url} failed.'
    )


def fetch_zip(commit_hash, zip_dir, *, org='python', binary=False, verbose=False):
    repo = 'cpython-bin-deps' if binary else 'cpython-source-deps'
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


def fetch_release_asset(asset_id, output_path, org):
    """Download a GitHub release asset.

    Release assets need the Content-Type header set to
    application/octet-stream to download the binary, so we can't use
    urlretrieve. Code here is based on urlretrieve.
    """
    url = f'https://api.github.com/repos/{org}/cpython-bin-deps/releases/assets/{asset_id}'
    metadata_resp = get_with_retries(url,
                                     headers={'Accept': 'application/vnd.github+json'})
    json_data = json.loads(metadata_resp.read())
    hash_info = json_data.get('digest')
    if not hash_info:
        raise RuntimeError(f'Release asset {asset_id} missing digest field in metadata')
    algorithm, hashsum = hash_info.split(':')
    if algorithm != 'sha256':
        raise RuntimeError(f'Unknown hash algorithm {algorithm} for asset {asset_id}')
    with contextlib.closing(
        get_with_retries(url, headers={'Accept': 'application/octet-stream'})
    ) as resp:
        hasher = hashlib.sha256()
        with open(output_path, 'wb') as fp:
            while block := resp.read(io.DEFAULT_BUFFER_SIZE):
                hasher.update(block)
                fp.write(block)
        if hasher.hexdigest() != hashsum:
            raise RuntimeError('Downloaded content hash did not match!')


def fetch_release(tag, tarball_dir, *, org='python', verbose=False):
    tarball_dir.mkdir(parents=True, exist_ok=True)
    asset_id = TAG_TO_ASSET_ID.get(tag)
    if asset_id is None:
        raise ValueError(f'Unknown tag for binary dependencies {tag}')
    output_path = tarball_dir / f'{tag}.tar.xz'
    fetch_release_asset(asset_id, output_path, org)
    return output_path


def extract_tarball(externals_dir, tarball_path, tag):
    output_path = externals_dir / tag
    shutil.unpack_archive(os.fspath(tarball_path), os.fspath(output_path))
    return output_path


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
    final_name = args.externals_dir / args.tag

    # Check if the dependency already exists in externals/ directory
    # (either already downloaded/extracted, or checked into the git tree)
    if final_name.exists():
        if args.verbose:
            print(f'{args.tag} already exists at {final_name}, skipping download.')
        return

    # Determine download method: release artifacts for large deps (like LLVM),
    # otherwise zip download from GitHub branches
    if args.tag in TAG_TO_ASSET_ID:
        tarball_path = fetch_release(
            args.tag,
            args.externals_dir / 'tarballs',
            org=args.organization,
            verbose=args.verbose,
        )
        extracted = extract_tarball(args.externals_dir, tarball_path, args.tag)
    else:
        # Use zip download from GitHub branches
        # (cpython-bin-deps if --binary, cpython-source-deps otherwise)
        zip_path = fetch_zip(
            args.tag,
            args.externals_dir / 'zips',
            org=args.organization,
            binary=args.binary,
            verbose=args.verbose,
        )
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
