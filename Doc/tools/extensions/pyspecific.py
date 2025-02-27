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

# Used in conf.py and updated here by python/release-tools/run_release.py
SOURCE_URI = 'https://github.com/python/cpython/tree/3.13/%s'

# monkey-patch reST parser to disable alphabetic and roman enumerated lists
from docutils.parsers.rst.states import Body
Body.enum.converters['loweralpha'] = \
    Body.enum.converters['upperalpha'] = \
    Body.enum.converters['lowerroman'] = \
    Body.enum.converters['upperroman'] = lambda x: None


class PyAwaitableMixin(object):
    def handle_signature(self, sig, signode):
        ret = super(PyAwaitableMixin, self).handle_signature(sig, signode)
        signode.insert(0, addnodes.desc_annotation('awaitable ', 'awaitable '))
        return ret


class PyAwaitableFunction(PyAwaitableMixin, PyFunction):
    def run(self):
        self.name = 'py:function'
        return PyFunction.run(self)


class PyAwaitableMethod(PyAwaitableMixin, PyMethod):
    def run(self):
        self.name = 'py:method'
        return PyMethod.run(self)


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
    app.add_object_type('opcode', 'opcode', '%s (opcode)', parse_opcode_signature)
    app.add_object_type('pdbcommand', 'pdbcmd', '%s (pdb command)', parse_pdb_command)
    app.add_object_type('monitoring-event', 'monitoring-event', '%s (monitoring event)', parse_monitoring_event)
    app.add_directive_to_domain('py', 'awaitablefunction', PyAwaitableFunction)
    app.add_directive_to_domain('py', 'awaitablemethod', PyAwaitableMethod)
    app.connect('env-check-consistency', patch_pairindextypes)
    return {'version': '1.0', 'parallel_read_safe': True}
