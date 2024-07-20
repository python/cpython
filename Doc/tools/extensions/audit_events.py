"""Support for documenting audit events."""

import re

from docutils import nodes

from sphinx.errors import NoUri
from sphinx.locale import _ as sphinx_gettext
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective


def audit_events_purge(app, env, docname):
    """This is to remove from env.all_audit_events old traces of removed
    documents.
    """
    if not hasattr(env, 'all_audit_events'):
        return
    fresh_all_audit_events = {}
    for name, event in env.all_audit_events.items():
        event["source"] = [(d, t) for d, t in event["source"] if d != docname]
        if event["source"]:
            # Only keep audit_events that have at least one source.
            fresh_all_audit_events[name] = event
    env.all_audit_events = fresh_all_audit_events


def audit_events_merge(app, env, docnames, other):
    """In Sphinx parallel builds, this merges env.all_audit_events from
    subprocesses.

    all_audit_events is a dict of names, with values like:
    {'source': [(docname, target), ...], 'args': args}
    """
    if not hasattr(other, 'all_audit_events'):
        return
    if not hasattr(env, 'all_audit_events'):
        env.all_audit_events = {}
    for name, value in other.all_audit_events.items():
        if name in env.all_audit_events:
            env.all_audit_events[name]["source"].extend(value["source"])
        else:
            env.all_audit_events[name] = value


class AuditEvent(SphinxDirective):

    has_content = True
    required_arguments = 1
    optional_arguments = 2
    final_argument_whitespace = True

    _label = [
        sphinx_gettext("Raises an :ref:`auditing event <auditing>` {name} with no arguments."),
        sphinx_gettext("Raises an :ref:`auditing event <auditing>` {name} with argument {args}."),
        sphinx_gettext("Raises an :ref:`auditing event <auditing>` {name} with arguments {args}."),
    ]

    @property
    def logger(self):
        cls = type(self)
        return logging.getLogger(cls.__module__ + "." + cls.__name__)

    def run(self):
        name = self.arguments[0]
        if len(self.arguments) >= 2 and self.arguments[1]:
            args = (a.strip() for a in self.arguments[1].strip("'\"").split(","))
            args = [a for a in args if a]
        else:
            args = []

        label = self._label[min(2, len(args))]
        text = label.format(name="``{}``".format(name),
                            args=", ".join("``{}``".format(a) for a in args if a))

        if not hasattr(self.env, 'all_audit_events'):
            self.env.all_audit_events = {}

        new_info = {
            'source': [],
            'args': args
        }
        info = self.env.all_audit_events.setdefault(name, new_info)
        if info is not new_info:
            if not self._do_args_match(info['args'], new_info['args']):
                self.logger.warning(
                    "Mismatched arguments for audit-event {}: {!r} != {!r}"
                    .format(name, info['args'], new_info['args'])
                )

        ids = []
        try:
            target = self.arguments[2].strip("\"'")
        except (IndexError, TypeError):
            target = None
        if not target:
            target = "audit_event_{}_{}".format(
                re.sub(r'\W', '_', name),
                len(info['source']),
            )
            ids.append(target)

        info['source'].append((self.env.docname, target))

        pnode = nodes.paragraph(text, classes=["audit-hook"], ids=ids)
        pnode.line = self.lineno
        if self.content:
            self.state.nested_parse(self.content, self.content_offset, pnode)
        else:
            n, m = self.state.inline_text(text, self.lineno)
            pnode.extend(n + m)

        return [pnode]

    # This list of sets are allowable synonyms for event argument names.
    # If two names are in the same set, they are treated as equal for the
    # purposes of warning. This won't help if number of arguments is
    # different!
    _SYNONYMS = [
        {"file", "path", "fd"},
    ]

    def _do_args_match(self, args1, args2):
        if args1 == args2:
            return True
        if len(args1) != len(args2):
            return False
        for a1, a2 in zip(args1, args2):
            if a1 == a2:
                continue
            if any(a1 in s and a2 in s for s in self._SYNONYMS):
                continue
            return False
        return True


class audit_event_list(nodes.General, nodes.Element):
    pass


class AuditEventListDirective(SphinxDirective):

    def run(self):
        return [audit_event_list('')]


def process_audit_events(app, doctree, fromdocname):
    for node in doctree.findall(audit_event_list):
        break
    else:
        return

    env = app.builder.env

    table = nodes.table(cols=3)
    group = nodes.tgroup(
        '',
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

    row = nodes.row()
    row += nodes.entry('', nodes.paragraph('', nodes.Text('Audit event')))
    row += nodes.entry('', nodes.paragraph('', nodes.Text('Arguments')))
    row += nodes.entry('', nodes.paragraph('', nodes.Text('References')))
    head += row

    for name in sorted(getattr(env, "all_audit_events", ())):
        audit_event = env.all_audit_events[name]

        row = nodes.row()
        node = nodes.paragraph('', nodes.Text(name))
        row += nodes.entry('', node)

        node = nodes.paragraph()
        for i, a in enumerate(audit_event['args']):
            if i:
                node += nodes.Text(", ")
            node += nodes.literal(a, nodes.Text(a))
        row += nodes.entry('', node)

        node = nodes.paragraph()
        backlinks = enumerate(sorted(set(audit_event['source'])), start=1)
        for i, (doc, label) in backlinks:
            if isinstance(label, str):
                ref = nodes.reference("", nodes.Text("[{}]".format(i)), internal=True)
                try:
                    ref['refuri'] = "{}#{}".format(
                        app.builder.get_relative_uri(fromdocname, doc),
                        label,
                    )
                except NoUri:
                    continue
                node += ref
        row += nodes.entry('', node)

        body += row

    for node in doctree.findall(audit_event_list):
        node.replace_self(table)


def setup(app):
    app.add_directive('audit-event', AuditEvent)
    app.add_directive('audit-event-table', AuditEventListDirective)
    app.connect('doctree-resolved', process_audit_events)
    app.connect('env-merge-info', audit_events_merge)
    app.connect('env-purge-doc', audit_events_purge)
    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
