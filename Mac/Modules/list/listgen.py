# Generated from 'Sap:CodeWarrior6:Metrowerks C/C++:Headers:Universal Headers 2.0.1f:Lists.h'

f = Function(ListRef, 'LNew',
    (Rect_ptr, 'rView', InMode),
    (Rect_ptr, 'dataBounds', InMode),
    (Point, 'cSize', InMode),
    (short, 'theProc', InMode),
    (WindowRef, 'theWindow', InMode),
    (Boolean, 'drawIt', InMode),
    (Boolean, 'hasGrow', InMode),
    (Boolean, 'scrollHoriz', InMode),
    (Boolean, 'scrollVert', InMode),
)
functions.append(f)

f = Method(short, 'LAddColumn',
    (short, 'count', InMode),
    (short, 'colNum', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(short, 'LAddRow',
    (short, 'count', InMode),
    (short, 'rowNum', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LDelColumn',
    (short, 'count', InMode),
    (short, 'colNum', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LDelRow',
    (short, 'count', InMode),
    (short, 'rowNum', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(Boolean, 'LGetSelect',
    (Boolean, 'next', InMode),
    (Cell, 'theCell', InOutMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(Cell, 'LLastClick',
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(Boolean, 'LNextCell',
    (Boolean, 'hNext', InMode),
    (Boolean, 'vNext', InMode),
    (Cell, 'theCell', InOutMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LSize',
    (short, 'listWidth', InMode),
    (short, 'listHeight', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LSetDrawingMode',
    (Boolean, 'drawIt', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LScroll',
    (short, 'dCols', InMode),
    (short, 'dRows', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LAutoScroll',
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LUpdate',
    (RgnHandle, 'theRgn', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LActivate',
    (Boolean, 'act', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LCellSize',
    (Point, 'cSize', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(Boolean, 'LClick',
    (Point, 'pt', InMode),
    (short, 'modifiers', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LAddToCell',
    (InBufferShortsize, 'dataPtr', InMode),
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LClrCell',
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LGetCell',
    (VarOutBufferShortsize, 'dataPtr', InOutMode),
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LRect',
    (Rect, 'cellRect', OutMode),
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LSetCell',
    (InBufferShortsize, 'dataPtr', InMode),
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LSetSelect',
    (Boolean, 'setIt', InMode),
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

f = Method(void, 'LDraw',
    (Cell, 'theCell', InMode),
    (ListRef, 'lHandle', InMode),
)
methods.append(f)

