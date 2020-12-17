
/*	$Id: tixGrFmt.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $	*/

/* 
 * tixGrFmt.c --
 *
 *	This module handles the formatting of the elements inside a Grid
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <tixGrid.h>

typedef struct FormatStruct {
    int x1, y1, x2, y2;
} FormatStruct;

typedef struct BorderFmtStruct {
    int x1, y1, x2, y2;
    Tk_3DBorder border;
    Tk_3DBorder selectBorder;	/* the border color color */
    int borderWidth;		/* Width of 3-D borders. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int xon, xoff;
    int yon, yoff;
    int filled;
} BorderFmtStruct;

typedef struct GridFmtStruct {
    int x1, y1, x2, y2;
    Tk_3DBorder border;		/* the border color color */
    Tk_3DBorder selectBorder;	/* the border color color */
    Tk_3DBorder bgBorder;	/* the background color */
    int borderWidth;		/* Width of 3-D borders. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int xon, xoff;
    int yon, yoff;
    Tk_Anchor anchor;
    int filled;
} GridFmtStruct;

static TIX_DECLARE_SUBCMD(Tix_GrFormatBorder);
static TIX_DECLARE_SUBCMD(Tix_GrFormatGrid);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrFormat);

typedef int CFG_TYPE;

static int		Tix_GrSaveColor _ANSI_ARGS_((WidgetPtr wPtr,
			    CFG_TYPE type, void * ptr));
static void	    	GetBlockPosn _ANSI_ARGS_((WidgetPtr wPtr, int x1,
			    int y1, int x2, int y2, int * bx1, int * by1,
			    int * bx2, int * by2));
static void 		GetRenderPosn _ANSI_ARGS_((WidgetPtr wPtr,
			    int bx1, int by1, int bx2, int by2, int * rx1,
			    int * ry1, int * rx2, int * ry2));
static void		Tix_GrFillCells _ANSI_ARGS_((WidgetPtr wPtr,
			    Tk_3DBorder border, Tk_3DBorder selectBorder,
			    int bx1, int by1, int bx2, int by2,
			    int borderWidth, int relief, int filled,
			    int bw[2][2]));
static int		GetInfo _ANSI_ARGS_((WidgetPtr wPtr,
			    Tcl_Interp *interp, int argc, CONST84 char **argv,
			    FormatStruct * infoPtr,
			    Tk_ConfigSpec * configSpecs));

#define DEF_GRID_ANCHOR			"se"
#define DEF_GRID_BORDER_XOFF		"0"
#define DEF_GRID_BORDER_XON		"1"
#define DEF_GRID_BORDER_YOFF		"0"
#define DEF_GRID_BORDER_YON		"1"
#define DEF_GRID_GRIDLINE_XOFF		"0"
#define DEF_GRID_GRIDLINE_XON		"1"
#define DEF_GRID_GRIDLINE_YOFF		"0"
#define DEF_GRID_GRIDLINE_YON		"1"
#define DEF_GRID_FILLED			"0"
#define DEF_GRID_BORDER_COLOR		NORMAL_BG
#define DEF_GRID_BORDER_MONO		WHITE
#define DEF_GRID_GRIDLINE_COLOR		BLACK 
#define DEF_GRID_GRIDLINE_MONO		BLACK

static Tk_ConfigSpec borderConfigSpecs[] = {
    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_GRID_BG_COLOR, Tk_Offset(BorderFmtStruct, border),
       TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_GRID_BG_MONO, Tk_Offset(BorderFmtStruct, border),
       TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_GRID_BORDER_WIDTH, Tk_Offset(BorderFmtStruct, borderWidth), 0},
    {TK_CONFIG_BOOLEAN, "-filled", "filled", "Filled",
       DEF_GRID_FILLED, Tk_Offset(BorderFmtStruct, filled), 0},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_GRID_RELIEF, Tk_Offset(BorderFmtStruct, relief), 0},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
       DEF_GRID_SELECT_BG_COLOR, Tk_Offset(BorderFmtStruct, selectBorder),
       TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
       DEF_GRID_SELECT_BG_MONO, Tk_Offset(BorderFmtStruct, selectBorder),
       TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_INT, "-xoff", "xoff", "Xoff",
       DEF_GRID_BORDER_XOFF, Tk_Offset(BorderFmtStruct, xoff), 0},
    {TK_CONFIG_INT, "-xon", "xon", "Xon",
       DEF_GRID_BORDER_XON, Tk_Offset(BorderFmtStruct, xon), 0},
    {TK_CONFIG_INT, "-yoff", "yoff", "Yoff",
       DEF_GRID_BORDER_YOFF, Tk_Offset(BorderFmtStruct, yoff), 0},
    {TK_CONFIG_INT, "-yon", "yon", "Yon",
       DEF_GRID_BORDER_YON, Tk_Offset(BorderFmtStruct, yon), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};	

static Tk_ConfigSpec gridConfigSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
       DEF_GRID_ANCHOR, Tk_Offset(GridFmtStruct, anchor), 0},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_GRID_BG_COLOR, Tk_Offset(GridFmtStruct, bgBorder),
       TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_GRID_BG_COLOR, Tk_Offset(GridFmtStruct, bgBorder),
       TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_BORDER, "-bordercolor", "borderColor", "BorderColor",
       DEF_GRID_GRIDLINE_COLOR, Tk_Offset(GridFmtStruct, border),
       TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-bordercolor", "borderColor", "BorderColor",
       DEF_GRID_GRIDLINE_MONO, Tk_Offset(GridFmtStruct, border),
       TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_GRID_BORDER_WIDTH, Tk_Offset(GridFmtStruct, borderWidth), 0},
    {TK_CONFIG_BOOLEAN, "-filled", "filled", "Filled",
       DEF_GRID_FILLED, Tk_Offset(GridFmtStruct, filled), 0},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_GRID_RELIEF, Tk_Offset(GridFmtStruct, relief), 0},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
       DEF_GRID_SELECT_BG_COLOR, Tk_Offset(GridFmtStruct, selectBorder),
       TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
       DEF_GRID_SELECT_BG_MONO, Tk_Offset(GridFmtStruct, selectBorder),
       TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_INT, "-xoff", "xoff", "Xoff",
       DEF_GRID_GRIDLINE_XOFF, Tk_Offset(GridFmtStruct, xoff), 0},
    {TK_CONFIG_INT, "-xon", "xon", "Xon",
       DEF_GRID_GRIDLINE_XON, Tk_Offset(GridFmtStruct, xon), 0},
    {TK_CONFIG_INT, "-yoff", "yoff", "Yoff",
       DEF_GRID_GRIDLINE_YOFF, Tk_Offset(GridFmtStruct, yoff), 0},
    {TK_CONFIG_INT, "-yon", "yon", "Yon",
       DEF_GRID_GRIDLINE_YON, Tk_Offset(GridFmtStruct, yon), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};	

int
Tix_GrFormat(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "border", 4, TIX_VAR_ARGS, Tix_GrFormatBorder,
	   "x1 y1 x2 y2 ?option value ...?"},
	{TIX_DEFAULT_LEN, "grid",    4, TIX_VAR_ARGS, Tix_GrFormatGrid,
	   "x1 y1 x2 y2 ?option value ...?"},
    };
    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "?option? ?arg ...?",
    };
    WidgetPtr wPtr = (WidgetPtr) clientData;

    if (wPtr->renderInfo == NULL) {
	Tcl_AppendResult(interp, "the \"format\" command can only be called ",
	    "by the -formatcmd handler of the tixGrid widget", NULL);
	return TCL_ERROR;
    }

    return Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc+1, argv-1);
}



static int
GetInfo(wPtr, interp, argc, argv, infoPtr, configSpecs)
    WidgetPtr wPtr;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
    FormatStruct * infoPtr;
    Tk_ConfigSpec * configSpecs;
{
    int temp;

    if (argc < 4) {
	return Tix_ArgcError(interp, argc+2, argv-2, 2, "x1 y1 x2 y2 ...");
    }
    if (Tcl_GetInt(interp, argv[0], &infoPtr->x1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &infoPtr->y1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &infoPtr->x2) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &infoPtr->y2) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tk_ConfigureWidget(interp, wPtr->dispData.tkwin, configSpecs,
	    argc-4, argv+4, (char *)infoPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (infoPtr->x1 > infoPtr->x2) {
	temp = infoPtr->x1;
	infoPtr->x1 = infoPtr->x2;
	infoPtr->x2 = temp;
    }
    if (infoPtr->y1 > infoPtr->y2) {
	temp = infoPtr->y1;
	infoPtr->y1 = infoPtr->y2;
	infoPtr->y2 = temp;
    }

    /* trivial rejects */
    if (infoPtr->x1 > wPtr->renderInfo->fmt.x2) {
	return TCL_BREAK;
    }
    if (infoPtr->x2 < wPtr->renderInfo->fmt.x1) {
	return TCL_BREAK;
    }
    if (infoPtr->y1 > wPtr->renderInfo->fmt.y2) {
	return TCL_BREAK;
    }
    if (infoPtr->y2 < wPtr->renderInfo->fmt.y1) {
	return TCL_BREAK;
    }

    /* the area is indeed visible, do some clipping */
    if (infoPtr->x1 < wPtr->renderInfo->fmt.x1) {
	infoPtr->x1 = wPtr->renderInfo->fmt.x1;
    }
    if (infoPtr->x2 > wPtr->renderInfo->fmt.x2) {
	infoPtr->x2 = wPtr->renderInfo->fmt.x2;
    }
    if (infoPtr->y1 < wPtr->renderInfo->fmt.y1) {
	infoPtr->y1 = wPtr->renderInfo->fmt.y1;
    }
    if (infoPtr->y2 > wPtr->renderInfo->fmt.y2) {
	infoPtr->y2 = wPtr->renderInfo->fmt.y2;
    }

    return TCL_OK;
}

static void
GetBlockPosn(wPtr, x1, y1, x2, y2, bx1, by1, bx2, by2)
    WidgetPtr wPtr;
    int x1;		/* cell index */
    int x2;
    int y1;
    int y2;
    int * bx1;		/* block index */
    int * by1;
    int * bx2;
    int * by2;
{
    *bx1 = x1;
    *bx2 = x2;
    *by1 = y1;
    *by2 = y2;

    switch (wPtr->renderInfo->fmt.whichArea) {
      case TIX_S_MARGIN:
	break;
      case TIX_X_MARGIN:
	*bx1 -= wPtr->scrollInfo[0].offset;
	*bx2 -= wPtr->scrollInfo[0].offset;
	break;
      case TIX_Y_MARGIN:
	*by1 -= wPtr->scrollInfo[1].offset;
	*by2 -= wPtr->scrollInfo[1].offset;
	break;
      case TIX_MAIN:
	*bx1 -= wPtr->scrollInfo[0].offset;
	*bx2 -= wPtr->scrollInfo[0].offset;
	*by1 -= wPtr->scrollInfo[1].offset;
	*by2 -= wPtr->scrollInfo[1].offset;
	break;
    }
} 

static void
GetRenderPosn(wPtr, bx1, by1, bx2, by2, rx1, ry1, rx2, ry2)
    WidgetPtr wPtr;
    int bx1;		/* block index */
    int by1;
    int bx2;
    int by2;
    int * rx1;		/* render buffer position */
    int * ry1;
    int * rx2;
    int * ry2;
{
    int x, y, i;


    for (x=0,i=0; i<=bx2; i++) {
	if (i == bx1) {
	    *rx1 = x;
	}
	if (i == bx2) {
	    *rx2 = x + wPtr->mainRB->dispSize[0][i].total - 1;
	    break;

	}
	x += wPtr->mainRB->dispSize[0][i].total;
    }


    for (y=0,i=0; i<=by2; i++) {
	if (i == by1) {
	    *ry1 = y;
	}
	if (i == by2) {
	    *ry2 = y + wPtr->mainRB->dispSize[1][i].total - 1;
	    break;
	}
	y += wPtr->mainRB->dispSize[1][i].total;
    }

    *rx1 += wPtr->renderInfo->origin[0];
    *rx2 += wPtr->renderInfo->origin[0];
    *ry1 += wPtr->renderInfo->origin[1];
    *ry2 += wPtr->renderInfo->origin[1];
} 

static void
Tix_GrFillCells(wPtr, border, selectBorder, bx1, by1, bx2, by2,
	borderWidth, relief, filled, bw)
    WidgetPtr wPtr;
    Tk_3DBorder border;
    Tk_3DBorder selectBorder;
    int bx1;
    int by1;
    int bx2;
    int by2;
    int borderWidth;
    int relief;
    int filled;
    int bw[2][2];
{
    int rx1, ry1, rx2, ry2;
    int i, j;
    Tk_3DBorder targetBorder;

    for (i=bx1; i<=bx2; i++) {
	for (j=by1; j<=by2; j++) {

	    if (filled) {
		GetRenderPosn(wPtr, i, j, i, j, &rx1,&ry1, &rx2,&ry2);

		if (wPtr->mainRB->elms[i][j].selected) {
		    targetBorder = selectBorder;
		} else {
		    targetBorder = border;
		}

		Tk_Fill3DRectangle(wPtr->dispData.tkwin,
		    wPtr->renderInfo->drawable,
		    targetBorder, rx1, ry1, rx2-rx1+1, ry2-ry1+1,
	    	    0, TK_RELIEF_FLAT);

		wPtr->mainRB->elms[i][j].filled = 1;
	    } else {
		if (!wPtr->mainRB->elms[i][j].filled) {
		    if (i == bx1) {
			if (wPtr->mainRB->elms[i][j].borderW[0][0] < bw[0][0]){
			    wPtr->mainRB->elms[i][j].borderW[0][0] = bw[0][0];
			}
		    }
		    if (i == bx2) {
			if (wPtr->mainRB->elms[i][j].borderW[0][1] < bw[0][1]){
			    wPtr->mainRB->elms[i][j].borderW[0][1] = bw[0][1];
			}
		    }
		    if (j == by1) {
			if (wPtr->mainRB->elms[i][j].borderW[1][0] < bw[1][0]){
			    wPtr->mainRB->elms[i][j].borderW[1][0] = bw[1][0];
			}
		    }
		    if (j == by2) {
			if (wPtr->mainRB->elms[i][j].borderW[1][1] < bw[1][1]){
			    wPtr->mainRB->elms[i][j].borderW[1][1] = bw[1][1];
			}
		    }
		}
	    }
	}
    }
    if (borderWidth > 0) {
	GetRenderPosn(wPtr, bx1, by1, bx2, by2, &rx1,&ry1, &rx2,&ry2);

	if (bx1 == bx2 &&  by1 == by2) {
	    /* special case: if a single cell is selected, we invert the
	     * border */

	    if (wPtr->mainRB->elms[bx1][by1].selected) {
		if (relief == TK_RELIEF_RAISED) {
		    relief = TK_RELIEF_SUNKEN;
		}
		else if (relief == TK_RELIEF_SUNKEN) {
		    relief = TK_RELIEF_RAISED;
		}
	    }
	}

	Tk_Draw3DRectangle(wPtr->dispData.tkwin,
	    wPtr->renderInfo->drawable,
	    border, rx1, ry1, rx2-rx1+1, ry2-ry1+1,
	    borderWidth, relief);
    }
}

static int
Tix_GrFormatBorder(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    BorderFmtStruct info;
    int code = TCL_OK;
    int bx1, bx2, by1, by2;
    int i, j;

    info.x1		= 0;
    info.y1		= 0;
    info.x2		= 0;
    info.y2		= 0;
    info.border		= NULL;
    info.borderWidth	= 0;
    info.selectBorder	= NULL;
    info.relief		= TK_RELIEF_FLAT;
    info.xon		= 0;
    info.xoff		= 0;
    info.yon		= 0;
    info.yoff		= 0;
    info.filled		= 0;

    if ((code = GetInfo(wPtr, interp, argc, argv, (FormatStruct*)&info,
	    borderConfigSpecs))!= TCL_OK) {
	goto done;
    }

    /* 
     * If the xon is not specified, then by default the xon is encloses the
     * whole region. Same for yon.
     */
    if (info.xon == 0) {
	info.xon = info.x2 - info.x1 + 1;
	info.xoff = 0;
    }
    if (info.yon == 0) {
	info.yon = info.y2 - info.y1 + 1;
	info.yoff = 0;
    }

    GetBlockPosn(wPtr, info.x1, info.y1, info.x2, info.y2,
	&bx1, &by1, &bx2, &by2);

#if 0
    /* now it works */
#ifdef __WIN32__
    if (bx1 == 0 && bx2 == 0 && by1 == 0 && by2 == 0) {
	/* some how this doesn't work in BC++ 4.5 */
	goto done;
    }
#endif
#endif

    for (i=bx1; i<=bx2; i+=(info.xon+info.xoff)) {
	for (j=by1; j<=by2; j+=(info.yon+info.yoff)) {
	    int _bx1, _by1, _bx2, _by2;
	    int borderWidths[2][2];

	    _bx1 = i;
	    _bx2 = i+info.xon-1;
	    _by1 = j;
	    _by2 = j+info.yon-1;

	    if (_bx2 > bx2) {
		_bx2 = bx2;
	    }
	    if (_by2 > by2) {
		_by2 = by2;
	    }

	    borderWidths[0][0] = info.borderWidth;
	    borderWidths[0][1] = info.borderWidth;
	    borderWidths[1][0] = info.borderWidth;
	    borderWidths[1][1] = info.borderWidth;

	    Tix_GrFillCells(wPtr, info.border, info.selectBorder,
		_bx1, _by1, _bx2, _by2,
		info.borderWidth, info.relief, info.filled, borderWidths);
	}
    }

  done:
    if (code == TCL_BREAK) {
	code = TCL_OK;
    }
    if (code == TCL_OK) {
	if (Tix_GrSaveColor(wPtr, TK_CONFIG_BORDER, (void*)info.border) == 0) {
	    info.border = (Tk_3DBorder)NULL;
	}
	if (Tix_GrSaveColor(wPtr, TK_CONFIG_BORDER, (void*)info.selectBorder)
		== 0) {
	    info.selectBorder = (Tk_3DBorder)NULL;
	}
	Tk_FreeOptions(borderConfigSpecs, (char *)&info,
	    wPtr->dispData.display, 0);
    }
    return code;
}

static int
Tix_GrFormatGrid(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    GridFmtStruct info;
    int code = TCL_OK;
    int rx1, rx2, ry1, ry2;
    int bx1, bx2, by1, by2;
    int i, j;
    GC gc;
    int borderWidths[2][2];

    info.x1		= 0;
    info.y1		= 0;
    info.x2		= 0;
    info.y2		= 0;
    info.border		= NULL;
    info.selectBorder	= NULL;
    info.bgBorder	= NULL;
    info.borderWidth	= 0;
    info.relief		= TK_RELIEF_FLAT;
    info.xon		= 1;
    info.xoff		= 0;
    info.yon		= 1;
    info.yoff		= 0;
    info.filled		= 0;

    if ((code = GetInfo(wPtr, interp, argc, argv, (FormatStruct*)&info,
	    gridConfigSpecs))!= TCL_OK) {
	goto done;
    }
    gc = Tk_3DBorderGC(wPtr->dispData.tkwin, info.border, 
	TK_3D_FLAT_GC);

    GetBlockPosn(wPtr, info.x1, info.y1, info.x2, info.y2,
	&bx1, &by1, &bx2, &by2);

    borderWidths[0][0] = 0;
    borderWidths[0][1] = 0;
    borderWidths[1][0] = 0;
    borderWidths[1][1] = 0;

    switch(info.anchor) {
      case TK_ANCHOR_N:
      case TK_ANCHOR_NE:
      case TK_ANCHOR_NW:
	borderWidths[1][0] = info.borderWidth;
	break;
      default:
	; /* do nothing. This line gets rid of compiler warnings */
    }
    switch(info.anchor) {
      case TK_ANCHOR_SE:
      case TK_ANCHOR_S:
      case TK_ANCHOR_SW:
	borderWidths[1][1] = info.borderWidth;
	break;
      default:
	; /* do nothing. This line gets rid of compiler warnings */
    }
    switch(info.anchor) {
      case TK_ANCHOR_SW:
      case TK_ANCHOR_W:
      case TK_ANCHOR_NW:
	borderWidths[0][0] = info.borderWidth;
	break;
      default:
	; /* do nothing. This line gets rid of compiler warnings */
    }
    switch(info.anchor) {
      case TK_ANCHOR_NE:
      case TK_ANCHOR_E:
      case TK_ANCHOR_SE:
	borderWidths[0][1] = info.borderWidth;
	break;
      default:
	; /* do nothing. This line gets rid of compiler warnings */
    }

    for (i=bx1; i<=bx2; i+=(info.xon+info.xoff)) {
	for (j=by1; j<=by2; j+=(info.yon+info.yoff)) {
	    int _bx1, _by1, _bx2, _by2;

	    _bx1 = i;
	    _bx2 = i+info.xon-1;
	    _by1 = j;
	    _by2 = j+info.yon-1;

	    if (_bx2 > bx2) {
		_bx2 = bx2;
	    }
	    if (_by2 > by2) {
		_by2 = by2;
	    }

	    Tix_GrFillCells(wPtr, info.bgBorder, info.selectBorder,
		_bx1, _by1, _bx2, _by2, 0, TK_RELIEF_FLAT, info.filled,
		borderWidths);

	    if (info.borderWidth > 0) {
		GetRenderPosn(wPtr, _bx1, _by1, _bx2, _by2,
		    &rx1,&ry1, &rx2,&ry2);

		switch(info.anchor) {
		  case TK_ANCHOR_N:
		  case TK_ANCHOR_NE:
		  case TK_ANCHOR_NW:
		    XDrawLine(wPtr->dispData.display,
		        wPtr->renderInfo->drawable, gc,
		    	rx1, ry1, rx2, ry1);
		    break;
		  default:
		    ; /* do nothing. This line gets rid of compiler warnings */
		}
		switch(info.anchor) {
		  case TK_ANCHOR_SE:
		  case TK_ANCHOR_S:
		  case TK_ANCHOR_SW:
		    XDrawLine(wPtr->dispData.display,
		        wPtr->renderInfo->drawable, gc,
		    	rx1, ry2, rx2, ry2);
		    break;
		  default:
		    ; /* do nothing. This line gets rid of compiler warnings */
		}
		switch(info.anchor) {
		  case TK_ANCHOR_SW:
		  case TK_ANCHOR_W:
		  case TK_ANCHOR_NW:
		    XDrawLine(wPtr->dispData.display,
		    	wPtr->renderInfo->drawable, gc,
		    	rx1, ry1, rx1, ry2);
		    break;
		  default:
		    ; /* do nothing. This line gets rid of compiler warnings */
		}
		switch(info.anchor) {
		  case TK_ANCHOR_NE:
		  case TK_ANCHOR_E:
		  case TK_ANCHOR_SE:
		    XDrawLine(wPtr->dispData.display,
		    	wPtr->renderInfo->drawable, gc,
		    	rx2, ry1, rx2, ry2);
		    break;
		  default:
		    ; /* do nothing. This line gets rid of compiler warnings */
		}
	    }
	}
    }

  done:
    if (code == TCL_BREAK) {
	code = TCL_OK;
    }
    if (code == TCL_OK) {
	if (Tix_GrSaveColor(wPtr, TK_CONFIG_BORDER, (void*)info.border) == 0) {
	    info.border    = (Tk_3DBorder)NULL;
	}
	if (Tix_GrSaveColor(wPtr, TK_CONFIG_BORDER, (void*)info.bgBorder)==0) {
	    info.bgBorder  = (Tk_3DBorder)NULL;
	}
	if (Tix_GrSaveColor(wPtr, TK_CONFIG_BORDER, (void*)info.selectBorder)
		== 0) {
	    info.selectBorder = (Tk_3DBorder)NULL;
	}
	Tk_FreeOptions(gridConfigSpecs, (char *)&info, wPtr->dispData.display,
	    0);
    }
    return code;
}


/* returns 1 if the caller can free the border/color */
static int Tix_GrSaveColor(wPtr, type, ptr)
    WidgetPtr wPtr;
    CFG_TYPE type;
    void * ptr;
{
    long pixel;
    Tix_ListIterator li;
    int found;
    ColorInfo * cPtr;

    if (type == TK_CONFIG_COLOR) {
	pixel = ((XColor *)ptr)->pixel;
    } else {
	pixel = Tk_3DBorderColor((Tk_3DBorder)ptr)->pixel;
    }

    Tix_SimpleListIteratorInit(&li);
    for (found = 0, Tix_SimpleListStart(&wPtr->colorInfo, &li);
	 !Tix_SimpleListDone(&li);
	 Tix_SimpleListNext (&wPtr->colorInfo, &li)) {

	cPtr = (ColorInfo *)li.curr;
	if (cPtr->pixel == pixel) {
	    cPtr->counter = wPtr->colorInfoCounter;
	    return 1;
	    
	}
    }

    cPtr = (ColorInfo *)ckalloc(sizeof(ColorInfo));
	
    if (type == TK_CONFIG_COLOR) {
	cPtr->color  = (XColor *)ptr;
    } else {
	cPtr->border = (Tk_3DBorder)ptr;
    }
    cPtr->type  = (int)type;
    cPtr->pixel = pixel;
    cPtr->counter = wPtr->colorInfoCounter;

    Tix_SimpleListAppend(&wPtr->colorInfo, (char*)cPtr, 0);
    return 0;
}

void
Tix_GrFreeUnusedColors(wPtr, freeAll)
    WidgetPtr wPtr;
    int freeAll;
{
    Tix_ListIterator li;
    ColorInfo * cPtr;

    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&wPtr->colorInfo, &li);
	 !Tix_SimpleListDone(&li);
	 Tix_SimpleListNext (&wPtr->colorInfo, &li)) {

	cPtr = (ColorInfo *)li.curr;
	if (freeAll || cPtr->counter < wPtr->colorInfoCounter) {
	    Tix_SimpleListDelete(&wPtr->colorInfo, &li);

	    if (cPtr->type == (int)(TK_CONFIG_COLOR)) {
		Tk_FreeColor(cPtr->color);
	    } else {
		Tk_Free3DBorder(cPtr->border);
	    }
	    ckfree((char*)cPtr);
	}
    }
}
