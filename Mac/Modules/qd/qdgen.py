# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:QuickDraw.h'

f = Function(void, 'OpenPort',
    (GrafPtr, 'port', InMode),
)
functions.append(f)

f = Function(void, 'InitPort',
    (GrafPtr, 'port', InMode),
)
functions.append(f)

f = Function(void, 'ClosePort',
    (GrafPtr, 'port', InMode),
)
functions.append(f)

f = Function(void, 'SetPort',
    (GrafPtr, 'port', InMode),
)
functions.append(f)

f = Function(void, 'GetPort',
    (GrafPtr, 'port', OutMode),
)
functions.append(f)

f = Function(void, 'GrafDevice',
    (short, 'device', InMode),
)
functions.append(f)

f = Function(void, 'PortSize',
    (short, 'width', InMode),
    (short, 'height', InMode),
)
functions.append(f)

f = Function(void, 'MovePortTo',
    (short, 'leftGlobal', InMode),
    (short, 'topGlobal', InMode),
)
functions.append(f)

f = Function(void, 'SetOrigin',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'SetClip',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'GetClip',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'ClipRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'InitCursor',
)
functions.append(f)

f = Function(void, 'HideCursor',
)
functions.append(f)

f = Function(void, 'ShowCursor',
)
functions.append(f)

f = Function(void, 'ObscureCursor',
)
functions.append(f)

f = Function(void, 'HidePen',
)
functions.append(f)

f = Function(void, 'ShowPen',
)
functions.append(f)

f = Function(void, 'GetPen',
    (Point, 'pt', InOutMode),
)
functions.append(f)

f = Function(void, 'PenSize',
    (short, 'width', InMode),
    (short, 'height', InMode),
)
functions.append(f)

f = Function(void, 'PenMode',
    (short, 'mode', InMode),
)
functions.append(f)

f = Function(void, 'PenNormal',
)
functions.append(f)

f = Function(void, 'MoveTo',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'Move',
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'LineTo',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'Line',
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'TextFont',
    (short, 'font', InMode),
)
functions.append(f)

f = Function(void, 'TextFace',
    (short, 'face', InMode),
)
functions.append(f)

f = Function(void, 'TextMode',
    (short, 'mode', InMode),
)
functions.append(f)

f = Function(void, 'TextSize',
    (short, 'size', InMode),
)
functions.append(f)

f = Function(void, 'SpaceExtra',
    (Fixed, 'extra', InMode),
)
functions.append(f)

f = Function(void, 'DrawChar',
    (short, 'ch', InMode),
)
functions.append(f)

f = Function(void, 'DrawString',
    (ConstStr255Param, 's', InMode),
)
functions.append(f)

f = Function(void, 'DrawText',
    (TextThingie, 'textBuf', InMode),
    (short, 'firstByte', InMode),
    (short, 'byteCount', InMode),
)
functions.append(f)

f = Function(short, 'CharWidth',
    (short, 'ch', InMode),
)
functions.append(f)

f = Function(short, 'StringWidth',
    (ConstStr255Param, 's', InMode),
)
functions.append(f)

f = Function(short, 'TextWidth',
    (TextThingie, 'textBuf', InMode),
    (short, 'firstByte', InMode),
    (short, 'byteCount', InMode),
)
functions.append(f)

f = Function(void, 'ForeColor',
    (long, 'color', InMode),
)
functions.append(f)

f = Function(void, 'BackColor',
    (long, 'color', InMode),
)
functions.append(f)

f = Function(void, 'ColorBit',
    (short, 'whichBit', InMode),
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
    (Rect, 'r', OutMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'InsetRect',
    (Rect, 'r', OutMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(Boolean, 'SectRect',
    (Rect_ptr, 'src1', InMode),
    (Rect_ptr, 'src2', InMode),
    (Rect, 'dstRect', OutMode),
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

f = Function(Boolean, 'EmptyRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'FrameRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'PaintRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'EraseRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'InvertRect',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'FrameOval',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'PaintOval',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'EraseOval',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'InvertOval',
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'FrameRoundRect',
    (Rect_ptr, 'r', InMode),
    (short, 'ovalWidth', InMode),
    (short, 'ovalHeight', InMode),
)
functions.append(f)

f = Function(void, 'PaintRoundRect',
    (Rect_ptr, 'r', InMode),
    (short, 'ovalWidth', InMode),
    (short, 'ovalHeight', InMode),
)
functions.append(f)

f = Function(void, 'EraseRoundRect',
    (Rect_ptr, 'r', InMode),
    (short, 'ovalWidth', InMode),
    (short, 'ovalHeight', InMode),
)
functions.append(f)

f = Function(void, 'InvertRoundRect',
    (Rect_ptr, 'r', InMode),
    (short, 'ovalWidth', InMode),
    (short, 'ovalHeight', InMode),
)
functions.append(f)

f = Function(void, 'FrameArc',
    (Rect_ptr, 'r', InMode),
    (short, 'startAngle', InMode),
    (short, 'arcAngle', InMode),
)
functions.append(f)

f = Function(void, 'PaintArc',
    (Rect_ptr, 'r', InMode),
    (short, 'startAngle', InMode),
    (short, 'arcAngle', InMode),
)
functions.append(f)

f = Function(void, 'EraseArc',
    (Rect_ptr, 'r', InMode),
    (short, 'startAngle', InMode),
    (short, 'arcAngle', InMode),
)
functions.append(f)

f = Function(void, 'InvertArc',
    (Rect_ptr, 'r', InMode),
    (short, 'startAngle', InMode),
    (short, 'arcAngle', InMode),
)
functions.append(f)

f = Function(RgnHandle, 'NewRgn',
)
functions.append(f)

f = Function(void, 'OpenRgn',
)
functions.append(f)

f = Function(void, 'CloseRgn',
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'DisposeRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'CopyRgn',
    (RgnHandle, 'srcRgn', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'SetEmptyRgn',
    (RgnHandle, 'rgn', InMode),
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

f = Function(void, 'RectRgn',
    (RgnHandle, 'rgn', InMode),
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'OffsetRgn',
    (RgnHandle, 'rgn', InMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'InsetRgn',
    (RgnHandle, 'rgn', InMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'SectRgn',
    (RgnHandle, 'srcRgnA', InMode),
    (RgnHandle, 'srcRgnB', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'UnionRgn',
    (RgnHandle, 'srcRgnA', InMode),
    (RgnHandle, 'srcRgnB', InMode),
    (RgnHandle, 'dstRgn', InMode),
)
functions.append(f)

f = Function(void, 'DiffRgn',
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

f = Function(Boolean, 'RectInRgn',
    (Rect_ptr, 'r', InMode),
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(Boolean, 'EqualRgn',
    (RgnHandle, 'rgnA', InMode),
    (RgnHandle, 'rgnB', InMode),
)
functions.append(f)

f = Function(Boolean, 'EmptyRgn',
    (RgnHandle, 'rgn', InMode),
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

f = Function(void, 'EraseRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'InvertRgn',
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(void, 'ScrollRect',
    (Rect_ptr, 'r', InMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
functions.append(f)

f = Function(PicHandle, 'OpenPicture',
    (Rect_ptr, 'picFrame', InMode),
)
functions.append(f)

f = Function(void, 'PicComment',
    (short, 'kind', InMode),
    (short, 'dataSize', InMode),
    (Handle, 'dataHandle', InMode),
)
functions.append(f)

f = Function(void, 'ClosePicture',
)
functions.append(f)

f = Function(void, 'DrawPicture',
    (PicHandle, 'myPicture', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'KillPicture',
    (PicHandle, 'myPicture', InMode),
)
functions.append(f)

f = Function(PolyHandle, 'OpenPoly',
)
functions.append(f)

f = Function(void, 'ClosePoly',
)
functions.append(f)

f = Function(void, 'KillPoly',
    (PolyHandle, 'poly', InMode),
)
functions.append(f)

f = Function(void, 'OffsetPoly',
    (PolyHandle, 'poly', InMode),
    (short, 'dh', InMode),
    (short, 'dv', InMode),
)
functions.append(f)

f = Function(void, 'FramePoly',
    (PolyHandle, 'poly', InMode),
)
functions.append(f)

f = Function(void, 'PaintPoly',
    (PolyHandle, 'poly', InMode),
)
functions.append(f)

f = Function(void, 'ErasePoly',
    (PolyHandle, 'poly', InMode),
)
functions.append(f)

f = Function(void, 'InvertPoly',
    (PolyHandle, 'poly', InMode),
)
functions.append(f)

f = Function(void, 'SetPt',
    (Point, 'pt', InOutMode),
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'LocalToGlobal',
    (Point, 'pt', InOutMode),
)
functions.append(f)

f = Function(void, 'GlobalToLocal',
    (Point, 'pt', InOutMode),
)
functions.append(f)

f = Function(short, 'Random',
)
functions.append(f)

f = Function(Boolean, 'GetPixel',
    (short, 'h', InMode),
    (short, 'v', InMode),
)
functions.append(f)

f = Function(void, 'ScalePt',
    (Point, 'pt', InOutMode),
    (Rect_ptr, 'srcRect', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'MapPt',
    (Point, 'pt', InOutMode),
    (Rect_ptr, 'srcRect', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'MapRect',
    (Rect, 'r', OutMode),
    (Rect_ptr, 'srcRect', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'MapRgn',
    (RgnHandle, 'rgn', InMode),
    (Rect_ptr, 'srcRect', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'MapPoly',
    (PolyHandle, 'poly', InMode),
    (Rect_ptr, 'srcRect', InMode),
    (Rect_ptr, 'dstRect', InMode),
)
functions.append(f)

f = Function(void, 'AddPt',
    (Point, 'src', InMode),
    (Point, 'dst', InOutMode),
)
functions.append(f)

f = Function(Boolean, 'EqualPt',
    (Point, 'pt1', InMode),
    (Point, 'pt2', InMode),
)
functions.append(f)

f = Function(Boolean, 'PtInRect',
    (Point, 'pt', InMode),
    (Rect_ptr, 'r', InMode),
)
functions.append(f)

f = Function(void, 'Pt2Rect',
    (Point, 'pt1', InMode),
    (Point, 'pt2', InMode),
    (Rect, 'dstRect', OutMode),
)
functions.append(f)

f = Function(void, 'PtToAngle',
    (Rect_ptr, 'r', InMode),
    (Point, 'pt', InMode),
    (short, 'angle', OutMode),
)
functions.append(f)

f = Function(Boolean, 'PtInRgn',
    (Point, 'pt', InMode),
    (RgnHandle, 'rgn', InMode),
)
functions.append(f)

f = Function(PixMapHandle, 'NewPixMap',
)
functions.append(f)

f = Function(void, 'DisposPixMap',
    (PixMapHandle, 'pm', InMode),
)
functions.append(f)

f = Function(void, 'DisposePixMap',
    (PixMapHandle, 'pm', InMode),
)
functions.append(f)

f = Function(void, 'CopyPixMap',
    (PixMapHandle, 'srcPM', InMode),
    (PixMapHandle, 'dstPM', InMode),
)
functions.append(f)

f = Function(PixPatHandle, 'NewPixPat',
)
functions.append(f)

f = Function(void, 'DisposPixPat',
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'DisposePixPat',
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'CopyPixPat',
    (PixPatHandle, 'srcPP', InMode),
    (PixPatHandle, 'dstPP', InMode),
)
functions.append(f)

f = Function(void, 'PenPixPat',
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'BackPixPat',
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(PixPatHandle, 'GetPixPat',
    (short, 'patID', InMode),
)
functions.append(f)

f = Function(void, 'FillCRect',
    (Rect_ptr, 'r', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'FillCOval',
    (Rect_ptr, 'r', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'FillCRoundRect',
    (Rect_ptr, 'r', InMode),
    (short, 'ovalWidth', InMode),
    (short, 'ovalHeight', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'FillCArc',
    (Rect_ptr, 'r', InMode),
    (short, 'startAngle', InMode),
    (short, 'arcAngle', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'FillCRgn',
    (RgnHandle, 'rgn', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'FillCPoly',
    (PolyHandle, 'poly', InMode),
    (PixPatHandle, 'pp', InMode),
)
functions.append(f)

f = Function(void, 'SetPortPix',
    (PixMapHandle, 'pm', InMode),
)
functions.append(f)

f = Function(void, 'AllocCursor',
)
functions.append(f)

f = Function(void, 'CharExtra',
    (Fixed, 'extra', InMode),
)
functions.append(f)

f = Function(long, 'GetCTSeed',
)
functions.append(f)

f = Function(void, 'SubPt',
    (Point, 'src', InMode),
    (Point, 'dst', InOutMode),
)
functions.append(f)

f = Function(void, 'SetClientID',
    (short, 'id', InMode),
)
functions.append(f)

f = Function(void, 'ProtectEntry',
    (short, 'index', InMode),
    (Boolean, 'protect', InMode),
)
functions.append(f)

f = Function(void, 'ReserveEntry',
    (short, 'index', InMode),
    (Boolean, 'reserve', InMode),
)
functions.append(f)

f = Function(short, 'QDError',
)
functions.append(f)

