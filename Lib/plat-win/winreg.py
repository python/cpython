"""
Windows registry support.

openKey( keyname ) 
    open existing key object

>>> key=openKey( r"HKLM\HARDWARE\DESCRIPTION\System" )

createKey( keyname ) 
    create a key if it doesn't already exist

>>> key=createKey( r"HKLM\SOFTWARE\Python\Test" )

deleteKey( keyname ) 
    delete a key if it exists
    Note: deleteKey may not be recursive on all platforms.

>>> key=createKey( r"HKLM\SOFTWARE\Python\Test" )

RemoteKey( machine, top_level_key ): 
    open a key on another machine.
    You can use the returned key as a basis for opens, creates and deletes
    on the other machine.

>>> key=RemoteKey( "somemachine", "HKLM" )

For information on the key API, open a key and look at its docstring.


"""

import _winreg
import sys
import exceptions
import array
from types import *
import string

class RegType:
    "Represents one of the types that can go into the registry"
    def __init__( self, msname, friendlyname ):
        self.msname=msname
        self.friendlyname=friendlyname
        self.intval=getattr( _winreg, msname )

    def __repr__( self ):
        "Return a useful representation of the type object"
        return "<RegType %d: %s %s>" % \
                        (self.intval, self.msname, self.friendlyname )

_typeConstants={ 
    _winreg.REG_NONE: 
        RegType( "REG_NONE", "None" ),
    _winreg.REG_SZ: 
        RegType( "REG_SZ", "String" ),
    _winreg.REG_EXPAND_SZ: 
        RegType("REG_EXPAND_SZ", "Expandable Template String" ),
    _winreg.REG_BINARY: 
        RegType("REG_BINARY", "Binary Data"),
    _winreg.REG_DWORD : 
        RegType("REG_DWORD", "Integer" ),
#    _winreg.REG_DWORD_LITTLE_ENDIAN : 
#        RegType("REG_DWORD_LITTLE_ENDIAN", "Integer"),
    _winreg.REG_DWORD_BIG_ENDIAN : 
        RegType("REG_DWORD_BIG_ENDIAN", "Big Endian Integer"),
    _winreg.REG_LINK : 
        RegType("REG_LINK", "Link"),
    _winreg.REG_MULTI_SZ : 
        RegType("REG_MULTI_SZ", "List of Strings"),
    _winreg.REG_RESOURCE_LIST : 
        RegType("REG_RESOURCE_LIST", "Resource List"),
    _winreg.REG_FULL_RESOURCE_DESCRIPTOR : 
        RegType( "REG_FULL_RESOURCE_DESCRIPTOR", 
                                            "Full Resource Descriptor" ),
    _winreg.REG_RESOURCE_REQUIREMENTS_LIST: 
        RegType( "REG_RESOURCE_REQUIREMENTS_LIST", 
                                            "Resource Requirements List" )
}

regtypes={}
for constant in _typeConstants.values():
    regtypes[constant.msname]=constant

class _DictBase:
    "Base class for dictionary-type objects"
    def __init__( self, key ):
        self.key=key

    def clear( self ):
        "Clear the list of keys/data values, as in dictionaries"
        keys=list( self.keys() )
        map( self.__delitem__, keys )

    def get( self, item, defaultVal=None ):
        "Get a key/value by name or index"
        try:
            return self.__getitem__( item )
        except (IndexError, EnvironmentError, WindowsError):
            return defaultVal

    def has_key( self, item ):
        "Check if a key/data value with a particular name exists"
        try:
            self.__getitem__( item )
            return 1
        except (IndexError, EnvironmentError, WindowsError):
            return 0

    def keys( self ):
        "Get a list of key/data value names"
        keys=[] 
        try:
            for i in xrange( 0, sys.maxint ):
                keyname = self._nameFromNum( i  )
                keys.append( keyname )
        except (IndexError, EnvironmentError, WindowsError):
            pass
        return keys

    def values( self ):
        "Get a list of key objects or data values"
        values=[]  # map() doesn't use the IndexError semantics...
        for i in self:
            values.append( i )
        return values

    def items( self ):
        "Get pairs of keyname/key object or valuename/value data"
        return map( None, self.keys(), self.values() )

    def __len__( self ):
        return len( self.keys() )

def _getName( item, nameFromNum ):
    "Helper function -- don't use it directly"
    if type( item ) == IntType:
        try:
            keyname = nameFromNum( item )
        except (WindowsError, EnvironmentError):
            raise IndexError, item

    elif type( item )==StringType: 
        keyname=item
    else:
        raise exceptions.TypeError, \
                "Requires integer index or string key name" 
    return keyname


class RegValuesDict( _DictBase ):
    "A dictionary of registry data values"
    def _nameFromNum( self, i ):
        return self.key._nameFromNum( i )

    def __getitem__( self, item ):
        return self.key.getValueNameDataAndType( item )

    def __setitem__( self, name, val):
        if type( val )==TupleType:
            data, datatype=val
            assert isinstance( datatype, RegType )
            self.key.setValue( name, data, datatype )
        else:
            self.key.setValue( name, val )

    def __delitem__( self, item ):
        valname=_getName( item, self._nameFromNum )
        self.key.deleteValue( valname )
    
class RegKeysDict( _DictBase ):
    "A dictionary of registry keys"
    def _nameFromNum( self, item ):
        return _winreg.EnumKey( self.key.handle, item )

    def __getitem__( self, item ):
        keyname=_getName( item, self._nameFromNum )
        return self.key.openSubkey( keyname )

    def __delitem__( self, item ):
        keyname=_getName( item, self._nameFromNum )
        self.key.deleteSubkey( keyname )

def openKey( keyname, samFlags=None ):
    "Open a key by name"
    lst=string.split( keyname, "\\", 1 )
    if len( lst )==2:
        hivename,path=lst
        return hives[hivename].openSubkey( path )
    else:
        hivename=lst[0]
        return hives[hivename]

def createKey( keyname ):
    lst=string.split( keyname, "\\", 1 )
    assert len( lst )==2
    hivename,path=lst
    return hives[hivename].createSubkey( path )

def deleteKey( keyname ):
    lst=string.split( keyname, "\\", 1 )
    assert len( lst )==2
    hivename,path=lst
    return hives[hivename].deleteSubkey( path )


class RegKey:
    "A registry key object"

    def __init__( self, name, handle=None ):
        self.name=name
        self.handle=handle

    def _nameFromNum( self, item ):
        "internal"
        (name,data,datatype)=_winreg.EnumValue( self.handle, item )
        return name

    def __nonzero__(self):
        "Is the key open?"
        if self.handle:
            return 1
        else:
            return 0

    def __cmp__ (self, other ):
        "Compare two keys for equality"
        if hasattr( other, "handle" ) and hasattr( other, "name" ):
            return cmp( self.name, other.name )
        else:
            return cmp( self.handle, other )

    def __repr__( self ):
        return "<Windows RegKey: %s>"% self.name

    def close(self ):
        "Close the key"
        return _winreg.CloseKey( self.handle )

    def getSubkeyNames( self ):
        "Get a list of subkey names"
        return self.getSubkeys().keys()

    def getValueNames( self ):
        "Get a list of value names"
        return self.getValues().keys()

    def deleteSubkey( self, subkey ):
        "Delete a subkey by name"
        return _winreg.DeleteKey( self.handle, subkey )
        
    def deleteValue( self, valname ):
        "Delete a value by name"
        return _winreg.DeleteValue( self.handle, valname )

    def createSubkey( self, keyname ):
        "Create a subkey by name"
        handle=_winreg.CreateKey( self.handle, keyname )
        return RegKey( self.name+"\\"+keyname, handle)

    def openSubkey( self, keyname, samFlags=None ):
        "Open a named subkey"
        if samFlags:
            handle=_winreg.OpenKey( self.handle, keyname, 0, samFlags )
        else:
            handle=_winreg.OpenKey( self.handle, keyname, 0 )
        return RegKey( self.name+"\\"+keyname, handle )

    def getSubkeys( self ):
        "Get a dictionary-like mapping of subkeys"
        return RegKeysDict( self )

    def getValues( self ):
        "Get a dictionary-like mapping of data values"
        return RegValuesDict( self )

    def getValueNameDataAndType( self, valname ):
        "Get a data value's name, data and type all at one time"
        try:
            if type( valname )==IntType:
                (valname,data,datatype)=_winreg.EnumValue( self.handle, valname )
            else:
                keyname=_getName( valname, self._nameFromNum )
                (data,datatype)=_winreg.QueryValueEx( self.handle, keyname )
        except (WindowsError, EnvironmentError):
            raise IndexError, valname

        if datatype==_winreg.REG_BINARY:
            # use arrays for binary data
            data=array.array( 'c', data )

        return (valname, data, _typeConstants[datatype] )

    def getValueData( self, valname ):
        "Get a data value's data."
        name, data, type=self.getValueNameDataAndType( valname )
        return data

    def setValue( self, valname, data, regtype=None ):
        "Set a data value's data (and optionally type)"
        if regtype:
            typeint=regtype.intval
        else:
            if type( data )==StringType:
                typeint=_winreg.REG_SZ
            elif type( data )==IntType:
                typeint=_winreg.REG_DWORD
            elif type( data )==array.ArrayType:
                typeint=_winreg.REG_BINARY
                data=data.tostring()
        _winreg.SetValueEx( self.handle, valname, 0, typeint, data )

    def flush(self ):
        "Make sure that all changes are written to the registry. "
        "Only use this if you know what you are doing. "
        "It isn't usually needed."
        _winreg.FlushKey( self.keyobbj )

    def save( self, filename ):
        "Save a key to a filename"
        _winreg.SaveKey( self.keyobj, filename )

    def load( self, subkey, filename ):
        "Load a key from a filename"
        return _winreg.RegLoadKey( self.handle, subkey, filename )


class RemoteKey( RegKey ):
    "Open a key on a remote machine"
    def __init__( self, machine, topLevelKey ):
        assert topLevelKey in _hivenames 
        self.handle =  _winreg.ConnectRegistry( machine, parentKey )
        self.name=r"\\%s\%s" % (machine, topLevelKey )

_hivenames = ["HKEY_CLASSES_ROOT","HKEY_CURRENT_USER","HKEY_LOCAL_MACHINE",
                "HKEY_USERS","HKEY_CURRENT_CONFIG","HKEY_DYN_DATA",
                "HKEY_PERFORMANCE_DATA"]
hives={}
for name in _hivenames:
    hives[name]=RegKey( name, getattr( _winreg, name ) )
hives["HKLM"]=hives["HKEY_LOCAL_MACHINE"]
hives["HKCR"]=hives["HKEY_CLASSES_ROOT"]
hives["HKCU"]=hives["HKEY_CURRENT_USER"]

_flagnames = ["KEY_ALL_ACCESS","KEY_CREATE_LINK", "KEY_CREATE_SUB_KEY",
             "KEY_ENUMERATE_SUB_KEYS", "KEY_EXECUTE", "KEY_NOTIFY", 
             "KEY_QUERY_VALUE", "KEY_READ", "KEY_SET_VALUE"]
flags={}
for name in _flagnames:
    flags[name]=getattr( _winreg, name )

_RegNotifyChangeKeyValueOptions=[ "REG_NOTIFY_CHANGE_ATTRIBUTES",
            "REG_NOTIFY_CHANGE_SECURITY", "REG_NOTIFY_CHANGE_LAST_SET",
            "REG_NOTIFY_CHANGE_NAME", "REG_LEGAL_CHANGE_FILTER" ]

_RegRestoreKeyOptions=["REG_WHOLE_HIVE_VOLATILE", 
    "REG_NO_LAZY_FLUSH",
    "REG_OPTION_VOLATILE",
    "REG_REFRESH_HIVE",
    "REG_OPTION_NON_VOLATILE",
    "REG_OPTION_BACKUP_RESTORE" ]

_RegCreateKeyExOptions=[
    "REG_LEGAL_OPTION",
    "REG_OPTION_RESERVED",
    "REG_OPTION_VOLATILE",
    "REG_OPTION_NON_VOLATILE",
    "REG_OPTION_BACKUP_RESTORE",
    "REG_CREATED_NEW_KEY",
    "REG_OPENED_EXISTING_KEY",
    "REG_OPTION_CREATE_LINK"]


#unusednames=_RegNotifyChangeKeyValueOptions+_RegRestoreKeyOptions+_RegCreateKeyExOptions

#typeConstantNames=map( lambda x: x.msname, typeConstants.values() )

#allnames=_hivenames+_flagnames+typeConstantNames+unusednames
#winregnames=_winreg.__dict__.keys()
#for name in winregnames:
#    if name not in allnames:
#        print name

