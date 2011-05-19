"""Upload a distribution to a project index."""

import os
import socket
import logging
import platform
import urllib.parse
from io import BytesIO
from base64 import standard_b64encode
from hashlib import md5
from urllib.error import HTTPError
from urllib.request import urlopen, Request

from packaging import logger
from packaging.errors import PackagingOptionError
from packaging.util import (spawn, read_pypirc, DEFAULT_REPOSITORY,
                            DEFAULT_REALM)
from packaging.command.cmd import Command


class upload(Command):

    description = "upload distribution to PyPI"

    user_options = [
        ('repository=', 'r',
         "repository URL [default: %s]" % DEFAULT_REPOSITORY),
        ('show-response', None,
         "display full response text from server"),
        ('sign', 's',
         "sign files to upload using gpg"),
        ('identity=', 'i',
         "GPG identity used to sign files"),
        ('upload-docs', None,
         "upload documentation too"),
        ]

    boolean_options = ['show-response', 'sign']

    def initialize_options(self):
        self.repository = None
        self.realm = None
        self.show_response = False
        self.username = ''
        self.password = ''
        self.show_response = False
        self.sign = False
        self.identity = None
        self.upload_docs = False

    def finalize_options(self):
        if self.repository is None:
            self.repository = DEFAULT_REPOSITORY
        if self.realm is None:
            self.realm = DEFAULT_REALM
        if self.identity and not self.sign:
            raise PackagingOptionError(
                "Must use --sign for --identity to have meaning")
        config = read_pypirc(self.repository, self.realm)
        if config != {}:
            self.username = config['username']
            self.password = config['password']
            self.repository = config['repository']
            self.realm = config['realm']

        # getting the password from the distribution
        # if previously set by the register command
        if not self.password and self.distribution.password:
            self.password = self.distribution.password

    def run(self):
        if not self.distribution.dist_files:
            raise PackagingOptionError(
                "No dist file created in earlier command")
        for command, pyversion, filename in self.distribution.dist_files:
            self.upload_file(command, pyversion, filename)
        if self.upload_docs:
            upload_docs = self.get_finalized_command("upload_docs")
            upload_docs.repository = self.repository
            upload_docs.username = self.username
            upload_docs.password = self.password
            upload_docs.run()

    # XXX to be refactored with register.post_to_server
    def upload_file(self, command, pyversion, filename):
        # Makes sure the repository URL is compliant
        scheme, netloc, url, params, query, fragments = \
            urllib.parse.urlparse(self.repository)
        if params or query or fragments:
            raise AssertionError("Incompatible url %s" % self.repository)

        if scheme not in ('http', 'https'):
            raise AssertionError("unsupported scheme " + scheme)

        # Sign if requested
        if self.sign:
            gpg_args = ["gpg", "--detach-sign", "-a", filename]
            if self.identity:
                gpg_args[2:2] = ["--local-user", self.identity]
            spawn(gpg_args,
                  dry_run=self.dry_run)

        # Fill in the data - send all the metadata in case we need to
        # register a new release
        with open(filename, 'rb') as f:
            content = f.read()

        data = self.distribution.metadata.todict()

        # extra upload infos
        data[':action'] = 'file_upload'
        data['protcol_version'] = '1'
        data['content'] = (os.path.basename(filename), content)
        data['filetype'] = command
        data['pyversion'] = pyversion
        data['md5_digest'] = md5(content).hexdigest()

        if command == 'bdist_dumb':
            data['comment'] = 'built for %s' % platform.platform(terse=True)

        if self.sign:
            with open(filename + '.asc') as fp:
                sig = fp.read()
            data['gpg_signature'] = [
                (os.path.basename(filename) + ".asc", sig)]

        # set up the authentication
        # The exact encoding of the authentication string is debated.
        # Anyway PyPI only accepts ascii for both username or password.
        user_pass = (self.username + ":" + self.password).encode('ascii')
        auth = b"Basic " + standard_b64encode(user_pass)

        # Build up the MIME payload for the POST data
        boundary = b'--------------GHSKFJDLGDS7543FJKLFHRE75642756743254'
        sep_boundary = b'\n--' + boundary
        end_boundary = sep_boundary + b'--'
        body = BytesIO()

        file_fields = ('content', 'gpg_signature')

        for key, value in data.items():
            # handle multiple entries for the same name
            if not isinstance(value, tuple):
                value = [value]

            content_dispo = '\nContent-Disposition: form-data; name="%s"' % key

            if key in file_fields:
                filename_, content = value
                filename_ = ';filename="%s"' % filename_
                body.write(sep_boundary)
                body.write(content_dispo.encode('utf-8'))
                body.write(filename_.encode('utf-8'))
                body.write(b"\n\n")
                body.write(content)
            else:
                for value in value:
                    value = str(value).encode('utf-8')
                    body.write(sep_boundary)
                    body.write(content_dispo.encode('utf-8'))
                    body.write(b"\n\n")
                    body.write(value)
                    if value and value.endswith(b'\r'):
                        # write an extra newline (lurve Macs)
                        body.write(b'\n')

        body.write(end_boundary)
        body.write(b"\n")
        body = body.getvalue()

        logger.info("Submitting %s to %s", filename, self.repository)

        # build the Request
        headers = {'Content-type':
                        'multipart/form-data; boundary=%s' %
                        boundary.decode('ascii'),
                   'Content-length': str(len(body)),
                   'Authorization': auth}

        request = Request(self.repository, data=body,
                          headers=headers)
        # send the data
        try:
            result = urlopen(request)
            status = result.code
            reason = result.msg
        except socket.error as e:
            logger.error(e)
            return
        except HTTPError as e:
            status = e.code
            reason = e.msg

        if status == 200:
            logger.info('Server response (%s): %s', status, reason)
        else:
            logger.error('Upload failed (%s): %s', status, reason)

        if self.show_response and logger.isEnabledFor(logging.INFO):
            sep = '-' * 75
            logger.info('%s\n%s\n%s', sep, result.read().decode(), sep)
