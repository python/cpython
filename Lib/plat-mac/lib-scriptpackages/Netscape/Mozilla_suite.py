"""Suite Mozilla suite: Experimental Mozilla suite
Level 1, version 1

Generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'MOSS'

class Mozilla_suite_Events:

    def Get_Import_Data(self, _no_object=None, _attributes={}, **_arguments):
        """Get Import Data: Returns a structure containing information that is of use to an external module in importing data from an external mail application into Communicator.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: vRefNum and dirID of profile folder (2+4 bytes), vRefNum and DirID of the local mail folder (2+4 bytes), window type of front window (0 if none, \xd4Brwz\xd5 browser, \xd4Addr\xd5 addressbook, \xd4Mesg\xd5 messenger, etc., 4 bytes)
        """
        _code = 'MOSS'
        _subcode = 'Impt'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Get_Profile_Name(self, _no_object=None, _attributes={}, **_arguments):
        """Get Profile Name: Get the current User Profile
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Name of the current profile, like \xd2Joe Bloggs\xd3. This is the name of the profile folder in the Netscape Users folder.
        """
        _code = 'MOSS'
        _subcode = 'upro'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Get_workingURL(self, _no_object=None, _attributes={}, **_arguments):
        """Get workingURL: Get the path to the running application in URL format.  This will allow a script to construct a relative URL
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Will return text of the from \xd2FILE://foo/applicationname\xd3
        """
        _code = 'MOSS'
        _subcode = 'wurl'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_Go = {
        'direction' : 'dire',
    }

    def Go(self, _object, _attributes={}, **_arguments):
        """Go: navigate a window: back, forward, again(reload), home)
        Required argument: window
        Keyword argument direction: undocumented, typecode 'dire'
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'gogo'

        aetools.keysubst(_arguments, self._argmap_Go)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'dire', _Enum_dire)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Handle_command(self, _object, _attributes={}, **_arguments):
        """Handle command: Handle a command
        Required argument: The command to handle
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'hcmd'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Open_Address_Book(self, _no_object=None, _attributes={}, **_arguments):
        """Open Address Book: Opens the address book
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'addr'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Open_Component(self, _object, _attributes={}, **_arguments):
        """Open Component: Open a Communicator component
        Required argument: The component to open
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'cpnt'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Open_Profile_Manager(self, _no_object=None, _attributes={}, **_arguments):
        """Open Profile Manager: Open the user profile manager (obsolete)
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'prfl'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def Open_bookmark(self, _object=None, _attributes={}, **_arguments):
        """Open bookmark: Reads in a bookmark file
        Required argument: If not available, reloads the current bookmark file
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'book'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_Read_help_file = {
        'with_index' : 'idid',
        'search_text' : 'sear',
    }

    def Read_help_file(self, _object, _attributes={}, **_arguments):
        """Read help file: Reads in the help file (file should be in the help file format)
        Required argument: undocumented, typecode 'alis'
        Keyword argument with_index: Index to the help file. Defaults to  \xd4DEFAULT\xd5)
        Keyword argument search_text: Optional text to search for
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'MOSS'
        _subcode = 'help'

        aetools.keysubst(_arguments, self._argmap_Read_help_file)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

_Enum_comp = {
    'Navigator' : 'navg',       # The Navigator component
    'InBox' : 'inbx',   # The InBox component
    'Newsgroups' : 'colb',      # The Newsgroups component
    'Composer' : 'cpsr',        # The Page Composer component
    'Conference' : 'conf',      # The Conference Component
    'Calendar' : 'cald',        # The Calendar Component
}

_Enum_dire = {
    'again' : 'agai',   # Again (reload)
    'home' : 'home',    # Home
    'backward' : 'prev',        # Previous page
    'forward' : 'next', # Next page
}

_Enum_ncmd = {
    'Get_new_mail' : '\x00\x00\x04W',   #
    'Send_queued_messages' : '\x00\x00\x04X',   #
    'Read_newsgroups' : '\x00\x00\x04\x04',     #
    'Show_Inbox' : '\x00\x00\x04\x05',  #
    'Show_Bookmarks_window' : '\x00\x00\x04\x06',       #
    'Show_History_window' : '\x00\x00\x04\x07', #
    'Show_Address_Book_window' : '\x00\x00\x04\t',      #
}


#
# Indices of types declared in this module
#
_classdeclarations = {
}

_propdeclarations = {
}

_compdeclarations = {
}

_enumdeclarations = {
    'comp' : _Enum_comp,
    'dire' : _Enum_dire,
    'ncmd' : _Enum_ncmd,
}
