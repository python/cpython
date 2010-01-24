"""distutils.command.register

Implements the Distutils 'register' command (register with the repository).
"""

# created 2002/10/21, Richard Jones

__revision__ = "$Id$"

import os, string, urllib2, getpass, urlparse
import StringIO

from distutils.core import PyPIRCCommand
from distutils.errors import *
from distutils import log

class register(PyPIRCCommand):

    description = ("register the distribution with the Python package index")
    user_options = PyPIRCCommand.user_options + [
        ('list-classifiers', None,
         'list the valid Trove classifiers'),
        ]
    boolean_options = PyPIRCCommand.boolean_options + [
        'verify', 'list-classifiers']

    def initialize_options(self):
        PyPIRCCommand.initialize_options(self)
        self.list_classifiers = 0

    def run(self):
        self.finalize_options()
        self._set_config()
        self.check_metadata()
        if self.dry_run:
            self.verify_metadata()
        elif self.list_classifiers:
            self.classifiers()
        else:
            self.send_metadata()

    def check_metadata(self):
        """Ensure that all required elements of meta-data (name, version,
           URL, (author and author_email) or (maintainer and
           maintainer_email)) are supplied by the Distribution object; warn if
           any are missing.
        """
        metadata = self.distribution.metadata

        missing = []
        for attr in ('name', 'version', 'url'):
            if not (hasattr(metadata, attr) and getattr(metadata, attr)):
                missing.append(attr)

        if missing:
            self.warn("missing required meta-data: " +
                      string.join(missing, ", "))

        if metadata.author:
            if not metadata.author_email:
                self.warn("missing meta-data: if 'author' supplied, " +
                          "'author_email' must be supplied too")
        elif metadata.maintainer:
            if not metadata.maintainer_email:
                self.warn("missing meta-data: if 'maintainer' supplied, " +
                          "'maintainer_email' must be supplied too")
        else:
            self.warn("missing meta-data: either (author and author_email) " +
                      "or (maintainer and maintainer_email) " +
                      "must be supplied")

    def _set_config(self):
        ''' Reads the configuration file and set attributes.
        '''
        config = self._read_pypirc()
        if config != {}:
            self.username = config['username']
            self.password = config['password']
            self.repository = config['repository']
            self.realm = config['realm']
            self.has_config = True
        else:
            if self.repository not in ('pypi', self.DEFAULT_REPOSITORY):
                raise ValueError('%s not found in .pypirc' % self.repository)
            if self.repository == 'pypi':
                self.repository = self.DEFAULT_REPOSITORY
            self.has_config = False

    def classifiers(self):
        ''' Fetch the list of classifiers from the server.
        '''
        response = urllib2.urlopen(self.repository+'?:action=list_classifiers')
        print response.read()

    def verify_metadata(self):
        ''' Send the metadata to the package index server to be checked.
        '''
        # send the info to the server and report the result
        (code, result) = self.post_to_server(self.build_post_data('verify'))
        print 'Server response (%s): %s'%(code, result)


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
            self.announce('''\
We need to know who you are, so please choose either:
 1. use your existing login,
 2. register as a new user,
 3. have the server generate a new password for you (and email it to you), or
 4. quit
Your selection [default 1]: ''', log.INFO)

            choice = raw_input()
            if not choice:
                choice = '1'
            elif choice not in choices:
                print 'Please choose one of the four options!'

        if choice == '1':
            # get the username and password
            while not username:
                username = raw_input('Username: ')
            while not password:
                password = getpass.getpass('Password: ')

            # set up the authentication
            auth = urllib2.HTTPPasswordMgr()
            host = urlparse.urlparse(self.repository)[1]
            auth.add_password(self.realm, host, username, password)
            # send the info to the server and report the result
            code, result = self.post_to_server(self.build_post_data('submit'),
                auth)
            self.announce('Server response (%s): %s' % (code, result),
                          log.INFO)

            # possibly save the login
            if not self.has_config and code == 200:
                self.announce(('I can store your PyPI login so future '
                               'submissions will be faster.'), log.INFO)
                self.announce('(the login will be stored in %s)' % \
                              self._get_rc_file(), log.INFO)

                choice = 'X'
                while choice.lower() not in 'yn':
                    choice = raw_input('Save your login (y/N)?')
                    if not choice:
                        choice = 'n'
                if choice.lower() == 'y':
                    self._store_pypirc(username, password)

        elif choice == '2':
            data = {':action': 'user'}
            data['name'] = data['password'] = data['email'] = ''
            data['confirm'] = None
            while not data['name']:
                data['name'] = raw_input('Username: ')
            while data['password'] != data['confirm']:
                while not data['password']:
                    data['password'] = getpass.getpass('Password: ')
                while not data['confirm']:
                    data['confirm'] = getpass.getpass(' Confirm: ')
                if data['password'] != data['confirm']:
                    data['password'] = ''
                    data['confirm'] = None
                    print "Password and confirm don't match!"
            while not data['email']:
                data['email'] = raw_input('   EMail: ')
            code, result = self.post_to_server(data)
            if code != 200:
                print 'Server response (%s): %s'%(code, result)
            else:
                print 'You will receive an email shortly.'
                print 'Follow the instructions in it to complete registration.'
        elif choice == '3':
            data = {':action': 'password_reset'}
            data['email'] = ''
            while not data['email']:
                data['email'] = raw_input('Your email address: ')
            code, result = self.post_to_server(data)
            print 'Server response (%s): %s'%(code, result)

    def build_post_data(self, action):
        # figure the data to send - the metadata plus some additional
        # information used by the package server
        meta = self.distribution.metadata
        data = {
            ':action': action,
            'metadata_version' : '1.0',
            'name': meta.get_name(),
            'version': meta.get_version(),
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
        if data['provides'] or data['requires'] or data['obsoletes']:
            data['metadata_version'] = '1.1'
        return data

    def post_to_server(self, data, auth=None):
        ''' Post a query to the server, and return a string response.
        '''
        self.announce('Registering %s to %s' % (data['name'],
                                                self.repository), log.INFO)
        # Build up the MIME payload for the urllib2 POST data
        boundary = '--------------GHSKFJDLGDS7543FJKLFHRE75642756743254'
        sep_boundary = '\n--' + boundary
        end_boundary = sep_boundary + '--'
        body = StringIO.StringIO()
        for key, value in data.items():
            # handle multiple entries for the same name
            if type(value) not in (type([]), type( () )):
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
        req = urllib2.Request(self.repository, body, headers)

        # handle HTTP and include the Basic Auth handler
        opener = urllib2.build_opener(
            urllib2.HTTPBasicAuthHandler(password_mgr=auth)
        )
        data = ''
        try:
            result = opener.open(req)
        except urllib2.HTTPError, e:
            if self.show_response:
                data = e.fp.read()
            result = e.code, e.msg
        except urllib2.URLError, e:
            result = 500, str(e)
        else:
            if self.show_response:
                data = result.read()
            result = 200, 'OK'
        if self.show_response:
            print '-'*75, data, '-'*75
        return result
