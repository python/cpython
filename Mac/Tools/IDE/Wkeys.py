spacekey                = ' '
returnkey               = '\r'
tabkey          = '\t'
enterkey                = '\003'
backspacekey    = '\010'
deletekey               = '\177'
clearkey                = '\033'
helpkey                 = '\005'

leftarrowkey    = '\034'
rightarrowkey   = '\035'
uparrowkey              = '\036'
downarrowkey    = '\037'
arrowkeys               = [leftarrowkey, rightarrowkey, uparrowkey, downarrowkey]

topkey          = '\001'
bottomkey               = '\004'
pageupkey               = '\013'
pagedownkey     = '\014'
scrollkeys              = [topkey, bottomkey, pageupkey, pagedownkey]

navigationkeys = arrowkeys + scrollkeys

keycodes = {
        "space"         : ' ',
        "return"                : '\r',
        "tab"                   : '\t',
        "enter"                 : '\003',
        "backspace"     : '\010',
        "delete"                : '\177',
        "help"          : '\005',
        "leftarrow"             : '\034',
        "rightarrow"    : '\035',
        "uparrow"               : '\036',
        "downarrow"     : '\037',
        "top"                   : '\001',
        "bottom"                : '\004',
        "pageup"                : '\013',
        "pagedown"      : '\014'
}

keynames = {}
for k, v in keycodes.items():
    keynames[v] = k
del k, v
