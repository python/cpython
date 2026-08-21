"""Support for client-side redirects for removed HTML anchors."""

from __future__ import annotations

from typing import TYPE_CHECKING
from urllib.parse import urlsplit

from docutils import nodes
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


class AnchorMapEntryNode(nodes.Element):
    pass


class AnchorMap(SphinxDirective):
    has_content = True

    def run(self) -> list[nodes.Node]:
        self.assert_has_content()

        entries = []
        messages = []

        for index, line in enumerate(self.content):
            if not (line := line.strip()):
                continue

            old_anchor, sep, target = line.partition(": ")
            old_anchor, target = old_anchor.strip(), target.strip()

            if not sep or not old_anchor or not target:
                raise self.error(
                    "anchormap entries should be like: 'old-html-fragment: target'"
                )

            children, parse_messages = self.parse_inline(
                target,
                lineno=self.content_offset + index,
            )
            entry = AnchorMapEntryNode("", *children, old_anchor=old_anchor)
            self.set_source_info(entry)
            entries.append(entry)
            messages.extend(parse_messages)

        if not entries:
            raise self.error("anchormap must contain at least one entry")

        return entries + messages


def process_anchor_maps(
    app: Sphinx,
    doctree: nodes.document,
    _docname: str,
) -> None:
    redirects = {}

    for entry in list(doctree.findall(AnchorMapEntryNode)):
        target = None
        references = list(entry.findall(nodes.reference))

        if len(references) == 1:
            if refuri := references[0].get("refuri"):
                parts = urlsplit(refuri)
                if (
                    not parts.scheme and not parts.netloc
                ):  # Check it's internal
                    target = refuri
            elif refid := references[0].get("refid"):
                target = f"#{refid}"

        if target is not None:
            redirects[entry["old_anchor"]] = target

        entry.parent.remove(entry)

    if app.builder.format == "html" and not app.builder.embedded:
        doctree["anchor_redirects"] = redirects


def add_anchor_redirects_to_context(
    app: Sphinx,
    _pagename: str,
    _templatename: str,
    context: dict[str, object],
    doctree: nodes.document | None,
) -> None:
    if doctree is None:
        return

    if redirects := doctree.get("anchor_redirects"):
        context["anchor_redirects"] = redirects


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("anchormap", AnchorMap)
    app.add_node(AnchorMapEntryNode)
    app.connect("doctree-resolved", process_anchor_maps)
    app.connect("html-page-context", add_anchor_redirects_to_context)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
