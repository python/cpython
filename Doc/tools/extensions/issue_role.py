"""Support for referencing issues in the tracker."""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.util.docutils import SphinxRole

if TYPE_CHECKING:
    from docutils.nodes import Element
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


class BPOIssue(SphinxRole):
    ISSUE_URI = "https://bugs.python.org/issue?@action=redirect&bpo={0}"

    def run(self) -> tuple[list[Element], list[nodes.system_message]]:
        issue = self.text

        # sanity check: there are no bpo issues within these two values
        if 47_261 < int(issue) < 400_000:
            msg = self.inliner.reporter.error(
                f"The BPO ID {issue!r} seems too high. "
                "Use :gh:`...` for GitHub IDs.",
                line=self.lineno,
            )
            prb = self.inliner.problematic(self.rawtext, self.rawtext, msg)
            return [prb], [msg]

        issue_url = self.ISSUE_URI.format(issue)
        refnode = nodes.reference(issue, f"bpo-{issue}", refuri=issue_url)
        self.set_source_info(refnode)
        return [refnode], []


class GitHubIssue(SphinxRole):
    ISSUE_URI = "https://github.com/python/cpython/issues/{0}"

    def run(self) -> tuple[list[Element], list[nodes.system_message]]:
        issue = self.text

        # sanity check: all GitHub issues have ID >= 32426
        # even though some of them are also valid BPO IDs
        if int(issue) < 32_426:
            msg = self.inliner.reporter.error(
                f"The GitHub ID {issue!r} seems too low. "
                "Use :issue:`...` for BPO IDs.",
                line=self.lineno,
            )
            prb = self.inliner.problematic(self.rawtext, self.rawtext, msg)
            return [prb], [msg]

        issue_url = self.ISSUE_URI.format(issue)
        refnode = nodes.reference(issue, f"gh-{issue}", refuri=issue_url)
        self.set_source_info(refnode)
        return [refnode], []


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_role("issue", BPOIssue())
    app.add_role("gh", GitHubIssue())

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
