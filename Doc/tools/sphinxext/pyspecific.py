# -*- coding: utf-8 -*-
"""
    pyspecific.py
    ~~~~~~~~~~~~~

    Sphinx extension with Python doc-specific markup.

    :copyright: 2008, 2009 by Georg Brandl.
    :license: Python license.
"""

ISSUE_URI = 'http://bugs.python.org/issue%s'

from docutils import nodes, utils

# monkey-patch reST parser to disable alphabetic and roman enumerated lists
from docutils.parsers.rst.states import Body
Body.enum.converters['loweralpha'] = \
    Body.enum.converters['upperalpha'] = \
    Body.enum.converters['lowerroman'] = \
    Body.enum.converters['upperroman'] = lambda x: None

# monkey-patch HTML translator to give versionmodified paragraphs a class
def new_visit_versionmodified(self, node):
    self.body.append(self.starttag(node, 'p', CLASS=node['type']))
    text = versionlabels[node['type']] % node['version']
    if len(node):
        text += ': '
    else:
        text += '.'
    self.body.append('<span class="versionmodified">%s</span>' % text)

from sphinx.writers.html import HTMLTranslator
from sphinx.locale import versionlabels
HTMLTranslator.visit_versionmodified = new_visit_versionmodified


# Support for marking up and linking to bugs.python.org issues

def issue_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    issue = utils.unescape(text)
    text = 'issue ' + issue
    refnode = nodes.reference(text, text, refuri=ISSUE_URI % issue)
    return [refnode], []


# Support for marking up implementation details

from sphinx.util.compat import Directive

class ImplementationDetail(Directive):

    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = True

    def run(self):
        pnode = nodes.compound(classes=['impl-detail'])
        content = self.content
        add_text = nodes.strong('CPython implementation detail:',
                                'CPython implementation detail:')
        if self.arguments:
            n, m = self.state.inline_text(self.arguments[0], self.lineno)
            pnode.append(nodes.paragraph('', '', *(n + m)))
        self.state.nested_parse(content, self.content_offset, pnode)
        if pnode.children and isinstance(pnode[0], nodes.paragraph):
            pnode[0].insert(0, add_text)
            pnode[0].insert(1, nodes.Text(' '))
        else:
            pnode.insert(0, nodes.paragraph('', '', add_text))
        return [pnode]


# Support for building "topic help" for pydoc

pydoc_topic_labels = [
    'assert', 'assignment', 'atom-identifiers', 'atom-literals',
    'attribute-access', 'attribute-references', 'augassign', 'binary',
    'bitwise', 'bltin-code-objects', 'bltin-ellipsis-object',
    'bltin-file-objects', 'bltin-null-object', 'bltin-type-objects', 'booleans',
    'break', 'callable-types', 'calls', 'class', 'coercion-rules',
    'comparisons', 'compound', 'context-managers', 'continue', 'conversions',
    'customization', 'debugger', 'del', 'dict', 'dynamic-features', 'else',
    'exceptions', 'exec', 'execmodel', 'exprlists', 'floating', 'for',
    'formatstrings', 'function', 'global', 'id-classes', 'identifiers', 'if',
    'imaginary', 'import', 'in', 'integers', 'lambda', 'lists', 'naming',
    'numbers', 'numeric-types', 'objects', 'operator-summary', 'pass', 'power',
    'print', 'raise', 'return', 'sequence-methods', 'sequence-types',
    'shifting', 'slicings', 'specialattrs', 'specialnames',
    'string-conversions', 'string-methods', 'strings', 'subscriptions', 'truth',
    'try', 'types', 'typesfunctions', 'typesmapping', 'typesmethods',
    'typesmodules', 'typesseq', 'typesseq-mutable', 'unary', 'while', 'with',
    'yield'
]

from os import path
from time import asctime
from pprint import pformat
from docutils.io import StringOutput
from docutils.utils import new_document

from sphinx.builders import Builder
from sphinx.writers.text import TextWriter


class PydocTopicsBuilder(Builder):
    name = 'pydoc-topics'

    def init(self):
        self.topics = {}

    def get_outdated_docs(self):
        return 'all pydoc topics'

    def get_target_uri(self, docname, typ=None):
        return ''  # no URIs

    def write(self, *ignored):
        writer = TextWriter(self)
        for label in self.status_iterator(pydoc_topic_labels,
                                          'building topics... ',
                                          length=len(pydoc_topic_labels)):
            if label not in self.env.labels:
                self.warn('label %r not in documentation' % label)
                continue
            docname, labelid, sectname = self.env.labels[label]
            doctree = self.env.get_and_resolve_doctree(docname, self)
            document = new_document('<section node>')
            document.append(doctree.ids[labelid])
            destination = StringOutput(encoding='utf-8')
            writer.write(document, destination)
            self.topics[label] = writer.output

    def finish(self):
        f = open(path.join(self.outdir, 'topics.py'), 'w')
        try:
            f.write('# Autogenerated by Sphinx on %s\n' % asctime())
            f.write('topics = ' + pformat(self.topics) + '\n')
        finally:
            f.close()


# Support for checking for suspicious markup

import suspicious


# Support for documenting Opcodes

import re
from sphinx import addnodes

opcode_sig_re = re.compile(r'(\w+(?:\+\d)?)\s*\((.*)\)')

def parse_opcode_signature(env, sig, signode):
    """Transform an opcode signature into RST nodes."""
    m = opcode_sig_re.match(sig)
    if m is None:
        raise ValueError
    opname, arglist = m.groups()
    signode += addnodes.desc_name(opname, opname)
    paramlist = addnodes.desc_parameterlist()
    signode += paramlist
    paramlist += addnodes.desc_parameter(arglist, arglist)
    return opname.strip()


def setup(app):
    app.add_role('issue', issue_role)
    app.add_directive('impl-detail', ImplementationDetail)
    app.add_builder(PydocTopicsBuilder)
    app.add_builder(suspicious.CheckSuspiciousMarkupBuilder)
    app.add_description_unit('opcode', 'opcode', '%s (opcode)',
                             parse_opcode_signature)
    app.add_description_unit('2to3fixer', '2to3fixer', '%s (2to3 fixer)')
