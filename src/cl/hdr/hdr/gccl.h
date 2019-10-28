/*
** Copyright (c) 2009 Actian Corporation
*/

/**
** Name: gccl.h - GC definitions.
**
** Description:
**	Contains platform specific GC definitions.
**
** History:
**      11-Aug-09 (gordy)
**          Created.
**      14-Mar-2011 (Ralph Loen)  SIR 122536
**          Remove GC_HOSTNAME_MAX definition for VMS; rely on default instead.
*/

#ifndef GCCL_INCLUDED
#define GCCL_INCLUDED 1

#ifdef NT_GENERIC

/*
** Note: The following symbols could be defined 
** directly from the system defined symbols, but 
** it would require including a number of system 
** header files, such as windows.h.
*/

/*
** Max host name length comes from WinInet.h
** and the symbol INTERNET_MAX_HOST_NAME_LENGTH.
*/
#define	GC_HOSTNAME_MAX		256

/*
** Max user name length comes from Lmcons.h
** and the symbol UNLEN.
*/
#define	GC_USERNAME_MAX		256

#elif defined(VMS)

/*
** On VMS, root host names are constrained to 16, for compatibilty with
** DECNet and cluster node names.  However, TCP provides no such limits
** on the FQDN of the host name.  Go with the default definition.
*/
/* #define	GC_HOSTNAME_MAX		(default) */

/*
** Documented max for SYS$GETUAI() is 12.
** Provide the standard Ingres max symbol length.
*/
#define	GC_USERNAME_MAX		32

#else	/* Unix */

#include <limits.h>

/*
** Posix provides HOST_NAME_MAX symbol as 
** maximum length returned by gethostname().
*/
#ifdef HOST_NAME_MAX
#define	GC_HOSTNAME_MAX		HOST_NAME_MAX
#endif

#endif	/* Unix */

/*
** Default maximum hostname length
*/
#ifndef GC_HOSTNAME_MAX
#define	GC_HOSTNAME_MAX		256
#endif


/*
** Default maximum username length
*/
#ifndef GC_USERNAME_MAX
#define	GC_USERNAME_MAX		256
#endif

#endif  /* GCCL_INCLUDED */

