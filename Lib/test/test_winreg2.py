from winreg import hives, createKey, openKey, flags, deleteKey, regtypes
import sys, traceback
import time
import array

def testHives():
    hivenames = ["HKEY_CLASSES_ROOT","HKEY_CURRENT_USER","HKEY_LOCAL_MACHINE",
                "HKEY_USERS","HKEY_CURRENT_CONFIG"] #, # 
    for hivename in hivenames:
        hive = hives[ hivename ]
        print hive.name
        assert hive.handle
        assert hive.getSubkeys()
    for hivename in ["HKEY_DYN_DATA", "HKEY_PERFORMANCE_DATA"]:
        hive =  hives[ hivename ]
        print hive.name
        assert hive.handle
        assert not hive.getValues()
        assert not hive.getSubkeys()

def testNonZero():
    key=hives["HKLM"].openSubkey( "SOFTWARE" )
    assert key.openSubkey( "Microsoft" )
    assert key
    key.close()
    assert not key
    try:
        key.openSubkey( "Microsoft" )
        assert 0
    except EnvironmentError:
        pass

def testCmp():
    HKLM=hives["HKLM"]
    assert (openKey("HKLM\\SOFTWARE")==\
                        HKLM.openSubkey("SOFTWARE"))
    assert not HKLM.openSubkey("SYSTEM")!=HKLM.openSubkey("SYSTEM")
    assert HKLM.openSubkey("SOFTWARE")!=HKLM.openSubkey("SYSTEM")
    assert not HKLM.openSubkey("SOFTWARE")==HKLM.openSubkey("SYSTEM")
    assert not HKLM.openSubkey("SOFTWARE")=="spam"
    assert ((HKLM.openSubkey("SOFTWARE")<"spam") !=
                        (HKLM.openSubkey("SOFTWARE")>"spam"))

def testClose(): 
    key=hives["HKLM"].openSubkey( "SOFTWARE" )
    assert key
    key.close()
    assert not key

def testOpen():
    assert openKey( r"HKLM" )
    assert openKey( r"HKLM\HARDWARE" )
    assert openKey( r"HKLM\HARDWARE\DESCRIPTION\System" )

def testOpenFailure():
    try:
        print openKey( r"HKCU\Software\Python\A\B\C\D" )
        assert 0 #
    except EnvironmentError:
        pass

def testDeleteKey():
    createKey( r"HKCU\Software\Python\A\B\C\D" )
    deleteKey( r"HKCU\Software\Python\A\B\C\D" )
    deleteKey( r"HKCU\Software\Python\A\B\C" )
    deleteKey( r"HKCU\Software\Python\A\B" )
    assert "A" in \
            openKey(r"HKCU\Software\Python").getSubkeys().keys()
    openKey( r"HKCU\Software\Python" ).deleteSubkey( r"A" )
    assert "A" not in \
            openKey(r"HKCU\Software\Python").getSubkeys().keys()

def testDeleteValue():
    key=createKey( r"HKCU\Software\Python\A" )
    key.setValue( "abcde", "fghij" )
    assert key.getValueData( "abcde" )=="fghij"
    assert "abcde" in key.getValues().keys()
    assert "fghij" in map( lambda x:x[1], key.getValues().values() )
    assert "abcde" in key.getValues().keys()
    key.deleteValue( "abcde" )
    assert "abcde" not in key.getValues().keys()
    assert "fghij" not in map( lambda x:x[1], key.getValues().values() )
    deleteKey( r"HKCU\Software\Python\A" )

def testCreateKey():
    key=createKey( r"HKCU\Software\Python\A" )
    assert openKey( r"HKCU\Software\Python").getSubkeys().has_key( "A" )
    deleteKey( r"HKCU\Software\Python\A" )
    assert not openKey( r"HKCU\Software\Python").getSubkeys().\
                        has_key( "A" )
    key=openKey( r"HKCU\Software\Python" ).createSubkey( "A" )

def testOpenKeyWithFlags():
    assert openKey( r"HKCU\Software\Python", 
                                flags["KEY_READ"])
    assert openKey( r"HKCU\Software\Python", 
                                flags["KEY_ALL_ACCESS"])

def testGetSubkeys():
    keys=openKey( r"HKCU\Software" ).getSubkeys()
    assert keys
    index=0
    for i in keys:
        index=index+1
    assert index==len( keys )

def testGetValueNameDataAndType(): pass

def testGetSubkeyNames():
    subkeyNames=hives["HKLM"].getSubkeyNames()
    assert len( subkeyNames )==len(hives["HKLM"].getSubkeys())
    for name in subkeyNames:
        assert type( name )==type("")

def testGetValueNames(): 
    valNames=hives["HKLM"].getValueNames()
    assert len( valNames )==len(hives["HKLM"].getValues())
    for name in valNames:
        assert type( name )==type("")

def testRepr():
    assert repr(hives["HKCU"])==str(hives["HKCU"])

def testSetStringValue():
    hives["HKCU"].setValue( "Blah", "abc" )
    assert hives["HKCU"].getValueData( "Blah" )=="abc"
    assert hives["HKCU"].getValues().has_key( "Blah" )
    del hives["HKCU"].getValues()[ "Blah" ]
    assert not hives["HKCU"].getValues().has_key( "Blah" )

    # Ensure Unicode works as we expect
    hives["HKCU"].setValue( u"Blah", u"abc" )
    assert hives["HKCU"].getValueData( "Blah" )=="abc"
    assert hives["HKCU"].getValues().has_key( u"Blah" )
    del hives["HKCU"].getValues()[ u"Blah" ]
    assert not hives["HKCU"].getValues().has_key( u"Blah" )

def testDeleteValue():
    hives["HKCU"].setValue( "Blah", "abc" )
    assert hives["HKCU"].getValues().has_key( "Blah" )
    del hives["HKCU"].getValues()[ "Blah" ]
    assert not hives["HKCU"].getValues().has_key( "Blah" )
    hives["HKCU"].setValue( "Blah", "abc" )
    hives["HKCU"].deleteValue( "Blah" )
    assert not hives["HKCU"].getValues().has_key( "Blah" )

def testKeyDict_ClearKeys():
    createKey( "HKLM\\Software\\a\\b\\c\\d\\e" )
    createKey( "HKLM\\Software\\a\\b\\c\\d\\f" )
    createKey( "HKLM\\Software\\a\\b\\c\\d\\g" )
    createKey( "HKLM\\Software\\a\\b\\c\\d\\h" )
    createKey( "HKLM\\Software\\a\\b\\c\\d\\i" )
    key=openKey( "HKLM\\Software\\a\\b\\c\\d" )
    assert key.getSubkeys()
    key.getSubkeys().clear()
    assert not key.getSubkeys()
    assert not openKey( "HKLM\\Software\\a\\b\\c\\d").getSubkeys()
    deleteKey( "HKLM\\Software\\a\\b\\c\\d" )
    deleteKey( "HKLM\\Software\\a\\b\\c" )
    deleteKey( "HKLM\\Software\\a\\b" )
    deleteKey( "HKLM\\Software\\a" )

def testUnicodeKeyName(): pass

def testUnicodeValueName(): pass

def testGetValueDataFromEnum(): pass

def testGetValueDataFromName(): pass

def testGetBinaryData(): pass


def testSetIntValue():
    key=createKey( "HKLM\\Software\\a\\b")
    key.setValue( "abcd", 5 )
    assert key.getValueData( "abcd" )==5
    assert key.getValues()[ "abcd" ][1]==5
    key.deleteValue( "abcd" )
    key.getValues()["abcd"]=5
    assert key.getValues()[ "abcd" ][1]==5
    key.deleteValue( "abcd" )
    key.getValues()["abcd"]=(5,regtypes["REG_DWORD"])
    assert key.getValues()[ "abcd" ][1]==5
    key.deleteValue( "abcd" )
    deleteKey( "HKLM\\Software\\a\\b")
    deleteKey( "HKLM\\Software\\a")

def testSetBinaryValue(): 
    key=createKey( "HKLM\\Software\\a\\b")
    key.setValue( "abcd", array.array( 'c', "PPPPPPPPPPPPPPP") )
    key.setValue( "abcde", array.array( 'c', "PPPPPPPPPPPPPPP"), 
                       regtypes["REG_BINARY"] )
    assert key.getValues()["abcd"][1]==key.getValues()["abcde"][1]
    key = None # Remove our reference.
    deleteKey( "HKLM\\Software\\a\\b")
    deleteKey( "HKLM\\Software\\a")

def testSetNone(): pass

def testSetString(): pass

def testSetExpandString(): pass

def testSetBinaryData(): pass

def testSetDword(): pass

def testSetDwordBigEndian(): pass

def testSetLink(): pass

def testSetMultiSz(): pass

def testSetResourceList(): pass

def testSetFullResourceDescription(): pass

def testSetResourceRequirementsList(): pass



def testFlush(): pass

def testSave(): pass

def testLoad(): pass





def testNoneType(): pass

def testStringType(): pass

def testExpandStringType(): pass

def testDWordType(): pass

def testDWordBigEndianType(): pass

def testLinkType(): pass

def testMultiStringType(): pass

def testResourceLinkType(): pass

def testResourceDescriptionType(): pass

def testResourceRequirementsListType(): pass


def getSubkeysDict(): pass

def testKeyDict_Get(): pass

def testKeyDict_HasKey(): pass

def testKeyDict_Keys(): pass

def testKeyDict_Values(): pass

def testKeyDict_Items(): pass

def testKeyDict_Length(): pass

def testKeyDict_Map(): pass

def testKeyDict_GetItem(): pass

def testKeyDict_DelItem(): pass

def testRemote(): pass

def testShortcuts(): pass



def getValues(): pass

def testValueDict_ClearKeys(): pass

def testValueDict_Get(): pass

def testValueDict_HasKey(): pass

def testValueDict_Keys(): pass

def testValueDict_Values(): pass

def testValueDict_Items(): pass

def testValueDict_Length(): pass

def testValueDict_Map(): pass

def testValueDict_GetItem(): pass

def testValueDict_DelItem(): pass

for name in globals().keys():
    if name[0:4]=="test":
        func=globals()[ name ]
        try:
            func()
            print "Test Passed: %s" % name
        except Exception, e:
            print "Test Failed: %s" % name
            traceback.print_exc()

j=[0]*len( regtypes )

k=0

def test1( basekey ):
    global k
    for (name, data, type) in basekey.getValues():
        j[type.intval]=j[type.intval]+1
        if j[type.intval]==1:
            pass
            #print "[[%s]] %s [%s] = %s "% (type.msname, name, basekey, value)
    keys=basekey.getSubkeys()
    index=0
    k=k+1
    if(k%2500)==0:
        dump()
    while 1:
        try:
            test1( keys[index] )
            index=index+1
        except IndexError:
            break
            print "Done", basekey, index
        except (WindowsError, EnvironmentError):
            index=index+1
            dumpfile.write( "Skipping %s %s"% (basekey, index))

def dump(dumpfile):
    for i in range( len( j )):
        dumpfile.write( "%s %s\n" %( i,j[i] ))

def testRecursive():
    dumpfile=open( "foo.txt", "w" )
    start=time.time()
    for hive in hives.values():
        dumpfile.write( "Hive: %s\n" % hive )
        test1( hive )
        dump(dumpfile)
    print time.time()-start
