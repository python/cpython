/*
** InterSLIP API.
*/

#include <Types.h>

/* States */
#define IS_IDLE		0
#define IS_WMODEM	1
#define IS_DIAL		2
#define IS_LOGIN	3
#define IS_RUN		4
#define IS_DISC		5

OSErr is_open();	/* Open InterSLIP driver (optional) */
OSErr is_connect();/* Connect */
OSErr is_disconnect();	/* Disconnect */
OSErr is_status(long *, long *, StringPtr *);	/* Get status, msg seq#, msg pointer */
OSErr is_getconfig(long *, long *, Str255 , Str255 , Str255 );	/* get config */
OSErr is_setconfig(long , long , Str255 , Str255 , Str255 );	/* set config */
