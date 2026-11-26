"""Support for marking up implementation details."""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


class ImplementationDetail(SphinxDirective):
    has_content = True
    final_argument_whitespace = True

    # This text is copied to templates/dummy.html
    label_text = sphinx_gettext("CPython implementation detail:")

    def run(self):
        self.assert_has_content()
        content_nodes = self.parse_content_to_nodes()

        # insert our prefix at the start of the first paragraph
        first_node = content_nodes[0]
        first_node[:0] = [
            nodes.strong(self.label_text, self.label_text),
            nodes.Text(" "),
        ]

        # create a new compound container node
        cnode = nodes.compound("", *content_nodes, classes=["impl-detail"])
        self.set_source_info(cnode)
        return [cnode]


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("impl-detail", ImplementationDetail)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
