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
Movie_methods.append(f)

f = Method(OSErr, 'PrerollMovie',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (Fixed, 'Rate', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'LoadMovieIntoRam',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
    (long, 'flags', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'LoadTrackIntoRam',
    (Track, 'theTrack', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
    (long, 'flags', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'LoadMediaIntoRam',
    (Media, 'theMedia', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
    (long, 'flags', InMode),
)
Media_methods.append(f)

f = Method(void, 'SetMovieActive',
    (Movie, 'theMovie', InMode),
    (Boolean, 'active', InMode),
)
Movie_methods.append(f)

f = Method(Boolean, 'GetMovieActive',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'StartMovie',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'StopMovie',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GoToBeginningOfMovie',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GoToEndOfMovie',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Boolean, 'IsMovieDone',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Boolean, 'GetMoviePreviewMode',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMoviePreviewMode',
    (Movie, 'theMovie', InMode),
    (Boolean, 'usePreview', InMode),
)
Movie_methods.append(f)

f = Method(void, 'ShowMoviePoster',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(TimeBase, 'GetMovieTimeBase',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Track, 'GetNextTrackForCompositing',
    (Movie, 'theMovie', InMode),
    (Track, 'theTrack', InMode),
)
Movie_methods.append(f)

f = Method(Track, 'GetPrevTrackForCompositing',
    (Movie, 'theMovie', InMode),
    (Track, 'theTrack', InMode),
)
Movie_methods.append(f)

f = Method(PicHandle, 'GetMoviePict',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
)
Movie_methods.append(f)

f = Method(PicHandle, 'GetTrackPict',
    (Track, 'theTrack', InMode),
    (TimeValue, 'time', InMode),
)
Track_methods.append(f)

f = Method(PicHandle, 'GetMoviePosterPict',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'UpdateMovie',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GetMovieBox',
    (Movie, 'theMovie', InMode),
    (Rect, 'boxRect', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieBox',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'boxRect', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetMovieDisplayClipRgn',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieDisplayClipRgn',
    (Movie, 'theMovie', InMode),
    (RgnHandle, 'theClip', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetMovieClipRgn',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieClipRgn',
    (Movie, 'theMovie', InMode),
    (RgnHandle, 'theClip', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetTrackClipRgn',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackClipRgn',
    (Track, 'theTrack', InMode),
    (RgnHandle, 'theClip', InMode),
)
Track_methods.append(f)

f = Method(RgnHandle, 'GetMovieDisplayBoundsRgn',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetTrackDisplayBoundsRgn',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(RgnHandle, 'GetMovieBoundsRgn',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetTrackMovieBoundsRgn',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(RgnHandle, 'GetTrackBoundsRgn',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(PixMapHandle, 'GetTrackMatte',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackMatte',
    (Track, 'theTrack', InMode),
    (PixMapHandle, 'theMatte', InMode),
)
Track_methods.append(f)

f = Function(void, 'DisposeMatte',
    (PixMapHandle, 'theMatte', InMode),
)
functions.append(f)

f = Function(Movie, 'NewMovie',
    (long, 'flags', InMode),
)
functions.append(f)

f = Method(OSErr, 'PutMovieIntoHandle',
    (Movie, 'theMovie', InMode),
    (Handle, 'publicMovie', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'PutMovieIntoDataFork',
    (Movie, 'theMovie', InMode),
    (short, 'fRefNum', InMode),
    (long, 'offset', InMode),
    (long, 'maxSize', InMode),
)
Movie_methods.append(f)

f = Method(TimeScale, 'GetMovieTimeScale',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieTimeScale',
    (Movie, 'theMovie', InMode),
    (TimeScale, 'timeScale', InMode),
)
Movie_methods.append(f)

f = Method(TimeValue, 'GetMovieDuration',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Fixed, 'GetMovieRate',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieRate',
    (Movie, 'theMovie', InMode),
    (Fixed, 'rate', InMode),
)
Movie_methods.append(f)

f = Method(Fixed, 'GetMoviePreferredRate',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMoviePreferredRate',
    (Movie, 'theMovie', InMode),
    (Fixed, 'rate', InMode),
)
Movie_methods.append(f)

f = Method(short, 'GetMoviePreferredVolume',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMoviePreferredVolume',
    (Movie, 'theMovie', InMode),
    (short, 'volume', InMode),
)
Movie_methods.append(f)

f = Method(short, 'GetMovieVolume',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieVolume',
    (Movie, 'theMovie', InMode),
    (short, 'volume', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GetMoviePreviewTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'previewTime', OutMode),
    (TimeValue, 'previewDuration', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMoviePreviewTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'previewTime', InMode),
    (TimeValue, 'previewDuration', InMode),
)
Movie_methods.append(f)

f = Method(TimeValue, 'GetMoviePosterTime',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMoviePosterTime',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'posterTime', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GetMovieSelection',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'selectionTime', OutMode),
    (TimeValue, 'selectionDuration', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieSelection',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'selectionTime', InMode),
    (TimeValue, 'selectionDuration', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieActiveSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Movie_methods.append(f)

f = Method(void, 'GetMovieActiveSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', OutMode),
    (TimeValue, 'duration', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMovieTimeValue',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'newtime', InMode),
)
Movie_methods.append(f)

f = Method(UserData, 'GetMovieUserData',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(long, 'GetMovieTrackCount',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Track, 'GetMovieTrack',
    (Movie, 'theMovie', InMode),
    (long, 'trackID', InMode),
)
Movie_methods.append(f)

f = Method(Track, 'GetMovieIndTrack',
    (Movie, 'theMovie', InMode),
    (long, 'index', InMode),
)
Movie_methods.append(f)

f = Method(Track, 'GetMovieIndTrackType',
    (Movie, 'theMovie', InMode),
    (long, 'index', InMode),
    (OSType, 'trackType', InMode),
    (long, 'flags', InMode),
)
Movie_methods.append(f)

f = Method(long, 'GetTrackID',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(Movie, 'GetTrackMovie',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(Track, 'NewMovieTrack',
    (Movie, 'theMovie', InMode),
    (Fixed, 'width', InMode),
    (Fixed, 'height', InMode),
    (short, 'trackVolume', InMode),
)
Movie_methods.append(f)

f = Method(Boolean, 'GetTrackEnabled',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackEnabled',
    (Track, 'theTrack', InMode),
    (Boolean, 'isEnabled', InMode),
)
Track_methods.append(f)

f = Method(long, 'GetTrackUsage',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackUsage',
    (Track, 'theTrack', InMode),
    (long, 'usage', InMode),
)
Track_methods.append(f)

f = Method(TimeValue, 'GetTrackDuration',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(TimeValue, 'GetTrackOffset',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackOffset',
    (Track, 'theTrack', InMode),
    (TimeValue, 'movieOffsetTime', InMode),
)
Track_methods.append(f)

f = Method(short, 'GetTrackLayer',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackLayer',
    (Track, 'theTrack', InMode),
    (short, 'layer', InMode),
)
Track_methods.append(f)

f = Method(Track, 'GetTrackAlternate',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackAlternate',
    (Track, 'theTrack', InMode),
    (Track, 'alternateT', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetAutoTrackAlternatesEnabled',
    (Movie, 'theMovie', InMode),
    (Boolean, 'enable', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SelectMovieAlternates',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(short, 'GetTrackVolume',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackVolume',
    (Track, 'theTrack', InMode),
    (short, 'volume', InMode),
)
Track_methods.append(f)

f = Method(void, 'GetTrackDimensions',
    (Track, 'theTrack', InMode),
    (Fixed, 'width', OutMode),
    (Fixed, 'height', OutMode),
)
Track_methods.append(f)

f = Method(void, 'SetTrackDimensions',
    (Track, 'theTrack', InMode),
    (Fixed, 'width', InMode),
    (Fixed, 'height', InMode),
)
Track_methods.append(f)

f = Method(UserData, 'GetTrackUserData',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(Media, 'NewTrackMedia',
    (Track, 'theTrack', InMode),
    (OSType, 'mediaType', InMode),
    (TimeScale, 'timeScale', InMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
Track_methods.append(f)

f = Method(Media, 'GetTrackMedia',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(Track, 'GetMediaTrack',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(TimeScale, 'GetMediaTimeScale',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(void, 'SetMediaTimeScale',
    (Media, 'theMedia', InMode),
    (TimeScale, 'timeScale', InMode),
)
Media_methods.append(f)

f = Method(TimeValue, 'GetMediaDuration',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(short, 'GetMediaLanguage',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(void, 'SetMediaLanguage',
    (Media, 'theMedia', InMode),
    (short, 'language', InMode),
)
Media_methods.append(f)

f = Method(short, 'GetMediaQuality',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(void, 'SetMediaQuality',
    (Media, 'theMedia', InMode),
    (short, 'quality', InMode),
)
Media_methods.append(f)

f = Method(void, 'GetMediaHandlerDescription',
    (Media, 'theMedia', InMode),
    (OSType, 'mediaType', OutMode),
    (Str255, 'creatorName', InMode),
    (OSType, 'creatorManufacturer', OutMode),
)
Media_methods.append(f)

f = Method(UserData, 'GetMediaUserData',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(MediaHandler, 'GetMediaHandler',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaHandler',
    (Media, 'theMedia', InMode),
    (MediaHandlerComponent, 'mH', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'BeginMediaEdits',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'EndMediaEdits',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaDefaultDataRefIndex',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
)
Media_methods.append(f)

f = Method(void, 'GetMediaDataHandlerDescription',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
    (OSType, 'dhType', OutMode),
    (Str255, 'creatorName', InMode),
    (OSType, 'creatorManufacturer', OutMode),
)
Media_methods.append(f)

f = Method(DataHandler, 'GetMediaDataHandler',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaDataHandler',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
    (DataHandlerComponent, 'dataHandler', InMode),
)
Media_methods.append(f)

f = Function(Component, 'GetDataHandler',
    (Handle, 'dataRef', InMode),
    (OSType, 'dataHandlerSubType', InMode),
    (long, 'flags', InMode),
)
functions.append(f)

f = Method(long, 'GetMediaSampleDescriptionCount',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(void, 'GetMediaSampleDescription',
    (Media, 'theMedia', InMode),
    (long, 'index', InMode),
    (SampleDescriptionHandle, 'descH', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaSampleDescription',
    (Media, 'theMedia', InMode),
    (long, 'index', InMode),
    (SampleDescriptionHandle, 'descH', InMode),
)
Media_methods.append(f)

f = Method(long, 'GetMediaSampleCount',
    (Media, 'theMedia', InMode),
)
Media_methods.append(f)

f = Method(void, 'SampleNumToMediaTime',
    (Media, 'theMedia', InMode),
    (long, 'logicalSampleNum', InMode),
    (TimeValue, 'sampleTime', OutMode),
    (TimeValue, 'sampleDuration', OutMode),
)
Media_methods.append(f)

f = Method(void, 'MediaTimeToSampleNum',
    (Media, 'theMedia', InMode),
    (TimeValue, 'time', InMode),
    (long, 'sampleNum', OutMode),
    (TimeValue, 'sampleTime', OutMode),
    (TimeValue, 'sampleDuration', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'AddMediaSample',
    (Media, 'theMedia', InMode),
    (Handle, 'dataIn', InMode),
    (long, 'inOffset', InMode),
    (unsigned_long, 'size', InMode),
    (TimeValue, 'durationPerSample', InMode),
    (SampleDescriptionHandle, 'sampleDescriptionH', InMode),
    (long, 'numberOfSamples', InMode),
    (short, 'sampleFlags', InMode),
    (TimeValue, 'sampleTime', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'AddMediaSampleReference',
    (Media, 'theMedia', InMode),
    (long, 'dataOffset', InMode),
    (unsigned_long, 'size', InMode),
    (TimeValue, 'durationPerSample', InMode),
    (SampleDescriptionHandle, 'sampleDescriptionH', InMode),
    (long, 'numberOfSamples', InMode),
    (short, 'sampleFlags', InMode),
    (TimeValue, 'sampleTime', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'GetMediaSample',
    (Media, 'theMedia', InMode),
    (Handle, 'dataOut', InMode),
    (long, 'maxSizeToGrow', InMode),
    (long, 'size', OutMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'sampleTime', OutMode),
    (TimeValue, 'durationPerSample', OutMode),
    (SampleDescriptionHandle, 'sampleDescriptionH', InMode),
    (long, 'sampleDescriptionIndex', OutMode),
    (long, 'maxNumberOfSamples', InMode),
    (long, 'numberOfSamples', OutMode),
    (short, 'sampleFlags', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'GetMediaSampleReference',
    (Media, 'theMedia', InMode),
    (long, 'dataOffset', OutMode),
    (long, 'size', OutMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'sampleTime', OutMode),
    (TimeValue, 'durationPerSample', OutMode),
    (SampleDescriptionHandle, 'sampleDescriptionH', InMode),
    (long, 'sampleDescriptionIndex', OutMode),
    (long, 'maxNumberOfSamples', InMode),
    (long, 'numberOfSamples', OutMode),
    (short, 'sampleFlags', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaPreferredChunkSize',
    (Media, 'theMedia', InMode),
    (long, 'maxChunkSize', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'GetMediaPreferredChunkSize',
    (Media, 'theMedia', InMode),
    (long, 'maxChunkSize', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaShadowSync',
    (Media, 'theMedia', InMode),
    (long, 'frameDiffSampleNum', InMode),
    (long, 'syncSampleNum', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'GetMediaShadowSync',
    (Media, 'theMedia', InMode),
    (long, 'frameDiffSampleNum', InMode),
    (long, 'syncSampleNum', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'InsertMediaIntoTrack',
    (Track, 'theTrack', InMode),
    (TimeValue, 'trackStart', InMode),
    (TimeValue, 'mediaTime', InMode),
    (TimeValue, 'mediaDuration', InMode),
    (Fixed, 'mediaRate', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'InsertTrackSegment',
    (Track, 'srcTrack', InMode),
    (Track, 'dstTrack', InMode),
    (TimeValue, 'srcIn', InMode),
    (TimeValue, 'srcDuration', InMode),
    (TimeValue, 'dstIn', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'InsertMovieSegment',
    (Movie, 'srcMovie', InMode),
    (Movie, 'dstMovie', InMode),
    (TimeValue, 'srcIn', InMode),
    (TimeValue, 'srcDuration', InMode),
    (TimeValue, 'dstIn', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'InsertEmptyTrackSegment',
    (Track, 'dstTrack', InMode),
    (TimeValue, 'dstIn', InMode),
    (TimeValue, 'dstDuration', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'InsertEmptyMovieSegment',
    (Movie, 'dstMovie', InMode),
    (TimeValue, 'dstIn', InMode),
    (TimeValue, 'dstDuration', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'DeleteTrackSegment',
    (Track, 'theTrack', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'DeleteMovieSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'ScaleTrackSegment',
    (Track, 'theTrack', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'oldDuration', InMode),
    (TimeValue, 'newDuration', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'ScaleMovieSegment',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'oldDuration', InMode),
    (TimeValue, 'newDuration', InMode),
)
Movie_methods.append(f)

f = Method(Movie, 'CutMovieSelection',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(Movie, 'CopyMovieSelection',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'PasteMovieSelection',
    (Movie, 'theMovie', InMode),
    (Movie, 'src', InMode),
)
Movie_methods.append(f)

f = Method(void, 'AddMovieSelection',
    (Movie, 'theMovie', InMode),
    (Movie, 'src', InMode),
)
Movie_methods.append(f)

f = Method(void, 'ClearMovieSelection',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Function(OSErr, 'PasteHandleIntoMovie',
    (Handle, 'h', InMode),
    (OSType, 'handleType', InMode),
    (Movie, 'theMovie', InMode),
    (long, 'flags', InMode),
    (ComponentInstance, 'userComp', InMode),
)
functions.append(f)

f = Method(OSErr, 'PutMovieIntoTypedHandle',
    (Movie, 'theMovie', InMode),
    (Track, 'targetTrack', InMode),
    (OSType, 'handleType', InMode),
    (Handle, 'publicMovie', InMode),
    (TimeValue, 'start', InMode),
    (TimeValue, 'dur', InMode),
    (long, 'flags', InMode),
    (ComponentInstance, 'userComp', InMode),
)
Movie_methods.append(f)

f = Method(Component, 'IsScrapMovie',
    (Track, 'targetTrack', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'CopyTrackSettings',
    (Track, 'srcTrack', InMode),
    (Track, 'dstTrack', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'CopyMovieSettings',
    (Movie, 'srcMovie', InMode),
    (Movie, 'dstMovie', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'AddEmptyTrackToMovie',
    (Track, 'srcTrack', InMode),
    (Movie, 'dstMovie', InMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
    (Track, 'dstTrack', OutMode),
)
Track_methods.append(f)

f = Method(OSErr, 'AddTrackReference',
    (Track, 'theTrack', InMode),
    (Track, 'refTrack', InMode),
    (OSType, 'refType', InMode),
    (long, 'addedIndex', OutMode),
)
Track_methods.append(f)

f = Method(OSErr, 'DeleteTrackReference',
    (Track, 'theTrack', InMode),
    (OSType, 'refType', InMode),
    (long, 'index', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'SetTrackReference',
    (Track, 'theTrack', InMode),
    (Track, 'refTrack', InMode),
    (OSType, 'refType', InMode),
    (long, 'index', InMode),
)
Track_methods.append(f)

f = Method(Track, 'GetTrackReference',
    (Track, 'theTrack', InMode),
    (OSType, 'refType', InMode),
    (long, 'index', InMode),
)
Track_methods.append(f)

f = Method(OSType, 'GetNextTrackReferenceType',
    (Track, 'theTrack', InMode),
    (OSType, 'refType', InMode),
)
Track_methods.append(f)

f = Method(long, 'GetTrackReferenceCount',
    (Track, 'theTrack', InMode),
    (OSType, 'refType', InMode),
)
Track_methods.append(f)

f = Method(OSErr, 'ConvertMovieToFile',
    (Movie, 'theMovie', InMode),
    (Track, 'onlyTrack', InMode),
    (FSSpec_ptr, 'outputFile', InMode),
    (OSType, 'fileType', InMode),
    (OSType, 'creator', InMode),
    (ScriptCode, 'scriptTag', InMode),
    (short, 'resID', OutMode),
    (long, 'flags', InMode),
    (ComponentInstance, 'userComp', InMode),
)
Movie_methods.append(f)

f = Function(TimeValue, 'TrackTimeToMediaTime',
    (TimeValue, 'value', InMode),
    (Track, 'theTrack', InMode),
)
functions.append(f)

f = Method(Fixed, 'GetTrackEditRate',
    (Track, 'theTrack', InMode),
    (TimeValue, 'atTime', InMode),
)
Track_methods.append(f)

f = Method(long, 'GetMovieDataSize',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Movie_methods.append(f)

f = Method(long, 'GetTrackDataSize',
    (Track, 'theTrack', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Track_methods.append(f)

f = Method(long, 'GetMediaDataSize',
    (Media, 'theMedia', InMode),
    (TimeValue, 'startTime', InMode),
    (TimeValue, 'duration', InMode),
)
Media_methods.append(f)

f = Method(Boolean, 'PtInMovie',
    (Movie, 'theMovie', InMode),
    (Point, 'pt', InMode),
)
Movie_methods.append(f)

f = Method(Boolean, 'PtInTrack',
    (Track, 'theTrack', InMode),
    (Point, 'pt', InMode),
)
Track_methods.append(f)

f = Method(void, 'SetMovieLanguage',
    (Movie, 'theMovie', InMode),
    (long, 'language', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'GetUserData',
    (UserData, 'theUserData', InMode),
    (Handle, 'data', InMode),
    (OSType, 'udType', InMode),
    (long, 'index', InMode),
)
UserData_methods.append(f)

f = Method(OSErr, 'AddUserData',
    (UserData, 'theUserData', InMode),
    (Handle, 'data', InMode),
    (OSType, 'udType', InMode),
)
UserData_methods.append(f)

f = Method(OSErr, 'RemoveUserData',
    (UserData, 'theUserData', InMode),
    (OSType, 'udType', InMode),
    (long, 'index', InMode),
)
UserData_methods.append(f)

f = Method(short, 'CountUserDataType',
    (UserData, 'theUserData', InMode),
    (OSType, 'udType', InMode),
)
UserData_methods.append(f)

f = Method(long, 'GetNextUserDataType',
    (UserData, 'theUserData', InMode),
    (OSType, 'udType', InMode),
)
UserData_methods.append(f)

f = Method(OSErr, 'AddUserDataText',
    (UserData, 'theUserData', InMode),
    (Handle, 'data', InMode),
    (OSType, 'udType', InMode),
    (long, 'index', InMode),
    (short, 'itlRegionTag', InMode),
)
UserData_methods.append(f)

f = Method(OSErr, 'GetUserDataText',
    (UserData, 'theUserData', InMode),
    (Handle, 'data', InMode),
    (OSType, 'udType', InMode),
    (long, 'index', InMode),
    (short, 'itlRegionTag', InMode),
)
UserData_methods.append(f)

f = Method(OSErr, 'RemoveUserDataText',
    (UserData, 'theUserData', InMode),
    (OSType, 'udType', InMode),
    (long, 'index', InMode),
    (short, 'itlRegionTag', InMode),
)
UserData_methods.append(f)

f = Function(OSErr, 'NewUserData',
    (UserData, 'theUserData', OutMode),
)
functions.append(f)

f = Function(OSErr, 'NewUserDataFromHandle',
    (Handle, 'h', InMode),
    (UserData, 'theUserData', OutMode),
)
functions.append(f)

f = Method(OSErr, 'PutUserDataIntoHandle',
    (UserData, 'theUserData', InMode),
    (Handle, 'h', InMode),
)
UserData_methods.append(f)

f = Method(void, 'GetMediaNextInterestingTime',
    (Media, 'theMedia', InMode),
    (short, 'interestingTimeFlags', InMode),
    (TimeValue, 'time', InMode),
    (Fixed, 'rate', InMode),
    (TimeValue, 'interestingTime', OutMode),
    (TimeValue, 'interestingDuration', OutMode),
)
Media_methods.append(f)

f = Method(void, 'GetTrackNextInterestingTime',
    (Track, 'theTrack', InMode),
    (short, 'interestingTimeFlags', InMode),
    (TimeValue, 'time', InMode),
    (Fixed, 'rate', InMode),
    (TimeValue, 'interestingTime', OutMode),
    (TimeValue, 'interestingDuration', OutMode),
)
Track_methods.append(f)

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
Movie_methods.append(f)

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
    (dummyshortptr, 'resId', InMode),
    (dummyStringPtr, 'resName', InMode),
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

f = Method(OSErr, 'AddMovieResource',
    (Movie, 'theMovie', InMode),
    (short, 'resRefNum', InMode),
    (short, 'resId', OutMode),
    (ConstStr255Param, 'resName', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'UpdateMovieResource',
    (Movie, 'theMovie', InMode),
    (short, 'resRefNum', InMode),
    (short, 'resId', InMode),
    (ConstStr255Param, 'resName', InMode),
)
Movie_methods.append(f)

f = Function(OSErr, 'RemoveMovieResource',
    (short, 'resRefNum', InMode),
    (short, 'resId', InMode),
)
functions.append(f)

f = Method(Boolean, 'HasMovieChanged',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(void, 'ClearMovieChanged',
    (Movie, 'theMovie', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'SetMovieDefaultDataRef',
    (Movie, 'theMovie', InMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'GetMovieDefaultDataRef',
    (Movie, 'theMovie', InMode),
    (Handle, 'dataRef', OutMode),
    (OSType, 'dataRefType', OutMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'SetMovieColorTable',
    (Movie, 'theMovie', InMode),
    (CTabHandle, 'ctab', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'GetMovieColorTable',
    (Movie, 'theMovie', InMode),
    (CTabHandle, 'ctab', OutMode),
)
Movie_methods.append(f)

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
Movie_methods.append(f)

f = Method(Movie, 'FlattenMovieData',
    (Movie, 'theMovie', InMode),
    (long, 'movieFlattenFlags', InMode),
    (FSSpec_ptr, 'theFile', InMode),
    (OSType, 'creator', InMode),
    (ScriptCode, 'scriptTag', InMode),
    (long, 'createMovieFileFlags', InMode),
)
Movie_methods.append(f)

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

f = Method(OSErr, 'MovieSearchText',
    (Movie, 'theMovie', InMode),
    (Ptr, 'text', InMode),
    (long, 'size', InMode),
    (long, 'searchFlags', InMode),
    (Track, 'searchTrack', OutMode),
    (TimeValue, 'searchTime', OutMode),
    (long, 'searchOffset', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'GetPosterBox',
    (Movie, 'theMovie', InMode),
    (Rect, 'boxRect', OutMode),
)
Movie_methods.append(f)

f = Method(void, 'SetPosterBox',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'boxRect', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetMovieSegmentDisplayBoundsRgn',
    (Movie, 'theMovie', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
)
Movie_methods.append(f)

f = Method(RgnHandle, 'GetTrackSegmentDisplayBoundsRgn',
    (Track, 'theTrack', InMode),
    (TimeValue, 'time', InMode),
    (TimeValue, 'duration', InMode),
)
Track_methods.append(f)

f = Method(ComponentResult, 'GetTrackStatus',
    (Track, 'theTrack', InMode),
)
Track_methods.append(f)

f = Method(ComponentResult, 'GetMovieStatus',
    (Movie, 'theMovie', InMode),
    (Track, 'firstProblemTrack', OutMode),
)
Movie_methods.append(f)

f = Method(MovieController, 'NewMovieController',
    (Movie, 'theMovie', InMode),
    (Rect_ptr, 'movieRect', InMode),
    (long, 'someFlags', InMode),
)
Movie_methods.append(f)

f = Method(OSErr, 'PutMovieOnScrap',
    (Movie, 'theMovie', InMode),
    (long, 'movieScrapFlags', InMode),
)
Movie_methods.append(f)

f = Function(Movie, 'NewMovieFromScrap',
    (long, 'newMovieFlags', InMode),
)
functions.append(f)

f = Method(OSErr, 'GetMediaDataRef',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
    (Handle, 'dataRef', OutMode),
    (OSType, 'dataRefType', OutMode),
    (long, 'dataRefAttributes', OutMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaDataRef',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'SetMediaDataRefAttributes',
    (Media, 'theMedia', InMode),
    (short, 'index', InMode),
    (long, 'dataRefAttributes', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'AddMediaDataRef',
    (Media, 'theMedia', InMode),
    (short, 'index', OutMode),
    (Handle, 'dataRef', InMode),
    (OSType, 'dataRefType', InMode),
)
Media_methods.append(f)

f = Method(OSErr, 'GetMediaDataRefCount',
    (Media, 'theMedia', InMode),
    (short, 'count', OutMode),
)
Media_methods.append(f)

f = Method(void, 'SetMoviePlayHints',
    (Movie, 'theMovie', InMode),
    (long, 'flags', InMode),
    (long, 'flagsMask', InMode),
)
Movie_methods.append(f)

f = Method(void, 'SetMediaPlayHints',
    (Media, 'theMedia', InMode),
    (long, 'flags', InMode),
    (long, 'flagsMask', InMode),
)
Media_methods.append(f)

f = Method(void, 'SetTrackLoadSettings',
    (Track, 'theTrack', InMode),
    (TimeValue, 'preloadTime', InMode),
    (TimeValue, 'preloadDuration', InMode),
    (long, 'preloadFlags', InMode),
    (long, 'defaultHints', InMode),
)
Track_methods.append(f)

f = Method(void, 'GetTrackLoadSettings',
    (Track, 'theTrack', InMode),
    (TimeValue, 'preloadTime', OutMode),
    (TimeValue, 'preloadDuration', OutMode),
    (long, 'preloadFlags', OutMode),
    (long, 'defaultHints', OutMode),
)
Track_methods.append(f)

f = Method(ComponentResult, 'MCSetMovie',
    (MovieController, 'mc', InMode),
    (Movie, 'theMovie', InMode),
    (WindowPtr, 'movieWindow', InMode),
    (Point, 'where', InMode),
)
MovieController_methods.append(f)

f = Method(Movie, 'MCGetIndMovie',
    (MovieController, 'mc', InMode),
    (short, 'index', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCRemoveMovie',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCIsPlayerEvent',
    (MovieController, 'mc', InMode),
    (EventRecord_ptr, 'e', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCDoAction',
    (MovieController, 'mc', InMode),
    (short, 'action', InMode),
    (mcactionparams, 'params', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetControllerAttached',
    (MovieController, 'mc', InMode),
    (Boolean, 'attach', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCIsControllerAttached',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetVisible',
    (MovieController, 'mc', InMode),
    (Boolean, 'visible', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCGetVisible',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCGetControllerBoundsRect',
    (MovieController, 'mc', InMode),
    (Rect, 'bounds', OutMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetControllerBoundsRect',
    (MovieController, 'mc', InMode),
    (Rect_ptr, 'bounds', InMode),
)
MovieController_methods.append(f)

f = Method(RgnHandle, 'MCGetControllerBoundsRgn',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(RgnHandle, 'MCGetWindowRgn',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCMovieChanged',
    (MovieController, 'mc', InMode),
    (Movie, 'm', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetDuration',
    (MovieController, 'mc', InMode),
    (TimeValue, 'duration', InMode),
)
MovieController_methods.append(f)

f = Method(TimeValue, 'MCGetCurrentTime',
    (MovieController, 'mc', InMode),
    (TimeScale, 'scale', OutMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCNewAttachedController',
    (MovieController, 'mc', InMode),
    (Movie, 'theMovie', InMode),
    (WindowPtr, 'w', InMode),
    (Point, 'where', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCDraw',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCActivate',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
    (Boolean, 'activate', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCIdle',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCKey',
    (MovieController, 'mc', InMode),
    (SInt8, 'key', InMode),
    (long, 'modifiers', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCClick',
    (MovieController, 'mc', InMode),
    (WindowPtr, 'w', InMode),
    (Point, 'where', InMode),
    (long, 'when', InMode),
    (long, 'modifiers', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCEnableEditing',
    (MovieController, 'mc', InMode),
    (Boolean, 'enabled', InMode),
)
MovieController_methods.append(f)

f = Method(long, 'MCIsEditingEnabled',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(Movie, 'MCCopy',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(Movie, 'MCCut',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCPaste',
    (MovieController, 'mc', InMode),
    (Movie, 'srcMovie', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCClear',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCUndo',
    (MovieController, 'mc', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCPositionController',
    (MovieController, 'mc', InMode),
    (Rect_ptr, 'movieRect', InMode),
    (Rect_ptr, 'controllerRect', InMode),
    (long, 'someFlags', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCGetControllerInfo',
    (MovieController, 'mc', InMode),
    (long, 'someFlags', OutMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetClip',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'theClip', InMode),
    (RgnHandle, 'movieClip', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCGetClip',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'theClip', OutMode),
    (RgnHandle, 'movieClip', OutMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCDrawBadge',
    (MovieController, 'mc', InMode),
    (RgnHandle, 'movieRgn', InMode),
    (RgnHandle, 'badgeRgn', OutMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCSetUpEditMenu',
    (MovieController, 'mc', InMode),
    (long, 'modifiers', InMode),
    (MenuHandle, 'mh', InMode),
)
MovieController_methods.append(f)

f = Method(ComponentResult, 'MCGetMenuString',
    (MovieController, 'mc', InMode),
    (long, 'modifiers', InMode),
    (short, 'item', InMode),
    (Str255, 'aString', InMode),
)
MovieController_methods.append(f)

f = Function(TimeBase, 'NewTimeBase',
)
functions.append(f)

f = Method(void, 'SetTimeBaseValue',
    (TimeBase, 'tb', InMode),
    (TimeValue, 't', InMode),
    (TimeScale, 's', InMode),
)
TimeBase_methods.append(f)

f = Method(Fixed, 'GetTimeBaseRate',
    (TimeBase, 'tb', InMode),
)
TimeBase_methods.append(f)

f = Method(void, 'SetTimeBaseRate',
    (TimeBase, 'tb', InMode),
    (Fixed, 'r', InMode),
)
TimeBase_methods.append(f)

f = Method(long, 'GetTimeBaseFlags',
    (TimeBase, 'tb', InMode),
)
TimeBase_methods.append(f)

f = Method(void, 'SetTimeBaseFlags',
    (TimeBase, 'tb', InMode),
    (long, 'timeBaseFlags', InMode),
)
TimeBase_methods.append(f)

f = Method(TimeBase, 'GetTimeBaseMasterTimeBase',
    (TimeBase, 'tb', InMode),
)
TimeBase_methods.append(f)

f = Method(ComponentInstance, 'GetTimeBaseMasterClock',
    (TimeBase, 'tb', InMode),
)
TimeBase_methods.append(f)

f = Method(Fixed, 'GetTimeBaseEffectiveRate',
    (TimeBase, 'tb', InMode),
)
TimeBase_methods.append(f)

