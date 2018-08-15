# -*- coding: utf-8 -*-
"""
    glossary_search.py
    ~~~~~~~~~~~~~~~~

    Feature search results for glossary items prominently.

    :license: Python license.
"""
from os import path
from sphinx.addnodes import glossary
from docutils.nodes import definition_list_item
import json


def process_glossary_nodes(app, doctree, fromdocname):
    terms = {}

    for node in doctree.traverse(glossary):
        for glossary_item in node.traverse(definition_list_item):
            term = glossary_item[0].astext().lower()
            definition = glossary_item[1]

            rendered = app.builder.render_partial(definition)
            terms[term] = rendered['html_body']

    if terms:
        with open(path.join(app.outdir, '_static', 'glossary.json'), 'w') as f:
            json.dump(terms, f)

def setup(app):
    app.connect('doctree-resolved', process_glossary_nodes)

    return {'version': '0.1', 'parallel_read_safe': True}
