/*  datetime.h
 */

#ifndef DATETIME_H
#define DATETIME_H

/* Fields are packed into successive bytes, each viewed as unsigned and
 * big-endian, unless otherwise noted:
 *
 * byte offset
 *  0 		year     2 bytes, 1-9999
 *  2		month    1 byte, 1-12
 *  3 		day      1 byte, 1-31
 *  4		hour     1 byte, 0-23
 *  5 		minute   1 byte, 0-59
 *  6 		second   1 byte, 0-59
 *  7 		usecond  3 bytes, 0-999999
 * 10
 */

/* # of bytes for year, month, and day. */
#define _PyDateTime_DATE_DATASIZE 4

/* # of bytes for hour, minute, second, and usecond. */
#define _PyDateTime_TIME_DATASIZE 6

/* # of bytes for year, month, day, hour, minute, second, and usecond. */
#define _PyDateTime_DATETIME_DATASIZE 10

#define _PyTZINFO_HEAD		\
	PyObject_HEAD		\
	long hashcode;		\
	char hastzinfo;		/* boolean flag */

typedef struct
{
	PyObject_HEAD
	long hashcode;
	unsigned char data[_PyDateTime_DATE_DATASIZE];
} PyDateTime_Date;

typedef struct
{
	PyObject_HEAD
	long hashcode;
	unsigned char data[_PyDateTime_DATETIME_DATASIZE];
} PyDateTime_DateTime;

typedef struct
{
	PyObject_HEAD
	long hashcode;
	unsigned char data[_PyDateTime_DATETIME_DATASIZE];
	PyObject *tzinfo;
} PyDateTime_DateTimeTZ;



#define _PyDateTime_TIMEHEAD	\
	_PyTZINFO_HEAD		\
	unsigned char data[_PyDateTime_TIME_DATASIZE];

typedef struct
{
	_PyDateTime_TIMEHEAD
} _PyDateTime_BaseTime;		/* hastzinfo false */

typedef struct
{
	_PyDateTime_TIMEHEAD
	PyObject *tzinfo;
} PyDateTime_Time;		/* hastzinfo true */


typedef struct
{
	PyObject_HEAD
	long hashcode;		/* -1 when unknown */
	int days;		/* -MAX_DELTA_DAYS <= days <= MAX_DELTA_DAYS */
	int seconds;		/* 0 <= seconds < 24*3600 is invariant */
	int microseconds;	/* 0 <= microseconds < 1000000 is invariant */
} PyDateTime_Delta;

typedef struct
{
	PyObject_HEAD		/* a pure abstract base clase */
} PyDateTime_TZInfo;

/* Apply for date, datetime, and datetimetz instances. */
#define PyDateTime_GET_YEAR(o)     ((((PyDateTime_Date*)o)->data[0] << 8) | \
                                     ((PyDateTime_Date*)o)->data[1])
#define PyDateTime_GET_MONTH(o)    (((PyDateTime_Date*)o)->data[2])
#define PyDateTime_GET_DAY(o)      (((PyDateTime_Date*)o)->data[3])

#define PyDateTime_DATE_GET_HOUR(o)        (((PyDateTime_DateTime*)o)->data[4])
#define PyDateTime_DATE_GET_MINUTE(o)      (((PyDateTime_DateTime*)o)->data[5])
#define PyDateTime_DATE_GET_SECOND(o)      (((PyDateTime_DateTime*)o)->data[6])
#define PyDateTime_DATE_GET_MICROSECOND(o) 		\
	((((PyDateTime_DateTime*)o)->data[7] << 16) |	\
         (((PyDateTime_DateTime*)o)->data[8] << 8)  |	\
          ((PyDateTime_DateTime*)o)->data[9])

/* Apply for time instances. */
#define PyDateTime_TIME_GET_HOUR(o)        (((PyDateTime_Time*)o)->data[0])
#define PyDateTime_TIME_GET_MINUTE(o)      (((PyDateTime_Time*)o)->data[1])
#define PyDateTime_TIME_GET_SECOND(o)      (((PyDateTime_Time*)o)->data[2])
#define PyDateTime_TIME_GET_MICROSECOND(o) 		\
	((((PyDateTime_Time*)o)->data[3] << 16) |	\
         (((PyDateTime_Time*)o)->data[4] << 8)  |	\
          ((PyDateTime_Time*)o)->data[5])

#define PyDate_Check(op) PyObject_TypeCheck(op, &PyDateTime_DateType)
#define PyDate_CheckExact(op) ((op)->ob_type == &PyDateTime_DateType)

#define PyDateTime_Check(op) PyObject_TypeCheck(op, &PyDateTime_DateTimeType)
#define PyDateTime_CheckExact(op) ((op)->ob_type == &PyDateTime_DateTimeType)

#define PyDateTimeTZ_Check(op) PyObject_TypeCheck(op, &PyDateTime_DateTimeTZType)
#define PyDateTimeTZ_CheckExact(op) ((op)->ob_type == &PyDateTime_DateTimeTZType)

#define PyTime_Check(op) PyObject_TypeCheck(op, &PyDateTime_TimeType)
#define PyTime_CheckExact(op) ((op)->ob_type == &PyDateTime_TimeType)

#define PyDelta_Check(op) PyObject_TypeCheck(op, &PyDateTime_DeltaType)
#define PyDelta_CheckExact(op) ((op)->ob_type == &PyDateTime_DeltaType)

#define PyTZInfo_Check(op) PyObject_TypeCheck(op, &PyDateTime_TZInfoType)
#define PyTZInfo_CheckExact(op) ((op)->ob_type == &PyDateTime_TZInfoType)

#endif
