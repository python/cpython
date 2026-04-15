"""Support for documenting version of changes, additions, deprecations."""

from __future__ import annotations

from docutils import nodes
from sphinx import addnodes
from sphinx.domains.changeset import (
    VersionChange,
    versionlabel_classes,
    versionlabels,
)
from sphinx.locale import _ as sphinx_gettext

TYPE_CHECKING = False
if TYPE_CHECKING:
    from docutils.nodes import Node
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


def expand_version_arg(argument: str, release: str) -> str:
    """Expand "next" to the current version"""
    if argument == "next":
        return sphinx_gettext("{} (unreleased)").format(release)
    return argument


class PyVersionChange(VersionChange):
    def run(self) -> list[Node]:
        # Replace the 'next' special token with the current development version
        self.arguments[0] = expand_version_arg(
            self.arguments[0], self.config.release
        )
        return super().run()


class DeprecatedRemoved(VersionChange):
    required_arguments = 2

    _deprecated_label = sphinx_gettext(
        "Deprecated since version %s, will be removed in version %s"
    )
    _removed_label = sphinx_gettext(
        "Deprecated since version %s, removed in version %s"
    )

    def run(self) -> list[Node]:
        # Replace the first two arguments (deprecated version and removed version)
        # with a single tuple of both versions.
        version_deprecated = expand_version_arg(
            self.arguments[0], self.config.release
        )
        version_removed = self.arguments.pop(1)
        if version_removed == "next":
            raise ValueError(
                "deprecated-removed:: second argument cannot be `next`"
            )
        self.arguments[0] = version_deprecated, version_removed

        # Set the label based on if we have reached the removal version
        current_version = tuple(map(int, self.config.version.split(".")))
        removed_version = tuple(map(int, version_removed.split(".")))
        if current_version < removed_version:
            versionlabels[self.name] = self._deprecated_label
            versionlabel_classes[self.name] = "deprecated"
        else:
            versionlabels[self.name] = self._removed_label
            versionlabel_classes[self.name] = "removed"
        try:
            return super().run()
        finally:
            # reset versionlabels and versionlabel_classes
            versionlabels[self.name] = ""
            versionlabel_classes[self.name] = ""


class SoftDeprecated(PyVersionChange):
    """Directive for soft deprecations that auto-links to the glossary term.

    Usage::

        .. soft-deprecated:: 3.15

           Use :func:`new_thing` instead.

    Renders as: "Soft deprecated since version 3.15: Use new_thing() instead."
    with "Soft deprecated" linking to the glossary definition.
    """

    def run(self) -> list[Node]:
        versionlabels[self.name] = sphinx_gettext(
            "Soft deprecated since version %s"
        )
        versionlabel_classes[self.name] = "softdeprecated"
        try:
            result = super().run()
        finally:
            versionlabels[self.name] = ""
            versionlabel_classes[self.name] = ""

        for node in result:
            # Add "versionchanged" class so existing theme CSS applies
            node["classes"] = node.get("classes", []) + ["versionchanged"]
            # Replace the plain-text "Soft deprecated" with a glossary reference
            for inline in node.findall(nodes.inline):
                if "versionmodified" in inline.get("classes", []):
                    self._add_glossary_link(inline)

        return result

    @staticmethod
    def _add_glossary_link(inline: nodes.inline) -> None:
        """Replace 'Soft deprecated' text with a cross-reference to the
        :term:`soft deprecated` glossary entry."""
        marker = sphinx_gettext("Soft deprecated")
        ref = addnodes.pending_xref(
            "",
            nodes.Text(marker),
            refdomain="std",
            reftype="term",
            reftarget="soft deprecated",
            refwarn=True,
        )

        for child in inline.children:
            if not isinstance(child, nodes.Text):
                continue

            text = str(child)
            idx = text.find(marker)
            if idx < 0:
                continue

            # Replace the text node with the split parts using docutils API
            new_nodes: list[nodes.Node] = []
            if idx > 0:
                new_nodes.append(nodes.Text(text[:idx]))

            new_nodes.append(ref)
            remainder = text[idx + len(marker) :]
            if remainder:
                new_nodes.append(nodes.Text(remainder))

            child.parent.replace(child, new_nodes)
            break


def setup(app: Sphinx) -> ExtensionMetadata:
    # Override Sphinx's directives with support for 'next'
    app.add_directive("versionadded", PyVersionChange, override=True)
    app.add_directive("versionchanged", PyVersionChange, override=True)
    app.add_directive("versionremoved", PyVersionChange, override=True)
    app.add_directive("deprecated", PyVersionChange, override=True)

    # Register the ``.. deprecated-removed::`` directive
    app.add_directive("deprecated-removed", DeprecatedRemoved)

    # Register the ``.. soft-deprecated::`` directive
    app.add_directive("soft-deprecated", SoftDeprecated)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
