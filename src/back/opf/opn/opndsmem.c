/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <sl.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define             OPN_DSMEMORY        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNDSMEM.C - return OPN_SUBTREE structure to memory manager
**
**  Description:
**      routine to return OPN_SUBTREE structure to memory manager
**
**  History:    
**      29-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
void opn_dsmemory(
	OPS_SUBQUERY *subquery,
	OPN_SUBTREE *stp);

/*{
** Name: opn_dsmemory	- return OPN_SUBTREE structure to memory manager
**
** Description:
**      The routine will free an OPN_SUBTREE structure
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      stp                             ptr to OPN_SUBTREE structure to return
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-may-86 (seputis)
**          initial creation
**	21-feb-02 (inkdo01)
**	    Clear ptrs to CO-nodes and up to OPN_EQS (for safety).
[@history_line@]...
*/
VOID
opn_dsmemory(
	OPS_SUBQUERY       *subquery,
	OPN_SUBTREE	   *stp)
{
    if (stp)
    {
	OPN_SUBTREE	    *tempstp;
	for (tempstp = stp; tempstp->opn_aggnext != stp; tempstp = tempstp->opn_aggnext)
	    ;
	tempstp->opn_aggnext = stp->opn_aggnext;    /* remove from linked list
						** if necessary */
	stp->opn_coforw = (OPO_CO *)subquery->ops_global->ops_estate.opn_stfree;
	stp->opn_coback = NULL;
	subquery->ops_global->ops_estate.opn_stfree = stp;
	stp->opn_eqsp = NULL;
    }
}
