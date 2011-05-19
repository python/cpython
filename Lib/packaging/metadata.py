"""Implementation of the Metadata for Python packages PEPs.

Supports all metadata formats (1.0, 1.1, 1.2).
"""

import re
import logging

from io import StringIO
from email import message_from_file
from packaging import logger
from packaging.markers import interpret
from packaging.version import (is_valid_predicate, is_valid_version,
                               is_valid_versions)
from packaging.errors import (MetadataMissingError,
                              MetadataConflictError,
                              MetadataUnrecognizedVersionError)

try:
    # docutils is installed
    from docutils.utils import Reporter
    from docutils.parsers.rst import Parser
    from docutils import frontend
    from docutils import nodes

    class SilentReporter(Reporter):

        def __init__(self, source, report_level, halt_level, stream=None,
                     debug=0, encoding='ascii', error_handler='replace'):
            self.messages = []
            Reporter.__init__(self, source, report_level, halt_level, stream,
                              debug, encoding, error_handler)

        def system_message(self, level, message, *children, **kwargs):
            self.messages.append((level, message, children, kwargs))

    _HAS_DOCUTILS = True
except ImportError:
    # docutils is not installed
    _HAS_DOCUTILS = False

# public API of this module
__all__ = ['Metadata', 'PKG_INFO_ENCODING', 'PKG_INFO_PREFERRED_VERSION']

# Encoding used for the PKG-INFO files
PKG_INFO_ENCODING = 'utf-8'

# preferred version. Hopefully will be changed
# to 1.2 once PEP 345 is supported everywhere
PKG_INFO_PREFERRED_VERSION = '1.0'

_LINE_PREFIX = re.compile('\n       \|')
_241_FIELDS = ('Metadata-Version', 'Name', 'Version', 'Platform',
               'Summary', 'Description',
               'Keywords', 'Home-page', 'Author', 'Author-email',
               'License')

_314_FIELDS = ('Metadata-Version', 'Name', 'Version', 'Platform',
               'Supported-Platform', 'Summary', 'Description',
               'Keywords', 'Home-page', 'Author', 'Author-email',
               'License', 'Classifier', 'Download-URL', 'Obsoletes',
               'Provides', 'Requires')

_314_MARKERS = ('Obsoletes', 'Provides', 'Requires')

_345_FIELDS = ('Metadata-Version', 'Name', 'Version', 'Platform',
               'Supported-Platform', 'Summary', 'Description',
               'Keywords', 'Home-page', 'Author', 'Author-email',
               'Maintainer', 'Maintainer-email', 'License',
               'Classifier', 'Download-URL', 'Obsoletes-Dist',
               'Project-URL', 'Provides-Dist', 'Requires-Dist',
               'Requires-Python', 'Requires-External')

_345_MARKERS = ('Provides-Dist', 'Requires-Dist', 'Requires-Python',
                'Obsoletes-Dist', 'Requires-External', 'Maintainer',
                'Maintainer-email', 'Project-URL')

_ALL_FIELDS = set()
_ALL_FIELDS.update(_241_FIELDS)
_ALL_FIELDS.update(_314_FIELDS)
_ALL_FIELDS.update(_345_FIELDS)


def _version2fieldlist(version):
    if version == '1.0':
        return _241_FIELDS
    elif version == '1.1':
        return _314_FIELDS
    elif version == '1.2':
        return _345_FIELDS
    raise MetadataUnrecognizedVersionError(version)


def _best_version(fields):
    """Detect the best version depending on the fields used."""
    def _has_marker(keys, markers):
        for marker in markers:
            if marker in keys:
                return True
        return False

    keys = list(fields)
    possible_versions = ['1.0', '1.1', '1.2']

    # first let's try to see if a field is not part of one of the version
    for key in keys:
        if key not in _241_FIELDS and '1.0' in possible_versions:
            possible_versions.remove('1.0')
        if key not in _314_FIELDS and '1.1' in possible_versions:
            possible_versions.remove('1.1')
        if key not in _345_FIELDS and '1.2' in possible_versions:
            possible_versions.remove('1.2')

    # possible_version contains qualified versions
    if len(possible_versions) == 1:
        return possible_versions[0]   # found !
    elif len(possible_versions) == 0:
        raise MetadataConflictError('Unknown metadata set')

    # let's see if one unique marker is found
    is_1_1 = '1.1' in possible_versions and _has_marker(keys, _314_MARKERS)
    is_1_2 = '1.2' in possible_versions and _has_marker(keys, _345_MARKERS)
    if is_1_1 and is_1_2:
        raise MetadataConflictError('You used incompatible 1.1 and 1.2 fields')

    # we have the choice, either 1.0, or 1.2
    #   - 1.0 has a broken Summary field but works with all tools
    #   - 1.1 is to avoid
    #   - 1.2 fixes Summary but is not widespread yet
    if not is_1_1 and not is_1_2:
        # we couldn't find any specific marker
        if PKG_INFO_PREFERRED_VERSION in possible_versions:
            return PKG_INFO_PREFERRED_VERSION
    if is_1_1:
        return '1.1'

    # default marker when 1.0 is disqualified
    return '1.2'


_ATTR2FIELD = {
    'metadata_version': 'Metadata-Version',
    'name': 'Name',
    'version': 'Version',
    'platform': 'Platform',
    'supported_platform': 'Supported-Platform',
    'summary': 'Summary',
    'description': 'Description',
    'keywords': 'Keywords',
    'home_page': 'Home-page',
    'author': 'Author',
    'author_email': 'Author-email',
    'maintainer': 'Maintainer',
    'maintainer_email': 'Maintainer-email',
    'license': 'License',
    'classifier': 'Classifier',
    'download_url': 'Download-URL',
    'obsoletes_dist': 'Obsoletes-Dist',
    'provides_dist': 'Provides-Dist',
    'requires_dist': 'Requires-Dist',
    'requires_python': 'Requires-Python',
    'requires_external': 'Requires-External',
    'requires': 'Requires',
    'provides': 'Provides',
    'obsoletes': 'Obsoletes',
    'project_url': 'Project-URL',
}

_PREDICATE_FIELDS = ('Requires-Dist', 'Obsoletes-Dist', 'Provides-Dist')
_VERSIONS_FIELDS = ('Requires-Python',)
_VERSION_FIELDS = ('Version',)
_LISTFIELDS = ('Platform', 'Classifier', 'Obsoletes',
               'Requires', 'Provides', 'Obsoletes-Dist',
               'Provides-Dist', 'Requires-Dist', 'Requires-External',
               'Project-URL', 'Supported-Platform')
_LISTTUPLEFIELDS = ('Project-URL',)

_ELEMENTSFIELD = ('Keywords',)

_UNICODEFIELDS = ('Author', 'Maintainer', 'Summary', 'Description')

_MISSING = object()


class NoDefault:
    """Marker object used for clean representation"""
    def __repr__(self):
        return '<NoDefault>'

_MISSING = NoDefault()


class Metadata:
    """The metadata of a release.

    Supports versions 1.0, 1.1 and 1.2 (auto-detected). You can
    instantiate the class with one of these arguments (or none):
    - *path*, the path to a METADATA file
    - *fileobj* give a file-like object with METADATA as content
    - *mapping* is a dict-like object
    """
    # TODO document that execution_context and platform_dependent are used
    # to filter on query, not when setting a key
    # also document the mapping API and UNKNOWN default key

    def __init__(self, path=None, platform_dependent=False,
                 execution_context=None, fileobj=None, mapping=None):
        self._fields = {}
        self.requires_files = []
        self.docutils_support = _HAS_DOCUTILS
        self.platform_dependent = platform_dependent
        self.execution_context = execution_context
        if [path, fileobj, mapping].count(None) < 2:
            raise TypeError('path, fileobj and mapping are exclusive')
        if path is not None:
            self.read(path)
        elif fileobj is not None:
            self.read_file(fileobj)
        elif mapping is not None:
            self.update(mapping)

    def _set_best_version(self):
        self._fields['Metadata-Version'] = _best_version(self._fields)

    def _write_field(self, file, name, value):
        file.write('%s: %s\n' % (name, value))

    def __getitem__(self, name):
        return self.get(name)

    def __setitem__(self, name, value):
        return self.set(name, value)

    def __delitem__(self, name):
        field_name = self._convert_name(name)
        try:
            del self._fields[field_name]
        except KeyError:
            raise KeyError(name)
        self._set_best_version()

    def __contains__(self, name):
        return (name in self._fields or
                self._convert_name(name) in self._fields)

    def _convert_name(self, name):
        if name in _ALL_FIELDS:
            return name
        name = name.replace('-', '_').lower()
        return _ATTR2FIELD.get(name, name)

    def _default_value(self, name):
        if name in _LISTFIELDS or name in _ELEMENTSFIELD:
            return []
        return 'UNKNOWN'

    def _check_rst_data(self, data):
        """Return warnings when the provided data has syntax errors."""
        source_path = StringIO()
        parser = Parser()
        settings = frontend.OptionParser().get_default_values()
        settings.tab_width = 4
        settings.pep_references = None
        settings.rfc_references = None
        reporter = SilentReporter(source_path,
                          settings.report_level,
                          settings.halt_level,
                          stream=settings.warning_stream,
                          debug=settings.debug,
                          encoding=settings.error_encoding,
                          error_handler=settings.error_encoding_error_handler)

        document = nodes.document(settings, reporter, source=source_path)
        document.note_source(source_path, -1)
        try:
            parser.parse(data, document)
        except AttributeError:
            reporter.messages.append((-1, 'Could not finish the parsing.',
                                      '', {}))

        return reporter.messages

    def _platform(self, value):
        if not self.platform_dependent or ';' not in value:
            return True, value
        value, marker = value.split(';')
        return interpret(marker, self.execution_context), value

    def _remove_line_prefix(self, value):
        return _LINE_PREFIX.sub('\n', value)

    #
    # Public API
    #
    def get_fullname(self):
        """Return the distribution name with version"""
        return '%s-%s' % (self['Name'], self['Version'])

    def is_metadata_field(self, name):
        """return True if name is a valid metadata key"""
        name = self._convert_name(name)
        return name in _ALL_FIELDS

    def is_multi_field(self, name):
        name = self._convert_name(name)
        return name in _LISTFIELDS

    def read(self, filepath):
        """Read the metadata values from a file path."""
        with open(filepath, 'r', encoding='ascii') as fp:
            self.read_file(fp)

    def read_file(self, fileob):
        """Read the metadata values from a file object."""
        msg = message_from_file(fileob)
        self._fields['Metadata-Version'] = msg['metadata-version']

        for field in _version2fieldlist(self['Metadata-Version']):
            if field in _LISTFIELDS:
                # we can have multiple lines
                values = msg.get_all(field)
                if field in _LISTTUPLEFIELDS and values is not None:
                    values = [tuple(value.split(',')) for value in values]
                self.set(field, values)
            else:
                # single line
                value = msg[field]
                if value is not None and value != 'UNKNOWN':
                    self.set(field, value)

    def write(self, filepath):
        """Write the metadata fields to filepath."""
        with open(filepath, 'w') as fp:
            self.write_file(fp)

    def write_file(self, fileobject):
        """Write the PKG-INFO format data to a file object."""
        self._set_best_version()
        for field in _version2fieldlist(self['Metadata-Version']):
            values = self.get(field)
            if field in _ELEMENTSFIELD:
                self._write_field(fileobject, field, ','.join(values))
                continue
            if field not in _LISTFIELDS:
                if field == 'Description':
                    values = values.replace('\n', '\n       |')
                values = [values]

            if field in _LISTTUPLEFIELDS:
                values = [','.join(value) for value in values]

            for value in values:
                self._write_field(fileobject, field, value)

    def update(self, other=None, **kwargs):
        """Set metadata values from the given iterable `other` and kwargs.

        Behavior is like `dict.update`: If `other` has a ``keys`` method,
        they are looped over and ``self[key]`` is assigned ``other[key]``.
        Else, ``other`` is an iterable of ``(key, value)`` iterables.

        Keys that don't match a metadata field or that have an empty value are
        dropped.
        """
        def _set(key, value):
            if key in _ATTR2FIELD and value:
                self.set(self._convert_name(key), value)

        if other is None:
            pass
        elif hasattr(other, 'keys'):
            for k in other.keys():
                _set(k, other[k])
        else:
            for k, v in other:
                _set(k, v)

        if kwargs:
            self.update(kwargs)

    def set(self, name, value):
        """Control then set a metadata field."""
        name = self._convert_name(name)

        if ((name in _ELEMENTSFIELD or name == 'Platform') and
            not isinstance(value, (list, tuple))):
            if isinstance(value, str):
                value = [v.strip() for v in value.split(',')]
            else:
                value = []
        elif (name in _LISTFIELDS and
              not isinstance(value, (list, tuple))):
            if isinstance(value, str):
                value = [value]
            else:
                value = []

        if logger.isEnabledFor(logging.WARNING):
            if name in _PREDICATE_FIELDS and value is not None:
                for v in value:
                    # check that the values are valid predicates
                    if not is_valid_predicate(v.split(';')[0]):
                        logger.warning(
                            '%r is not a valid predicate (field %r)',
                            v, name)
            # FIXME this rejects UNKNOWN, is that right?
            elif name in _VERSIONS_FIELDS and value is not None:
                if not is_valid_versions(value):
                    logger.warning('%r is not a valid version (field %r)',
                                   value, name)
            elif name in _VERSION_FIELDS and value is not None:
                if not is_valid_version(value):
                    logger.warning('%r is not a valid version (field %r)',
                                   value, name)

        if name in _UNICODEFIELDS:
            if name == 'Description':
                value = self._remove_line_prefix(value)

        self._fields[name] = value
        self._set_best_version()

    def get(self, name, default=_MISSING):
        """Get a metadata field."""
        name = self._convert_name(name)
        if name not in self._fields:
            if default is _MISSING:
                default = self._default_value(name)
            return default
        if name in _UNICODEFIELDS:
            value = self._fields[name]
            return value
        elif name in _LISTFIELDS:
            value = self._fields[name]
            if value is None:
                return []
            res = []
            for val in value:
                valid, val = self._platform(val)
                if not valid:
                    continue
                if name not in _LISTTUPLEFIELDS:
                    res.append(val)
                else:
                    # That's for Project-URL
                    res.append((val[0], val[1]))
            return res

        elif name in _ELEMENTSFIELD:
            valid, value = self._platform(self._fields[name])
            if not valid:
                return []
            if isinstance(value, str):
                return value.split(',')
        valid, value = self._platform(self._fields[name])
        if not valid:
            return None
        return value

    def check(self, strict=False, restructuredtext=False):
        """Check if the metadata is compliant. If strict is False then raise if
        no Name or Version are provided"""
        # XXX should check the versions (if the file was loaded)
        missing, warnings = [], []

        for attr in ('Name', 'Version'):  # required by PEP 345
            if attr not in self:
                missing.append(attr)

        if strict and missing != []:
            msg = 'missing required metadata: %s' % ', '.join(missing)
            raise MetadataMissingError(msg)

        for attr in ('Home-page', 'Author'):
            if attr not in self:
                missing.append(attr)

        if _HAS_DOCUTILS and restructuredtext:
            warnings.extend(self._check_rst_data(self['Description']))

        # checking metadata 1.2 (XXX needs to check 1.1, 1.0)
        if self['Metadata-Version'] != '1.2':
            return missing, warnings

        def is_valid_predicates(value):
            for v in value:
                if not is_valid_predicate(v.split(';')[0]):
                    return False
            return True

        for fields, controller in ((_PREDICATE_FIELDS, is_valid_predicates),
                                   (_VERSIONS_FIELDS, is_valid_versions),
                                   (_VERSION_FIELDS, is_valid_version)):
            for field in fields:
                value = self.get(field, None)
                if value is not None and not controller(value):
                    warnings.append('Wrong value for %r: %s' % (field, value))

        return missing, warnings

    def todict(self):
        """Return fields as a dict.

        Field names will be converted to use the underscore-lowercase style
        instead of hyphen-mixed case (i.e. home_page instead of Home-page).
        """
        data = {
            'metadata_version': self['Metadata-Version'],
            'name': self['Name'],
            'version': self['Version'],
            'summary': self['Summary'],
            'home_page': self['Home-page'],
            'author': self['Author'],
            'author_email': self['Author-email'],
            'license': self['License'],
            'description': self['Description'],
            'keywords': self['Keywords'],
            'platform': self['Platform'],
            'classifier': self['Classifier'],
            'download_url': self['Download-URL'],
        }

        if self['Metadata-Version'] == '1.2':
            data['requires_dist'] = self['Requires-Dist']
            data['requires_python'] = self['Requires-Python']
            data['requires_external'] = self['Requires-External']
            data['provides_dist'] = self['Provides-Dist']
            data['obsoletes_dist'] = self['Obsoletes-Dist']
            data['project_url'] = [','.join(url) for url in
                                   self['Project-URL']]

        elif self['Metadata-Version'] == '1.1':
            data['provides'] = self['Provides']
            data['requires'] = self['Requires']
            data['obsoletes'] = self['Obsoletes']

        return data

    # Mapping API

    def keys(self):
        return _version2fieldlist(self['Metadata-Version'])

    def __iter__(self):
        for key in self.keys():
            yield key

    def values(self):
        return [self[key] for key in list(self.keys())]

    def items(self):
        return [(key, self[key]) for key in list(self.keys())]
