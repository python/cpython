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
kOnSystemDisk                 = -32768	# previously was 0x8000 but that is an unsigned value whereas vRefNum is signed
kOnAppropriateDisk            = -32767	# Generally, the same as kOnSystemDisk, but it's clearer that this isn't always the 'boot' disk.
# Folder Domains - Carbon only.  
kSystemDomain                 = -32766	# Read-only system hierarchy.
kLocalDomain                  = -32765	# All users of a single machine have access to these resources.
kNetworkDomain                = -32764	# All users configured to use a common network server has access to these resources.
kUserDomain                   = -32763	# Read/write. Resources that are private to the user.
kClassicDomain                = -32762	# Domain referring to the currently configured Classic System Folder

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

# Alias types
kContainerFolderAliasType	= 'fdrp'
kContainerTrashAliasType	= 'trsh'
kContainerHardDiskAliasType	= 'hdsk'
kContainerFloppyAliasType	= 'flpy'
kContainerServerAliasType	= 'srvr'
kApplicationAliasType		= 'adrp'
kContainerAliasType			= 'drop'
# types for Special folder aliases
kSystemFolderAliasType		= 'fasy'
kAppleMenuFolderAliasType	= 'faam'
kStartupFolderAliasType		= 'fast'
kPrintMonitorDocsFolderAliasType = 'fapn'
kPreferencesFolderAliasType	= 'fapf'
kControlPanelFolderAliasType = 'fact'
kExtensionFolderAliasType	= 'faex'
kExportedFolderAliasType	= 'faet'
kDropFolderAliasType		= 'fadr'
kSharedFolderAliasType		= 'fash'
kMountedFolderAliasType		= 'famn'

# New FindFolder constants
kExtensionDisabledFolderType = 'extD'
kControlPanelDisabledFolderType = 'ctrD'
kSystemExtensionDisabledFolderType = 'macD'
kStartupItemsDisabledFolderType = 'strD'
kShutdownItemsDisabledFolderType = 'shdD'
kApplicationsFolderType		= 'apps'
kDocumentsFolderType		= 'docs'

kVolumeRootFolderType		= 'root'
kChewableItemsFolderType	= 'flnt'
kApplicationSupportFolderType = 'asup'
kTextEncodingsFolderType	= '€tex'
kStationeryFolderType		= 'odst'
kOpenDocFolderType			= 'odod'
kOpenDocShellPlugInsFolderType = 'odsp'
kEditorsFolderType			= 'oded'
kOpenDocEditorsFolderType	= '€odf'
kOpenDocLibrariesFolderType	= 'odlb'
kGenEditorsFolderType		= '€edi'
kHelpFolderType				= '€hlp'
kInternetPlugInFolderType	= '€net'
kModemScriptsFolderType		= '€mod'
kPrinterDescriptionFolderType = 'ppdf'
kPrinterDriverFolderType	= '€prd'
kScriptingAdditionsFolderType = '€scr'
kSharedLibrariesFolderType	= '€lib'
kVoicesFolderType			= 'fvoc'
kControlStripModulesFolderType = 'sdev'
kAssistantsFolderType		= 'ast€'
kUtilitiesFolderType		= 'uti€'
kAppleExtrasFolderType		= 'aex€'
kContextualMenuItemsFolderType = 'cmnu'
kMacOSReadMesFolderType		= 'mor€'
kALMModulesFolderType		= 'walk'
kALMPreferencesFolderType	= 'trip'
kALMLocationsFolderType		= 'fall'
kColorSyncProfilesFolderType = 'prof'
kThemesFolderType			= 'thme'
kFavoritesFolderType		= 'favs'

# Don't remember why this is here
READ = 1
WRITE = 2
smAllScripts = -3

