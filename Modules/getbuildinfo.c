#include "Python.h"

#ifdef macintosh
#include "macbuildno.h"
#endif

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifndef DATE
#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "xx/xx/xx"
#endif
#endif

#ifndef TIME
#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "xx:xx:xx"
#endif
#endif

#ifndef BUILD
#define BUILD 0
#endif

#ifdef __VMS
#  ifdef __DECC
#    pragma extern_model save
#    pragma extern_model strict_refdef
extern long ctl$gl_imghdrbf;
#    pragma extern_model restore
#  endif

#  ifdef __ALPHA
#    define EIHD$L_IMGIDOFF 24
#    define EIHI$Q_LINKTIME  8
#    define _IMGIDOFF EIHD$L_IMGIDOFF
#    define _LINKTIME EIHI$Q_LINKTIME
#  else
#    define IHD$W_IMGIDOFF  6
#    define IHI$Q_LINKTIME 56
#    define _IMGIDOFF IHD$W_IMGIDOFF
#    define _LINKTIME IHI$Q_LINKTIME
#  endif /* __VMS */

long*
vms__get_linktime (void)
{
	long* al_imghdrbf;
	unsigned short* aw_imgidoff;
	unsigned short	  w_imgidoff;
	long* aq_linktime;
	unsigned char* ab_ihi;

	al_imghdrbf = &ctl$gl_imghdrbf;

	al_imghdrbf = (long *)*al_imghdrbf;
	al_imghdrbf = (long *)*al_imghdrbf;

	aw_imgidoff = (unsigned short *)
		((unsigned char *)al_imghdrbf + _IMGIDOFF);

	w_imgidoff = *aw_imgidoff;

	ab_ihi = (unsigned char *)al_imghdrbf + w_imgidoff;

	aq_linktime = (long *) (ab_ihi + _LINKTIME);

	return aq_linktime;
} /* vms__get_linktime (void) */
extern void vms__cvt_v2u_time (long * aq_vmstime, time_t * al_unixtime);
			    /* input		, output */
#endif /* __VMS */


const char *
Py_GetBuildInfo(void)
{
	static char buildinfo[50];
#ifdef __VMS
	time_t l_unixtime;

	vms__cvt_v2u_time(vms__get_linktime (), &l_unixtime );

	memset(buildinfo, 0, 40);
	sprintf(buildinfo, "#%d, %.24s", BUILD, ctime (&l_unixtime));
#else
	PyOS_snprintf(buildinfo, sizeof(buildinfo),
		      "#%d, %.20s, %.9s", BUILD, DATE, TIME);
#endif
	return buildinfo;
}
