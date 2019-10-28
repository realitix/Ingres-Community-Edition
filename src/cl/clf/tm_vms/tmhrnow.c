/*
** Copyright (c) 2009 Actian Corporation 
*/
#include <compat.h>
#include <tm.h>
#include <tmtz.h>
#include <gen64def.h>
#include <ssdef.h>
#include <starlet.h>

FUNC_EXTERN long int TMconv(struct _generic_64 *tm);

/*
** Name: TMhrnow
**
** Definition:
**      Get the current system time in high resolution (currently defined
**      to be a value in seconds and nanoseconds). On solaris, this function can
**      simply be defined to be clock_gettime() since the system structure,
**      timespec, actually matches the definition of HRSYSTIME, and can be 
**	defined to look the same. On some platforms, a TMhrnow() function may 
**      really need to be written, and should be placed in cl!clf!tm time.c
**      HRSYSTIME is defined to be:
**      struct HRSYSTIME
**      {
**          i4     tv_sec;         ** number of secs since 1-jan-1970 **
**          i4     tv_nsec;        ** nanoseconds **
**      };
**
**      Note: platforms that define this function to be a system function
**      should avoid declaring a prototype for it in tm.h, since this will be
**      macro'd out and cause syntax errors in the compile, and anyway it should
**      already be prototyped in the included system header. If you do need to
**      write a real version of this function, make sure you exclude the 
**      platforms that have it defined to a system call from any prototype you
**      add to tm.h
**
** History:
**      14-jan-2009 (joea)
**          Created.
**      07-Apr-2011 (horda03) b125249
**          Use clock_gettime if TMHRNOW_WRAPPED_CLOCKGETTIME defined, or
**          use TMet to remove need to massage local time into GMT using
**          the Ingres TZ (as it may have a different setting to the OS).
*/

#ifdef TMHRNOW_WRAPPED_CLOCKGETTIME

#   undef TMhrnow

#endif


STATUS
TMhrnow( HRSYSTIME *stime)
{
#ifdef TMHRNOW_WRAPPED_CLOCKGETTIME

    return clock_gettime( CLOCK_REALTIME, stime );

#else
    SYSTIME cur_syst;

    /* TMet() on VMS returns a unique time, so no need
    ** to massage the stime value to make it unique, just
    ** need to convert the msecs to nsecs.
    */
    TMet( &cur_syst );

    stime->tv_sec = cur_syst.-M_secs;
    stime->tv_nsec = cur_syst.TM_msecs * NANO_PER_MILLI;
#endif

    return OK;
}
