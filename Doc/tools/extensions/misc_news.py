"""Support for including Misc/NEWS."""

from __future__ import annotations

import re
from pathlib import Path
from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from typing import Final

    from docutils.nodes import Node
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


BLURB_HEADER = """\
+++++++++++
Python News
+++++++++++
"""

bpo_issue_re: Final[re.Pattern[str]] = re.compile(
    "(?:issue #|bpo-)([0-9]+)", re.ASCII
)
gh_issue_re: Final[re.Pattern[str]] = re.compile(
    "gh-(?:issue-)?([0-9]+)", re.ASCII | re.IGNORECASE
)
whatsnew_re: Final[re.Pattern[str]] = re.compile(
    r"^what's new in (.*?)\??$", re.ASCII | re.IGNORECASE | re.MULTILINE
)


class MiscNews(SphinxDirective):
    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {}

    def run(self) -> list[Node]:
        # Get content of NEWS file
        source, _ = self.get_source_info()
        news_file = Path(source).resolve().parent / self.arguments[0]
        self.env.note_dependency(news_file)
        try:
            news_text = news_file.read_text(encoding="utf-8")
        except (OSError, UnicodeError):
            text = sphinx_gettext("The NEWS file is not available.")
            return [nodes.strong(text, text)]

        # remove first 3 lines as they are the main heading
        news_text = news_text.removeprefix(BLURB_HEADER)

        news_text = bpo_issue_re.sub(r":issue:`\1`", news_text)
        # Fallback handling for GitHub issues
        news_text = gh_issue_re.sub(r":gh:`\1`", news_text)
        news_text = whatsnew_re.sub(r"\1", news_text)

        self.state_machine.insert_input(news_text.splitlines(), str(news_file))
        return []


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("miscnews", MiscNews)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
