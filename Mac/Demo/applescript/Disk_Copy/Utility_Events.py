"""Suite Utility Events: Commands that allow the user to select Disk Copy files
Level 1, version 1

Generated from Macintosh HD:Hulpprogramma's:Disk Copy
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'ddsk'

class Utility_Events_Events:

    _argmap_select_disk_image = {
            'with_prompt' : 'SELp',
    }

    def select_disk_image(self, _no_object=None, _attributes={}, **_arguments):
        """select disk image: Prompt the user to select a disk image
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to a disk image
        """
        _code = 'UTIL'
        _subcode = 'SEL1'

        aetools.keysubst(_arguments, self._argmap_select_disk_image)
        if _no_object is not None: raise TypeError('No direct arg expected')y

        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_select_DiskScript = {
            'with_prompt' : 'SELp',
    }

    def select_DiskScript(self, _no_object=None, _attributes={}, **_arguments):
        """select DiskScript: Prompt the user to select a DiskScript
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to a DiskScript
        """
        _code = 'UTIL'
        _subcode = 'SEL2'

        aetools.keysubst(_arguments, self._argmap_select_DiskScript)
        if _no_object is not None: raise TypeError('No direct arg expected')
        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_select_disk_image_or_DiskScript = {
            'with_prompt' : 'SELp',
    }

    def select_disk_image_or_DiskScript(self, _no_object=None, _attributes={}, **_arguments):
        """select disk image or DiskScript: Prompt the user to select a disk image or DiskScript
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to disk image or a DiskScript
        """
        _code = 'UTIL'
        _subcode = 'SEL3'

        aetools.keysubst(_arguments, self._argmap_select_disk_image_or_DiskScript)
        if _no_object is not None: raise TypeError('No direct arg expected')

        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_select_floppy_disk_image = {
            'with_prompt' : 'SELp',
    }

    def select_floppy_disk_image(self, _no_object=None, _attributes={}, **_arguments):
        """select floppy disk image: Prompt the user to select a floppy disk image
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to a floppy disk image
        """
        _code = 'UTIL'
        _subcode = 'SEL4'

        aetools.keysubst(_arguments, self._argmap_select_floppy_disk_image)
        if _no_object is not None: raise TypeError('No direct arg expected')

        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_select_disk = {
            'with_prompt' : 'SELp',
    }

    def select_disk(self, _no_object=None, _attributes={}, **_arguments):
        """select disk: Prompt the user to select a disk volume
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to the disk
        """
        _code = 'UTIL'
        _subcode = 'SEL5'

        aetools.keysubst(_arguments, self._argmap_select_disk)
        if _no_object is not None: raise TypeError('No direct arg expected')

        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_select_folder = {
            'with_prompt' : 'SELp',
    }

    def select_folder(self, _no_object=None, _attributes={}, **_arguments):
        """select folder: Prompt the user to select a folder
        Keyword argument with_prompt: the prompt string to be displayed
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a reference to the folder
        """
        _code = 'UTIL'
        _subcode = 'SEL6'

        aetools.keysubst(_arguments, self._argmap_select_folder)
        if _no_object is not None: raise TypeError('No direct arg expected')

        aetools.enumsubst(_arguments, 'SELp', _Enum_TEXT)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_log = {
            'time_stamp' : 'TSMP',
    }

    def log(self, _object, _attributes={}, **_arguments):
        """log: Add a string to the log window
        Required argument: the string to add to the log window
        Keyword argument time_stamp: Should the log entry be time-stamped? (false if not supplied)
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'UTIL'
        _subcode = 'LOG '

        aetools.keysubst(_arguments, self._argmap_log)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'TSMP', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.has_key('errn'):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

_Enum_TEXT = None # XXXX enum TEXT not found!!
_Enum_bool = None # XXXX enum bool not found!!

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
