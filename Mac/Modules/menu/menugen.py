# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Menus.h'

f = Function(void, 'InitMenus',
)
functions.append(f)

f = Function(MenuHandle, 'NewMenu',
    (short, 'menuID', InMode),
    (Str255, 'menuTitle', InMode),
)
functions.append(f)

f = Function(MenuHandle, 'GetMenu',
    (short, 'resourceID', InMode),
)
functions.append(f)

f = Method(void, 'DisposeMenu',
    (MenuHandle, 'theMenu', InMode),
)
methods.append(f)

f = Method(void, 'AppendMenu',
    (MenuHandle, 'menu', InMode),
    (ConstStr255Param, 'data', InMode),
)
methods.append(f)

f = Method(void, 'AddResMenu',
    (MenuHandle, 'theMenu', InMode),
    (ResType, 'theType', InMode),
)
methods.append(f)

f = Method(void, 'InsertResMenu',
    (MenuHandle, 'theMenu', InMode),
    (ResType, 'theType', InMode),
    (short, 'afterItem', InMode),
)
methods.append(f)

f = Method(void, 'InsertMenu',
    (MenuHandle, 'theMenu', InMode),
    (short, 'beforeID', InMode),
)
methods.append(f)

f = Function(void, 'DrawMenuBar',
)
functions.append(f)

f = Function(void, 'InvalMenuBar',
)
functions.append(f)

f = Function(void, 'DeleteMenu',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'ClearMenuBar',
)
functions.append(f)

f = Function(Handle, 'GetNewMBar',
    (short, 'menuBarID', InMode),
)
functions.append(f)

f = Function(Handle, 'GetMenuBar',
)
functions.append(f)

f = Function(void, 'SetMenuBar',
    (Handle, 'menuList', InMode),
)
functions.append(f)

f = Method(void, 'InsMenuItem',
    (MenuHandle, 'theMenu', InMode),
    (ConstStr255Param, 'itemString', InMode),
    (short, 'afterItem', InMode),
)
methods.append(f)

f = Method(void, 'DelMenuItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Function(long, 'MenuKey',
    (short, 'ch', InMode),
)
functions.append(f)

f = Function(void, 'HiliteMenu',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Method(void, 'SetItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (ConstStr255Param, 'itemString', InMode),
)
methods.append(f)

f = Method(void, 'GetItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (Str255, 'itemString', OutMode),
)
methods.append(f)

f = Method(void, 'DisableItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Method(void, 'EnableItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Method(void, 'CheckItem',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (Boolean, 'checked', InMode),
)
methods.append(f)

f = Method(void, 'SetItemMark',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'markChar', InMode),
)
methods.append(f)

f = Method(void, 'GetItemMark',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'markChar', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemIcon',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'iconIndex', InMode),
)
methods.append(f)

f = Method(void, 'GetItemIcon',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'iconIndex', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemStyle',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'chStyle', InMode),
)
methods.append(f)

f = Method(void, 'GetItemStyle',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (Style, 'chStyle', OutMode),
)
methods.append(f)

f = Method(void, 'CalcMenuSize',
    (MenuHandle, 'theMenu', InMode),
)
methods.append(f)

f = Method(short, 'CountMItems',
    (MenuHandle, 'theMenu', InMode),
)
methods.append(f)

f = Function(MenuHandle, 'GetMHandle',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'FlashMenuBar',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'SetMenuFlash',
    (short, 'count', InMode),
)
functions.append(f)

f = Function(long, 'MenuSelect',
    (Point, 'startPt', InMode),
)
functions.append(f)

f = Function(void, 'InitProcMenu',
    (short, 'resID', InMode),
)
functions.append(f)

f = Method(void, 'GetItemCmd',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'cmdChar', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemCmd',
    (MenuHandle, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'cmdChar', InMode),
)
methods.append(f)

f = Method(long, 'PopUpMenuSelect',
    (MenuHandle, 'menu', InMode),
    (short, 'top', InMode),
    (short, 'left', InMode),
    (short, 'popUpItem', InMode),
)
methods.append(f)

f = Function(long, 'MenuChoice',
)
functions.append(f)

f = Function(void, 'DelMCEntries',
    (short, 'menuID', InMode),
    (short, 'menuItem', InMode),
)
functions.append(f)
