# Generated from 'Sap:CW8 Gold:Metrowerks CodeWarrior:MacOS Support:Headers:Universal Headers:Components.h'

f = Function(Component, 'RegisterComponentResource',
    (ComponentResourceHandle, 'tr', InMode),
    (short, 'global', InMode),
)
functions.append(f)

f = Method(OSErr, 'UnregisterComponent',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Function(Component, 'FindNextComponent',
    (Component, 'aComponent', InMode),
    (ComponentDescription, 'looking', InMode),
)
functions.append(f)

f = Function(long, 'CountComponents',
    (ComponentDescription, 'looking', InMode),
)
functions.append(f)

f = Method(OSErr, 'GetComponentInfo',
    (Component, 'aComponent', InMode),
    (ComponentDescription, 'cd', OutMode),
    (Handle, 'componentName', InMode),
    (Handle, 'componentInfo', InMode),
    (Handle, 'componentIcon', InMode),
)
c_methods.append(f)

f = Function(long, 'GetComponentListModSeed',
)
functions.append(f)

f = Method(ComponentInstance, 'OpenComponent',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Method(OSErr, 'CloseComponent',
    (ComponentInstance, 'aComponentInstance', InMode),
)
ci_methods.append(f)

f = Method(OSErr, 'GetComponentInstanceError',
    (ComponentInstance, 'aComponentInstance', InMode),
)
ci_methods.append(f)

f = Method(long, 'ComponentFunctionImplemented',
    (ComponentInstance, 'ci', InMode),
    (short, 'ftnNumber', InMode),
)
ci_methods.append(f)

f = Method(long, 'GetComponentVersion',
    (ComponentInstance, 'ci', InMode),
)
ci_methods.append(f)

f = Method(long, 'ComponentSetTarget',
    (ComponentInstance, 'ci', InMode),
    (ComponentInstance, 'target', InMode),
)
ci_methods.append(f)

f = Method(void, 'SetComponentInstanceError',
    (ComponentInstance, 'aComponentInstance', InMode),
    (OSErr, 'theError', InMode),
)
ci_methods.append(f)

f = Method(long, 'GetComponentRefcon',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Method(void, 'SetComponentRefcon',
    (Component, 'aComponent', InMode),
    (long, 'theRefcon', InMode),
)
c_methods.append(f)

f = Method(short, 'OpenComponentResFile',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Function(OSErr, 'CloseComponentResFile',
    (short, 'refnum', InMode),
)
functions.append(f)

f = Method(Handle, 'GetComponentInstanceStorage',
    (ComponentInstance, 'aComponentInstance', InMode),
)
ci_methods.append(f)

f = Method(void, 'SetComponentInstanceStorage',
    (ComponentInstance, 'aComponentInstance', InMode),
    (Handle, 'theStorage', InMode),
)
ci_methods.append(f)

f = Method(long, 'GetComponentInstanceA5',
    (ComponentInstance, 'aComponentInstance', InMode),
)
ci_methods.append(f)

f = Method(void, 'SetComponentInstanceA5',
    (ComponentInstance, 'aComponentInstance', InMode),
    (long, 'theA5', InMode),
)
ci_methods.append(f)

f = Method(long, 'CountComponentInstances',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Method(OSErr, 'SetDefaultComponent',
    (Component, 'aComponent', InMode),
    (short, 'flags', InMode),
)
c_methods.append(f)

f = Function(ComponentInstance, 'OpenDefaultComponent',
    (OSType, 'componentType', InMode),
    (OSType, 'componentSubType', InMode),
)
functions.append(f)

f = Method(Component, 'CaptureComponent',
    (Component, 'capturedComponent', InMode),
    (Component, 'capturingComponent', InMode),
)
c_methods.append(f)

f = Method(OSErr, 'UncaptureComponent',
    (Component, 'aComponent', InMode),
)
c_methods.append(f)

f = Function(long, 'RegisterComponentResourceFile',
    (short, 'resRefNum', InMode),
    (short, 'global', InMode),
)
functions.append(f)

f = Method(OSErr, 'GetComponentIconSuite',
    (Component, 'aComponent', InMode),
    (Handle, 'iconSuite', OutMode),
)
c_methods.append(f)

