"""Register a release with a project index."""

# Contributed by Richard Jones

import io
import getpass
import urllib.error
import urllib.parse
import urllib.request

from packaging import logger
from packaging.util import (read_pypirc, generate_pypirc, DEFAULT_REPOSITORY,
                            DEFAULT_REALM, get_pypirc_path)
from packaging.command.cmd import Command

class register(Command):

    description = "register a release with PyPI"
    user_options = [
        ('repository=', 'r',
         "repository URL [default: %s]" % DEFAULT_REPOSITORY),
        ('show-response', None,
         "display full response text from server"),
        ('list-classifiers', None,
         "list valid Trove classifiers"),
        ('strict', None ,
         "stop the registration if the metadata is not fully compliant")
        ]

    boolean_options = ['show-response', 'list-classifiers', 'strict']

    def initialize_options(self):
        self.repository = None
        self.realm = None
        self.show_response = False
        self.list_classifiers = False
        self.strict = False

    def finalize_options(self):
        if self.repository is None:
            self.repository = DEFAULT_REPOSITORY
        if self.realm is None:
            self.realm = DEFAULT_REALM

    def run(self):
        self._set_config()

        # Check the package metadata
        check = self.distribution.get_command_obj('check')
        if check.strict != self.strict and not check.all:
            # If check was already run but with different options,
            # re-run it
            check.strict = self.strict
            check.all = True
            self.distribution.have_run.pop('check', None)
            self.run_command('check')

        if self.dry_run:
            self.verify_metadata()
        elif self.list_classifiers:
            self.classifiers()
        else:
            self.send_metadata()

    def _set_config(self):
        ''' Reads the configuration file and set attributes.
        '''
        config = read_pypirc(self.repository, self.realm)
        if config != {}:
            self.username = config['username']
            self.password = config['password']
            self.repository = config['repository']
            self.realm = config['realm']
            self.has_config = True
        else:
            if self.repository not in ('pypi', DEFAULT_REPOSITORY):
                raise ValueError('%s not found in .pypirc' % self.repository)
            if self.repository == 'pypi':
                self.repository = DEFAULT_REPOSITORY
            self.has_config = False

    def classifiers(self):
        ''' Fetch the list of classifiers from the server.
        '''
        response = urllib.request.urlopen(self.repository+'?:action=list_classifiers')
        logger.info(response.read())

    def verify_metadata(self):
        ''' Send the metadata to the package index server to be checked.
        '''
        # send the info to the server and report the result
        code, result = self.post_to_server(self.build_post_data('verify'))
        logger.info('server response (%s): %s', code, result)


    def send_metadata(self):
        ''' Send the metadata to the package index server.

            Well, do the following:
            1. figure who the user is, and then
            2. send the data as a Basic auth'ed POST.

            First we try to read the username/password from $HOME/.pypirc,
            which is a ConfigParser-formatted file with a section
            [distutils] containing username and password entries (both
            in clear text). Eg:

                [distutils]
                index-servers =
                    pypi

                [pypi]
                username: fred
                password: sekrit

            Otherwise, to figure who the user is, we offer the user three
            choices:

             1. use existing login,
             2. register as a new user, or
             3. set the password to a random string and email the user.

        '''
        # TODO factor registration out into another method
        # TODO use print to print, not logging

        # see if we can short-cut and get the username/password from the
        # config
        if self.has_config:
            choice = '1'
            username = self.username
            password = self.password
        else:
            choice = 'x'
            username = password = ''

        # get the user's login info
        choices = '1 2 3 4'.split()
        while choice not in choices:
            logger.info('''\
We need to know who you are, so please choose either:
 1. use your existing login,
 2. register as a new user,
 3. have the server generate a new password for you (and email it to you), or
 4. quit
Your selection [default 1]: ''')

            choice = input()
            if not choice:
                choice = '1'
            elif choice not in choices:
                print('Please choose one of the four options!')

        if choice == '1':
            # get the username and password
            while not username:
                username = input('Username: ')
            while not password:
                password = getpass.getpass('Password: ')

            # set up the authentication
            auth = urllib.request.HTTPPasswordMgr()
            host = urllib.parse.urlparse(self.repository)[1]
            auth.add_password(self.realm, host, username, password)
            # send the info to the server and report the result
            code, result = self.post_to_server(self.build_post_data('submit'),
                auth)
            logger.info('Server response (%s): %s', code, result)

            # possibly save the login
            if code == 200:
                if self.has_config:
                    # sharing the password in the distribution instance
                    # so the upload command can reuse it
                    self.distribution.password = password
                else:
                    logger.info(
                        'I can store your PyPI login so future submissions '
                        'will be faster.\n(the login will be stored in %s)',
                        get_pypirc_path())
                    choice = 'X'
                    while choice.lower() not in 'yn':
                        choice = input('Save your login (y/N)?')
                        if not choice:
                            choice = 'n'
                    if choice.lower() == 'y':
                        generate_pypirc(username, password)

        elif choice == '2':
            data = {':action': 'user'}
            data['name'] = data['password'] = data['email'] = ''
            data['confirm'] = None
            while not data['name']:
                data['name'] = input('Username: ')
            while data['password'] != data['confirm']:
                while not data['password']:
                    data['password'] = getpass.getpass('Password: ')
                while not data['confirm']:
                    data['confirm'] = getpass.getpass(' Confirm: ')
                if data['password'] != data['confirm']:
                    data['password'] = ''
                    data['confirm'] = None
                    print("Password and confirm don't match!")
            while not data['email']:
                data['email'] = input('   EMail: ')
            code, result = self.post_to_server(data)
            if code != 200:
                logger.info('server response (%s): %s', code, result)
            else:
                logger.info('you will receive an email shortly; follow the '
                            'instructions in it to complete registration.')
        elif choice == '3':
            data = {':action': 'password_reset'}
            data['email'] = ''
            while not data['email']:
                data['email'] = input('Your email address: ')
            code, result = self.post_to_server(data)
            logger.info('server response (%s): %s', code, result)

    def build_post_data(self, action):
        # figure the data to send - the metadata plus some additional
        # information used by the package server
        data = self.distribution.metadata.todict()
        data[':action'] = action
        return data

    # XXX to be refactored with upload.upload_file
    def post_to_server(self, data, auth=None):
        ''' Post a query to the server, and return a string response.
        '''
        if 'name' in data:
            logger.info('Registering %s to %s', data['name'], self.repository)
        # Build up the MIME payload for the urllib2 POST data
        boundary = '--------------GHSKFJDLGDS7543FJKLFHRE75642756743254'
        sep_boundary = '\n--' + boundary
        end_boundary = sep_boundary + '--'
        body = io.StringIO()
        for key, value in data.items():
            # handle multiple entries for the same name
            if not isinstance(value, (tuple, list)):
                value = [value]

            for value in value:
                body.write(sep_boundary)
                body.write('\nContent-Disposition: form-data; name="%s"'%key)
                body.write("\n\n")
                body.write(value)
                if value and value[-1] == '\r':
                    body.write('\n')  # write an extra newline (lurve Macs)
        body.write(end_boundary)
        body.write("\n")
        body = body.getvalue()

        # build the Request
        headers = {
            'Content-type': 'multipart/form-data; boundary=%s; charset=utf-8'%boundary,
            'Content-length': str(len(body))
        }
        req = urllib.request.Request(self.repository, body, headers)

        # handle HTTP and include the Basic Auth handler
        opener = urllib.request.build_opener(
            urllib.request.HTTPBasicAuthHandler(password_mgr=auth)
        )
        data = ''
        try:
            result = opener.open(req)
        except urllib.error.HTTPError as e:
            if self.show_response:
                data = e.fp.read()
            result = e.code, e.msg
        except urllib.error.URLError as e:
            result = 500, str(e)
        else:
            if self.show_response:
                data = result.read()
            result = 200, 'OK'
        if self.show_response:
            dashes = '-' * 75
            logger.info('%s%s%s', dashes, data, dashes)

        return result
