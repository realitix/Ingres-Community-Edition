/*
**Copyright (c) 2004, 2010, 2011 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <me.h>
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
#include    <bt.h>

/* external routine declarations definitions */
#define        OPV_CTREES      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPV_CTREES.C - compare two query trees for equality
**
**  Description:
**      Compare two query trees for equality
**
**  History:
**      28-jun-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      24-Jun-99 (hanal04)
**          b97219. Adjust the logic in opv_ctress() to address the
**          nullability introduced by outer-joins.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	12-Apr-2011 (kiria01)
**	    Added opv_qtree_compare*, opv_qtree_norm and opv_qtree_size.
**	    Lifted from PST functions of similar name to do essentiallty
**	    the same thing but in OPF.
**  18-Apr-2011 (bonro01)
**      Fix Windows compile problem caused by lack of define for MEcmp, MEcopy
**      caused by revision 2042.  Add me.h
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of pst_resolve
**/

/* TABLE OF CONTENTS */
static bool opv_cvar(
	OPS_STATE *global,
	PST_QNODE *node1,
	PST_QNODE *node2);
bool opv_ctrees(
	OPS_STATE *global,
	PST_QNODE *node1,
	PST_QNODE *node2);
i4 opv_qtree_compare(
	OPV_STK_CMP *ctx,
	PST_QNODE **a,
	PST_QNODE **b,
	bool fixup);
i4 opv_qtree_compare_norm(
	OPV_STK_CMP *ctx,
	PST_QNODE **a,
	PST_QNODE **b);
i4 opv_qtree_norm(
	OPV_STK *stk,
	PST_QNODE **nodep,
	DB_ERROR *err_blk);
u_i4 opv_qtree_size(
	OPV_STK *stk,
	PST_QNODE *node);

OPV_DEFINE_STK_FNS(node, PST_QNODE*);
OPV_DEFINE_STK_FNS(nodep, PST_QNODE**);


/*{
** Name: opv_cvar	- compare 2 variables with same table ID
**
** Description:
**      Make a comparison of 2 variables with the same table ID
**	by ensuring that all references to that table ID are used 
**      consistently in other comparisions.  One tree is considered
**	to be the "master" and the other the slave, in that the master's
**	table ID's will not change but eventually the slave's table ID's
**	will change.
**
** Inputs:
**      node1                           root of the master query subtree to be compared
**      node2                           root of the slave query subtree to be compared
**      global                          ptr to global state variable
**                                      for error reporting
**
** Outputs:
**
**	Returns:
**	    TRUE - is var nodes are logically equal
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-nov-90 (seputis)
**          initial creation
**	27-mar-92 (seputis)
**	    -b43339 compare IDs only, do not translate when looking at
**	    target list
[@history_template@]...
*/
static bool
opv_cvar(
	OPS_STATE          *global,
	PST_QNODE          *node1,
	PST_QNODE          *node2)
{
    OPV_GRV	    *grvp;
    bool	    ret_val;
    OPV_GRT	    *gbase;

    gbase = global->ops_rangetab.opv_base;
    grvp = gbase->opv_grv[node2->pst_sym.pst_value.pst_s_var.pst_vno];
    if ((grvp->opv_compare == OPV_NOGVAR)
	&&
	!(global->ops_gmask & OPS_CIDONLY))
    {	/* new mapping found for slave range variable */
	OPV_GRV	    **grvpp;

	/* check that no other variable has this mapping */
	for (grvpp = &gbase->opv_grv[0]; grvpp == &gbase->opv_grv[OPV_MAXVAR]; grvpp++)
	    if ((*grvpp)
		&&
		((*grvpp)->opv_compare == node1->pst_sym.pst_value.pst_s_var.pst_vno)
		)
		return(FALSE);	/* 2 table ID's cannot map to the same table */

	grvp->opv_compare = node1->pst_sym.pst_value.pst_s_var.pst_vno;
	ret_val = TRUE;
    }
    else 
	ret_val = (node1->pst_sym.pst_value.pst_s_var.pst_vno == grvp->opv_compare);
    return(ret_val);
}

/*{
** Name: opv_ctrees	- compare two subtrees for equality
**
** Description:
**      Check if two subtrees are identical (with respect to aggregate
**      processing).  The query tree comparison is a structural comparison
**      so that order is important if two aggregates are to be run together.
**      Comparison of constants within the query tree is done using ADF
**      and care is made to ensure that "pattern-matching" equality is
**      ignored.
**
**      The 4.0 version of this routine did MEcmp, but this will not
**      work on machines which align components of structures.  Some machines
**      may not require aligned components but compilers will align anyways
**      in order to improve performance.  This routine
**      compares the components directly, and is more complicated, but more
**      accurate.  This other alternative is to ensure that every node is
**      zeroed prior to being used in the query tree.  This is not done.
**
** Inputs:
**      node1                           root of the query subtree to be compared
**      node2                           root of the query subtree to be compared
**      global                          ptr to global state variable
**                                      for error reporting
**
** Outputs:
**	Returns:
**          TRUE - if subtrees are equal
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-apr-86 (seputis)
**          initial creation from sameafcn in aggregate.c
**	30-apr-90 (seputis)
**	    fix b21461, did not recognize subselect node
**	23-may-91 (seputis)
**	    compare constants of different types correctly
**      24-Jun-99 (hanal04)
**          b97219. When we check to see whether two nodes are of the same
**          DB_DATA_VALUE we must account for nullability introduced by
**          an outer-join as follows;
**             i) ABSOLUTE types should be compared.
**            ii) ADJUSTED lengths should be compared. Adjustments as follows;
**                  N1 non-nullable, N2 non-nullable => no adjustment
**                  N1 nullable, N2 non-nullable => add 1 to N2's length
**                  N1 non-nullable, N2 nullable => add -1 to N2's length
**                  N1 nullable, N2 nullable => no adjustment
**    22-oct-99 (inkdo01)
**        Support for new case expression node types.
**	11-mar-02 (inkdo01)
**	    Support for sequence operator node type.
**	5-dec-03 (inkdo01)
**	    Add support for PST_MOP, PST_OPERAND (SUBSTRING) nodes.
**	6-apr-06 (dougi)
**	    Add support for PST_GSET/GBCR/GCL nodes (for GROUP BY support).
**	30-Dec-08 (kibro01) b121419
**	    Test left node as well in CONST check in case this is actually
**	    an IN-list (flagged by pst_flags in the node above).  
**	22-Apr-09 (kibro01) b121950
**	    If there are two straight VARs and they originate from the same
**	    base field (such as when a by-list variable is included twice,
**	    once with a correlation name) make sure they get eliminated here,
**	    or else rows can go missing when NULL values appear.
**	3-Jun-2009 (kschendel) b122118
**	    IN-lists can be very long, use looping instead of recursion to
**	    traverse an IN-list (see above b121419).
**      29-jul-2009 (huazh01)
**          remove the fix to b121950. It's been rewritten as b122361.
**	26-Oct-09 (smeke01) b122793
**	    Allow commutativity for BOPs ADI_ADD_OP and ADI_MUL_OP.
**	18-may-2011 (dougi)
**	    Add PST_WINDOW for window functions.
*/
bool
opv_ctrees(
	OPS_STATE          *global,
	PST_QNODE          *node1,
	PST_QNODE          *node2)
{
    i4  length_adjust = 0;

    if (!(node1 && node2))
	return(!(node1 || node2));	    /* TRUE if both are NULL */

    if (node1->pst_sym.pst_type != node2->pst_sym.pst_type)
	return( FALSE );                    /* symbol types do not match */

    switch (node1->pst_sym.pst_type)
    {

    case PST_ROOT:		    /* bitmaps may not be up to date */
    case PST_AGHEAD:		    /* bitmaps may not be up to date */
    case PST_SUBSEL:		    /* bitmaps may not be up to date */
    case PST_BYHEAD:
    case PST_OPERAND:
    case PST_CASEOP:
    case PST_WHLIST:
    case PST_WHOP:
    case PST_AND:
    case PST_OR:
    case PST_GSET:
    case PST_GCL:
    case PST_WINDOW:
    {
	return( opv_ctrees( global, node1->pst_left, node2->pst_left )
		&&
		opv_ctrees( global, node1->pst_right, node2->pst_right )
	      );
    }
    case PST_NOT:
	return (opv_ctrees( global, node1->pst_left, node2->pst_left ));
	    /*FIXME - is left or right branch used for NOT */
    case PST_SEQOP:
    case PST_QLEND:
    case PST_TREE:
	return( TRUE );

    case PST_GBCR:
	return(node1->pst_sym.pst_value.pst_s_group.pst_groupmask ==
	    node2->pst_sym.pst_value.pst_s_group.pst_groupmask);

    case PST_UOP:
    case PST_BOP:
    case PST_AOP:
    case PST_COP:
    case PST_MOP:
    case PST_CONST:
    case PST_VAR:
    case PST_RESDOM:
    {
        /* b97219 - Assess the length adjustment if any */
        if((node1->pst_sym.pst_dataval.db_datatype < 0) &&
           (node2->pst_sym.pst_dataval.db_datatype > 0))
            length_adjust = 1;
        if((node1->pst_sym.pst_dataval.db_datatype > 0) &&
            (node2->pst_sym.pst_dataval.db_datatype < 0))
           length_adjust = -1;

	/* check that the DB_DATA_VALUE matches */
        /* b97219 - Compare absolute data types and adjusted lengths */
	if (    
		(abs(node1->pst_sym.pst_dataval.db_datatype) 
		!= 
		abs(node2->pst_sym.pst_dataval.db_datatype))
	    ||
		(node1->pst_sym.pst_dataval.db_prec
		!= 
		node2->pst_sym.pst_dataval.db_prec)
	    ||
		(node1->pst_sym.pst_dataval.db_length
		!= 
		(node2->pst_sym.pst_dataval.db_length+length_adjust))
	    )
	    return (FALSE);
	break;
    }
    case PST_SORT:
    {
	return (    (node1->pst_sym.pst_value.pst_s_sort.pst_srvno
		    ==
		    node2->pst_sym.pst_value.pst_s_sort.pst_srvno)
		&&
		    (node1->pst_sym.pst_value.pst_s_sort.pst_srasc
		    ==
		    node2->pst_sym.pst_value.pst_s_sort.pst_srasc)
		&&
		opv_ctrees( global, node1->pst_left, node2->pst_left )
		&&
		opv_ctrees( global, node1->pst_right, node2->pst_right )
	        );
    }
    case PST_CURVAL:
# ifdef  E_OP0682_UNEXPECTED_NODE
	opx_error( E_OP0682_UNEXPECTED_NODE);
# endif
    default:
	{
# ifdef E_OP0681_UNKNOWN_NODE
	    opx_error( E_OP0681_UNKNOWN_NODE);
# endif
	}
    }

    switch (node1->pst_sym.pst_type)
    {
    case PST_RESDOM:
    {
	return (    (node1->pst_sym.pst_value.pst_s_rsdm.pst_rsno
		    ==
		    node2->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
		&&
		opv_ctrees( global, node1->pst_left, node2->pst_left )
		&&
		opv_ctrees( global, node1->pst_right, node2->pst_right )
	        );
    }
    case PST_UOP:
    case PST_BOP:
    case PST_AOP:
    case PST_COP:
    case PST_MOP:
    {
	bool commutative = FALSE;

	if (node1->pst_sym.pst_type == PST_BOP) 
	{
	    /* 
	    ** Check the commutativity of the operator. Since our first test below is 
	    ** to make sure that the operators are identical, it doesn't matter which one
	    ** we pick here.
	    */
	    ADI_OP_ID opno = node1->pst_sym.pst_value.pst_s_op.pst_opno; 

	    if (opno == ADI_ADD_OP || opno == ADI_MUL_OP)
		commutative = TRUE;
	    else 
		commutative = FALSE; 
	}

	if (node1->pst_sym.pst_value.pst_s_op.pst_opno != node2->pst_sym.pst_value.pst_s_op.pst_opno)
		return FALSE;
	if (node1->pst_sym.pst_value.pst_s_op.pst_opinst != node2->pst_sym.pst_value.pst_s_op.pst_opinst && !commutative)
		return FALSE;
	if ( opv_ctrees(global, node1->pst_left, node2->pst_left ) && opv_ctrees( global, node1->pst_right, node2->pst_right) )
		return TRUE;
	else
	if ( commutative && opv_ctrees(global, node1->pst_left, node2->pst_right) && opv_ctrees(global, node1->pst_right, node2->pst_left) )
		return TRUE;
	else
		return FALSE;
    }
    case PST_CONST:
	/* Loop down the IN-list if there is one */
	do
	{
	    OPS_PNUM		parm1;
	    OPS_PNUM		parm2;

	    /* Not the same if one is an IN-list and the other isn't. */
	    if ((node1->pst_left != NULL && node2->pst_left == NULL)
	      || (node1->pst_left == NULL && node2->pst_left != NULL))
		return (FALSE);		/* IN-list pointers must correspond */

	    parm1 = node1->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
	    parm2 = node2->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
	    if (parm1 != parm2)
	    {
		if ((parm1 <= global->ops_qheader->pst_numparm)
		    ||
		    (parm2 <= global->ops_qheader->pst_numparm)
		    ||
		    (parm1 > global->ops_gdist.opd_user_parameters)
		    ||
		    (parm2 > global->ops_gdist.opd_user_parameters)
		    ||
		    (   node1->pst_sym.pst_value.pst_s_cnst.pst_pmspec
			!=
			node2->pst_sym.pst_value.pst_s_cnst.pst_pmspec
		    )
		    )
		    return(FALSE);	    /* check for non-repeat parameters
					    ** and non-simple aggregate constants
					    ** i.e. user parameters which are
					    ** constant for all invocations, these
					    ** should be treated as equal
					    ** also pattern matching should match */
	    }
	    else if (parm1 == 0)
	    {
		/* Compare non-parameter constants for equality.
		** Comparer returns nonzero if datavalues are not equal.
		*/
		if (opu_dcompare( global, &node1->pst_sym.pst_dataval,
			    &node2->pst_sym.pst_dataval))
		    return (FALSE);
	    }
	    node1 = node1->pst_left;
	    node2 = node2->pst_left;
	} while (node1 != NULL);
	return (TRUE);

    case PST_VAR:
	{
	    if (node1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		!=
		node2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		return(FALSE);		    /* if attributes are not equal
					    ** then var nodes cannot be equal
					    ** even if table IDs are not 
					    ** equal */
	    }
	    if (node1->pst_sym.pst_value.pst_s_var.pst_vno
		==
		node2->pst_sym.pst_value.pst_s_var.pst_vno)
		return(TRUE);
	    else if ((global->ops_gmask & OPS_IDSAME) 
		&&
		(global->ops_gmask & OPS_IDCOMPARE)
		)
	    {
		/* check that variable mapping can be used to make 2 range variables with
		** different table ID's equal */
		if ((	global->ops_rangetab.opv_base->opv_grv
			    [node1->pst_sym.pst_value.pst_s_var.pst_vno]->opv_same
			!=
			OPV_NOGVAR
		    )
		    &&
		    (	global->ops_rangetab.opv_base->opv_grv
			    [node1->pst_sym.pst_value.pst_s_var.pst_vno]->opv_same
			==
			global->ops_rangetab.opv_base->opv_grv
			    [node2->pst_sym.pst_value.pst_s_var.pst_vno]->opv_same
		    )
		    )
		    return(opv_cvar(global,node1,node2)); /* since the table ID's
					    ** are the same check if a valid mapping
					    ** can be found */
	    }
	    return(FALSE);
	}
    default:
	{
#ifdef    E_OP0681_UNKNOWN_NODE
	    opx_error( E_OP0681_UNKNOWN_NODE);
#endif
	}
    }
    return (FALSE);
}
/*{
** Name: opv_qtree_compare* - Compare two query trees
**
** Description:
**	Compares two query trees and returns an ordering.
**	opv_qtree_compare_norm returns a biased ordering for
**	normalisation.
**
** Inputs:
**	ctx		Control block:
**	 .stk		Recursion stack - must be initialised
**	 .cb		OPS_STATE context
**	 .n_covno	Number of valid VAR vno mapping
**	 .co_vnos[]	VAR vno mapping pairs vector
**	 .reversed	If fixup indicates reversal of sense
**	 .inconsistant	If fixup indicates prior error
**	a		Tree 1 (earlier)
**	b		Tree 2 (later)
**	fixup		Bool as to whether vnos be fixed up
**
** Outputs:
**	ctx
**	 .same		TRUE if trees match
**	 .reversed	Usually [a] expected to be 'earlier' than [b]
**			but if not .reversed will be set.
**	 .inconsistant	Error flag set if [a] & [b] not strictly
**			ordered.
**
**	Returns:
**	    cmp		Difference result.
**
**	Exceptions:
**	    None
**
** Side Effects:
**	None - stack left intact.
**
** History:
**	
**	12-Apr-2011 (kiria01)
**	    Transferred from pst_qtree_compare*
*/

i4
opv_qtree_compare(
    OPV_STK_CMP *ctx,
    PST_QNODE **a,
    PST_QNODE **b,
    bool fixup)
{
    register i4	res = 0;
    i4		depth = 0;
    i4		whlist_count = 0;
    bool	descend = TRUE;

    ctx->same = TRUE;
    if (!fixup)
	ctx->reversed = ctx->inconsistant = FALSE;

    while (a || b)
    {
	PST_SYMVALUE *av, *bv;
	if (!a)
	{
	    res = -1;
	    goto different;
	}
	if (!b)
	{
	    res = 1;
	    goto different;
	}
	if (res = (*a)->pst_sym.pst_type - (*b)->pst_sym.pst_type)
	{
	    /* Arrange that CONSTs drift left and QLEND right */
	    if ((*a)->pst_sym.pst_type == PST_CONST ||
		(*a)->pst_sym.pst_type == PST_QLEND)
		res = 1;
	    else if ((*b)->pst_sym.pst_type == PST_CONST ||
		     (*b)->pst_sym.pst_type == PST_QLEND)
		res = -1;
	    goto different;
	}

	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && ((*a)->pst_left || (*a)->pst_right ||
			(*b)->pst_left || (*b)->pst_right))
	{
	    /* Delay node evaluation (types match) */
	    opv_push_nodep(&ctx->stk, b);
	    opv_push_nodep(&ctx->stk, a);
	    /* Increment depth counters for children */
	    switch ((*a)->pst_sym.pst_type)
	    {
	    case PST_WHLIST: whlist_count++; break;
	    case PST_ROOT:
	    case PST_SUBSEL:
		depth++;
		if ((*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next &&
		    (*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    opv_push_nodep(&ctx->stk, &(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    opv_push_nodep(&ctx->stk, &(*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    opv_push_nodep(&ctx->stk, OPV_DESCEND_MARK);
		    if ((res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_setop -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_setop) ||
			(res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_dups -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_dups) ||
			(res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_nest -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_nest))
		    {
			goto different;
		    }
		}
		else if ((*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    res = -1;
		    goto different;
		}
		else if ((*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    res = 1;
		    goto different;
		}
		break;
	    default:
		break;
	    }
	    if ((*a)->pst_left)
	    {
		if (!(*b)->pst_left)
		{
		    res = -1;
		    goto different;
		}

		if ((*a)->pst_right)
		{
		    if (!(*b)->pst_right)
		    {
			res = -1;
			goto different;
		    }
		    if (res = (*a)->pst_right->pst_sym.pst_type -
			    (*b)->pst_right->pst_sym.pst_type)
		    {
			if ((*a)->pst_right->pst_sym.pst_type == PST_CONST ||
			    (*a)->pst_right->pst_sym.pst_type == PST_QLEND)
			    res = 1;
			else
			if ((*b)->pst_right->pst_sym.pst_type == PST_CONST ||
			    (*b)->pst_right->pst_sym.pst_type == PST_QLEND)
			    res = -1;
			goto different;
		    }

		    /* Delay RHS (types match) */
		    opv_push_nodep(&ctx->stk, &(*b)->pst_right);
		    opv_push_nodep(&ctx->stk, &(*a)->pst_right);
		    if ((*a)->pst_right->pst_right ||
				(*a)->pst_right->pst_left)
			/* Mark that the top nodes need descending */
			opv_push_nodep(&ctx->stk, OPV_DESCEND_MARK);
		}
		a = &(*a)->pst_left;
		b = &(*b)->pst_left;
		continue;
	    }
	    else if ((*a)->pst_right)
	    {
		if ((*b)->pst_left || !(*b)->pst_right)
		{
		    res = 1;
		    goto different;
		}
		a = &(*a)->pst_right;
		b = &(*b)->pst_right;
		continue;
	    }
	    else
	    {
		res = 1;
		goto different;
	    }
	}

	/*** Depth first traverse ***/
	av = &(*a)->pst_sym.pst_value;
	bv = &(*b)->pst_sym.pst_value;
	switch ((*a)->pst_sym.pst_type)
	{
	    
	case PST_UOP:
	case PST_BOP:
	case PST_AOP:
	case PST_COP:
	case PST_MOP:
	    if ((res = av->pst_s_op.pst_opno - bv->pst_s_op.pst_opno) ||
		(res = av->pst_s_op.pst_opinst - bv->pst_s_op.pst_opinst) ||
		(res = av->pst_s_op.pst_distinct - bv->pst_s_op.pst_distinct) ||
		(res = av->pst_s_op.pst_opmeta - bv->pst_s_op.pst_opmeta) ||
		(res = av->pst_s_op.pst_escape - bv->pst_s_op.pst_escape) ||
		(res = av->pst_s_op.pst_pat_flags - bv->pst_s_op.pst_pat_flags) ||
		(res = av->pst_s_op.pst_flags - bv->pst_s_op.pst_flags))
	    {
		goto different;
	    }
	    if (av->pst_s_op.pst_joinid != bv->pst_s_op.pst_joinid)
	    {
	    }
	    break;
	case PST_CONST:
	    /* Single out real constants */
	    if ((res = (av->pst_s_cnst.pst_tparmtype == PST_USER &&
			!av->pst_s_cnst.pst_parm_no) -
		       (bv->pst_s_cnst.pst_tparmtype == PST_USER &&
			!bv->pst_s_cnst.pst_parm_no)))
		goto different;
	    if ((res = av->pst_s_cnst.pst_tparmtype - bv->pst_s_cnst.pst_tparmtype) ||
		(res = av->pst_s_cnst.pst_parm_no - bv->pst_s_cnst.pst_parm_no) ||
		(res = av->pst_s_cnst.pst_pmspec - bv->pst_s_cnst.pst_pmspec) ||
		(res = av->pst_s_cnst.pst_cqlang - bv->pst_s_cnst.pst_cqlang))
	    {
		goto different;
	    }
	    if ((res = (*a)->pst_sym.pst_dataval.db_datatype - (*b)->pst_sym.pst_dataval.db_datatype) ||
		(res = (*a)->pst_sym.pst_dataval.db_length - (*b)->pst_sym.pst_dataval.db_length) ||
		(res = (*a)->pst_sym.pst_dataval.db_prec - (*b)->pst_sym.pst_dataval.db_prec))
	    {
		goto different;
	    }
	    if ((*a)->pst_sym.pst_dataval.db_data && (*b)->pst_sym.pst_dataval.db_data)
	    {
		if (res = MEcmp((*a)->pst_sym.pst_dataval.db_data, (*b)->pst_sym.pst_dataval.db_data,
			    (*a)->pst_sym.pst_dataval.db_length))
		    goto different;
	    }
	    else if ((*a)->pst_sym.pst_dataval.db_data || (*b)->pst_sym.pst_dataval.db_data)
	    {
		if ((*a)->pst_sym.pst_dataval.db_data)
		    res = -1;
		else
		    res = 1;
		goto different;
	    }
	    break;
	case PST_RESDOM:
	case PST_BYHEAD:
	    if (res = av->pst_s_rsdm.pst_ttargtype - bv->pst_s_rsdm.pst_ttargtype)
	    {
		goto different;
	    }
	    break;
	case PST_FWDVAR:
	    res = 0;
	    goto different;
	case PST_VAR:
	    if ((res = av->pst_s_var.pst_atno.db_att_id -
					bv->pst_s_var.pst_atno.db_att_id) ||
		(res = MEcmp(&av->pst_s_var.pst_atname, &bv->pst_s_var.pst_atname,
			sizeof(av->pst_s_var.pst_atname))) ||
		(!ctx->stk.cb ||
		    !ctx->stk.cb->ops_qheader ||
			!ctx->stk.cb->ops_qheader->pst_rangetab) &&
		    (res = av->pst_s_var.pst_vno - bv->pst_s_var.pst_vno))
	    {
		goto different;
	    }
	    res = av->pst_s_var.pst_vno - bv->pst_s_var.pst_vno;
	    if (res)
	    {
		PST_RNGENTRY **rngs = ctx->stk.cb->ops_qheader->pst_rangetab;
		PST_RNGENTRY *at = rngs[av->pst_s_var.pst_vno];
		PST_RNGENTRY *bt = rngs[bv->pst_s_var.pst_vno];
		u_i4 i;
		/* vno values differ - if in a sub-select context, see
		** if they refer to the same tables and if so, we will
		** tentatively accept as the same as long as all the
		** subsequent vno match consistently. 
		** We will also return the result. */
		if (!depth ||
		    (res = at->pst_rgtype - bt->pst_rgtype) ||
		    (res = at->pst_rgparent - bt->pst_rgparent) ||
		    (res = at->pst_rngvar.db_tab_base - bt->pst_rngvar.db_tab_base) ||
		    (res = at->pst_rngvar.db_tab_index - bt->pst_rngvar.db_tab_index))
		{
		    goto different;
		}
		for (i = 0; i < ctx->n_covno; i++)
		{
		    if (ctx->co_vnos[i].a == av->pst_s_var.pst_vno)
		    {
			if (ctx->co_vnos[i].b == bv->pst_s_var.pst_vno)
			{
			    if (fixup)
			    {
				i4 lose = ctx->reversed
					? ctx->co_vnos[i].a
					: ctx->co_vnos[i].b;
				i4 keep = ctx->reversed
					? ctx->co_vnos[i].b
					: ctx->co_vnos[i].a;
				if (rngs[keep]->pst_rgtype != PST_UNUSED)
				{
				    PST_RNGENTRY *rv = rngs[lose];
				    av->pst_s_var.pst_vno = keep;
				    bv->pst_s_var.pst_vno = keep;
				    if (rv->pst_rgtype != PST_UNUSED)
				    {
					PST_QNODE *rt = opv_antecedant_by_2types(
					    &ctx->stk, ctx->reversed
					    ?*a:*b, PST_SUBSEL, PST_AGHEAD);
					BTset(keep, &rt->pst_sym.pst_value.pst_s_root.pst_tvrm);
					BTclear(lose, &rt->pst_sym.pst_value.pst_s_root.pst_tvrm);
					/* Clear the inner and the outer relation mask
					** It is unlikely that these will be set. */
					MEfill(sizeof(PST_J_MASK), 0, (PTR)&rv->pst_inner_rel);
					MEfill(sizeof(PST_J_MASK), 0, (PTR)&rv->pst_outer_rel);
				    }
				}
			    }
			    break;
			}
			/* vno not consistant */
			goto different;
		    }
		    else if (ctx->co_vnos[i].b == bv->pst_s_var.pst_vno)
			/* vno not consistant */
			goto different;
		}
		if (i == ctx->n_covno)
		{
		    /* new vno - accept for now */
		    ctx->co_vnos[i].a = av->pst_s_var.pst_vno;
		    ctx->co_vnos[i].b = bv->pst_s_var.pst_vno;
		    if (ctx->co_vnos[i].a > ctx->co_vnos[i].b)
			if (!i)
			    ctx->reversed = TRUE;
			else if (!ctx->reversed)
			    ctx->inconsistant = TRUE;
		    ctx->n_covno++;
		}
	    }
	    break;
	case PST_ROOT:
	case PST_SUBSEL:
	    depth--;
	    /*FALLTHROUGH*/
	case PST_AGHEAD:
	    if ((res = av->pst_s_root.pst_dups - bv->pst_s_root.pst_dups) ||
		(res = av->pst_s_root.pst_qlang - bv->pst_s_root.pst_qlang) ||
		(res = av->pst_s_root.pst_mask1 - bv->pst_s_root.pst_mask1) ||
		(res = av->pst_s_root.pst_project - bv->pst_s_root.pst_project))
	    {
		goto different;
	    }
	    break;
	case PST_SORT:
	    if ((res = av->pst_s_sort.pst_srvno - bv->pst_s_sort.pst_srvno) ||
		(res = av->pst_s_sort.pst_srasc - bv->pst_s_sort.pst_srasc))
	    {
		goto different;
	    }
	    break;
	case PST_CURVAL:
	    if (res = MEcmp(&av->pst_s_curval, &bv->pst_s_curval,
			sizeof(av->pst_s_curval)))
	    {
		goto different;
	    }
	    break;
	case PST_RULEVAR:
	    if ((res = av->pst_s_rule.pst_rltype - bv->pst_s_rule.pst_rltype) ||
		(res = av->pst_s_rule.pst_rltargtype - bv->pst_s_rule.pst_rltargtype) ||
		(res = av->pst_s_rule.pst_rltno - bv->pst_s_rule.pst_rltno))
	    {
		goto different;
	    }
	    break;
	case PST_TAB:
	    if (res = MEcmp(&av->pst_s_tab, &bv->pst_s_tab,
			sizeof(av->pst_s_tab)))
	    {
		goto different;
	    }
	    break;
	case PST_CASEOP:
	    if ((res = av->pst_s_case.pst_caselen - bv->pst_s_case.pst_caselen) ||
		(res = av->pst_s_case.pst_casedt - bv->pst_s_case.pst_casedt) ||
		(res = av->pst_s_case.pst_caseprec - bv->pst_s_case.pst_caseprec) ||
		(res = av->pst_s_case.pst_casetype - bv->pst_s_case.pst_casetype))
	    {
		goto different;
	    }
	    break;
	case PST_SEQOP:
	    if (res = MEcmp(&av->pst_s_seqop, &bv->pst_s_seqop, sizeof (av->pst_s_seqop)))
	    {
		goto different;
	    }
	    break;
	case PST_GBCR:
	    if (res = av->pst_s_group.pst_groupmask - bv->pst_s_group.pst_groupmask)
	    {
		goto different;
	    }
	    break;

	case PST_AND:
	case PST_OR:
	case PST_NOT:
	case PST_QLEND:
	case PST_TREE:
	case PST_OPERAND:
	case PST_WHLIST:
	case PST_WHOP:
	case PST_GCL:
	case PST_GSET:
	default:
	    break;
	}
	a = opv_pop_nodep(&ctx->stk);
	if (descend = (a == OPV_DESCEND_MARK))
	    a = opv_pop_nodep(&ctx->stk);
	b = opv_pop_nodep(&ctx->stk);
	continue;
different:
	ctx->same = FALSE;
	break;
    }
    opv_pop_all(&ctx->stk);
    return res;
}

i4
opv_qtree_compare_norm(
    OPV_STK_CMP *ctx,
    PST_QNODE **a,
    PST_QNODE **b)
{
    u_i4 asize, bsize;

    ctx->same = FALSE;
    if (!a && !b)
    {
	ctx->same = TRUE;
	return 0;
    }
    if (!a)
	return 1;
    if (!b)
	return -1;
    /* Make minus unweighted to ensure that sub-trees that
    ** are simple negatives have the same weight */
    while (*a &&
	    (*a)->pst_sym.pst_type == PST_UOP &&
	    (*a)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	a = &(*a)->pst_left;
    while (*b &&
	    (*b)->pst_sym.pst_type == PST_UOP &&
	    (*b)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	b = &(*b)->pst_left;

    if (!*a && !*b)
    {
	ctx->same = TRUE;
	return 0;
    }
    if (!*a)
	return 1;
    if (!*b)
	return -1;
    if ((*a)->pst_sym.pst_type != (*b)->pst_sym.pst_type)
    {
	/* Arrange that CONSTs and QLENDs drift right */
	if ((*a)->pst_sym.pst_type == PST_CONST ||
	    (*a)->pst_sym.pst_type == PST_QLEND)
	    return 1;
	if ((*b)->pst_sym.pst_type == PST_CONST ||
	    (*b)->pst_sym.pst_type == PST_QLEND)
	    return -1;
    }
    asize = opv_qtree_size(&ctx->stk, *a);
    bsize = opv_qtree_size(&ctx->stk, *b);
    if (asize < bsize)
	return -1;
    if (asize > bsize)
	return 1;
    return opv_qtree_compare(ctx, a, b, FALSE);
}

/*
** Name: opv_qtree_norm - normalise 1 tree for compares.
**
** Description:
**	Does a flattened, depth first traversal of the parse tree looking
**
** Input:
**	stk		Shared stack context
**	nodep		Address of pointer to global tree to flatten
**
** Output:
**	nodep		The tree may be updated.
**	err_blk		Any error returned.
**
** Side effects:
**	Will re-order '+'s, '*', ANDs and ORs to normalised form. In the
**      case of arithmetic expressions with associative operators, the tree
**	will be left weighted with constants folded. Boolean factors will
**	be initially weighted by join id to drift them right wards.
**
** History:
**	12-Apr-2011 (kiria01)
**	    Transferred from pst_qtree_norm
**	06-Jul-2011 (kiria01) b125511
**	    Protect tree descent from including pointers to nodes that
**	    may have been disolved.
*/

DB_STATUS
opv_qtree_norm(
	OPV_STK		*stk,
	PST_QNODE	**nodep,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status = E_DB_OK;
    OPV_STK_CMP		cmp_ctx;
    i4			opno;
    i4			dt, ldt, rdt;
    PST_TYPE		ty;
    PST_QNODE		*node;
    PST_QNODE		fake;
    DB_LANG		qlang = DB_SQL;

    OPV_STK_CMP_INIT(cmp_ctx, stk->cb);
    if (stk->cb && stk->cb->ops_qheader && stk->cb->ops_qheader->pst_qtree &&
	 stk->cb->ops_qheader->pst_qtree->pst_sym.pst_type == PST_ROOT)
	qlang = stk->cb->ops_qheader->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang;

    while (nodep)
    {
	node = *nodep;

	/*** Top-down traverse ***/
	switch (ty = node->pst_sym.pst_type)
	{
	case PST_BOP:
	    opno = node->pst_sym.pst_value.pst_s_op.pst_opno;
	    dt = abs(node->pst_sym.pst_dataval.db_datatype);
	    ldt = abs(node->pst_left->pst_sym.pst_dataval.db_datatype);
	    rdt = abs(node->pst_right->pst_sym.pst_dataval.db_datatype);
	    if (opno == ADI_ADD_OP && (/* ensure not really a CONCAT */
			ldt == DB_INT_TYPE ||
			ldt == DB_FLT_TYPE ||
			rdt == DB_DEC_TYPE) && (
			rdt == DB_INT_TYPE ||
			rdt == DB_FLT_TYPE ||
			rdt == DB_DEC_TYPE) ||
		opno == ADI_MUL_OP)
	    {
		PST_QNODE *l, *c, **p = nodep;
		bool reresolve = FALSE;

		/* *Not* fake = **nodep since the node will be allocated
		** exact-size, and we mustn't copy past the end of *nodep.
		*/
		MEcopy((PTR) (*nodep),
			sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE),
			(PTR) &fake);
		if ((*p)->pst_sym.pst_value.pst_s_op.pst_fdesc)
		{
		    (*p)->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL;
		    reresolve = TRUE;
		}
		do
		{
		    while ((l = (*p)->pst_left) &&
			l->pst_sym.pst_type == PST_BOP &&
			l->pst_sym.pst_value.pst_s_op.pst_opno == opno &&
			abs(l->pst_sym.pst_dataval.db_datatype) == dt)
		    {
			if (l->pst_sym.pst_value.pst_s_op.pst_fdesc)
			{
			    l->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL;
			    reresolve = TRUE;
			}
			(*p)->pst_left = l->pst_right;
			l->pst_right = *p;
			*p = l;
		    }
		    p = &(*p)->pst_right;
		}
		while ((*p)->pst_sym.pst_type == PST_BOP &&
			(*p)->pst_sym.pst_value.pst_s_op.pst_opno == opno &&
			abs((*p)->pst_sym.pst_dataval.db_datatype) == dt);
		fake.pst_left = *p;
		fake.pst_right = NULL;
		*p = &fake;
		/* At this point the common logical operators are a RHS list
		** and (*p) points to the fake tail which we shall detach
		** eventually. The constants will have moved to the end and
		** the list which will look like:
		**     +		This arrangement is the wrong way
		**    / \		round to what we really want which is
		**   e1  +		a LHS order but shortly we will rotate
		**	/ \		the tree as we resolve the operators.
		**     e2  +
		**	  / \		With constants drifted to the right
		**	 c1  F		we can cause these to be folded and
		**	    /		once the tree is rotated the constants
		**	   c2		will be in the right place for being
		**			ignored as non-core constants.
		*/
		l = *nodep;
		while (c = l->pst_right)
		{
		    do
		    {
			if (opv_qtree_compare_norm(&cmp_ctx,
					&l->pst_left,
					&c->pst_left) > 0)
			{
			    PST_QNODE *t = c->pst_left;
			    c->pst_left = l->pst_left;
			    l->pst_left = t;
			}
		    } while (c = c->pst_right);
		    l = l->pst_right;
		}
		/* Find the first of any constants */
		p = nodep;
		c = NULL;
		while ((*p)->pst_right && (c = (*p)->pst_left))
		{
		    if (c->pst_sym.pst_type == PST_CONST &&
			c->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
			!c->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		    {
			/* Found first of at least 2 constants (as (*p)->pst_right != 0)
			** Arrange that they be folded. */
			while ((*p)->pst_right) /* *p != &fake */
			{
			    PST_QNODE **q = p;
			    while ((*q)->pst_right != &fake)
				q = &(*q)->pst_right;
			    /* q refers now to the training pair of constants.
			    ** Fiddle tail for folding to this and call resolver
			    **	  +		to do the folding. As we are
			    **	 / \		only dealing with true constants
			    **	e1  +		we can resolve the types now
			    **	   / \		regardless of outer parse state.
			    **	  e2  F
			    **	     /
			    **	    +
			    **	   / \
			    **	  c1  c2
			    */
			    c = *q;
			    *q = &fake;
			    c->pst_right = fake.pst_left;
			    fake.pst_left = c;
			    if (status = pst_resolve(NULL, stk->cb->ops_adfcb,
					&fake.pst_left, qlang, err_blk))
				return status;
			}
			break;
		    }
		    p = &(*p)->pst_right;
		}
		/* drop fake */
		*p = fake.pst_left;
		if (p != nodep)
		{
		    for(;;)
		    {  
			node = *nodep;
			if (node->pst_right == fake.pst_left)
			    break;
			*nodep = (*nodep)->pst_right;
			node->pst_right = (*nodep)->pst_left;
			if (reresolve &&
			    (status = pst_resolve(NULL, stk->cb->ops_adfcb, &node,
					qlang, err_blk)))
				return status;
					    return status;
			(*nodep)->pst_left = node;
		    }
		}
		if (reresolve &&
		    (status = pst_resolve(NULL, stk->cb->ops_adfcb, nodep,
				qlang, err_blk)))
		    return status;

		do {
		    if ((*nodep)->pst_right)
			opv_push_nodep(stk, &(*nodep)->pst_right);
		    p = nodep;
		    nodep = &(*nodep)->pst_left;
		} while ((l = *nodep) &&
		    l->pst_sym.pst_type == PST_BOP &&
		    l->pst_sym.pst_value.pst_s_op.pst_opno == opno);
		nodep = &(*p)->pst_left;
		continue;
	    }
	    break;
	case PST_AND:
	case PST_OR:
	    {
		PST_QNODE *l, *c, **p = nodep;

		/* *Not* fake = **nodep since the node will be allocated
		** exact-size, and we mustn't copy past the end of *nodep.
		*/
		MEcopy((PTR) (*nodep),
			sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE),
			(PTR) &fake);
		do
		{
		    while ((l = (*p)->pst_left) &&
			l->pst_sym.pst_type == ty)
		    {
			(*p)->pst_left = l->pst_right;
			l->pst_right = *p;
			*p = l;
		    }
		    p = &(*p)->pst_right;
		} while ((*p)->pst_sym.pst_type == ty);
		
		fake.pst_left = *p;
		fake.pst_right = NULL;
		*p = &fake;
		/* At this point the common logical operators are a RHS list
		** and (*p) points to the odd tail which we shall detach
		** and use as the comparitor down the list which will look
		** like:		  A
		**			 / \
		**		        e1  A
		**		           / \
		**			  e2  A
		**			     / \
		**			    e3  F
		**			       /
		**			      e4
		*/
		l = *nodep;
		while (c = l->pst_right)
		{
		    do
		    {
			if (opv_qtree_compare_norm(&cmp_ctx,
					&l->pst_left,
					&c->pst_left) > 0)
			{
			    PST_QNODE *t = c->pst_left;
			    c->pst_left = l->pst_left;
			    l->pst_left = t;
			}
		    } while (c = c->pst_right);
		    l = l->pst_right;
		}
		*p = NULL;
		while (*nodep)
		{
		    if ((*nodep)->pst_left)
			opv_push_nodep(stk, &(*nodep)->pst_left);
		    nodep = &(*nodep)->pst_right;
		}
		*nodep = fake.pst_left;
		continue;
	    }
	    break;
	default:
	    break;
	}
	if (node->pst_left)
	{
	    if (node->pst_right)
		opv_push_nodep(stk, &node->pst_right);
	    nodep = &node->pst_left;
	}
	else if (node->pst_right)
	    nodep = &node->pst_right;
	else
	    nodep = opv_pop_nodep(stk);
    }

    return status;
}

/*
** Name: opv_qtree_size - size the tree
**
** Description:
**	Does a flattened parse tree count ignoring minuses.
**
** Input:
**	stk		Shared stack context
**	node		Address of tree to size
**
** Output:
**	none
**
** Side effects:
**	None
**
** History:
**	12-Apr-2011 (kiria01)
**	    Transferred from pst_qtree_size
*/

u_i4
opv_qtree_size(
	OPV_STK		*stk,
	PST_QNODE	*node)
{
    i4			tot = 0;

    while (node)
    {
	if (node->pst_sym.pst_type != PST_UOP ||
		node->pst_sym.pst_value.pst_s_op.pst_opno != ADI_MINUS_OP)
	    tot++;
	if (node->pst_left)
	{
	    if (node->pst_right)
		opv_push_node(stk, node->pst_right);
	    node = node->pst_left;
	}
	else if (!(node = node->pst_right))
	    node = opv_pop_node(stk);
    }

    return tot;
}
