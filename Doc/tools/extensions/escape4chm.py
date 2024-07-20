"""
Escape the `body` part of .chm source file to 7-bit ASCII,
to fix visual effects on some MBCS Windows systems.

https://github.com/python/cpython/issues/76355
"""

from __future__ import annotations

import re
from html.entities import codepoint2name
from pathlib import Path
from typing import TYPE_CHECKING

from sphinx.application import Sphinx
from sphinx.util import logging

if TYPE_CHECKING:
    from typing import Any

    from docutils import nodes
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata

logger = logging.getLogger(__name__)


def escape_for_chm(
    app: Sphinx,
    _page_name: str,
    _template_name: str,
    context: dict[str, Any],
    _doctree: nodes.document,
) -> None:
    """Escape the characters with a codepoint over ``0x7F``."""
    # only works for .chm output
    if app.builder.name != "htmlhelp":
        return

    # escape the `body` part to 7-bit ASCII
    body = context.get("body")
    if body is not None:
        context["body"] = re.sub(r"[^\x00-\x7F]", _escape, body)


def _escape(match: re.Match[str]) -> str:
    codepoint = ord(match.group(0))
    if codepoint in codepoint2name:
        return f"&{codepoint2name[codepoint]};"
    return f"&#{codepoint};"


def fixup_keywords(app: Sphinx, exception: Exception) -> None:
    # only works for .chm output
    if exception or app.builder.name != "htmlhelp":
        return

    logger.info("Fixing HTML escapes in keywords file...")
    keywords_path = Path(app.outdir) / f"{app.config.htmlhelp_basename}.hhk"
    index = keywords_path.read_bytes()
    keywords_path.write_bytes(index.replace(b"&#x27;", b"&#39;"))


def setup(app: Sphinx) -> ExtensionMetadata:
    app.connect("html-page-context", escape_for_chm)
    app.connect("build-finished", fixup_keywords)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
