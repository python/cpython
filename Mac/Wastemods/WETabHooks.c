/*
 *	WETabHooks.c
 *
 *	WASTE TABS PACKAGE
 *	Hooks for adding tab support to WASTE
 *
 *	Written by:
 *		Mark Alldritt (original code)
 *		Dan Crevier (line breaks)
 *		John Daub (maintenance)
 *		Jonathan Kew (variable-width tabs)
 *		Marco Piovanelli (?)
 *		Bert Seltzer (horizontal scrolling)
 *
 */

#include "WETabs.h"
#include "WETabHooks.h"

#define FIXROUND(f)			((SInt16) (((f) + 0x00008000) >> 16))
#define BSL(A, B)			(((SInt32) (A)) << (B))

static const Point kOneToOneScaling = { 1, 1 } ;

pascal void _WETabDrawText
	(
		const char * pText,
		SInt32 textLength,
		Fixed slop,
		JustStyleCode styleRunPosition,
		WEReference we
	)
{
#pragma unused ( slop, styleRunPosition )

	LongRect destRect;
	SInt32 beginChar = 0;
	SInt32 ii;
	SInt16 tabWidth;
	SInt16 destLeft;
	Point penPos;
	SInt16 tabSize = WEGetTabSize(we);

	WEGetDestRect(&destRect, we);
	destLeft = (SInt16) destRect.left;

	for ( ii = 0; ii < textLength; ii++ )
	{
		if (pText[ii] == '\t')
		{
			DrawText(pText, beginChar, ii - beginChar);

			/* advance the pen to the next tab stop */
			GetPen(&penPos);
			tabWidth = tabSize - (penPos.h - destLeft) % tabSize;
			MoveTo(penPos.h + tabWidth, penPos.v);
			beginChar = ii + 1;
		}
	}	/* for */

	DrawText(pText, beginChar, textLength - beginChar);
}

pascal SInt32 _WETabPixelToChar
	(
		const char * pText,
		SInt32 textLength,
		Fixed slop,
		Fixed *width,
		WEEdge *edge,
		JustStyleCode styleRunPosition,
		Fixed hPos,
		WEReference we
	)
{
	SInt32 beginChar = 0;
	SInt32 offset = 0;
	SInt32 ii;
	Fixed lastWidth;
	Fixed tabWidth;
	SInt16 tabSize = WEGetTabSize(we);

	/* loop through every character in the segment looking for tabs */
	for ( ii = 0; ii < textLength; ii++ )
	{
		/* exit now if width has gone negative */
		/* (i.e., if we have found which glyph was hit) */
		if (*width <= 0)
		{
			break;
		}

		/* tab found? */
		if (pText[ii] == '\t')
		{
			/* calculate the width of the sub-segment preceding the tab */
			lastWidth = *width;
			offset += PixelToChar((char *)pText + beginChar, ii - beginChar, slop,
					lastWidth, (Boolean *) edge, width, styleRunPosition,
					kOneToOneScaling, kOneToOneScaling);
			beginChar = ii + 1;

			/* hit point past sub-segment? */
			if (*width >= 0)
			{
				/* increment hPos by width of sub-segment preceding the tab */
				hPos += (lastWidth - *width);

				/* calculate the width of the tab "glyph" (as a Fixed value) */
				tabWidth = BSL(tabSize - FIXROUND(hPos) % tabSize, 16);

				/* increment hPos by width of tab character */
				hPos += tabWidth;

				/* hit point within tab glyph? */
				if (*width < tabWidth)
				{
					/* yes: determine which half of tab glyph was hit */
					if (*width > (tabWidth >> 1))
					{
						*edge = kTrailingEdge;	/* second (trailing) edge of tab */
						offset++;
					}
					else
					{
						*edge = kLeadingEdge;	/* first (leading) edge of tab */
					}

					/* returning -1 (as Fixed) in width means we're finished */
					*width = 0xFFFF0000;
				}
				else
				{
					/* hit point is past tab: keep looping */
					offset++;
					*width -= tabWidth;
				}
			} /* if (*width >= 0) */
		} /* if tab found */
	} /* for */

	/* no more tabs in this segment: process the last sub-segment */
	if (*width >= 0)
	{
		lastWidth = *width;
		offset += PixelToChar((char *)pText + beginChar, textLength - beginChar, slop,
					lastWidth, (Boolean *) edge, width, styleRunPosition,
					kOneToOneScaling, kOneToOneScaling);
	}

	/* round width to nearest integer value */
	/* this is supposed to fix an incompatibility with the WorldScript Power Adapter */
	*width = (*width + 0x00008000) & 0xFFFF0000;

	return offset;
}

pascal SInt16 _WETabCharToPixel
	(
		const char * pText,
		SInt32 textLength,
		Fixed slop,
		SInt32 offset,
		SInt16 direction,
		JustStyleCode styleRunPosition,
		SInt16 hPos,
		WEReference we
	)
{
	LongRect destRect;
	SInt32 beginChar = 0;
	SInt32 ii;
	SInt16 width;
	SInt16 destLeft;
	SInt16 totalWidth = 0;
	SInt16 tabSize = WEGetTabSize(we);

	WEGetDestRect(&destRect, we);
	destLeft = (SInt16) destRect.left;

	/* measure text up to offset, if offset is within this segment,
		otherwise to textLength */
	if (offset > textLength)
	{
		offset = textLength;
	}

	for ( ii = 0; ii < offset; ii++ )
	{
		if (pText[ii] == '\t')
		{
			/* calculate the pixel width of the subsegment preceding the tab */
			width = TextWidth(pText, beginChar, ii - beginChar);
			totalWidth += width;
			hPos += width;

			/* calculate tab width */
			width = tabSize - (hPos - destLeft) % tabSize;
			totalWidth += width;
			hPos += width;

			/* go to next subsegment */
			beginChar = ii + 1;
		}
	} /* for */

	/* calculate width of remaining characters */
	width = CharToPixel((char *)pText + beginChar, textLength - beginChar, slop,
						offset - beginChar, direction, styleRunPosition,
						kOneToOneScaling, kOneToOneScaling);
	totalWidth += width;

	return totalWidth;
}

pascal StyledLineBreakCode _WETabLineBreak
	(
		const char * pText,
		SInt32 textLength,
		SInt32 textStart,
		SInt32 textEnd,
		Fixed *textWidth,
		SInt32 *textOffset,
		WEReference we
	)
{
	LongRect destRect;
	SInt32 beginChar = textStart;
	SInt32 ii;
	Fixed tabWidth;
	SInt16 destWidth;
	StyledLineBreakCode breakCode = smBreakOverflow;
	SInt16 tabSize = WEGetTabSize(we);

	WEGetDestRect(&destRect, we);
	destWidth = (SInt16) (destRect.right - destRect.left);

	for ( ii = textStart; ii < textEnd; ii++ )
	{
		if (pText[ii] == 0x0D)
		{
			/* found a <return>, so stop looking ahead for tabs */
			ii++;
			break;
		}
		if (pText[ii] == '\t')
		{
			/* do previous "segment" */
			breakCode = StyledLineBreak((char *)pText, textLength, beginChar, ii, 0, textWidth, textOffset);
			if ((breakCode != smBreakOverflow) || (ii >= textLength))
			{
				break;
			}
			beginChar = ii + 1;

			/* calculate tab width (as a Fixed value) */
			tabWidth = BSL(tabSize - (destWidth - FIXROUND(*textWidth)) % tabSize, 16);

			/* if tabWidth > pixelWidth we break in tab */
			/* don't move tab to next line */
			if (tabWidth > *textWidth)
			{
				breakCode = smBreakWord;
				*textOffset = ii + 1;
				break;
			}
			else
			{
				*textWidth -= tabWidth;
			}
		}
	} /* for */

	/* do last sub-segment */
	if ((ii - beginChar >= 0) && (breakCode == smBreakOverflow))
	{
		breakCode = StyledLineBreak((char *)pText, textLength, beginChar, ii, 0, textWidth, textOffset);
	}

	return breakCode;
}
