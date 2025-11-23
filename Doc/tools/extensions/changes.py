"""Support for documenting version of changes, additions, deprecations."""

from __future__ import annotations

from typing import TYPE_CHECKING

from sphinx.domains.changeset import (
    VersionChange,
    versionlabel_classes,
    versionlabels,
)
from sphinx.locale import _ as sphinx_gettext

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


class TerribleHashableDictHack:
    def __init__(self, deprecated, removed):
        self.deprecated = deprecated
        self.removed = removed

    def __hash__(self):
        return hash((self.deprecated, self.removed))

    def __getitem__(self, name):
        return getattr(self, name)


class FormatToPctTranslator:
    def __getitem__(self, name):
        return f'%({name})s'


class DeprecatedRemoved(VersionChange):
    required_arguments = 2

    _deprecated_label = sphinx_gettext(
        "Deprecated since version {deprecated}, will be removed in version {removed}"
    )
    _removed_label = sphinx_gettext(
        "Deprecated since version {deprecated}, removed in version {removed}"
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
        self.arguments[0] = TerribleHashableDictHack(
            deprecated=version_deprecated,
            removed=version_removed,
        )

        # Set the label based on if we have reached the removal version
        current_version = tuple(map(int, self.config.version.split(".")))
        removed_version = tuple(map(int, version_removed.split(".")))
        if current_version < removed_version:
            versionlabels[self.name] = self._deprecated_label.format_map(
                FormatToPctTranslator()
            )
            versionlabel_classes[self.name] = "deprecated"
        else:
            versionlabels[self.name] = self._removed_label.format_map(
                FormatToPctTranslator()
            )
            versionlabel_classes[self.name] = "removed"
        try:
            return super().run()
        finally:
            # reset versionlabels and versionlabel_classes
            versionlabels[self.name] = ""
            versionlabel_classes[self.name] = ""


def setup(app: Sphinx) -> ExtensionMetadata:
    # Override Sphinx's directives with support for 'next'
    app.add_directive("versionadded", PyVersionChange, override=True)
    app.add_directive("versionchanged", PyVersionChange, override=True)
    app.add_directive("versionremoved", PyVersionChange, override=True)
    app.add_directive("deprecated", PyVersionChange, override=True)

    # Register the ``.. deprecated-removed::`` directive
    app.add_directive("deprecated-removed", DeprecatedRemoved)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
