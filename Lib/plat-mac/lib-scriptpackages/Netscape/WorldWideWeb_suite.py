"""Suite WorldWideWeb suite, as defined in Spyglass spec.:
Level 1, version 1

Generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'WWW!'

class WorldWideWeb_suite_Events:

    _argmap_OpenURL = {
        'to' : 'INTO',
        'toWindow' : 'WIND',
        'flags' : 'FLGS',
        'post_data' : 'POST',
        'post_type' : 'MIME',
        'progressApp' : 'PROG',
    }

    def OpenURL(self, _object, _attributes={}, **_arguments):
        """OpenURL: Opens a URL. Allows for more options than GetURL event
        Required argument: URL
        Keyword argument to: file destination
        Keyword argument toWindow: window iD
        Keyword argument flags: Binary: any combination of 1, 2 and 4 is allowed: 1 and 2 mean force reload the document. 4 is ignored
        Keyword argument post_data: Form posting data
        Keyword argument post_type: MIME type of the posting data. Defaults to application/x-www-form-urlencoded
        Keyword argument progressApp: Application that will display progress
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: ID of the loading window
        """
        _code = 'WWW!'
        _subcode = 'OURL'

        aetools.keysubst(_arguments, self._argmap_OpenURL)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_ShowFile = {
        'MIME_type' : 'MIME',
        'Window_ID' : 'WIND',
        'URL' : 'URL ',
    }

    def ShowFile(self, _object, _attributes={}, **_arguments):
        """ShowFile: Similar to OpenDocuments, except that it specifies the parent URL, and MIME type of the file
        Required argument: File to open
        Keyword argument MIME_type: MIME type
        Keyword argument Window_ID: Window to open the file in
        Keyword argument URL: Use this as a base URL
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Window ID of the loaded window. 0 means ShowFile failed, FFFFFFF means that data was not appropriate type to display in the browser.
        """
        _code = 'WWW!'
        _subcode = 'SHWF'

        aetools.keysubst(_arguments, self._argmap_ShowFile)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_cancel_progress = {
        'in_window' : 'WIND',
    }

    def cancel_progress(self, _object=None, _attributes={}, **_arguments):
        """cancel progress: Interrupts the download of the document in the given window
        Required argument: progress ID, obtained from the progress app
        Keyword argument in_window: window ID of the progress to cancel
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'CNCL'

        aetools.keysubst(_arguments, self._argmap_cancel_progress)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def find_URL(self, _object, _attributes={}, **_arguments):
        """find URL: If the file was downloaded by Netscape, you can call FindURL to find out the URL used to download the file.
        Required argument: File spec
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: The URL
        """
        _code = 'WWW!'
        _subcode = 'FURL'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def get_window_info(self, _object=None, _attributes={}, **_arguments):
        """get window info: Returns the information about the window as a list. Currently the list contains the window title and the URL. You can get the same information using standard Apple Event GetProperty.
        Required argument: window ID
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: undocumented, typecode 'list'
        """
        _code = 'WWW!'
        _subcode = 'WNFO'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def list_windows(self, _no_object=None, _attributes={}, **_arguments):
        """list windows: Lists the IDs of all the hypertext windows
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: List of unique IDs of all the hypertext windows
        """
        _code = 'WWW!'
        _subcode = 'LSTW'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_parse_anchor = {
        'relative_to' : 'RELA',
    }

    def parse_anchor(self, _object, _attributes={}, **_arguments):
        """parse anchor: Resolves the relative URL
        Required argument: Main URL
        Keyword argument relative_to: Relative URL
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Parsed  URL
        """
        _code = 'WWW!'
        _subcode = 'PRSA'

        aetools.keysubst(_arguments, self._argmap_parse_anchor)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def register_URL_echo(self, _object=None, _attributes={}, **_arguments):
        """register URL echo: Registers the \xd2echo\xd3 application. Each download from now on will be echoed to this application.
        Required argument: Application signature
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'RGUE'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_register_protocol = {
        'for_protocol' : 'PROT',
    }

    def register_protocol(self, _object=None, _attributes={}, **_arguments):
        """register protocol: Registers application as a \xd2handler\xd3 for this protocol with a given prefix. The handler will receive \xd2OpenURL\xd3, or if that fails, \xd2GetURL\xd3 event.
        Required argument: Application sig
        Keyword argument for_protocol: protocol prefix: \xd2finger:\xd3, \xd2file\xd3,
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: TRUE if registration has been successful
        """
        _code = 'WWW!'
        _subcode = 'RGPR'

        aetools.keysubst(_arguments, self._argmap_register_protocol)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_register_viewer = {
        'MIME_type' : 'MIME',
        'with_file_type' : 'FTYP',
    }

    def register_viewer(self, _object, _attributes={}, **_arguments):
        """register viewer: Registers an application as a \xd4special\xd5 viewer for this MIME type. The application will be launched with ViewDoc events
        Required argument: Application sig
        Keyword argument MIME_type: MIME type viewer is registering for
        Keyword argument with_file_type: Mac file type for the downloaded files
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: TRUE if registration has been successful
        """
        _code = 'WWW!'
        _subcode = 'RGVW'

        aetools.keysubst(_arguments, self._argmap_register_viewer)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_register_window_close = {
        'for_window' : 'WIND',
    }

    def register_window_close(self, _object=None, _attributes={}, **_arguments):
        """register window close: Netscape will notify registered application when this window closes
        Required argument: Application signature
        Keyword argument for_window: window ID
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: true if successful
        """
        _code = 'WWW!'
        _subcode = 'RGWC'

        aetools.keysubst(_arguments, self._argmap_register_window_close)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def unregister_URL_echo(self, _object, _attributes={}, **_arguments):
        """unregister URL echo: cancels URL echo
        Required argument: application signature
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'UNRU'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_unregister_protocol = {
        'for_protocol' : 'PROT',
    }

    def unregister_protocol(self, _object=None, _attributes={}, **_arguments):
        """unregister protocol: reverses the effects of \xd2register protocol\xd3
        Required argument: Application sig.
        Keyword argument for_protocol: protocol prefix. If none, unregister for all protocols
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: TRUE if successful
        """
        _code = 'WWW!'
        _subcode = 'UNRP'

        aetools.keysubst(_arguments, self._argmap_unregister_protocol)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_unregister_viewer = {
        'MIME_type' : 'MIME',
    }

    def unregister_viewer(self, _object, _attributes={}, **_arguments):
        """unregister viewer: Revert to the old way of handling this MIME type
        Required argument: Application sig
        Keyword argument MIME_type: MIME type to be unregistered
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: TRUE if the event was successful
        """
        _code = 'WWW!'
        _subcode = 'UNRV'

        aetools.keysubst(_arguments, self._argmap_unregister_viewer)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_unregister_window_close = {
        'for_window' : 'WIND',
    }

    def unregister_window_close(self, _object=None, _attributes={}, **_arguments):
        """unregister window close: Undo for register window close
        Required argument: Application signature
        Keyword argument for_window: window ID
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: true if successful
        """
        _code = 'WWW!'
        _subcode = 'UNRC'

        aetools.keysubst(_arguments, self._argmap_unregister_window_close)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def webActivate(self, _object=None, _attributes={}, **_arguments):
        """webActivate: Makes Netscape the frontmost application, and selects a given window. This event is here for suite completeness/ cross-platform compatibility only, you should use standard AppleEvents instead.
        Required argument: window to bring to front
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'ACTV'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


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
}
