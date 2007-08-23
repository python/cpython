"""Suite Web Browser Suite: Class of events supported by Web Browser applications
Level 1, version 1

Generated from /Applications/Internet Explorer.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'WWW!'

class Web_Browser_Suite_Events:

    def Activate(self, _object=None, _attributes={}, **_arguments):
        """Activate: Activate Internet Explorer and optionally select window designated by Window Identifier.
        Required argument: Window Identifier
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Window Identifier of window to activate
        """
        _code = 'WWW!'
        _subcode = 'ACTV'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def CloseAllWindows(self, _no_object=None, _attributes={}, **_arguments):
        """CloseAllWindows: Closes all windows
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Success
        """
        _code = 'WWW!'
        _subcode = 'CLSA'

        if _arguments: raise TypeError('No optional args expected')
        if _no_object != None: raise TypeError('No direct arg expected')


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_CloseWindow = {
        'ID' : 'WIND',
        'Title' : 'TITL',
    }

    def CloseWindow(self, _no_object=None, _attributes={}, **_arguments):
        """CloseWindow: Close the window specified by either Window Identifier or Title. If no parameter is specified, close the top window.
        Keyword argument ID: ID of the window to close. (Can use -1 for top window)
        Keyword argument Title: Title of the window to close
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Success
        """
        _code = 'WWW!'
        _subcode = 'CLOS'

        aetools.keysubst(_arguments, self._argmap_CloseWindow)
        if _no_object != None: raise TypeError('No direct arg expected')


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def GetWindowInfo(self, _object, _attributes={}, **_arguments):
        """GetWindowInfo: Returns a window info record (URL/Title) for the specified window.
        Required argument: Window Identifier of the window
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns:
        """
        _code = 'WWW!'
        _subcode = 'WNFO'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def ListWindows(self, _no_object=None, _attributes={}, **_arguments):
        """ListWindows: Returns list of Window Identifiers for all open windows.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: undocumented, typecode 'list'
        """
        _code = 'WWW!'
        _subcode = 'LSTW'

        if _arguments: raise TypeError('No optional args expected')
        if _no_object != None: raise TypeError('No direct arg expected')


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_OpenURL = {
        'to' : 'INTO',
        'toWindow' : 'WIND',
        'Flags' : 'FLGS',
        'FormData' : 'POST',
        'MIME_Type' : 'MIME',
    }

    def OpenURL(self, _object, _attributes={}, **_arguments):
        """OpenURL: Retrieves URL off the Web.
        Required argument: Fully-qualified URL
        Keyword argument to: Target file for saving downloaded data
        Keyword argument toWindow: Target window for resource at URL (-1 for top window, 0 for new window)
        Keyword argument Flags: Valid Flags settings are: 1-Ignore the document cache; 2-Ignore the image cache; 4-Operate in background mode.
        Keyword argument FormData: data to post
        Keyword argument MIME_Type: MIME type of data being posted
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'OURL'

        aetools.keysubst(_arguments, self._argmap_OpenURL)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_ParseAnchor = {
        'withURL' : 'RELA',
    }

    def ParseAnchor(self, _object, _attributes={}, **_arguments):
        """ParseAnchor: Combines a base URL and a relative URL to produce a fully-qualified URL
        Required argument: Base URL
        Keyword argument withURL: Relative URL that is combined with the Base URL (in the direct object) to produce a fully-qualified URL.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Fully-qualified URL
        """
        _code = 'WWW!'
        _subcode = 'PRSA'

        aetools.keysubst(_arguments, self._argmap_ParseAnchor)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_ShowFile = {
        'MIME_Type' : 'MIME',
        'Window_Identifier' : 'WIND',
        'URL' : 'URL ',
    }

    def ShowFile(self, _object, _attributes={}, **_arguments):
        """ShowFile: FileSpec containing data of specified MIME type to be rendered in window specified by Window Identifier.
        Required argument: The file
        Keyword argument MIME_Type: MIME type
        Keyword argument Window_Identifier: Identifier of the target window for the URL. (Can use -1 for top window)
        Keyword argument URL: URL that allows this document to be reloaded.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'WWW!'
        _subcode = 'SHWF'

        aetools.keysubst(_arguments, self._argmap_ShowFile)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
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
