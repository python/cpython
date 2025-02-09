"""Support for documenting platform availability"""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx import addnodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

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


class Availability(SphinxDirective):
    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True

    def run(self) -> list[nodes.container]:
        title = sphinx_gettext("Availability")
        refnode = addnodes.pending_xref(
            title,
            nodes.inline(title, title, classes=["xref", "std", "std-ref"]),
            refdoc=self.env.docname,
            refdomain="std",
            refexplicit=True,
            reftarget="availability",
            reftype="ref",
            refwarn=True,
        )
        sep = nodes.Text(": ")
        parsed, msgs = self.state.inline_text(self.arguments[0], self.lineno)
        pnode = nodes.paragraph(title, "", refnode, sep, *parsed, *msgs)
        self.set_source_info(pnode)
        cnode = nodes.container("", pnode, classes=["availability"])
        self.set_source_info(cnode)
        if self.content:
            self.state.nested_parse(self.content, self.content_offset, cnode)
        self.parse_platforms()

        return [cnode]

    def parse_platforms(self) -> dict[str, str | bool]:
        """Parse platform information from arguments

        Arguments is a comma-separated string of platforms. A platform may
        be prefixed with "not " to indicate that a feature is not available.

        Example::

           .. availability:: Windows, Linux >= 4.2, not WASI

        Arguments like "Linux >= 3.17 with glibc >= 2.27" are currently not
        parsed into separate tokens.
        """
        platforms = {}
        for arg in self.arguments[0].rstrip(".").split(","):
            arg = arg.strip()
            platform, _, version = arg.partition(" >= ")
            if platform.startswith("not "):
                version = False
                platform = platform.removeprefix("not ")
            elif not version:
                version = True
            platforms[platform] = version

        if unknown := set(platforms).difference(KNOWN_PLATFORMS):
            logger.warning(
                "Unknown platform%s or syntax '%s' in '.. availability:: %s', "
                "see %s:KNOWN_PLATFORMS for a set of known platforms.",
                "s" if len(platforms) != 1 else "",
                " ".join(sorted(unknown)),
                self.arguments[0],
                __file__,
            )

        return platforms


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("availability", Availability)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
