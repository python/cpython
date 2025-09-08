"""Support for documenting platform availability"""

from __future__ import annotations

from typing import TYPE_CHECKING

from sphinx.util import logging
from support import SmartItemList

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata

logger = logging.getLogger("availability")

# known platform, libc, and threading implementations
_PLATFORMS = frozenset({
    "AIX",
    "Android",
    "BSD",
    "DragonFlyBSD",
    "Emscripten",
    "FreeBSD",
    "GNU/kFreeBSD",
    "iOS",
    "Linux",
    "macOS",
    "NetBSD",
    "OpenBSD",
    "POSIX",
    "Solaris",
    "Unix",
    "VxWorks",
    "WASI",
    "Windows",
})
_LIBC = frozenset({
    "BSD libc",
    "glibc",
    "musl",
})
_THREADING = frozenset({
    # POSIX platforms with pthreads
    "pthreads",
})
KNOWN_PLATFORMS = _PLATFORMS | _LIBC | _THREADING


class Availability(SmartItemList):
    """Parse platform information from arguments

    Arguments is a comma-separated string of platforms. A platform may
    be prefixed with "not " to indicate that a feature is not available.

    Example::

       .. availability:: Windows, Linux >= 4.2, not WASI

    Arguments like "Linux >= 3.17 with glibc >= 2.27" are currently not
    parsed into separate tokens.
    """

    title = "Availability"
    reftarget = "availability"
    classes = ["availability"]

    def check_information(self, platforms: dict[str, str | bool], /) -> None:
        unknown = platforms.keys() - KNOWN_PLATFORMS
        self._check_information(
            logger,
            f"{__file__}:KNOWN_PLATFORMS",
            unknown,
            ("platform", "platforms"),
            len(platforms),
        )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("availability", Availability)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
