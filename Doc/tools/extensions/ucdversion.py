"""
    ucdversion.py
    ~~~~~~~~~~~~~

    Sphinx extension to handle updating the Unicode data version in docs.

    :copyright: 2019 by Noah Massman-Hall
"""
import re

def parse_ucd_version(app, docname, source):
    parsed = re.sub(r'\|ucd_version\|', app.config.UCD_VERSION, source[0])
    source[0] = parsed

def setup(app):
    app.add_config_value('UCD_VERSION', '0.0.0', 'env')
    app.connect('source-read', parse_ucd_version)
    return {'version': '1.0', 'parallel_read_safe': True}
