# Generated from 'SWDev:Codewarrior Pro 5:Metrowerks CodeWarrior:MacOS Support:Universal:Interfaces:CIncludes:Drag.h'

def FOUR_CHAR_CODE(x): return x
from TextEdit import *
from QuickDraw import *


kDragHasLeftSenderWindow	= (1 << 0)
kDragInsideSenderApplication = (1 << 1)
kDragInsideSenderWindow		= (1 << 2)
kDragRegionAndImage			= (1 << 4)
flavorSenderOnly			= (1 << 0)
flavorSenderTranslated		= (1 << 1)
flavorNotSaved				= (1 << 2)
flavorSystemTranslated		= (1 << 8)
kDragStandardTranslucency = 0L
kDragDarkTranslucency = 1L
kDragDarkerTranslucency = 2L
kDragOpaqueTranslucency = 3L
kDragRegionBegin = 1
kDragRegionDraw = 2
kDragRegionHide = 3
kDragRegionIdle = 4
kDragRegionEnd = 5
kZoomNoAcceleration = 0
kZoomAccelerate = 1
kZoomDecelerate = 2
flavorTypeHFS = FOUR_CHAR_CODE('hfs ')
flavorTypePromiseHFS = FOUR_CHAR_CODE('phfs')
flavorTypeDirectory = FOUR_CHAR_CODE('diry')
kFlavorTypeClippingName = FOUR_CHAR_CODE('clnm')
kFlavorTypeClippingFilename = FOUR_CHAR_CODE('clfn')
kFlavorTypeDragToTrashOnly = FOUR_CHAR_CODE('fdtt')
kFlavorTypeFinderNoTrackingBehavior = FOUR_CHAR_CODE('fntb')
kDragTrackingEnterHandler = 1
kDragTrackingEnterWindow = 2
kDragTrackingInWindow = 3
kDragTrackingLeaveWindow = 4
kDragTrackingLeaveHandler = 5
dragHasLeftSenderWindow = kDragHasLeftSenderWindow
dragInsideSenderApplication = kDragInsideSenderApplication
dragInsideSenderWindow = kDragInsideSenderWindow
dragTrackingEnterHandler = kDragTrackingEnterHandler
dragTrackingEnterWindow = kDragTrackingEnterWindow
dragTrackingInWindow = kDragTrackingInWindow
dragTrackingLeaveWindow = kDragTrackingLeaveWindow
dragTrackingLeaveHandler = kDragTrackingLeaveHandler
dragRegionBegin = kDragRegionBegin
dragRegionDraw = kDragRegionDraw
dragRegionHide = kDragRegionHide
dragRegionIdle = kDragRegionIdle
dragRegionEnd = kDragRegionEnd
zoomNoAcceleration = kZoomNoAcceleration
zoomAccelerate = kZoomAccelerate
zoomDecelerate = kZoomDecelerate
kDragStandardImage = kDragStandardTranslucency
kDragDarkImage = kDragDarkTranslucency
kDragDarkerImage = kDragDarkerTranslucency
kDragOpaqueImage = kDragOpaqueTranslucency
