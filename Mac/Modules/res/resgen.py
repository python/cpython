# Generated from 'Sap:CodeWarrior6:Metrowerks C/C++:Headers:Universal Headers 2.0.1f:Resources.h'

f = ResFunction(short, 'InitResources',
)
functions.append(f)

f = ResFunction(void, 'RsrcZoneInit',
)
functions.append(f)

f = ResFunction(void, 'CloseResFile',
    (short, 'refNum', InMode),
)
functions.append(f)

f = ResFunction(short, 'ResError',
)
functions.append(f)

f = ResFunction(short, 'CurResFile',
)
functions.append(f)

f = ResMethod(short, 'HomeResFile',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResFunction(void, 'CreateResFile',
    (ConstStr255Param, 'fileName', InMode),
)
functions.append(f)

f = ResFunction(short, 'OpenResFile',
    (ConstStr255Param, 'fileName', InMode),
)
functions.append(f)

f = ResFunction(void, 'UseResFile',
    (short, 'refNum', InMode),
)
functions.append(f)

f = ResFunction(short, 'CountTypes',
)
functions.append(f)

f = ResFunction(short, 'Count1Types',
)
functions.append(f)

f = ResFunction(void, 'GetIndType',
    (ResType, 'theType', OutMode),
    (short, 'index', InMode),
)
functions.append(f)

f = ResFunction(void, 'Get1IndType',
    (ResType, 'theType', OutMode),
    (short, 'index', InMode),
)
functions.append(f)

f = ResFunction(void, 'SetResLoad',
    (Boolean, 'load', InMode),
)
functions.append(f)

f = ResFunction(short, 'CountResources',
    (ResType, 'theType', InMode),
)
functions.append(f)

f = ResFunction(short, 'Count1Resources',
    (ResType, 'theType', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'GetIndResource',
    (ResType, 'theType', InMode),
    (short, 'index', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'Get1IndResource',
    (ResType, 'theType', InMode),
    (short, 'index', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'GetResource',
    (ResType, 'theType', InMode),
    (short, 'theID', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'Get1Resource',
    (ResType, 'theType', InMode),
    (short, 'theID', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'GetNamedResource',
    (ResType, 'theType', InMode),
    (ConstStr255Param, 'name', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'Get1NamedResource',
    (ResType, 'theType', InMode),
    (ConstStr255Param, 'name', InMode),
)
functions.append(f)

f = ResMethod(void, 'LoadResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'ReleaseResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'DetachResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResFunction(short, 'UniqueID',
    (ResType, 'theType', InMode),
)
functions.append(f)

f = ResFunction(short, 'Unique1ID',
    (ResType, 'theType', InMode),
)
functions.append(f)

f = ResMethod(short, 'GetResAttrs',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'GetResInfo',
    (Handle, 'theResource', InMode),
    (short, 'theID', OutMode),
    (ResType, 'theType', OutMode),
    (Str255, 'name', OutMode),
)
resmethods.append(f)

f = ResMethod(void, 'SetResInfo',
    (Handle, 'theResource', InMode),
    (short, 'theID', InMode),
    (ConstStr255Param, 'name', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'AddResource',
    (Handle, 'theData', InMode),
    (ResType, 'theType', InMode),
    (short, 'theID', InMode),
    (ConstStr255Param, 'name', InMode),
)
resmethods.append(f)

f = ResMethod(long, 'GetResourceSizeOnDisk',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(long, 'GetMaxResourceSize',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(long, 'RsrcMapEntry',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'SetResAttrs',
    (Handle, 'theResource', InMode),
    (short, 'attrs', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'ChangedResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResMethod(void, 'RemoveResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResFunction(void, 'UpdateResFile',
    (short, 'refNum', InMode),
)
functions.append(f)

f = ResMethod(void, 'WriteResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

f = ResFunction(void, 'SetResPurge',
    (Boolean, 'install', InMode),
)
functions.append(f)

f = ResFunction(short, 'GetResFileAttrs',
    (short, 'refNum', InMode),
)
functions.append(f)

f = ResFunction(void, 'SetResFileAttrs',
    (short, 'refNum', InMode),
    (short, 'attrs', InMode),
)
functions.append(f)

f = ResFunction(short, 'OpenRFPerm',
    (ConstStr255Param, 'fileName', InMode),
    (short, 'vRefNum', InMode),
    (SignedByte, 'permission', InMode),
)
functions.append(f)

f = ResFunction(Handle, 'RGetResource',
    (ResType, 'theType', InMode),
    (short, 'theID', InMode),
)
functions.append(f)

f = ResFunction(short, 'HOpenResFile',
    (short, 'vRefNum', InMode),
    (long, 'dirID', InMode),
    (ConstStr255Param, 'fileName', InMode),
    (SignedByte, 'permission', InMode),
)
functions.append(f)

f = ResFunction(void, 'HCreateResFile',
    (short, 'vRefNum', InMode),
    (long, 'dirID', InMode),
    (ConstStr255Param, 'fileName', InMode),
)
functions.append(f)

f = ResFunction(short, 'FSpOpenResFile',
    (FSSpec_ptr, 'spec', InMode),
    (SignedByte, 'permission', InMode),
)
functions.append(f)

f = ResFunction(void, 'FSpCreateResFile',
    (FSSpec_ptr, 'spec', InMode),
    (OSType, 'creator', InMode),
    (OSType, 'fileType', InMode),
    (ScriptCode, 'scriptTag', InMode),
)
functions.append(f)

f = ResMethod(void, 'SetResourceSize',
    (Handle, 'theResource', InMode),
    (long, 'newSize', InMode),
)
resmethods.append(f)

f = ResMethod(Handle, 'GetNextFOND',
    (Handle, 'fondHandle', InMode),
)
resmethods.append(f)

