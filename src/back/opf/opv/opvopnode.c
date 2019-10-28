/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
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
#define        OPV_OPNODE      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPVOPNODE.C - create a PST_OP_NODE
**
**  Description:
**      This routine will create a query tree PST_OP_NODE node as well
**	as CASEOP, WHLIST AND WHOP nodes.
**
**  History:
**      1-jul-86 (seputis)
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	08-Jun-2011 (kiria01) b125435
**	    Chain OPERANDS on right hand side.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
PST_QNODE *opv_opnode(
	OPS_STATE *global,
	i4 type,
	ADI_OP_ID operator,
	OPL_IOUTER ojid);
PST_QNODE *opv_casenode(
	OPS_STATE *global,
	i4 type);
PST_QNODE *opv_whlistop(
	OPS_STATE *global);
PST_QNODE *opv_operand(
	OPS_STATE *global,
	PST_QNODE *opndp);

/*{
** Name: opv_opnode	- make a PST_OP_NODE node
**
** Description:
**      This procedure allocates and initializes a query tree PST_OP_NODE node.
**      Routine is used specifically to allocate a PST_AND node or a PST_BOP
**      node for a "=" in which the pst_dataval component is not defined.
**
** Inputs:
**      global                          ptr to global state variable
**      type                            type of PST_OP_NODE to be created
**      operator                        ADF operator id for node
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a PST_OP_NODE node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-86 (seputis)
**          initial creation
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
[@history_line@]...
*/
PST_QNODE *
opv_opnode(
	OPS_STATE          *global,
	i4                type,
	ADI_OP_ID	   operator,
	OPL_IOUTER	   ojid)
{
    register PST_QNODE *qnode;      /* ptr used for symbol allocation */
    i4		       op_size;

    op_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE);
    qnode = (PST_QNODE *) opu_memory(global, op_size); /* init PST_AND node
                                            ** to terminate qualification list*/
    MEfill(op_size, (u_char)0, (PTR)qnode);
    /* qnode->pst_left = NULL; */
    /* qnode->pst_right = NULL; */
    qnode->pst_sym.pst_type = type; /* create PST_OP_NODE node type */
    /* pst_dataval not defined for relational operators, AND nodes */
    /* qnode->pst_sym.pst_dataval.db_datatype = DB_NODT; */
    /* qnode->pst_sym.pst_dataval.db_prec = 0; */
    /* qnode->pst_sym.pst_dataval.db_length = 0; */
    /* qnode->pst_sym.pst_dataval.db_data = NULL; */
    /* qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0; */
    qnode->pst_sym.pst_len = sizeof(PST_OP_NODE);
    qnode->pst_sym.pst_value.pst_s_op.pst_opno = operator;
    qnode->pst_sym.pst_value.pst_s_op.pst_opinst =
    qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid =
    qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NOFI;
    qnode->pst_sym.pst_value.pst_s_op.pst_joinid = ojid;
    qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
    return( qnode );
}

/*{
** Name: opv_casenode	- make a PST_CASEOP_NODE node
**
** Description:
**      This procedure allocates and initializes a query tree PST_OP_NODE node.
**      Routine is used specifically to allocate a PST_AND node or a PST_BOP
**      node for a "=" in which the pst_dataval component is not defined.
**
** Inputs:
**      global                          ptr to global state variable
**      type                            type of CASE node (SIMPLE or SEARCHED)
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a PST_CASE_NODE node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-oct-2009 (dougi)
**	     Written.
*/
PST_QNODE *
opv_casenode(
	OPS_STATE          *global,
	i4                type)
{
    register PST_QNODE *qnode;      /* ptr used for symbol allocation */
    i4		       node_size;

    node_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_CASE_NODE);
    qnode = (PST_QNODE *) opu_memory(global, node_size);
    MEfill(node_size, (u_char)0, (PTR)qnode);
    /* qnode->pst_left = NULL; */
    /* qnode->pst_right = NULL; */
    /* qnode->pst_sym.pst_dataval.db_datatype = DB_NODT; */
    /* qnode->pst_sym.pst_dataval.db_prec = 0; */
    /* qnode->pst_sym.pst_dataval.db_length = 0; */
    /* qnode->pst_sym.pst_dataval.db_data = NULL; */
    /* qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0; */
    qnode->pst_sym.pst_type = PST_CASEOP;
    qnode->pst_sym.pst_len = sizeof(PST_CASE_NODE);
    qnode->pst_sym.pst_value.pst_s_case.pst_casetype = type;
    return( qnode );
}

/*{
** Name: opv_whlistop	- make a PST_WHLIST/WHOP node pair
**
** Description:
**      This function allocates a pair of empty parse tree nodes - one a
**	PST_WHLIST and the other a PST_WHOP (they come in pairs).
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a PST_WHLIST node which
**	    addresses a PST_WHOP node from its pst_right ptr
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-oct-2009 (dougi)
**	     Written.
*/
PST_QNODE *
opv_whlistop(
	OPS_STATE          *global)
{
    register PST_QNODE *qnode, *qnode1;
    i4		       node_size;

    node_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE);

    qnode = (PST_QNODE *) opu_memory(global, 2*node_size);
							/* get 2 of 'em */
    MEfill(2*node_size, (u_char)0, (PTR)qnode);
    /* qnode->pst_left = NULL; */
    /* qnode->pst_right = NULL; */
    /* qnode->pst_sym.pst_dataval.db_datatype = DB_NODT; */
    /* qnode->pst_sym.pst_dataval.db_prec = 0; */
    /* qnode->pst_sym.pst_dataval.db_length = 0; */
    /* qnode->pst_sym.pst_dataval.db_data = NULL; */
    /* qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0; */
    qnode->pst_sym.pst_type = PST_WHLIST;
    qnode->pst_sym.pst_len = 0;
    qnode1 = qnode->pst_right = (PST_QNODE *) &qnode->pst_sym.pst_value;
					/* addr of 2nd empty PST_QNODE */
    qnode1->pst_sym.pst_type = PST_WHOP;
    qnode1->pst_sym.pst_len = 0;
    return( qnode );
}

/*{
** Name: opv_operand	- make a PST_OPERAND node
**
** Description:
**      This function allocates a PST_OPERAND node for use under a PST_MOP.
**
** Inputs:
**      global                  ptr to global state variable
**	opndp			ptr to operand to store under node
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a PST_OPERAND
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-nov-2009 (dougi)
**	     Written.
*/
PST_QNODE *
opv_operand(
	OPS_STATE	*global,
	PST_QNODE	*opndp)
{
    register PST_QNODE *qnode;
    i4		       node_size;

    node_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE);

    qnode = (PST_QNODE *) opu_memory(global, node_size);
    MEfill(node_size, (u_char)0, (PTR)qnode);
    qnode->pst_sym.pst_type = PST_OPERAND;
    qnode->pst_sym.pst_len = 0;
    qnode->pst_left = opndp;
    /* Copy operand dataval into PST_OPERAND node. */
    STRUCT_ASSIGN_MACRO(opndp->pst_sym.pst_dataval, qnode->pst_sym.pst_dataval);
    return( qnode );
}
