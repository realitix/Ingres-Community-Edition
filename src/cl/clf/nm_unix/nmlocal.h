/*
** Copyright (c) 1985, 2010 Actian Corporation
*/

/*
** NM.h -- Name header
**
** History: Log:	nmlocal.h,v
**	3-15-83   - Written (jen)
**
**	Revision 1.2  86/05/23  02:51:57  daveb
**	Update for cached symbols.
**
**	Revision 30.2  85/09/17  11:11:52  roger
**	NMgtUserAt() is now a macro.
**		
**	11-Mar-89 (GordonW)
**	    Added extern to NMtime for symbol table mdoification time
**	25-jan-1989 (greg)
**	    Removed obsolete junk.
**	16-aug-95 (hanch04)
**	    Removed NMgtIngAt, now defined in nm.h
**      12-Dec-95 (fanra01)
**          getenv function defined as FUNC_EXTERN.
**          Changed extern to GLOBALREF.
**          Changed extern funtions to FUNC_EXTERN.
**	03-jun-1996 (canor01)
**	    Moved definition of NM_static structure here from nmfiles.c.
**	08-jan-2004 (somsa01)
**	    Added storage of symbol.bak, symbol.log, and symbol.lck locations
**	    in II_NM_STATIC.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	29-mar-2004 (somsa01)
**	    Added storage of symbol.bak and symbol.log locations in
**	    II_NM_STATIC.
**	31-Aug-2004 (jenjo02)
**	    Added stuff for symbol.lck file.
**      16-Feb-2010 (hanje04)
**          SIR 123296
**          Add cond_LSB_BUILD option for LSB compliant builds. This redefines
**          the locations for files depending on their type:
**              SYS:    Fix II_SYSTEM for LSB builds
**              FILES:  Static configuration and message files (II_CONFIG)
**              ADMIN:  Dynamic configuration files (config.dat symbol.tbl)
**                      Any files opened for writing or created by the server
**                      or other processes now live in the this location.
**                      II_ADMIN
**              LOG:    Any and all .log files (errlog.log etc.)
**              LIB:    Libraries
**	22-Mar-2010 (hanje04)
**	    SIR 123296
**	    Add buffer for /usr/bin (UBIN)
**	14-Jun-2010 (whiro01)
**	    Take out "NMstIngAt" which conflicts (in C++) with the def in
**	    nmcl.h
**	15-nov-2010 (stephenb)
**	    Correctly proto forward funcs.
**	21-Sep-2011 (hanje04)
**	    Bug 123079
**	    Move SYM structure to clf!hdr_unix_win nmsym.h so that it can
**	    be accessed by the new common!toolsapi code.
**	19-Oct-2011 (kiria01) b125861
**	    Tidy up windows NM exports
*/
 
# include	<nmsym.h>
# include	<tm.h>
# include	<mu.h>
 
# define	MAXLINE		256

FUNC_EXTERN char	*getenv();
 
# define	NMgtUserAt(A,B)	(*B = getenv(A))
 
typedef struct _II_NM_STATIC
{
        char            Locbuf[ MAX_LOC + 1 ];  /* for NMloc() */
        bool            ReadFailed;             /* for NMreadsyms */
        LOCATION        Sysloc;                 /* II_SYSTEM */
        char            Sysbuf[ MAX_LOC + 1 ];  /* II_SYSTEM */
        LOCATION        Admloc;                 /* II_ADMIN */
        char            Admbuf[ MAX_LOC + 1 ];  /* II_ADMIN */
	LOCATION        Bakloc;                 /* II_CONFIG / II_ADMIN */
	char            Bakbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
	LOCATION        Logloc;                 /* II_CONFIG / II_ADMIN */
	char            Logbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
	LOCATION        Lckloc;                 /* II_CONFIG / II_ADMIN */
	char            Lckbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
# ifdef conf_LSB_BUILD
# define NM_LSB_SYSDIR "/usr/libexec"
# define NM_LSB_ADMDIR "/var/lib/ingres/files"
# define NM_LSB_CFGDIR "/usr/share/ingres/files"
	LOCATION	Cfgloc;			/* II_CONFIG */
	char		Cfgbuf[ MAX_LOC +1 ];	/* II_CONFIG */
# define NM_LSB_LOGDIR "/var/log/ingres"
	LOCATION	IILogloc;		/* II_LOG */
	char		IILogbuf[ MAX_LOC + 1 ];	/* II_LOG */
# define NM_LSB_LIBDIR "/usr/lib/ingres"
	LOCATION	Libloc;			/* /usr/lib/ingres */
	char		Libbuf[ MAX_LOC +1 ];	/* /usr/lib/ingres */
# define NM_LSB_UBINDIR "/usr/bin"
	LOCATION	Ubinloc;		/* /usr/bin */
	char		Ubinbuf[ MAX_LOC +1 ];	/* /usr/bin */
# endif
        bool            Init;                   /* NM_initsym called */
	bool		SymIsLocked;		/* symbol.lck held */
        MU_SEMAPHORE    Sem;                    /* semaphore */
} II_NM_STATIC;

#if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
#if 0
believed not referenced outside of DLL
GLOBALDLLREF II_NM_STATIC NM_static;
GLOBALDLLREF SYSTIME	NMtime;		/* modification time */
GLOBALDLLREF LOCATION	NMSymloc;	/* location of symbol.tbl */
GLOBALDLLREF LOCATION	NMBakSymloc;	/* location of symbol.bak */
GLOBALDLLREF LOCATION	NMLogSymloc;	/* location of symbol.log */
GLOBALDLLREF LOCATION	NMLckSymloc;	/* location of symbol.lck */
#endif
#else
# ifndef __IN_NMDATA_C__
GLOBALREF II_NM_STATIC	NM_static;
GLOBALREF SYSTIME	NMtime;		/* modification time */
GLOBALREF LOCATION	NMSymloc;	/* location of symbol.tbl */
GLOBALREF LOCATION	NMBakSymloc;	/* location of symbol.bak */
GLOBALREF LOCATION	NMLogSymloc;	/* location of symbol.log */
GLOBALREF LOCATION	NMLckSymloc;	/* location of symbol.lck */
# endif
#endif
 
/* Function declarations */
 
FUNC_EXTERN FILE	*NMopensyms(char *);
FUNC_EXTERN STATUS	NMaddsym(register char 	*name,
				 register char	*value,
				 register SYM	*endsp);
FUNC_EXTERN STATUS	NMreadsyms(void);
FUNC_EXTERN STATUS	NMwritesyms(void);
FUNC_EXTERN STATUS	NMflushIng(void);
FUNC_EXTERN STATUS	NMlocksyms(void);
FUNC_EXTERN VOID	NMunlocksyms(void);
FUNC_EXTERN STATUS	NM_initsym(void);
FUNC_EXTERN STATUS	NMzapIngAt(register char *name);

# define	NULL_STRING	""
# define	NULL_CHR	1
# define	NULL_NL		2
# define	NULL_NL_NULL	3
 
/* end of private nm.h */
