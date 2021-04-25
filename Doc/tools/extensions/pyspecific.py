# -*- coding: utf-8 -*-
"""
    pyspecific.py
    ~~~~~~~~~~~~~

    Sphinx extension with Python doc-specific markup.

    :copyright: 2008-2014 by Georg Brandl.
    :license: Python license.
"""

import re
import io
from os import getenv, path
from time import asctime
from pprint import pformat
from docutils.io import StringOutput
from docutils.parsers.rst import Directive
from docutils.utils import new_document

from docutils import nodes, utils

from sphinx import addnodes
from sphinx.builders import Builder
try:
    from sphinx.errors import NoUri
except ImportError:
    from sphinx.environment import NoUri
from sphinx.locale import translators
from sphinx.util import status_iterator, logging
from sphinx.util.nodes import split_explicit_title
from sphinx.writers.text import TextWriter, TextTranslator
from sphinx.writers.latex import LaTeXTranslator

try:
    from sphinx.domains.python import PyFunction, PyMethod
except ImportError:
    from sphinx.domains.python import PyClassmember as PyMethod
    from sphinx.domains.python import PyModulelevel as PyFunction

# Support for checking for suspicious markup

import suspicious


ISSUE_URI = 'https://bugs.python.org/issue%s'
SOURCE_URI = 'https://github.com/python/cpython/tree/master/%s'

# monkey-patch reST parser to disable alphabetic and roman enumerated lists
from docutils.parsers.rst.states import Body
Body.enum.converters['loweralpha'] = \
    Body.enum.converters['upperalpha'] = \
    Body.enum.converters['lowerroman'] = \
    Body.enum.converters['upperroman'] = lambda x: None


# Support for marking up and linking to bugs.python.org issues

def issue_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    issue = utils.unescape(text)
    text = 'bpo-' + issue
    refnode = nodes.reference(text, text, refuri=ISSUE_URI % issue)
    return [refnode], []


# Support for linking to Python source files easily

def source_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    has_t, title, target = split_explicit_title(text)
    title = utils.unescape(title)
    target = utils.unescape(target)
    refnode = nodes.reference(title, title, refuri=SOURCE_URI % target)
    return [refnode], []


# Support for marking up implementation details

class ImplementationDetail(Directive):

    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = True

    # This text is copied to templates/dummy.html
    label_text = 'CPython implementation detail:'

    def run(self):
        pnode = nodes.compound(classes=['impl-detail'])
        label = translators['sphinx'].gettext(self.label_text)
        content = self.content
        add_text = nodes.strong(label, label)
        if self.arguments:
            n, m = self.state.inline_text(self.arguments[0], self.lineno)
            pnode.append(nodes.paragraph('', '', *(n + m)))
        self.state.nested_parse(content, self.content_offset, pnode)
        if pnode.children and isinstance(pnode[0], nodes.paragraph):
            content = nodes.inline(pnode[0].rawsource, translatable=True)
            content.source = pnode[0].source
            content.line = pnode[0].line
            content += pnode[0].children
            pnode[0].replace_self(nodes.paragraph('', '', content,
                                                  translatable=False))
            pnode[0].insert(0, add_text)
            pnode[0].insert(1, nodes.Text(' '))
        else:
            pnode.insert(0, nodes.paragraph('', '', add_text))
        return [pnode]


# Support for documenting platform availability

class Availability(Directive):

    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True

    def run(self):
        availability_ref = ':ref:`Availability <availability>`: '
        pnode = nodes.paragraph(availability_ref + self.arguments[0],
                                classes=["availability"],)
        n, m = self.state.inline_text(availability_ref, self.lineno)
        pnode.extend(n + m)
        n, m = self.state.inline_text(self.arguments[0], self.lineno)
        pnode.extend(n + m)
        return [pnode]


# Support for documenting audit event

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


class AuditEvent(Directive):

    has_content = True
    required_arguments = 1
    optional_arguments = 2
    final_argument_whitespace = True

    _label = [
        "Raises an :ref:`auditing event <auditing>` {name} with no arguments.",
        "Raises an :ref:`auditing event <auditing>` {name} with argument {args}.",
        "Raises an :ref:`auditing event <auditing>` {name} with arguments {args}.",
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

        label = translators['sphinx'].gettext(self._label[min(2, len(args))])
        text = label.format(name="``{}``".format(name),
                            args=", ".join("``{}``".format(a) for a in args if a))

        env = self.state.document.settings.env
        if not hasattr(env, 'all_audit_events'):
            env.all_audit_events = {}

        new_info = {
            'source': [],
            'args': args
        }
        info = env.all_audit_events.setdefault(name, new_info)
        if info is not new_info:
            if not self._do_args_match(info['args'], new_info['args']):
                self.logger.warn(
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

        info['source'].append((env.docname, target))

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


class AuditEventListDirective(Directive):

    def run(self):
        return [audit_event_list('')]


# Support for documenting decorators

class PyDecoratorMixin(object):
    def handle_signature(self, sig, signode):
        ret = super(PyDecoratorMixin, self).handle_signature(sig, signode)
        signode.insert(0, addnodes.desc_addname('@', '@'))
        return ret

    def needs_arglist(self):
        return False


class PyDecoratorFunction(PyDecoratorMixin, PyFunction):
    def run(self):
        # a decorator function is a function after all
        self.name = 'py:function'
        return PyFunction.run(self)


# TODO: Use sphinx.domains.python.PyDecoratorMethod when possible
class PyDecoratorMethod(PyDecoratorMixin, PyMethod):
    def run(self):
        self.name = 'py:method'
        return PyMethod.run(self)


class PyCoroutineMixin(object):
    def handle_signature(self, sig, signode):
        ret = super(PyCoroutineMixin, self).handle_signature(sig, signode)
        signode.insert(0, addnodes.desc_annotation('coroutine ', 'coroutine '))
        return ret


class PyAwaitableMixin(object):
    def handle_signature(self, sig, signode):
        ret = super(PyAwaitableMixin, self).handle_signature(sig, signode)
        signode.insert(0, addnodes.desc_annotation('awaitable ', 'awaitable '))
        return ret


class PyCoroutineFunction(PyCoroutineMixin, PyFunction):
    def run(self):
        self.name = 'py:function'
        return PyFunction.run(self)


class PyCoroutineMethod(PyCoroutineMixin, PyMethod):
    def run(self):
        self.name = 'py:method'
        return PyMethod.run(self)


class PyAwaitableFunction(PyAwaitableMixin, PyFunction):
    def run(self):
        self.name = 'py:function'
        return PyFunction.run(self)


class PyAwaitableMethod(PyAwaitableMixin, PyMethod):
    def run(self):
        self.name = 'py:method'
        return PyMethod.run(self)


class PyAbstractMethod(PyMethod):

    def handle_signature(self, sig, signode):
        ret = super(PyAbstractMethod, self).handle_signature(sig, signode)
        signode.insert(0, addnodes.desc_annotation('abstractmethod ',
                                                   'abstractmethod '))
        return ret

    def run(self):
        self.name = 'py:method'
        return PyMethod.run(self)


# Support for documenting version of removal in deprecations

class DeprecatedRemoved(Directive):
    has_content = True
    required_arguments = 2
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = {}

    _deprecated_label = 'Deprecated since version {deprecated}, will be removed in version {removed}'
    _removed_label = 'Deprecated since version {deprecated}, removed in version {removed}'

    def run(self):
        node = addnodes.versionmodified()
        node.document = self.state.document
        node['type'] = 'deprecated-removed'
        version = (self.arguments[0], self.arguments[1])
        node['version'] = version
        env = self.state.document.settings.env
        current_version = tuple(int(e) for e in env.config.version.split('.'))
        removed_version = tuple(int(e) for e in self.arguments[1].split('.'))
        if current_version < removed_version:
            label = self._deprecated_label
        else:
            label = self._removed_label

        label = translators['sphinx'].gettext(label)
        text = label.format(deprecated=self.arguments[0], removed=self.arguments[1])
        if len(self.arguments) == 3:
            inodes, messages = self.state.inline_text(self.arguments[2],
                                                      self.lineno+1)
            para = nodes.paragraph(self.arguments[2], '', *inodes, translatable=False)
            node.append(para)
        else:
            messages = []
        if self.content:
            self.state.nested_parse(self.content, self.content_offset, node)
        if len(node):
            if isinstance(node[0], nodes.paragraph) and node[0].rawsource:
                content = nodes.inline(node[0].rawsource, translatable=True)
                content.source = node[0].source
                content.line = node[0].line
                content += node[0].children
                node[0].replace_self(nodes.paragraph('', '', content, translatable=False))
            node[0].insert(0, nodes.inline('', '%s: ' % text,
                                           classes=['versionmodified']))
        else:
            para = nodes.paragraph('', '',
                                   nodes.inline('', '%s.' % text,
                                                classes=['versionmodified']),
                                   translatable=False)
            node.append(para)
        env = self.state.document.settings.env
        # deprecated pre-Sphinx-2 method
        if hasattr(env, 'note_versionchange'):
            env.note_versionchange('deprecated', version[0], node, self.lineno)
        # new method
        else:
            env.get_domain('changeset').note_changeset(node)
        return [node] + messages


# Support for including Misc/NEWS

issue_re = re.compile('(?:[Ii]ssue #|bpo-)([0-9]+)')
whatsnew_re = re.compile(r"(?im)^what's new in (.*?)\??$")


class MiscNews(Directive):
    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {}

    def run(self):
        fname = self.arguments[0]
        source = self.state_machine.input_lines.source(
            self.lineno - self.state_machine.input_offset - 1)
        source_dir = getenv('PY_MISC_NEWS_DIR')
        if not source_dir:
            source_dir = path.dirname(path.abspath(source))
        fpath = path.join(source_dir, fname)
        self.state.document.settings.record_dependencies.add(fpath)
        try:
            with io.open(fpath, encoding='utf-8') as fp:
                content = fp.read()
        except Exception:
            text = 'The NEWS file is not available.'
            node = nodes.strong(text, text)
            return [node]
        content = issue_re.sub(r'`bpo-\1 <https://bugs.python.org/issue\1>`__',
                               content)
        content = whatsnew_re.sub(r'\1', content)
        # remove first 3 lines as they are the main heading
        lines = ['.. default-role:: obj', ''] + content.splitlines()[3:]
        self.state_machine.insert_input(lines, fname)
        return []


# Support for building "topic help" for pydoc

pydoc_topic_labels = [
    'assert', 'assignment', 'async', 'atom-identifiers', 'atom-literals',
    'attribute-access', 'attribute-references', 'augassign', 'await',
    'binary', 'bitwise', 'bltin-code-objects', 'bltin-ellipsis-object',
    'bltin-null-object', 'bltin-type-objects', 'booleans',
    'break', 'callable-types', 'calls', 'class', 'comparisons', 'compound',
    'context-managers', 'continue', 'conversions', 'customization', 'debugger',
    'del', 'dict', 'dynamic-features', 'else', 'exceptions', 'execmodel',
    'exprlists', 'floating', 'for', 'formatstrings', 'function', 'global',
    'id-classes', 'identifiers', 'if', 'imaginary', 'import', 'in', 'integers',
    'lambda', 'lists', 'naming', 'nonlocal', 'numbers', 'numeric-types',
    'objects', 'operator-summary', 'pass', 'power', 'raise', 'return',
    'sequence-types', 'shifting', 'slicings', 'specialattrs', 'specialnames',
    'string-methods', 'strings', 'subscriptions', 'truth', 'try', 'types',
    'typesfunctions', 'typesmapping', 'typesmethods', 'typesmodules',
    'typesseq', 'typesseq-mutable', 'unary', 'while', 'with', 'yield'
]


class PydocTopicsBuilder(Builder):
    name = 'pydoc-topics'

    default_translator_class = TextTranslator

    def init(self):
        self.topics = {}
        self.secnumbers = {}

    def get_outdated_docs(self):
        return 'all pydoc topics'

    def get_target_uri(self, docname, typ=None):
        return ''  # no URIs

    def write(self, *ignored):
        writer = TextWriter(self)
        for label in status_iterator(pydoc_topic_labels,
                                     'building topics... ',
                                     length=len(pydoc_topic_labels)):
            if label not in self.env.domaindata['std']['labels']:
                self.env.logger.warn('label %r not in documentation' % label)
                continue
            docname, labelid, sectname = self.env.domaindata['std']['labels'][label]
            doctree = self.env.get_and_resolve_doctree(docname, self)
            document = new_document('<section node>')
            document.append(doctree.ids[labelid])
            destination = StringOutput(encoding='utf-8')
            writer.write(document, destination)
            self.topics[label] = writer.output

    def finish(self):
        f = open(path.join(self.outdir, 'topics.py'), 'wb')
        try:
            f.write('# -*- coding: utf-8 -*-\n'.encode('utf-8'))
            f.write(('# Autogenerated by Sphinx on %s\n' % asctime()).encode('utf-8'))
            f.write(('topics = ' + pformat(self.topics) + '\n').encode('utf-8'))
        finally:
            f.close()


# Support for documenting Opcodes

opcode_sig_re = re.compile(r'(\w+(?:\+\d)?)(?:\s*\((.*)\))?')


def parse_opcode_signature(env, sig, signode):
    """Transform an opcode signature into RST nodes."""
    m = opcode_sig_re.match(sig)
    if m is None:
        raise ValueError
    opname, arglist = m.groups()
    signode += addnodes.desc_name(opname, opname)
    if arglist is not None:
        paramlist = addnodes.desc_parameterlist()
        signode += paramlist
        paramlist += addnodes.desc_parameter(arglist, arglist)
    return opname.strip()


# Support for documenting pdb commands

pdbcmd_sig_re = re.compile(r'([a-z()!]+)\s*(.*)')

# later...
# pdbargs_tokens_re = re.compile(r'''[a-zA-Z]+  |  # identifiers
#                                   [.,:]+     |  # punctuation
#                                   [\[\]()]   |  # parens
#                                   \s+           # whitespace
#                                   ''', re.X)


def parse_pdb_command(env, sig, signode):
    """Transform a pdb command signature into RST nodes."""
    m = pdbcmd_sig_re.match(sig)
    if m is None:
        raise ValueError
    name, args = m.groups()
    fullname = name.replace('(', '').replace(')', '')
    signode += addnodes.desc_name(name, name)
    if args:
        signode += addnodes.desc_addname(' '+args, ' '+args)
    return fullname


def process_audit_events(app, doctree, fromdocname):
    for node in doctree.traverse(audit_event_list):
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

    for node in doctree.traverse(audit_event_list):
        node.replace_self(table)


def setup(app):
    app.add_role('issue', issue_role)
    app.add_role('source', source_role)
    app.add_directive('impl-detail', ImplementationDetail)
    app.add_directive('availability', Availability)
    app.add_directive('audit-event', AuditEvent)
    app.add_directive('audit-event-table', AuditEventListDirective)
    app.add_directive('deprecated-removed', DeprecatedRemoved)
    app.add_builder(PydocTopicsBuilder)
    app.add_builder(suspicious.CheckSuspiciousMarkupBuilder)
    app.add_object_type('opcode', 'opcode', '%s (opcode)', parse_opcode_signature)
    app.add_object_type('pdbcommand', 'pdbcmd', '%s (pdb command)', parse_pdb_command)
    app.add_object_type('2to3fixer', '2to3fixer', '%s (2to3 fixer)')
    app.add_directive_to_domain('py', 'decorator', PyDecoratorFunction)
    app.add_directive_to_domain('py', 'decoratormethod', PyDecoratorMethod)
    app.add_directive_to_domain('py', 'coroutinefunction', PyCoroutineFunction)
    app.add_directive_to_domain('py', 'coroutinemethod', PyCoroutineMethod)
    app.add_directive_to_domain('py', 'awaitablefunction', PyAwaitableFunction)
    app.add_directive_to_domain('py', 'awaitablemethod', PyAwaitableMethod)
    app.add_directive_to_domain('py', 'abstractmethod', PyAbstractMethod)
    app.add_directive('miscnews', MiscNews)
    app.connect('doctree-resolved', process_audit_events)
    app.connect('env-merge-info', audit_events_merge)
    app.connect('env-purge-doc', audit_events_purge)
    return {'version': '1.0', 'parallel_read_safe': True}
