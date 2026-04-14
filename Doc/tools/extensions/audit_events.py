"""Support for documenting audit events."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.errors import NoUri
from sphinx.locale import _ as sphinx_gettext
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from collections.abc import Iterator, Set

    from sphinx.application import Sphinx
    from sphinx.builders import Builder
    from sphinx.environment import BuildEnvironment

logger = logging.getLogger(__name__)

# This list of sets are allowable synonyms for event argument names.
# If two names are in the same set, they are treated as equal for the
# purposes of warning. This won't help if the number of arguments is
# different!
_SYNONYMS = [
    frozenset({"file", "path", "fd"}),
]


class AuditEvents:
    def __init__(self) -> None:
        self.events: dict[str, list[str]] = {}
        self.sources: dict[str, set[tuple[str, str]]] = {}

    def __iter__(self) -> Iterator[tuple[str, list[str], tuple[str, str]]]:
        for name, args in self.events.items():
            for source in self.sources[name]:
                yield name, args, source

    def add_event(
        self, name, args: list[str], source: tuple[str, str]
    ) -> None:
        if name in self.events:
            self._check_args_match(name, args)
        else:
            self.events[name] = args
        self.sources.setdefault(name, set()).add(source)

    def _check_args_match(self, name: str, args: list[str]) -> None:
        current_args = self.events[name]
        msg = (
            f"Mismatched arguments for audit-event {name}: "
            f"{current_args!r} != {args!r}"
        )
        if current_args == args:
            return
        if len(current_args) != len(args):
            logger.warning(msg)
            return
        for a1, a2 in zip(current_args, args, strict=False):
            if a1 == a2:
                continue
            if any(a1 in s and a2 in s for s in _SYNONYMS):
                continue
            logger.warning(msg)
            return

    def id_for(self, name) -> str:
        source_count = len(self.sources.get(name, set()))
        name_clean = re.sub(r"\W", "_", name)
        return f"audit_event_{name_clean}_{source_count}"

    def rows(self) -> Iterator[tuple[str, list[str], Set[tuple[str, str]]]]:
        for name in sorted(self.events.keys()):
            yield name, self.events[name], self.sources[name]


def initialise_audit_events(app: Sphinx) -> None:
    """Initialise the audit_events attribute on the environment."""
    if not hasattr(app.env, "audit_events"):
        app.env.audit_events = AuditEvents()


def audit_events_purge(
    app: Sphinx, env: BuildEnvironment, docname: str
) -> None:
    """This is to remove traces of removed documents from env.audit_events."""
    fresh_audit_events = AuditEvents()
    for name, args, (doc, target) in env.audit_events:
        if doc != docname:
            fresh_audit_events.add_event(name, args, (doc, target))


def audit_events_merge(
    app: Sphinx,
    env: BuildEnvironment,
    docnames: list[str],
    other: BuildEnvironment,
) -> None:
    """In Sphinx parallel builds, this merges audit_events from subprocesses."""
    for name, args, source in other.audit_events:
        env.audit_events.add_event(name, args, source)


class AuditEvent(SphinxDirective):
    has_content = True
    required_arguments = 1
    optional_arguments = 2
    final_argument_whitespace = True

    _label = [
        sphinx_gettext(
            "Raises an :ref:`auditing event <auditing>` "
            "{name} with no arguments."
        ),
        sphinx_gettext(
            "Raises an :ref:`auditing event <auditing>` "
            "{name} with argument {args}."
        ),
        sphinx_gettext(
            "Raises an :ref:`auditing event <auditing>` "
            "{name} with arguments {args}."
        ),
    ]

    def run(self) -> list[nodes.paragraph]:
        name = self.arguments[0]
        if len(self.arguments) >= 2 and self.arguments[1]:
            args = [
                arg
                for argument in self.arguments[1].strip("'\"").split(",")
                if (arg := argument.strip())
            ]
        else:
            args = []
        ids = []
        try:
            target = self.arguments[2].strip("\"'")
        except (IndexError, TypeError):
            target = None
        if not target:
            target = self.env.audit_events.id_for(name)
            ids.append(target)
        self.env.audit_events.add_event(name, args, (self.env.docname, target))

        node = nodes.paragraph("", classes=["audit-hook"], ids=ids)
        self.set_source_info(node)
        if self.content:
            node.rawsource = '\n'.join(self.content)  # for gettext
            self.state.nested_parse(self.content, self.content_offset, node)
        else:
            num_args = min(2, len(args))
            text = self._label[num_args].format(
                name=f"``{name}``",
                args=", ".join(f"``{a}``" for a in args),
            )
            node.rawsource = text  # for gettext
            parsed, messages = self.state.inline_text(text, self.lineno)
            node += parsed
            node += messages
        return [node]


class audit_event_list(nodes.General, nodes.Element):  # noqa: N801
    pass


class AuditEventListDirective(SphinxDirective):
    def run(self) -> list[audit_event_list]:
        return [audit_event_list()]


class AuditEventListTransform(SphinxPostTransform):
    default_priority = 500

    def run(self) -> None:
        if self.document.next_node(audit_event_list) is None:
            return

        table = self._make_table(self.app.builder, self.env.docname)
        for node in self.document.findall(audit_event_list):
            node.replace_self(table)

    def _make_table(self, builder: Builder, docname: str) -> nodes.table:
        table = nodes.table(cols=3)
        group = nodes.tgroup(
            "",
            nodes.colspec(colwidth=30),
            nodes.colspec(colwidth=55),
            nodes.colspec(colwidth=15),
            cols=3,
        )
        head = nodes.thead()
        body = nodes.tbody()

        table += group
        group += head
        group += body

        head += nodes.row(
            "",
            nodes.entry("", nodes.paragraph("", "Audit event")),
            nodes.entry("", nodes.paragraph("", "Arguments")),
            nodes.entry("", nodes.paragraph("", "References")),
        )

        for name, args, sources in builder.env.audit_events.rows():
            body += self._make_row(builder, docname, name, args, sources)

        return table

    @staticmethod
    def _make_row(
        builder: Builder,
        docname: str,
        name: str,
        args: list[str],
        sources: Set[tuple[str, str]],
    ) -> nodes.row:
        row = nodes.row()
        name_node = nodes.paragraph("", nodes.Text(name))
        row += nodes.entry("", name_node)

        args_node = nodes.paragraph()
        for arg in args:
            args_node += nodes.literal(arg, arg)
            args_node += nodes.Text(", ")
        if len(args_node.children) > 0:
            args_node.children.pop()  # remove trailing comma
        row += nodes.entry("", args_node)

        backlinks_node = nodes.paragraph()
        backlinks = enumerate(sorted(sources), start=1)
        for i, (doc, label) in backlinks:
            if isinstance(label, str):
                ref = nodes.reference("", f"[{i}]", internal=True)
                try:
                    target = (
                        f"{builder.get_relative_uri(docname, doc)}#{label}"
                    )
                except NoUri:
                    continue
                else:
                    ref["refuri"] = target
                    backlinks_node += ref
        row += nodes.entry("", backlinks_node)
        return row


def setup(app: Sphinx):
    app.add_directive("audit-event", AuditEvent)
    app.add_directive("audit-event-table", AuditEventListDirective)
    app.add_post_transform(AuditEventListTransform)
    app.connect("builder-inited", initialise_audit_events)
    app.connect("env-purge-doc", audit_events_purge)
    app.connect("env-merge-info", audit_events_merge)
    return {
        "version": "2.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
