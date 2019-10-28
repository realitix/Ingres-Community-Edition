/*
**Copyright (c) 2004 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSFGLOB.C - Global variables in parser facility
**
**  Description:
**      This file defines the global variables in the parser facility. 
**      There will be only one: the server control block.
**
**          NO FUNCTIONS
**
**
**  History:    
**      07-jan-86 (jeff)    
**          written
**      02-sep-86 (seputis)
**          removed definitions which were explicitly initialized - (since
**          lint complained) should explicitly initialized
**	    all fields for IBM group
**	05-nov-86 (daved)
**	    all values of server control block are initialized in 
**	    psq_start.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      13-jul-1993 (ed)
**          unnest dbms.h
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	01-Jul-2011 (kiria01) b125504
**	    This module is now in effect redundant as all the real data
**	    that was in here is really local and private to PSF.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
[@history_template@]...
**/


/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF i4		      Psf_dummy = 0;
