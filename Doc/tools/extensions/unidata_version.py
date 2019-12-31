"""
    unidata_version.py
    ~~~~~~~~~~~~~

    Sphinx extension to handle updating the Unicode data version in docs.

    https://bugs.python.org/issue22593
"""
import re

def parse_UNIDATA_VERSION(app, docname, source):
    parsed = re.sub(r'\|UNIDATA_VERSION\|', app.config.UNIDATA_VERSION, source[0])
    source[0] = parsed

def setup(app):
    app.add_config_value('UNIDATA_VERSION', '0.0.0', 'env')
    app.connect('source-read', parse_UNIDATA_VERSION)
    return {'version': '1.0', 'parallel_read_safe': True}
