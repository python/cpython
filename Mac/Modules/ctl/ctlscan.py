# Scan <Controls.h>, generating ctlgen.py.
import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

def main():
#	input = "Controls.h" # Universal Headers < 3.3
	input = ["Controls.h", "ControlDefinitions.h"] # Universal Headers >= 3.3
	output = "ctlgen.py"
	defsoutput = TOOLBOXDIR + "Controls.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now doing 'import ctlsupport' ==="
	import ctlsupport
	print "=== Done.  It's up to you to compile Ctlmodule.c ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("ControlHandle", "ControlRef") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("from TextEdit import *\n")
		self.defsfile.write("from QuickDraw import *\n")
		self.defsfile.write("from Dragconst import *\n")
		self.defsfile.write("from CarbonEvents import *\n")
		self.defsfile.write("from Appearance import *\n")
		self.defsfile.write("kDataBrowserItemAnyState = -1\n")
		self.defsfile.write("kControlBevelButtonCenterPopupGlyphTag = -1\n")
		self.defsfile.write("kDataBrowserClientPropertyFlagsMask = 0xFF << 24  # kDataBrowserClientPropertyFlagsOffset\n")
		self.defsfile.write("\n")

	def makeblacklistnames(self):
		return [
			'FindControlUnderMouse', # Generated manually, returns an existing control, not a new one.
			'DisposeControl', # Generated manually
			'KillControls', # Implied by close of dialog
			'SetCtlAction',
			'TrackControl',	# Generated manually
			'HandleControlClick',	# Generated manually
			'SetControlData',	# Generated manually
			'GetControlData',	# Generated manually
			'kControlBevelButtonCenterPopupGlyphTag', # Constant with funny definition
			'kDataBrowserClientPropertyFlagsMask',  # ditto
			'kDataBrowserItemAnyState',   # and ditto
			# The following are unavailable for static 68k (appearance manager)
##			'GetBevelButtonMenuValue',
##			'SetBevelButtonMenuValue',
##			'GetBevelButtonMenuHandle',
##			'SetBevelButtonTransform',
			'SetBevelButtonGraphicAlignment',
			'SetBevelButtonTextAlignment',
			'SetBevelButtonTextPlacement',
##			'SetImageWellTransform',
##			'GetTabContentRect',
##			'SetTabEnabled',
##			'SetDisclosureTriangleLastValue',
## 			# Unavailable in CW Pro 3 libraries
## 			'SetUpControlTextColor',
## 			# Unavailable in Jack's CW Pro 5.1 libraries
## 			'GetControlRegion',
## 			'RemoveControlProperty',
## 			'IsValidControlHandle',
## 			'SetControl32BitMinimum',
## 			'GetControl32BitMinimum',
## 			'SetControl32BitMaximum',
## 			'GetControl32BitMaximum',
## 			'SetControl32BitValue',
## 			'GetControl32BitValue',
## 			'SetControlViewSize',
## 			'GetControlViewSize',
			# Generally Bad News
			'GetControlProperty',
			'SetControlProperty',
			'GetControlPropertySize',
			'SendControlMessage', # Parameter changed from long to void* from UH3.3 to UH3.4
			'CreateTabsControl',  # wrote manually
			
			# too lazy for now
			'GetImageWellContentInfo',
			'GetBevelButtonContentInfo',
			]

	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'GetAuxiliaryControlRecord',
				'SetControlColor',
				# These have suddenly disappeared in UH 3.3.2...
##				'GetBevelButtonMenuValue',
##				'SetBevelButtonMenuValue',
##				'GetBevelButtonMenuHandle',
##				'SetBevelButtonTransform',
##				'SetImageWellTransform',
##				'GetTabContentRect',
##				'SetTabEnabled',
##				'SetDisclosureTriangleLastValue',
			]),
			('#if TARGET_API_MAC_CARBON', [
				'IsAutomaticControlDragTrackingEnabledForWindow',
				'SetAutomaticControlDragTrackingEnabledForWindow',
				'HandleControlDragReceive',
				'HandleControlDragTracking',
				'GetControlByID',
				'IsControlDragTrackingEnabled',
				'SetControlDragTrackingEnabled',
				'GetControlPropertyAttributes',
				'ChangeControlPropertyAttributes',
				'GetControlID',
				'SetControlID',
				'HandleControlSetCursor',
				'GetControlClickActivation',
				'HandleControlContextualMenuClick',
				
				"CreateBevelButtonControl",
				"CreateImageWellControl",
				"CreatePictureControl",
				"CreateIconControl",
				"CreatePushButtonWithIconControl",
				"SetBevelButtonContentInfo",
				"SetImageWellContentInfo",
				"AddDataBrowserListViewColumn",
				
				"CreateDataBrowserControl",
				"CreateScrollingTextBoxControl",
				"CreateRadioGroupControl",
				"CreatePopupButtonControl",
				"CreateCheckBoxControl",
				"CreateRadioButtonControl",
				"CreatePushButtonControl",
				"CreateWindowHeaderControl",
				"CreateStaticTextControl",
				"CreateEditTextControl",
				"CreateUserPaneControl",
				"CreateClockControl",
				"CreatePlacardControl",
				"CreatePopupArrowControl",
				"CreatePopupGroupBoxControl",
				"CreateCheckGroupBoxControl",
				"CreateGroupBoxControl",
				"CreateSeparatorControl",
				"CreateChasingArrowsControl",
				"CreateLittleArrowsControl",
				"CreateProgressBarControl",
				"CreateDisclosureTriangleControl",
				"GetDataBrowserColumnViewDisplayType",
				"SetDataBrowserColumnViewDisplayType",
				"GetDataBrowserColumnViewPathLength",
				"GetDataBrowserColumnViewPath",
				"GetDataBrowserListViewDisclosureColumn",
				"SetDataBrowserListViewDisclosureColumn",
				"GetDataBrowserListViewUsePlainBackground",
				"SetDataBrowserListViewUsePlainBackground",
				"GetDataBrowserListViewHeaderBtnHeight",
				"SetDataBrowserListViewHeaderBtnHeight",
				"AutoSizeDataBrowserListViewColumns",
				"GetDataBrowserTableViewColumnProperty",
				"GetDataBrowserTableViewColumnPosition",
				"SetDataBrowserTableViewColumnPosition",
				"GetDataBrowserTableViewItemRow",
				"SetDataBrowserTableViewItemRow",
				"GetDataBrowserTableViewItemID",
				"GetDataBrowserTableViewGeometry",
				"SetDataBrowserTableViewGeometry",
				"GetDataBrowserTableViewNamedColumnWidth",
				"SetDataBrowserTableViewNamedColumnWidth",
				"GetDataBrowserTableViewItemRowHeight",
				"SetDataBrowserTableViewItemRowHeight",
				"GetDataBrowserTableViewColumnWidth",
				"SetDataBrowserTableViewColumnWidth",
				"GetDataBrowserTableViewRowHeight",
				"SetDataBrowserTableViewRowHeight",
				"GetDataBrowserTableViewHiliteStyle",
				"SetDataBrowserTableViewHiliteStyle",
				"GetDataBrowserTableViewColumnCount",
				"RemoveDataBrowserTableViewColumn",
				"GetDataBrowserItemPartBounds",
				"GetDataBrowserEditItem",
				"SetDataBrowserEditItem",
				"GetDataBrowserEditText",
				"SetDataBrowserEditText",
				"GetDataBrowserPropertyFlags",
				"SetDataBrowserPropertyFlags",
				"GetDataBrowserSelectionFlags",
				"SetDataBrowserSelectionFlags",
				"GetDataBrowserSortProperty",
				"SetDataBrowserSortProperty",
				"GetDataBrowserHasScrollBars",
				"SetDataBrowserHasScrollBars",
				"GetDataBrowserScrollPosition",
				"SetDataBrowserScrollPosition",
				"GetDataBrowserSortOrder",
				"SetDataBrowserSortOrder",
				"GetDataBrowserTarget",
				"SetDataBrowserTarget",
				"GetDataBrowserScrollBarInset",
				"SetDataBrowserScrollBarInset",
				"GetDataBrowserActiveItems",
				"SetDataBrowserActiveItems",
				"RevealDataBrowserItem",
				"GetDataBrowserItemState",
				"IsDataBrowserItemSelected",
				"GetDataBrowserItemCount",
				"GetDataBrowserItems",
				"SortDataBrowserContainer",
				"CloseDataBrowserContainer",
				"OpenDataBrowserContainer",
				"MoveDataBrowserSelectionAnchor",
				"GetDataBrowserSelectionAnchor",
				"ExecuteDataBrowserEditCommand",
				"EnableDataBrowserEditCommand",
				"SetDataBrowserViewStyle",
				"GetDataBrowserViewStyle",
				"GetControlCommandID",
				"SetControlCommandID",
				"CopyControlTitleAsCFString",
				"SetControlTitleWithCFString",
			]),
			('#if ACCESSOR_CALLS_ARE_FUNCTIONS', [
				# XXX These are silly, they should be #defined to access the fields
				# directly. Later...
				'GetControlBounds',
				'IsControlHilited',
				'GetControlHilite',
				'GetControlOwner',
				'GetControlDataHandle',
				'GetControlPopupMenuHandle',
				'GetControlPopupMenuID',
				'SetControlDataHandle',
				'SetControlBounds',
				'SetControlPopupMenuHandle',
				'SetControlPopupMenuID',
			]),
			('#if TARGET_API_MAC_OSX', [
				'CreateRoundButtonControl',
				'CreateDisclosureButtonControl',
				'CreateRelevanceBarControl',
				'DisableControl',
				'EnableControl',
				'IsControlEnabled',
				'CreateEditUnicodeTextControl',
				'CopyDataBrowserEditText',
			]),
			]
			
	def makeblacklisttypes(self):
		return [
			'ProcPtr',
			'ControlActionUPP',
			'Ptr',
			'ControlDefSpec', # Don't know how to do this yet
			'ControlDefSpec_ptr', # ditto
			'Collection', # Ditto
			# not-yet-supported stuff in Universal Headers 3.4:
			'ControlColorUPP',
			'ControlKind',  # XXX easy: 2-tuple containing 2 OSType's
#			'ControlTabEntry_ptr', # XXX needed for tabs
#			'ControlButtonContentInfoPtr',
#			'ControlButtonContentInfo',  # XXX ugh: a union
#			'ControlButtonContentInfo_ptr',  # XXX ugh: a union
			'ListDefSpec_ptr',  # XXX see _Listmodule.c, tricky but possible
			'DataBrowserItemID_ptr',  # XXX array of UInt32, for BrowserView
			'DataBrowserItemUPP',
			'DataBrowserItemDataRef', # XXX void *
			'DataBrowserCallbacks', # difficult struct
			'DataBrowserCallbacks_ptr',
			'DataBrowserCustomCallbacks',
			'DataBrowserCustomCallbacks_ptr',
##			'DataBrowserTableViewColumnDesc',
##			'DataBrowserListViewColumnDesc',
			'CFDataRef',
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),

			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),

##			# For TrackControl
##			([("ProcPtr", "actionProc", "InMode")],
##			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),
##			([("ControlActionUPP", "actionProc", "InMode")],
##			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),

			# For GetControlTitle
			([('Str255', 'title', 'InMode')],
			 [('Str255', 'title', 'OutMode')]),

			([("ControlHandle", "*", "OutMode")],
			 [("ExistingControlHandle", "*", "*")]),
			([("ControlRef", "*", "OutMode")],	# Ditto, for Universal Headers
			 [("ExistingControlHandle", "*", "*")]),
			 
			([("Rect_ptr", "*", "ReturnMode")], # GetControlBounds
			 [("void", "*", "ReturnMode")]),

			([("DataBrowserListViewColumnDesc", "*", "OutMode")],
			 [("DataBrowserListViewColumnDesc", "*", "InMode")]),
			 
			([("ControlButtonContentInfoPtr", 'outContent', "InMode")],
			 [("ControlButtonContentInfoPtr", '*', "OutMode")]),
			 
			([("ControlButtonContentInfo", '*', "OutMode")],
			 [("ControlButtonContentInfo", '*', "InMode")]),
			]

if __name__ == "__main__":
	main()
