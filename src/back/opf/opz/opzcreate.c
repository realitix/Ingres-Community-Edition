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
#include    <ade.h>
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

/* external routine declarations definitions */
#define        OPZ_CREATE      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPZCREATE.C - create and initialize joinop attribute table
**
**  Description:
**      Routines to create and initialize joinop attribute table
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	 2-sep-97 (hayke02)
**	    In opz_ctree() and opz_or() we now check for a NULL opl_gbase
**	    when an outer join is defined and an AND or OR has been found.
**	    This situation can arise if a view, which is partially or wholly
**	    based on a select from another view, is outer joined. This change
**	    fixes bug 83772.
**      10-Dec-98 (hanal04)
**          In opz_ctree() do not call opz_bop() for BOP nodes that
**          have equal left and right nodes except for nullability
**          caused by outer-join. b94310.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of opz_bop and opz_ctree to support
**	    the changes in opa_resolve reflecting pst_resolve change.
**/

/* TABLE OF CONTENTS */
static void opz_or(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
static void opz_bop(
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep);
static void opz_ojvmap(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep,
	OPV_GRV *grvp,
	OPV_BMVARS *vmap,
	OPV_IVARS joinopvar);
static void opv_tidview(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep,
	OPV_IVARS joinopvar,
	bool boolinit);
static void opz_varfixup(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
static void opz_var(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
static void opz_ctree(
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep);
static void opz_corelated(
	OPS_SUBQUERY *subquery,
	OPV_IVARS joinopvar);
static void opz_parmmap(
	OPS_SUBQUERY *subquery,
	OPV_BMVARS *mapptr,
	PST_QNODE *nodep);
void opz_create(
	OPS_SUBQUERY *subquery);

/*{
** Name: opz_or	- process OR node for outer joins
**
** Description:
**      This routine will only be called if there is an outer join defined in the 
**      query and an OR has been found.  When a variable is referenced outside
**	the scope of the ON clause in the outer join in which the variable is defined
**	then it causes the outer join to "degenerate" unless the variable is found
**	in an "IS NULL" or is part of an "OR".  In the case of an "OR" only if it
**	is visible on both sides of the tree, will it cause the outer join to
**	degenerate.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to OR node
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-nov-89 (seputis)
**          initial creation
**	8-jan-03 (inkdo01)
**	    Fix 0395's from OJs nested inside unflattened views.
**	30-april-2007 (dougi) Bug 120270
**	    Apply view restriction to derived tables and with clauses, too.
*/
static VOID
opz_or(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *nodep)
{
    OPV_BMVARS	    right_map;
    OPV_BMVARS	    *original_map;
    OPV_BMVARS	    previous_map;
    bool	    oj_found;
    bool	    view = FALSE;

    if (subquery->ops_gentry >= 0 &&
	subquery->ops_gentry < subquery->ops_global->ops_qheader->pst_rngvar_count &&
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry] &&
	(subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_RTREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_WETREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_DRTREE))
	view = TRUE;			/* TRUE if sq is unflattened view */

    if ((nodep->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN)
	&&
	subquery->ops_global->ops_goj.opl_gbase)
    {
	/* rename OP nodes to local subquery join ids */
	if (nodep->pst_sym.pst_value.pst_s_op.pst_joinid 
	    > subquery->ops_global->ops_goj.opl_glv)
	    opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
	nodep->pst_sym.pst_value.pst_s_op.pst_joinid = 
	    subquery->ops_global->ops_goj.opl_gbase->
	    opl_gojt[nodep->pst_sym.pst_value.pst_s_op.pst_joinid];
				    /* joinid has already been processed */
	if (!view && nodep->pst_sym.pst_value.pst_s_op.pst_joinid < 0)
	    opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
    }
    else
	nodep->pst_sym.pst_value.pst_s_op.pst_joinid = OPL_NOOUTER;
    subquery->ops_oj.opl_cojid = nodep->pst_sym.pst_value.pst_s_op.pst_joinid;
				/* save the ID so that the variable
				** map for outer joins can be updated */
    oj_found = (subquery->ops_global->ops_goj.opl_mask & OPL_OJFOUND);
    if (oj_found)
    {
	if (subquery->ops_oj.opl_cojid == OPL_NOOUTER)
	    original_map = subquery->ops_oj.opl_where;
	else
	    original_map = subquery->ops_oj.opl_base
		->opl_ojt[subquery->ops_oj.opl_cojid]->opl_onclause;
	MEcopy((PTR)original_map, sizeof(previous_map), (PTR)&previous_map);
    }
    if (nodep->pst_right)
	opz_ctree( subquery, &nodep->pst_right); /* rename right side */
    if (oj_found)
    {
	MEcopy((PTR)original_map, sizeof(right_map), (PTR)&right_map);
	MEfill(sizeof(*original_map), (u_char)0, (PTR)original_map);
    }
    if (nodep->pst_left)
	opz_ctree( subquery, &nodep->pst_left); /* rename right side */
    if (oj_found)
    {
	BTand((i4)BITS_IN(right_map), (char *)&right_map, (char *)original_map);
				    /* only if both left and right subtrees have
				    ** the variables referenced, can this variable
				    ** be considered to cause a degenerate join */
	BTor((i4)BITS_IN(previous_map), (char *)&previous_map, (char *)original_map);
    }
}

/*{
** Name: opz_bop	- process bop node 
**
** Description:
**      This routine will avoid updating the outer join maps for implicit joins 
**      such as those created for aggregate linkbacks 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to implicitly generated node
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-may-90 (seputis)
**          initial creation
**      26-apr-93 (ed)
**          correct outer join problem with correlated queries as in
**	    select * from r left join s on r.a=s.a
**		where 1 = (select avg(t.b) from t where t.c=s.c)
**	    - s.c is not nullable, but on the correlation after the outer
**	    join s.c can become nullable, so special handling is 
**	    required for this case.
**	8-jan-03 (inkdo01)
**	    Fix 0395's from OJs nested inside unflattened views.
**	8-sep-04 (inkdo01)
**	    Add chack for opl_gbase (not always there for outer/inner join
**	    syntax involving views.
**	30-april-2007 (dougi) Bug 120270
**	    Apply view restriction to derived tables and with clauses, too.
**	15-Feb-2011 (kschendel)
**	    Delete some ancient #if-0'ed outer join code, causes confusion.
**	8-july-2011 (dougi) m2232
**	    Tolerate "numeric_norm" UOP under BOP being tested for correlations
**	    under OJs.
*/
static VOID
opz_bop(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **nodep)
{
    bool	    view = FALSE;
    PST_QNODE	*node = *nodep;
    if (subquery->ops_gentry >= 0 &&
	subquery->ops_gentry < subquery->ops_global->ops_qheader->pst_rngvar_count &&
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry] &&
	(subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_RTREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_WETREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_DRTREE))
	view = TRUE;			/* TRUE if sq is unflattened view */

    if (node->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN
	&&
	subquery->ops_global->ops_goj.opl_gbase)
    {
	/* rename OP nodes to local subquery join ids */
	if (node->pst_sym.pst_value.pst_s_op.pst_joinid 
	    > subquery->ops_global->ops_goj.opl_glv)
	    opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
	node->pst_sym.pst_value.pst_s_op.pst_joinid = 
	    subquery->ops_global->ops_goj.opl_gbase->
	    opl_gojt[node->pst_sym.pst_value.pst_s_op.pst_joinid];
				    /* joinid has already been processed */
	if (!view && node->pst_sym.pst_value.pst_s_op.pst_joinid < 0)
	    opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
    }
    else
	node->pst_sym.pst_value.pst_s_op.pst_joinid = OPL_NOOUTER;
    subquery->ops_oj.opl_cojid = node->pst_sym.pst_value.pst_s_op.pst_joinid;
				/* save the ID so that the variable
				** map for outer joins can be updated */
/* This code needs to avoid updating the outer join maps for implicit joins since
** queries like 
**	select * from temp left join iirelation r1 on temp.a1=reltups where
**      temp.a1 = (select count(r2.reltid) from iirelation r2 where r2.reltups=r1.reltups)
** would be considered to be a degenerate join since implicit link backs of agg
** temps are in the where clause
*/
    subquery->ops_oj.opl_smask |= OPL_IMPVAR; /* this flag prevents the 
					    ** ON clause maps from being updated
					    ** for implicit joins */
    if (node->pst_right)
	opz_ctree( subquery, &node->pst_right); /* rename right side */
    if (node->pst_left)
	opz_ctree( subquery, &node->pst_left); /* rename left side */
    subquery->ops_oj.opl_smask &= (~OPL_IMPVAR); /* trun off flag which prevents the 
					    ** ON clause maps from being updated
					    ** for implicit joins */
    if (subquery->ops_oj.opl_base)
    {
	OPV_BMVARS		**aggmapp;  /* ptr to ptr to map to update
				** with vars which could possibily cause
				** correlated problems */
	OPL_OUTER		*outerp;
	OPV_BMVARS		comap;	    /* map of variables which are
					    ** corelated within this aggregate
					    */
	OPS_SUBQUERY		*agg_subquery;
	PST_QNODE		*varp;

	/* check if link back variable is part of an outer join */
	if (subquery->ops_oj.opl_cojid == OPL_NOOUTER)
	    aggmapp = &subquery->ops_oj.opl_agscope;
	else
	{   /* for corelated aggregates in subselects */
	    outerp = subquery->ops_oj.opl_base
		->opl_ojt[subquery->ops_oj.opl_cojid];
	    aggmapp = &outerp->opl_agscope;
	}
	if (!(*aggmapp))
	{
	    *aggmapp= (OPV_BMVARS *)opu_memory(subquery->ops_global,
		(i4)sizeof(**aggmapp));
	    MEfill(sizeof(**aggmapp), (u_char)0, (char *)*aggmapp);
	}
	MEfill(sizeof(comap), (u_char)0, (PTR)&comap);
	opv_mapvar(node, (OPV_GBMVARS *)&comap); /* get map of vars in 
					    ** implicit linkback, to be used
					    ** to determine if a retry is
					    ** necessary after eliminating 
					    ** degenerate joins,... if a var
					    ** which exists as the inner of
					    ** an outer join is found then
					    ** a retry is necessary */
	BTor((i4)BITS_IN(**aggmapp), (char *)&comap, (char *)(*aggmapp));

	/* create map of correlated variables for this aggregate which
	** will help in finding cases of degenerate outer joins */
	if (node->pst_left->pst_sym.pst_type == PST_VAR)
	    varp = node->pst_left;
	else if (node->pst_left->pst_sym.pst_type == PST_UOP &&
	    node->pst_left->pst_sym.pst_value.pst_s_op.pst_opno ==
					ADI_NUMNORM_OP &&
	    node->pst_left->pst_left->pst_sym.pst_type == PST_VAR)
	    varp = node->pst_left->pst_left;
	else opx_error(E_OP04AD_CORRELATED); /* expecting var node reference to
					    ** temporary */
	agg_subquery = subquery->ops_vars.opv_base->opv_rt[varp->
	    pst_sym.pst_value.pst_s_var.pst_vno]->opv_grv->opv_gquery;
	if (!agg_subquery)
	    opx_error(E_OP04AD_CORRELATED);
	if (agg_subquery->ops_oj.opl_parent != subquery)
	{
	    agg_subquery->ops_oj.opl_parent = subquery;
	    MEcopy((PTR)&comap, sizeof(agg_subquery->ops_oj.opl_implicit),
		(PTR)&agg_subquery->ops_oj.opl_implicit); /* initialize if
					    ** first time for this subquery */
	}
	else
	    BTor((i4)BITS_IN(comap), (char *)&comap, 
		(char *)&agg_subquery->ops_oj.opl_implicit);

    }

    {   /* only the link-back of the aggregate resdoms is interesting
	** so only one level of resolution is needed for the optimization
	*/
	opa_resolve(subquery, nodep);
    }
}

/*{
** Name: opz_ojvmap	- update map of visible variables
**
** Description:
**	Very often a correlated aggregate would have the property that if the
**	correlated value is NULL then the aggregate result would be NULL
**	this routine will try to establish is this is true for any variables
**	and update the bit map appropriately.
**
**      Also update the map of variables associated with an outer join id and
**	the aggregate, such that if an attribute could be NULL due to the
**	outer join semantics and the attribute is correlated into the aggregate.
**	This case is important since the calculation of the aggregate is such that
**	the NULL may not be in the BY list, so that the link back of the aggregate
**	result would be incorrect.
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      grvp                            ptr to global variable representing subquery
**					which is possibly correlated
**      vmap                            map of variables to update with those that
**					would imply a NULL in the aggregate result
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-apr-90 (seputis)
**          initial creation
**      19-apr-93 (ed)
**          - find implicit correlations by use opz_bop and the PST_IMPLICIT
**	    flag in the PST_BOP_NODE
[@history_line@]...
[@history_template@]...
*/
static VOID
opz_ojvmap(
	OPS_SUBQUERY       *subquery,
	PST_QNODE	   *nodep,
	OPV_GRV            *grvp,
	OPV_BMVARS         *vmap,
	OPV_IVARS	   joinopvar)
{
    OPS_SUBQUERY	*agg_subquery;
    PST_QNODE		*qnodep;
    OPZ_IATTS		attr;
    agg_subquery = grvp->opv_gquery;
    /* check for the count aggregate, since a NULL
    ** would produce a 0 instead of a NULL*/

    attr = subquery->ops_attrs.opz_base->opz_attnums[nodep->pst_sym.
	pst_value.pst_s_var.pst_atno.db_att_id]->opz_attnm.db_att_id;
				/* translate joinop to temp table attribute
				** number */
    /* local range variables are referenced in this case */
    for (qnodep = agg_subquery->ops_root->pst_left;
	qnodep 
	&& 
	(qnodep->pst_sym.pst_type == PST_RESDOM) 
	&&
	(qnodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno != attr);
	qnodep = qnodep->pst_left)
    {
    }
    if (!qnodep || (qnodep->pst_sym.pst_type != PST_RESDOM))
	opx_error(E_OP068F_MISSING_RESDOM);		/* missing resdom */
    if (qnodep->pst_right->pst_sym.pst_type == PST_AOP)
    {
	ADI_OP_ID	    opno;
	opno = qnodep->pst_right->pst_sym.pst_value.pst_s_op.pst_opno;
	if ((opno == subquery->ops_global->ops_cb->ops_server->opg_count)
	    || 
	    (opno == subquery->ops_global->ops_cb->ops_server->opg_scount))
	    return;		    /* count aggregate will always return
				** 0, even if the where clause has
				** a NULL, FIXME this is hardcoded here
				** and in ADF, need a function instance
				** mask for this */
    }
    else
	return;		    /* not an AOP so probably an implicit 
				** join back */
    {
	OPV_BMVARS	bylistmap;

	MEfill(sizeof(bylistmap), 0, (PTR)&bylistmap);
	opv_mapvar(agg_subquery->ops_byexpr, (OPV_GBMVARS *)&bylistmap);

	if (BTsubset((char *)&bylistmap,
		(char *)agg_subquery->ops_oj.opl_where, 
		(i4)sizeof(*agg_subquery->ops_oj.opl_where))
	    ||
	    (	(agg_subquery->ops_vars.opv_prv == 1)
		&&
		(agg_subquery->ops_mask & OPS_COAGG) /* special case of
				** a variable elimination, since only one
				** variable exists in query, it must have
				** been the one which was correlated, this
				** should eventually cause an OP000A retry
				** but before then a degenerate outer join
				** may be found via the correlation */
	    ))
	    BTor((i4)sizeof(*vmap), (char *)&agg_subquery->ops_oj.opl_implicit,
		(char *)vmap);
    }
}

/*{
** Name: opv_tidview	- create a function attribute for the TID
**
** Description:
**      The routine creates a function attribute for the TID attribute
**	associated with the view.  A TID attribute for a view should
**	only be created if duplicate handling is required, not for tid
**	joins 
**
** Inputs:
**      subquery                        ptr to subquery being referenced
**      nodep                           ptr to parse tree node of TID
**      grvp                            ptr to VIEW global range variable
**      joinopvar                       local range variable number
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**	20-oct-92 (ed)
**	    - remove prototypes for unix
**	23-oct-93 (ed)
**	    - bug 54818 - incorrect duplicate handling when union view and 
**	    subselects are used
**	3-Jun-2009 (kschendel) b122118
**	    Keep junk out of collID's.  Make static, only called locally.
[@history_template@]...
*/
static void
opv_tidview(
		OPS_SUBQUERY	*subquery,
		PST_QNODE	*nodep,
		OPV_IVARS	joinopvar,
		bool		boolinit)
{
    OPV_VARS		*varp;
    OPZ_IATTS		attr;       /* joinop function attribute */
    varp = subquery->ops_vars.opv_base->opv_rt[joinopvar];
    if (varp->opv_mask & OPV_TIDVIEW)
    {	/* function attribute has already been defined so
	** lookup this value and substitute the local atno */
	attr = varp->opv_viewtid;
    }
    else
    {	/* create a function attribute which references the
	** new self-modifying ADF function */
	OPZ_FATTS	*func_attr; /* ptr to function attribute used
				    ** to contain constant node */
	PST_QNODE	*const_ptr; /* ptr to constant node */
	PST_QNODE	*incint_ptr; /* ptr to incint function node */
	DB_DATA_VALUE   datavalue;
	OPS_STATE	*global;

	global = subquery->ops_global;
	datavalue.db_datatype = DB_INT_TYPE;
	datavalue.db_length = 4;
	datavalue.db_prec = 0;
	datavalue.db_collID = DB_NOCOLLATION;
	datavalue.db_data = NULL;
	const_ptr = opv_constnode(global, &datavalue, ++global->ops_parmtotal);
	if (global->ops_procedure->pst_isdbp)
	{
	    ops_decvar(global, const_ptr); /* create a variable
				    ** declaration which can be used for
				    ** the puesdo-tid */
	}
	else
	{
	    PST_QNODE		*andp;
	    OPS_RQUERY		*rqueryp;
	    /* repeat query parameters are not initialized so add a
	    ** constant qualification to be executed which will initialize
	    ** this variable */
	    for (rqueryp = global->ops_rqlist; rqueryp;
		rqueryp = rqueryp->ops_rqnext)
		if (rqueryp->ops_rqnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no
		    == const_ptr->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		    rqueryp->ops_rqmask |= OPS_RQSPECIAL; /* mark
				    ** repeat query parameter as special so
				    ** OPC does not get confused */
	    andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, (OPL_IOUTER)PST_NOJOIN);
	    andp->pst_left = opv_opnode(global, PST_BOP, global->ops_cb->
		ops_server->opg_iirow_count, (OPL_IOUTER)PST_NOJOIN);
	    andp->pst_left->pst_right = opv_intconst(global, 0); /* initialize
				    ** row count increment counter */
	    andp->pst_left->pst_left = opv_opnode(global, PST_BOP, ADI_ADD_OP,
		(OPL_IOUTER)PST_NOJOIN);
	    andp->pst_left->pst_left->pst_left = const_ptr;
	    andp->pst_left->pst_left->pst_right = opv_i1const(global, 1); 
				    /* parameter indicates initialization 
				    ** should occur */
	    opa_resolve(subquery, &andp->pst_left->pst_left);
	    opa_resolve(subquery, &andp->pst_left);
	    if (boolinit)
	    {	/* attach to list of constants to be evaluated once */
		andp->pst_right = subquery->ops_bfs.opb_bfconstants;
		subquery->ops_bfs.opb_bfconstants = andp;
	    }
	    else
	    {
		andp->pst_right = subquery->ops_root->pst_right;
		subquery->ops_root->pst_right = andp;
	    }
	}
        incint_ptr = opv_opnode(subquery->ops_global, PST_BOP, 
	    global->ops_cb->ops_server->opg_iirow_count, (OPL_IOUTER)PST_NOJOIN);
	incint_ptr->pst_left = const_ptr;
	incint_ptr->pst_right =  opv_i1const(global, 0); /* this causes
				    ** rowcount increment counter to increment
				    ** and not initialize */
	func_attr = opz_fmake( subquery, incint_ptr, OPZ_SVAR);
	opa_resolve(subquery, &incint_ptr);
	STRUCT_ASSIGN_MACRO( incint_ptr->pst_sym.pst_dataval, 
	    func_attr->opz_dataval);
	attr = opz_ftoa( subquery, func_attr, joinopvar, incint_ptr,
	    &func_attr->opz_dataval, OPZ_SVAR);
	varp->opv_viewtid = attr;
	varp->opv_mask |= OPV_TIDVIEW;
	
    }
    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id = attr;
}

/*{
** Name: opz_varfixup   - fixup datatype on var nodes
**
** Description:
**      This routine should be called if the aggregate temporary had
**      a variable substitution optimization, and the equi-join attributes
**      have different types.  The correct datatype is copied into the
**      var node.
**
** Inputs:
**      subquery                        ptr to subquery of aggregate
**      nodep                           ptr to var node of parent
**
** Outputs:
**
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      5-jan-93 (ed)
**          initial creation for bug 47541, 47912 - used to copy
**          changed datatype from aggregate temp to var node in parent
[@history_template@]...
*/
static VOID
opz_varfixup(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *nodep)
{
    PST_QNODE       *resdomp;
    for (resdomp = subquery->ops_root->pst_left;
        resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
        resdomp = resdomp->pst_left)
    {   /* find correct resdom for attribute */
        if (nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
            ==
            resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
        {   /* copy datatype in case change has been made */
            STRUCT_ASSIGN_MACRO(resdomp->pst_sym.pst_dataval,
                nodep->pst_sym.pst_dataval); /* copy datatype
                                    ** which should match underlying
                                    ** resdom of temporary */
            return;
        }
    }
    opx_error(E_OP028A_MISSING_CORELATION);
}

/*{
** Name: opz_var	- process var node
**
** Description:
**      process var node, assign joinop attribute number and variable 
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      nodep                           ptr to var node to process
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-apr-93 (ed)
**          initial creation
**	13-mar-97 (inkdo01)
**	    Flag joinop attrs referenced in where/on clauses.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
**	30-april-2007 (dougi) Bug 120270
**	    Apply view restriction to derived tables and with clauses, too.
**	29-aug-2008 (dougi)
**	    Copy pst_atname from matching RESDOM for X100 queries when VAR
**	    references a result column of another SUBQUERY.
**	8-dec-2008 (dougi)
**	    One last heroic attempt to copy a fabricated RESDOM name to the
**	    corresponding PST_VAR (for X100, mostly).
*/
static VOID
opz_var(subquery, nodep)
OPS_SUBQUERY       *subquery;
PST_QNODE          *nodep;
{	/* found a var node so create a joinop attribute and relabel node */
    OPV_IVARS              joinopvar;
    OPV_GRV                *grvp;	    /* ptr to global range variable */
    OPV_IGVARS             grv;         /* global range var number of
					** subselect */
    PST_QNODE		   *rsdmp;
    DB_ATTNUM		   saveattid = nodep->pst_sym.pst_value.pst_s_var.
					pst_atno.db_att_id;
    bool		   x100 = (subquery->ops_global->ops_gmask & OPS_X100)
					? TRUE : FALSE;

    if ((grv = (OPV_IGVARS)nodep->pst_sym.pst_value.pst_s_var.pst_vno) < 0 ||
	    !(grvp = subquery->ops_global->ops_rangetab.opv_base->opv_grv[grv]))
	return;

    joinopvar = subquery->ops_global->ops_rangetab.opv_lrvbase[grv];
				    /* get local range variable number*/
    if ((nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == DB_IMTID)
	&&
	(subquery->ops_mask & OPS_TIDVIEW)
	&&
	(grvp->opv_qrt != OPV_NOGVAR)
	&&
	(subquery->ops_global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype
	==
	PST_DRTREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype
	==
	PST_WETREE ||
	subquery->ops_global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype
	==
	PST_RTREE))			    /* this is a view which does not
					** have a TID attribute so create
					** a function attribute which 
					** can be used to define duplicate
					** handling similiar to a TID */
	opv_tidview(subquery, nodep, joinopvar, FALSE);
    else if (grvp->opv_gsubselect)
    {   /* special handling required for subselect substitutions */
	OPV_SUBSELECT	*selectp;   /* ptr to subselect node being
					** analyzed */
	bool            found;          /* consistency check boolean */
	OPV_VARS        *varp;          /* Joinopvar range variable ptr */

	found = FALSE;
	varp = subquery->ops_vars.opv_base->opv_rt[joinopvar];
	for (selectp = varp->opv_subselect;
	    selectp; selectp = selectp->opv_nsubselect)
	{	/* traverse list of subselects referenced in subquery and
	    ** check if a joinop attribute has already been assigned */
	    if (selectp->opv_sgvar == grv)
	    {   /* select has been found */
		found = TRUE;
		if (grvp->opv_created == OPS_SELECT)
		{	/* assign a new joinop attribute to this node */
		    if (selectp->opv_atno == OPZ_NOATTR)
		    {
			selectp->opv_atno = opz_addatts(subquery, joinopvar,
			    OPZ_SNUM, &nodep->pst_sym.pst_dataval);
		    }
		    subquery->ops_attrs.opz_base->
			opz_attnums[selectp->opv_atno]->opz_gvar = grv; /*
					** since subselects may have
					** multiple global range variables
					** mapped into one local range
					** variable, make sure the
					** correct global var is referenced
					** in the joinop attribute element*/
		}
		else
		{	/* assign a new joinop attribute to this node */
		    OPZ_DMFATTR		dmfattr;
		    OPV_GRV		*jvgrv = varp->opv_grv;

		    dmfattr = nodep->pst_sym.pst_value.pst_s_var.
			    pst_atno.db_att_id; /* function aggregate */
		    if ((   (jvgrv->opv_created == OPS_FAGG)
			    ||
			    (jvgrv->opv_created == OPS_HFAGG)
			    ||
			    (jvgrv->opv_created == OPS_RFAGG)
			)
			&&
			(jvgrv->opv_gquery->ops_mask & OPS_VOPT)
			)
		    {
			opz_varfixup(jvgrv->opv_gquery, nodep);
		    }
		    selectp->opv_atno = opz_addatts(subquery, joinopvar,
			dmfattr, &nodep->pst_sym.pst_dataval);
		    subquery->ops_attrs.opz_base->
			opz_attnums[selectp->opv_atno]->opz_gvar = grv; /*
					** since subselects may have
					** multiple global range variables
					** mapped into one local range
					** variable, make sure the
					** correct global var is referenced
					** in the joinop attribute element*/
		}
		nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		    = selectp->opv_atno;
		break;		    
	    }
	}
	/* For X100, we need a column name for PST_VARs that map to
	** RESDOMs in some other subquery - look for it here. */
	if (x100)
	 for (rsdmp = grvp->opv_gsubselect->opv_subquery->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM; 
		rsdmp = rsdmp->pst_left)
	  if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno == saveattid)
	  {
	    /* Got a match - copy pst_rsname to the VAR node. */
	    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			sizeof(DB_ATT_NAME), 
		(char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname);
	    break;
	  }
#ifdef E_OP038B_NO_SUBSELECT
	if (!found)
	    opx_error(E_OP038B_NO_SUBSELECT);
#endif
    }
    else
    {
	/* If this is still sq, look for RESDOM for a name to copy. */
	if (grvp->opv_gquery)
	 for (rsdmp = grvp->opv_gquery->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	  if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 
			nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	  {
	    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			sizeof(DB_ATT_NAME), 
		    (char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname);
	    break;
	  }

	nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
	    = opz_addatts( subquery, joinopvar,
		nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, 
		&nodep->pst_sym.pst_dataval);
    }

    nodep->pst_sym.pst_value.pst_s_var.pst_vno = joinopvar; /* change the 
				** global range 
				** variable number to the joinop 
				** range variable number */
    if (subquery->ops_global->ops_goj.opl_mask & OPL_QUAL)
    {
	OPZ_ATTS	*attrp;
	
	attrp = subquery->ops_attrs.opz_base->opz_attnums[nodep->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	attrp->opz_mask |= OPZ_QUALATT; /* flag joinop attrs in where/on
				** clauses (for index substitution logic) */
    }
    if ((subquery->ops_global->ops_goj.opl_mask & OPL_OJFOUND)
	&&
	(subquery->ops_global->ops_goj.opl_mask & OPL_QUAL) /* only 
				** include variables referenced
				** in qualification since only these
				** effect degenerate joins */
	&&
	!(subquery->ops_oj.opl_smask & (OPL_ISNULL | OPL_IMPVAR |
					OPL_CASEOP))
	)
    {	/* call BT routines only if there exists 
	** the possibility of an outer join, but do not
	** include variables which are defined within the
	** scope of an IS NULL, since this does not cause outer
	** joins to degenerate */
	OPV_BMVARS	    *vmap;

	if (subquery->ops_oj.opl_cojid == OPL_NOOUTER ||
		subquery->ops_oj.opl_base == (OPL_OJT *) NULL)
	    vmap = subquery->ops_oj.opl_where;
	else
	    vmap = subquery->ops_oj.opl_base
		->opl_ojt[subquery->ops_oj.opl_cojid]->opl_onclause;
	if (!grvp->opv_relation
	    &&
	    (	(grvp->opv_created == OPS_FAGG)
		||
		(grvp->opv_created == OPS_RSAGG)
		||
		(grvp->opv_created == OPS_HFAGG)
		||
		(grvp->opv_created == OPS_RFAGG)
	    )
	    &&
	    (	grvp->opv_gquery->ops_oj.opl_parent == subquery) /* this
				** indicates at least one implicit
				** joinback was found */
	    )
	    /* look at aggregate or subselect and determine
	    ** which variables, if referenced would return
	    ** a NULL if the correlated parameter from that
	    ** variable is NULL */
	    opz_ojvmap(subquery, nodep, grvp, vmap, joinopvar);
	else
	    BTset((i4)joinopvar, (char *)vmap);
    }
}

/*{
** Name: opz_ctree	- create joinop attribute and relabel query tree
**
** Description:
**      Create a joinop attribute for each attribute referenced in the 
**      subquery and relabel the query tree to use joinop range variables 
**      and joinop attribute numbers
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**      subquery                        ptr to subquery being analyzed
**          .opz_attrs                  joinop attributes created
**          .ops_root                   query tree relabelled to use
**                                      joinop attributes and joinop
**                                      range variables
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jun-86 (seputis)
**          initial creation from cpytree in cpytree.c
**      18-sep-92 (ed)
**          - bug 44850 - changed mechanism for global to local mapping
**	20-apr-93 (ed)
**	    - changed test for special case functions which convert NULLs
**	    into non-null results to use adi_npre_instr which is more
**	    general than testing for each particular case.
**      10-Dec-98 (hanal04)
**          Aggregate functions performed on results of views containing
**          outer joins may lead to a BOP node where the left and right
**          nodes are the nullable/non-nullable versions of the same
**          attribute from the same relation. Do not call opz_bop() for
**          BOP nodes where this is the case. b94310.
**	25-nov-02 (inkdo01)
**	    Add code to treat case expressions like "is null" in outer
**	    join queries - that is, verify that they don't depend on the
**	    OJ evaluation before they are evaluated before placement.
**	2-dec-02 (inkdo01)
**	    Different logic required for case expressions. New flag is 
**	    introduced to augment change above.
**	8-jan-03 (inkdo01)
**	    Fix 0395's from OJs nested inside unflattened views.
**	9-jan-04 (inkdo01)
**	    Ignore PST_CONSTs to prevent recursion on long IN-list 
**	    generated chains.
**	8-sep-04 (inkdo01)
**	    Add chack for opl_gbase (not always there for outer/inner join
**	    syntax involving views.
**	14-apr-05 (hayke02)
**	    Switch off OPS_COLLATION in ops_mask for CASE statements
**	    (PST_CASEOP) to prevents unnecessary extra attributes due to the
**	    fix for 112873, INGSRV 2940. This change fixes bug 114308, problem
**	    INGSRV 3253.
**	 8-nov-05 (hayke02)
**	    Switch on OPS_VAREQVAR when we are processing an ADI_EQ_OP PST_BOP
**	    node with PST_VAR nodes left and right. This change fixes bug
**	    115420 problem INGSRV 3465.
**	21-mar-06 (hayke02)
**	    We now do not set OPL_ISNULL for ifnull() functions in outer
**	    join ON clauses. This allows the correct vars to be set in
**	    opl_onclause, and prevents E_OP0482/SEGV in oph_ojsvar(). This
**	    change fixes bug 115757.
**	20-oct-06 (hayke02)
**	    Add new boolean ops_vareqvar, and set it TRUE when OPS_VAREQVAR
**	    is set. We then unset OPS_VAREQVAR when we return from opz_ctree()
**	    and ops_vareqvar is TRUE. This prevents OPS_VAREQVAR reamining set
**	    when we are dealing with non-OPS_VAREQVAR PST_BOP nodes and their
**	    child nodes. This change fixes bug 116868.
**	30-april-2007 (dougi) Bug 120270
**	    Apply view restriction to derived tables and with clauses, too.
**      06-oct-2008 (huazh01)
**          Back out the fix to bug 115757 and let the fix to bug 117793 
**          handles it. This fixes bug 121014. 
**	3-Mar-2011 (kschendel) b125146
**	    Make sure mops are handled like other operators.  We need the
**	    null-ignoring test for e.g. NVL2.
**	11-mar-2011 (dougi)
**	    Trap X100 queries with pseudoTID requirements.
**      02-may-2011 (huazh01)
**          remove the fix to b114308, b112873, b115420, and b116868. (b125322)
*/
static VOID
opz_ctree(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **nodep)
{
    OPS_STATE	    *global = subquery->ops_global;
    bool	    view = FALSE;

    if (subquery->ops_gentry >= 0 &&
	subquery->ops_gentry < global->ops_qheader->pst_rngvar_count &&
	global->ops_qheader->pst_rangetab[subquery->ops_gentry] &&
	(global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_RTREE ||
	global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_WETREE ||
	global->ops_qheader->pst_rangetab[subquery->ops_gentry]->
		pst_rgtype == PST_DRTREE))
	view = TRUE;			/* TRUE if sq is unflattened view */

    /* avoid creating local variables if possible since this will affect the
    ** depth of the recursion 
    */
    for (;*nodep;) 
    {	/* use iteration on the right side, for long resdom lists */
	PST_QNODE *node = *nodep;
	switch (node->pst_sym.pst_type)
	{
	case PST_VAR:
	{   
	    opz_var(subquery, node);
	    return;
	}
	case PST_CONST:
	    return;	/* CONST's are uninteresting, but might have
			** long pst_left IN-list induced chains */
	case PST_BOP:
	{
	    if (node->pst_sym.pst_value.pst_s_op.pst_flags & PST_IMPLICIT)
	    {
		/*
		** Note that PST_IMPLICIT bops are equals nodes introduced
		** by opa...not user query bops.
		** b94310 - Do not call opz_bop() is cases where the left
		** and right nodes are the same attribute. Caused by
		** nullable/non-nullable columns from outer join.
		*/
		if(!((node->pst_right->pst_sym.pst_type == PST_VAR) &&
                 (node->pst_left->pst_sym.pst_type == PST_VAR) &&
                 (node->pst_right->pst_sym.pst_value.pst_s_var.pst_vno ==
                  node->pst_left->pst_sym.pst_value.pst_s_var.pst_vno) &&
                 (node->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
                 node->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)))
 
		{
		    opz_bop(subquery, nodep); /* do not update the opl_where
                                            ** clause when joins are added
                                            ** implicitly */
		    return;
		}

	    }
	    /* FALL THRU */
	}
	case PST_MOP:
	case PST_UOP:
	{
	    if ((   node->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_npre_instr
		    == 
		    ADE_0CXI_IGNORE
		)
		&&
		(   global->ops_cb->ops_server->opg_isnotnull
		    !=
		    node->pst_sym.pst_value.pst_s_op.pst_opno
		))
	    {	/* special case handling for ISNULL/IFNULL/IFTRUE since inner join
		** values which are defaulted are NULL and thus this
		** boolean factor when referenced in an ON clause
		** cannot change the characteristics of the outer join 
		** i.e. degenerate a left join to an inner join */
		subquery->ops_oj.opl_smask |= OPL_ISNULL; /* mark this
					** subtree as referencing an ISNULL
					** because then variable referenced
					** can take on NULL values and not
					** eliminate an outer join */
		if (node->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN
		&&
		global->ops_goj.opl_gbase)
		{
		    /* rename OP nodes to local subquery join ids */
		    if (node->pst_sym.pst_value.pst_s_op.pst_joinid 
			> global->ops_goj.opl_glv)
			opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
		    node->pst_sym.pst_value.pst_s_op.pst_joinid = 
			global->ops_goj.opl_gbase->
			opl_gojt[node->pst_sym.pst_value.pst_s_op.pst_joinid];
						/* joinid has already been processed */
		    if (!view && node->pst_sym.pst_value.pst_s_op.pst_joinid < 0)
			opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
		}
		else
		    node->pst_sym.pst_value.pst_s_op.pst_joinid = OPL_NOOUTER;
		subquery->ops_oj.opl_cojid = node->pst_sym.pst_value.pst_s_op.pst_joinid;
					    /* save the ID so that the variable
					    ** map for outer joins can be updated */
		if (node->pst_left)
		    opz_ctree( subquery, &node->pst_left); /* rename left side */
		if (node->pst_right)
		    opz_ctree( subquery, &node->pst_right); /* rename right side */
		subquery->ops_oj.opl_cojid = node->pst_sym.pst_value.pst_s_op.pst_joinid;
					/* save the ID so that the variable
					** map for outer joins can be updated */
		subquery->ops_oj.opl_smask &= (~OPL_ISNULL);	/* reset the ISNULL flag */
		return;
	    }
	}
	    /* FALL THRU */
	case PST_OR:
	{
	    if ((global->ops_goj.opl_mask & OPL_OJFOUND)
		&&
		(node->pst_sym.pst_type == PST_OR))
	    {	/* handle special case of ORs, which do not update
		** outer join variable maps */
		opz_or(subquery, node);
		return;
	    }
	    /* FALL THRU */
	}
	case PST_AND:
	case PST_AOP:
	case PST_COP:
	{
	    if (global->ops_goj.opl_mask & OPL_OJFOUND)
	    {   /* process all the PST_OP_NODE types for outer join */
		if ((node->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN)
		    &&
		    global->ops_goj.opl_gbase)
		{
		    /* rename OP nodes to local subquery join ids */
		    if (node->pst_sym.pst_value.pst_s_op.pst_joinid 
			> global->ops_goj.opl_glv)
			opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
		    node->pst_sym.pst_value.pst_s_op.pst_joinid = 
			global->ops_goj.opl_gbase->
			opl_gojt[node->pst_sym.pst_value.pst_s_op.pst_joinid];
						/* joinid has already been processed */
		    if (!view && node->pst_sym.pst_value.pst_s_op.pst_joinid < 0)
			opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
		}
		else
		    node->pst_sym.pst_value.pst_s_op.pst_joinid = OPL_NOOUTER;
		subquery->ops_oj.opl_cojid = node->pst_sym.pst_value.pst_s_op.pst_joinid;
					    /* save the ID so that the variable
					    ** map for outer joins can be updated */
		if (global->ops_goj.opl_mask & OPL_QUAL)
		{
		    if (node->pst_left)
			opz_ctree( subquery, &node->pst_left); /* rename left side */
		}
		else
		{
		    if (node->pst_right)
			opz_ctree( subquery, &node->pst_right); /* rename right side */
		}
		subquery->ops_oj.opl_cojid = node->pst_sym.pst_value.pst_s_op.pst_joinid;
					/* save the ID so that the variable
					** map for outer joins can be updated */
		break;
	    }
	}
	case PST_RESDOM:
	{
	    PST_QNODE	*node1p;
	    OPV_GRV	*grvp;
	    OPV_IGVARS	grv;
	    DB_ATT_ID	attid;

	    if ((global->ops_gmask & OPS_X100) &&
		(subquery->ops_mask & OPS_TIDVIEW) &&
		(node1p = node->pst_right)->pst_sym.pst_type == PST_VAR &&
		node1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 
								DB_IMTID &&
		(grv = (OPV_IGVARS)node1p->pst_sym.pst_value.pst_s_var.
							pst_vno) >= 0 &&
		(grvp = global->ops_rangetab.opv_base->opv_grv[grv]) !=
							(OPV_GRV *) NULL &&
		grvp->opv_qrt > OPV_NOGVAR &&
		(global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype ==
							PST_RTREE ||
		global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype ==
							PST_DRTREE ||
		global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgtype ==
							PST_WETREE))
	    {
		/* If all those tests succeed, we may have a pseudoTID in a VW
		** query and the parse tree must be fiddled to generate the
		** right stuff for the cross compiler.
		**
		** But first verify that the virtual table involves more 
		** than 1 table or at least 1 other virtual table. */
		if (grvp->opv_gquery->ops_vars.opv_rv > 1 ||
		    (grvp->opv_gquery->ops_vars.opv_rv > 0 &&
		    grvp->opv_gquery->ops_vars.opv_base->opv_rt[0]->
			opv_grv->opv_gquery != (OPS_SUBQUERY *) NULL))
		{
		    node->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |=
							PST_RS_PSEUDOTID;
		    grvp->opv_gquery->ops_mask2 |= OPS_X100_TIDVIEW;
		}
		break;		/* exit switch */
	    }
	    /* Otherwise, drop into "default". */
	}
	default:
	{
	    if (node->pst_sym.pst_type == PST_CASEOP && 
		!(subquery->ops_oj.opl_smask & OPL_CASEOP))
	    {
		/* "Case" expressions have the same outer join implications as 
		** "is null" predicates. They may need to be evaluated after 
		** the outer join, so should not be used to determine 
		** whether an OJ can be degenerated. This code checks for 
		** case expressions not already inside another case and then
		** behaves like an "is null". This fixes bug 109174. */
		subquery->ops_oj.opl_smask |= OPL_CASEOP; /* mark this
					** subtree as referencing a case expr
					** because the variable referenced
					** can take on NULL values and not
					** eliminate an outer join */

		if (node->pst_left)
		    opz_ctree( subquery, &node->pst_left); /* rename left side */
		if (node->pst_right)
		    opz_ctree( subquery, &node->pst_right); /* rename right side */
		subquery->ops_oj.opl_smask &= (~OPL_CASEOP);	/* reset the CASEOP flag */

		return;
	    }
	    if (global->ops_goj.opl_mask & OPL_QUAL)
	    {
		if (node->pst_left)
		    opz_ctree( subquery, &node->pst_left); /* rename left side */
	    }
	    else
	    {
		if (node->pst_right)
		    opz_ctree( subquery, &node->pst_right); /* rename right side */
	    }
	}
	}
	if (global->ops_goj.opl_mask & OPL_QUAL)
	    nodep = &node->pst_right;
	else
	    nodep = &node->pst_left;
    }
}

/*{
** Name: opz_corelated	- create local corelated descriptor list
**
** Description:
**      This procedure will make a copy of the corelated attribute
**      descriptor list found in the global range variable associated with
**      the subselect, and use local range variable numbers and joinop
**      attribute numbers.
**
** Inputs:
**      subquery                        subquery being processed
**      joinopvar                       local range var representing
**                                      the subselect to convert
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
**      22-mar-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opz_corelated(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          joinopvar)
{
    OPV_SUBSELECT       *subp; /* current subselect being analyzed */

    for (subp = subquery->ops_vars.opv_base->opv_rt[joinopvar]->opv_subselect;
	subp;
	subp = subp->opv_nsubselect)
    {
	OPV_SEPARM	*gparmp;    /* ptr to subselect corelated descriptor
				    ** associated with global range variable
				    */
	OPV_GRV         *grvp;      /* ptr to global range variable
				    ** representing the subselect */

	grvp = subquery->ops_global->ops_rangetab.opv_base->
	    opv_grv[subp->opv_sgvar];
	for (gparmp = grvp->opv_gsubselect->opv_parm;
	    gparmp;
	    gparmp = gparmp->opv_senext)
	{
	    OPV_SEPARM	*localparm; /* ptr to local corelated descriptor */
	    i4		varnodesize; /* size in bytes of a var node */
	    PST_QNODE   *nodep;     /* ptr to newly created var node
                                    ** representing local range var 
                                    ** joinop attribute of corelated
                                    ** variable */
	    OPV_IVARS   vno;        /* local range var of correlated parameter
                                    */
	    localparm = (OPV_SEPARM *) opu_memory(subquery->ops_global, 
		(i4)sizeof(OPV_SEPARM));
	    localparm->opv_senext = subp->opv_parm;
	    subp->opv_parm = localparm;	/* add to linked list */
	    localparm->opv_seconst = gparmp->opv_seconst; /* use same
				    ** constant node since values will not
                                    ** change */
	    varnodesize = sizeof(PST_QNODE)
		    + sizeof(PST_VAR_NODE)
		    - sizeof(PST_SYMVALUE);

	    nodep =
	    localparm->opv_sevar = (PST_QNODE *)opu_memory(subquery->ops_global,
		varnodesize);
	    MEcopy((PTR)gparmp->opv_sevar, varnodesize,
		    (PTR) nodep);
            vno = subquery->ops_global->ops_rangetab.opv_lrvbase
                [nodep->pst_sym.pst_value.pst_s_var.pst_vno];
	    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id 
		= opz_addatts( subquery, vno,
		    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, 
		    &nodep->pst_sym.pst_dataval); /* get or create a joinop
					    ** attribute */
	    nodep->pst_sym.pst_value.pst_s_var.pst_vno = vno; /* 
					    ** assign the joinop 
					    ** range variable number */
	    
	}
    }
}

/*{
** Name: opz_parmmap	- set varno's into TPROC parmmap's
**
** Description:
**      Scan parm spec looking for PST_VAR nodes and set corresponding
**	varno into proc's parmmap.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**	mapptr				ptr to TPROC parmmap
**      nodep                           ptr to parm spec subtree
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-may-2008 (dougi)
**	    Initially written for table procedures.
*/
static VOID
opz_parmmap(
    OPS_SUBQUERY   *subquery,
    OPV_BMVARS	   *mapptr,
    PST_QNODE      *nodep)

{

    /* Recurse down left, iterate down right, switching on node type to
    ** locate PST_VAR nodes. */

    for ( ; nodep != (PST_QNODE *) NULL; )
     switch (nodep->pst_sym.pst_type) {

      case PST_VAR:
	BTset (nodep->pst_sym.pst_value.pst_s_var.pst_vno, (char *)mapptr);
	return;

      default:
	/* For everything else, just process left/right subtrees. */
	if (nodep->pst_left)
	    opz_parmmap(subquery, mapptr, nodep->pst_left);
	nodep = nodep->pst_right;
	break;
    }

    return;
}

/*{
** Name: opz_create	- create the joinop attr and function attr tables
**
** Description:
**      The procedure will initialize the joinop attributes table variables and
**      function attributes table variables.  This routine needs to be called
**      prior to any references to these tables.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**
** Outputs:
**      subquery->ops_attrs             joinop attribute structure initialized
**      subquery->ops_funcs             joinop function attribute structure 
**                                      initialized
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation
**	24-jul-88 (seputis)
**          init opz_fmask, partial fix for b6570
**	26-apr-96 (inkdo01)
**	    relocated func attr init stuff for bug 76134.
**	24-may-96 (kch)
**	    The initialization of the function attributes array
**	    (subquery->ops_funcs) has been moved to before the calls to
**	    opz_ctree(). This prevents a SEGV in opz_fmake() when funcp is
**	    assigned to subquery->ops_funcs.opz_fbase->opz_fatts[].. This
**	    change fixes bug 76684.
**	 6-sep-04 (hayke02)
**	    Switch on OPS_COLLATION during traversal of left sub-tree (resdom
**	    list) if a collation sequence is present. This will cause
**	    OPZ_COLLATT attribtues to be created, and prevent incorrect results
**	    (see problem INGSRV 2450, bug 110675). This change fixes problem
**	    INGSRV 2940, bug 112873.
**	5-may-2008 (dougi)
**	    Add support for table procedures - scan parm specs.
*/
VOID
opz_create(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;		    /* global state variable */

    subquery->ops_attrs.opz_av = 0;	    /* initialize number
                                            **  of joinop attributes assigned
                                            */
    global = subquery->ops_global;	    /* ptr to global state variable */
    if (global->ops_mstate.ops_tat)
    {
	subquery->ops_attrs.opz_base = global->ops_mstate.ops_tat; /* use full size OPZ_AT
                                            ** structure if available */
    }
    else
    {
	global->ops_mstate.ops_tat =
	subquery->ops_attrs.opz_base = (OPZ_AT *) opu_memory( 
	    global, (i4) sizeof(OPZ_AT) ); /* get memory for 
                                            ** array of ptrs to joinop 
                                            ** attributes */
    }

    /* initialize the function attributes array */
    subquery->ops_funcs.opz_fv = 0;         /* initialize number
                                            **  of function attributes assigned
                                            */
    subquery->ops_funcs.opz_fmask = 0;	    /* initialize mask of booleans for
					    ** function attributes */
    if (global->ops_mstate.ops_tft)
    {
	subquery->ops_funcs.opz_fbase = global->ops_mstate.ops_tft; /* use full size OPZ_FT
                                            ** structure if available */
    }
    else
    {
	global->ops_mstate.ops_tft =
	subquery->ops_funcs.opz_fbase =(OPZ_FT *) opu_memory( 
	    global, (i4) sizeof(OPZ_FT) ); /* get memory for 
                                            ** array of pointers to function 
                                            ** attributes
                                            */
    }

    if (global->ops_goj.opl_mask & OPL_OJFOUND)
	opl_ojinit(subquery);		    /* initialize some structures
					    ** prior to gathering data about the
					    ** attributes for outer joins */
    global->ops_goj.opl_mask |= OPL_QUAL;   /* the qualification is being
					    ** processed */
    opz_ctree(subquery, &subquery->ops_root->pst_right); /* create joinop attributes and
                                            ** relabel var nodes to use
                                            ** joinop range variables and
                                            ** joinop attribute numbers
                                            */
    global->ops_goj.opl_mask &= (~OPL_QUAL); /* the target list is being
					    ** processed */

    opz_ctree(subquery, &subquery->ops_root->pst_left); /* create joinop attributes and
                                            ** relabel var nodes to use
                                            ** joinop range variables and
                                            ** joinop attribute numbers
                                            */

    /* Look for table procedures (if there) and process their parm specs. */
    if (subquery->ops_mask2 & OPS_TPROC)
    {
	OPV_VARS	*varp;
	i4	i;

	/* Loop over range table looking for tprocs. */
	for (i = 0; i < subquery->ops_vars.opv_rv; i++)
	 if ((varp = subquery->ops_vars.opv_base->opv_rt[i])
					->opv_mask & OPV_LTPROC)
	 {
	    opz_ctree(subquery, &global->ops_qheader->pst_rangetab[
			varp->opv_gvar]->pst_rgroot);
	    opz_parmmap(subquery, varp->opv_parmmap,
		global->ops_qheader->pst_rangetab[varp->opv_gvar]->pst_rgroot);
	 }
    }

    if (subquery->ops_joinop.opj_virtual)
    {	/* process the correlated virtual tables */
	OPV_IGVARS	    virtual;
	OPV_RT              *vbase;

	vbase = subquery->ops_vars.opv_base;
	for (virtual = 0; virtual < subquery->ops_vars.opv_prv; virtual++)
	{
	    OPV_GRV		*gvarp;
	    gvarp = vbase->opv_rt[virtual]->opv_grv;
	    if( gvarp && gvarp->opv_gsubselect )
		opz_corelated(subquery, virtual); /* if a subselect struct 
					    ** exists then check for 
					    ** correlated variables */
	}
    }
    subquery->ops_vmflag = FALSE;           /* var map is invalid with new
                                            ** joinop variables defined */
}
