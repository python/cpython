/*
 * Prototypes for mactcpglue routines and includes/structures needed
 * by those.
 *
 * Jack Jansen, CWI, 1994.
 *
 * Adapted from mac socket library, which has in turn adapted from ncsa telnet.
 * Original authors: Tom Milligan, Charlie Reiman
 */
  
#include <MacTCPCommonTypes.h>
#include <GetMyIPAddr.h>
#include <TCPPB.h>
#include <UDPPB.h>
#include <AddressXlation.h>

#ifndef __MWERKS__
#define TCPIOCompletionUPP TCPIOCompletionProc
#define TCPNotifyUPP TCPNotifyProc
#define UDPIOCompletionUPP UDPIOCompletionProc
#define UDPNotifyUPP UDPNotifyProc
#define NewTCPIOCompletionProc(x) (x)
#define NewTCPNotifyProc(x) (x)
#define NewUDPIOCompletionProc(x) (x)
#define NewUDPNotifyProc(x) (x)
#endif /* __MWERKS__ */

#if defined(powerc) || defined (__powerc)
#pragma options align=mac68k
#endif

typedef	struct	miniwds
	{
	unsigned short length;
	char * ptr;
	unsigned short terminus;	/* must be zero'd for use */
	} miniwds;

#if defined(powerc) || defined(__powerc)
#pragma options align=reset
#endif


OSErr xOpenDriver(void);
OSErr xPBControl(TCPiopb *pb, TCPIOCompletionUPP completion);
OSErr xPBControlSync(TCPiopb *pb);
OSErr xTCPCreate(int buflen, TCPNotifyUPP notify, void *udp, TCPiopb *pb);
OSErr xTCPPassiveOpen(TCPiopb *pb, short port, TCPIOCompletionUPP completion, void *udp);
OSErr xTCPActiveOpen(TCPiopb *pb, short port, long rhost, short rport, TCPIOCompletionUPP completion);
OSErr xTCPRcv(TCPiopb *pb, char *buf, int buflen, int timeout, TCPIOCompletionUPP completion);
OSErr xTCPNoCopyRcv(TCPiopb *,rdsEntry *,int,int,TCPIOCompletionUPP);
OSErr xTCPBufReturn(TCPiopb *pb,rdsEntry *rds,TCPIOCompletionUPP completion);
OSErr xTCPSend(TCPiopb *pb, wdsEntry *wds, Boolean push, Boolean urgent, TCPIOCompletionUPP completion);
OSErr xTCPClose(TCPiopb *pb,TCPIOCompletionUPP completion);
OSErr xTCPAbort(TCPiopb *pb);
OSErr xTCPRelease(TCPiopb *pb);

OSErr xUDPCreate(UDPiopb *pb,int buflen,ip_port *port, UDPNotifyUPP asr, void *udp);
OSErr xUDPRead(UDPiopb *pb,int timeout, UDPIOCompletionUPP completion);
OSErr xUDPBfrReturn(UDPiopb *pb, char *buff);
OSErr xUDPWrite(UDPiopb *pb,ip_addr host,ip_port port,miniwds *wds,
		UDPIOCompletionUPP completion);
OSErr xUDPRelease(UDPiopb *pb);

ip_addr xIPAddr(void);
long xNetMask(void);
unsigned short xMaxMTU(void);

