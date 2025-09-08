from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx import addnodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence
    from logging import Logger
    from typing import ClassVar


class SmartItemList(SphinxDirective):
    title: ClassVar[str]
    reftarget: ClassVar[str]
    classes: ClassVar[Sequence[str]]

    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True

    def run(self) -> list[nodes.container]:
        title = sphinx_gettext(self.title)
        refnode = addnodes.pending_xref(
            title,
            nodes.inline(title, title, classes=["xref", "std", "std-ref"]),
            refdoc=self.env.docname,
            refdomain="std",
            refexplicit=True,
            reftarget=self.reftarget,
            reftype="ref",
            refwarn=True,
        )
        sep = nodes.Text(": ")
        parsed, msgs = self.state.inline_text(self.arguments[0], self.lineno)
        pnode = nodes.paragraph(title, "", refnode, sep, *parsed, *msgs)
        self.set_source_info(pnode)
        cnode = nodes.container("", pnode, classes=self.classes)
        self.set_source_info(cnode)
        if self.content:
            self.state.nested_parse(self.content, self.content_offset, cnode)

        items = self.parse_information()
        self.check_information(items)

        return [cnode]

    def parse_information(self) -> dict[str, str | bool]:
        """Parse information from arguments.

        Arguments is a comma-separated string of versioned named items.
        Each item may be prefixed with "not " to indicate that a feature
        is not available.

        Example::

           .. <directive>:: <item>, <item> >= <version>, not <item>

        Arguments like "<item> >= major.minor with <flavor> >= x.y.z" are
        currently not parsed into separate tokens.
        """
        items = {}
        for arg in self.arguments[0].rstrip(".").split(","):
            arg = arg.strip()
            name, _, version = arg.partition(" >= ")
            if name.startswith("not "):
                version = False
                name = name.removeprefix("not ")
            elif not version:
                version = True
            items[name] = version
        return items

    def check_information(self, items: dict[str, str | bool], /) -> None:
        raise NotImplementedError

    def _check_information(
        self,
        logger: Logger,
        seealso: str,
        unknown: Iterable[str],
        parsed_items_descr: tuple[str, str],
        parsed_items_count: int,
        /,
    ):
        if unknown := " ".join(sorted(unknown)):
            logger.warning(
                "Unknown %s or syntax '%s' in '.. %s:: %s', "
                "see %s for a set of known %s.",
                parsed_items_descr[int(parsed_items_count > 1)],
                unknown,
                self.name,
                self.arguments[0],
                seealso,
                parsed_items_descr[1],
            )
