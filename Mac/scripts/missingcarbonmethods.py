# Methods that are missing in Carbon.
# This module is mainly for documentation purposes, but you could use
# it to automatically search for usage of methods that are missing.
#

missing_icglue = [
	'ICFindConfigFile',
	'ICFindUserConfigFile',
	'ICChooseConfig',
	'ICChooseNewConfig',
]

missing_Help = [
	'Help'
]

missing_Scrap = [
	'InfoScrap',
	'GetScrap',
	'ZeroScrap',
	'PutScrap',
]
	
missing_Win = [
	'GetAuxWin',
	'GetWindowDataHandle',
	'SaveOld',
	'DrawNew',
	'SetWinColor',
	'SetDeskCPat',
	'InitWindows',
	'InitFloatingWindows',
	'GetWMgrPort',
	'GetCWMgrPort',
	'ValidRgn',		# Use versions with Window in their name
	'ValidRect',
	'InvalRgn',
	'InvalRect',
	'IsValidWindowPtr', # I think this is useless for Python, but not sure...
	'GetWindowZoomFlag',	# Not available in Carbon
	'GetWindowTitleWidth',	# Ditto
	]

missing_Snd = [
	'MACEVersion',
	'SPBRecordToFile',
	'Exp1to6',
	'Comp6to1',
	'Exp1to3',
	'Comp3to1',
	'SndControl',
	'SndStopFilePlay',
	'SndStartFilePlay',
	'SndPauseFilePlay',
	]

missing_Res = [
	'RGetResource',
	'OpenResFile',
	'CreateResFile',
	'RsrcZoneInit',
	'InitResources',
	'RsrcMapEntry',
	]

missing_Qt = [
	'SpriteMediaGetIndImageProperty',	# XXXX Why isn't this in carbon?
	'CheckQuickTimeRegistration',
	'SetMovieAnchorDataRef',
	'GetMovieAnchorDataRef',
	'GetMovieLoadState',
	'OpenADataHandler',
	'MovieMediaGetCurrentMovieProperty',
	'MovieMediaGetCurrentTrackProperty',
	'MovieMediaGetChildMovieDataReference',
	'MovieMediaSetChildMovieDataReference',
	'MovieMediaLoadChildMovieFromDataReference',
	'Media3DGetViewObject',
	]

missing_Qd = [
##	'device',	# Too many false positives
	'portBits',
	'portPixMap',
	'portVersion',
	'grafVars',
	]

missing_Qdoffs = [
	]


missing_Menu = [
	'GetMenuItemRefCon2',
	'SetMenuItemRefCon2',
	'EnableItem',
	'DisableItem',
	'CheckItem',
	'CountMItems',
	'OpenDeskAcc',
	'SystemEdit',
	'SystemMenu',
	'SetMenuFlash',
	'InitMenus',
	'InitProcMenu',
	]

missing_List = [
	]

missing_Icn = [
	'IconServicesTerminate',
	]

missing_Fm = [
	'InitFonts',
	'SetFontLock',
	'FlushFonts',
	]

missing_Evt = [
	'SystemEvent',
	'SystemTask',
	'SystemClick',
	'GetOSEvent',
	'OSEventAvail',
	]

missing_Dlg = [
	'SetGrafPortOfDialog',
	]

missing_Ctl = [
	'GetAuxiliaryControlRecord',
	'SetControlColor',
	]

missing_Cm = [
	'SetComponentInstanceA5',
	'GetComponentInstanceA5',
	]

missing_App = [
	'GetThemeMetric',
	]

missing_AE = [
	'AEGetDescDataSize',
	'AEReplaceDescData',
	]
	

missing = []
for name in dir():
	if name[:8] == 'missing_':
		missing = missing + eval(name)
del name
		
def _search():
	# Warning: this function only works on Unix
	import string, os
	re = string.join(missing, '|')
	re = """[^a-zA-Z0-9_'"](%s)[^a-zA-Z0-9_'"]""" % re
	os.system("find . -name '*.py' -print | xargs egrep '%s'"%re)

if __name__ == '__main__':
	_search()
