"""Support for documenting system library requirements."""

from __future__ import annotations

from typing import TYPE_CHECKING

from sphinx.util import logging
from support import SmartItemList

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


logger = logging.getLogger("requirements")

# Known non-vendored dependencies.
KNOWN_REQUIREMENTS = frozenset({
    # libssl implementations
    "OpenSSL",
    "AWS-LC",
    "BoringSSL",
    "LibreSSL",
})


class Requirements(SmartItemList):
    """Parse dependencies information from arguments.

    Arguments is a comma-separated string of dependencies. A dependency may
    be prefixed with "not " to indicate that a feature is not available if
    it was used.

    Example::

       .. availability:: OpenSSL >= 3.5, not BoringSSL

    Arguments like "OpenSSL >= 3.5 with FIPS mode on" are currently not
    parsed into separate tokens.
    """

    title = "Requirements"
    reftarget = "requirements-notes"
    classes = ["requirements"]

    def check_information(self, requirements, /):
        unknown = requirements.keys() - KNOWN_REQUIREMENTS
        self._check_information(
            logger,
            f"{__file__}:KNOWN_REQUIREMENTS",
            unknown,
            ("requirement", "requirements"),
            len(requirements),
        )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("requirements", Requirements)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
