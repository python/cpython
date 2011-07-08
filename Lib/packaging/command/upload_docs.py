"""Upload HTML documentation to a project index."""

import os
import base64
import socket
import zipfile
import logging
import http.client
import urllib.parse
from io import BytesIO

from packaging import logger
from packaging.util import (read_pypirc, DEFAULT_REPOSITORY, DEFAULT_REALM,
                            encode_multipart)
from packaging.errors import PackagingFileError
from packaging.command.cmd import Command


def zip_dir(directory):
    """Compresses recursively contents of directory into a BytesIO object"""
    destination = BytesIO()
    with zipfile.ZipFile(destination, "w") as zip_file:
        for root, dirs, files in os.walk(directory):
            for name in files:
                full = os.path.join(root, name)
                relative = root[len(directory):].lstrip(os.path.sep)
                dest = os.path.join(relative, name)
                zip_file.write(full, dest)
    return destination


class upload_docs(Command):

    description = "upload HTML documentation to PyPI"

    user_options = [
        ('repository=', 'r',
         "repository URL [default: %s]" % DEFAULT_REPOSITORY),
        ('show-response', None,
         "display full response text from server"),
        ('upload-dir=', None,
         "directory to upload"),
        ]

    def initialize_options(self):
        self.repository = None
        self.realm = None
        self.show_response = False
        self.upload_dir = None
        self.username = ''
        self.password = ''

    def finalize_options(self):
        if self.repository is None:
            self.repository = DEFAULT_REPOSITORY
        if self.realm is None:
            self.realm = DEFAULT_REALM
        if self.upload_dir is None:
            build = self.get_finalized_command('build')
            self.upload_dir = os.path.join(build.build_base, "docs")
            if not os.path.isdir(self.upload_dir):
                self.upload_dir = os.path.join(build.build_base, "doc")
        logger.info('Using upload directory %s', self.upload_dir)
        self.verify_upload_dir(self.upload_dir)
        config = read_pypirc(self.repository, self.realm)
        if config != {}:
            self.username = config['username']
            self.password = config['password']
            self.repository = config['repository']
            self.realm = config['realm']

    def verify_upload_dir(self, upload_dir):
        self.ensure_dirname('upload_dir')
        index_location = os.path.join(upload_dir, "index.html")
        if not os.path.exists(index_location):
            mesg = "No 'index.html found in docs directory (%s)"
            raise PackagingFileError(mesg % upload_dir)

    def run(self):
        name = self.distribution.metadata['Name']
        version = self.distribution.metadata['Version']
        zip_file = zip_dir(self.upload_dir)

        fields = [(':action', 'doc_upload'),
                  ('name', name), ('version', version)]
        files = [('content', name, zip_file.getvalue())]
        content_type, body = encode_multipart(fields, files)

        credentials = self.username + ':' + self.password
        auth = b"Basic " + base64.encodebytes(credentials.encode()).strip()

        logger.info("Submitting documentation to %s", self.repository)

        scheme, netloc, url, params, query, fragments = urllib.parse.urlparse(
            self.repository)
        if scheme == "http":
            conn = http.client.HTTPConnection(netloc)
        elif scheme == "https":
            conn = http.client.HTTPSConnection(netloc)
        else:
            raise AssertionError("unsupported scheme %r" % scheme)

        try:
            conn.connect()
            conn.putrequest("POST", url)
            conn.putheader('Content-type', content_type)
            conn.putheader('Content-length', str(len(body)))
            conn.putheader('Authorization', auth)
            conn.endheaders()
            conn.send(body)

        except socket.error as e:
            logger.error(e)
            return

        r = conn.getresponse()

        if r.status == 200:
            logger.info('Server response (%s): %s', r.status, r.reason)
        elif r.status == 301:
            location = r.getheader('Location')
            if location is None:
                location = 'http://packages.python.org/%s/' % name
            logger.info('Upload successful. Visit %s', location)
        else:
            logger.error('Upload failed (%s): %s', r.status, r.reason)

        if self.show_response and logger.isEnabledFor(logging.INFO):
            sep = '-' * 75
            logger.info('%s\n%s\n%s', sep, r.read().decode('utf-8'), sep)
