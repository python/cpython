# -*- coding: utf-8 -*-
"""
    pyspecific.py
    ~~~~~~~~~~~~~

    Sphinx extension with Python doc-specific markup.

    :copyright: 2008 by Georg Brandl.
    :license: Python license.
"""

ISSUE_URI = 'http://bugs.python.org/issue%s'

from docutils import nodes, utils

def issue_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    issue = utils.unescape(text)
    text = 'issue ' + issue
    refnode = nodes.reference(text, text, refuri=ISSUE_URI % issue)
    return [refnode], []


def setup(app):
    app.add_role('issue', issue_role)
