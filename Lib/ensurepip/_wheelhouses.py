"""Pre-bundled wheel discovery."""

from __future__ import annotations

from functools import cache

from ._structs import (
    BundledDistributionPackage,
    ReplacementDistributionPackage,
    REMOTE_DIST_PKGS,
)


def _discover_ondisk_bundled_packages() -> tuple[
        BundledDistributionPackage,
        ...,
]:
    return tuple(
        BundledDistributionPackage.from_remote_dist(remote_pkg)
        for remote_pkg in REMOTE_DIST_PKGS
    )


def _discover_ondisk_replacement_packages() -> tuple[
        ReplacementDistributionPackage,
        ...,
]:
    try:
        return tuple(
            ReplacementDistributionPackage.from_remote_dist(remote_pkg)
            for remote_pkg in REMOTE_DIST_PKGS
        )
    except LookupError:
        return ()


@cache
def discover_ondisk_packages() -> dict[
        str,
        BundledDistributionPackage | ReplacementDistributionPackage,
]:
    """Return a mapping of packages found on disk.

    The result is either a list of distribution packages from
    ``WHEEL_PKG_DIR`` xor the bundled one.
    """
    ondisk_packages = (
        _discover_ondisk_replacement_packages()
        or _discover_ondisk_bundled_packages()
    )
    return {pkg.project_name: pkg for pkg in ondisk_packages}
