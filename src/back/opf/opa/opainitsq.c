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
#include    <di.h>
#include    <sd.h>
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
#define             OPA_INITSQ          TRUE
#include    <me.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPAINITSQ.C - routine to initialize subquery structure
**
**  Description:
**      Contains routine to initialize subquery structure
**
**  History:    
**      26-jun-86 (seputis)    
**          initial creation from init_aggcomp in aggregate.c
**      27-oct-86 (seputis)    
**          init opa_rview to fix count * bug
**      16-dec-88 (seputis)    
**          be more flexible in duplicate semantics for performance
**	    for the min, max, any, aggregates
**	18-dec-90 (kwatts)
**	Smart Disk integration: Force enumeration if Smart Disk Is possible
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	29-Jul-2004 (hanje04)
**	    Remove references to SD (Smart Disk) functions
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
void opa_ifagg(
	OPS_SUBQUERY *new_subquery,
	PST_QNODE *byhead);
OPS_SUBQUERY *opa_initsq(
	OPS_STATE *global,
	PST_QNODE **agg_qnode,
	OPS_SUBQUERY *father);

/*{
** Name: opa_ifagg	- initialize function aggregate structure
**
** Description:
**      This was extracted into a routine since query flattening will 
**      convert simple aggregates to function aggregates in some cases 
**      and it would be desirable to have one location for initialization 
**
** Inputs:
**      new_subquery                    ptr to subquery structure to be
**					initialized
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
**      27-apr-89 (seputis)
**          initial creation
{@history_template@}
[@history_template@]...
*/
VOID
opa_ifagg(
	OPS_SUBQUERY       *new_subquery,
	PST_QNODE	   *byhead)
{
    OPS_STATE	    *global;

    global = new_subquery->ops_global;
    if (!new_subquery->ops_agg.opa_father)
    {   /* no father so this must be the main query, or a projected
	** bylist */
	new_subquery->ops_sqtype = OPS_MAIN;
	new_subquery->ops_width = 0;    /* resdom widths added on recursive
					** pass over query tree */
	new_subquery->ops_agg.opa_graft = NULL; /* no substitutions here */
	new_subquery->ops_agg.opa_byhead = NULL;
	new_subquery->ops_agg.opa_aop = NULL; 
    }
    else
    {
	/* process function aggregate */
	new_subquery->ops_sqtype = OPS_RFAGG; /* assume OPS_RFAGG unless
					** temporary is needed */
	global->ops_qheader->pst_agintree = TRUE; /* parser should set
					** this, but if a simple aggregate
					** is converted to a function aggregate
					** then this needs to be set by OPF */
	global->ops_astate.opa_funcagg = TRUE; /* 
					** function aggregate found in 
					** query */
	new_subquery->ops_agg.opa_byhead = byhead; /* find 
					** BYHEAD node
					** - this node also indicates that
					** a function aggregate is being
					** processed, by being not NULL
					*/
	new_subquery->ops_agg.opa_aop 
	    = new_subquery->ops_agg.opa_byhead->pst_right; /* find the AOP 
					** node */

	/* compute width of count field + aggregate result size
	** - the bylist widths will be added on the recursive pass of the
	** query tree
	*/
	new_subquery->ops_width = OPA_CSIZE + 
	    new_subquery->ops_agg.opa_aop->pst_sym.pst_dataval.db_length; 

    }
    new_subquery->ops_agg.opa_multi_parent = FALSE;
    MEfill(sizeof(OPV_GBMVARS), 0, 
	(char *)&new_subquery->ops_agg.opa_blmap); /* bitmap for bylist */
    MEfill(sizeof(OPV_GBMVARS), 0, 
	(char *)&new_subquery->ops_agg.opa_visible);  /* temp - used later */
}


/*{
** Name: opa_initsq	- initialize subquery structure
**
** Description:
**      This routine will allocate and initialize a subquery structure.
**	The structure will be places on the global->ops_subquery list
**
** Inputs:
**      global                          ptr to global state variable
**      agg_qnode			ptr to ptr to query tree node 
**                                      representing the PST_AGHEAD
**                                      or a PST_ROOT node
**      father                          ptr to subquery representing outer
**                                      aggregate or root node
**                                      - NULL if root node, or the
**                                      projection of a bylist for an
**                                      aggregate
**
** Outputs:
**      global->ops_subquery		subquery will be linked into
**                                      list of subqueries
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jun-86 (seputis)
**          initial creation from init_aggcomp in aggregate.c
**	29-sep-88 (seputis)
**          fix count (*) problem
**	26-oct-88 (seputis)
**          removed to fix count * problem
**      18-aug-89 (seputis)
**          do special duplicate handling for aggregates
**      10-sep-89 (seputis)
**          create bitmap opa_flatten to fix chris date bug
**      12-jun-90 (seputis)
**          fix shape query causing OPF scoping error
**      12-jul-90 (seputis)
**          added OPS_ARESOURCE test for enumeration to correct
**	    resource estimate problems b30809
**	2-jan-91 (seputis)
**	    added ops_tonly for top sort node removal
**	18-dec-90 (kwatts)
**	Smart Disk integration: Force enumeration if Smart Disk Is
**	    possible
**	18-jul-92 (kwatts)
**	    Corrected cast on first param of call to SDinformation for 
**	    lint (and possibly some compilers).
**      15-dec-91 (seputis)
**          fix b40402 - corelated group by aggregate give wrong answer
**      26-dec-91 (seputis)
**          - 41526 - if duplicates to be removed then mark keeptid
**          map as active with no duplicates needed
**      3-may-94 (ed)
**          - remove obsolete variable
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	29-Jul-2004 (hanje04)
**	    Remove references to SD (Smart Disk) functions
**	22-Jul-2005 (toumi01)
**	    Init new ops_mask2, the addition of which was necessitated
**	    by integration of change 472571 from the 2.6 code line,
**	    as Doug and Keith had completed to use up the last flag bit.
**	30-Aug-2006 (kschendel)
**	    Eliminate redundant opo_topsort bool.
**	9-dec-2008 (dougi)
**	    Set OPS_SOMESAGGS flag to speed X100 cross compile.
**	16-april-2009 (dougi)
**	    Remove chunks of #ifdef 0'd code to make it easier to read.
*/
OPS_SUBQUERY *
opa_initsq(
	OPS_STATE          *global,
	PST_QNODE          **agg_qnode,
	OPS_SUBQUERY       *father)
{
    OPS_SUBQUERY          *new_subquery; /* new subquery being analyzed */

    new_subquery = (OPS_SUBQUERY *) 
	opu_memory( global, (i4) sizeof( OPS_SUBQUERY ) );
    MEfill(sizeof(*new_subquery), (u_char)0, (PTR)new_subquery);
    new_subquery->ops_global = global;
    new_subquery->ops_root = *agg_qnode; /* set ptr to the root of the 
                                        ** aggregate query tree */
    {	/* initialize the bit map fields of the root node */
	MEfill(sizeof(PST_J_MASK), 0,
	    (char *)&new_subquery->ops_root->pst_sym.pst_value.
			pst_s_root.pst_lvrm);
	MEfill(sizeof(PST_J_MASK), 0,
	    (char *)&new_subquery->ops_root->pst_sym.pst_value.
			pst_s_root.pst_rvrm);
    }
    new_subquery->ops_gentry = OPV_NOGVAR; /* no result relation yet */
    new_subquery->ops_result = OPV_NOGVAR; /* no result relation yet */
    new_subquery->ops_localres = OPV_NOVAR; /* no result relation yet */
    new_subquery->ops_projection = OPV_NOGVAR; /* no projection yet */
    new_subquery->ops_mode = PSQ_RETINTO; /* query mode for aggregate*/
    new_subquery->ops_cost = 0.0;	/* init cost */
    new_subquery->ops_tsubquery = ++global->ops_astate.opa_subcount;  /* set
				    ** query ID */
    new_subquery->ops_timeout = TRUE;/* TRUE if plan times out, set to FALSE
				     ** if plan does time out */

    /* A bunch of #ifdef 0'd init code was removed from here. */

    new_subquery->ops_enum = global->ops_cb->ops_alter.ops_qep 
	|| 
	(global->ops_cb->ops_smask & OPS_MDISTRIBUTED) /* distributed will always
					** go to enumeration for now */
	||
	(global->ops_cb->ops_check && opt_strace( global->ops_cb, OPT_FCOTREE))
                                        /* if query tree is to be displayed
                                        ** then go through enumeration in order
                                        ** to get accurate cost estimates, even
                                        ** if it is an OVQP
					** - FIXME - also set to TRUE if a 
                                        ** function aggregate
                                        ** statistics would be useful even in
                                        ** the case of a one variable subquery
                                        */
	||
	(global->ops_cb->ops_alter.ops_amask & OPS_ARESOURCE) ;
	/* Removed SDinformation() call */

    /* now initialize the ops_agg component which is visible only
    ** to aggregate processing */

    new_subquery->ops_agg.opa_graft = agg_qnode; /* set ptr to ptr to 
					** query tree node which is the 
                                        ** aggregate or root represented by 
                                        ** this aggregate list element
			                ** - eventually used for a substitution
                                        ** of the aggregate result */
    new_subquery->ops_agg.opa_father = father;  /* set father to outer
			                ** aggregate */
    new_subquery->ops_joinop.opj_select = OPV_NOVAR; /* set if subselect is
                                        ** found in the subquery */
    {   /* outer join initializations */
        new_subquery->ops_oj.opl_aggid = PST_NOJOIN;
        if (global->ops_qheader->pst_numjoins > 0)
        {       /* if outer joins exist then copy the parser range table entries
            ** since these may be modified during flattening */
            new_subquery->ops_oj.opl_pinner = (OPL_PARSER *)opu_memory(global,
                (i4)(2 * sizeof(*new_subquery->ops_oj.opl_pinner)));
            MEfill(2 * sizeof(*new_subquery->ops_oj.opl_pinner),
                (u_char)0, (PTR)new_subquery->ops_oj.opl_pinner);
            new_subquery->ops_oj.opl_pouter = &new_subquery->ops_oj.opl_pinner[1];
        }
    }
    {	/* distributed data base initialization */
	new_subquery->ops_dist.opd_target = OPD_NOSITE; /* the first parent will
					** change this and all subsequent parents
					** will use this site */
	new_subquery->ops_dist.opd_local = OPD_NOSITE;
	new_subquery->ops_dist.opd_updtsj = OPV_NOVAR;
	new_subquery->ops_dist.opd_create = OPD_NOTEMP; /* temporary used by OPC
					** for create table as select handling */
    }
    if (new_subquery->ops_root->pst_left->pst_sym.pst_type == PST_AOP) 
    {   /* a PST_AOP node will be the left node of the AGHEAD iff 
        ** it is a simple aggregate */
        new_subquery->ops_sqtype = OPS_SAGG;
        new_subquery->ops_agg.opa_byhead = NULL; /* set byhead ptr to NULL to 
					** indicate no bylist exists, and also
					** to indicate a simple aggregate is 
                                        ** being processed
					*/
        new_subquery->ops_agg.opa_aop = new_subquery->ops_root->pst_left;
	global->ops_gmask |= OPS_SOMESAGGS; /* show at least one SAGG */
        /* init width to the width of the aggregate */
        new_subquery->ops_width
	    = new_subquery->ops_agg.opa_aop->pst_sym.pst_dataval.db_length;
    }
    else if (new_subquery->ops_root->pst_sym.pst_type == PST_SUBSEL)
    {
	new_subquery->ops_sqtype = OPS_SELECT;
        new_subquery->ops_width = 0;
        new_subquery->ops_agg.opa_byhead = NULL; 
        new_subquery->ops_agg.opa_aop 
	    = new_subquery->ops_root->pst_left->pst_right; /* for subselects
					** define the target expression to be
                                        ** that which is defined on the resdom
                                        ** node */
    }
    else
	opa_ifagg(new_subquery, (*agg_qnode)->pst_left); /* initialize function 
					** aggregate subquery */


    /* the duplicate semantics for an aggregate is only placed into
    ** the AOP node, and the AGHEAD node is always set to ALLDUPS
    ** so transfer the AOP duplicate handling to the AGHEAD node */
    if ((new_subquery->ops_root->pst_sym.pst_type == PST_AGHEAD)
	&&
	new_subquery->ops_agg.opa_aop	/* for a projection subquery, there will not be
					** an AOP, but a PST_AGHEAD node is used */
	)
    {
	switch (new_subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_distinct)
	{
	case PST_DISTINCT:
	    new_subquery->ops_duplicates = OPS_DREMOVE; /* remove duplicates */
            new_subquery->ops_agg.opa_tidactive = TRUE; /* no tids needed */
	    break;	
	case PST_DONTCARE:
	    new_subquery->ops_duplicates = OPS_DUNDEFINED; /* not important for
					    ** this subquery, probably an aggregate
					    ** min, max, any */
	    break;
	case PST_NDISTINCT:
	default:
	    new_subquery->ops_duplicates = OPS_DKEEP; /* keep duplicates
					    ** unless otherwise indicated */
	    break;
	}
    }
    else
    {
	switch ((*agg_qnode)->pst_sym.pst_value.pst_s_root.pst_dups)
	{	/* FIXME - use PST values directly */
	case PST_NODUPS:
	    new_subquery->ops_duplicates = OPS_DREMOVE; /* remove duplicates */
            new_subquery->ops_agg.opa_tidactive = TRUE; /* no tids needed */
	    break;	
	case PST_DNTCAREDUPS:
	    new_subquery->ops_duplicates = OPS_DUNDEFINED; /* not important for
					    ** this subquery, probably an aggregate
					    ** min, max, any */
	    break;
	case PST_ALLDUPS:
	    {
		if (!father			/* if this is the main query */
		    && 
		    (global->ops_cb->ops_smask & OPS_MDISTRIBUTED) /* and this is
					    ** a distributed thread */
		    )
		{
		    i4	qmode;
		    qmode = global->ops_qheader->pst_mode;
		    if ((qmode == PSQ_DELETE)
			||
			(qmode == PSQ_REPLACE)
			)
		    {
			new_subquery->ops_duplicates = OPS_DUNDEFINED; /* for delete
					    ** and replace in SQL an exists will
					    ** be placed around the query so that
					    ** duplicates do not matter */
			break;
		    }
		}
	    }
	default:
	    new_subquery->ops_duplicates = OPS_DKEEP; /* keep duplicates
					    ** unless otherwise indicated */
	    break;
	}
    }
    /* ops_vars, ops_attrs, ops_eclass, ops_bfs, ops_funcs  are not
    ** used by aggregate processing, but initialized by the joinop
    ** phase and the remaining phases of the optimizer
    ** - thus it is not initialized here
    */
    return (new_subquery);
}
