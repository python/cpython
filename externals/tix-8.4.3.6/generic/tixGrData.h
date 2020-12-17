
/*	$Id: tixGrData.h,v 1.1.1.1 2000/05/17 11:08:42 idiscovery Exp $	*/

/*
 * tixGData.h --
 *
 *	Defines portable data structure for tixGrid.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_GRID_DATA_H_
#define _TIX_GRID_DATA_H_

/*
 * Data structure that stored the cells in a Grid widget. It is optimized
 * for column/row insertion and deletion.
 *
 * - A grid is divideded into a set of rows and columns. Each row and column
 *   is divided into a set of cells.
 *
 * - The following discusses the structure of a row. The structure of a
 *   column is the reverse of a row.
 *
 *   Row y is stored in the hash table TixGridDataSet.index[1] with
 *   the index y. Hence, to search for row y, we use the FindHashEntry
 *   operation:
 *
 *	row_y = TixGridDataSet.index[1].FindHashEntry(y);
 *
 *   To locate a cell (x,y), we can first find the row y, and then
 *   locate the cell at column x of this row. Note that the cell is
 *   *not* indexed by its column position (y), but rather by the hash
 *   table of the column y. The following example illustrates how cell
 *   (x,y) can be searched:
 *
 *	row_y = TixGridDataSet.index[1].FindHashEntry(y);
 *	col_x = TixGridDataSet.index[0].FindHashEntry(x);
 *
 *	cell_xy = row_x.list.FindHashEntry(&col_x);
 *
 *   The advantage of this arrangement is it is very efficient to
 *   insert a row into into the grid -- we just have to fix the
 *   indices of the rows table. For example, if, after the insertion,
 *   row_y is now moved to the row y1, we change its index from y to
 *   y1. In general, an insertion operation takes log(n) time in a
 *   grid that contains n items.
 *
 */
typedef struct TixGridDataSet {
    Tcl_HashTable index[2];		/* the row and column indices */
    					/* index[0] holds the columns 
					 * (horizontal index)
					 */
    int maxIdx[2];			/* the max row/col, or {-1,-1}
					 * if there are no rows/col
					 */
} TixGridDataSet;

#define TIX_GR_AUTO			0
#define TIX_GR_DEFAULT			1
#define TIX_GR_DEFINED_PIXEL		2
#define TIX_GR_DEFINED_CHAR		3

typedef struct TixGridSize {
    int sizeType;
    int sizeValue;			/* width or height */
    int pixels;
    int pad0, pad1;
    double charValue;
} TixGridSize;

typedef struct TixGridRowCol {
    /* private: */
    Tcl_HashTable table;

    /* public: */
    int dispIndex;			/* the row or column in which
					 * this TixGridRowCol is displayed */
    TixGridSize size;
} TixGridRowCol;


#endif
