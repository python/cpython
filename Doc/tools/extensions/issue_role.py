"""Support for marking up and linking to the issues in the tracker."""

from __future__ import annotations

from collections.abc import Sequence
from typing import TYPE_CHECKING

from docutils import nodes
from docutils.parsers.rst.states import Inliner
from docutils.utils import unescape


if TYPE_CHECKING:
    from typing import Any

    from docutils.nodes import Node
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


ISSUE_URI = "https://bugs.python.org/issue?@action=redirect&bpo=%s"
GH_ISSUE_URI = "https://github.com/python/cpython/issues/%s"


def issue_role(
    typ: str,
    rawtext: str,
    text: str,
    lineno: int,
    inliner: Inliner,
    options: dict[str, Any] = {},
    content: Sequence[str] = [],
) -> tuple[list[Node], list[nodes.system_message]]:
    issue = unescape(text)
    # sanity check: there are no bpo issues within these two values
    if 47261 < int(issue) < 400000:
        msg = inliner.reporter.error(
            f"The BPO ID {text!r} seems too high -- "
            "use :gh:`...` for GitHub IDs",
            line=lineno,
        )
        prb = inliner.problematic(rawtext, rawtext, msg)
        return [prb], [msg]
    text = "bpo-" + issue
    refnode = nodes.reference(text, text, refuri=ISSUE_URI % issue)
    return [refnode], []


def gh_issue_role(
    typ: str,
    rawtext: str,
    text: str,
    lineno: int,
    inliner: Inliner,
    options: dict[str, Any] = {},
    content: Sequence[str] = [],
) -> tuple[list[Node], list[nodes.system_message]]:
    issue = unescape(text)
    # sanity check: all GitHub issues have ID >= 32426
    # even though some of them are also valid BPO IDs
    if int(issue) < 32426:
        msg = inliner.reporter.error(
            f"The GitHub ID {text!r} seems too low -- "
            "use :issue:`...` for BPO IDs",
            line=lineno,
        )
        prb = inliner.problematic(rawtext, rawtext, msg)
        return [prb], [msg]
    text = "gh-" + issue
    refnode = nodes.reference(text, text, refuri=GH_ISSUE_URI % issue)
    return [refnode], []


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_role("issue", issue_role)
    app.add_role("gh", gh_issue_role)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
