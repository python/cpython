f = Function(void, 'OpenDeskAcc',
	(Str255, 'name', InMode),
)
functions.append(f)

as_resource_body = """
return ResObj_New((Handle)_self->ob_itself);
"""

f = ManualGenerator("as_Resource", as_resource_body)
f.docstring = lambda : "Return this Menu as a Resource"

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

f = Function(void, 'DrawMenuBar',
)
functions.append(f)

