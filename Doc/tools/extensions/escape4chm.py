"""
Escape the `body` part of .chm source file to 7-bit ASCII, to fix visual
effect on some MBCS Windows systems.

https://bugs.python.org/issue32174
"""

import re
from html.entities import codepoint2name

# escape the characters which codepoint > 0x7F
def _process(string):
    def escape(matchobj):
        codepoint = ord(matchobj.group(0))

        name = codepoint2name.get(codepoint)
        if name is None:
            return '&#%d;' % codepoint
        else:
            return '&%s;' % name

    return re.sub(r'[^\x00-\x7F]', escape, string)

def escape_for_chm(app, pagename, templatename, context, doctree):
    # only works for .chm output
    if not hasattr(app.builder, 'name') or app.builder.name != 'htmlhelp':
        return

    # escape the `body` part to 7-bit ASCII
    body = context.get('body')
    if body is not None:
        context['body'] = _process(body)

def setup(app):
    # `html-page-context` event emitted when the HTML builder has
    # created a context dictionary to render a template with.
    app.connect('html-page-context', escape_for_chm)

    return {'version': '1.0', 'parallel_read_safe': True}
