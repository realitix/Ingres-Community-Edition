/*
** Copyright (c) 1995, 2009, 2011 Actian Corporation
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <cm.h>

/*
** Name:	cmdata.c
**
** Description:	Global data for cm facility.
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	11-apr-2007 (gupsh01)
**	    Added Global to indicate if CM 
**	    refers to UTF8 characters.
**	22-may-2007 (gupsh01)
**	    Added CM_utf8casetab and CM_utf8attrtab.
**	14-jul-2008 (gupsh01)
**	    Modify CM_utf8casetab and CM_utf8attrtab to 
**	    CM_casetab_UTF8 and CM_attrtab_UTF8.
**	12-aug-2008 (gupsh01)
**	    Revert GLOBALREF declarations to extern
**	    as this breaks Windows.
**      02-feb-2009 (joea)
**          Change CM_isUTF8 to bool.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Initialize CM_isDBL
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Default CM_isDBL to doublebyte.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	10-Oct-2011 (kiria01) b125830
**	    Moved Unicode table globals into cmutype.roc and non-Unicode
**	    tables to cmtype.roc so that const references can be supported.
*/

GLOBALDEF bool			CM_isUTF8 =		FALSE;
GLOBALDEF bool			CM_isDBL =		TRUE;
