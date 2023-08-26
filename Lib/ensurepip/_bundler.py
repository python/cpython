"""Build time dist downloading and bundling logic."""

from __future__ import annotations

import sys
from contextlib import suppress
from importlib.resources import as_file as _traversable_to_pathlib_ctx

from ._structs import BUNDLED_WHEELS_PATH, REMOTE_DIST_PKGS


def ensure_wheels_are_downloaded(*, verbosity: bool = False) -> None:
    """Download wheels into bundle if they are not there yet."""
    for pkg in REMOTE_DIST_PKGS:
        existing_whl_file_path = BUNDLED_WHEELS_PATH / pkg.wheel_file_name
        with suppress(FileNotFoundError):
            if pkg.matches(existing_whl_file_path.read_bytes()):
                if verbosity:
                    print(
                        f'A valid `{pkg.wheel_file_name}` is already '
                        'present in cache. Skipping download.',
                        file=sys.stderr,
                    )
                continue

        if verbosity:
            print(
                f'Downloading `{pkg.wheel_file_name}`...',
                file=sys.stderr,
            )
        downloaded_whl_contents = pkg.download_verified_wheel_contents()

        if verbosity:
            print(
                f'Saving `{pkg.wheel_file_name}` to disk...',
                file=sys.stderr,
            )
        with _traversable_to_pathlib_ctx(BUNDLED_WHEELS_PATH) as bundled_dir:
            whl_file_path = bundled_dir / pkg.wheel_file_name
            whl_file_path.write_bytes(downloaded_whl_contents)
