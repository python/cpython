f = Function(MenuHandle, 'as_Menu', (Handle, 'h', InMode))
functions.append(f)

f = Method(Handle, 'as_Resource', (MenuHandle, 'h', InMode))
methods.append(f)

# The following have "Mac" prepended to their names in the include file
# since UH 3.1, sigh...
f = Function(MenuHandle, 'GetMenu',
    (short, 'resourceID', InMode),
)
functions.append(f)

f = Method(void, 'AppendMenu',
    (MenuHandle, 'menu', InMode),
    (ConstStr255Param, 'data', InMode),
)
methods.append(f)

f = Method(void, 'InsertMenu',
    (MenuHandle, 'theMenu', InMode),
    (short, 'beforeID', InMode),
)
methods.append(f)

f = Function(void, 'DeleteMenu',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Method(void, 'InsertMenuItem',
    (MenuHandle, 'theMenu', InMode),
    (ConstStr255Param, 'itemString', InMode),
    (short, 'afterItem', InMode),
)
methods.append(f)

f = Method(void, 'EnableMenuItem',
    (MenuHandle, 'theMenu', InMode),
    (UInt16, 'item', InMode),
)
methods.append(f)

f = Method(void, 'CheckMenuItem',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (Boolean, 'checked', InMode),
)
methods.append(f)


f = Function(void, 'DrawMenuBar',
)
functions.append(f)


#
# The following functions take an *optional* MenuRef as their first argument
#

f = Function(ItemCount, 'CountMenuItemsWithCommandID',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
)
functions.append(f)

f = Function(OSStatus, 'GetIndMenuItemWithCommandID',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
    (UInt32, 'inItemIndex', InMode),
    (MenuRef, 'outMenu', OutMode),
    (MenuItemIndex, 'outIndex', OutMode),
)
functions.append(f)

f = Function(void, 'EnableMenuCommand',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
)
functions.append(f)

f = Function(void, 'DisableMenuCommand',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
)
functions.append(f)

f = Function(Boolean, 'IsMenuCommandEnabled',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
)
functions.append(f)

f = Function(OSStatus, 'SetMenuCommandMark',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
    (UniChar, 'inMark', InMode),
)
functions.append(f)

f = Function(OSStatus, 'GetMenuCommandMark',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
    (UniChar, 'outMark', OutMode),
)
functions.append(f)

f = Function(OSStatus, 'GetMenuCommandPropertySize',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
    (OSType, 'inPropertyCreator', InMode),
    (OSType, 'inPropertyTag', InMode),
    (ByteCount, 'outSize', OutMode),
)
functions.append(f)

f = Function(OSStatus, 'RemoveMenuCommandProperty',
    (OptMenuRef, 'inMenu', InMode),
    (MenuCommand, 'inCommandID', InMode),
    (OSType, 'inPropertyCreator', InMode),
    (OSType, 'inPropertyTag', InMode),
)
functions.append(f)
