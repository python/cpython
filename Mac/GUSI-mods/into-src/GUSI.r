/*********************************************************************
Project	:	GUSI				-	Grand Unified Socket Interface
File		:	GUSI.r			-	Include this
Author	:	Matthias Neeracher
Language	:	MPW Rez 3.0

$Log$
Revision 1.1  1998/08/18 14:52:37  jack
Putting Python-specific GUSI modifications under CVS.

Revision 1.3  1994/12/30  19:33:34  neeri
Enlargened message box for select folder dialog.

Revision 1.2  1994/08/10  00:34:18  neeri
Sanitized for universal headers.

Revision 1.1  1994/02/25  02:12:04  neeri
Initial revision

Revision 0.5  1993/05/21  00:00:00  neeri
suffixes

Revision 0.4  1993/01/31  00:00:00  neeri
Daemon

Revision 0.3  1993/01/03  00:00:00  neeri
autoSpin

Revision 0.2  1992/09/24  00:00:00  neeri
Don't include CKID, create GUSIRsrc_P.h

Revision 0.1  1992/07/13  00:00:00  neeri
.rsrc

*********************************************************************/

#include "Types.r"
#include "GUSIRsrc_P.h"

include "GUSI.rsrc" not 'ckid';

/* Define a resource ('GU…I', GUSIRsrcID) to override GUSI defaults 		
	To avoid having to change the Rez file every time I introduce another
	feature, the preprocessor variable GUSI_PREF_VERSION by default keeps
	everything compatible with version 1.0.2. Just define GUSI_PREF_VERSION
	to be the version you want to use.
*/

#ifndef GUSI_PREF_VERSION
#define GUSI_PREF_VERSION '0102'
#endif

type 'GU…I' {
	literal longint	text 	=	'TEXT';	/* Type for creat'ed files 				*/
	literal longint	mpw	=	'MPS ';	/* Creator for creat'ed files  			*/
	byte 		noAutoSpin, autoSpin;		/* Automatically spin cursor ?			*/
#if GUSI_PREF_VERSION > '0102'
	boolean 	useChdir, dontUseChdir;		/* Use chdir() ?							*/
	boolean	approxStat, accurateStat;	/* statbuf.st_nlink = # of subdirectories ? */
#if GUSI_PREF_VERSION >= '0181'
	boolean	noDelayConsole, DelayConsole;	/* Delay opening console window until needed? */
	fill		bit[1];
#else
	boolean	noTCPDaemon, isTCPDaemon;	/* Inetd client ?							*/
	boolean	noUDPDaemon, isUDPDaemon;
#endif
#if GUSI_PREF_VERSION >= '0150'
#if GUSI_PREF_VERSION >= '0181'
	boolean	wantAppleEvents, noAppleEvents; /* Always solicit AppleEvents */
#else
	boolean	noConsole, hasConsole;		/* Are we providing our own dev:console ? (Obsolete) */
#endif
#if GUSI_PREF_VERSION >= '0180'
	boolean	autoInitGraf, noAutoInitGraf;	/* Automatically do InitGraf ? */
	boolean	exclusiveOpen, sharedOpen;	/* Shared open() ? 							*/
	boolean	noSigPipe, sigPipe;			/* raise SIGPIPE on write to closed PIPE */
#else
	fill 		bit[3];
#endif
#else
	fill 		bit[4];
#endif
	literal longint = GUSI_PREF_VERSION;
#if GUSI_PREF_VERSION >= '0120'
	integer = $$Countof(SuffixArray);
	wide array SuffixArray {
			literal longint;					/* Suffix of file */
			literal longint;					/* Type for file */
			literal longint;					/* Creator for file */
	};
#endif
#endif
};

type 'TMPL' {
	wide array {
		pstring;
		literal longint;
	};
};

resource 'TMPL' (GUSIRsrcID, "GU…I") {
	{
		"Type of created files",		'TNAM',
		"Creator of created files",	'TNAM',
		"Automatically spin cursor", 	'DBYT',
#if GUSI_PREF_VERSION > '0102'
		"Not using chdir()",				'BBIT',
		"Accurate stat()",				'BBIT',
		"TCP daemon",						'BBIT',
		"UDP daemon",						'BBIT',
#if GUSI_PREF_VERSION >= '0150'
		"Own Console",						'BBIT',
#else
		"Reserved",							'BBIT',
#endif
#if GUSI_PREF_VERSION >= '0180'
		"Don't initialize QuickDraw",	'BBIT',
		"Open files shared",				'BBIT',
		"Raise SIGPIPE",					'BBIT',
#else
		"Reserved",							'BBIT',
		"Reserved",							'BBIT',
		"Reserved",							'BBIT',
#endif
		"Version (don't change)",		'TNAM',		
#if GUSI_PREF_VERSION >= '0120'
		"NumSuffices",						'OCNT',
		"*****",								'LSTC',
		"Suffix",							'TNAM',
		"Type for suffix",				'TNAM',
		"Creator for suffix",			'TNAM',
		"*****",								'LSTE',
#endif
#endif		
	}
};

resource 'DLOG' (GUSIRsrcID, "Get Directory") {
	{0, 0, 217, 348}, 
	dBoxProc, 
	invisible, 
	noGoAway, 
	0x0, 
	10240, 
	"",
	alertPositionMainScreen
};

resource 'DITL' (GUSIRsrcID, "Get Directory") {
	{
		{ 142, 256,  160, 336},	Button		{enabled,"Open"},
		{1152,  59, 1232,  77},	Button		{enabled,"Hidden"},
		{ 193, 256,  211, 336},	Button		{enabled,"Cancel"},
		{  43, 232,   63, 347},	UserItem		{disabled},
		{  72, 256,   90, 336},	Button		{enabled,"Eject"},
		{  97, 256,  115, 336},	Button		{enabled,"Drive"},
		{  43,  12,  189, 230},	UserItem		{enabled},
		{  43, 229,  189, 246},	UserItem		{enabled},
		{ 128, 252,  129, 340},	UserItem		{disabled},
		{1044,  20, 1145, 116},	StaticText	{disabled,""},
		{ 167, 256,  185, 336},	Button		{enabled,"Directory"},
		{   0,  30,   18, 215},	Button		{enabled,"Select Current Directory:"},
		{ 200,  20, 1145, 222},	StaticText	{disabled,"Select a Folder"}
	}
};


