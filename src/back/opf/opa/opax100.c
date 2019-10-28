/*
**Copyright (c) 2009,2011 Actian Corporation
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
#include    <x100.h>
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
**  Name: OPAX100.C - routines to apply transformations specific to
**	X100. The first will be transforms of the any() aggregate and 
**	the statistical aggregates - hence the function opa_x100_xform().
**
**  Description:
**      Transforms the complex statistical functions into the equivalent
**	primitive aggregates.
**
**  History:    
**      5-oct-2009 (dougi)
**	    Written.
**	08-Jun-2011 (kiria01) b125435
**	    Chain OPERANDS on right hand side.
**	4-july-2011 (dougi) m2196
**	    Replace all pst_resolve()s with opa_gresolve()s.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of opa_gresolve reflecting
**	    pst_resolve change.
**/

/* Static variables and other such stuff. */

static DB_DATA_VALUE	tid_dv = {NULL, DB_TID8_LENGTH, DB_INT_TYPE, 0, -1};
static DB_ATT_ID	tid_att = { 0 };
static char		tid_name[] = {'t', 'i', 'd'};

static PST_QNODE *
opa_x100_stddev_pop_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead);

static PST_QNODE *
opa_x100_stddev_samp_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead);

static PST_QNODE *
opa_x100_var_pop_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead);

static PST_QNODE *
opa_x100_var_samp_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead);

static PST_QNODE *
opa_x100_any_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	**aopp,
	bool		byhead);


/*{
** Name: opa_x100_get_x_y	- generate subtree to get a x or y value
**
** Description:
**	Builds parse tree fragment needed to produce a valid x or y value
**	for binary aggregate functions (both x and y must be not null).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	aopp		ptr to binary agg AOP (and to its parms)
**	opp		ptr to x/y operand 
**
** Outputs:
**      none
**
**	Returns:
**	    ptr to parse tree fragment that produces a x or y
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocates returned tree fragment
**
** History:
**      19-oct-2009 (dougi)
**	    Written.
**	8-feb-2010 (dougi)
**	    Coerce to f8 - all x, y values in stat aggs must be f8.
**	10-Feb-2011 (kiria01)
**	    Ensure that opp is copied before returned.
*/
static PST_QNODE *
opa_x100_get_x_y(
	OPS_STATE	*global,
	PST_QNODE	*aopp,
	PST_QNODE	*opp)

{
    PST_QNODE		*xptr, *yptr, *nodep, *node1p, *casep, *whlistp, *andp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* X and y values for binary aggregates can only come from (x, y) pairs
    ** in which both values are not null. If x & y are declared not null, 
    ** we just return opp. If one of x or y are nullable, we return 
    ** "case when x/y is not null then x/y else null end". Otherwise we return
    ** "case when x is not null and y is not null then x/y else null end".
    */

    xptr = aopp->pst_left;
    yptr = aopp->pst_right;

    /* Both not nullable?? */
    if (xptr->pst_sym.pst_dataval.db_datatype > 0 &&
	yptr->pst_sym.pst_dataval.db_datatype > 0)
    {
	opv_copytree(global, &opp);
	return (opp);
    }

    /* Exactly one not nullable?? */
    if ((xptr->pst_sym.pst_dataval.db_datatype > 0 &&
	(nodep = yptr)->pst_sym.pst_dataval.db_datatype < 0) ||
	((nodep = xptr)->pst_sym.pst_dataval.db_datatype < 0 &&
	yptr->pst_sym.pst_dataval.db_datatype > 0))
    {
	opv_copytree(global, &nodep);
	casep = opv_casenode(global, PST_SEARCHED_CASE);
	casep->pst_left = whlistp = opv_whlistop(global);
	opv_copytree(global, &opp);			/* copy x/y */
	whlistp->pst_right->pst_left = opa_makeop(global, PST_UOP,
	    global->ops_cb->ops_server->opg_isnotnull, nodep, (PST_QNODE *) NULL);
	whlistp->pst_right->pst_right = opp;
	whlistp->pst_left = opv_whlistop(global);
	whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);

	opa_gresolve(global, &casep);
	return(casep);
    }

    /* This is the ugly case - both are nullable and we only return a value
    ** when both are not null. */
    nodep = xptr; node1p = yptr;
    opv_copytree(global, &nodep);
    opv_copytree(global, &node1p);

    /* "x is not null and y is not null" */
    andp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
		opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_isnotnull, nodep, (PST_QNODE *) NULL),
		opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_isnotnull, node1p, (PST_QNODE *) NULL));
    
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_right->pst_left = andp;
    opv_copytree(global, &opp);			/* copy x/y */
    whlistp->pst_right->pst_right = opp;
    whlistp->pst_left = opv_whlistop(global);
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);

    opa_gresolve(global, &casep);

    /* Check for f8 result and coerce it, if necessary. */
    if (abs(casep->pst_sym.pst_dataval.db_datatype) != DB_FLT_TYPE ||
	casep->pst_sym.pst_dataval.db_length < 8)
	casep = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
		opg_f8, casep, (PST_QNODE *) NULL);	/* coerce to f8 */

    return(casep);
}

/*{
** Name: opa_x100_count_x_y	- generate subtree to do count(x, y)
**
** Description:
**	Builds parse tree fragment needed to count rows in a group for 
**	binary aggregate functions
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	aopp		ptr to binary agg AOP (and to its parms)
**
** Outputs:
**      none
**
**	Returns:
**	    ptr to parse tree fragment that counts the rows
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocates returned tree fragment
**
** History:
**      19-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_count_x_y(
	OPS_STATE	*global,
	PST_QNODE	*aopp)

{
    PST_QNODE		*xptr, *yptr, *nodep, *node1p, *casep, *whlistp, *andp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* The count() for a binary aggregate is the count of non-null (x, y)
    ** pairs. If x & y are both not nullable, this is simply count(*). If 
    ** one is nullable, it is count(x) or count(y), whichever is the
    ** nullable parm. Otherwise it is "sum(case when x is not null and y
    ** is not null then 1 else 0 end)".
    */

    xptr = aopp->pst_left;
    yptr = aopp->pst_right;

    /* Both not nullable?? */
    if (xptr->pst_sym.pst_dataval.db_datatype > 0 &&
	yptr->pst_sym.pst_dataval.db_datatype > 0)
	return opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_scount, (PST_QNODE *) NULL, (PST_QNODE *) NULL);

    /* Exactly one not nullable?? */
    if ((xptr->pst_sym.pst_dataval.db_datatype > 0 &&
	(nodep = yptr)->pst_sym.pst_dataval.db_datatype < 0) ||
	((nodep = xptr)->pst_sym.pst_dataval.db_datatype < 0 &&
	yptr->pst_sym.pst_dataval.db_datatype > 0))
    {
	opv_copytree(global, &nodep);
	return opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_count, nodep, (PST_QNODE *) NULL);
    }

    /* This is the ugly case - both are nullable and we only count when both
    ** are not null. */
    nodep = xptr; node1p = yptr;
    opv_copytree(global, &nodep);
    opv_copytree(global, &node1p);

    /* "x is not null and y is not null" */
    andp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
		opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_isnotnull, nodep, (PST_QNODE *) NULL),
		opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_isnotnull, node1p, (PST_QNODE *) NULL));
    
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_right->pst_left = andp;
    whlistp->pst_right->pst_right = opv_intconst(global, 1);
    whlistp->pst_left = opv_whlistop(global);
    whlistp->pst_left->pst_right->pst_right = opv_intconst(global, 0);

    opa_gresolve(global, &casep);

    return(opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
		opg_sum, casep, (PST_QNODE *) NULL));
}

/*{
** Name: opa_x100_n_sum_x_y	- generate subtree to compute N * sum(x*y)
**
** Description:
**	Builds parse tree fragment needed to compute frequently occurring
**	subexpression used in binary aggregate functions. Note that the "x*y"
**	component may be x*y, X**2 or y**2.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to AGHEAD that will be copied for this fragment
**			[Will be relieved of its AOP]
**	nptr		ptr to tree fragment for N [Will be consumed]
**	xptr		ptr to tree fragment for x
**	yptr		ptr to tree fragment for y (may be same as x)
**	byhead		TRUE means grouped query (with PST_BYHEAD)
**
** Outputs:
**      none
**
**	Returns:
**	    ptr to parse tree fragment that computes the expression
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_n_sum_x_y (
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*nptr,
	PST_QNODE	*xptr,
	PST_QNODE	*yptr,
	bool		byhead)
{
    PST_QNODE	*wrk1p, *aghp;

    /* Start by copying PST_AGHEAD subtree. */
    aghp = nodep;
    if (byhead)
	aghp->pst_left->pst_right = (PST_QNODE *) NULL;
    else aghp->pst_left = (PST_QNODE *) NULL;

    opv_copytree(global, &aghp);

    /* Copy x, y make x*y, then sum that. */
    opv_copytree(global, &xptr);
    opv_copytree(global, &yptr);

    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
		xptr, yptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->opg_sum,
		wrk1p, (PST_QNODE *) NULL);

    /* Attach the sum to the AGHEAD. */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else
	aghp->pst_left = wrk1p;

    /* No need to copy N - it is for us alone - just multiply. */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
		nptr, aghp);

    return(wrk1p);


}

/*{
** Name: opa_x100_sum_x_sum_y	- generate subtree to compute sum(x) * sum(y)
**
** Description:
**	Builds parse tree fragment needed to compute frequently occurring
**	subexpression used in binary aggregate functions. Note that the 
**	"sum(x) * sum(y)" component may be sum(x)*sum(x), sum(x)*sum(y) or
**	sum(y)*sum(y)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to AGHEAD that will be copied for this fragment
**			[Will be relieved of its AOP]
**	xptr		ptr to tree fragment for x
**	yptr		ptr to tree fragment for y (may be same as x)
**	byhead		TRUE means grouped query (with PST_BYHEAD)
**
** Outputs:
**      none
**
**	Returns:
**	    ptr to parse tree fragment that computes the expression
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_sum_x_sum_y(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*xptr,
	PST_QNODE	*yptr,
	bool		byhead)
{
    PST_QNODE	*wrk1p, *wrk2p, *agh1p, *agh2p;

    /* Start by copying PST_AGHEAD subtree. */
    agh1p = nodep;
    if (byhead)
	agh1p->pst_left->pst_right = (PST_QNODE *) NULL;
    else agh1p->pst_left = (PST_QNODE *) NULL;

    opv_copytree(global, &agh1p);
    agh2p = agh1p;
    opv_copytree(global, &agh2p);		/* 1 for each sum() */

    /* Copy x, make sum(x), then copy y, make sum(y). */
    opv_copytree(global, &xptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->opg_sum,
		xptr, (PST_QNODE *) NULL);

    opv_copytree(global, &yptr);
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->opg_sum,
		yptr, (PST_QNODE *) NULL);

    /* Attach the sum's to the AGHEADs and multiply 'em. */
    if (byhead)
    {
	agh1p->pst_left->pst_right = wrk1p;
	agh2p->pst_left->pst_right = wrk2p;
    }
    else
    {
	agh1p->pst_left = wrk1p;
	agh2p->pst_left = wrk2p;
    }

    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
		agh1p, agh2p);

    return(wrk1p);
}

/*{
** Name: opa_x100_stddev_pop_xform	- turns stddev_pop() into an expression 
**	that X100 can process
**
** Description:
**	Expands stddev_pop(x) to sqrt(var_pop(x))
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the stddev_pop() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      8-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_stddev_pop_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*varpopp, *wrkp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* Just do var_pop() expansion and take its sqrt. */
    varpopp = opa_x100_var_pop_xform(global, nodep, aopp, byhead);
    wrkp = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
		opg_sqrt, varpopp, (PST_QNODE *) NULL);
    return(wrkp);
}

/*{
** Name: opa_x100_stddev_samp_xform	- turns stddev_samp() into an expression
**	that X100 can process
**
** Description:
**	Expands stddev_samp(x) to sqrt(var_samp(x))
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the stddev_samp() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      8-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_stddev_samp_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*varsampp, *wrkp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* Just do var_samp() expansion and take its sqrt. */
    varsampp = opa_x100_var_samp_xform(global, nodep, aopp, byhead);
    wrkp = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
		opg_sqrt, varsampp, (PST_QNODE *) NULL);
    return(wrkp);
}

/*{
** Name: opa_x100_var_pop_xform	- turns var_pop() into an expression that 
**	X100 can process
**
** Description:
**	Expands var_pop(x) to (sum(x*x)-sum(x)*sum(x)/count(x))/count(x).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the var_pop() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      8-oct-2009 (dougi)
**	    Written.
**	10-feb-2011 (dougi)
**	    Wrap abs() around computation to fix stddevs.
**	10-Feb-2011 (kiria01)
**	    Add missing tree copies for var_pop.
*/
static PST_QNODE *
opa_x100_var_pop_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *x1ptr, *wrk1p, *wrk2p, *wrk3p, *aghp;

    /* var_pop(x) expands to abs(sum(x*x)-sum(x)*sum(x)/count(x))/count(x).
    ** Do it all here. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */
    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_left;				/* get operand ptr */
    opv_copytree(global, &xptr);			/* copy 'em twice */
    if (abs(xptr->pst_sym.pst_dataval.db_datatype) != DB_FLT_TYPE ||
	xptr->pst_sym.pst_dataval.db_length < 8)
	xptr = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
			opg_f8, xptr, (PST_QNODE *) NULL); /* coerce to f8 */
    x1ptr = xptr;
    opv_copytree(global, &x1ptr);			
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, xptr, x1ptr);

    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    opv_copytree(global, &xptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk3p = wrk2p = aghp;
    opv_copytree(global, &wrk3p);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    xptr = aopp->pst_left;				/* get operand ptr */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_count, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);
    wrk1p = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
			opg_abs, wrk1p, (PST_QNODE *) NULL);
    opv_copytree(global, &wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk3p);

    return(wrk1p);

}

/*{
** Name: opa_x100_var_samp_xform	- turns var_samp() into an expression 
**	that X100 can process
**
** Description:
**	Expands var_samp(x) to case when count(x) <= 1 then NULL else 
**	(sum(x*x)-sum(x)*sum(x)/count(x))/(count(x)-1).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the var_pop() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      8-oct-2009 (dougi)
**	    Written.
**	09-Feb-2011 (kiria01) m1451
**	    Correct problem with overlapping parse trees breaking var_samp and
**	    stddev_samp.
**	10-feb-2011 (dougi)
**	    Wrap abs() around computation to fix stddevs.
*/
static PST_QNODE *
opa_x100_var_samp_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *x1ptr, *wrk1p, *wrk2p, *wrk3p, *cntp, *con1p, *casep, 
	*whlistp, *nullp, *aghp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* var_samp(x) expands to case when count(x) <= 1 then NULL else 
    ** abs(sum(x*x)-sum(x)*sum(x)/count(x))/(count(x)-1).
    ** Do it all here. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */
    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_left;				/* get operand ptr */
    opv_copytree(global, &xptr);			/* copy it */
    if (abs(xptr->pst_sym.pst_dataval.db_datatype) != DB_FLT_TYPE ||
	xptr->pst_sym.pst_dataval.db_length < 8)
	xptr = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
			opg_f8, xptr, (PST_QNODE *) NULL); /* coerce to f8 */
    x1ptr = xptr;
    opv_copytree(global, &x1ptr);			/* copy again */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, xptr, x1ptr);

    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    opv_copytree(global, &xptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk3p = wrk2p = aghp;

    opv_copytree(global, &wrk3p);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    xptr = aopp->pst_left;				/* get original opnd */
    cntp = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_count, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);			/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = cntp;
    else aghp->pst_left = cntp;
    cntp = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, cntp);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);
    wrk1p = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
			opg_abs, wrk1p, (PST_QNODE *) NULL);
    opv_copytree(global, &cntp);
    con1p = opv_intconst(global, 1);
    wrk3p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, cntp, con1p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk3p);

    /* Now wrap case around it. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    opv_copytree(global, &cntp);			/* copy count(x) */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP,
	global->ops_cb->ops_server->opg_le, cntp, con1p);
    nullp = opv_nullconst(global);
    whlistp->pst_right->pst_right = nullp;
    whlistp->pst_left = opv_whlistop(global);
    whlistp->pst_left->pst_right->pst_right = wrk1p;

    opa_gresolve(global, &casep);

    return(casep);

}

/*{
** Name: opa_x100_corr_xform	- turns corr() into an expression 
**	that X100 can process
**
** Description:
**	Builds parse tree fragment needed to count rows in a group to implement
**	corr(y, x)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the corr() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_corr_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *wrk4p, *wrk5p, *wrk6p,
		*n1ptr, *n2ptr, *n3ptr, *aghp, *casep, *whlistp;

    /* corr(y, x) expands to sqrt(power((count(y, x)*sum(x*y)-sum(x)*sum(y), 2)/
    **		((count(y, x)*sum(x**2)-sum(x)*sum(x))*
    **			(count(y, x)*sum(y**2)-sum(y)*sum(y))))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_right;				/* get operand ptrs */
    yptr = aopp->pst_left;

    /* Get a valid x & y (x & y both not null). */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* Make 2 count(y, x)s. */
    n1ptr = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = n1ptr;
    else aghp->pst_left = n1ptr;
    n1ptr = n2ptr = n3ptr = aghp;
    opv_copytree(global, &n2ptr);
    opv_copytree(global, &n3ptr);

    /* Generate "count*sum(x*y)",  "sum(x)*sum(y)", "count*sum(x**2)"
    ** "sum(x)*sum(x)", "count(y, x)*sum(y**2)" and "sum(y)*sum(y)". */
    wrk1p = opa_x100_n_sum_x_y(global, nodep, n1ptr, xptr, yptr, byhead);
    wrk2p = opa_x100_sum_x_sum_y(global, nodep, xptr, yptr, byhead);
    wrk3p = opa_x100_n_sum_x_y(global, nodep, n2ptr, xptr, xptr, byhead);
    wrk4p = opa_x100_sum_x_sum_y(global, nodep, xptr, xptr, byhead);
    wrk5p = opa_x100_n_sum_x_y(global, nodep, n3ptr, yptr, yptr, byhead);
    wrk6p = opa_x100_sum_x_sum_y(global, nodep, yptr, yptr, byhead);

    /* Generate the result sqrt(power((a - b), 2)/((c -d)*(e-f))). */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk1p, wrk2p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_pow,
			wrk1p, opv_intconst(global, 2));
    wrk3p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk3p, wrk4p);
    wrk5p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk5p, wrk6p);
    wrk4p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
			wrk3p, wrk5p);
    wrk4p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_div,
			wrk1p, wrk4p);
    wrk4p = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->opg_sqrt,
			wrk4p, (PST_QNODE *) NULL);

    /* Finally, wrap CASE around result to detect NULL result conditions. 
    ** Condition for non-null result is count > 0 and count*sum(x**2) !=
    ** sum(x)*sum(x) and count*sum(y**2) != sum(y)*sum(y). */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the denominators */
    opv_copytree(global, &wrk5p);		/* copy the denominators */
    opv_copytree(global, &n1ptr);		/* and the count */
    wrk1p = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ge, 
				n1ptr, opv_intconst(global, 1)),
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk3p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_left = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	wrk1p, opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk5p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_right = wrk4p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);



}

/*{
** Name: opa_x100_covar_pop_xform	- turns covar_pop() into an expression 
**	that X100 can process
**
** Description:
**	Expands covar_pop(x, y) to (sum(x*y)-sum(x)*sum(y)/
**					count(x, y))/count(x, y).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the covar_pop() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      17-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_covar_pop_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *aghp, *casep, *whlistp;

    /* covar_pop(x, y) expands to (sum(x*y)-sum(x)*sum(y)/
    **					count(x, y))/count(x, y).
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    yptr = aopp->pst_left;				/* get operand ptrs */
    xptr = aopp->pst_right;

    /* Get a valid x, y (x & y both not null). (allocated for us!) */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* sum(x * y) */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, xptr, yptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* sum(x) * sum(y) */
    opv_copytree(global, &xptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    opv_copytree(global, &yptr);			/* copy again */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, yptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    wrk3p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);
    opv_copytree(global, &wrk3p);		/* copy count(x, y) */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk3p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk3p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);


}

/*{
** Name: opa_x100_covar_samp_xform	- turns covar_samp() into an expression 
**	that X100 can process
**
** Description:
**	Expands covar_samp(x) to case when count(x, y) <= 1 then NULL else 
**	(sum(x*y)-sum(x)*sum(y)/count(x, y))/(count(x, y)-1).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the covar_samp() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      17-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_covar_samp_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *casep, *whlistp, *aghp;
    DB_STATUS	status;
    DB_ERROR	error;

    /* covar_samp(x, y) expands to case when count(x, y) <= 1 then NULL else 
    ** (sum(x*y)-sum(x)*sum(y)/count(x, y))/(count(x, y)-1).
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    yptr = aopp->pst_left;				/* get operand ptrs */
    xptr = aopp->pst_right;

    /* Get a valid x, y (x & y both not null). (allocated for us!) */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* sum(x * y) */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, xptr, yptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* sum(x) * sum(y) */
    opv_copytree(global, &xptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    opv_copytree(global, &yptr);			/* copy again */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, yptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    wrk3p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);
    opv_copytree(global, &wrk3p);		/* copy count(x, y) */

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk3p, opv_intconst(global, 1));	
						/* count(x, y) - 1 */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0 or 1. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy count(x, y) */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_gt, wrk3p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);


}

/*{
** Name: opa_x100_regr_avgx_xform	- turns regr_avgx() into an expression 
**	that X100 can process
**
** Description:
**	Expands regr_avgx(x, y) to sum(x)/count(x, y).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_avgx() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_avgx_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *wrk1p, *wrk2p, *aghp, *casep, *whlistp;

    /* regr_avgx(y, x) expands to (sum(x)/count(x, y))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_right;				/* get operand ptr */

    /* Get a valid x (x & y both not null). (allocated for us!) */
    xptr = opa_x100_get_x_y(global, aopp, xptr);

    /* sum(x) */
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* count(x, y) */
    wrk2p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk2p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk2p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);


}

/*{
** Name: opa_x100_regr_avgy_xform	- turns regr_avgy() into an expression 
**	that X100 can process
**
** Description:
**	Expands regr_avgy(x, y) to sum(y)/count(x, y).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_avgy() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_avgy_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*yptr, *wrk1p, *wrk2p, *aghp, *casep, *whlistp;

    /* regr_avgy(y, x) expands to (sum(y)/count(x, y))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    yptr = aopp->pst_left;				/* get operand ptr */

    /* Get a valid y (x & y both not null). */
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* sum(y) */
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, yptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* count(x, y) */
    wrk2p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk2p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk2p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);


}

/*{
** Name: opa_x100_regr_count_xform	- turns regr_count() into an expression 
**	that X100 can process
**
** Description:
**	Builds parse tree fragment needed to count rows in a group to implement
**	regr_count(x, y)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_count() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      4-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_count_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*wrkp;

    /* regr_count(x, y) is simply count(x, y). */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    wrkp = opa_x100_count_x_y(global, aopp);

    if (byhead)
	nodep->pst_left->pst_right = wrkp;
    else nodep->pst_left = wrkp;

    return(nodep);
}

/*{
** Name: opa_x100_regr_intercept_xform	- turns regr_intercept() into an 
**	expression that X100 can process
**
** Description:
**	Builds parse tree fragment needed to count rows in a group to implement
**	regr_intercept(y, x)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_intercept() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_intercept_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*x1ptr, *x2ptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *wrk4p,
		*nptr, *aghp, *casep, *whlistp;

    /* intercept(y, x) expands to: (sum(y)*sum(x**2)-sum(x)*sum(x*y)/
    **			(count(y, x)*sum(x**2)-sum(x)*sum(x))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    x1ptr = aopp->pst_right;				/* get operand ptrs */
    yptr = aopp->pst_left;

    /* Get a valid x & y (x & y both not null). */
    x1ptr = x2ptr = opa_x100_get_x_y(global, aopp, x1ptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);
    opv_copytree(global, &x2ptr);			/* duplicate */

    /* Make count(y, x). */
    nptr = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = nptr;
    else aghp->pst_left = nptr;
    nptr = aghp;

    /* Generate "x**2", "x*y", "sum(y)*sum(x**2)", "sum(x)*sum(x*y)", 
    ** "count*sum(x**2)" and "sum(x)*sum(x)". */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
			x1ptr, x2ptr);
    wrk1p = opa_x100_sum_x_sum_y(global, nodep, yptr, wrk1p, byhead);
    opv_copytree(global, &x1ptr);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
			x1ptr, yptr);
    wrk2p = opa_x100_sum_x_sum_y(global, nodep, x1ptr, wrk2p, byhead);
    wrk3p = opa_x100_n_sum_x_y(global, nodep, nptr, x1ptr, x1ptr, byhead);
    wrk4p = opa_x100_sum_x_sum_y(global, nodep, x1ptr, x1ptr, byhead);

    /* Generate the result (a - b)/(c -d). */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk1p, wrk2p);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk3p, wrk4p);
    wrk3p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_div,
			wrk1p, wrk2p);

    /* Finally, wrap CASE around result to detect NULL result conditions. 
    ** Condition for non-null result is count > 0 and count*sum(x**2) !=
    ** sum(x)*sum(x). */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk2p);		/* copy the denominator */
    opv_copytree(global, &nptr);		/* and the count */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ge, 
				nptr, opv_intconst(global, 1)),
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk2p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_right = wrk3p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);

}

/*{
** Name: opa_x100_regr_r2_xform	- turns regr_r2() into an expression that 
**	X100 can process
**
** Description:
**	Builds parse tree fragment needed to count rows in a group to implement
**	regr_r2(y, x)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_r2() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_r2_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *wrk4p, *wrk5p, *wrk6p,
		*n1ptr, *n2ptr, *n3ptr, *aghp, *casep, *whlistp;

    /* r2(y, x) expands to power((count(y, x)*sum(x*y)-sum(x)*sum(y), 2)/
    **		((count(y, x)*sum(x**2)-sum(x)*sum(x))*
    **			(count(y, x)*sum(y**2)-sum(y)*sum(y)))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_right;				/* get operand ptrs */
    yptr = aopp->pst_left;

    /* Get a valid x & y (x & y both not null). */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* Make 2 count(y, x)s. */
    n1ptr = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = n1ptr;
    else aghp->pst_left = n1ptr;
    n1ptr = n2ptr = n3ptr = aghp;
    opv_copytree(global, &n2ptr);
    opv_copytree(global, &n3ptr);

    /* Generate "count*sum(x*y)",  "sum(x)*sum(y)", "count*sum(x**2)"
    ** "sum(x)*sum(x)", "count(y, x)*sum(y**2)" and "sum(y)*sum(y)". */
    wrk1p = opa_x100_n_sum_x_y(global, nodep, n1ptr, xptr, yptr, byhead);
    wrk2p = opa_x100_sum_x_sum_y(global, nodep, xptr, yptr, byhead);
    wrk3p = opa_x100_n_sum_x_y(global, nodep, n2ptr, xptr, xptr, byhead);
    wrk4p = opa_x100_sum_x_sum_y(global, nodep, xptr, xptr, byhead);
    wrk5p = opa_x100_n_sum_x_y(global, nodep, n3ptr, yptr, yptr, byhead);
    wrk6p = opa_x100_sum_x_sum_y(global, nodep, yptr, yptr, byhead);

    /* Generate the result power((a - b), 2)/((c -d)*(e-f)). */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk1p, wrk2p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_pow,
			wrk1p, opv_intconst(global, 2));
    wrk3p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk3p, wrk4p);
    wrk5p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk5p, wrk6p);
    wrk4p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_mul,
			wrk3p, wrk5p);
    wrk4p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_div,
			wrk1p, wrk4p);

    /* Finally, wrap CASE around result to detect NULL result conditions. 
    ** Condition for non-null result is count > 0, count*sum(x**2) !=
    ** sum(x)*sum(x) and count*sum(y**2) != sum(y)*sum(y). */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the denominators */
    opv_copytree(global, &wrk5p);		/* copy the denominators */
    opv_copytree(global, &n1ptr);		/* and the count */
    wrk1p = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ge, 
				n1ptr, opv_intconst(global, 1)),
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk3p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_left = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	wrk1p, opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk5p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_right = wrk4p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);

}

/*{
** Name: opa_x100_regr_slope_xform	- turns regr_r2() into an expression 
**	that X100 can process
**
** Description:
**	Builds parse tree fragment needed to count rows in a group to implement
**	regr_slope(y, x)
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_slope() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_slope_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *wrk4p,
		*n1ptr, *n2ptr, *aghp, *casep, *whlistp;

    /* slope(y, x) expands to (count(y, x)*sum(x*y)-sum(x)*sum(y)/
    **			(count(y, x)*sum(x**2)-sum(x)*sum(x))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    xptr = aopp->pst_right;				/* get operand ptrs */
    yptr = aopp->pst_left;

    /* Get a valid x & y (x & y both not null). */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* Make 2 count(y, x)s. */
    n1ptr = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = n1ptr;
    else aghp->pst_left = n1ptr;
    n1ptr = n2ptr = aghp;
    opv_copytree(global, &n2ptr);

    /* Generate "count*sum(x*y)", "count*sum(x**2)", "sum(x)*sum(y)" and
    ** "sum(x)*sum(x)". */
    wrk1p = opa_x100_n_sum_x_y(global, nodep, n1ptr, xptr, yptr, byhead);
    wrk2p = opa_x100_n_sum_x_y(global, nodep, n2ptr, xptr, xptr, byhead);
    wrk3p = opa_x100_sum_x_sum_y(global, nodep, xptr, yptr, byhead);
    wrk4p = opa_x100_sum_x_sum_y(global, nodep, xptr, xptr, byhead);

    /* Generate the result (a - b)/(c -d). */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk1p, wrk3p);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_sub,
			wrk2p, wrk4p);
    wrk3p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_div,
			wrk1p, wrk2p);

    /* Finally, wrap CASE around result to detect NULL result conditions. 
    ** Condition for non-null result is count > 0 and count*sum(x**2) !=
    ** sum(x)*sum(x). */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk2p);		/* copy the denominator */
    opv_copytree(global, &n1ptr);		/* and the count */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ge, 
				n1ptr, opv_intconst(global, 1)),
	opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne, 
				wrk2p, opv_intconst(global, 0)));
    whlistp->pst_right->pst_right = wrk3p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);
}

/*{
** Name: opa_x100_regr_sxx_xform	- turns regr_sxx() into an expression 
**	that X100 can process
**
** Description:
**	Expands regr_sxx(y, x) to (sum(x*x) - sum(x)*sum(x)/count(x, y)).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_sxx() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_sxx_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*x1ptr, *x2ptr, *wrk1p, *wrk2p, *wrk3p, *aghp, *casep, *whlistp;

    /* regr_sxx(y, x) expands to (sum(x*x)-sum(x)*sum(x)/count(x, y))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    x1ptr = aopp->pst_right;				/* get operand ptr */

    /* Get a valid x (x & y both not null). (allocated for us!) */
    x1ptr = x2ptr = opa_x100_get_x_y(global, aopp, x1ptr);

    /* sum(x * x) */
    opv_copytree(global, &x2ptr);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, x1ptr, x2ptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* sum(x) * sum(x) */
    opv_copytree(global, &x1ptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, x1ptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    opv_copytree(global, &x2ptr);			/* copy again */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, x2ptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    /* count(x, y) */
    wrk3p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk3p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);

}

/*{
** Name: opa_x100_regr_sxy_xform	- turns regr_sxy() into an expression 
**	that X100 can process
**
** Description:
**	Expands regr_sxy(y, x) to (sum(x*y) - sum(x)*sum(y)/count(x, y)).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_sxy() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_sxy_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*xptr, *yptr, *wrk1p, *wrk2p, *wrk3p, *aghp, *casep, *whlistp;

    /* regr_sxx(y, x) expands to (sum(x*y)-sum(x)*sum(y)/count(x, y))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    yptr = aopp->pst_left;				/* get operand ptrs */
    xptr = aopp->pst_right;

    /* Get a valid x, y (x & y both not null). (allocated for us!) */
    xptr = opa_x100_get_x_y(global, aopp, xptr);
    yptr = opa_x100_get_x_y(global, aopp, yptr);

    /* sum(x * y) */
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, xptr, yptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* sum(x) * sum(y) */
    opv_copytree(global, &xptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, xptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    opv_copytree(global, &yptr);			/* copy again */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, yptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    /* count(x, y) */
    wrk3p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk3p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);

}

/*{
** Name: opa_x100_regr_syy_xform	- turns regr_syy() into an expression 
**	that X100 can process
**
** Description:
**	Expands regr_syy(y, x) to (sum(y*y) - sum(y)*sum(y)/count(x, y)).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the regr_syy() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-feb-2010 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_regr_syy_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	*aopp,
	bool		byhead)

{
    PST_QNODE	*y1ptr, *y2ptr, *wrk1p, *wrk2p, *wrk3p, *aghp, *casep, *whlistp;

    /* regr_syy(y, x) expands to (sum(y*y)-sum(y)*sum(y)/count(x, y))
    ** NOTE: in the binary aggregates, the dependent variable (y) is the
    ** first parm and the independent variable (x) is the 2nd parm. */

    /* Set attach point for AOPs to NULL (for copying).They're ugly because
    ** each AOP must be under a AGHEAD and (possibly) a BYHEAD (if there's 
    ** grouping. */

    if (byhead)
	nodep->pst_left->pst_right = (PST_QNODE *) NULL;
    else nodep->pst_left = (PST_QNODE *) NULL;

    y1ptr = aopp->pst_left;				/* get operand ptr */

    /* Get a valid x (x & y both not null). (1 allocated for us!) */
    y1ptr = y2ptr = opa_x100_get_x_y(global, aopp, y1ptr);

    /* sum(x * x) */
    opv_copytree(global, &y2ptr);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, y1ptr, y2ptr);
    wrk1p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, wrk1p, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk1p;
    else aghp->pst_left = wrk1p;
    wrk1p = aghp;

    /* sum(x) * sum(x) */
    opv_copytree(global, &y1ptr);			/* copy again */
    wrk2p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, y1ptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk2p;
    else aghp->pst_left = wrk2p;
    wrk2p = aghp;

    opv_copytree(global, &y2ptr);			/* copy again */
    wrk3p = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_sum, y2ptr, (PST_QNODE *) NULL);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_mul, wrk2p, wrk3p);

    /* count(x, y) */
    wrk3p = opa_x100_count_x_y(global, aopp);
    aghp = nodep;
    opv_copytree(global, &aghp);		/* copy the AGHEAD subtree */
    if (byhead)
	aghp->pst_left->pst_right = wrk3p;
    else aghp->pst_left = wrk3p;
    wrk3p = aghp;

    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_div, wrk2p, wrk3p);
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_sub, wrk1p, wrk2p);

    /* After all that, we must wrap a case around it to return NULL when the 
    ** count is 0. */
    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    opv_copytree(global, &wrk3p);		/* copy the AGHEAD subtree */
    whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP, 
	global->ops_cb->ops_server->opg_ge, wrk3p, opv_intconst(global, 1));
    whlistp->pst_right->pst_right = wrk1p;
    whlistp->pst_left->pst_right->pst_right = opv_nullconst(global);
    return(casep);


}

/*{
** Name: opa_x100_any_xform	- turns any() into expression that X100 can do
**
** Description:
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to PST_AGHEAD that owns the any() PST_AOP
**	aopp		ptr to the PST_AOP
**	byhead		TRUE means grouped query (with PST_BYHEAD)
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
**	Allocates returned tree fragment
**
** History:
**      5-oct-2009 (dougi)
**	    Written.
*/
static PST_QNODE *
opa_x100_any_xform(
	OPS_STATE	*global,
	PST_QNODE	*nodep,
	PST_QNODE	**aopp,
	bool		byhead)

{
    PST_QNODE	*casep, *bopp, *whlistp, *constp;
    DB_STATUS	status;
    DB_ERROR	error;


    /* Must turn this into a "case" that looks like "case when count(x) > 0
    ** then 1 else 0 end", where we are transforming "any(x)". */

    casep = opv_casenode(global, PST_SEARCHED_CASE);
    casep->pst_left = whlistp = opv_whlistop(global);
    whlistp->pst_left = opv_whlistop(global);
    whlistp->pst_right->pst_left = opv_opnode(global, PST_BOP, global->ops_cb->
		ops_server->opg_ge, (OPL_IOUTER) PST_NOJOIN);
    whlistp->pst_right->pst_right = opv_intconst(global, 1);
    whlistp->pst_right->pst_left->pst_right = whlistp->pst_right->pst_right;
    whlistp->pst_right->pst_left->pst_left = nodep;
    (*aopp)->pst_sym.pst_value.pst_s_op.pst_opno = global->ops_cb->
						ops_server->opg_count;
    whlistp->pst_left->pst_right->pst_right = opv_intconst(global, 0);

    /* Resolve the "count(x)". */
    opa_gresolve(global, aopp);

    /* Resolve the ">" predicate. */
    opa_gresolve(global, &whlistp->pst_right->pst_left);

    /* Resolve the "case". */
    opa_gresolve(global, &casep);

    return(casep);
}

/*{
** Name: opa_x100_xform	- looks for agg funcs to transform
**
** Description:
**      This function traverses the subquery structures looking for
**	aggregate subqueries. Then it descends the parse tree looking for
**	AOP nodes that are transformable (currently any() and the stat aggs).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**
** Outputs:
**      none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	May rearrange tree and allocate extra nodes.
**
** History:
**      5-oct-2009 (dougi)
**	    Written.
**	3-nov-2009 (dougi)
**	    Expanded to support selective transform of other functions (e.g.
**	    2-op substring(), ANSI trim()).
**	14-Feb-2011 (kiria01) m1477
**	    Add missing handling for unions & remove most of the recursion.
**	3-Mar-2011 (kschendel) m1475
**	    OPF doesn't need ifnull in general any more, fetch it here.
*/
VOID
opa_x100_xform(
	OPS_STATE	*global,
	PST_QNODE	**nodep)

{
    PST_QNODE	*aopp;
    bool	byhead = FALSE;

    /* Recurse and switch through the parse tree, looking for AOPs to check. */
    while (*nodep)
    {
	PST_QNODE *node = *nodep;
	switch (node->pst_sym.pst_type)
	{
	case PST_SUBSEL:
	case PST_ROOT:
	    if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		opa_x100_xform(global,
			&node->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
	    break;
	case PST_AGHEAD:
	    /* This sits atop the PST_AOP that we're really interested in. */
	    if (!((byhead = node->pst_left->pst_sym.pst_type == PST_BYHEAD) ||
		  (aopp = node->pst_left)->pst_sym.pst_type == PST_AOP))
		break;
	    if (byhead && (aopp = node->pst_left->pst_right)->
				    pst_sym.pst_type != PST_AOP)
		break;

	    /* Check for the AOPs we want - any() and the stat aggs. */
	    switch (aopp->pst_sym.pst_value.pst_s_op.pst_opno)
	    {
	    case ADI_ANY_OP:
		*nodep = opa_x100_any_xform(global, node, &aopp, byhead);
		break;
	    case ADI_CORR_OP:
		*nodep = opa_x100_corr_xform(global, node, aopp, byhead);
		break;
	    case ADI_COVAR_POP_OP:
		*nodep = opa_x100_covar_pop_xform(global, node, aopp, byhead);
		break;
	    case ADI_COVAR_SAMP_OP:
		*nodep = opa_x100_covar_samp_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_AVGX_OP:
		*nodep = opa_x100_regr_avgx_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_AVGY_OP:
		*nodep = opa_x100_regr_avgy_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_COUNT_OP:
		/* Wrap ifnull(..., 0) around count to produce non-null result. */
		{
		    ADF_CB adfcb;
		    ADI_OP_ID ifnull_op;
		    MEfill(sizeof(ADF_CB), 0, &adfcb);
		    adfcb.adf_qlang = DB_SQL;
		    (void) adi_opid(&adfcb, "ifnull", &ifnull_op);
		    *nodep = opa_makeop(global, PST_BOP, ifnull_op,
			opa_x100_regr_count_xform(global, node, aopp, byhead),
			opv_dblconst(global, (f8)0.0));
		}
		break;
	    case ADI_REGR_INTERCEPT_OP:
		*nodep = opa_x100_regr_intercept_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_R2_OP:
		*nodep = opa_x100_regr_r2_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_SLOPE_OP:
		*nodep = opa_x100_regr_slope_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_SXX_OP:
		*nodep = opa_x100_regr_sxx_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_SXY_OP:
		*nodep = opa_x100_regr_sxy_xform(global, node, aopp, byhead);
		break;
	    case ADI_REGR_SYY_OP:
		*nodep = opa_x100_regr_syy_xform(global, node, aopp, byhead);
		break;
	    case ADI_STDDEV_POP_OP:
		*nodep = opa_x100_stddev_pop_xform(global, node, aopp, byhead);
		break;
	    case ADI_STDDEV_SAMP_OP:
		*nodep = opa_x100_stddev_samp_xform(global, node, aopp, byhead);
		break;
	    case ADI_VAR_POP_OP:
		*nodep = opa_x100_var_pop_xform(global, node, aopp, byhead);
		break;
	    case ADI_VAR_SAMP_OP:
		*nodep = opa_x100_var_samp_xform(global, node, aopp, byhead);
		break;
	    }
	    break;

	case PST_BOP:
	    /* Check for the various BOPs of interest. */
	    switch (node->pst_sym.pst_value.pst_s_op.pst_opno)
	    {
	    case ADI_ATRIM_OP:
		{
		    i4	trimop, trimchar;
		    u_i1	*wrkp;

		    /* The following all need to be (and should be) true. */
		    if (node->pst_right->pst_sym.pst_type != PST_CONST ||
			node->pst_right->pst_sym.pst_dataval.db_datatype != DB_CHA_TYPE ||
			node->pst_right->pst_sym.pst_dataval.db_length < 2)
			break;

		    /* Turn the ANSI trim() into a ternary function for X100. */
		    node->pst_sym.pst_type = PST_MOP;
		    wrkp = (u_i1 *)node->pst_right->pst_sym.pst_dataval.db_data;
		    trimop = wrkp[0];
		    trimchar = wrkp[1];

		    /* Reconstitute parms as PST_OPERAND nodes under PST_MOP. */
		    node->pst_right = opv_operand(global, opv_intconst(global, trimop));
		    node->pst_right->pst_right = opv_operand(global,
						    opv_intconst(global, trimchar));
		}
		break;

	    default:
		break;
	    }
	    break;
	case PST_CONST:
	    /* Avoid uselessly recursing potential INLIST fragments etc. */
	    return;
	default:
	    break;
	}
	if (node->pst_left)
	{
	    if (node->pst_right)
		opa_x100_xform(global, &node->pst_right);
	    nodep = &node->pst_left;
	    continue;
	}
	else if (node->pst_right)
	{
	    nodep = &node->pst_right;
	    continue;
	}
	else
	    break;
    }
}
