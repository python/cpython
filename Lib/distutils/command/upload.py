"""distutils.command.upload

Implements the Distutils 'upload' subcommand (upload package to PyPI)."""

from distutils.errors import *
from distutils.core import Command
from distutils.sysconfig import get_python_version
from distutils.spawn import spawn
from distutils import log
from md5 import md5
import os
import platform
import ConfigParser
import httplib
import base64
import urlparse
import cStringIO as StringIO

class upload(Command):

    description = "upload binary package to PyPI"

    DEFAULT_REPOSITORY = 'http://www.python.org/pypi'

    user_options = [
        ('repository=', 'r',
         "url of repository [default: %s]" % DEFAULT_REPOSITORY),
        ('show-response', None,
         'display full response text from server'),
        ('sign', 's',
         'sign files to upload using gpg'),
        ]
    boolean_options = ['show-response', 'sign']

    def initialize_options(self):
        self.username = ''
        self.password = ''
        self.repository = ''
        self.show_response = 0
        self.sign = False

    def finalize_options(self):
        if os.environ.has_key('HOME'):
            rc = os.path.join(os.environ['HOME'], '.pypirc')
            if os.path.exists(rc):
                self.announce('Using PyPI login from %s' % rc)
                config = ConfigParser.ConfigParser({
                        'username':'',
                        'password':'',
                        'repository':''})
                config.read(rc)
                if not self.repository:
                    self.repository = config.get('server-login', 'repository')
                if not self.username:
                    self.username = config.get('server-login', 'username')
                if not self.password:
                    self.password = config.get('server-login', 'password')
        if not self.repository:
            self.repository = self.DEFAULT_REPOSITORY

    def run(self):
        if not self.distribution.dist_files:
            raise DistutilsOptionError("No dist file created in earlier command")
        for command, filename in self.distribution.dist_files:
            self.upload_file(command, filename)

    def upload_file(self, command, filename):
        # Sign if requested
        if self.sign:
            spawn(("gpg", "--detach-sign", "-a", filename),
                  dry_run=self.dry_run)

        # Fill in the data
        content = open(filename).read()
        data = {
            ':action':'file_upload',
            'protcol_version':'1',
            'name':self.distribution.get_name(),
            'version':self.distribution.get_version(),
            'content':(os.path.basename(filename),content),
            'filetype':command,
            'pyversion':get_python_version(),
            'md5_digest':md5(content).hexdigest(),
            }
        comment = ''
        if command == 'bdist_rpm':
            dist, version, id = platform.dist()
            if dist:
                comment = 'built for %s %s' % (dist, version)
        elif command == 'bdist_dumb':
            comment = 'built for %s' % platform.platform(terse=1)
        elif command == 'sdist':
            data['pyversion'] = ''
        data['comment'] = comment

        if self.sign:
            data['gpg_signature'] = (os.path.basename(filename) + ".asc",
                                     open(filename+".asc").read())

        # set up the authentication
        auth = "Basic " + base64.encodestring(self.username + ":" + self.password).strip()

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
                value = str(value)
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
            self.announce(e.msg, log.ERROR)
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

