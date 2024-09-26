"""Support for the download page.

When the documentation is built and served from the docs server,
we prefer to link to the `up-to-date archives`_ instead of the
static releases on the python.org FTP site.
In an attempt to reduce confusion about these archives,
we re-title the page with the short version (major.minor)
instead of a full release version.

Contents:

* The ``:download-archive:`` role implements variable download links.
* The ``download_only_html()`` function prevents building the download
  page on non-HTML builders.

.. _up-to-date archives: https://docs.python.org/3/archives/
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxRole

if TYPE_CHECKING:
    from docutils.nodes import Node, system_message
    from sphinx.application import Sphinx


class DownloadArchiveRole(SphinxRole):
    def run(self) -> tuple[list[Node], list[system_message]]:
        tags = self.env.app.tags
        if "daily" in tags and "format_html" in tags:
            dl_base = "archives/"
            dl_version = self.config.version
        else:
            dl_base = "https://www.python.org/ftp/python/doc/"
            dl_version = self.config.release

        ref_text = sphinx_gettext("Download")
        uri = dl_base + self.text.replace("$VERSION", dl_version)
        refnode = nodes.reference("Download", ref_text, refuri=uri)
        return [refnode], []


def download_only_html(app: Sphinx) -> None:
    """Don't create the download page for non-HTML builders."""
    if "format_html" not in app.tags:
        app.config.exclude_patterns.append("download.rst")


def setup(app):
    app.add_role("download-archive", DownloadArchiveRole())
    app.connect("builder-inited", download_only_html)
