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
#define             OPN_CHECKSORT       TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNCKSRT.C - see if need to put a top sort node on the tree.
**
**  Description:
**      see if a top sort node needs to be placed on the minimum cost tree
**      FIXME- the top sort node should be calculated was part of the minimum
**      cost tree, by using interesting orderings, which may avoid the top
**      sort node altogether.  It would also help if indexes were included
**      for the orderings they provide, as well as if boolean factors exist
**      on the indexed attributes.  Also, need to request histograms for the
**      subquery if a function aggregate is being evaluated, so that these
**      will be available for future joins of temporaries; this can be done
**      by looking at places where rootflg is referenced, and in opn_sm1.
**
**  History:
**      7-jun-86 (seputis)    
**          initial creation from checksort.c
**	19-aug-92 (rickh)
**	    Added some header files.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Mar-2008 (kschendel) b122118
**	    Delete notused OJ-filter stuff.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
void opn_createsort(
	OPS_SUBQUERY *subquery,
	OPO_CO *sortcop,
	OPO_CO *outercop,
	OPN_EQS *eqsp);
static bool opn_rdups(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
bool opn_checksort(
	OPS_SUBQUERY *subquery,
	OPO_CO **oldcopp,
	OPO_COST oldcost,
	OPO_BLOCKS newdio,
	OPO_CPU newcpu,
	OPN_EQS *eqsp,
	OPO_COST *newcostp,
	OPO_ISORT available_ordering,
	bool *sortrequired,
	OPO_CO *newcop);
OPO_CO *opn_oldplan(
	OPS_SUBQUERY *subquery,
	OPO_CO **oldcopp);
void opn_cleanup(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPO_BLOCKS *dio,
	OPO_CPU *cpu);

/*{
** Name: opn_createsort - create a sort node on top of the outer
**
** Description:
**	Sort node is input to this routine due to out-of-memory error
**      handling reasons.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**	sortcop				ptr to sort node to initialize
**	outercop			ptr to node to sort
**	eqsp				ptr to OPN_EQS struct containing sort cost
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    may add node to top of minimum cost ordering tree
**
** History:
**	22-sep-88 (seputis)
**          initial creation, extracted from opn_checksort
**	12-jan-89 (seputis)
**          modify sort cost for top sort node to take into
**	    account possibility of in memory sort
**      8-may-92 (seputis)
**          - b44212, check for overflow in opo_pointercnt
**	1-Apr-2011 (kschendel)
**	    Delete some goofy #if 0 code for readability.
*/
VOID
opn_createsort(
	OPS_SUBQUERY       *subquery,
	OPO_CO		   *sortcop,
	OPO_CO             *outercop,
	OPN_EQS            *eqsp)
{
    OPO_BLOCKS         dio;         /* disk i/o needed for reformat */
    OPO_TUPLES         tuples;      /* estimate of tuples in reformat*/
    OPS_WIDTH	       width;	    /* tuple width */

    sortcop->opo_maps = outercop->opo_maps; /* equivalence classes 
					** required for this node */

    sortcop->opo_union.opo_ojid	= OPL_NOOUTER;
    sortcop->opo_maps = outercop->opo_maps;
					/* ptr to map of equivalence classes 
					** needed by target list to outer node
                                        */
    STRUCT_ASSIGN_MACRO(outercop->opo_cost, sortcop->opo_cost);
    if (eqsp)
    {
	width = eqsp->opn_relwid;
	dio = eqsp->opn_dio;
	tuples = sortcop->opo_cost.opo_tups;
	sortcop->opo_cost.opo_cpu	+= eqsp->opn_cpu;
    }
    else
    {
	OPO_CPU            cpu;         /* cpu needed for reformat */

	width = subquery->ops_width;
	tuples = outercop->opo_cost.opo_reltotb * outercop->opo_cost.opo_tpb;

	opn_reformat(subquery, tuples, subquery->ops_width, &dio, &cpu);

	sortcop->opo_cost.opo_cpu	+= cpu;
    }
    if (((width + DB_SROVERHEAD) * sortcop->opo_cost.opo_tups)
	>
	subquery->ops_global->ops_cb->ops_alter.ops_qsmemsize
	)				    /* check for in memory sort */
	sortcop->opo_cost.opo_dio	+= dio;
    sortcop->opo_outer		= outercop;
    sortcop->opo_sjpr		= DB_RSORT;
    sortcop->opo_storage	= DB_SORT_STORE;
    outercop->opo_pointercnt++;
    if (!(outercop->opo_pointercnt))
	opx_error(E_OP04A8_USAGEOVFL); /* report error if usage count overflows */
}

/*{
** Name: opn_rdups	- check if duplicates are possible
**
** Description:
**	This routine starts with the given CO node and checks to see whether
**	duplicates on the eqc's given by ops-tonly map are possible.
**
**	Originally, the idea was to see if FSM joins were such that the
**	dups removing topsort could be eliminated.  However in the modern
**	world of hash joins, we often prefer to leave the topsort so that
**	merge joins can be transformed to hash joins.  Also, there are
**	many situations where dups might be introduced in a non-obvious
**	way:  outer joins (nulls on the inner), joins or sorts on more
**	atts than are in the distinct list, partitioned tables, etc.
**
**	So, all that we do now is check for an ORIG (or PR/ORIG) against
**	a table that is unique keyed on the distinct columns.
**
**	NOTE:  the return value has been reversed (4-may-2011) from the
**	original sense!  rdups used to return "dups are impossible".
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	cop				ptr to cost ordering node being analyzed
** Outputs:
**
**	Returns:
**	    TRUE - duplicates are possible
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jul-90 (seputis)
**          initial creation
**	20-dec-90 (seputis)
**	    - added support to remove top sort nodes where possible
**	26-dec-91 (seputis)
**	    - bug 41526 - eliminated redundant code, and redundant 
**	    parameters to simplify routine.
**	2-feb-93 (ed)
**	    -bug 47377 - top sort node elimination enhancement using
**	    DMF uniqueness of indices
**      28-may-93 (ed)
**          - bug 52281 - look at tidmap only if tids are active
**	30-jun-97 (inkdo01)
**	    SortU's under joins aren't enough to guarantee distinct results
**	    from an outer join (because of the possibility of null inners).
**	 9-oct-01 (hayke02)
**	    Do a top sort if we have a 'not exists'/'not in' to outer join
**	    transformation (ops_smask & OPS_SUBSELOJ). This change fixes
**	    bug 105300.
**	4-May-2011 (kschendel)
**	    Recent subsel transform changes are causing the subsel-oj flag to
**	    not always be set.  Upon consideration, we'd usually rather keep
**	    the top-sort anyway;  dropping the top-sort can interfere with
**	    opj's attempts to change merge joins into hash joins, and one
**	    final sort is usually better than pre-join sorts and merge joins.
**	    So, stub most of this out, keeping only the unique-keyed check.
**	    Also, reverse the return sense, for caller's convenience.
*/
static bool
opn_rdups(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    bool	ret_value;
    OPV_VARS	*varp;		    /* ptr to variable descriptor of base
				    ** relation */

    /* If there's an inner, can't be an ORIG or PROJ-REST, say dups */
    if (cop->opo_inner != NULL)
	return (TRUE);

    /* If PR, move to ORIG */
    if (cop->opo_outer != NULL && cop->opo_sjpr == DB_PR)
	cop = cop->opo_outer;
    else
	return (TRUE);

    /* Now pointing at an orig */
    ret_value = TRUE;
    varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];
    if (varp->opv_index.opv_poffset >= 0)
	varp = subquery->ops_vars.opv_base->opv_rt[varp->opv_index.opv_poffset];
    /* Note that this UNIQUE flag excludes subselects, etc */
    if (varp->opv_mask & OPV_UNIQUE)
    {   /* check for DMF multi-attribute unique key */
	OPV_BMVARS	    vmap;	    /* create map of single variable
					** to determine if DMF uniqueness will
					** prevent duplicate introduction by
					** this variable */
	MEfill(sizeof(vmap), (u_char)0, (PTR)&vmap);
	BTset((i4)varp->opv_index.opv_poffset, (char *)&vmap);
	ret_value = ! opn_uniquecheck(subquery, &vmap, subquery->ops_tonly,
	    OPE_NOEQCLS);
    }
    return(ret_value);
}

/*{
** Name: opn_checksort	- see if need to put a top sort node on the tree.
**
** Description:
**      The following conditions for putting a top sort node on the CO tree
**      are check for and initialized in subquery->ops_msort.opo_topsort
**	put top reformat node on tree if:
**		a) user level query		AND
**		b) sortby clause or (ret into and default
**			rel struct of heapsort or cheapsort).
**
**		OR
**
**		ops_duplicates == OPS_DREMOVE(in which case aggregate processing
**			wishes us to remove dups).
**
**	eventually we can handle the removedups case that the
**	aggregate processing needs (just add appropriate args here and there
**	and check them here)
**
**	we handle default rel structs of heapsort, isam and hash.
**
**	where mode is append, replace or delete, we could sort the tuples
**	on tids. this would make the updates go faster since we would not
**	be randomly hitting pages.  however this would change the semantics
**	of updates with batch processing turned off.
**	
**		Also, cannot add reformat to hash or isam as a
**		top node cause it will mess up multi-statement
**		transactions FIXME
**
**      Place a SORT node on top of the query if it has been determined that
**      the halloween problem could occur for a replace or delete.
**
**      FIXME - cannot have a default reformat to cheapsort since 
**      DB_SORT_STORE is not recognized.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**	oldcopp				old best plan to be deallocated if it
**                                      is another top sort node
**      oldcost                         old best cost used to determine if this
**                                      new plan with the sort node is better
**      newdio                          dio of new plan without sort cost
**	newcpu				cpu of new plan without sort cost
**	eqsp                            sort cost info for new plan
**      newcostp                        ptr to return new best cost
**      available_ordering		sortedness available to avoid top sort node
**	sortrequired			if TRUE then do not remove top sort node
**					since a sort node is required for correct
**					deferred update semantics
**
** Outputs:
**	Returns:
**	    TRUE if old plan is still better than new plan
**	Exceptions:
**	    none
**
** Side Effects:
**	    may deallocate old sort node
**
** History:
**	7-jun-86 (seputis)
**          initial creation from checksort
**      22-sep-88 (seputis)
**          split into opn_checksort, opn_createsort, opn_cleanup
**	    since top sort node cost was not considered in the enumeration
**          module, and it was difficult to proceed with the distributed
**	    optimization model unless this was done... FIXME, this is the
**          first step to improving this routine so if a user specified ordering
**	    is required, then do not add a sort node unless necessary
**	20-dec-90 (seputis)
**	    - added support to remove top sort nodes where possible
**	20-feb-91 (seputis)
**	    - fixed cost estimate problem with top sort node removal
**	18-apr-91 (seputis)
**	    - b36920 - floating point handling problem
**	12-jun-91 (seputis)
**	    - top sort node removal should be ignored for distributed
**	    at this point, since control of access path is not possible.
**	12-aug-91 (seputis)
**	    - fix b38691 - print error if aggregate temp is too large
**	27-dec-91 (seputis)
**	    - fixed several top sort node removal problems including
**	    41526
**      5-jan-93 (ed)
**          - b48049 - remove parameter to opo_combine as part of simplification
**          of routine
**	17-jul-98 (inkdo01)
**	    Add rules to qualify top sort removal for descending sorts.
**	12-dec-98 (inkdo01)
**	    Add gateway check to descending sort, since they look like (but
**	    aren't) Btrees.
**	17-aug-99 (inkdo01)
**	    Added hack test to prevent read backwards through multi-attr index
**	    when first col is in constant BF and order by is on 2nd col.
**	13-nov-00 (inkdo01)
**	    Another hack test to see if avail. ordering is unqiue and left
**	    subset of requested ordering and, therefore, good enough.
**	 7-mar-01 (hayke02)
**	    The above fix for bug 100482 has been modified so that we break
**	    if the DB_SJ CO nodes have had their outers sorted (indicating
**	    an FSM or PSM join). This prevents either E_OP0497 or incorrectly
**	    ordered data when the top sort is removed by the fix for 100482.
**	    This change fixes bug 104151.
**	21-mar-01 (hayke02)
**	    Further modification of above fix for bug 100482 - we now do not
**	    set sortrequired to FALSE if all the ordering eqclasses are not
**	    found in the btree unique var being examined. This change fixes
**	    bug 104288.
**	 2-jul-01 (hayke02 for inkdo01)
**	    Modification to descending top sort removal code to prevent
**	    incorrect result set from FSM joins. This change fixes bug
**	    105172.
**	13-jul-01 (hayke02)
**	    Modification to descending top sort removal code to prevent
**	    incorrect result set from index/TID joins. This change fixes
**	    bug 105222.
**	24-jan-01 (hayke02)
**	    Further modification to descending top sort removal code -
**	    we now do not remove the top sort if the outer is not a btree
**	    nor sorted on the join attribute. This change fixes bug 105198.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	25-feb-04 (hayke02)
**	    Further modification to descending top sort removal code -
**	    we now do not remove the top sort if the orig_sjpr is DB_SJ
**	    (simple join) and the orig outer is sorted. This change fixes
**	    problem INGSRV 2595, bug 111281.
**	12-Aug-2008 (kibro01) b120763
**	    Keep a top sort even if this is a BTREE table if it is also
**	    partitioned and the partitioning rule is DBDS_RULE_HASH, because
**	    then we're not actually going to get the results in order.
[@history_line@]...
*/
bool
opn_checksort(
	OPS_SUBQUERY       *subquery,
	OPO_CO		   **oldcopp,
	OPO_COST	   oldcost,
	OPO_BLOCKS	   newdio,
	OPO_CPU            newcpu,
	OPN_EQS            *eqsp,
	OPO_COST           *newcostp,
	OPO_ISORT	   available_ordering,
	bool		   *sortrequired,
	OPO_CO		   *newcop)
{
    /* check if top sort node is required, and add any costs associated
    ** with creating a relation */

    OPZ_AT	*abase = subquery->ops_attrs.opz_base;
    OPO_COST	sortjcost;
    OPO_ISORT	result;	    /* ordering defined if needed by an order by list */
    OPO_ISORT	jordering;

    result = OPE_NOEQCLS;
    if (!(*sortrequired))
    {
	/* if sortrequired is TRUE then sort node cannot be removed due
	** to deferred update semantics */
	if ((subquery->ops_msort.opo_mask & OPO_MRDUPS)
	    &&
	    !(subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	    )
	{
	    /* Keep topsort unless rdups says no dups are possible */
	    *sortrequired = (newcop == NULL) || opn_rdups(subquery, newcop);
	}
	else
	    *sortrequired = TRUE;

	if ((   !(subquery->ops_msort.opo_mask & OPO_MRDUPS)
		||
		!(*sortrequired)
	    )
	    && 
	    (subquery->ops_msort.opo_mask & OPO_MREMOVE)
	    &&
	    !(subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED))
	{
	    /* check that current ordering "available_ordering" can provide necessary sortedness
	    ** in which case the sort node can be avoided */
	    OPO_EQLIST	*tsortp;
	    OPO_ISORT	oj_ordering;
	    OPO_CO	*orig;
	    OPV_VARS	*varp;
	    OPL_OJTYPE	ojt;
	    bool	single;

	    *sortrequired = TRUE;
	    sortjcost = opn_cost(subquery, newdio, newcpu);
	    if (sortjcost < oldcost)    /* check if more expensive even is sort is avoided */
	    {
		tsortp = subquery->ops_msort.opo_tsortp;
		if (tsortp->opo_eqorder[1] == OPE_NOEQCLS)
		{
		    jordering = subquery->ops_msort.opo_tsortp->opo_eqorder[0]; /* single
					    ** attribute ordering for sortedness */
		    single = TRUE;
		}
		else
		{
		    jordering = opo_cordering(subquery, subquery->ops_msort.opo_tsortp, 
			FALSE);		    /* find the exact ordering associated 
					    ** with this list */
		    single = FALSE;
		}
		oj_ordering = opo_combine(subquery, jordering, available_ordering, jordering, 
		    &subquery->ops_eclass.ope_maps.opo_eqcmap, &result);
		if (result == jordering) 
		{
		    *sortrequired = FALSE;	    /* sort node can be avoided */
		}
		else if (!single)
		/* If multi-attribute sort, see if order is driven by Btree
		** orig node in which index is unique and left subset of
		** desired ordering. If so, no top sort is required. */

		 for (orig = newcop; orig; orig = orig->opo_outer)
		  if (orig->opo_sjpr == DB_ORIG &&
			orig->opo_storage == DB_BTRE_STORE &&
			(varp = subquery->ops_vars.opv_base->opv_rt
			    [orig->opo_union.opo_orig])->opv_uniquemap)
		  {
		    i4	i, ucount;
		    OPV_VARS *chk_var;
		    DB_PART_DEF *ppart;

		    /* We found a unique Btree orig node without any intervening
		    ** nodes (e.g. sort node) which might reorder the data. Now
		    ** check if key structure matches high order eqc's of
		    ** ordering. */
		    ucount = BTcount((char *)varp->opv_uniquemap, 
					(i4)subquery->ops_eclass.ope_ev);
		    for (i = 0; i < ucount; i++)
		     if (!BTtest((u_i4)tsortp->opo_eqorder[i], 
				(char *)varp->opv_uniquemap) ||
			abase->opz_attnums[varp->opv_mbf.opb_kbase->
			 opb_keyorder[i].opb_attno]->opz_equcls != 
			 tsortp->opo_eqorder[i]) break;
						/* check key structure matches
						** order */
		    if (i < ucount) break;

		    /* check that all of the ordering eqclasses are contained
		    ** in this btree unique var - bug 104288 */
		    for (i = 1; tsortp->opo_eqorder[i] != OPE_NOEQCLS; i++)
			if (BTtest((u_i4)tsortp->opo_eqorder[i],
			(char *)&varp->opv_maps.opo_eqcmap) == FALSE) break;

		    if (tsortp->opo_eqorder[i] != OPE_NOEQCLS) break;

		    /* If this is a partitioned table, and the partitioning
		    ** rule is HASH, then don't remove the sort, since we
		    ** won't get the elements in the order we're expecting.
		    ** (kibro01) b120763
		    */
		    chk_var = subquery->ops_vars.opv_base->opv_rt
				[orig->opo_union.opo_orig];
		    if (chk_var &&
			chk_var->opv_grv && 
			chk_var->opv_grv->opv_relation &&
			(ppart = chk_var->opv_grv->opv_relation->rdr_parts))
		    {
			for (i = 0; i < ppart->ndims; i++)
			{
			    if (ppart->dimension[i].distrule == 
					DBDS_RULE_HASH)
				break;
			}
			if (i < ppart->ndims) break;
		    }
	
		    *sortrequired = FALSE;
		  }
		  /* Only allowable intervening QEP nodes are PRs and joins 
		  ** which aren't full/left outer joins (whose algorithms
		  ** can reorder rows). */
		  else if (orig->opo_sjpr == DB_PR ||
			orig->opo_sjpr == DB_SJ && !(orig->opo_osort) &&
			(!subquery->ops_oj.opl_base ||
			orig->opo_union.opo_ojid == OPL_NOOUTER ||
			(ojt = subquery->ops_oj.opl_base->opl_ojt[orig->
			 opo_union.opo_ojid]->opl_type) != OPL_FULLJOIN &&
			 ojt != OPL_RIGHTJOIN)) continue;
		  else break;		/* order lost in QEP shuffle */
	    }
	}
    }
    /* Top sort removal for descending sorts of joins can't happen for
    ** merge joins. Make sure it's NOT a merge. */

    if (!(*sortrequired) && subquery->ops_msort.opo_mask & OPO_MDESC)
    {
	OPO_CO	*orig;
	OPV_GRV	*varp;		    /* ptr to variable descriptor of 
				    ** base relation */

	/* First assure there's no merge join to foul the descending order. */
	for (orig = newcop; orig && !(*sortrequired); orig = orig->opo_outer)
	{
	    if (orig->opo_sjpr == DB_ORIG)
	     varp = subquery->ops_vars.opv_base->opv_rt[orig->opo_union.opo_orig]
		->opv_grv;
	    else varp = NULL;
	    if ((orig->opo_inner && orig->opo_inner->opo_sjpr != DB_ORIG &&
		orig->opo_sjpr != DB_ORIG) ||
		(orig->opo_outer && (!((orig->opo_outer->opo_storage ==
		DB_SORT_STORE) && (orig->opo_outer->opo_ordeqc ==
		orig->opo_sjeqc)) && (orig->opo_outer->opo_storage !=
		DB_BTRE_STORE))) ||
	    (orig->opo_sjpr == DB_ORIG && (orig->opo_storage != DB_BTRE_STORE ||
		varp == NULL ||
		varp->opv_relation->rdr_rel->tbl_status_mask & DMT_GATEWAY ||
		varp->opv_relation->rdr_rel->tbl_id.db_tab_index != 0)) ||
		((orig->opo_sjpr == DB_SJ) && orig->opo_osort))
	     *sortrequired = TRUE;	     /* MUST be a merge join or non-
					     ** BTREE */
	    else if (orig->opo_sjpr == DB_ORIG) 
	    {
		OPO_EQLIST	*orig_list;
		/* Assure 1st column of index isn't in a constant BF. */
		if (orig->opo_ordeqc < subquery->ops_eclass.ope_ev ||
			subquery->ops_bfs.opb_bfeqc == NULL) break;
		if ((orig_list = subquery->ops_msort.opo_base->opo_stable
			[orig->opo_ordeqc-subquery->ops_eclass.ope_ev]->opo_eqlist) 
			== NULL || BTtest((i4)orig_list->opo_eqorder[0],
			(char *)subquery->ops_bfs.opb_bfeqc)) *sortrequired = TRUE;
		break;
	    }
	}

    }
	

    if (*sortrequired)
    {
	sortjcost = opn_cost(subquery, newdio + eqsp->opn_dio, newcpu + eqsp->opn_cpu);
	if ((	(sortjcost >= oldcost)
		&&
		(	!(subquery->ops_global->ops_gmask & OPS_BFPEXCEPTION)
		    ||
		    (subquery->ops_global->ops_gmask & OPS_FPEXCEPTION)
		)
	    )
	    ||
	    (subquery->ops_mask & OPS_CAGGTUP)
	    )
	    return(TRUE);		    /* return if too expensive after adding costs
					    ** unless previous best plan had exceptions, and
					    ** current best plan was produced without
					    ** exceptions, or if this is an aggregate in
					    ** which a temp file is needed for the top
					    ** sort node, but this would cause an error
					    ** since the tuple size is too large */
	*newcostp = sortjcost;
    }
    if ((*oldcopp) && ((*oldcopp)->opo_sjpr == DB_RSORT) )
    {   /* deallocate the previous sort node if it existed */
	OPO_CO	*tempcop;
	tempcop = (*oldcopp)->opo_outer;
	opn_dcmemory(subquery, *oldcopp);
	*oldcopp = tempcop;
    }
    return(FALSE);			    /* plan is still better after adding sort cost
					    ** or avoiding sort cost */
}

/*{
** Name: opn_oldplan	- delete old best plan
**
** Description:
**      This routine will delete a CO node if it is a top sort node, otherwise leave it alone. 
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      copp                            ptr to ptr of CO node to delete
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
**      22-sep-88 (seputis)
**          initial creation
**      20-dec-90 (seputis)
**          fix distributed access violation
[@history_line@]...
[@history_template@]...
*/
OPO_CO *
opn_oldplan(
	OPS_SUBQUERY       *subquery,
	OPO_CO             **oldcopp)
{
    if ((*oldcopp) && ((*oldcopp)->opo_sjpr == DB_RSORT) )
    {   /* deallocate the previous sort node if it existed and replace it
	** with the child */
	OPO_CO	*tempcop;

        (*oldcopp)->opo_variant.opo_scost = NULL; /* this should point
			    ** to a cost array or be NULL, FIXME when
			    ** sort nodes are explicit interior nodes
			    ** then this needs to be smarter */
	tempcop = (*oldcopp)->opo_outer;
	opn_dcmemory(subquery, *oldcopp);
	*oldcopp = tempcop;
    }
    return (opn_cmemory(subquery));
}

/*{
** Name: opn_cleanup	- routine called after CO tree has been selected
**
** Description:
**      This routine will add the cost of creating a new relation to the CO tree.
**      Since any plan will have this fixed cost, it can be added after all plans
**      have been evaluated. 
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      cop                             ptr to root cop for subquery
**
** Outputs:
**      cop->opo_cost.opo_dio           updated with create cost
**      cop->opo_cost.opo_cpu           updated with create cost
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-sep-88 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opn_cleanup(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPO_BLOCKS	   *dio,
	OPO_CPU		   *cpu)
{
    if  (subquery->ops_mode == PSQ_RETINTO)
    {
	/* if a retrieve into a rel then add the cost of writing the relation
	** which is two direct disk io's per page. Add in 10 for creating,
	** opening and closing the result relation 
	*/
	(*dio) += 2.0 * cop->opo_cost.opo_reltotb + DB_COCRELATION;
	(*cpu) += cop->opo_cost.opo_tups;
    }

}
