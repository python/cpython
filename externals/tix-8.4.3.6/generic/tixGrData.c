
/*	$Id: tixGrData.c,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/* 
 * tixGrData.c --
 *
 *	This module manipulates the data structure for a Grid widget.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixGrid.h>

/* need "(unsigned int)" to prevent sign-extension on 64-bit machines, and
 * "(unsigned long)" to avoid an egcs warning
 */
#define FIX(X) ((char*)(unsigned long)(unsigned int)(X))

static int		FindRowCol _ANSI_ARGS_((TixGridDataSet * dataSet,
			    int x, int y, TixGridRowCol * rowcol[2],
			    Tcl_HashEntry * hashPtrs[2]));
static TixGridRowCol *	InitRowCol _ANSI_ARGS_((int index));
static int		RowColMaxSize _ANSI_ARGS_((WidgetPtr wPtr,
			    int which, TixGridRowCol *rowCol,
			    TixGridSize * defSize));

static TixGridRowCol *
InitRowCol(index)
    int index;
{
    TixGridRowCol * rowCol = (TixGridRowCol *)ckalloc(sizeof(TixGridRowCol));

    rowCol->dispIndex	   = index;
    rowCol->size.sizeType  = TIX_GR_DEFAULT;
    rowCol->size.sizeValue = 0;
    rowCol->size.charValue = 0;
    rowCol->size.pad0	   = 2;
    rowCol->size.pad1	   = 2;
    rowCol->size.pixels	   = 0;

    Tcl_InitHashTable(&rowCol->table, TCL_ONE_WORD_KEYS);

    return rowCol;
}

/*----------------------------------------------------------------------
 * TixGridDataSetInit --
 *
 *	Create an instance of the TixGridDataSet data structure.
 *
 *----------------------------------------------------------------------
 */
TixGridDataSet *
TixGridDataSetInit()
{
    TixGridDataSet * dataSet =(TixGridDataSet*)ckalloc(sizeof(TixGridDataSet));

    Tcl_InitHashTable(&dataSet->index[0], TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&dataSet->index[1], TCL_ONE_WORD_KEYS);

    dataSet->maxIdx[0] = -1;
    dataSet->maxIdx[1] = -1;

    return dataSet;
}

/*----------------------------------------------------------------------
 * TixGridDataSetFree --
 *
 *	Frees an instance of the TixGridDataSet data structure.
 *
 *----------------------------------------------------------------------
 */

void
TixGridDataSetFree(dataSet)
    TixGridDataSet* dataSet;
{
    Tcl_HashSearch hashSearch;
    Tcl_HashEntry *hashPtr;
    TixGridRowCol *rcPtr;
    int i;

    for (i=0; i<2; i++) {
	for (hashPtr = Tcl_FirstHashEntry(&dataSet->index[i], &hashSearch);
		 hashPtr;
		 hashPtr = Tcl_NextHashEntry(&hashSearch)) {
	    rcPtr = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
	    if (rcPtr->table.numEntries > 0) {
		fprintf(stderr, "Grid hash entry leaked: %d : %d\n", i,
		    rcPtr->dispIndex);
	    }

	    Tcl_DeleteHashTable(&rcPtr->table);
	    ckfree((char*)rcPtr);
	}
    }

    Tcl_DeleteHashTable(&dataSet->index[0]);
    Tcl_DeleteHashTable(&dataSet->index[1]);
    ckfree((char*)dataSet);
}

/*----------------------------------------------------------------------
 * TixGridDataFindEntry --
 *
 * Results:
 *	Return the element if it exists. Otherwise returns NULL.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

char *
TixGridDataFindEntry(dataSet, x, y)
    TixGridDataSet * dataSet;
    int x;
    int y;
{
    TixGridRowCol *col, *row;
    Tcl_HashEntry *hashPtr;

    /* (1) Find the row and column */
    if (!(hashPtr = Tcl_FindHashEntry(&dataSet->index[0], FIX(x)))) {
	return NULL;
    }
    col = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);

    if (!(hashPtr = Tcl_FindHashEntry(&dataSet->index[1], FIX(y)))) {
	return NULL;
    }
    row = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);

    /* (2) Find the entry */
    if (row->table.numEntries < col->table.numEntries) {
	if (!(hashPtr = Tcl_FindHashEntry(&row->table, (char*)col))) {
	    return NULL;
	}
    }
    else {
	if (!(hashPtr = Tcl_FindHashEntry(&col->table, (char*)row))) {
	    return NULL;
	}
    }

    return (char *)Tcl_GetHashValue(hashPtr);
}

/*----------------------------------------------------------------------
 * FindRowCol --
 *
 *	Internal function: finds row and column info an entry.
 *
 * Results:
 *	Returns true if BOTH row and column exist. If so, the row and
 *	column info is returned in the rowcol.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

static int
FindRowCol(dataSet, x, y, rowcol, hashPtrs)
    TixGridDataSet * dataSet;	/* The Grid dataset. */
    int x, y;			/* Location of the cell. */
    TixGridRowCol * rowcol[2];	/* Returns information about the row/col. */
    Tcl_HashEntry * hashPtrs[2];/* Returns hash table info about the row/col.*/
{
    hashPtrs[0] = Tcl_FindHashEntry(&dataSet->index[0], FIX(x));
    if (hashPtrs[0] != NULL) {
	rowcol[0] = (TixGridRowCol *)Tcl_GetHashValue(hashPtrs[0]);
    } else {
	return 0;
    }

    hashPtrs[1] = Tcl_FindHashEntry(&dataSet->index[1], FIX(y));
    if (hashPtrs[1] != NULL) {
	rowcol[1] = (TixGridRowCol *)Tcl_GetHashValue(hashPtrs[1]);
    } else {
	return 0;
    }

    return 1;
}

/*----------------------------------------------------------------------
 * TixGridDataCreateEntry --
 *
 *	Find or create the entry at the specified index.
 *
 * Results:
 *	A handle to the entry.
 *
 * Side effects:
 *	A new entry is created if it is not already in the dataset.
 *----------------------------------------------------------------------
 */

CONST84 char *
TixGridDataCreateEntry(dataSet, x, y, defaultEntry)
    TixGridDataSet * dataSet;
    int x;
    int y;
    CONST84 char * defaultEntry;
{
    TixGridRowCol *rowcol[2];
    Tcl_HashEntry *hashPtr;
    int isNew, i, dispIndex[2];

    dispIndex[0] = x;
    dispIndex[1] = y;

    for (i=0; i<2; i++) {
	hashPtr = Tcl_CreateHashEntry(&dataSet->index[i],
	    FIX(dispIndex[i]), &isNew);

	if (!isNew) {
	    rowcol[i] = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
	} else {
	    rowcol[i] = InitRowCol(dispIndex[i]);
	    Tcl_SetHashValue(hashPtr, (char*)rowcol[i]);

	    if (dataSet->maxIdx[i] < dispIndex[i]) {
		dataSet->maxIdx[i] = dispIndex[i];
	    }
	}
    }

    hashPtr = Tcl_CreateHashEntry(&rowcol[0]->table,
	(char*)rowcol[1], &isNew);

    if (!isNew) {
	return (char *) Tcl_GetHashValue(hashPtr);
    }
    else {
	TixGrEntry *chPtr = (TixGrEntry *)defaultEntry;

	Tcl_SetHashValue(hashPtr, (char*)chPtr);
	chPtr->entryPtr[0] = hashPtr;

	hashPtr = Tcl_CreateHashEntry(&rowcol[1]->table,
	    (char*)rowcol[0], &isNew);
	Tcl_SetHashValue(hashPtr, (char*)defaultEntry);
	chPtr->entryPtr[1] = hashPtr;

	return defaultEntry;
    }
}

/*----------------------------------------------------------------------
 * TixGridDataDeleteEntry --
 *
 *	Deletes the entry at the specified index.
 *
 * Results:
 *	True iff the entry existed and was deleted.
 *
 * Side effects:
 *	If there is an entry at the index, it is deleted.
 *----------------------------------------------------------------------
 */

int
TixGridDataDeleteEntry(dataSet, x, y)
    TixGridDataSet * dataSet;	/* The Grid dataset. */
    int x;			/* Column number of the entry. */
    int y;			/* Row number of the entry. */
{
    TixGridRowCol *rowcol[2];
    Tcl_HashEntry *hashPtrs[2];	/* Hash entries of the row/col. */
    Tcl_HashEntry *cx, *cy;	/* Hash entries of the cell in the row/col. */

    if (!FindRowCol(dataSet, x, y, rowcol, hashPtrs)) {
	/*
	 * The row and/or the column do not exist.
	 */
	return 0;
    }

    cx = Tcl_FindHashEntry(&rowcol[0]->table, (char*)rowcol[1]);
    cy = Tcl_FindHashEntry(&rowcol[1]->table, (char*)rowcol[0]);

    if (cx == NULL && cy == NULL) {
	return 0;
    }
    else if (cx != NULL && cy != NULL) {
	Tcl_DeleteHashEntry(cx);
	Tcl_DeleteHashEntry(cy);
    }
    else {
	panic("Inconsistent grid dataset: (%d,%d) : %x %x", x, y, cx, cy);
    }

    return 1;

    /*
     * ToDo: trim down the hash table.
     */
}

/* return value: has the size of the grid changed as a result of sorting */
int
TixGridDataUpdateSort(dataSet, axis, start, end, items)
    TixGridDataSet * dataSet;
    int axis;
    int start;
    int end;
    Tix_GrSortItem *items;
{
    TixGridRowCol **ptr;
    Tcl_HashEntry *hashPtr;
    int numItems = end - start + 1;
    int i, k, max;

    if (numItems <= 0) {
	return 0;
    }

    ptr = (TixGridRowCol **)ckalloc(numItems * sizeof(TixGridRowCol *));

    for (k=0,i=start; i<=end; i++,k++) {
	if (!(hashPtr = Tcl_FindHashEntry(&dataSet->index[axis], FIX(i)))) {
	    /*
	     * This row/col doesn't exist
	     */
	    ptr[k] = NULL;
	} else {
	    ptr[k] = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
	    Tcl_DeleteHashEntry(hashPtr);
	}
    }

    max = start;
    for (k=0,i=start; i<=end; i++,k++) {
	int pos, isNew;
	pos = items[k].index - start;

	if (ptr[pos] != NULL) {
	    hashPtr = Tcl_CreateHashEntry(&dataSet->index[axis], FIX(i),
		&isNew);
	    Tcl_SetHashValue(hashPtr, (char*)ptr[pos]);
	    ptr[pos]->dispIndex = i;
	    max = i;
	}
    }

    ckfree((char*)ptr);

    if (end+1 >= dataSet->maxIdx[axis]) {
	if (dataSet->maxIdx[axis] != max+1) {
	    dataSet->maxIdx[axis] = max+1;
	    return 1;				/* size changed */
	}
    }
    return 0;					/* size not changed */
}


static int
RowColMaxSize(wPtr, which, rowCol, defSize)
    WidgetPtr wPtr;
    int which;				/* 0=cols, 1=rows */
    TixGridRowCol *rowCol;
    TixGridSize * defSize;
{
    Tcl_HashSearch hashSearch;
    Tcl_HashEntry *hashPtr;
    TixGrEntry * chPtr;
    int maxSize = 1;

    if (rowCol->table.numEntries == 0) {
	return defSize->pixels;
    }

    for (hashPtr = Tcl_FirstHashEntry(&rowCol->table, &hashSearch);
	 hashPtr;
	 hashPtr = Tcl_NextHashEntry(&hashSearch)) {

	chPtr = (TixGrEntry *)Tcl_GetHashValue(hashPtr);
	if (maxSize < chPtr->iPtr->base.size[which]) {
	    maxSize = chPtr->iPtr->base.size[which];
	}
    }

    return maxSize;
}

/*
 *----------------------------------------------------------------------
 * TixGridDataGetRowColSize --
 *
 *	Returns width of a column or height of a row.
 *	
 * Results:
 *	The width or height.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

int
TixGridDataGetRowColSize(wPtr, dataSet, which, index, defSize, pad0, pad1)
    WidgetPtr wPtr;		/* Info about Grid widget */
    TixGridDataSet * dataSet;	/* Dataset of the Grid */
    int which;			/* 0=cols, 1=rows */
    int index;			/* Column or row number */
    TixGridSize * defSize;	/* The default size for the grid cells */
    int *pad0;			/* Holds return value of horizontal padding. */
    int *pad1;			/* Holds return value of vertical padding. */
{
    TixGridRowCol *rowCol;
    Tcl_HashEntry *hashPtr;
    int size;

    if (!(hashPtr = Tcl_FindHashEntry(&dataSet->index[which], FIX(index)))) {
	size  = defSize->pixels;
	*pad0 = defSize->pad0;
	*pad1 = defSize->pad1;
    }
    else {
	rowCol = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
    
	switch (rowCol->size.sizeType) {
	  case TIX_GR_AUTO:
	    size  = RowColMaxSize(wPtr, which, rowCol, defSize);
	    *pad0 = rowCol->size.pad0;
	    *pad1 = rowCol->size.pad1;
	    break;

	  case TIX_GR_DEFINED_PIXEL:
	    size  = rowCol->size.sizeValue;
	    *pad0 = rowCol->size.pad0;
	    *pad1 = rowCol->size.pad1;
	    break;

	  case TIX_GR_DEFINED_CHAR:
	    size  = (int)(rowCol->size.charValue * wPtr->fontSize[which]);
	    *pad0 = rowCol->size.pad0;
	    *pad1 = rowCol->size.pad1;
	    break;

	  case TIX_GR_DEFAULT:
	  default:			/* some error ?? */
	    if (defSize->sizeType == TIX_GR_AUTO) {
		size = RowColMaxSize(wPtr, which, rowCol, defSize);
	    } else {
		size = defSize->pixels;
	    }
	    *pad0 = defSize->pad0;
	    *pad1 = defSize->pad1;
	}
    }

    return size;
}

int
TixGridDataGetIndex(interp, wPtr, xStr, yStr, xPtr, yPtr)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    CONST84 char * xStr;
    CONST84 char * yStr;
    int * xPtr;
    int * yPtr;
{
    CONST84 char * str[2];
    int * ptr[2];
    int i;

    str[0] = xStr;
    str[1] = yStr;
    ptr[0] = xPtr;
    ptr[1] = yPtr;

    for (i=0; i<2; i++) {
	if (str[i] == NULL) {		/* ignore this index */
	    continue;
	}

	if (strcmp(str[i], "max") == 0) {
	    *ptr[i] = wPtr->dataSet->maxIdx[i];
	    if (*ptr[i] < wPtr->hdrSize[i]) {
		*ptr[i] = wPtr->hdrSize[i];
	    }
	}
	else if (strcmp(str[i], "end") == 0) {
	    *ptr[i] = wPtr->dataSet->maxIdx[i] + 1;
	    if (*ptr[i] < wPtr->hdrSize[i]) {
		*ptr[i] = wPtr->hdrSize[i];
	    }
	} else {
	    if (Tcl_GetInt(interp, str[i], ptr[i]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (*ptr[i] < 0) {
	    *ptr[i] = 0;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * TixGridDataConfigRowColSize --
 *
 *	Configure width of column or height of rows.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	The column/rows size will be changed in an idle event.
 *----------------------------------------------------------------------
 */

int
TixGridDataConfigRowColSize(interp, wPtr, dataSet, which, index, argc, argv,
	argcErrorMsg, changed_ret)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    TixGridDataSet * dataSet;
    int which;			/* 0=cols, 1=rows */
    int index;
    int argc;
    CONST84 char ** argv;
    CONST84 char * argcErrorMsg;
    int *changed_ret;		/* Returns whether size has been changed. */
{
    TixGridRowCol *rowCol;
    Tcl_HashEntry *hashPtr;
    int isNew, code;

    hashPtr = Tcl_CreateHashEntry(&dataSet->index[which], FIX(index), &isNew);

    if (!isNew) {
	rowCol = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
    } else {
	rowCol = InitRowCol(index);
	Tcl_SetHashValue(hashPtr, (char*)rowCol);

	if (dataSet->maxIdx[which] < index) {
	    dataSet->maxIdx[which] = index;
	}
    }

    code = Tix_GrConfigSize(interp, wPtr, argc, argv, &rowCol->size,
	argcErrorMsg, changed_ret);

    if (changed_ret) {
	*changed_ret |= isNew;
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 * TixGridDataGetGridSize --
 *
 *	Returns the number of rows and columns of the grid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

/*
 * ToDo: maintain numCol and numRow info while adding entries.
 */

void
TixGridDataGetGridSize(dataSet, numCol_ret, numRow_ret)
    TixGridDataSet * dataSet;
    int *numCol_ret;
    int *numRow_ret;
{
    int maxSize[2], i;
    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch hashSearch;
    TixGridRowCol * rowCol;

    maxSize[0] = 1;
    maxSize[1] = 1;

    if (dataSet->index[0].numEntries == 0 || dataSet->index[1].numEntries==0) {
	goto done;
    }

    for (i=0; i<2; i++) {
	
	for (hashPtr = Tcl_FirstHashEntry(&dataSet->index[i], &hashSearch);
	     hashPtr;
	     hashPtr = Tcl_NextHashEntry(&hashSearch)) {

	    rowCol = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
	    if (maxSize[i] < rowCol->dispIndex+1) {
		maxSize[i] = rowCol->dispIndex+1;
	    }
	}
    }

  done:
    if (numCol_ret) {
	*numCol_ret  = maxSize[0];
    }
    if (numRow_ret) {
	*numRow_ret = maxSize[1];
    }
}


/*
 * the following four functions return true if done -- no more rows or cells
 * are left to traverse
 */

int
TixGrDataFirstRow(dataSet, rowSearchPtr)
    TixGridDataSet* dataSet;
    Tix_GrDataRowSearch * rowSearchPtr;
{
    rowSearchPtr->hashPtr = Tcl_FirstHashEntry(&dataSet->index[0],
	&rowSearchPtr->hashSearch);

    if (rowSearchPtr->hashPtr != NULL) {
	rowSearchPtr->row = (TixGridRowCol *)Tcl_GetHashValue(
		rowSearchPtr->hashPtr);
	return 0;
    } else {
	rowSearchPtr->row = NULL;
	return 1;
    }
}

int
TixGrDataNextRow(rowSearchPtr)
    Tix_GrDataRowSearch * rowSearchPtr;
{
    rowSearchPtr->hashPtr = Tcl_NextHashEntry(&rowSearchPtr->hashSearch);

    if (rowSearchPtr->hashPtr != NULL) {
	rowSearchPtr->row = (TixGridRowCol *)Tcl_GetHashValue(
		rowSearchPtr->hashPtr);
	return 0;
    } else {
	rowSearchPtr->row = NULL;
	return 1;
    }
}

int
TixGrDataFirstCell(rowSearchPtr, cellSearchPtr)
    Tix_GrDataRowSearch * rowSearchPtr;
    Tix_GrDataCellSearch * cellSearchPtr;
{
    cellSearchPtr->hashPtr = Tcl_FirstHashEntry(&rowSearchPtr->row->table,
	&cellSearchPtr->hashSearch);

    if (cellSearchPtr->hashPtr != NULL) {
	cellSearchPtr->data = (char *)Tcl_GetHashValue(
		cellSearchPtr->hashPtr);
	return 0;
    } else {
	cellSearchPtr->data = NULL;
	return 1;
    }
}

int
TixGrDataNextCell(cellSearchPtr)
    Tix_GrDataCellSearch * cellSearchPtr;
{
    cellSearchPtr->hashPtr = Tcl_NextHashEntry(&cellSearchPtr->hashSearch);

    if (cellSearchPtr->hashPtr != NULL) {
	cellSearchPtr->data = (char *)Tcl_GetHashValue(
		cellSearchPtr->hashPtr);
	return 0;
    } else {
	cellSearchPtr->data = NULL;
	return 1;
    }
}

/*----------------------------------------------------------------------
 * TixGridDataDeleteSearchedEntry --
 *
 *	Deletes an entry returned by one of the search functions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an entry at the index, it is deleted.
 *----------------------------------------------------------------------
 */

void
TixGridDataDeleteSearchedEntry(cellSearchPtr)
    Tix_GrDataCellSearch * cellSearchPtr;
{
    TixGrEntry * chPtr = (TixGrEntry *)cellSearchPtr->data;

    Tcl_DeleteHashEntry(chPtr->entryPtr[0]);
    Tcl_DeleteHashEntry(chPtr->entryPtr[1]);
}

/*
 *----------------------------------------------------------------------
 * TixGridDataDeleteRange --
 *
 *	Deletes the rows (columns) at the given range.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given rows (columns) are deleted.
 *----------------------------------------------------------------------
 */

void
TixGridDataDeleteRange(wPtr, dataSet, which, from, to)
    WidgetPtr wPtr;		/* Info about the grid widget. */
    TixGridDataSet * dataSet;	/* Dataset of the Grid */
    int which;			/* 0=cols, 1=rows */
    int from;			/* Starting column/row. */
    int to;			/* Ending column/row (inclusive). */
{
    int tmp, i, other, deleted = 0;

    if (from < 0 ) {
	from = 0;
    }
    if (to < 0 ) {
	to = 0;
    }
    if (from > to) {
	tmp  = to;
	to   = from;
	from = tmp;
    }
    if (which == 0) {
	other = 1;
    } else {
	other = 0;
    }

    for (i=from; i<=to; i++) {
	Tcl_HashEntry *hashPtr, *hp, *toDel;
	TixGridRowCol *rcPtr, *rcp;
	Tcl_HashSearch hashSearch;

	hashPtr = Tcl_FindHashEntry(&dataSet->index[which], FIX(i));
	if (hashPtr != NULL) {
	    rcPtr = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);

	    for (hp = Tcl_FirstHashEntry(&dataSet->index[other], &hashSearch);
		    hp;
		    hp = Tcl_NextHashEntry(&hashSearch)) {

		rcp = (TixGridRowCol *)Tcl_GetHashValue(hp);
		toDel = Tcl_FindHashEntry(&rcp->table, (char*)rcPtr);
		if (toDel != NULL) {
		    TixGrEntry * chPtr;

		    chPtr = (TixGrEntry *)Tcl_GetHashValue(toDel);
		    if (chPtr) {
			deleted = 1;
			Tix_GrFreeElem(chPtr);
		    }

		    Tcl_DeleteHashEntry(toDel);
		}
	    }

	    Tcl_DeleteHashEntry(hashPtr);
	    Tcl_DeleteHashTable(&rcPtr->table);
	    ckfree((char*)rcPtr);
	}
    }

    if (deleted) {
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    }
}

/*
 *----------------------------------------------------------------------
 * TixGridDataMoveRange --
 *
 *	Moves a range of row (columns) by a given offset. E.g. move 2-4 by 2
 *	changes the rows 2,3,4 to 4,5,6.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Rows (columns) at locations where the given rows will be moved
 *	to are deleted.
 *----------------------------------------------------------------------
 */

void
TixGridDataMoveRange(wPtr, dataSet, which, from, to, by)
    WidgetPtr wPtr;		/* Info about the grid widget. */
    TixGridDataSet * dataSet;	/* Dataset of the Grid */
    int which;			/* 0=cols, 1=rows */
    int from;			/* Starting column/row. */
    int to;			/* Ending column/row (inclusive). */
    int by;			/* The distance of the move. */
{
    int tmp, i, s, e, incr;
    int df, dt;			/* Rows inside this range will be deleted
				 * before the given rows are moved. */

    if (by == 0) {
	return;
    }
    if (from < 0 ) {
	from = 0;
    }
    if (to < 0 ) {
	to = 0;
    }
    if (from > to) {
	tmp  = to;
	to   = from;
	from = tmp;
    }

    if ((from + by) < 0) {
	/*
	 * Delete the leading rows that will be moved beyond the top of grid.
	 */
	int n;			/* Number of rows to delete. */

	n = - (from + by);
	if (n > (to - from + 1)) {
	    n =  to - from + 1;
	}
	TixGridDataDeleteRange(wPtr, dataSet, which, from, (from+n-1));
	from = from + n;

	if (from > to) {
	    /*
	     * All the rows have been deleted.
	     */
	    return;
	}
    }

    /*
     * Delete rows at locations where the given rows will be moved to.
     */
    df = from + by;
    dt = to   + by;

    if (by > 0) {
	if (df <= to) {
	    df = to + 1;
	}
    } else {
	if (dt >= from) {
	    dt = from - 1;
	}
    }
    TixGridDataDeleteRange(wPtr, dataSet, which, df, dt);

    /*
     * Rename the rows.
     */
    if (by > 0) {
	s    = to;
	e    = from-1;
	incr = -1;
    } else {
	s    = from;
	e    = to+1;
	incr = 1;
    } 

    for (i=s; i!=e; i+=incr) {
	Tcl_HashEntry *hashPtr;
	TixGridRowCol *rcPtr;
	int isNew;

	hashPtr = Tcl_FindHashEntry(&dataSet->index[which], FIX(i));
	if (hashPtr != NULL) {
	    rcPtr = (TixGridRowCol *)Tcl_GetHashValue(hashPtr);
	    rcPtr->dispIndex = i+by;
	    Tcl_DeleteHashEntry(hashPtr);
	    hashPtr = Tcl_CreateHashEntry(&dataSet->index[which], FIX(i+by),
		    &isNew);
	    Tcl_SetHashValue(hashPtr, (char*)rcPtr);
	}
    }
}
