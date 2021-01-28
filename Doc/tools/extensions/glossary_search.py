# -*- coding: utf-8 -*-
"""
    glossary_search.py
    ~~~~~~~~~~~~~~~~

    Feature search results for glossary items prominently.

    :license: Python license.
"""
from os import path
from sphinx.addnodes import glossary
from sphinx.util import logging
from docutils.nodes import definition_list_item
import json


logger = logging.getLogger(__name__)


def process_glossary_nodes(app, doctree, fromdocname):
    if app.builder.format != 'html':
        return

    terms = {}

    for node in doctree.traverse(glossary):
        for glossary_item in node.traverse(definition_list_item):
            term = glossary_item[0].astext().lower()
            definition = glossary_item[1]

            rendered = app.builder.render_partial(definition)
            terms[term] = {
                'title': glossary_item[0].astext(),
                'body': rendered['html_body']
            }

    if hasattr(app.env, 'glossary_terms'):
        app.env.glossary_terms.update(terms)
    else:
        app.env.glossary_terms = terms

def on_build_finish(app, exc):
    if not hasattr(app.env, 'glossary_terms'):
        return
    if not app.env.glossary_terms:
        return

    logger.info('Writing glossary.json', color='green')
    with open(path.join(app.outdir, '_static', 'glossary.json'), 'w') as f:
        json.dump(app.env.glossary_terms, f)


def setup(app):
    app.connect('doctree-resolved', process_glossary_nodes)
    app.connect('build-finished', on_build_finish)

    return {'version': '0.1', 'parallel_read_safe': True}
