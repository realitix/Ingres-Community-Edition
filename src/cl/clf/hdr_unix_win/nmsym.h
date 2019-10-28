#ifndef __NMSYM_H_INC__
#define __NMSYM_H_INC__
/*
** Copyright (c) 2011 Ingres Corporation
*/

/*
** NMsym.h -- Expose the private SYM structure defn from NM
**
** History:
**	21-Nov-2011 (hanje04)
**	    Bug 123079
**	    Created from nmlocal.h to allow access from common!toolsapi
**	19-Oct-2011 (kiria01) b125861
**	    Pull in the associated definition that should have followed.
*/
 
typedef struct _SYM
{
	struct _SYM	*s_next;
	char		*s_sym;
	char		*s_val;
	struct _SYM	*s_last;
} SYM;

#if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF SYM *s_list;
#else
# ifndef __IN_NMDATA_C__
GLOBALREF SYM *s_list;
# endif
#endif
 
#endif /*__NMSYM_H_INC__*/
