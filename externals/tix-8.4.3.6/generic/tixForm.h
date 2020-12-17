
/*	$Id: tixForm.h,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 * tixForm.h --
 *
 *	Declares the internal functions and data types for the Tix Form
 *	geometry manager.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_FORM_H
#define _TIX_FORM_H

#define SIDE0	0
#define SIDE1	1

#define NEXT_SIDE(x) (!x)

#define SIDEX	0
#define SIDEY	1

#define AXIS_X	0
#define AXIS_Y	1

#define OPPO_SIDE0  1
#define OPPO_SIDE1  2
#define OPPO_ALL    3

#define PINNED_SIDE0  4
#define PINNED_SIDE1  8
#define PINNED_ALL   12

#define ATT_NONE		0
#define ATT_GRID		1
#define ATT_OPPOSITE		2
#define ATT_PARALLEL		3

/*
 * The following structures carry information about the client windows
 */
typedef union {
    int			grid;
    struct _FormInfo  * widget;
} Attachment;

typedef struct {
    int pcnt;		/* percentage anchor point */
    int disp;		/* displacement from the percentage anchor point*/
} Side;

typedef struct _FormInfo {
    Tk_Window		tkwin;
    struct _MasterInfo* master;		/* The master of this window */
    struct _FormInfo  * next;

    int			depend;		/* used to detect circular dependency*/

    /* These are specified by the user and set by the "tixForm" command
     */
    Attachment		att[2][2];	/* anchor of attachment */
    int			off[2][2];	/* offset of attachment */
    char		isDefault[2][2];/* Is this side a default attachment*/

    char		attType[2][2];	/* type of attachment
					   GRID or PIXEL*/
    int			pad[2][2];	/* value of padding */

    /* These values are calculated by the PinnClient() functions
     * and are used to calculated the required size of the master
     * inside CalculateMasterGeometry(), as well as the positions
     * of the clients inside ArrangeGeometry()
     */
    Side		side[2][2];
    int			sideFlags[2];

    /* These values are used to place the clients into the clients
     */
    int			posn[2][2];

    /* These things are for Spring'ing */
    int			spring[2][2];
    struct _FormInfo  * strWidget[2][2];
    int 		springFail[2];
    int			fill[2];
} FormInfo;


/*
 * The following structures carry information about the master windows
 */
typedef struct {
    unsigned int	isDeleted : 1;
    unsigned int	repackPending : 1;
} MasterFlags;

typedef struct _MasterInfo {
    Tk_Window		tkwin;
    struct _FormInfo  * client;
    struct _FormInfo  * client_tail;
    int			numClients;
    int			reqSize[2];
    int			numRequests;	/* This is used to detect
					 * whether two geometry managers
					 * are used to manage the same
					 * master window
					 */
    int			grids[2];
    MasterFlags		flags;
} MasterInfo;

/* tixFormMisc.c */


EXTERN int 		TixFm_Configure _ANSI_ARGS_((FormInfo *clientPtr,
			    Tk_Window topLevel,
			    Tcl_Interp* interp, int argc, CONST84 char **argv));

/* tixForm.c */
EXTERN FormInfo * 	TixFm_GetFormInfo _ANSI_ARGS_((Tk_Window tkwin,
			    int create));
EXTERN void 		TixFm_StructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent * eventPtr));
EXTERN void 		TixFm_AddToMaster _ANSI_ARGS_((MasterInfo *masterPtr,
			    FormInfo *clientPtr));
EXTERN void 		TixFm_DeleteMaster _ANSI_ARGS_((
			    MasterInfo *masterPtr));
EXTERN void		TixFm_FreeMasterInfo _ANSI_ARGS_((
			    ClientData clientData));
EXTERN FormInfo * 	TixFm_FindClientPtrByName _ANSI_ARGS_((
			    Tcl_Interp * interp, CONST84 char * name,
			    Tk_Window topLevel));
EXTERN void		TixFm_ForgetOneClient _ANSI_ARGS_((
			    FormInfo *clientPtr));
EXTERN void  		TixFm_Unlink _ANSI_ARGS_((FormInfo *clientPtr));
EXTERN void  		TixFm_UnlinkFromMaster _ANSI_ARGS_((
			    FormInfo *clientPtr));
#endif /* _TIX_FORM_H */
