/*
 * Glue routines for mactcp module.
 * Jack Jansen, CWI, 1994.
 *
 * Adapted from mactcp socket library, which was in turn
 * adapted from ncsa telnet code.
 *
 * Original authors: Tom Milligan, Charlie Reiman
 */
 
# include <Memory.h>
# include <Files.h>
# include <Errors.h>

#include "tcpglue.h"
#include <Devices.h>

static short driver = 0;

#ifndef __powerc
/*
 * Hack fix for MacTCP 1.0.X bug
 *
 * This hack doesn't work on the PPC. But then, people with new machines
 * shouldn't run ancient buggy software. -- Jack.
 */
 
pascal char *ReturnA5(void) = {0x2E8D};
#endif /* !__powerc */

OSErr xOpenDriver() 
{ 
	if (driver == 0) 
	{ 
		ParamBlockRec pb; 
		OSErr io; 
		
		pb.ioParam.ioCompletion = 0L; 
		pb.ioParam.ioNamePtr = "\p.IPP"; 
		pb.ioParam.ioPermssn = fsCurPerm; 
		io = PBOpen(&pb,false); 
		if (io != noErr) 
			return(io); 
		driver = pb.ioParam.ioRefNum; 
	}
	return noErr;
}

/*
 * create a TCP stream
 */
OSErr xTCPCreate(buflen,notify,udp, pb) 
	int buflen;
	TCPNotifyUPP notify;
	void *udp;
	TCPiopb *pb;
{	
	pb->ioCRefNum = driver;
	pb->csCode = TCPCreate;
	pb->csParam.create.rcvBuff = (char *)NewPtr(buflen);
	pb->csParam.create.rcvBuffLen = buflen;
	pb->csParam.create.notifyProc = notify;
	pb->csParam.create.userDataPtr = udp;
	return (xPBControlSync(pb));
}


/*
 * start listening for a TCP connection
 */
OSErr xTCPPassiveOpen(TCPiopb *pb, short port, TCPIOCompletionUPP completion,
	void *udp)
{
	if (driver == 0)
		return(invalidStreamPtr);

	pb->ioCRefNum = driver;
	pb->csCode = TCPPassiveOpen;
	pb->csParam.open.validityFlags = timeoutValue | timeoutAction;
	pb->csParam.open.ulpTimeoutValue = 255 /* seconds */;
	pb->csParam.open.ulpTimeoutAction = 0 /* 1:abort 0:report */;
	pb->csParam.open.commandTimeoutValue = 0 /* infinity */;
	pb->csParam.open.remoteHost = 0;
	pb->csParam.open.remotePort = 0;
	pb->csParam.open.localHost = 0;
	pb->csParam.open.localPort = port;
	pb->csParam.open.dontFrag = 0;
	pb->csParam.open.timeToLive = 0;
	pb->csParam.open.security = 0;
	pb->csParam.open.optionCnt = 0;
	pb->csParam.open.userDataPtr = udp;
	return (xPBControl(pb,completion));
}

/*
 * connect to a remote TCP
 */
OSErr xTCPActiveOpen(TCPiopb *pb, short port, long rhost, short rport, 
	TCPIOCompletionUPP completion)
{
	if (driver == 0)
		return(invalidStreamPtr);

	pb->ioCRefNum = driver;
	pb->csCode = TCPActiveOpen;
	pb->csParam.open.validityFlags = timeoutValue | timeoutAction;
	pb->csParam.open.ulpTimeoutValue = 60 /* seconds */;
	pb->csParam.open.ulpTimeoutAction = 1 /* 1:abort 0:report */;
	pb->csParam.open.commandTimeoutValue = 0;
	pb->csParam.open.remoteHost = rhost;
	pb->csParam.open.remotePort = rport;
	pb->csParam.open.localHost = 0;
	pb->csParam.open.localPort = port;
	pb->csParam.open.dontFrag = 0;
	pb->csParam.open.timeToLive = 0;
	pb->csParam.open.security = 0;
	pb->csParam.open.optionCnt = 0;
	return (xPBControl(pb,completion));
}

OSErr xTCPNoCopyRcv(pb,rds,rdslen,timeout,completion) 
	TCPiopb *pb;
	rdsEntry *rds; 
	int rdslen;
	int	timeout;
	TCPIOCompletionUPP completion;
{
	
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPNoCopyRcv;
	pb->csParam.receive.commandTimeoutValue = timeout; /* seconds, 0 = blocking */
	pb->csParam.receive.rdsPtr = (Ptr)rds;
	pb->csParam.receive.rdsLength = rdslen;
	return (xPBControl(pb,completion));
}

OSErr xTCPBufReturn(TCPiopb *pb,rdsEntry *rds,TCPIOCompletionUPP completion)
	{
	pb->ioCRefNum = driver;
	pb->csCode = TCPRcvBfrReturn;
	pb->csParam.receive.rdsPtr = (Ptr)rds;
	
	return (xPBControl(pb,completion));
	}
	
/*
 * send data
 */
OSErr xTCPSend(TCPiopb *pb, wdsEntry *wds, Boolean push, Boolean urgent, TCPIOCompletionUPP completion)
{
	if (driver == 0)
		return invalidStreamPtr;
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPSend;
	pb->csParam.send.validityFlags = timeoutValue | timeoutAction;
	pb->csParam.send.ulpTimeoutValue = 60 /* seconds */;
	pb->csParam.send.ulpTimeoutAction = 0 /* 0:abort 1:report */;
	pb->csParam.send.pushFlag = push;
	pb->csParam.send.urgentFlag = urgent;
	pb->csParam.send.wdsPtr = (Ptr)wds;
	return (xPBControl(pb,completion));
}


/*
 * close a connection
 */
OSErr xTCPClose(TCPiopb *pb,TCPIOCompletionUPP completion) 
{
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPClose;
	pb->csParam.close.validityFlags = timeoutValue | timeoutAction;
	pb->csParam.close.ulpTimeoutValue = 60 /* seconds */;
	pb->csParam.close.ulpTimeoutAction = 1 /* 1:abort 0:report */;
	return (xPBControl(pb,completion));
}

/*
 * abort a connection
 */
OSErr xTCPAbort(TCPiopb *pb) 
{
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPAbort;
	return (xPBControlSync(pb));
}

/*
 * close down a TCP stream (aborting a connection, if necessary)
 */
OSErr xTCPRelease(pb) 
	TCPiopb *pb;
{
	OSErr io;
	
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPRelease;
	io = xPBControlSync(pb);
	if (io == noErr)
		DisposPtr(pb->csParam.create.rcvBuff); /* there is no release pb */
	return(io);
}

#if 0

int
xTCPBytesUnread(sp) 
	SocketPtr sp;
{
	TCPiopb	*pb;
	OSErr io;
	
	if (!(pb = sock_fetch_pb(sp)))
		return -1;		/* panic */
	
	if (driver == 0)
		return(-1);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPStatus;
	io = xPBControlSync(pb);
	if (io != noErr)
		return(-1);
	return(pb->csParam.status.amtUnreadData);
}

int
xTCPBytesWriteable(sp)
	SocketPtr sp;
	{
	TCPiopb *pb;
	OSErr	io;
	long	amount;
	
	if (!(pb = sock_fetch_pb(sp)))
		return -1;		/* panic */
	
	if (driver == 0)
		return(-1);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPStatus;
	io = xPBControlSync(pb);
	if (io != noErr)
		return(-1);
	amount = pb->csParam.status.sendWindow-pb->csParam.status.amtUnackedData;
	if (amount < 0)
		amount = 0;
	return amount;
	}
	
int xTCPWriteBytesLeft(SocketPtr sp)
	{
	TCPiopb *pb;
	OSErr	io;
	
	if (!(pb = sock_fetch_pb(sp)))
		return -1;		/* panic */
	
	if (driver == 0)
		return(-1);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPStatus;
	io = xPBControlSync(pb);
	if (io != noErr)
		return(-1);
	return (pb->csParam.status.amtUnackedData);
	}
#endif

OSErr xTCPStatus(TCPiopb *pb, TCPStatusPB **spb)
	{
	OSErr io;
	
	if (driver == 0)
		return(-1);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPStatus;
	io = xPBControlSync(pb);
	if (io == noErr)
		*spb = &pb->csParam.status;
	return(io);
	}


/*
 * create a UDP stream, hook it to a socket.
 */
OSErr xUDPCreate(UDPiopb *pb,int buflen,ip_port *port, UDPNotifyUPP asr, void *udp)
	{	
	OSErr   io;
	
	pb->ioCRefNum = driver;
	pb->csCode = UDPCreate;
	pb->csParam.create.rcvBuff = (char *)NewPtr(buflen);
	pb->csParam.create.rcvBuffLen = buflen;
	pb->csParam.create.notifyProc = asr;
	pb->csParam.create.userDataPtr = udp;
	pb->csParam.create.localPort = *port;
	if ( (io = xPBControlSync( (TCPiopb *)pb ) ) != noErr)
		return io;
		
	*port = pb->csParam.create.localPort;
	return noErr;
	}

/*
 * ask for incoming data
 */
OSErr xUDPRead(UDPiopb *pb, int timeout, UDPIOCompletionUPP completion) 
	{
	
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = UDPRead;
	pb->csParam.receive.timeOut = timeout;
	pb->csParam.receive.secondTimeStamp = 0/* must be zero */;
	return (xPBControl ( (TCPiopb *)pb, (TCPIOCompletionUPP)completion ));
	}

OSErr xUDPBfrReturn(UDPiopb *pb, char *buff) 
	{
	
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = UDPBfrReturn;
	pb->csParam.receive.rcvBuff = buff;
	return ( xPBControl( (TCPiopb *)pb,(TCPIOCompletionUPP)-1 ) );
	}

/*
 * send data
 */
OSErr xUDPWrite(UDPiopb	*pb,ip_addr host,ip_port port,miniwds *wds,
		UDPIOCompletionUPP completion) 
	{
		
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = UDPWrite;
	pb->csParam.send.remoteHost = host;
	pb->csParam.send.remotePort = port;
	pb->csParam.send.wdsPtr = (Ptr)wds;
	pb->csParam.send.checkSum = true;
	pb->csParam.send.sendLength = 0/* must be zero */;
	return (xPBControl( (TCPiopb *)pb, (TCPIOCompletionUPP)completion));
	}

/*
 * close down a UDP stream (aborting a read, if necessary)
 */
OSErr xUDPRelease(UDPiopb *pb) {
	OSErr io;

	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = UDPRelease;
	io = xPBControlSync( (TCPiopb *)pb );
	if (io == noErr) {
		DisposPtr(pb->csParam.create.rcvBuff);
		}
	return(io);
	}

ip_addr xIPAddr(void) 
{
	struct GetAddrParamBlock pbr;
	OSErr io;
	
	pbr.ioCRefNum = driver;
	pbr.csCode = ipctlGetAddr;
	io = xPBControlSync( (TCPiopb *)&pbr );
	if (io != noErr)
		return(0);
	return(pbr.ourAddress);
}

long xNetMask() 
{
	struct GetAddrParamBlock pbr;
	OSErr io;
	
	pbr.ioCRefNum = driver;
	pbr.csCode = ipctlGetAddr;
	io = xPBControlSync( (TCPiopb *)&pbr);
	if (io != noErr)
		return(0);
	return(pbr.ourNetMask);
}

unsigned short xMaxMTU()
{
	struct UDPiopb pbr;
	OSErr io;
	
	pbr.ioCRefNum = driver;
	pbr.csCode = UDPMaxMTUSize;
	pbr.csParam.mtu.remoteHost = xIPAddr();
	io = xPBControlSync( (TCPiopb *)&pbr );
	if (io != noErr)
		return(0);
	return(pbr.csParam.mtu.mtuSize);
}

OSErr xPBControlSync(TCPiopb *pb) 
{ 
	(pb)->ioCompletion = 0L; 
	return PBControl((ParmBlkPtr)(pb),false); 
}

#pragma segment SOCK_RESIDENT

OSErr xTCPRcv(pb,buf,buflen,timeout,completion) 
	TCPiopb *pb;
	Ptr buf; 
	int buflen;
	int	timeout;
	TCPIOCompletionUPP completion;
{
	
	if (driver == 0)
		return(invalidStreamPtr);
	
	pb->ioCRefNum = driver;
	pb->csCode = TCPRcv;
	pb->csParam.receive.commandTimeoutValue = timeout; /* seconds, 0 = blocking */
	pb->csParam.receive.rcvBuff = buf;
	pb->csParam.receive.rcvBuffLen = buflen;
	return (xPBControl(pb,completion));
}

OSErr xPBControl(TCPiopb *pb,TCPIOCompletionUPP completion) 
{ 
#ifndef __MWERKS__
	pb->ioNamePtr = ReturnA5();
#endif
	
	if (completion == 0L) 
	{ 
		(pb)->ioCompletion = 0L; 
		return(PBControl((ParmBlkPtr)(pb),false));		/* sync */
	} 
	else if (completion == (TCPIOCompletionUPP)-1L) 
	{ 
		(pb)->ioCompletion = 0L; 
		return(PBControl((ParmBlkPtr)(pb),true));		/* async */
	} 
	else 
	{  
		(pb)->ioCompletion = completion;
		return(PBControl((ParmBlkPtr)(pb),true));		/* async */
	} 
}

