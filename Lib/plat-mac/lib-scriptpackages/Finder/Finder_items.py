"""Suite Finder items: Commands used with file system items, and basic item definition
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_items_Events:

    def add_to_favorites(self, _object, _attributes={}, **_arguments):
        """add to favorites: (NOT AVAILABLE YET) Add the items to the user\xd5s Favorites
        Required argument: the items to add to the collection of Favorites
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'ffav'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_clean_up = {
        'by' : 'by  ',
    }

    def clean_up(self, _object, _attributes={}, **_arguments):
        """clean up: (NOT AVAILABLE YET) Arrange items in window nicely (only applies to open windows in icon view that are not kept arranged)
        Required argument: the window to clean up
        Keyword argument by: the order in which to clean up the objects (name, index, date, etc.)
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'fclu'

        aetools.keysubst(_arguments, self._argmap_clean_up)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def eject(self, _object=None, _attributes={}, **_arguments):
        """eject: Eject the specified disk(s)
        Required argument: the disk(s) to eject
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'ejct'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def empty(self, _object=None, _attributes={}, **_arguments):
        """empty: Empty the trash
        Required argument: \xd2empty\xd3 and \xd2empty trash\xd3 both do the same thing
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'empt'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def erase(self, _object, _attributes={}, **_arguments):
        """erase: (NOT AVAILABLE) Erase the specified disk(s)
        Required argument: the items to erase
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'fera'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def reveal(self, _object, _attributes={}, **_arguments):
        """reveal: Bring the specified object(s) into view
        Required argument: the object to be made visible
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'mvis'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_update = {
        'necessity' : 'nec?',
        'registering_applications' : 'reg?',
    }

    def update(self, _object, _attributes={}, **_arguments):
        """update: Update the display of the specified object(s) to match their on-disk representation
        Required argument: the item to update
        Keyword argument necessity: only update if necessary (i.e. a finder window is open).  default is false
        Keyword argument registering_applications: register applications. default is true
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'fupd'

        aetools.keysubst(_arguments, self._argmap_update)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class item(aetools.ComponentItem):
    """item - An item """
    want = 'cobj'
class _Prop_bounds(aetools.NProperty):
    """bounds - the bounding rectangle of the item (can only be set for an item in a window viewed as icons or buttons) """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_comment(aetools.NProperty):
    """comment - the comment of the item, displayed in the \xd2Get Info\xd3 window """
    which = 'comt'
    want = 'utxt'
class _Prop_container(aetools.NProperty):
    """container - the container of the item """
    which = 'ctnr'
    want = 'obj '
class _Prop_creation_date(aetools.NProperty):
    """creation date - the date on which the item was created """
    which = 'ascd'
    want = 'ldt '
class _Prop_description(aetools.NProperty):
    """description - a description of the item """
    which = 'dscr'
    want = 'utxt'
class _Prop_disk(aetools.NProperty):
    """disk - the disk on which the item is stored """
    which = 'cdis'
    want = 'obj '
class _Prop_displayed_name(aetools.NProperty):
    """displayed name - the user-visible name of the item """
    which = 'dnam'
    want = 'utxt'
class _Prop_everyones_privileges(aetools.NProperty):
    """everyones privileges -  """
    which = 'gstp'
    want = 'priv'
class _Prop_extension_hidden(aetools.NProperty):
    """extension hidden - Is the item's extension hidden from the user? """
    which = 'hidx'
    want = 'bool'
class _Prop_group(aetools.NProperty):
    """group - the user or group that has special access to the container """
    which = 'sgrp'
    want = 'utxt'
class _Prop_group_privileges(aetools.NProperty):
    """group privileges -  """
    which = 'gppr'
    want = 'priv'
class _Prop_icon(aetools.NProperty):
    """icon - the icon bitmap of the item """
    which = 'iimg'
    want = 'ifam'
class _Prop_index(aetools.NProperty):
    """index - the index in the front-to-back ordering within its container """
    which = 'pidx'
    want = 'long'
class _Prop_information_window(aetools.NProperty):
    """information window - the information window for the item """
    which = 'iwnd'
    want = 'obj '
class _Prop_kind(aetools.NProperty):
    """kind - the kind of the item """
    which = 'kind'
    want = 'utxt'
class _Prop_label_index(aetools.NProperty):
    """label index - the label of the item """
    which = 'labi'
    want = 'long'
class _Prop_locked(aetools.NProperty):
    """locked - Is the file locked? """
    which = 'aslk'
    want = 'bool'
class _Prop_modification_date(aetools.NProperty):
    """modification date - the date on which the item was last modified """
    which = 'asmo'
    want = 'ldt '
class _Prop_name(aetools.NProperty):
    """name - the name of the item """
    which = 'pnam'
    want = 'utxt'
class _Prop_name_extension(aetools.NProperty):
    """name extension - the name extension of the item (such as \xd2txt\xd3) """
    which = 'nmxt'
    want = 'utxt'
class _Prop_owner(aetools.NProperty):
    """owner - the user that owns the container """
    which = 'sown'
    want = 'utxt'
class _Prop_owner_privileges(aetools.NProperty):
    """owner privileges -  """
    which = 'ownr'
    want = 'priv'
class _Prop_physical_size(aetools.NProperty):
    """physical size - the actual space used by the item on disk """
    which = 'phys'
    want = 'comp'
class _Prop_position(aetools.NProperty):
    """position - the position of the item within its parent window (can only be set for an item in a window viewed as icons or buttons) """
    which = 'posn'
    want = 'QDpt'
class _Prop_properties(aetools.NProperty):
    """properties - every property of an item """
    which = 'pALL'
    want = 'reco'
class _Prop_size(aetools.NProperty):
    """size - the logical size of the item """
    which = 'ptsz'
    want = 'comp'
class _Prop_url(aetools.NProperty):
    """url - the url of the item """
    which = 'pURL'
    want = 'utxt'

items = item
item._superclassnames = []
item._privpropdict = {
    'bounds' : _Prop_bounds,
    'comment' : _Prop_comment,
    'container' : _Prop_container,
    'creation_date' : _Prop_creation_date,
    'description' : _Prop_description,
    'disk' : _Prop_disk,
    'displayed_name' : _Prop_displayed_name,
    'everyones_privileges' : _Prop_everyones_privileges,
    'extension_hidden' : _Prop_extension_hidden,
    'group' : _Prop_group,
    'group_privileges' : _Prop_group_privileges,
    'icon' : _Prop_icon,
    'index' : _Prop_index,
    'information_window' : _Prop_information_window,
    'kind' : _Prop_kind,
    'label_index' : _Prop_label_index,
    'locked' : _Prop_locked,
    'modification_date' : _Prop_modification_date,
    'name' : _Prop_name,
    'name_extension' : _Prop_name_extension,
    'owner' : _Prop_owner,
    'owner_privileges' : _Prop_owner_privileges,
    'physical_size' : _Prop_physical_size,
    'position' : _Prop_position,
    'properties' : _Prop_properties,
    'size' : _Prop_size,
    'url' : _Prop_url,
}
item._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cobj' : item,
}

_propdeclarations = {
    'ascd' : _Prop_creation_date,
    'aslk' : _Prop_locked,
    'asmo' : _Prop_modification_date,
    'cdis' : _Prop_disk,
    'comt' : _Prop_comment,
    'ctnr' : _Prop_container,
    'dnam' : _Prop_displayed_name,
    'dscr' : _Prop_description,
    'gppr' : _Prop_group_privileges,
    'gstp' : _Prop_everyones_privileges,
    'hidx' : _Prop_extension_hidden,
    'iimg' : _Prop_icon,
    'iwnd' : _Prop_information_window,
    'kind' : _Prop_kind,
    'labi' : _Prop_label_index,
    'nmxt' : _Prop_name_extension,
    'ownr' : _Prop_owner_privileges,
    'pALL' : _Prop_properties,
    'pURL' : _Prop_url,
    'pbnd' : _Prop_bounds,
    'phys' : _Prop_physical_size,
    'pidx' : _Prop_index,
    'pnam' : _Prop_name,
    'posn' : _Prop_position,
    'ptsz' : _Prop_size,
    'sgrp' : _Prop_group,
    'sown' : _Prop_owner,
}

_compdeclarations = {
}

_enumdeclarations = {
}
