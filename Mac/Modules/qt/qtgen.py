# Generated from 'Sap:CodeWarrior7:Metrowerks CodeWarrior:MacOS Support:Headers:Universal Headers:Movies.h'

f = Function(OSErr, 'EnterMovies',
)
functions.append(f)

f = Function(void, 'ExitMovies',
)
functions.append(f)

f = Function(OSErr, 'GetMoviesError',
)
functions.append(f)

f = Function(void, 'ClearMoviesStickyError',
)
functions.append(f)

f = Function(OSErr, 'GetMoviesStickyError',
)
functions.append(f)

f = Method(void, 'MoviesTask',
    (Movie, 'theMovie', InMode),
    (long, 'maxMilliSecToUse', InMode),
)
methods.append(f)

f = Method(OSErr, 'PrerollMovie',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (Fixed, 'Rate', InMode),
)
methods.append(f)

f = Method(OSErr, 'LoadMovieIntoRam',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
    (long, 'flags', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieActive',
    (Movie, 'theMovie', InMode),
    (Boolean, 'active', InMode),
)
methods.append(f)

f = Method(Boolean, 'GetMovieActive',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'StartMovie',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'StopMovie',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'GoToBeginningOfMovie',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'GoToEndOfMovie',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(Boolean, 'IsMovieDone',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(Boolean, 'GetMoviePreviewMode',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMoviePreviewMode',
    (Movie, 'theMovie', InMode),
    (Boolean, 'usePreview', InMode),
)
methods.append(f)

f = Method(void, 'ShowMoviePoster',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(PicHandle, 'GetMoviePict',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
)
methods.append(f)

f = Method(PicHandle, 'GetMoviePosterPict',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(OSErr, 'UpdateMovie',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'GetMovieBox',
    (Movie, 'theMovie', InMode),
    (Rect, 'boxRect', OutMode),
)
methods.append(f)

f = Method(void, 'SetMovieBox',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'boxRect', InMode),
)
methods.append(f)

f = Method(RgnHandle, 'GetMovieDisplayClipRgn',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieDisplayClipRgn',
    (Movie, 'theMovie', InMode),
    (RgnHandle, 'theClip', InMode),
)
methods.append(f)

f = Method(RgnHandle, 'GetMovieClipRgn',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieClipRgn',
    (Movie, 'theMovie', InMode),
    (RgnHandle, 'theClip', InMode),
)
methods.append(f)

f = Method(RgnHandle, 'GetMovieDisplayBoundsRgn',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(RgnHandle, 'GetMovieBoundsRgn',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Function(Movie, 'NewMovie',
    (long, 'flags', InMode),
)
functions.append(f)

f = Method(OSErr, 'PutMovieIntoHandle',
    (Movie, 'theMovie', InMode),
    (Handle, 'publicMovie', InMode),
)
methods.append(f)

f = Method(OSErr, 'PutMovieIntoDataFork',
    (Movie, 'theMovie', InMode),
    (short, 'fRefNum', InMode),
    (long, 'offset', InMode),
    (long, 'maxSize', InMode),
)
methods.append(f)

f = Method(TimeScale, 'GetMovieTimeScale',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieTimeScale',
    (Movie, 'theMovie', InMode),
    (TimeScale, 'timeScale', InMode),
)
methods.append(f)

f = Method(TimeValue, 'GetMovieDuration',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(Fixed, 'GetMovieRate',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieRate',
    (Movie, 'theMovie', InMode),
    (Fixed, 'rate', InMode),
)
methods.append(f)

f = Method(Fixed, 'GetMoviePreferredRate',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMoviePreferredRate',
    (Movie, 'theMovie', InMode),
    (Fixed, 'rate', InMode),
)
methods.append(f)

f = Method(short, 'GetMoviePreferredVolume',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMoviePreferredVolume',
    (Movie, 'theMovie', InMode),
    (short, 'volume', InMode),
)
methods.append(f)

f = Method(short, 'GetMovieVolume',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieVolume',
    (Movie, 'theMovie', InMode),
    (short, 'volume', InMode),
)
methods.append(f)

f = Method(void, 'GetMoviePreviewTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'previewTime', OutMode),
    (TimeValue, 'previewDuration', OutMode),
)
methods.append(f)

f = Method(void, 'SetMoviePreviewTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'previewTime', InMode),
    (TimeValue, 'previewDuration', InMode),
)
methods.append(f)

f = Method(TimeValue, 'GetMoviePosterTime',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetMoviePosterTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'posterTime', InMode),
)
methods.append(f)

f = Method(void, 'GetMovieSelection',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'selectionTime', OutMode),
    (TimeValue, 'selectionDuration', OutMode),
)
methods.append(f)

f = Method(void, 'SetMovieSelection',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'selectionTime', InMode),
    (TimeValue, 'selectionDuration', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieActiveSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
methods.append(f)

f = Method(void, 'GetMovieActiveSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', OutMode),
    (TimeValue, 'duration', OutMode),
)
methods.append(f)

f = Method(void, 'SetMovieTimeValue',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'newtime', InMode),
)
methods.append(f)

f = Method(long, 'GetMovieTrackCount',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'SetAutoTrackAlternatesEnabled',
    (Movie, 'theMovie', InMode),
    (Boolean, 'enable', InMode),
)
methods.append(f)

f = Method(void, 'SelectMovieAlternates',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(OSErr, 'InsertMovieSegment',
    (Movie, 'srcMovie', InMode),
    (Movie, 'dstMovie', InMode),
    (TimeValue, 'srcIn', InMode),
    (TimeValue, 'srcDuration', InMode),
    (TimeValue, 'dstIn', InMode),
)
methods.append(f)

f = Method(OSErr, 'InsertEmptyMovieSegment',
    (Movie, 'dstMovie', InMode),
    (TimeValue, 'dstIn', InMode),
    (TimeValue, 'dstDuration', InMode),
)
methods.append(f)

f = Method(OSErr, 'DeleteMovieSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
methods.append(f)

f = Method(OSErr, 'ScaleMovieSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'oldDuration', InMode),
    (TimeValue, 'newDuration', InMode),
)
methods.append(f)

f = Method(Movie, 'CutMovieSelection',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(Movie, 'CopyMovieSelection',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'PasteMovieSelection',
    (Movie, 'theMovie', InMode),
    (Movie, 'src', InMode),
)
methods.append(f)

f = Method(void, 'AddMovieSelection',
    (Movie, 'theMovie', InMode),
    (Movie, 'src', InMode),
)
methods.append(f)

f = Method(void, 'ClearMovieSelection',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Function(OSErr, 'PasteHandleIntoMovie',
    (Handle, 'h', InMode),
    (OSType, 'handleType', InMode),
    (Movie, 'theMovie', InMode),
    (long, 'flags', InMode),
    (ComponentInstance, 'userComp', InMode),
)
functions.append(f)

f = Method(OSErr, 'CopyMovieSettings',
    (Movie, 'srcMovie', InMode),
    (Movie, 'dstMovie', InMode),
)
methods.append(f)

f = Method(long, 'GetMovieDataSize',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
methods.append(f)

f = Method(Boolean, 'PtInMovie',
    (Movie, 'theMovie', InMode),
    (Point, 'pt', InMode),
)
methods.append(f)

f = Method(void, 'SetMovieLanguage',
    (Movie, 'theMovie', InMode),
    (long, 'language', InMode),
)
methods.append(f)

f = Method(void, 'GetMovieNextInterestingTime',
    (Movie, 'theMovie', InMode),
    (short, 'interestingTimeFlags', InMode),
    (short, 'numMediaTypes', InMode),
    (OSType_ptr, 'whichMediaTypes', InMode),
    (TimeValue, 'time', InMode),
    (Fixed, 'rate', InMode),
    (TimeValue, 'interestingTime', OutMode),
    (TimeValue, 'interestingDuration', OutMode),
)
methods.append(f)

f = Function(OSErr, 'CreateMovieFile',
    (FSSpec_ptr, 'fileSpec', InMode),
    (OSType, 'creator', InMode),
    (ScriptCode, 'scriptTag', InMode),
    (long, 'createMovieFileFlags', InMode),
    (short, 'resRefNum', OutMode),
    (Movie, 'newmovie', OutMode),
)
functions.append(f)

f = Function(OSErr, 'OpenMovieFile',
    (FSSpec_ptr, 'fileSpec', InMode),
    (short, 'resRefNum', OutMode),
    (SInt8, 'permission', InMode),
)
functions.append(f)

f = Function(OSErr, 'CloseMovieFile',
    (short, 'resRefNum', InMode),
)
functions.append(f)

f = Function(OSErr, 'DeleteMovieFile',
    (FSSpec_ptr, 'fileSpec', InMode),
)
functions.append(f)

f = Function(OSErr, 'NewMovieFromFile',
    (Movie, 'theMovie', OutMode),
    (short, 'resRefNum', InMode),
    (short, 'resId', OutMode),
    (StringPtr, 'resName', InMode),
    (short, 'newMovieFlags', InMode),
    (Boolean, 'dataRefWasChanged', OutMode),
)
functions.append(f)

f = Function(OSErr, 'NewMovieFromHandle',
    (Movie, 'theMovie', OutMode),
    (Handle, 'h', InMode),
    (short, 'newMovieFlags', InMode),
    (Boolean, 'dataRefWasChanged', OutMode),
)
functions.append(f)

f = Function(OSErr, 'NewMovieFromDataFork',
    (Movie, 'theMovie', OutMode),
    (short, 'fRefNum', InMode),
    (long, 'fileOffset', InMode),
    (short, 'newMovieFlags', InMode),
    (Boolean, 'dataRefWasChanged', OutMode),
)
functions.append(f)

f = Function(OSErr, 'NewMovieFromUserProc',
    (Movie, 'm', OutMode),
    (short, 'flags', InMode),
    (Boolean, 'dataRefWasChanged', OutMode),
    (GetMovieUPP, 'getProc', InMode),
    (void, 'refCon', OutMode),
    (Handle, 'defaultDataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
functions.append(f)

f = Method(OSErr, 'AddMovieResource',
    (Movie, 'theMovie', InMode),
    (short, 'resRefNum', InMode),
    (short, 'resId', OutMode),
    (ConstStr255Param, 'resName', InMode),
)
methods.append(f)

f = Method(OSErr, 'UpdateMovieResource',
    (Movie, 'theMovie', InMode),
    (short, 'resRefNum', InMode),
    (short, 'resId', InMode),
    (ConstStr255Param, 'resName', InMode),
)
methods.append(f)

f = Function(OSErr, 'RemoveMovieResource',
    (short, 'resRefNum', InMode),
    (short, 'resId', InMode),
)
functions.append(f)

f = Method(Boolean, 'HasMovieChanged',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(void, 'ClearMovieChanged',
    (Movie, 'theMovie', InMode),
)
methods.append(f)

f = Method(OSErr, 'SetMovieDefaultDataRef',
    (Movie, 'theMovie', InMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
methods.append(f)

f = Method(OSErr, 'GetMovieDefaultDataRef',
    (Movie, 'theMovie', InMode),
    (Handle, 'dataRef', OutMode),
    (OSType, 'dataRefType', OutMode),
)
methods.append(f)

f = Method(OSErr, 'SetMovieColorTable',
    (Movie, 'theMovie', InMode),
    (CTabHandle, 'ctab', InMode),
)
methods.append(f)

f = Method(OSErr, 'GetMovieColorTable',
    (Movie, 'theMovie', InMode),
    (CTabHandle, 'ctab', OutMode),
)
methods.append(f)

f = Method(void, 'FlattenMovie',
    (Movie, 'theMovie', InMode),
    (long, 'movieFlattenFlags', InMode),
    (FSSpec_ptr, 'theFile', InMode),
    (OSType, 'creator', InMode),
    (ScriptCode, 'scriptTag', InMode),
    (long, 'createMovieFileFlags', InMode),
    (short, 'resId', OutMode),
    (ConstStr255Param, 'resName', InMode),
)
methods.append(f)

f = Method(Movie, 'FlattenMovieData',
    (Movie, 'theMovie', InMode),
    (long, 'movieFlattenFlags', InMode),
    (FSSpec_ptr, 'theFile', InMode),
    (OSType, 'creator', InMode),
    (ScriptCode, 'scriptTag', InMode),
    (long, 'createMovieFileFlags', InMode),
)
methods.append(f)

f = Function(HandlerError, 'GetVideoMediaGraphicsMode',
    (MediaHandler, 'mh', InMode),
    (long, 'graphicsMode', OutMode),
    (RGBColor, 'opColor', OutMode),
)
functions.append(f)

f = Function(HandlerError, 'SetVideoMediaGraphicsMode',
    (MediaHandler, 'mh', InMode),
    (long, 'graphicsMode', InMode),
    (RGBColor_ptr, 'opColor', InMode),
)
functions.append(f)

f = Function(HandlerError, 'GetSoundMediaBalance',
    (MediaHandler, 'mh', InMode),
    (short, 'balance', OutMode),
)
functions.append(f)

f = Function(HandlerError, 'SetSoundMediaBalance',
    (MediaHandler, 'mh', InMode),
    (short, 'balance', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'AddTextSample',
    (MediaHandler, 'mh', InMode),
    (Ptr, 'text', InMode),
    (unsigned_long, 'size', InMode),
    (short, 'fontNumber', InMode),
    (short, 'fontSize', InMode),
    (short, 'textFace', InMode),
    (RGBColor, 'textColor', OutMode),
    (RGBColor, 'backColor', OutMode),
    (short, 'textJustification', InMode),
    (Rect, 'textBox', OutMode),
    (long, 'displayFlags', InMode),
    (TimeValue, 'scrollDelay', InMode),
    (short, 'hiliteStart', InMode),
    (short, 'hiliteEnd', InMode),
    (RGBColor, 'rgbHiliteColor', OutMode),
    (TimeValue, 'duration', InMode),
    (TimeValue, 'sampleTime', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'AddTESample',
    (MediaHandler, 'mh', InMode),
    (TEHandle, 'hTE', InMode),
    (RGBColor, 'backColor', OutMode),
    (short, 'textJustification', InMode),
    (Rect, 'textBox', OutMode),
    (long, 'displayFlags', InMode),
    (TimeValue, 'scrollDelay', InMode),
    (short, 'hiliteStart', InMode),
    (short, 'hiliteEnd', InMode),
    (RGBColor, 'rgbHiliteColor', OutMode),
    (TimeValue, 'duration', InMode),
    (TimeValue, 'sampleTime', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'AddHiliteSample',
    (MediaHandler, 'mh', InMode),
    (short, 'hiliteStart', InMode),
    (short, 'hiliteEnd', InMode),
    (RGBColor, 'rgbHiliteColor', OutMode),
    (TimeValue, 'duration', InMode),
    (TimeValue, 'sampleTime', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'FindNextText',
    (MediaHandler, 'mh', InMode),
    (Ptr, 'text', InMode),
    (long, 'size', InMode),
    (short, 'findFlags', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'foundTime', OutMode),
    (TimeValue, 'foundDuration', OutMode),
    (long, 'offset', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'HiliteTextSample',
    (MediaHandler, 'mh', InMode),
    (TimeValue, 'sampleTime', InMode),
    (short, 'hiliteStart', InMode),
    (short, 'hiliteEnd', InMode),
    (RGBColor, 'rgbHiliteColor', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'SetTextSampleData',
    (MediaHandler, 'mh', InMode),
    (void, 'data', OutMode),
    (OSType, 'dataType', InMode),
)
functions.append(f)

f = Method(void, 'GetPosterBox',
    (Movie, 'theMovie', InMode),
    (Rect, 'boxRect', OutMode),
)
methods.append(f)

f = Method(void, 'SetPosterBox',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'boxRect', InMode),
)
methods.append(f)

f = Method(RgnHandle, 'GetMovieSegmentDisplayBoundsRgn',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
)
methods.append(f)

f = Method(ComponentInstance, 'NewMovieController',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'movieRect', InMode),
    (long, 'someFlags', InMode),
)
methods.append(f)

f = Function(void, 'DisposeMovieController',
    (ComponentInstance, 'mc', InMode),
)
functions.append(f)

f = Method(void, 'ShowMovieInformation',
    (Movie, 'theMovie', InMode),
    (ModalFilterUPP, 'filterProc', InMode),
    (long, 'refCon', InMode),
)
methods.append(f)

f = Method(OSErr, 'PutMovieOnScrap',
    (Movie, 'theMovie', InMode),
    (long, 'movieScrapFlags', InMode),
)
methods.append(f)

f = Function(Movie, 'NewMovieFromScrap',
    (long, 'newMovieFlags', InMode),
)
functions.append(f)

f = Method(void, 'SetMoviePlayHints',
    (Movie, 'theMovie', InMode),
    (long, 'flags', InMode),
    (long, 'flagsMask', InMode),
)
methods.append(f)

f = Function(ComponentResult, 'MCSetMovie',
    (MovieController, 'mc', InMode),
    (Movie, 'theMovie', InMode),
    (WindowPtr, 'movieWindow', InMode),
    (Point, 'where', InMode),
)
functions.append(f)

f = Function(Movie, 'MCGetIndMovie',
    (MovieController, 'mc', InMode),
    (short, 'index', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCRemoveMovie',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCIsPlayerEvent',
    (MovieController, 'mc', InMode),
    (EventRecord_ptr, 'e', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCDoAction',
    (MovieController, 'mc', InMode),
    (short, 'action', InMode),
    (void, 'params', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetControllerAttached',
    (MovieController, 'mc', InMode),
    (Boolean, 'attach', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCIsControllerAttached',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetVisible',
    (MovieController, 'mc', InMode),
    (Boolean, 'visible', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCGetVisible',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCGetControllerBoundsRect',
    (MovieController, 'mc', InMode),
    (Rect, 'bounds', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetControllerBoundsRect',
    (MovieController, 'mc', InMode),
    (Rect_ptr, 'bounds', InMode),
)
functions.append(f)

f = Function(RgnHandle, 'MCGetControllerBoundsRgn',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(RgnHandle, 'MCGetWindowRgn',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCMovieChanged',
    (MovieController, 'mc', InMode),
    (Movie, 'm', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetDuration',
    (MovieController, 'mc', InMode),
    (TimeValue, 'duration', InMode),
)
functions.append(f)

f = Function(TimeValue, 'MCGetCurrentTime',
    (MovieController, 'mc', InMode),
    (TimeScale, 'scale', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCNewAttachedController',
    (MovieController, 'mc', InMode),
    (Movie, 'theMovie', InMode),
    (WindowPtr, 'w', InMode),
    (Point, 'where', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCDraw',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCActivate',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
    (Boolean, 'activate', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCIdle',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCKey',
    (MovieController, 'mc', InMode),
    (SInt8, 'key', InMode),
    (long, 'modifiers', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCClick',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
    (Point, 'where', InMode),
    (long, 'when', InMode),
    (long, 'modifiers', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCEnableEditing',
    (MovieController, 'mc', InMode),
    (Boolean, 'enabled', InMode),
)
functions.append(f)

f = Function(long, 'MCIsEditingEnabled',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(Movie, 'MCCopy',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(Movie, 'MCCut',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCPaste',
    (MovieController, 'mc', InMode),
    (Movie, 'srcMovie', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCClear',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCUndo',
    (MovieController, 'mc', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCPositionController',
    (MovieController, 'mc', InMode),
    (Rect_ptr, 'movieRect', InMode),
    (Rect_ptr, 'controllerRect', InMode),
    (long, 'someFlags', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCGetControllerInfo',
    (MovieController, 'mc', InMode),
    (long, 'someFlags', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetClip',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'theClip', InMode),
    (RgnHandle, 'movieClip', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCGetClip',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'theClip', OutMode),
    (RgnHandle, 'movieClip', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCDrawBadge',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'movieRgn', InMode),
    (RgnHandle, 'badgeRgn', OutMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCSetUpEditMenu',
    (MovieController, 'mc', InMode),
    (long, 'modifiers', InMode),
    (MenuHandle, 'mh', InMode),
)
functions.append(f)

f = Function(ComponentResult, 'MCGetMenuString',
    (MovieController, 'mc', InMode),
    (long, 'modifiers', InMode),
    (short, 'item', InMode),
    (Str255, 'aString', InMode),
)
functions.append(f)

f = Function(OSErr, 'QueueSyncTask',
    (QTSyncTaskPtr, 'task', InMode),
)
functions.append(f)

f = Function(OSErr, 'DequeueSyncTask',
    (QTSyncTaskPtr, 'qElem', InMode),
)
functions.append(f)

