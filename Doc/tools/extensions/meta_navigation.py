"""Include generated HTML pages in the previous/next navigation flow."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from sphinx.locale import _ as sphinx_gettext

if TYPE_CHECKING:
    from docutils.nodes import document
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata


_TEMPLATE_SOURCES = {
    'download': 'tools/templates/download.html',
    'search': 'tools/templates/search.html',
}


def _has_module_index(app: Sphinx) -> bool:
    return any(
        name == 'py-modindex' for name, *_ in app.builder.domain_indices
    )


def _genindex_pages(app: Sphinx, context: dict[str, Any]) -> list[str]:
    if not app.builder.use_index:
        return []

    pages = ['genindex']
    if app.config.html_split_index:
        pages.extend(
            f'genindex-{key}'
            for key, _entries in context.get('genindexentries', ())
        )
        pages.append('genindex-all')
    return pages


def _page_title(app: Sphinx, pagename: str) -> str:
    if title_node := app.env.titles.get(pagename):
        return app.builder.render_partial(title_node)['title']

    if pagename == 'py-modindex':
        return sphinx_gettext('Python Module Index')
    if pagename in {'genindex', 'genindex-all'}:
        return sphinx_gettext('Index')
    if pagename.startswith('genindex-'):
        return (
            f"{sphinx_gettext('Index')} – {pagename.removeprefix('genindex-')}"
        )
    if pagename == 'search':
        return sphinx_gettext('Search')
    if pagename == 'download':
        return sphinx_gettext('Download')
    raise ValueError(f'unknown generated page: {pagename}')


def _splice_flow(
    app: Sphinx,
    pagename: str,
    context: dict[str, Any],
    flow: list[str],
) -> None:
    try:
        position = flow.index(pagename)
    except ValueError:
        return

    relation_links = {
        rellink[2]: rellink
        for rellink in context['rellinks']
        if rellink[2] in {'N', 'P'}
    }
    for offset, direction, accesskey in (
        (1, 'next', 'N'),
        (-1, 'prev', 'P'),
    ):
        target_position = position + offset
        if not 0 <= target_position < len(flow):
            context.pop(direction, None)
            relation_links.pop(accesskey, None)
            continue

        target = flow[target_position]
        title = _page_title(app, target)
        context[direction] = {
            'link': context['pathto'](target),
            'title': title,
        }
        relation_links[accesskey] = (
            target,
            title,
            accesskey,
            sphinx_gettext('previous' if direction == 'prev' else 'next'),
        )

    context['rellinks'] = [
        rellink
        for rellink in context['rellinks']
        if rellink[2] not in {'N', 'P'}
    ]
    context['rellinks'].extend(
        relation_links[accesskey]
        for accesskey in ('N', 'P')
        if accesskey in relation_links
    )


def add_meta_page_relations(
    app: Sphinx,
    pagename: str,
    _templatename: str,
    context: dict[str, Any],
    _doctree: document | None,
) -> None:
    if app.builder.name != 'html':
        return

    if pagename in _TEMPLATE_SOURCES:
        context['page_source_path'] = _TEMPLATE_SOURCES[pagename]

    flow = []
    docname = app.config.root_doc
    while docname is not None:
        if docname == 'glossary':
            # prepend modindex and genindex to glossary
            if _has_module_index(app):
                flow.append('py-modindex')
            flow.extend(_genindex_pages(app, context))

        flow.append(docname)
        if docname == 'glossary' and app.builder.search:
            flow.append('search')
        if (
            docname == 'bugs'
            and 'download' in app.config.html_additional_pages
        ):
            flow.append('download')

        docname = app.builder.relations[docname][2]

    _splice_flow(app, pagename, context, flow)


def setup(app: Sphinx) -> ExtensionMetadata:
    app.connect('html-page-context', add_meta_page_relations)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
