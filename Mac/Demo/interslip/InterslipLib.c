/*
** InterslipLib - Routines to talk to InterSLIP. Version 1.1, 31-Oct-1995.
**
**
** (c) Jack Jansen, CWI, 1995 <jack@cwi.nl>
*/

#include <Devices.h>

#include "InterslipLib.h"

static CntrlParam iopb;
static short refnum = -1;

OSErr is_open()
{
	if ( refnum >= 0 ) return 0;
	return OpenDriver("\p.InterSLIP", &refnum);
}

OSErr is_connect()
{
	iopb.ioCRefNum = refnum;
	iopb.ioVRefNum = refnum;
	iopb.ioCompletion = (UniversalProcPtr) 0;
	iopb.csCode = 2;
	return PBControlImmed((ParmBlkPtr)&iopb);
}

OSErr is_disconnect()
{
	iopb.ioCRefNum = refnum;
	iopb.ioVRefNum = refnum;
	iopb.ioCompletion = (UniversalProcPtr) 0;
	iopb.csCode = 3;
	return PBControlImmed((ParmBlkPtr)&iopb);
}

OSErr is_status(long *status, long *msgseqnum, StringPtr *msg)
{
	long *csp;
	OSErr err;
	
	iopb.ioCRefNum = refnum;
	iopb.ioVRefNum = refnum;
	iopb.ioCompletion = (UniversalProcPtr) 0;
	iopb.csCode = 4;
	if( err = PBControlImmed((ParmBlkPtr)&iopb) )
		return err;
	csp = (long *)&iopb.csParam;
	*status = csp[0];
	*msgseqnum = csp[1];
	*msg = (unsigned char *)csp[2];
	return 0;
}

OSErr is_getconfig(long *baudrate, long *flags, 
		Str255 idrvnam, Str255 odrvnam, Str255 cfgnam)
{
	long *csp;
	OSErr err;
	
	iopb.ioCRefNum = refnum;
	iopb.ioVRefNum = refnum;
	iopb.ioCompletion = (UniversalProcPtr) 0;
	iopb.csCode = 6;
	csp = (long *)&iopb.csParam;
	csp[2] = (long)idrvnam;
	csp[3] = (long)odrvnam;
	csp[4] = (long)cfgnam;
	if( err = PBControlImmed((ParmBlkPtr)&iopb) )
		return err;
	*baudrate = csp[0];
	*flags = csp[1];
	return 0;
}

OSErr is_setconfig(long baudrate, long flags, 
		Str255 idrvnam, Str255 odrvnam, Str255 cfgnam)
{
	long *csp;
	OSErr err;
	
	iopb.ioCRefNum = refnum;
	iopb.ioVRefNum = refnum;
	iopb.ioCompletion = (UniversalProcPtr) 0;
	iopb.csCode = 7;
	csp = (long *)&iopb.csParam;
	csp[0] = baudrate;
	csp[1] = flags;
	csp[2] = (long)idrvnam;
	csp[3] = (long)odrvnam;
	csp[4] = (long)cfgnam;
	return PBControlImmed((ParmBlkPtr)&iopb);
}

	
