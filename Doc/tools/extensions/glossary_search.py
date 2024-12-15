"""Feature search results for glossary items prominently."""

from __future__ import annotations

import json
from pathlib import Path
from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.addnodes import glossary
from sphinx.util import logging

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata

logger = logging.getLogger(__name__)


def process_glossary_nodes(
    app: Sphinx,
    doctree: nodes.document,
    _docname: str,
) -> None:
    if app.builder.format != 'html' or app.builder.embedded:
        return

    if hasattr(app.env, 'glossary_terms'):
        terms = app.env.glossary_terms
    else:
        terms = app.env.glossary_terms = {}

    for node in doctree.findall(glossary):
        for glossary_item in node.findall(nodes.definition_list_item):
            term = glossary_item[0].astext()
            definition = glossary_item[-1]

            rendered = app.builder.render_partial(definition)
            terms[term.lower()] = {
                'title': term,
                'body': rendered['html_body'],
            }


def write_glossary_json(app: Sphinx, _exc: Exception) -> None:
    if not getattr(app.env, 'glossary_terms', None):
        return

    logger.info('Writing glossary.json', color='green')
    dest = Path(app.outdir, '_static', 'glossary.json')
    dest.parent.mkdir(exist_ok=True)
    dest.write_text(json.dumps(app.env.glossary_terms), encoding='utf-8')


def setup(app: Sphinx) -> ExtensionMetadata:
    app.connect('doctree-resolved', process_glossary_nodes)
    app.connect('build-finished', write_glossary_json)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
