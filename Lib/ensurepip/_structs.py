"""Data structures to make the control flow easy."""

from __future__ import annotations

from contextlib import AbstractContextManager, nullcontext
from hashlib import sha256 as _compute_sha256
from dataclasses import dataclass
from functools import cache, cached_property
from importlib.resources import (
    abc as _resources_abc,
    as_file as _traversable_to_pathlib_ctx,
    files as _get_traversable_dir_for,
)
from pathlib import Path
from sysconfig import get_config_var
from urllib.request import urlopen


BUNDLED_WHEELS_PATH: _resources_abc.Traversable = (
    _get_traversable_dir_for(__package__) / "_bundled"
)
"""Path to the wheels that CPython bundles within ensurepip."""

_WHL_TAG: str = 'py3'


@cache
def _convert_project_spec_to_wheel_name(
        project_name: str,
        project_version: str,
) -> str:
    return (
        f'{project_name !s}-{project_version !s}-'
        f'{_WHL_TAG !s}-none-any.whl'
    )


@cache
def _get_wheel_pkg_dir_from_sysconfig() -> Path:
    """Read path to wheels Linux downstream distributions prefer."""
    # `WHEEL_PKG_DIR` is a directory of system wheel packages. Some Linux
    # distribution packaging policies recommend against bundling dependencies.
    # For example, Fedora installs wheel packages in the
    # `/usr/share/python-wheels/` directory and don't install
    # the `ensurepip._bundled` package.
    try:
        return Path(get_config_var('WHEEL_PKG_DIR')).resolve()
    except TypeError as none_type_err:
        raise LookupError(
            'The compile-time `WHEEL_PKG_DIR` is unset so there is '
            'no place for looking up the wheels.',
        ) from none_type_err


@dataclass(frozen=True)
class _RemoteDistributionPackage:
    """Structure representing a wheel on PyPI."""

    project_name: str
    """PyPI project name."""

    project_version: str
    """PyPI project version."""

    wheel_sha256: str
    """PyPI wheel SHA-256 hash."""

    @cached_property
    def wheel_file_name(self) -> str:
        """Name of the wheel file on remote server."""
        return _convert_project_spec_to_wheel_name(
            self.project_name, self.project_version,
        )

    @cached_property
    def wheel_file_url(self) -> str:
        """URL to the wheel file on remote server."""
        return (
            f'https://files.pythonhosted.org/packages/{_WHL_TAG !s}/'
            f'{self.project_name[0] !s}/{self.project_name !s}/'
            f'{self.wheel_file_name !s}'
        )

    @cached_property
    def verifiable_wheel_file_url(self) -> str:
        """URL to the wheel file on remote server that includes hash."""
        return f'{self.wheel_file_url !s}#sha256={self.wheel_sha256 !s}'

    def download_verified_wheel_contents(self) -> memoryview:
        """Retrieve the remote wheel contents and verify its hash.

        :raises ValueError: If the recorded SHA-256 hash doesn't match
                            the downloaded payload.

        Returns the URL contents as a :py:class:`memoryview` object
        on success.
        """
        with urlopen(self.wheel_file_url) as downloaded_fd:
            resource_content = memoryview(downloaded_fd.read())

        if not self.matches(resource_content):
            raise ValueError(f"The payload's hash is invalid for {self !r}.")

        return resource_content

    def matches(
            self,
            wheel_content: bytes | memoryview,
            /,
    ) -> bool:
        """Verify the content's SHA-256 hash against recorded value."""
        return self.wheel_sha256 == _compute_sha256(wheel_content).hexdigest()

    def __repr__(self) -> str:
        """Render remote distribution package instance for humans."""
        return (
            f'{self.project_name !s} == {self.project_version !s}'
            f' @ {self.verifiable_wheel_file_url !s}'
        )


@dataclass(frozen=True)
class BundledDistributionPackage:
    """Structure representing a wheel under ``ensurepip/_bundled/``."""

    project_name: str
    """PyPI project name."""

    project_version: str
    """PyPI project version."""

    wheel_name: str
    """Wheel package file name."""

    wheel_path: _resources_abc.Traversable
    """Wheel package file path."""

    @classmethod
    def from_project_spec(
            cls,
            project_name: str,
            project_version: str,
            /,
    ) -> BundledDistributionPackage:
        """Create a replacement package from name and version spec."""
        wheel_name = _convert_project_spec_to_wheel_name(
            project_name, project_version,
        )
        wheel_path = BUNDLED_WHEELS_PATH / wheel_name
        return cls(project_name, project_version, wheel_name, wheel_path)

    @classmethod
    def from_remote_dist(
            cls,
            remote_distribution_pkg: _RemoteDistributionPackage,
            /,
    ) -> BundledDistributionPackage:
        """Create a replacement package from its remote counterpart."""
        return cls.from_project_spec(
            remote_distribution_pkg.project_name,
            remote_distribution_pkg.project_version,
        )

    def as_pathlib_ctx(self) -> AbstractContextManager[Path]:
        """Make a context manager exposing a :mod:`pathlib` instance."""
        return _traversable_to_pathlib_ctx(self.wheel_path)


@dataclass(frozen=True)
class ReplacementDistributionPackage:
    """Structure representing a wheel under ``WHEEL_PKG_DIR``."""

    project_name: str
    """PyPI project name."""

    project_version: str
    """PyPI project version."""

    wheel_name: str
    """Wheel package file name."""

    wheel_path: Path
    """Wheel package file path."""

    @classmethod
    def from_distribution_package_name(
            cls,
            dist_pkg_name: str,
            /,
    ) -> ReplacementDistributionPackage:
        """Look up a replacement package from name.

        :raises LookupError: If ``WHEEL_PKG_DIR`` is not set or the package
                             wheel is nowhere to be found.

        """
        wheel_file_name_prefix = f'{dist_pkg_name !s}-'

        wheel_pkg_dir = _get_wheel_pkg_dir_from_sysconfig()
        dist_matching_wheels = wheel_pkg_dir.glob(
            f'{wheel_file_name_prefix !s}*.whl',
        )

        try:
            first_matching_dist_wheel = sorted(dist_matching_wheels)[0]
        except IndexError as index_err:
            raise LookupError(
                '`WHEEL_PKG_DIR` does not contain any wheel files '
                f'for `{dist_pkg_name !s}`.',
            ) from index_err

        wheel_name = first_matching_dist_wheel.name
        dist_pkg_version = (
            # Extract '21.2.4' from 'pip-21.2.4-py3-none-any.whl'
            wheel_name.
            removeprefix(wheel_file_name_prefix).
            partition('-')[0]
        )

        return cls(
            dist_pkg_name, dist_pkg_version,
            wheel_name, first_matching_dist_wheel,
        )

    @classmethod
    def from_remote_dist(
            cls,
            remote_distribution_pkg: _RemoteDistributionPackage,
            /,
    ) -> ReplacementDistributionPackage:
        """Create a replacement package from its remote counterpart."""
        return cls.from_distribution_package_name(
            remote_distribution_pkg.project_name,
        )

    def as_pathlib_ctx(self) -> AbstractContextManager[Path]:
        """Make a context manager exposing a :mod:`pathlib` instance."""
        return nullcontext(self.wheel_path)


PIP_REMOTE_DIST = _RemoteDistributionPackage(
    'pip',
    '23.2.1',
    '7ccf472345f20d35bdc9d1841ff5f313260c2c33fe417f48c30ac46cccabf5be',
)
"""Pip distribution package on PyPI."""

REMOTE_DIST_PKGS = (
    PIP_REMOTE_DIST,
)
"""Distribution packages provisioned by ``ensurepip``."""
