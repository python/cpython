/*
** Configuration file for dynamically loaded cfm68k/ppc PythonCore,
** interpreter and Applet.
**
** Note: enabling the switches below is not enough to enable the
** specific features, you may also need different sets of sources.
*/

#define USE_GUSI		/* Stdio implemented with GUSI */
/* #define USE_TOOLBOX		/* Include toolbox modules in core Python */
/* #define USE_QT		/* Include quicktime modules in core Python */
/* #define USE_WASTE		/* Include waste module in core Python */
/* #define USE_MACSPEECH	/* Include macspeech module in core Python */
/* #define USE_IMG		/* Include img modules in core Python */
/* #define USE_MACCTB		/* Include ctb module in core Python */
/* #define USE_STDWIN		/* Include stdwin module in core Python */
/* #define USE_MACTCP		/* Include mactcp (*not* socket) modules in core */
/* #define USE_TK		/* Include _tkinter module in core Python */
/* #define MAC_TCL		/* This *must* be on if USE_TK is on */
#define USE_MAC_SHARED_LIBRARY		/* Enable code to add shared-library resources */
#define USE_MAC_APPLET_SUPPORT		/* Enable code to run a PYC resource */
