"""distutils.command.upload

Implements the Distutils 'upload' subcommand (upload package to PyPI)."""

from distutils.errors import *
from distutils.core import PyPIRCCommand
from distutils.spawn import spawn
from distutils import log
from hashlib import md5
import os
import socket
import platform
import httplib
from base64 import standard_b64encode
import urlparse
import cStringIO as StringIO
from ConfigParser import ConfigParser


class upload(PyPIRCCommand):

    description = "upload binary package to PyPI"

    user_options = PyPIRCCommand.user_options + [
        ('sign', 's',
         'sign files to upload using gpg'),
        ('identity=', 'i', 'GPG identity used to sign files'),
        ]

    boolean_options = PyPIRCCommand.boolean_options + ['sign']

    def initialize_options(self):
        PyPIRCCommand.initialize_options(self)
        self.username = ''
        self.password = ''
        self.show_response = 0
        self.sign = False
        self.identity = None

    def finalize_options(self):
        PyPIRCCommand.finalize_options(self)
        if self.identity and not self.sign:
            raise DistutilsOptionError(
                "Must use --sign for --identity to have meaning"
            )
        config = self._read_pypirc()
        if config != {}:
            self.username = config['username']
            self.password = config['password']
            self.repository = config['repository']
            self.realm = config['realm']

    def run(self):
        if not self.distribution.dist_files:
            raise DistutilsOptionError("No dist file created in earlier command")
        for command, pyversion, filename in self.distribution.dist_files:
            self.upload_file(command, pyversion, filename)

    def upload_file(self, command, pyversion, filename):
        # Sign if requested
        if self.sign:
            gpg_args = ["gpg", "--detach-sign", "-a", filename]
            if self.identity:
                gpg_args[2:2] = ["--local-user", self.identity]
            spawn(gpg_args,
                  dry_run=self.dry_run)

        # Fill in the data - send all the meta-data in case we need to
        # register a new release
        content = open(filename,'rb').read()
        meta = self.distribution.metadata
        data = {
            # action
            ':action': 'file_upload',
            'protcol_version': '1',

            # identify release
            'name': meta.get_name(),
            'version': meta.get_version(),

            # file content
            'content': (os.path.basename(filename),content),
            'filetype': command,
            'pyversion': pyversion,
            'md5_digest': md5(content).hexdigest(),

            # additional meta-data
            'metadata_version' : '1.0',
            'summary': meta.get_description(),
            'home_page': meta.get_url(),
            'author': meta.get_contact(),
            'author_email': meta.get_contact_email(),
            'license': meta.get_licence(),
            'description': meta.get_long_description(),
            'keywords': meta.get_keywords(),
            'platform': meta.get_platforms(),
            'classifiers': meta.get_classifiers(),
            'download_url': meta.get_download_url(),
            # PEP 314
            'provides': meta.get_provides(),
            'requires': meta.get_requires(),
            'obsoletes': meta.get_obsoletes(),
            }
        comment = ''
        if command == 'bdist_rpm':
            dist, version, id = platform.dist()
            if dist:
                comment = 'built for %s %s' % (dist, version)
        elif command == 'bdist_dumb':
            comment = 'built for %s' % platform.platform(terse=1)
        data['comment'] = comment

        if self.sign:
            data['gpg_signature'] = (os.path.basename(filename) + ".asc",
                                     open(filename+".asc").read())

        # set up the authentication
        auth = "Basic " + standard_b64encode(self.username + ":" +
                                             self.password)

        # Build up the MIME payload for the POST data
        boundary = '--------------GHSKFJDLGDS7543FJKLFHRE75642756743254'
        sep_boundary = '\n--' + boundary
        end_boundary = sep_boundary + '--'
        body = StringIO.StringIO()
        for key, value in data.items():
            # handle multiple entries for the same name
            if type(value) != type([]):
                value = [value]
            for value in value:
                if type(value) is tuple:
                    fn = ';filename="%s"' % value[0]
                    value = value[1]
                else:
                    fn = ""

                body.write(sep_boundary)
                body.write('\nContent-Disposition: form-data; name="%s"'%key)
                body.write(fn)
                body.write("\n\n")
                body.write(value)
                if value and value[-1] == '\r':
                    body.write('\n')  # write an extra newline (lurve Macs)
        body.write(end_boundary)
        body.write("\n")
        body = body.getvalue()

        self.announce("Submitting %s to %s" % (filename, self.repository), log.INFO)

        # build the Request
        # We can't use urllib2 since we need to send the Basic
        # auth right with the first request
        schema, netloc, url, params, query, fragments = \
            urlparse.urlparse(self.repository)
        assert not params and not query and not fragments
        if schema == 'http':
            http = httplib.HTTPConnection(netloc)
        elif schema == 'https':
            http = httplib.HTTPSConnection(netloc)
        else:
            raise AssertionError, "unsupported schema "+schema

        data = ''
        loglevel = log.INFO
        try:
            http.connect()
            http.putrequest("POST", url)
            http.putheader('Content-type',
                           'multipart/form-data; boundary=%s'%boundary)
            http.putheader('Content-length', str(len(body)))
            http.putheader('Authorization', auth)
            http.endheaders()
            http.send(body)
        except socket.error, e:
            self.announce(str(e), log.ERROR)
            return

        r = http.getresponse()
        if r.status == 200:
            self.announce('Server response (%s): %s' % (r.status, r.reason),
                          log.INFO)
        else:
            self.announce('Upload failed (%s): %s' % (r.status, r.reason),
                          log.ERROR)
        if self.show_response:
            print '-'*75, r.read(), '-'*75
