/*
** Copyright (c) 1996, 2004 Actian Corporation
*/
#define __IN_NMDATA_C__

# include	<compat.h>
# include	<gl.h>
# include       <iicommon.h>
# include	<er.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include	<cs.h>
# include       "nmlocal.h"

/*
** Name:	nmdata.c
**
** Description:	Global data for NM facility.
**
** History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	08-jan-2004 (somsa01)
**	    Added symbol.bak, symbol.log, and symbol.lck locations.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	29-mar-2004 (somsa01)
**	    Added symbol.bak, and symbol.log locations.
**	31-Aug-2004 (jenjo02)
**	    Added symbol.lck location.
**     13-Jul-2011 (hanal04) SIR 125458
**         Update cs_elog calls inline with sc0e_putAsFcn changes.
**	19-Oct-2011 (kiria01) b125861
**	    Tidy up windows NM exports
*/

/* List of symbols, empty until symbol.tbl is read. */
GLOBALDEF SYM		*s_list ZERO_FILL;	/* lst of symbols */

GLOBALDEF SYSTIME	NMtime ZERO_FILL;	/* modification time */
GLOBALDEF LOCATION	NMSymloc ZERO_FILL;	/* location of symbol.tbl */
GLOBALDEF II_NM_STATIC  NM_static ZERO_FILL;
GLOBALDEF LOCATION	NMBakSymloc ZERO_FILL;	/* location of symbol.bak */
GLOBALDEF LOCATION	NMLogSymloc ZERO_FILL;	/* location of symbol.log */
GLOBALDEF LOCATION	NMLckSymloc ZERO_FILL;	/* location of symbol.lck */

