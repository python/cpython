"""Support for documenting platform availability"""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx import addnodes
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

class Availability(SphinxDirective):

    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True

    # known platform, libc, and threading implementations
    known_platforms = frozenset({
        "AIX", "Android", "BSD", "DragonFlyBSD", "Emscripten", "FreeBSD",
        "GNU/kFreeBSD", "Linux", "NetBSD", "OpenBSD", "POSIX", "Solaris",
        "Unix", "VxWorks", "WASI", "Windows", "macOS", "iOS",
        # libc
        "BSD libc", "glibc", "musl",
        # POSIX platforms with pthreads
        "pthreads",
    })

    def run(self):
        availability_ref = ':ref:`Availability <availability>`: '
        avail_nodes, avail_msgs = self.state.inline_text(
            availability_ref + self.arguments[0],
            self.lineno)
        pnode = nodes.paragraph(availability_ref + self.arguments[0],
                                '', *avail_nodes, *avail_msgs)
        self.set_source_info(pnode)
        cnode = nodes.container("", pnode, classes=["availability"])
        self.set_source_info(cnode)
        if self.content:
            self.state.nested_parse(self.content, self.content_offset, cnode)
        self.parse_platforms()

        return [cnode]

    def parse_platforms(self):
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
                platform = platform[4:]
            elif not version:
                version = True
            platforms[platform] = version

        unknown = set(platforms).difference(self.known_platforms)
        if unknown:
            cls = type(self)
            logger = logging.getLogger(cls.__qualname__)
            logger.warning(
                f"Unknown platform(s) or syntax '{' '.join(sorted(unknown))}' "
                f"in '.. availability:: {self.arguments[0]}', see "
                f"{__file__}:{cls.__qualname__}.known_platforms for a set "
                "known platforms."
            )

        return platforms



def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("availability", Availability)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
