/*
 * tkTrig.c --
 *
 *	This file contains a collection of trigonometry utility routines that
 *	are used by Tk and in particular by the canvas code. It also has
 *	miscellaneous geometry functions used by canvases.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkCanvas.h"

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/*
 *--------------------------------------------------------------
 *
 * TkLineToPoint --
 *
 *	Compute the distance from a point to a finite line segment.
 *
 * Results:
 *	The return value is the distance from the line segment whose
 *	end-points are *end1Ptr and *end2Ptr to the point given by *pointPtr.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
TkLineToPoint(
    double end1Ptr[2],		/* Coordinates of first end-point of line. */
    double end2Ptr[2],		/* Coordinates of second end-point of line. */
    double pointPtr[2])		/* Points to coords for point. */
{
    double x, y;

    /*
     * Compute the point on the line that is closest to the point. This must
     * be done separately for vertical edges, horizontal edges, and other
     * edges.
     */

    if (end1Ptr[0] == end2Ptr[0]) {

	/*
	 * Vertical edge.
	 */

	x = end1Ptr[0];
	if (end1Ptr[1] >= end2Ptr[1]) {
	    y = MIN(end1Ptr[1], pointPtr[1]);
	    y = MAX(y, end2Ptr[1]);
	} else {
	    y = MIN(end2Ptr[1], pointPtr[1]);
	    y = MAX(y, end1Ptr[1]);
	}
    } else if (end1Ptr[1] == end2Ptr[1]) {

	/*
	 * Horizontal edge.
	 */

	y = end1Ptr[1];
	if (end1Ptr[0] >= end2Ptr[0]) {
	    x = MIN(end1Ptr[0], pointPtr[0]);
	    x = MAX(x, end2Ptr[0]);
	} else {
	    x = MIN(end2Ptr[0], pointPtr[0]);
	    x = MAX(x, end1Ptr[0]);
	}
    } else {
	double m1, b1, m2, b2;

	/*
	 * The edge is neither horizontal nor vertical. Convert the edge to a
	 * line equation of the form y = m1*x + b1. Then compute a line
	 * perpendicular to this edge but passing through the point, also in
	 * the form y = m2*x + b2.
	 */

	m1 = (end2Ptr[1] - end1Ptr[1])/(end2Ptr[0] - end1Ptr[0]);
	b1 = end1Ptr[1] - m1*end1Ptr[0];
	m2 = -1.0/m1;
	b2 = pointPtr[1] - m2*pointPtr[0];
	x = (b2 - b1)/(m1 - m2);
	y = m1*x + b1;
	if (end1Ptr[0] > end2Ptr[0]) {
	    if (x > end1Ptr[0]) {
		x = end1Ptr[0];
		y = end1Ptr[1];
	    } else if (x < end2Ptr[0]) {
		x = end2Ptr[0];
		y = end2Ptr[1];
	    }
	} else {
	    if (x > end2Ptr[0]) {
		x = end2Ptr[0];
		y = end2Ptr[1];
	    } else if (x < end1Ptr[0]) {
		x = end1Ptr[0];
		y = end1Ptr[1];
	    }
	}
    }

    /*
     * Compute the distance to the closest point.
     */

    return hypot(pointPtr[0] - x, pointPtr[1] - y);
}

/*
 *--------------------------------------------------------------
 *
 * TkLineToArea --
 *
 *	Determine whether a line lies entirely inside, entirely outside, or
 *	overlapping a given rectangular area.
 *
 * Results:
 *	-1 is returned if the line given by end1Ptr and end2Ptr is entirely
 *	outside the rectangle given by rectPtr. 0 is returned if the polygon
 *	overlaps the rectangle, and 1 is returned if the polygon is entirely
 *	inside the rectangle.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkLineToArea(
    double end1Ptr[2],		/* X and y coordinates for one endpoint of
				 * line. */
    double end2Ptr[2],		/* X and y coordinates for other endpoint of
				 * line. */
    double rectPtr[4])		/* Points to coords for rectangle, in the
				 * order x1, y1, x2, y2. X1 must be no larger
				 * than x2, and y1 no larger than y2. */
{
    int inside1, inside2;

    /*
     * First check the two points individually to see whether they are inside
     * the rectangle or not.
     */

    inside1 = (end1Ptr[0] >= rectPtr[0]) && (end1Ptr[0] <= rectPtr[2])
	    && (end1Ptr[1] >= rectPtr[1]) && (end1Ptr[1] <= rectPtr[3]);
    inside2 = (end2Ptr[0] >= rectPtr[0]) && (end2Ptr[0] <= rectPtr[2])
	    && (end2Ptr[1] >= rectPtr[1]) && (end2Ptr[1] <= rectPtr[3]);
    if (inside1 != inside2) {
	return 0;
    }
    if (inside1 & inside2) {
	return 1;
    }

    /*
     * Both points are outside the rectangle, but still need to check for
     * intersections between the line and the rectangle. Horizontal and
     * vertical lines are particularly easy, so handle them separately.
     */

    if (end1Ptr[0] == end2Ptr[0]) {
	/*
	 * Vertical line.
	 */

	if (((end1Ptr[1] >= rectPtr[1]) ^ (end2Ptr[1] >= rectPtr[1]))
		&& (end1Ptr[0] >= rectPtr[0])
		&& (end1Ptr[0] <= rectPtr[2])) {
	    return 0;
	}
    } else if (end1Ptr[1] == end2Ptr[1]) {
	/*
	 * Horizontal line.
	 */

	if (((end1Ptr[0] >= rectPtr[0]) ^ (end2Ptr[0] >= rectPtr[0]))
		&& (end1Ptr[1] >= rectPtr[1])
		&& (end1Ptr[1] <= rectPtr[3])) {
	    return 0;
	}
    } else {
	double m, x, y, low, high;

	/*
	 * Diagonal line. Compute slope of line and use for intersection
	 * checks against each of the sides of the rectangle: left, right,
	 * bottom, top.
	 */

	m = (end2Ptr[1] - end1Ptr[1])/(end2Ptr[0] - end1Ptr[0]);
	if (end1Ptr[0] < end2Ptr[0]) {
	    low = end1Ptr[0];
	    high = end2Ptr[0];
	} else {
	    low = end2Ptr[0];
	    high = end1Ptr[0];
	}

	/*
	 * Left edge.
	 */

	y = end1Ptr[1] + (rectPtr[0] - end1Ptr[0])*m;
	if ((rectPtr[0] >= low) && (rectPtr[0] <= high)
		&& (y >= rectPtr[1]) && (y <= rectPtr[3])) {
	    return 0;
	}

	/*
	 * Right edge.
	 */

	y += (rectPtr[2] - rectPtr[0])*m;
	if ((y >= rectPtr[1]) && (y <= rectPtr[3])
		&& (rectPtr[2] >= low) && (rectPtr[2] <= high)) {
	    return 0;
	}

	/*
	 * Bottom edge.
	 */

	if (end1Ptr[1] < end2Ptr[1]) {
	    low = end1Ptr[1];
	    high = end2Ptr[1];
	} else {
	    low = end2Ptr[1];
	    high = end1Ptr[1];
	}
	x = end1Ptr[0] + (rectPtr[1] - end1Ptr[1])/m;
	if ((x >= rectPtr[0]) && (x <= rectPtr[2])
		&& (rectPtr[1] >= low) && (rectPtr[1] <= high)) {
	    return 0;
	}

	/*
	 * Top edge.
	 */

	x += (rectPtr[3] - rectPtr[1])/m;
	if ((x >= rectPtr[0]) && (x <= rectPtr[2])
		&& (rectPtr[3] >= low) && (rectPtr[3] <= high)) {
	    return 0;
	}
    }
    return -1;
}

/*
 *--------------------------------------------------------------
 *
 * TkThickPolyLineToArea --
 *
 *	This function is called to determine whether a connected series of
 *	line segments lies entirely inside, entirely outside, or overlapping a
 *	given rectangular area.
 *
 * Results:
 *	-1 is returned if the lines are entirely outside the area, 0 if they
 *	overlap, and 1 if they are entirely inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkThickPolyLineToArea(
    double *coordPtr,		/* Points to an array of coordinates for the
				 * polyline: x0, y0, x1, y1, ... */
    int numPoints,		/* Total number of points at *coordPtr. */
    double width,		/* Width of each line segment. */
    int capStyle,		/* How are end-points of polyline drawn?
				 * CapRound, CapButt, or CapProjecting. */
    int joinStyle,		/* How are joints in polyline drawn?
				 * JoinMiter, JoinRound, or JoinBevel. */
    double *rectPtr)		/* Rectangular area to check against. */
{
    double radius, poly[10];
    int count;
    int changedMiterToBevel;	/* Non-zero means that a mitered corner had to
				 * be treated as beveled after all because the
				 * angle was < 11 degrees. */
    int inside;			/* Tentative guess about what to return, based
				 * on all points seen so far: one means
				 * everything seen so far was inside the area;
				 * -1 means everything was outside the area.
				 * 0 means overlap has been found. */

    radius = width/2.0;
    inside = -1;

    if ((coordPtr[0] >= rectPtr[0]) && (coordPtr[0] <= rectPtr[2])
	    && (coordPtr[1] >= rectPtr[1]) && (coordPtr[1] <= rectPtr[3])) {
	inside = 1;
    }

    /*
     * Iterate through all of the edges of the line, computing a polygon for
     * each edge and testing the area against that polygon. In addition, there
     * are additional tests to deal with rounded joints and caps.
     */

    changedMiterToBevel = 0;
    for (count = numPoints; count >= 2; count--, coordPtr += 2) {
	/*
	 * If rounding is done around the first point of the edge then test a
	 * circular region around the point with the area.
	 */

	if (((capStyle == CapRound) && (count == numPoints))
		|| ((joinStyle == JoinRound) && (count != numPoints))) {
	    poly[0] = coordPtr[0] - radius;
	    poly[1] = coordPtr[1] - radius;
	    poly[2] = coordPtr[0] + radius;
	    poly[3] = coordPtr[1] + radius;
	    if (TkOvalToArea(poly, rectPtr) != inside) {
		return 0;
	    }
	}

	/*
	 * Compute the polygonal shape corresponding to this edge, consisting
	 * of two points for the first point of the edge and two points for
	 * the last point of the edge.
	 */

	if (count == numPoints) {
	    TkGetButtPoints(coordPtr+2, coordPtr, width,
		    capStyle == CapProjecting, poly, poly+2);
	} else if ((joinStyle == JoinMiter) && !changedMiterToBevel) {
	    poly[0] = poly[6];
	    poly[1] = poly[7];
	    poly[2] = poly[4];
	    poly[3] = poly[5];
	} else {
	    TkGetButtPoints(coordPtr+2, coordPtr, width, 0, poly, poly+2);

	    /*
	     * If the last joint was beveled, then also check a polygon
	     * comprising the last two points of the previous polygon and the
	     * first two from this polygon; this checks the wedges that fill
	     * the beveled joint.
	     */

	    if ((joinStyle == JoinBevel) || changedMiterToBevel) {
		poly[8] = poly[0];
		poly[9] = poly[1];
		if (TkPolygonToArea(poly, 5, rectPtr) != inside) {
		    return 0;
		}
		changedMiterToBevel = 0;
	    }
	}
	if (count == 2) {
	    TkGetButtPoints(coordPtr, coordPtr+2, width,
		    capStyle == CapProjecting, poly+4, poly+6);
	} else if (joinStyle == JoinMiter) {
	    if (TkGetMiterPoints(coordPtr, coordPtr+2, coordPtr+4,
		    (double) width, poly+4, poly+6) == 0) {
		changedMiterToBevel = 1;
		TkGetButtPoints(coordPtr, coordPtr+2, width, 0, poly+4,
			poly+6);
	    }
	} else {
	    TkGetButtPoints(coordPtr, coordPtr+2, width, 0, poly+4, poly+6);
	}
	poly[8] = poly[0];
	poly[9] = poly[1];
	if (TkPolygonToArea(poly, 5, rectPtr) != inside) {
	    return 0;
	}
    }

    /*
     * If caps are rounded, check the cap around the final point of the line.
     */

    if (capStyle == CapRound) {
	poly[0] = coordPtr[0] - radius;
	poly[1] = coordPtr[1] - radius;
	poly[2] = coordPtr[0] + radius;
	poly[3] = coordPtr[1] + radius;
	if (TkOvalToArea(poly, rectPtr) != inside) {
	    return 0;
	}
    }

    return inside;
}

/*
 *--------------------------------------------------------------
 *
 * TkPolygonToPoint --
 *
 *	Compute the distance from a point to a polygon.
 *
 * Results:
 *	The return value is 0.0 if the point referred to by pointPtr is within
 *	the polygon referred to by polyPtr and numPoints. Otherwise the return
 *	value is the distance of the point from the polygon.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
TkPolygonToPoint(
    double *polyPtr,		/* Points to an array coordinates for closed
				 * polygon: x0, y0, x1, y1, ... The polygon
				 * may be self-intersecting. */
    int numPoints,		/* Total number of points at *polyPtr. */
    double *pointPtr)		/* Points to coords for point. */
{
    double bestDist;		/* Closest distance between point and any edge
				 * in polygon. */
    int intersections;		/* Number of edges in the polygon that
				 * intersect a ray extending vertically
				 * upwards from the point to infinity. */
    int count;
    register double *pPtr;

    /*
     * Iterate through all of the edges in the polygon, updating bestDist and
     * intersections.
     *
     * TRICKY POINT: when computing intersections, include left x-coordinate
     * of line within its range, but not y-coordinate. Otherwise if the point
     * lies exactly below a vertex we'll count it as two intersections.
     */

    bestDist = 1.0e36;
    intersections = 0;

    for (count = numPoints, pPtr = polyPtr; count > 1; count--, pPtr += 2) {
	double x, y, dist;

	/*
	 * Compute the point on the current edge closest to the point and
	 * update the intersection count. This must be done separately for
	 * vertical edges, horizontal edges, and other edges.
	 */

	if (pPtr[2] == pPtr[0]) {

	    /*
	     * Vertical edge.
	     */

	    x = pPtr[0];
	    if (pPtr[1] >= pPtr[3]) {
		y = MIN(pPtr[1], pointPtr[1]);
		y = MAX(y, pPtr[3]);
	    } else {
		y = MIN(pPtr[3], pointPtr[1]);
		y = MAX(y, pPtr[1]);
	    }
	} else if (pPtr[3] == pPtr[1]) {

	    /*
	     * Horizontal edge.
	     */

	    y = pPtr[1];
	    if (pPtr[0] >= pPtr[2]) {
		x = MIN(pPtr[0], pointPtr[0]);
		x = MAX(x, pPtr[2]);
		if ((pointPtr[1] < y) && (pointPtr[0] < pPtr[0])
			&& (pointPtr[0] >= pPtr[2])) {
		    intersections++;
		}
	    } else {
		x = MIN(pPtr[2], pointPtr[0]);
		x = MAX(x, pPtr[0]);
		if ((pointPtr[1] < y) && (pointPtr[0] < pPtr[2])
			&& (pointPtr[0] >= pPtr[0])) {
		    intersections++;
		}
	    }
	} else {
	    double m1, b1, m2, b2;
	    int lower;		/* Non-zero means point below line. */

	    /*
	     * The edge is neither horizontal nor vertical. Convert the edge
	     * to a line equation of the form y = m1*x + b1. Then compute a
	     * line perpendicular to this edge but passing through the point,
	     * also in the form y = m2*x + b2.
	     */

	    m1 = (pPtr[3] - pPtr[1])/(pPtr[2] - pPtr[0]);
	    b1 = pPtr[1] - m1*pPtr[0];
	    m2 = -1.0/m1;
	    b2 = pointPtr[1] - m2*pointPtr[0];
	    x = (b2 - b1)/(m1 - m2);
	    y = m1*x + b1;
	    if (pPtr[0] > pPtr[2]) {
		if (x > pPtr[0]) {
		    x = pPtr[0];
		    y = pPtr[1];
		} else if (x < pPtr[2]) {
		    x = pPtr[2];
		    y = pPtr[3];
		}
	    } else {
		if (x > pPtr[2]) {
		    x = pPtr[2];
		    y = pPtr[3];
		} else if (x < pPtr[0]) {
		    x = pPtr[0];
		    y = pPtr[1];
		}
	    }
	    lower = (m1*pointPtr[0] + b1) > pointPtr[1];
	    if (lower && (pointPtr[0] >= MIN(pPtr[0], pPtr[2]))
		    && (pointPtr[0] < MAX(pPtr[0], pPtr[2]))) {
		intersections++;
	    }
	}

	/*
	 * Compute the distance to the closest point, and see if that is the
	 * best distance seen so far.
	 */

	dist = hypot(pointPtr[0] - x, pointPtr[1] - y);
	if (dist < bestDist) {
	    bestDist = dist;
	}
    }

    /*
     * We've processed all of the points. If the number of intersections is
     * odd, the point is inside the polygon.
     */

    if (intersections & 0x1) {
	return 0.0;
    }
    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * TkPolygonToArea --
 *
 *	Determine whether a polygon lies entirely inside, entirely outside, or
 *	overlapping a given rectangular area.
 *
 * Results:
 *	-1 is returned if the polygon given by polyPtr and numPoints is
 *	entirely outside the rectangle given by rectPtr. 0 is returned if the
 *	polygon overlaps the rectangle, and 1 is returned if the polygon is
 *	entirely inside the rectangle.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPolygonToArea(
    double *polyPtr,		/* Points to an array coordinates for closed
				 * polygon: x0, y0, x1, y1, ... The polygon
				 * may be self-intersecting. */
    int numPoints,		/* Total number of points at *polyPtr. */
    register double *rectPtr)	/* Points to coords for rectangle, in the
				 * order x1, y1, x2, y2. X1 and y1 must be
				 * lower-left corner. */
{
    int state;			/* State of all edges seen so far (-1 means
				 * outside, 1 means inside, won't ever be
				 * 0). */
    int count;
    register double *pPtr;

    /*
     * Iterate over all of the edges of the polygon and test them against the
     * rectangle. Can quit as soon as the state becomes "intersecting".
     */

    state = TkLineToArea(polyPtr, polyPtr+2, rectPtr);
    if (state == 0) {
	return 0;
    }
    for (pPtr = polyPtr+2, count = numPoints-1; count >= 2;
	    pPtr += 2, count--) {
	if (TkLineToArea(pPtr, pPtr+2, rectPtr) != state) {
	    return 0;
	}
    }

    /*
     * If all of the edges were inside the rectangle we're done. If all of the
     * edges were outside, then the rectangle could still intersect the
     * polygon (if it's entirely enclosed). Call TkPolygonToPoint to figure
     * this out.
     */

    if (state == 1) {
	return 1;
    }
    if (TkPolygonToPoint(polyPtr, numPoints, rectPtr) == 0.0) {
	return 0;
    }
    return -1;
}

/*
 *--------------------------------------------------------------
 *
 * TkOvalToPoint --
 *
 *	Computes the distance from a given point to a given oval, in canvas
 *	units.
 *
 * Results:
 *	The return value is 0 if the point given by *pointPtr is inside the
 *	oval. If the point isn't inside the oval then the return value is
 *	approximately the distance from the point to the oval. If the oval is
 *	filled, then anywhere in the interior is considered "inside"; if the
 *	oval isn't filled, then "inside" means only the area occupied by the
 *	outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
double
TkOvalToPoint(
    double ovalPtr[4],		/* Pointer to array of four coordinates (x1,
				 * y1, x2, y2) defining oval's bounding
				 * box. */
    double width,		/* Width of outline for oval. */
    int filled,			/* Non-zero means oval should be treated as
				 * filled; zero means only consider
				 * outline. */
    double pointPtr[2])		/* Coordinates of point. */
{
    double xDelta, yDelta, scaledDistance, distToOutline, distToCenter;
    double xDiam, yDiam;

    /*
     * Compute the distance between the center of the oval and the point in
     * question, using a coordinate system where the oval has been transformed
     * to a circle with unit radius.
     */

    xDelta = (pointPtr[0] - (ovalPtr[0] + ovalPtr[2])/2.0);
    yDelta = (pointPtr[1] - (ovalPtr[1] + ovalPtr[3])/2.0);
    distToCenter = hypot(xDelta, yDelta);
    scaledDistance = hypot(xDelta / ((ovalPtr[2] + width - ovalPtr[0])/2.0),
	    yDelta / ((ovalPtr[3] + width - ovalPtr[1])/2.0));

    /*
     * If the scaled distance is greater than 1 then it means no hit. Compute
     * the distance from the point to the edge of the circle, then scale this
     * distance back to the original coordinate system.
     *
     * Note: this distance isn't completely accurate. It's only an
     * approximation, and it can overestimate the correct distance when the
     * oval is eccentric.
     */

    if (scaledDistance > 1.0) {
	return (distToCenter/scaledDistance) * (scaledDistance - 1.0);
    }

    /*
     * Scaled distance less than 1 means the point is inside the outer edge of
     * the oval. If this is a filled oval, then we have a hit. Otherwise, do
     * the same computation as above (scale back to original coordinate
     * system), but also check to see if the point is within the width of the
     * outline.
     */

    if (filled) {
	return 0.0;
    }
    if (scaledDistance > 1E-10) {
	distToOutline = (distToCenter/scaledDistance) * (1.0 - scaledDistance)
		- width;
    } else {
	/*
	 * Avoid dividing by a very small number (it could cause an arithmetic
	 * overflow). This problem occurs if the point is very close to the
	 * center of the oval.
	 */

	xDiam = ovalPtr[2] - ovalPtr[0];
	yDiam = ovalPtr[3] - ovalPtr[1];
	if (xDiam < yDiam) {
	    distToOutline = (xDiam - width)/2;
	} else {
	    distToOutline = (yDiam - width)/2;
	}
    }

    if (distToOutline < 0.0) {
	return 0.0;
    }
    return distToOutline;
}

/*
 *--------------------------------------------------------------
 *
 * TkOvalToArea --
 *
 *	Determine whether an oval lies entirely inside, entirely outside, or
 *	overlapping a given rectangular area.
 *
 * Results:
 *	-1 is returned if the oval described by ovalPtr is entirely outside
 *	the rectangle given by rectPtr. 0 is returned if the oval overlaps the
 *	rectangle, and 1 is returned if the oval is entirely inside the
 *	rectangle.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkOvalToArea(
    register double *ovalPtr,	/* Points to coordinates defining the
				 * bounding rectangle for the oval: x1, y1,
				 * x2, y2. X1 must be less than x2 and y1 less
				 * than y2. */
    register double *rectPtr)	/* Points to coords for rectangle, in the
				 * order x1, y1, x2, y2. X1 and y1 must be
				 * lower-left corner. */
{
    double centerX, centerY, radX, radY, deltaX, deltaY;

    /*
     * First, see if oval is entirely inside rectangle or entirely outside
     * rectangle.
     */

    if ((rectPtr[0] <= ovalPtr[0]) && (rectPtr[2] >= ovalPtr[2])
	    && (rectPtr[1] <= ovalPtr[1]) && (rectPtr[3] >= ovalPtr[3])) {
	return 1;
    }
    if ((rectPtr[2] < ovalPtr[0]) || (rectPtr[0] > ovalPtr[2])
	    || (rectPtr[3] < ovalPtr[1]) || (rectPtr[1] > ovalPtr[3])) {
	return -1;
    }

    /*
     * Next, go through the rectangle side by side. For each side of the
     * rectangle, find the point on the side that is closest to the oval's
     * center, and see if that point is inside the oval. If at least one such
     * point is inside the oval, then the rectangle intersects the oval.
     */

    centerX = (ovalPtr[0] + ovalPtr[2])/2;
    centerY = (ovalPtr[1] + ovalPtr[3])/2;
    radX = (ovalPtr[2] - ovalPtr[0])/2;
    radY = (ovalPtr[3] - ovalPtr[1])/2;

    deltaY = rectPtr[1] - centerY;
    if (deltaY < 0.0) {
	deltaY = centerY - rectPtr[3];
	if (deltaY < 0.0) {
	    deltaY = 0;
	}
    }
    deltaY /= radY;
    deltaY *= deltaY;

    /*
     * Left side:
     */

    deltaX = (rectPtr[0] - centerX)/radX;
    deltaX *= deltaX;
    if ((deltaX + deltaY) <= 1.0) {
	return 0;
    }

    /*
     * Right side:
     */

    deltaX = (rectPtr[2] - centerX)/radX;
    deltaX *= deltaX;
    if ((deltaX + deltaY) <= 1.0) {
	return 0;
    }

    deltaX = rectPtr[0] - centerX;
    if (deltaX < 0.0) {
	deltaX = centerX - rectPtr[2];
	if (deltaX < 0.0) {
	    deltaX = 0;
	}
    }
    deltaX /= radX;
    deltaX *= deltaX;

    /*
     * Bottom side:
     */

    deltaY = (rectPtr[1] - centerY)/radY;
    deltaY *= deltaY;
    if ((deltaX + deltaY) < 1.0) {
	return 0;
    }

    /*
     * Top side:
     */

    deltaY = (rectPtr[3] - centerY)/radY;
    deltaY *= deltaY;
    if ((deltaX + deltaY) < 1.0) {
	return 0;
    }

    return -1;
}

/*
 *--------------------------------------------------------------
 *
 * TkIncludePoint --
 *
 *	Given a point and a generic canvas item header, expand the item's
 *	bounding box if needed to include the point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The boudn.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
void
TkIncludePoint(
    register Tk_Item *itemPtr,	/* Item whose bounding box is being
				 * calculated. */
    double *pointPtr)		/* Address of two doubles giving x and y
				 * coordinates of point. */
{
    int tmp;

    tmp = (int) (pointPtr[0] + 0.5);
    if (tmp < itemPtr->x1) {
	itemPtr->x1 = tmp;
    }
    if (tmp > itemPtr->x2) {
	itemPtr->x2 = tmp;
    }
    tmp = (int) (pointPtr[1] + 0.5);
    if (tmp < itemPtr->y1) {
	itemPtr->y1 = tmp;
    }
    if (tmp > itemPtr->y2) {
	itemPtr->y2 = tmp;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkBezierScreenPoints --
 *
 *	Given four control points, create a larger set of XPoints for a Bezier
 *	curve based on the points.
 *
 * Results:
 *	The array at *xPointPtr gets filled in with numSteps XPoints
 *	corresponding to the Bezier spline defined by the four control points.
 *	Note: no output point is generated for the first input point, but an
 *	output point *is* generated for the last input point.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkBezierScreenPoints(
    Tk_Canvas canvas,		/* Canvas in which curve is to be drawn. */
    double control[],		/* Array of coordinates for four control
				 * points: x0, y0, x1, y1, ... x3 y3. */
    int numSteps,		/* Number of curve points to generate. */
    register XPoint *xPointPtr)	/* Where to put new points. */
{
    int i;
    double u, u2, u3, t, t2, t3;

    for (i = 1; i <= numSteps; i++, xPointPtr++) {
	t = ((double) i)/((double) numSteps);
	t2 = t*t;
	t3 = t2*t;
	u = 1.0 - t;
	u2 = u*u;
	u3 = u2*u;
	Tk_CanvasDrawableCoords(canvas,
		(control[0]*u3 + 3.0 * (control[2]*t*u2 + control[4]*t2*u)
		    + control[6]*t3),
		(control[1]*u3 + 3.0 * (control[3]*t*u2 + control[5]*t2*u)
		    + control[7]*t3),
		&xPointPtr->x, &xPointPtr->y);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkBezierPoints --
 *
 *	Given four control points, create a larger set of points for a Bezier
 *	curve based on the points.
 *
 * Results:
 *	The array at *coordPtr gets filled in with 2*numSteps coordinates,
 *	which correspond to the Bezier spline defined by the four control
 *	points. Note: no output point is generated for the first input point,
 *	but an output point *is* generated for the last input point.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkBezierPoints(
    double control[],		/* Array of coordinates for four control
				 * points: x0, y0, x1, y1, ... x3 y3. */
    int numSteps,		/* Number of curve points to generate. */
    register double *coordPtr)	/* Where to put new points. */
{
    int i;
    double u, u2, u3, t, t2, t3;

    for (i = 1; i <= numSteps; i++, coordPtr += 2) {
	t = ((double) i)/((double) numSteps);
	t2 = t*t;
	t3 = t2*t;
	u = 1.0 - t;
	u2 = u*u;
	u3 = u2*u;
	coordPtr[0] = control[0]*u3
		+ 3.0 * (control[2]*t*u2 + control[4]*t2*u) + control[6]*t3;
	coordPtr[1] = control[1]*u3
		+ 3.0 * (control[3]*t*u2 + control[5]*t2*u) + control[7]*t3;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkMakeBezierCurve --
 *
 *	Given a set of points, create a new set of points that fit parabolic
 *	splines to the line segments connecting the original points. Produces
 *	output points in either of two forms.
 *
 *	Note: the name of this function should *not* be taken to mean that it
 *	interprets the input points as directly defining Bezier curves.
 *	Rather, it internally computes a Bezier curve representation of each
 *	parabolic spline segment. (These Bezier curves are then flattened to
 *	produce the points filled into the output arrays.)
 *
 * Results:
 *	Either or both of the xPoints or dblPoints arrays are filled in. The
 *	return value is the number of points placed in the arrays. Note: if
 *	the first and last points are the same, then a closed curve is
 *	generated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkMakeBezierCurve(
    Tk_Canvas canvas,		/* Canvas in which curve is to be drawn. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints,		/* Number of points at pointPtr. */
    int numSteps,		/* Number of steps to use for each spline
				 * segments (determines smoothness of
				 * curve). */
    XPoint xPoints[],		/* Array of XPoints to fill in (e.g. for
				 * display). NULL means don't fill in any
				 * XPoints. */
    double dblPoints[])		/* Array of points to fill in as doubles, in
				 * the form x0, y0, x1, y1, .... NULL means
				 * don't fill in anything in this form. Caller
				 * must make sure that this array has enough
				 * space. */
{
    int closed, outputPoints, i;
    int numCoords = numPoints*2;
    double control[8];

    /*
     * If the curve is a closed one then generate a special spline that spans
     * the last points and the first ones. Otherwise just put the first point
     * into the output.
     */

    if (!pointPtr) {
	/*
	 * Of pointPtr == NULL, this function returns an upper limit of the
	 * array size to store the coordinates. This can be used to allocate
	 * storage, before the actual coordinates are calculated.
	 */

	return 1 + numPoints * numSteps;
    }

    outputPoints = 0;
    if ((pointPtr[0] == pointPtr[numCoords-2])
	    && (pointPtr[1] == pointPtr[numCoords-1])) {
	closed = 1;
	control[0] = 0.5*pointPtr[numCoords-4] + 0.5*pointPtr[0];
	control[1] = 0.5*pointPtr[numCoords-3] + 0.5*pointPtr[1];
	control[2] = 0.167*pointPtr[numCoords-4] + 0.833*pointPtr[0];
	control[3] = 0.167*pointPtr[numCoords-3] + 0.833*pointPtr[1];
	control[4] = 0.833*pointPtr[0] + 0.167*pointPtr[2];
	control[5] = 0.833*pointPtr[1] + 0.167*pointPtr[3];
	control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	if (xPoints != NULL) {
	    Tk_CanvasDrawableCoords(canvas, control[0], control[1],
		    &xPoints->x, &xPoints->y);
	    TkBezierScreenPoints(canvas, control, numSteps, xPoints+1);
	    xPoints += numSteps+1;
	}
	if (dblPoints != NULL) {
	    dblPoints[0] = control[0];
	    dblPoints[1] = control[1];
	    TkBezierPoints(control, numSteps, dblPoints+2);
	    dblPoints += 2*(numSteps+1);
	}
	outputPoints += numSteps+1;
    } else {
	closed = 0;
	if (xPoints != NULL) {
	    Tk_CanvasDrawableCoords(canvas, pointPtr[0], pointPtr[1],
		    &xPoints->x, &xPoints->y);
	    xPoints += 1;
	}
	if (dblPoints != NULL) {
	    dblPoints[0] = pointPtr[0];
	    dblPoints[1] = pointPtr[1];
	    dblPoints += 2;
	}
	outputPoints += 1;
    }

    for (i = 2; i < numPoints; i++, pointPtr += 2) {
	/*
	 * Set up the first two control points. This is done differently for
	 * the first spline of an open curve than for other cases.
	 */

	if ((i == 2) && !closed) {
	    control[0] = pointPtr[0];
	    control[1] = pointPtr[1];
	    control[2] = 0.333*pointPtr[0] + 0.667*pointPtr[2];
	    control[3] = 0.333*pointPtr[1] + 0.667*pointPtr[3];
	} else {
	    control[0] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	    control[1] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	    control[2] = 0.167*pointPtr[0] + 0.833*pointPtr[2];
	    control[3] = 0.167*pointPtr[1] + 0.833*pointPtr[3];
	}

	/*
	 * Set up the last two control points. This is done differently for
	 * the last spline of an open curve than for other cases.
	 */

	if ((i == (numPoints-1)) && !closed) {
	    control[4] = .667*pointPtr[2] + .333*pointPtr[4];
	    control[5] = .667*pointPtr[3] + .333*pointPtr[5];
	    control[6] = pointPtr[4];
	    control[7] = pointPtr[5];
	} else {
	    control[4] = .833*pointPtr[2] + .167*pointPtr[4];
	    control[5] = .833*pointPtr[3] + .167*pointPtr[5];
	    control[6] = 0.5*pointPtr[2] + 0.5*pointPtr[4];
	    control[7] = 0.5*pointPtr[3] + 0.5*pointPtr[5];
	}

	/*
	 * If the first two points coincide, or if the last two points
	 * coincide, then generate a single straight-line segment by
	 * outputting the last control point.
	 */

	if (((pointPtr[0] == pointPtr[2]) && (pointPtr[1] == pointPtr[3]))
		|| ((pointPtr[2] == pointPtr[4])
		&& (pointPtr[3] == pointPtr[5]))) {
	    if (xPoints != NULL) {
		Tk_CanvasDrawableCoords(canvas, control[6], control[7],
			&xPoints[0].x, &xPoints[0].y);
		xPoints++;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = control[6];
		dblPoints[1] = control[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	    continue;
	}

	/*
	 * Generate a Bezier spline using the control points.
	 */


	if (xPoints != NULL) {
	    TkBezierScreenPoints(canvas, control, numSteps, xPoints);
	    xPoints += numSteps;
	}
	if (dblPoints != NULL) {
	    TkBezierPoints(control, numSteps, dblPoints);
	    dblPoints += 2*numSteps;
	}
	outputPoints += numSteps;
    }
    return outputPoints;
}

/*
 *--------------------------------------------------------------
 *
 * TkMakeRawCurve --
 *
 *	Interpret the given set of points as the raw knots and control points
 *	defining a sequence of cubic Bezier curves. Create a new set of points
 *	that fit these Bezier curves. Output points are produced in either of
 *	two forms.
 *
 * Results:
 *	Either or both of the xPoints or dblPoints arrays are filled in. The
 *	return value is the number of points placed in the arrays.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkMakeRawCurve(
    Tk_Canvas canvas,		/* Canvas in which curve is to be drawn. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints,		/* Number of points at pointPtr. */
    int numSteps,		/* Number of steps to use for each curve
				 * segment (determines smoothness of
				 * curve). */
    XPoint xPoints[],		/* Array of XPoints to fill in (e.g. for
				 * display). NULL means don't fill in any
				 * XPoints. */
    double dblPoints[])		/* Array of points to fill in as doubles, in
				 * the form x0, y0, x1, y1, .... NULL means
				 * don't fill in anything in this form.
				 * Caller must make sure that this array has
				 * enough space. */
{
    int outputPoints, i;
    int numSegments = (numPoints+1)/3;
    double *segPtr;

    /*
     * The input describes a curve with s Bezier curve segments if there are
     * 3s+1, 3s, or 3s-1 input points. In the last two cases, 1 or 2 initial
     * points from the first curve segment are reused as defining points also
     * for the last curve segment. In the case of 3s input points, this will
     * automatically close the curve.
     */

    if (!pointPtr) {
	/*
	 * If pointPtr == NULL, this function returns an upper limit of the
	 * array size to store the coordinates. This can be used to allocate
	 * storage, before the actual coordinates are calculated.
	 */

	return 1 + numSegments * numSteps;
    }

    outputPoints = 0;
    if (xPoints != NULL) {
	Tk_CanvasDrawableCoords(canvas, pointPtr[0], pointPtr[1],
		&xPoints->x, &xPoints->y);
	xPoints += 1;
    }
    if (dblPoints != NULL) {
	dblPoints[0] = pointPtr[0];
	dblPoints[1] = pointPtr[1];
	dblPoints += 2;
    }
    outputPoints += 1;

    /*
     * The next loop handles all curve segments except one that overlaps the
     * end of the list of coordinates.
     */

    for (i=numPoints,segPtr=pointPtr ; i>=4 ; i-=3,segPtr+=6) {
	if (segPtr[0]==segPtr[2] && segPtr[1]==segPtr[3] &&
		segPtr[4]==segPtr[6] && segPtr[5]==segPtr[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line. A
	     * single point is sufficient.
	     */

	    if (xPoints != NULL) {
		Tk_CanvasDrawableCoords(canvas, segPtr[6], segPtr[7],
			&xPoints->x, &xPoints->y);
		xPoints += 1;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = segPtr[6];
		dblPoints[1] = segPtr[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    if (xPoints != NULL) {
		TkBezierScreenPoints(canvas, segPtr, numSteps, xPoints);
		xPoints += numSteps;
	    }
	    if (dblPoints != NULL) {
		TkBezierPoints(segPtr, numSteps, dblPoints);
		dblPoints += 2*numSteps;
	    }
	    outputPoints += numSteps;
	}
    }

    /*
     * If at this point i>1, then there is some point which has not yet been
     * used. Make another curve segment.
     */

    if (i > 1) {
	int j;
	double control[8];

	/*
	 * Copy the relevant coordinates to control[], so that it can be
	 * passed as a unit to e.g. TkBezierPoints.
	 */

	for (j=0; j<2*i; j++) {
	    control[j] = segPtr[j];
	}
	for (; j<8; j++) {
	    control[j] = pointPtr[j-2*i];
	}

	/*
	 * Then we just do the same things as above.
	 */

	if (control[0]==control[2] && control[1]==control[3] &&
		control[4]==control[6] && control[5]==control[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line. A
	     * single point is sufficient.
	     */

	    if (xPoints != NULL) {
		Tk_CanvasDrawableCoords(canvas, control[6], control[7],
			&xPoints->x, &xPoints->y);
		xPoints += 1;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = control[6];
		dblPoints[1] = control[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    if (xPoints != NULL) {
		TkBezierScreenPoints(canvas, control, numSteps, xPoints);
		xPoints += numSteps;
	    }
	    if (dblPoints != NULL) {
		TkBezierPoints(control, numSteps, dblPoints);
		dblPoints += 2*numSteps;
	    }
	    outputPoints += numSteps;
	}
    }

    return outputPoints;
}

/*
 *--------------------------------------------------------------
 *
 * TkMakeBezierPostscript --
 *
 *	This function generates Postscript commands that create a path
 *	corresponding to a given Bezier curve.
 *
 * Results:
 *	None. Postscript commands to generate the path are appended to the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkMakeBezierPostscript(
    Tcl_Interp *interp,		/* Interpreter in whose result the Postscript
				 * is to be stored. */
    Tk_Canvas canvas,		/* Canvas widget for which the Postscript is
				 * being generated. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints)		/* Number of points at pointPtr. */
{
    int closed, i;
    int numCoords = numPoints*2;
    double control[8];
    Tcl_Obj *psObj;

    /*
     * If the curve is a closed one then generate a special spline that spans
     * the last points and the first ones. Otherwise just put the first point
     * into the path.
     */

    if ((pointPtr[0] == pointPtr[numCoords-2])
	    && (pointPtr[1] == pointPtr[numCoords-1])) {
	closed = 1;
	control[0] = 0.5*pointPtr[numCoords-4] + 0.5*pointPtr[0];
	control[1] = 0.5*pointPtr[numCoords-3] + 0.5*pointPtr[1];
	control[2] = 0.167*pointPtr[numCoords-4] + 0.833*pointPtr[0];
	control[3] = 0.167*pointPtr[numCoords-3] + 0.833*pointPtr[1];
	control[4] = 0.833*pointPtr[0] + 0.167*pointPtr[2];
	control[5] = 0.833*pointPtr[1] + 0.167*pointPtr[3];
	control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	psObj = Tcl_ObjPrintf(
		"%.15g %.15g moveto\n"
		"%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		control[0], Tk_CanvasPsY(canvas, control[1]),
		control[2], Tk_CanvasPsY(canvas, control[3]),
		control[4], Tk_CanvasPsY(canvas, control[5]),
		control[6], Tk_CanvasPsY(canvas, control[7]));
    } else {
	closed = 0;
	control[6] = pointPtr[0];
	control[7] = pointPtr[1];
	psObj = Tcl_ObjPrintf("%.15g %.15g moveto\n",
		control[6], Tk_CanvasPsY(canvas, control[7]));
    }

    /*
     * Cycle through all the remaining points in the curve, generating a curve
     * section for each vertex in the linear path.
     */

    for (i = numPoints-2, pointPtr += 2; i > 0; i--, pointPtr += 2) {
	control[2] = 0.333*control[6] + 0.667*pointPtr[0];
	control[3] = 0.333*control[7] + 0.667*pointPtr[1];

	/*
	 * Set up the last two control points. This is done differently for
	 * the last spline of an open curve than for other cases.
	 */

	if ((i == 1) && !closed) {
	    control[6] = pointPtr[2];
	    control[7] = pointPtr[3];
	} else {
	    control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	    control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	}
	control[4] = 0.333*control[6] + 0.667*pointPtr[0];
	control[5] = 0.333*control[7] + 0.667*pointPtr[1];

	Tcl_AppendPrintfToObj(psObj,
		"%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		control[2], Tk_CanvasPsY(canvas, control[3]),
		control[4], Tk_CanvasPsY(canvas, control[5]),
		control[6], Tk_CanvasPsY(canvas, control[7]));
    }

    Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
    Tcl_DecrRefCount(psObj);
}

/*
 *--------------------------------------------------------------
 *
 * TkMakeRawCurvePostscript --
 *
 *	This function interprets the input points as the raw knot and control
 *	points for a curve composed of Bezier curve segments, just like
 *	TkMakeRawCurve. It generates Postscript commands that create a path
 *	corresponding to this given curve.
 *
 * Results:
 *	None. Postscript commands to generate the path are appended to the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkMakeRawCurvePostscript(
    Tcl_Interp *interp,		/* Interpreter in whose result the Postscript
				 * is to be stored. */
    Tk_Canvas canvas,		/* Canvas widget for which the Postscript is
				 * being generated. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints)		/* Number of points at pointPtr. */
{
    int i;
    double *segPtr;
    Tcl_Obj *psObj;

    /*
     * Put the first point into the path.
     */

    psObj = Tcl_ObjPrintf("%.15g %.15g moveto\n",
	    pointPtr[0], Tk_CanvasPsY(canvas, pointPtr[1]));

    /*
     * Loop through all the remaining points in the curve, generating a
     * straight line or curve section for every three of them.
     */

    for (i=numPoints-1,segPtr=pointPtr ; i>=3 ; i-=3,segPtr+=6) {
	if (segPtr[0]==segPtr[2] && segPtr[1]==segPtr[3] &&
		segPtr[4]==segPtr[6] && segPtr[5]==segPtr[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line.
	     */

	    Tcl_AppendPrintfToObj(psObj, "%.15g %.15g lineto\n",
		    segPtr[6], Tk_CanvasPsY(canvas, segPtr[7]));
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    Tcl_AppendPrintfToObj(psObj,
		    "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		    segPtr[2], Tk_CanvasPsY(canvas, segPtr[3]),
		    segPtr[4], Tk_CanvasPsY(canvas, segPtr[5]),
		    segPtr[6], Tk_CanvasPsY(canvas, segPtr[7]));
	}
    }

    /*
     * If there are any points left that haven't been used, then build the
     * last segment and generate Postscript in the same way for that.
     */

    if (i > 0) {
	int j;
	double control[8];

	for (j=0; j<2*i+2; j++) {
	    control[j] = segPtr[j];
	}
	for (; j<8; j++) {
	    control[j] = pointPtr[j-2*i-2];
	}

	if (control[0]==control[2] && control[1]==control[3] &&
		control[4]==control[6] && control[5]==control[7]) {
	    /*
	     * Straight line.
	     */

	    Tcl_AppendPrintfToObj(psObj, "%.15g %.15g lineto\n",
		    control[6], Tk_CanvasPsY(canvas, control[7]));
	} else {
	    /*
	     * Bezier curve segment.
	     */

	    Tcl_AppendPrintfToObj(psObj,
		    "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		    control[2], Tk_CanvasPsY(canvas, control[3]),
		    control[4], Tk_CanvasPsY(canvas, control[5]),
		    control[6], Tk_CanvasPsY(canvas, control[7]));
	}
    }

    Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
    Tcl_DecrRefCount(psObj);
}

/*
 *--------------------------------------------------------------
 *
 * TkGetMiterPoints --
 *
 *	Given three points forming an angle, compute the coordinates of the
 *	inside and outside points of the mitered corner formed by a line of a
 *	given width at that angle.
 *
 * Results:
 *	If the angle formed by the three points is less than 11 degrees then 0
 *	is returned and m1 and m2 aren't modified. Otherwise 1 is returned and
 *	the points at m1 and m2 are filled in with the positions of the points
 *	of the mitered corner.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkGetMiterPoints(
    double p1[],		/* Points to x- and y-coordinates of point
				 * before vertex. */
    double p2[],		/* Points to x- and y-coordinates of vertex
				 * for mitered joint. */
    double p3[],		/* Points to x- and y-coordinates of point
				 * after vertex. */
    double width,		/* Width of line. */
    double m1[],		/* Points to place to put "left" vertex point
				 * (see as you face from p1 to p2). */
    double m2[])		/* Points to place to put "right" vertex
				 * point. */
{
    double theta1;		/* Angle of segment p2-p1. */
    double theta2;		/* Angle of segment p2-p3. */
    double theta;		/* Angle between line segments (angle of
				 * joint). */
    double theta3;		/* Angle that bisects theta1 and theta2 and
				 * points to m1. */
    double dist;		/* Distance of miter points from p2. */
    double deltaX, deltaY;	/* X and y offsets cooresponding to dist
				 * (fudge factors for bounding box). */
    double p1x, p1y, p2x, p2y, p3x, p3y;
#ifndef _MSC_VER
    static const double elevenDegrees = (11.0*2.0*PI)/360.0;
#else /* msvc8 with -fp:strict requires it this way */
    static const double elevenDegrees = 0.19198621771937624;
#endif

    /*
     * Round the coordinates to integers to mimic what happens when the line
     * segments are displayed; without this code, the bounding box of a
     * mitered line can be miscomputed greatly.
     */

    p1x = floor(p1[0]+0.5);
    p1y = floor(p1[1]+0.5);
    p2x = floor(p2[0]+0.5);
    p2y = floor(p2[1]+0.5);
    p3x = floor(p3[0]+0.5);
    p3y = floor(p3[1]+0.5);

    if (p2y == p1y) {
	theta1 = (p2x < p1x) ? 0 : PI;
    } else if (p2x == p1x) {
	theta1 = (p2y < p1y) ? PI/2.0 : -PI/2.0;
    } else {
	theta1 = atan2(p1y - p2y, p1x - p2x);
    }

    if (p3y == p2y) {
	theta2 = (p3x > p2x) ? 0 : PI;
    } else if (p3x == p2x) {
	theta2 = (p3y > p2y) ? PI/2.0 : -PI/2.0;
    } else {
	theta2 = atan2(p3y - p2y, p3x - p2x);
    }

    theta = theta1 - theta2;
    if (theta > PI) {
	theta -= 2*PI;
    } else if (theta < -PI) {
	theta += 2*PI;
    }

    if ((theta < elevenDegrees) && (theta > -elevenDegrees)) {
	return 0;
    }

    dist = 0.5*width/sin(0.5*theta);
    if (dist < 0.0) {
	dist = -dist;
    }

    /*
     * Compute theta3 (make sure that it points to the left when looking from
     * p1 to p2).
     */

    theta3 = (theta1 + theta2)/2.0;
    if (sin(theta3 - (theta1 + PI)) < 0.0) {
	theta3 += PI;
    }
    deltaX = dist*cos(theta3);
    m1[0] = p2x + deltaX;
    m2[0] = p2x - deltaX;
    deltaY = dist*sin(theta3);
    m1[1] = p2y + deltaY;
    m2[1] = p2y - deltaY;

    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * TkGetButtPoints --
 *
 *	Given two points forming a line segment, compute the coordinates of
 *	two endpoints of a rectangle formed by bloating the line segment until
 *	it is width units wide.
 *
 * Results:
 *	There is no return value. M1 and m2 are filled in to correspond to m1
 *	and m2 in the diagram below:
 *
 *		   ----------------* m1
 *				   |
 *		p1 *---------------* p2
 *				   |
 *		   ----------------* m2
 *
 *	M1 and m2 will be W units apart, with p2 centered between them and
 *	m1-m2 perpendicular to p1-p2. However, if "project" is true then m1
 *	and m2 will be as follows:
 *
 *		   -------------------* m1
 *				  p2  |
 *		p1 *---------------*  |
 *				      |
 *		   -------------------* m2
 *
 *	In this case p2 will be width/2 units from the segment m1-m2.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkGetButtPoints(
    double p1[],		/* Points to x- and y-coordinates of point
				 * before vertex. */
    double p2[],		/* Points to x- and y-coordinates of vertex
				 * for mitered joint. */
    double width,		/* Width of line. */
    int project,		/* Non-zero means project p2 by an additional
				 * width/2 before computing m1 and m2. */
    double m1[],		/* Points to place to put "left" result point,
				 * as you face from p1 to p2. */
    double m2[])		/* Points to place to put "right" result
				 * point. */
{
    double length;		/* Length of p1-p2 segment. */
    double deltaX, deltaY;	/* Increments in coords. */

    width *= 0.5;
    length = hypot(p2[0] - p1[0], p2[1] - p1[1]);
    if (length == 0.0) {
	m1[0] = m2[0] = p2[0];
	m1[1] = m2[1] = p2[1];
    } else {
	deltaX = -width * (p2[1] - p1[1]) / length;
	deltaY = width * (p2[0] - p1[0]) / length;
	m1[0] = p2[0] + deltaX;
	m2[0] = p2[0] - deltaX;
	m1[1] = p2[1] + deltaY;
	m2[1] = p2[1] - deltaY;
	if (project) {
	    m1[0] += deltaY;
	    m2[0] += deltaY;
	    m1[1] -= deltaX;
	    m2[1] -= deltaX;
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
