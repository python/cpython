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

typedef	struct	miniwds
	{
	unsigned short length;
	char * ptr;
	unsigned short terminus;	/* must be zero'd for use */
	} miniwds;


OSErr xOpenDriver(void);
OSErr xPBControl(TCPiopb *pb, TCPIOCompletionProc completion);
OSErr xPBControlSync(TCPiopb *pb);
OSErr xTCPCreate(int buflen, TCPNotifyProc notify, void *udp, TCPiopb *pb);
OSErr xTCPPassiveOpen(TCPiopb *pb, short port, TCPIOCompletionProc completion, void *udp);
OSErr xTCPActiveOpen(TCPiopb *pb, short port, long rhost, short rport, TCPIOCompletionProc completion);
OSErr xTCPRcv(TCPiopb *pb, char *buf, int buflen, int timeout, TCPIOCompletionProc completion);
OSErr xTCPNoCopyRcv(TCPiopb *,rdsEntry *,int,int,TCPIOCompletionProc);
OSErr xTCPBufReturn(TCPiopb *pb,rdsEntry *rds,TCPIOCompletionProc completion);
OSErr xTCPSend(TCPiopb *pb, wdsEntry *wds, Boolean push, Boolean urgent, TCPIOCompletionProc completion);
OSErr xTCPClose(TCPiopb *pb,TCPIOCompletionProc completion);
OSErr xTCPAbort(TCPiopb *pb);
OSErr xTCPRelease(TCPiopb *pb);

OSErr xUDPCreate(UDPiopb *pb,int buflen,ip_port *port, UDPNotifyProc asr, void *udp);
OSErr xUDPRead(UDPiopb *pb,int timeout, UDPIOCompletionProc completion);
OSErr xUDPBfrReturn(UDPiopb *pb, char *buff);
OSErr xUDPWrite(UDPiopb *pb,ip_addr host,ip_port port,miniwds *wds,
		UDPIOCompletionProc completion);
OSErr xUDPRelease(UDPiopb *pb);

ip_addr xIPAddr(void);
long xNetMask(void);
unsigned short xMaxMTU(void);

