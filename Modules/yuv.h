
#ifndef Py_YUV_H
#define Py_YUV_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * SVideo YUV 4:1:1 format.
 *
 * 4 consecutive quadwords describe 8 pixels on 2 lines, as depicted
 * below.  An array of (width/4) of the below structure describes 2
 * scan lines.
 *
 * +-------------------+
 * | 00 | 01 | 02 | 03 | . . .
 * +-------------------+
 * | 10 | 11 | 12 | 13 | . . .
 * +-------------------+
 */
struct yuv411 {
	struct {
		unsigned int dummy:8;
		unsigned int y0:8;
		unsigned int u0:2;
		unsigned int v0:2;
		unsigned int y1:8;
		unsigned int u1:2;
		unsigned int v1:2;
	} v[4];
};

#define YUV411_Y00(y)	(y).v[0].y0
#define YUV411_Y01(y)	(y).v[1].y0
#define YUV411_Y02(y)	(y).v[2].y0
#define YUV411_Y03(y)	(y).v[3].y0
#define YUV411_Y10(y)	(y).v[0].y1
#define YUV411_Y11(y)	(y).v[1].y1
#define YUV411_Y12(y)	(y).v[2].y1
#define YUV411_Y13(y)	(y).v[3].y1
#define YUV411_U00(y)	((y).v[0].u0<<6|(y).v[1].u0<<4|(y).v[2].u0<<2|(y).v[3].u0)
#define YUV411_U01(y)	YUV411_U00(y)
#define YUV411_U02(y)	YUV411_U00(y)
#define YUV411_U03(y)	YUV411_U00(y)
#define YUV411_U10(y)	((y).v[0].u1<<6|(y).v[1].u1<<4|(y).v[2].u1<<2|(y).v[3].u1)
#define YUV411_U11(y)	YUV411_U10(y)
#define YUV411_U12(y)	YUV411_U10(y)
#define YUV411_U13(y)	YUV411_U10(y)
#define YUV411_V00(y)	((y).v[0].v0<<6|(y).v[1].v0<<4|(y).v[2].v0<<2|(y).v[3].v0)
#define YUV411_V01(y)	YUV411_V00(y)
#define YUV411_V02(y)	YUV411_V00(y)
#define YUV411_V03(y)	YUV411_V00(y)
#define YUV411_V10(y)	((y).v[0].v1<<6|(y).v[1].v1<<4|(y).v[2].v1<<2|(y).v[3].v1)
#define YUV411_V11(y)	YUV411_V10(y)
#define YUV411_V12(y)	YUV411_V10(y)
#define YUV411_V13(y)	YUV411_V10(y)

/*
 * Compression Library YUV 4:2:2 format.
 *
 * 1 longword describes 2 pixels.
 *
 * +-------+
 * | 0 | 1 |
 * +-------+
 */
struct yuv422 {
	unsigned int u:8;
	unsigned int y0:8;
	unsigned int v:8;
	unsigned int y1:8;
};
#define YUV422_Y0(y)	(y).y0
#define YUV422_Y1(y)	(y).y1
#define YUV422_U0(y)	(y).u
#define YUV422_U1(y)	(y).u
#define YUV422_V0(y)	(y).v
#define YUV422_V1(y)	(y).v

/*
 * Compression library YUV 4:2:2 Duplicate Chroma format.
 *
 * This is like the previous format, but the U and V values are
 * duplicated vertically (and hence there is some redundancy in the
 * data).  With other words, lines 2*n and 2*n+1 have the same U and V
 * values but different Y values.
 */

/*
 * Conversion functions.
 */
void yuv_sv411_to_cl422dc(int, void *, void *, int, int);
void yuv_sv411_to_cl422dc_quartersize(int, void *, void *, int, int);
void yuv_sv411_to_cl422dc_sixteenthsize(int, void *, void *, int, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_YUV_H */
