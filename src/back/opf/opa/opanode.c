/*
**Copyright (c) 2009 Actian Corporation
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
#include    <adfops.h>
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
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPANODE.C - routines similar to pstnode.c
**
**  Description:
**      routines similar to pstnode.c
**
**  History:    
**      5-oct-2009 (dougi)
**	    Written.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
PST_QNODE *opa_makeop(
	OPS_STATE *global,
	PST_TYPE optype,
	ADI_OP_ID opid,
	PST_QNODE *operand1,
	PST_QNODE *operand2);


/*{
** Name: opa_makeop	- generate a PST_OP_NODE
**
** Description:
**	Allocates a PST_OP_NODE of the requested type, sets its operaqnds 
**	and resolves it.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	optype		type of operator (BOP, UOP, AOP)
**	opid		op code
**	operand1	ptr to 1st operand
**	operand2	ptr to 2nd operand or NULL
**
** Outputs:
**      none
**
**	Returns:
**	    ptr to revised parse tree fragment
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-oct-2009 (dougi)
**	    Written.
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of opa_gresolve reflecting
**	    pst_resolve change.
*/
FUNC_EXTERN PST_QNODE *
opa_makeop(
	OPS_STATE	*global,
	PST_TYPE	optype,
	ADI_OP_ID	opid,
	PST_QNODE	*operand1,
	PST_QNODE	*operand2)

{
    PST_QNODE		*nodep;
    DB_STATUS	status;
    DB_ERROR	error;


    /* Simply allocate the node and fill it in. */
    nodep = opv_opnode(global, optype, opid, (OPL_IOUTER) PST_NOJOIN);
    nodep->pst_left = operand1;
    nodep->pst_right = operand2;

    if (opid >= (ADI_OP_ID) 0)
    {
	/* Only resolve real operators (not PST_AND, for example). */
	opa_gresolve(global, &nodep);
    }

    return(nodep);
}
