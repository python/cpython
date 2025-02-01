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

from docutils import nodes
from docutils.parsers.rst import directives
from docutils.utils import unescape
from sphinx import addnodes
from sphinx.domains.python import PyFunction, PyMethod, PyModule
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxDirective


ISSUE_URI = 'https://bugs.python.org/issue?@action=redirect&bpo=%s'
GH_ISSUE_URI = 'https://github.com/python/cpython/issues/%s'
# Used in conf.py and updated here by python/release-tools/run_release.py
SOURCE_URI = 'https://github.com/python/cpython/tree/main/%s'

# monkey-patch reST parser to disable alphabetic and roman enumerated lists
from docutils.parsers.rst.states import Body
Body.enum.converters['loweralpha'] = \
    Body.enum.converters['upperalpha'] = \
    Body.enum.converters['lowerroman'] = \
    Body.enum.converters['upperroman'] = lambda x: None

# Support for marking up and linking to bugs.python.org issues

def issue_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    issue = unescape(text)
    # sanity check: there are no bpo issues within these two values
    if 47261 < int(issue) < 400000:
        msg = inliner.reporter.error(f'The BPO ID {text!r} seems too high -- '
                                     'use :gh:`...` for GitHub IDs', line=lineno)
        prb = inliner.problematic(rawtext, rawtext, msg)
        return [prb], [msg]
    text = 'bpo-' + issue
    refnode = nodes.reference(text, text, refuri=ISSUE_URI % issue)
    return [refnode], []


# Support for marking up and linking to GitHub issues

def gh_issue_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    issue = unescape(text)
    # sanity check: all GitHub issues have ID >= 32426
    # even though some of them are also valid BPO IDs
    if int(issue) < 32426:
        msg = inliner.reporter.error(f'The GitHub ID {text!r} seems too low -- '
                                     'use :issue:`...` for BPO IDs', line=lineno)
        prb = inliner.problematic(rawtext, rawtext, msg)
        return [prb], [msg]
    text = 'gh-' + issue
    refnode = nodes.reference(text, text, refuri=GH_ISSUE_URI % issue)
    return [refnode], []


# Support for marking up implementation details

class ImplementationDetail(SphinxDirective):

    has_content = True
    final_argument_whitespace = True

    # This text is copied to templates/dummy.html
    label_text = sphinx_gettext('CPython implementation detail:')

    def run(self):
        self.assert_has_content()
        pnode = nodes.compound(classes=['impl-detail'])
        content = self.content
        add_text = nodes.strong(self.label_text, self.label_text)
        self.state.nested_parse(content, self.content_offset, pnode)
        content = nodes.inline(pnode[0].rawsource, translatable=True)
        content.source = pnode[0].source
        content.line = pnode[0].line
        content += pnode[0].children
        pnode[0].replace_self(nodes.paragraph(
            '', '', add_text, nodes.Text(' '), content, translatable=False))
        return [pnode]


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


# Support for including Misc/NEWS

issue_re = re.compile('(?:[Ii]ssue #|bpo-)([0-9]+)', re.I)
gh_issue_re = re.compile('(?:gh-issue-|gh-)([0-9]+)', re.I)
whatsnew_re = re.compile(r"(?im)^what's new in (.*?)\??$")


class MiscNews(SphinxDirective):
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
        self.env.note_dependency(path.abspath(fpath))
        try:
            with io.open(fpath, encoding='utf-8') as fp:
                content = fp.read()
        except Exception:
            text = 'The NEWS file is not available.'
            node = nodes.strong(text, text)
            return [node]
        content = issue_re.sub(r':issue:`\1`', content)
        # Fallback handling for the GitHub issue
        content = gh_issue_re.sub(r':gh:`\1`', content)
        content = whatsnew_re.sub(r'\1', content)
        # remove first 3 lines as they are the main heading
        lines = ['.. default-role:: obj', ''] + content.splitlines()[3:]
        self.state_machine.insert_input(lines, fname)
        return []


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


def parse_monitoring_event(env, sig, signode):
    """Transform a monitoring event signature into RST nodes."""
    signode += addnodes.desc_addname('sys.monitoring.events.', 'sys.monitoring.events.')
    signode += addnodes.desc_name(sig, sig)
    return sig


def patch_pairindextypes(app, _env) -> None:
    """Remove all entries from ``pairindextypes`` before writing POT files.

    We want to run this just before writing output files, as the check to
    circumvent is in ``I18nBuilder.write_doc()``.
    As such, we link this to ``env-check-consistency``, even though it has
    nothing to do with the environment consistency check.
    """
    if app.builder.name != 'gettext':
        return

    # allow translating deprecated index entries
    try:
        from sphinx.domains.python import pairindextypes
    except ImportError:
        pass
    else:
        # Sphinx checks if a 'pair' type entry on an index directive is one of
        # the Sphinx-translated pairindextypes values. As we intend to move
        # away from this, we need Sphinx to believe that these values don't
        # exist, by deleting them when using the gettext builder.
        pairindextypes.clear()


def setup(app):
    app.add_role('issue', issue_role)
    app.add_role('gh', gh_issue_role)
    app.add_directive('impl-detail', ImplementationDetail)
    app.add_object_type('opcode', 'opcode', '%s (opcode)', parse_opcode_signature)
    app.add_object_type('pdbcommand', 'pdbcmd', '%s (pdb command)', parse_pdb_command)
    app.add_object_type('monitoring-event', 'monitoring-event', '%s (monitoring event)', parse_monitoring_event)
    app.add_directive_to_domain('py', 'coroutinefunction', PyCoroutineFunction)
    app.add_directive_to_domain('py', 'coroutinemethod', PyCoroutineMethod)
    app.add_directive_to_domain('py', 'awaitablefunction', PyAwaitableFunction)
    app.add_directive_to_domain('py', 'awaitablemethod', PyAwaitableMethod)
    app.add_directive_to_domain('py', 'abstractmethod', PyAbstractMethod)
    app.add_directive('miscnews', MiscNews)
    app.connect('env-check-consistency', patch_pairindextypes)
    return {'version': '1.0', 'parallel_read_safe': True}
