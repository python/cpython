f = Function(void, 'SetPort',
        (GrafPtr, 'thePort', InMode),
)
functions.append(f)

f = Function(CursHandle, 'GetCursor',
    (short, 'cursorID', InMode),
)
functions.append(f)

f = Function(void, 'SetCursor',
    (Cursor_ptr, 'crsr', InMode),
)
functions.append(f)

f = Function(void, 'ShowCursor',
)
functions.append(f)

f = Function(void, 'LineTo',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'SetRect',
    (Rect, 'r', OutMode),
    (short, 'left', InMode),
    (short, 'top', InMode),
    (short, 'right', InMode),
    (short, 'bottom', InMode),
)
functions.append(f)

f = Function(void, 'OffsetRect',
    (Rect, 'r', InOutMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'InsetRect',
    (Rect, 'r', InOutMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'UnionRect',
    (Rect_ptr, 'src1', InMode),
    (Rect_ptr, 'src2', InMode),
    (Rect, 'dstRect', OutMode),
)
functions.append(f)

f = Function(Boolean, 'EqualRect',
    (Rect_ptr, 'rect1', InMode),
    (Rect_ptr, 'rect2', InMode),
)
functions.append(f)

f = Function(void, 'FrameRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'InvertRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'FillRect',
    (Rect_ptr, 'r', InMode),
    (Pattern_ptr, 'pat', InMode),
)
functions.append(f)

f = Function(void, 'CopyRgn',
    (RgnHandle, 'srcRgn', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'SetRectRgn',
    (RgnHandle, 'rgn', InMode),
    (short, 'left', InMode),
    (short, 'top', InMode),
    (short, 'right', InMode),
    (short, 'bottom', InMode),
)
functions.append(f)

f = Function(void, 'OffsetRgn',
    (RgnHandle, 'rgn', InMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'UnionRgn',
    (RgnHandle, 'srcRgnA', InMode),
    (RgnHandle, 'srcRgnB', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'XorRgn',
    (RgnHandle, 'srcRgnA', InMode),
    (RgnHandle, 'srcRgnB', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(Boolean, 'EqualRgn',
    (RgnHandle, 'rgnA', InMode),
    (RgnHandle, 'rgnB', InMode),
)
functions.append(f)

f = Function(void, 'FrameRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'PaintRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'InvertRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'FillRgn',
    (RgnHandle, 'rgn', InMode),
    (Pattern_ptr, 'pat', InMode),
)
functions.append(f)

f = Function(Boolean, 'GetPixel',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(Boolean, 'PtInRect',
    (Point, 'pt', InMode),
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'DrawText',
    (TextThingie, 'textBuf', InMode),
    (short, 'firstByte', InMode),
    (short, 'byteCount', InMode),
)
functions.append(f)
