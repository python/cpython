# Generated from 'Sap:CW8 Gold:Metrowerks CodeWarrior:MacOS Support:Headers:Universal Headers:Fonts.h'

f = Function(void, 'InitFonts',
)
functions.append(f)

f = Function(void, 'GetFontName',
    (short, 'familyID', InMode),
    (Str255, 'name', OutMode),
)
functions.append(f)

f = Function(void, 'GetFNum',
    (ConstStr255Param, 'name', InMode),
    (short, 'familyID', OutMode),
)
functions.append(f)

f = Function(Boolean, 'RealFont',
    (short, 'fontNum', InMode),
    (short, 'size', InMode),
)
functions.append(f)

f = Function(void, 'SetFontLock',
    (Boolean, 'lockFlag', InMode),
)
functions.append(f)

f = Function(void, 'SetFScaleDisable',
    (Boolean, 'fscaleDisable', InMode),
)
functions.append(f)

f = Function(void, 'FontMetrics',
    (FMetricRecPtr, 'theMetrics', OutMode),
)
functions.append(f)

f = Function(void, 'SetFractEnable',
    (Boolean, 'fractEnable', InMode),
)
functions.append(f)

f = Function(short, 'GetDefFontSize',
)
functions.append(f)

f = Function(Boolean, 'IsOutline',
    (Point, 'numer', InMode),
    (Point, 'denom', InMode),
)
functions.append(f)

f = Function(void, 'SetOutlinePreferred',
    (Boolean, 'outlinePreferred', InMode),
)
functions.append(f)

f = Function(Boolean, 'GetOutlinePreferred',
)
functions.append(f)

f = Function(void, 'SetPreserveGlyph',
    (Boolean, 'preserveGlyph', InMode),
)
functions.append(f)

f = Function(Boolean, 'GetPreserveGlyph',
)
functions.append(f)

f = Function(OSErr, 'FlushFonts',
)
functions.append(f)

f = Function(short, 'GetSysFont',
)
functions.append(f)

f = Function(short, 'GetAppFont',
)
functions.append(f)

