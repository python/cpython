# MACFS.py - Constants used by macfs routines
# Derived from Finder.h and Folders.h

# Flags in FInfo.Flags field:
kIsOnDesk					= 0x1
kColor						= 0xE
kIsShared					= 0x40
kHasBeenInited				= 0x100
kHasCustomIcon				= 0x400
kIsStationery				= 0x800
kIsStationary				= 0x800
kNameLocked					= 0x1000
kHasBundle					= 0x2000
kIsInvisible				= 0x4000
kIsAlias					= 0x8000

# Constants for FindFolder
kOnSystemDisk				= 0x8000
kSystemFolderType			= 'macs'	# the system folder
kDesktopFolderType			= 'desk'	# the desktop folder; objects in this folder show on the desk top.
kTrashFolderType			= 'trsh'	# the trash folder; objects in this folder show up in the trash
kWhereToEmptyTrashFolderType = 'empt'	# the "empty trash" folder; Finder starts empty from here down
kPrintMonitorDocsFolderType	= 'prnt'	# Print Monitor documents
kStartupFolderType			= 'strt'	# Finder objects (applications, documents, DAs, aliases, to...) to open at startup go here
kAppleMenuFolderType		= 'amnu'	# Finder objects to put into the Apple menu go here
kControlPanelFolderType		= 'ctrl'	# Control Panels go here (may contain INITs)
kExtensionFolderType		= 'extn'	# Finder extensions go here
kFontsFolderType			= 'font'	# Fonts go here
kPreferencesFolderType		= 'pref'	# preferences for applications go here
kTemporaryFolderType		= 'temp'
