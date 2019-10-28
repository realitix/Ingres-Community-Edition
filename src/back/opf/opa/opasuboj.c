/*
**Copyright (c) 2004, 2010, 2011 Actian Corporation
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
#include    <x100.h>
#include    <adfops.h>
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
**  Name: OPASUBOJ.C - routines to detect qualifying "not exists",
**	"not in", "not = any" and "<> all" subselects and transform 
**	them to outer joins.
**
**  Description:
**      perform "not exists" subselect to outer join transform.
**
**  History:    
**      9-aug-99 (inkdo01)
**	    Written.
**	20-mar-02 (hayke02)
**	    Added new function opa_subsel_byheadchk() to check for byhead
**	    nodes - if we find one, we don't do the OJ transform.
**	1-apr-04 (inkdo01)
**	    Change TID to 8 bytes for partitioning support.
**	18-Aug-2009 (kiria01)
**	    Added allow_antijoin flag to a number of routines to allow for
**	    dropping back to non-antijoin in tha case of a containing OR that
**	    will affect processing.
**	10-Mar-2011 (kiria01) m1506
**	    Rework joinid allocation to be driven from the subselects own
**	    allotted .pst_ss_joind. This will mean that all subtrees that
**	    are derived from the same source subselect will inherit the same
**	    joinid and hence will not end up duplicating and confusing opa
**	    later where the same factors may then appear to conflict.
**	12-apr-2011 (dougi) m1629
**	    Flatten EXISTS and IN subqueries if nested in NOT EXISTS/IN/...
**	14-Apr-2011 (kiria01) m1538
**	    Flatten EXISTS and IN subqueries in other contexts.
**	    Split out opa_subsel_underor into opa_subsel_exists and
**	    opa_subsel_eq_any. Removed the need for opa_subsel_search_qtree
**	    adding flattening to target list.
**	    Added opa_cosubquery_full to provide finer resolution of link
**	    & correlation structure. Handle AGHEAD common WHERE clause as a
**	    unit - see opa_subsel_agh_*()
**	19-Apr-2011 (kiria01)
**	    Prior fixes in main opasuboj.c not needed as follows:
**		b121740 (kibro01) Aggregate WHERE clause copies get handled
**			once through the AGHEAD linkage in m1538,
**		s121883 (kiria01) Target list is descended.
**		b122066,b123408 (smeke01) Range table already traversed
**			and included RTREE, DRTREE & WETREE.
**		b123752 (smeke01) Join-id for SS flattening is now assigned
**			in PST_SUBSEL .pst_ss_joinid by parser. Also, the
**			handling of multiple aggregate WHERE clause means
**			that there is no change of the joinid getting out of
**			step.
**		b124009 (smeke01) Also addressed by m1538.
**	17-Jun-2011 (kiria01) m1943
**	    Ifdef'd out the USE_SEMIJOIN sections. Presently this transformation
**	    degrades TPC-H q20. This may be due to a number of factors:
**	     1) Representing semijoins in the parse tree using outer join joinids
**		falls foul of any code that sifts out these operators either to
**		process of exclude them. In general they should be treatable as
**		special inner joins.
**	     2) The cost analyser needs teaching about semijoin characteristics.
**	     3) It may also be that the semijoin is simply not as good as the Quel
**		use of duplicate emlimination.
**	    Further research will need to be done to realise the potential. Other
**	    semijoins represented as OJ will break the two following assumptions seen
**	    in the code:
**	     1) Tree copies of fragments tend not to have joinids due to such being
**		left intact - not true with semi-joins and in suck of restrictions.
**	     2) View processing assumes uniqueness of OJ id for identification - also
**		not true with semi-joins.
**	4-july-2011 (dougi) m2196
**	    Replace all pst_resolve()s by opa_gresolve()s.
**	8-july-2011 (dougi) m2229
**	    Removed USE_SEMIJOIN limitations, but only for update/delete.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of opa_gresolve reflecting
**	    pst_resolve change.
**/


/* Static variables and other such stuff. */
#ifndef MAX_NESTING
#define MAX_NESTING 100
#endif

static DB_DATA_VALUE	tid_dv = {NULL, DB_TID8_LENGTH, DB_INT_TYPE, 0, -1};
static DB_ATT_ID	tid_att = { 0 };
static char		tid_name[] = {'t', 'i', 'd'};

/* TABLE OF CONTENTS */
static void opa_subsel_joinid(
	OPS_STATE *global,
	PST_QNODE *node,
	PST_J_ID joinid);
static void opa_subsel_makeoj(
	OPS_STATE *global,
	PST_QTREE *header,
	PST_QNODE *node,
	PST_QNODE *root,
	PST_QNODE *outer_root,
	PST_QNODE *inner_root,
	OPL_BMOJ **ojmapp);
static bool opa_subsel_viewchk(
	OPS_STATE *global,
	PST_QNODE *subselp);
static bool opa_subsel_byheadchk(
	PST_QNODE *subselp);
static void opa_notex_transform(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	PST_QNODE *inner_root,
	bool *gotone,
	bool allow_antijoin,
	bool underor);
static bool opa_notex_analyze(
	OPS_STATE *global,
	PST_QNODE *outer_root,
	PST_QNODE *inner_root,
	PST_QNODE *whnode,
	bool *outer,
	bool *inner,
	bool *equcorr,
	bool *nested_subsel);
static void opa_neany_transform(
	OPS_STATE *global,
	PST_QNODE *bopp);
static void opa_noncorrall_transform(
	OPS_STATE *global,
	PST_QNODE *outer_root,
	PST_QNODE *inner_root,
	PST_QNODE **nodepp);
static void opa_corrall_transform(
	OPS_STATE *global,
	PST_QNODE **nodepp);
static void opa_notin_transform(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	PST_QNODE *bopp,
	bool *gotone,
	bool allow_antijoin,
	bool underor);
static bool opa_notin_analyze(
	OPS_STATE *global,
	PST_QNODE *root,
	PST_QNODE *opp);
static void opa_demorgan(
	OPS_STATE *global,
	PST_QNODE **notpp,
	PST_QNODE **nodepp);
static void opa_subsel_exists(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	bool *gotone,
	bool allow_antijoin,
	bool underor);
static void opa_subsel_count_movepreds(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QNODE *root,
	OPV_GBMVARS *rootantimap,
	OPV_GBMVARS *varmap);
static void opa_subsel_countpreds(
	OPS_STATE *global,
	PST_QNODE *root, 
	PST_QNODE **rsdmp, 
	PST_QNODE *aghp, 
	PST_QNODE **sqrhs, 
	PST_J_ID joinid,
	i4 rngno);
static void opa_subsel_countsq(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	bool *gotone);
static void opa_subsel_eq_any(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	bool *gotone,
	bool allow_antijoin,
	bool underor);
static bool opa_subsel_search(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	bool *gotone,
	bool allow_antijoin,
	bool underor,
	bool undernotex_in);
void opa_subsel_to_oj(
	OPS_STATE *global,
	bool *gotone);
static u_i4 opa_cosubquery_full(
	OPS_STATE *global,
	PST_QNODE *outer,
	PST_QNODE *inner,
	u_i4 break_state,
	bool blocking_or,
	OPV_GBMVARS *varmap);
static void opa_subsel_agh_lace(
	OPS_STATE *global,
	PST_QNODE *root,
	bool enable);
static void opa_subsel_agh_common(
	PST_QNODE *agh);
static bool opa_subsel_agh_count_present(
	OPS_STATE *global,
	PST_QNODE *agh);

OPV_DEFINE_STK_FNS(nodep, PST_QNODE**);
OPV_DEFINE_STK_FNS(node, PST_QNODE*);

#define COVAR_I_SEEN		0x00000001	/* An Inner VAR referenced */
#define COVAR_O_SEEN		0x00000002	/* An Outer VAR referenced */
#define COVAR_X_SEEN		0x00000004	/* An eXternal VAR referenced */
#define COVAR_SS_SEEN		0x00000008	/* An unflattened SS found */
#define COVAR_RESTRICT		0x00000010	/* Restriction on Inner found */
#define COVAR_COREL		0x00000020	/* f(I) OP g(O) or f(O) OP g(I) */
#define COVAR_EQUIJOIN		0x00000040	/* COVAR_COREL & has EQUI */
#define COVAR_NONEQUIJOIN	0x00000080	/* COVAR_COREL & has NONEQUI */
#define COVAR_RESTRICTO		0x00000100	/* Restriction on Outer found */
#define COVAR_CORELO		0x00000200	/* f(O) OP g(X) or f(X) OP g(O) */
#define COVAR_EQUIJOINO		0x00000400	/* COVAR_CORELO & has EQUI */
#define COVAR_NONEQUIJOINO	0x00000800	/* COVAR_CORELO & has NONEQUI */
#define COVAR_RESTRICTX		0x00001000	/* Restriction on eXternal found */
#define COVAR_CORELX		0x00002000	/* f(I) OP g(X) or f(X) OP g(I) */
#define COVAR_EQUIJOINX		0x00004000	/* COVAR_CORELX & has EQUI */
#define COVAR_NONEQUIJOINX	0x00008000	/* COVAR_CORELX & has NONEQUI */
#define COVAR_COREL_MIX		0x00010000	/* f(I,O) OP g(O,I) */
#define COVAR_HAS_COUNT		0x00020000	/* COUNT seen */
#define COVAR_HAS_UNION		0x00040000	/* SS has a union */
#define COVAR_MESS		0x00080000	/* Correlation is complex */
#define	COVAR_SIMPAGG_IN_TL	0x00100000	/* Agg in target list (ungrouped) */
#define COVAR_CONST_TL		0x00200000	/* Target list has no VAR nodes */
#define COVAR_UNDEROR		0x00400000	/* Working state - is in OR scope */
#define COVAR_IN_WHERE		0x00800000	/* Working state - 0=Tgt list, 1=WHERE  */
#define COVAR_GROUPED		0x01000000	/* Working state - BYHEAD in scope */
/* state[0] values may overlay Working state */
#define COVAR_OR_ACTIVE		COVAR_UNDEROR	/* A correlation was masked by OR
						** and blocking_or requested */


/*{
** Name: opa_subsel_joinid	- assigns new OJ joinid to ON clause
**
** Description:
**      This function descends the subselect where clause, assigning 
**	the joinid of the new outer join to all its connective and 
**	relop nodes (thus making into the ON clause).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	node		ptr to subselect where clause parse tree fragment
**	joinid		joinid of new outer join
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
**	    none
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**      16-aug-2004 (huazh01)
**          Add support for PST_COP, PST_CASEOP, PST_WHOP, 
**          PST_WHLIST, PST_MOP and PST_SEQOP. 
**          b112821, INGSRV2934. 
**	16-aug-04 (inkdo01)
**	    Add PST_COP to list of transformees.
**	19-feb-07 (kibro01) b117485
**	    Only change the joinid if the symbol is a structure which
**	    contains a JOINID (i.e. PST_OP_NODEs, but not things like NOT)
**
[@history_template@]...
*/
static void
opa_subsel_joinid(
	OPS_STATE	   *global,
	PST_QNODE	   *node,
	PST_J_ID	   joinid)

{

    /* Simply descend the fragment looking for operator nodes whose
    ** pst_joinid's must be reassigned. */

    while (node)	/* loop is returned from */
     switch (node->pst_sym.pst_type) {

      case PST_AND:
      case PST_OR:
      case PST_BOP:
      case PST_CASEOP:
      case PST_WHOP:
      case PST_WHLIST:
      case PST_MOP:
      case PST_SEQOP: 
	opa_subsel_joinid(global, node->pst_right, joinid);
				/* recurse on right side */

	/* Fall through to iterate on left with the UOPs. */
      case PST_UOP:
      case PST_COP:
      case PST_NOT:
	/* This is where pst_joinid is reassigned. */
	if (node->pst_sym.pst_type == PST_AND ||
	    node->pst_sym.pst_type == PST_OR ||
	    node->pst_sym.pst_type == PST_UOP ||
	    node->pst_sym.pst_type == PST_BOP ||
	    node->pst_sym.pst_type == PST_AOP ||
	    node->pst_sym.pst_type == PST_COP ||
	    node->pst_sym.pst_type == PST_MOP)
	{
	    if (node->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN)
		node->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
	}
	node = node->pst_left;
	break;

      default:
	return;			/* return from anything else */
    }	/* end of switch and while loop */

}


/*{
** Name: opa_subsel_makeoj	- performs activities to introduce OJ
**
** Description:
**      This function does all the stuff needed to add an "oj" (semi/
**	anti/subq join) to process flattened subquery. 
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	header		ptr to parse tree header
**	node		ptr to subselect where clause parse tree fragment
**			about to become ON clause
**	root		ptr to 
**	outer_root	ptr to root of main parse tree
**	inner_root	ptr to root node of `
**	ojmapp		ptr to ptr to semi/anti/subqjmap to set
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
**	    none
**
** History:
**      20-nov-2010 (dougi)
**	    Written for common support of flattened NOT IN/EXISTS and ORed
**	    IN/EXISTS subqueries.
*/
static void
opa_subsel_makeoj(
	OPS_STATE	*global,
	PST_QTREE	*header,
	PST_QNODE	*node,
	PST_QNODE	*root,
	PST_QNODE	*outer_root,
	PST_QNODE	*inner_root,
	OPL_BMOJ	**ojmapp)

{
    OPV_IGVARS	varno;
    PST_J_ID	joinid = inner_root->pst_sym.pst_value.pst_s_root.pst_ss_joinid;


    /* node is the new ON clause - flag as such. */
    opa_subsel_joinid(global, node, joinid);
    global->ops_goj.opl_mask = OPL_OJFOUND;
    if (global->ops_goj.opl_glv < joinid)	/* check some global OJ stuff */
	opx_error(E_OP038E_MAXOUTERJOIN);
    /* Make sure opl_semi/anti/subqjmap is allocated and set joinid in it. */
    if (ojmapp)
    {
	if ((*ojmapp) == (OPL_BMOJ *) NULL)
	{
	    (*ojmapp) = (OPL_BMOJ *) opu_memory(global, sizeof(OPL_BMOJ));
	    MEfill(sizeof(OPL_BMOJ), 0, (char *) (*ojmapp));
	}
	BTset((i4)joinid, (char *)(*ojmapp));
    }

    /* Loop over outer_root pst_tvrm, ORing joinid into their range table 
    ** pst_outer_rel's. Then loop over inner_root pst_tvrm, ORing joinid
    ** into their range table pst_inner_rel's. */

    for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
    {
	PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	BTset((i4)joinid, (PTR)&rngp->pst_outer_rel.pst_j_mask);
    }

    for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
    {
	PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	BTset((i4)joinid, (PTR)&rngp->pst_inner_rel.pst_j_mask);
    }

    /* Finally, OR the inner_root pst_tvrm into the outer_root pst_tvrm. */

    BTor((i4)BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	(PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	(PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
    outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	BTcount((PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* update from clause cardinality */

    /* And really finally, if we're under an AGHEAD, OR the inner_root 
    ** pst_tvrm into the AGHEAD (root) pst_tvrm. */

    if (root->pst_sym.pst_type == PST_AGHEAD)
    {
	BTor((i4)BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	    BTcount((PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm,
		BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm));
    }

}


/*{
** Name: opa_subsel_viewchk	- check for unflattened view in subsel
**
** Description:
**      This function checks the subselect from clause for an unflattened
**	view. This is indication that the view failed PSF's flattening 
**	rules (union, agg., etc.) and is likely too complex for this 
**	transform.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	subselp		ptr to subselect where clause parse tree fragment
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE - if subsel has view in from clause
**	    FALSE - if subsel doesn't have view in from clause
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-nov-01 (inkdo01)
**	    Written.
**	23-mar-2011 (dougi)
**	    Extend to include derived tables and common table expressions.
*/
static bool
opa_subsel_viewchk(
	OPS_STATE	   *global,
	PST_QNODE	   *subselp)

{
    PST_RNGENTRY	**rangetab = global->ops_qheader->pst_rangetab;
    i4			i;

    /* Simply loop over bits in subsel's from clause bit map, checking
    ** corresponding entries in range table for views. */

    for (i = -1; (i = BTnext(i, (char *)&subselp->pst_sym.pst_value.pst_s_root.pst_tvrm,
	BITS_IN(subselp->pst_sym.pst_value.pst_s_root.pst_tvrm))) >= 0;)
     if (rangetab[i]->pst_rgtype == PST_RTREE ||
	    rangetab[i]->pst_rgtype == PST_DRTREE ||
	    rangetab[i]->pst_rgtype == PST_WETREE) 
	return(TRUE);

    return(FALSE);		/* no views if we get here */
}


/*{
** Name: opa_subsel_byheadchk	- check for byhead node in subsel
**
** Description:
**	This function checks the subselect for a byhead node (aggregate
**	group by or having). This is an indication that there will be a
**	aggregate temp table which will break the transform.
**
** Inputs:
**	subselp		ptr to subselect where clause parse tree fragment
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE - if subsel has a byhead node
**	    FALSE - if subsel doesn't have a byhead node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-mar-02 (hayke02)
**	    Written.
**
[@history_template@]...
*/
static bool
opa_subsel_byheadchk(
	PST_QNODE	   *subselp)

{
    PST_QNODE		*r_qnode;

    for (r_qnode = subselp->pst_right; r_qnode; r_qnode = r_qnode->pst_right)
    {
	PST_QNODE	*l_qnode;

	for (l_qnode = r_qnode->pst_left; l_qnode; l_qnode = l_qnode->pst_left)
	    if (l_qnode->pst_sym.pst_type == PST_BYHEAD)
		return(TRUE);
    }
    return(FALSE);		/* no byhead nodes if we get here */
}


/*{
** Name: opa_notex_transform	- verify transform potential and do it
**
** Description:
**      This function confirms that the subselect it was given satisfies
**	the requirements for transformation and then applies said
**	transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to head of NOT EXISTS parse tree 
**			fragment (i.e., the PST_NOT node)
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	inner_root	ptr to root node of subselect 
**	allow_antijoin	flag to allow transform to antijoin.
**	underor	flag indicating whether we're a disjunct or conjunct
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means a transform was done
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**	21-nov-01 (inkdo01)
**	    Add check for views in subsel from clause - these are a complication
**	    we can't handle yet (the fact that they're still views means PSF
**	    couldn't flatten them - agg, union views or sommat like that).
**	23-nov-01 (inkdo01)
**	    Minor heuristic that avoids the transform when combination
**	    of subsel and main query tables exceeds 8 (OPF CPU time
**	    may go off the chart).
**	16-may-05 (inkdo01)
**	    Changes to allow OJ transform under PST_AGHEAD (and tidied 
**	    up placement of new predicates in parse tree).
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**	27-oct-2008 (dougi)
**	    Add antijoin transform for X100 (and later, Ingres).
**	20-nov-2010 (dougi)
**	    Collapse a bunch of stuff into a function call and support NOT
**	    EXISTS disjuncts.
**	24-Feb-2011 (kiria01)
**	    Spurious memory error avoided by not reusing node in place.
**	22-mar-2011 (dougi)
**	    Can't handle views for VW subqueries under ORs.
**	1-apr-2011 (dougi)
**	    Simplify splicing of "is null" predicate.
**	29-Jul-2011 (kiria01)
**	    Moved the IFNULL resolve up before the root->pst_right (which can =*nodep)
**	    is updated.
*/
static void
opa_notex_transform(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	PST_QNODE	   *inner_root,
	bool		   *gotone,
	bool		   allow_antijoin,
	bool		   underor)

{
    PST_QNODE	*andp, *opp;
    OPV_IGVARS	varno;
    OPL_BMOJ	**ojmapp;
    PST_J_ID	ojoinid;
    i4		ocnt = outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    i4		icnt = inner_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    bool	outer = FALSE, inner = FALSE, nested_subsel = FALSE;
    bool	equcorr = FALSE;
    bool	x100 = (global->ops_gmask & OPS_X100) ? TRUE : FALSE;
    bool	exists = ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_exists);
    DB_STATUS	status;
    DB_ERROR	error;


    /* First verify that subselect has no view (union/agg) in its from. */
    if ((!x100 || underor) && opa_subsel_viewchk(global, inner_root))
	return;

    /* Second verify that subselect has no byhead nodes */
    if (!x100 && opa_subsel_byheadchk(inner_root))
	return;

    /* If resulting from clause exceeds 8 because of addition of subsel,
    ** query may be too expensive to compile. */
    if (!x100 && ocnt + icnt > 8 && ocnt < 8)
	return;

    /* Next check if this subselect really is transformable. */
    if (!opa_notex_analyze(global, outer_root, inner_root,
	    inner_root->pst_right, &outer, &inner, &equcorr, &nested_subsel)) 
    {
	if (!(inner && x100 && exists))
	    return;				/* if not, leave right away */
    }

    /* Subselect passes the test, now do the transform:
    **   - add "subselect_table.TID is null" to where clause, or, if X100,
    **	flag as "antijoin"
    **   - subselect where clause becomes outer select on clause
    **   - subselect tables become inner to outer tables
    **   - outer tables become outer to subselect tables
    **   - subselect pst_tvrm is ORed to outer select pst_tvrm
    */

    varno = BTnext((i4)-1, (PTR)&inner_root->pst_sym.pst_value.pst_s_root.
	pst_tvrm, BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* subselect table varno */
    ojoinid = (*nodep)->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid;

    /* If not an X100 query or this is a disjunct, create "TID is null" 
    ** predicate. */
    if (!allow_antijoin || underor)
    {
	opp = opv_opnode(global, PST_UOP, 
		(exists) ? global->ops_cb->ops_server->opg_isnotnull :
		global->ops_cb->ops_server->opg_isnull, ojoinid);
	opp->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					/* stick TID var under isnull */
	(*nodep) = opp;
	/* Have to resolve the new "is null". Normally, opa_resolve does
	** this for OPF. But we don't have a subquery structure yet, and
	** the subquery is required by opa_resolve. So we'll now do the
	** new opa_gresolve() call instead. */
	opa_gresolve(global, nodep);

	/* Suck IS NULL to where NOT EXISTS used to be, then AND the 
	** subquery to the rest as outer join ON clause. */
	root->pst_right = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
				root->pst_right, inner_root->pst_right);
	root->pst_right->pst_sym.pst_value.pst_s_op.pst_joinid = ojoinid;

    }
    else (*nodep) = inner_root->pst_right;	/* subsel WHERE becomes ON */

    /* Join ID of "on" clause is 1 bigger than parse tree's pst_numjoins.
    ** Assign it to all connectives and relops in subsel where clause. */

    if (allow_antijoin)
    {
	if (underor)
	    ojmapp = &global->ops_goj.opl_subqjmap;
	else if (exists)
	    ojmapp = &global->ops_goj.opl_semijmap;
	else ojmapp = &global->ops_goj.opl_antijmap;
    }
    else ojmapp = (OPL_BMOJ **) NULL;

    /* Call replaces some common code. */
    opa_subsel_makeoj(global, header, inner_root->pst_right, root,
		outer_root, inner_root, ojmapp);

    *gotone = TRUE;			/* tell the outside world */

}

/*{
** Name: opa_notex_analyze	- analyze "not exists" subselect for
**	transformation potential
**
** Description:
**      Thus function searches the where clause of a "not exists" subselect
**	to see if it can benefit from the outer join transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	outer_root	ptr to root node of outer select 
**	inner_root	ptr to root node of subselect
**	whnode		ptr to where clause node currently being examined
**	outer		ptr to switch indicating outer VAR reference
**	inner		ptr to switch indicating inner VAR reference
**	equcorr		ptr to switch indicating "=" correlation predicate
**
** Outputs:
**      outer, inner	set to TRUE if outer/inner VAR reference found
**			in subtree
**	equcorr		set to TRUE if "=" predicate found in subtree
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**	14-june-01 (inkdo01)
**	    Fixed logic for analysis, properly look for correlation predicate
**	    and tolerate simple restrictions.
**	21-june-02 (inkdo01)
**	    Minor changes to allow subselect with multiple predicates to be
**	    properly processed (they used to incorrectly miss the flattening
**	    potential).
**	17-Aug-2009 (kiria01)
**	    Relax the restriction on OR as the structure of the qualifier will still
**	    retain the OR in place with the sub-select replaced by resdom IS NULL.
**
[@history_template@]...
*/
static bool
opa_notex_analyze(
	OPS_STATE	   *global,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *inner_root,
	PST_QNODE	   *whnode,
	bool		   *outer,
	bool		   *inner,
	bool		   *equcorr,
	bool		   *nested_subsel)
	
{
    PST_QNODE	*lnodep, *rnodep;


    /* This recursive function searches a where clause fragment (from 
    ** the subselect under consideration) to determine whether there
    ** are any correlation predicates. This is the criterion which 
    ** must be met for the subselect to be transformed to an outer
    ** join.
    **
    ** the function is a big loop which switches on the where clause node 
    ** type and tries to set "outer" and "inner" to indicate some 
    ** correlation predicate.
    */

    while (whnode && whnode->pst_sym.pst_type != PST_QLEND)
     switch (whnode->pst_sym.pst_type) 
    {
      case PST_AND:
      case PST_OR:
	opa_notex_analyze(global, outer_root, inner_root,
		whnode->pst_left, outer, inner, equcorr, nested_subsel);
					/* recurse down left side */
	if (*nested_subsel) return(FALSE);  /* nested subselect forces return */

	whnode = whnode->pst_right;	/* iterate down right side */
	break;

      case PST_BOP:
	/* First, check for "=" correlation predicate. */
	if (whnode->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_eq &&
	    (lnodep = whnode->pst_left)->pst_sym.pst_type == PST_VAR &&
	    (rnodep = whnode->pst_right)->pst_sym.pst_type == PST_VAR &&
	    (BTtest((u_i2)lnodep->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm) &&
	    BTtest((u_i2)rnodep->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm) ||
	    BTtest((u_i2)lnodep->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm) &&
	    BTtest((u_i2)rnodep->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm)))
	{
	    *equcorr = *outer = *inner = TRUE;
	    return(!(*nested_subsel));
	}

	/* Otherwise, recurse. */
	opa_notex_analyze(global, outer_root, inner_root,
		whnode->pst_left, outer, inner, equcorr, nested_subsel);
					/* recurse down left side */
	if (*nested_subsel) return(FALSE);  /* nested subselect forces return */

	whnode = whnode->pst_right;	/* iterate down right side */
	break;

      case PST_NOT:
      case PST_UOP:
	whnode = whnode->pst_left;	/* just iterate on NOT & UOP */
	break;

      case PST_CONST:
	return(*outer && *inner &&!(*nested_subsel));

      case PST_SUBSEL:
	/* Transform doesn't work with nested subselects. */
	*nested_subsel = TRUE;
	return(FALSE);

      case PST_VAR:
	/* Check if column covers either outer or inner select. */
	if (BTtest((u_i2)whnode->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    *outer = TRUE;
	else if (BTtest((u_i2)whnode->pst_sym.pst_value.pst_s_var.pst_vno,
		(PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    *inner = TRUE;

	return(*outer && *inner &&!(*nested_subsel));

      default:
	return(FALSE);			/* all others return empty */

    }
    return(*outer && *inner && !(*nested_subsel));

}

/*{
** Name: opa_neany_transform	- transform "... <> any (..."
**
** Description:
**      This function transforms the "... <> any (..." subquery into a weird
**	and wonderful equivalent exists subquery that avoids the need for 
**	SE joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	bopp		ptr to "=" or "<>" node of subsel compare
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    modified parse tree under bopp.
**
** History:
**      26-oct-2009 (dougi)
**	    Written.
*/
static VOID
opa_neany_transform(
	OPS_STATE	*global,
	PST_QNODE	*bopp)

{
    PST_QNODE	*aghp, *subsp, *rsdmp, *sqwhp, *expr1p, *expr2p, *wrk1p, *wrk2p;


    /* ... expr1 <> any (select expr2 ... is transformed to ... exists (
    ** select 1 ... where ... having count(expr2) > 0 and (max(expr2) <>
    ** min(expr2) or max(expr2) <> expr1) ... */

    /* Start by saving stuff - expr1, the SUBSEL ptr, its RESDOM ptr, ... */
    expr1p = bopp->pst_left;
    subsp = bopp->pst_right;
    rsdmp = subsp->pst_left;
    expr2p = rsdmp->pst_right;
    rsdmp->pst_right = opv_intconst(global, 1);
    sqwhp = subsp->pst_right;
    bopp->pst_left = subsp;
    bopp->pst_right = (PST_QNODE *) NULL;

    /* Change original <> operator to the "exists". */
    bopp->pst_sym.pst_type = PST_UOP;
    bopp->pst_sym.pst_value.pst_s_op.pst_opno = 
				global->ops_cb->ops_server->opg_exists;
    bopp->pst_sym.pst_value.pst_s_op.pst_opinst = -1;

    /* Copy PST_SUBSEL to turn into PST_AGHEAD. */
    subsp->pst_left = subsp->pst_right = (PST_QNODE *) NULL;
    aghp = subsp;
    opv_copytree(global, &aghp);
    subsp->pst_left = rsdmp;
    subsp->pst_sym.pst_value.pst_s_root.pst_mask1 = (PST_1CORR_VAR_FLAG_VALID |
							PST_2CORR_VARS_FOUND);
    aghp->pst_sym.pst_type = PST_AGHEAD;

    /* Generate "count(expr2) > 0" and attach with AND. */
    aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
				opg_count, expr2p, (PST_QNODE *) NULL);
    STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
    aghp->pst_right = sqwhp;
    wrk1p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
				opg_gt, aghp, opv_intconst(global, 0));
    subsp->pst_right = wrk1p = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
				wrk1p, (PST_QNODE *)NULL);

    /* Generate "min(expr2) <> max(expr2)" and stick under OR. */
    opv_copytree(global, &aghp);
    aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_max, aghp->pst_left->pst_left, (PST_QNODE *) NULL);
    wrk2p = aghp;
    opv_copytree(global, &aghp);
    aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_min, aghp->pst_left->pst_left, (PST_QNODE *) NULL);
    wrk2p = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_ne,
			wrk2p, aghp);
    wrk1p->pst_right = wrk2p = opa_makeop(global, PST_OR, (ADI_OP_ID) -1,
			wrk2p, (PST_QNODE *) NULL);

    /* Generate "min(expr2) <> expr1" and attach to the end. */
    opv_copytree(global, &aghp);
    STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
    wrk2p->pst_right = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_ne, aghp, expr1p);

}

/*{
** Name: opa_noncorrall_transform - transform non-correlated all subqueries
**
** Description:
**      This function transforms the "... <compop> all (..." subqueries into
**	equivalent join queries with a derived table to compute the subquery
**	thus avoiding SE joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	outer_root	ptr to root of outer query
**	inner_root	ptr to root of inner query
**	nodepp		ptr to ptr to compop BOP node
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    modified parse tree under bopp.
**
** History:
**      28-oct-2009 (dougi)
**	    Written.
**	6-july-2011 (dougi) m2219
**	    Set new range var (for derived table) in inner root, too.
*/
static VOID
opa_noncorrall_transform(
	OPS_STATE	*global,
	PST_QNODE	*outer_root,
	PST_QNODE	*inner_root,
	PST_QNODE	**nodepp)

{

    PST_QNODE	*rootp, *bopp, *aghp, *rsdmp, *sqwhp, *expr1p, *expr2p, *andp, *orp, *wrk1p;
    PST_QNODE	*varcsp, *varcep, *varaep, *vara1p;
    ADI_FI_DESC	*fidesc;
    PST_RNGENTRY **tempptr;
    i4		rngno, ceno;
    ADI_OP_ID	opcode, aggop;
    DB_ATT_ID	attid;
    DB_STATUS	status;
    DB_ERROR	error;


    /* ... expr1 <compop> all (select expr2 ... is transformed to ... 
    ** select ... from ..., (select max/min(expr2) as maxq/minq, count(expr2)
    ** as counts[, count(expr2) as counte] from ... ) as subqnnn ... where ...
    ** and (counts = 0 or (expr1 >/< maxq/minq [and counts = counte])
    */

    /* Figure out whether we need max or min. */
    bopp = *nodepp;
    if ((opcode = bopp->pst_sym.pst_value.pst_s_op.pst_opno) == 
				global->ops_cb->ops_server->opg_ge || 
	    opcode == global->ops_cb->ops_server->opg_gt)
	aggop = global->ops_cb->ops_server->opg_max;
    else aggop = global->ops_cb->ops_server->opg_min;

    /* Copy existing range table and allocate/init entry for derived table. */
    rngno = global->ops_qheader->pst_rngvar_count;
    if (rngno + 1 >= PST_NUMVARS)
        opx_error(E_OP0202_VAR_OVERFLOW);   /* report error if no parser range
                                            ** variables are available */
    tempptr = (PST_RNGENTRY **)opu_memory(global, (i4)((rngno + 1)* 
						sizeof(PST_RNGENTRY *)));
    MEcopy((PTR) global->ops_qheader->pst_rangetab, 
            (rngno * sizeof(PST_RNGENTRY *)), (PTR)tempptr);
    global->ops_qheader->pst_rangetab = tempptr; 
    global->ops_qheader->pst_rngvar_count++;
    global->ops_qheader->pst_rangetab[rngno] =
        (PST_RNGENTRY *)opu_memory(global, (i4)sizeof(PST_RNGENTRY));
    MEfill(sizeof(PST_RNGENTRY), 0, 
	(char *)global->ops_qheader->pst_rangetab[rngno]);
    global->ops_qheader->pst_rangetab[rngno]->pst_rgtype = PST_DRTREE;
    BTset(rngno, (char *)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
						/* flag ref to derived table */
    if (inner_root != NULL && inner_root->pst_sym.pst_type == PST_AGHEAD)
	BTset(rngno, (char *)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm);

    /* Then save stuff - expr1, the SUBSEL ptr, its RESDOM ptr, ... */
    expr1p = bopp->pst_left;
    rootp = bopp->pst_right;
    global->ops_qheader->pst_rangetab[rngno]->pst_rgroot = rootp;
    rsdmp = rootp->pst_left;
    expr2p = rsdmp->pst_right;
    rsdmp->pst_right = opv_intconst(global, 1);
    sqwhp = rootp->pst_right;
    rootp->pst_sym.pst_type = PST_ROOT;		/* SUBSEL becomes new ROOT */
    rootp->pst_left = rootp->pst_right = (PST_QNODE *) NULL;
    aghp = rootp;
    opv_copytree(global, &aghp);		/* make first AGHEAD */
    rootp->pst_right = opv_qlend(global);

    /* Start building AGHEAD structures anchoring aggs. Count(*) first, then 
    ** max/min and, if nullable sq result, count(expr2). */
    aghp->pst_sym.pst_type = PST_AGHEAD;
    aghp->pst_right = sqwhp;
    aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->ops_server->
			opg_scount, (PST_QNODE *) NULL,(PST_QNODE *) NULL);
    STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
    wrk1p = rsdmp = opv_resdom(global, aghp, &aghp->pst_sym.pst_dataval);
    wrk1p->pst_left = opv_qlend(global);
    wrk1p->pst_left->pst_sym.pst_type = PST_TREE;
    wrk1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 1;
    rootp->pst_left = wrk1p;

    opv_copytree(global, &aghp);		/* make next AGHEAD */
    aghp->pst_left = opa_makeop(global, PST_AOP, aggop, expr2p, 
							(PST_QNODE *) NULL);
    STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
    wrk1p = opv_resdom(global, aghp, &aghp->pst_sym.pst_dataval);
    wrk1p->pst_left = rootp->pst_left;
    wrk1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 2;
    rootp->pst_left = wrk1p;

    if (opcode == global->ops_cb->ops_server->opg_eq)
    {
	/* "= all" requires min and max. */
	opv_copytree(global, &aghp);		/* make next AGHEAD */
	aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->
			ops_server->opg_max, expr2p, (PST_QNODE *) NULL);
	STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
	wrk1p = opv_resdom(global, aghp, &aghp->pst_sym.pst_dataval);
	wrk1p->pst_left = rootp->pst_left;
	wrk1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 3;
	rootp->pst_left = wrk1p;
	ceno = 4;
    }
    else ceno = 3;

    if (expr2p->pst_sym.pst_dataval.db_datatype < 0)
    {
	/* Nullable subquery result requires computing count(expr2), too. */
	opv_copytree(global, &aghp);		/* make next AGHEAD */
	aghp->pst_left = opa_makeop(global, PST_AOP, global->ops_cb->
			ops_server->opg_count, expr2p, (PST_QNODE *) NULL);
	STRUCT_ASSIGN_MACRO(aghp->pst_left->pst_sym.pst_dataval, 
		aghp->pst_sym.pst_dataval);
	wrk1p = opv_resdom(global, aghp, &aghp->pst_sym.pst_dataval);
	wrk1p->pst_left = rootp->pst_left;
	wrk1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno = ceno;
	rootp->pst_left = wrk1p;
    }

    /* Derived table is done - now splice into main query. */
    (*nodepp) = orp = opa_makeop(global, PST_OR, (ADI_OP_ID)-1, 
				(PST_QNODE *) NULL, (PST_QNODE *) NULL);

    /* "count(*) = 0" */
    attid.db_att_id = 1;
    varcsp = opv_varnode(global, &rsdmp->pst_sym.pst_dataval, rngno, &attid);
    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
	sizeof(DB_ATT_NAME), 
	(char *)&varcsp->pst_sym.pst_value.pst_s_var.pst_atname);
    orp->pst_left = opa_makeop(global, PST_BOP, global->ops_cb->ops_server->
			opg_eq, varcsp, opv_intconst(global, 0));

    /* "expr1 >/< max/min" */
    rsdmp = rootp->pst_left;
    if (opcode == global->ops_cb->ops_server->opg_eq)
	rsdmp = rsdmp->pst_left;
    if (expr2p->pst_sym.pst_dataval.db_datatype < 0)
	rsdmp = rsdmp->pst_left;
    attid.db_att_id = 2;
    varaep = opv_varnode(global, &rsdmp->pst_sym.pst_dataval, rngno, &attid);
    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
	sizeof(DB_ATT_NAME), 
	(char *)&varaep->pst_sym.pst_value.pst_s_var.pst_atname);
    orp->pst_right = opa_makeop(global, PST_BOP, opcode, expr1p, varaep);

    /* If "= ALL", add "max(expr2) = min(expr2)". */
    if (opcode == global->ops_cb->ops_server->opg_eq)
    {
	andp = opa_makeop(global, PST_AND, (ADI_OP_ID)-1, (PST_QNODE *) NULL,
					orp->pst_right);
	orp->pst_right = andp;
	rsdmp = rootp->pst_left;
	if (expr2p->pst_sym.pst_dataval.db_datatype < 0)
	    rsdmp = rsdmp->pst_left;		/* skip count(expr2) */

	attid.db_att_id = 3;
	vara1p = opv_varnode(global, &rsdmp->pst_sym.pst_dataval, rngno, &attid);
	MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
	    sizeof(DB_ATT_NAME), 
	    (char *)&vara1p->pst_sym.pst_value.pst_s_var.pst_atname);
	andp->pst_left = opa_makeop(global, PST_BOP, global->ops_cb->
				ops_server->opg_eq, varaep, vara1p);
	orp = andp;
    }
    
    /* If expr2 is nullable, also do "count(*) = count(expr2)". */
    if (expr2p->pst_sym.pst_dataval.db_datatype < 0)
    {
	andp = opa_makeop(global, PST_AND, (ADI_OP_ID)-1, (PST_QNODE *) NULL,
					orp->pst_right);
	orp->pst_right = andp;
	rsdmp = rootp->pst_left;
	attid.db_att_id = ceno;
	varcep = opv_varnode(global, &rsdmp->pst_sym.pst_dataval, rngno, &attid);
	MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
	    sizeof(DB_ATT_NAME), 
	    (char *)&varcep->pst_sym.pst_value.pst_s_var.pst_atname);
	andp->pst_left = opa_makeop(global, PST_BOP, global->ops_cb->
				ops_server->opg_eq, varcsp, varcep);
    }
    
}

/*{
** Name: opa_corrall_transform	- transform correlated all subqueries
**
** Description:
**      This function transforms the "... <compop> all (..." subqueries into
**	equivalent "not exists" subqueries that avoids the need for 
**	SE joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodepp		ptr to ptr to compop BOP node
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    modified parse tree under bopp.
**
** History:
**      27-oct-2009 (dougi)
**	    Written.
*/
static VOID
opa_corrall_transform(
	OPS_STATE	*global,
	PST_QNODE	**nodepp)

{

    PST_QNODE	*bopp, *aghp, *subsp, *rsdmp, *sqwhp, *expr1p, *expr2p, *andp, *orp;
    ADI_FI_DESC	*fidesc;
    DB_STATUS	status;
    DB_ERROR	error;


    /* ... expr1 <compop> all (select expr2 ... is transformed to ... 
    ** not exists (select 1 ... where ... and (expr1 <revcompop> expr2 or
    ** expr2 is null) ... */

    /* First, flip the comparison operation. */
    bopp = *nodepp;
    status = adi_fidesc(global->ops_adfcb, bopp->pst_sym.pst_value.
				pst_s_op.pst_fdesc->adi_cmplmnt, &fidesc);
    if (DB_FAILURE_MACRO(status))
	    opx_verror( status, E_OP0781_ADI_FIDESC,
		global->ops_adfcb->adf_errcb.ad_errcode); /* error if
			** the function instance descriptor cannot be found */

    /* Then save stuff - expr1, the SUBSEL ptr, its RESDOM ptr, ... */
    expr1p = bopp->pst_left;
    subsp = bopp->pst_right;
    rsdmp = subsp->pst_left;
    expr2p = rsdmp->pst_right;
    rsdmp->pst_right = opv_intconst(global, 1);
    sqwhp = subsp->pst_right;
    bopp->pst_left = subsp;
    bopp->pst_right = (PST_QNODE *) NULL;

    /* Replace "<compop>" by "not exists". */
    *nodepp = bopp;
    bopp->pst_sym.pst_type = PST_UOP;
    memset(&bopp->pst_sym.pst_value.pst_s_op, 0, sizeof(PST_OP_NODE));
    bopp->pst_sym.pst_value.pst_s_op.pst_opno = 
			global->ops_cb->ops_server->opg_nexists;
    bopp->pst_sym.pst_value.pst_s_op.pst_opinst = -1;
    
    /* Glue "and" at head of WHERE clause, then place "or" on its lhs. */
    subsp->pst_right = andp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
				(PST_QNODE *) NULL, sqwhp);
    andp->pst_left = orp = opa_makeop(global, PST_OR, (ADI_OP_ID) -1,
				(PST_QNODE *) NULL, (PST_QNODE *) NULL);

    /* expr1 <revcompop> expr2 or expr2 is null */
    orp->pst_left = opa_makeop(global, PST_BOP, fidesc->adi_fiopid, expr1p,
				expr2p);
    opv_copytree(global, &expr2p);
    orp->pst_right = opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_isnull, expr2p, (PST_QNODE *) NULL);

}

/*{
** Name: opa_notin_transform	- verify transform potential and do it
**
** Description:
**      This function confirms that the subselect it was given satisfies
**	the requirements for transformation and then applies said
**	transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to head of NOT IN parse tree 
**			fragment (i.e., the PST_BOP node)
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	bopp		ptr to "=" or "<>" node of subsel compare
**	allow_antijoin	flag to allow transform to antijoin.
**	underor		flag indicating whether we're a disjunct or conjunct
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means a transform was done
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-aug-99 (inkdo01)
**	    Written.
**	21-nov-01 (inkdo01)
**	    Add check for views in subsel from clause: these are a complication 
**	    we can't handle yet (the fact that they're still views means PSF
**	    couldn't flatten them - agg, union views or sommat like that).
**	23-nov-01 (inkdo01)
**	    Minor heursitic that avoids the transform when combination
**	    of subsel and main query tables exceeds 8 (OPF CPU time
**	    may go off the chart).
**	20-mar-01 (hayke02)
**	    Add a pst_resolve() call for the bopp node ("=" for on clause).
**	    This prevents "<>" predicates from erroneously appearing in the
**	    query tree. This change fixes bug 107309.
**	25-oct-02 (inkdo01)
**	    Disallow "not in"s when both comparands are expressions (creates
**	    CP join instead of more efficient join).
**	30-mar-04 (hayke02)
**	    Back out the previous change and fix INGSRV 1957, bug 108930 in
**	    opz_fboth() instead. This change fixes problem INGSRV 2732, bug
**	    111899.
**	16-may-05 (inkdo01)
**	    Changes to aloow OJ transform under PST_AGHEAD (and tidied 
**	    up placement of new predicates in parse tree).
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**	27-oct-2008 (dougi)
**	    Add antijoin transform for X100 (and later, Ingres).
**	28-sep-2009 (dougi)
**	    Logic to broaden antijoin support to all [NOT]ANY/ALL subselects
**	    (for X100).
**	22-nov-2010 (dougi)
**	    Collapse a bunch of stuff into a function call.
**	15-mar-2011 (dougi)
**	    Fix bad op in "compop any" under OR.
*/
static void
opa_notin_transform(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	PST_QNODE	   *bopp,
	bool		   *gotone,
	bool		   allow_antijoin,
	bool		   underor)

{
    PST_QNODE	*andp, *varp, *opp, *workp;
    PST_QNODE	*inner_root = bopp->pst_right;	/* PST_SUBSEL ptr */
    OPL_BMOJ	**ojmapp;
    OPV_IGVARS	varno;
    ADI_OP_ID	opcode;
    PST_J_ID	ojoinid;
    i4		ocnt = outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    i4		icnt = inner_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    i4		opmeta = bopp->pst_sym.pst_value.pst_s_op.pst_opmeta;
    bool	outer = FALSE, inner = FALSE, nested_subsel = FALSE;
    bool	notin = FALSE, correlated, equcorr = FALSE;
    bool	x100 = (global->ops_gmask & OPS_X100) ? TRUE : FALSE;
    DB_STATUS	status;
    DB_ERROR	error;
    ADI_FI_DESC	*fidesc = bopp->pst_sym.pst_value.pst_s_op.pst_fdesc;


    /* First check if this subselect really is transformable. */
    if (inner_root->pst_left->pst_sym.pst_type != PST_RESDOM) 
			return;		/* sanity check */
    if (!x100 && opa_subsel_viewchk(global, inner_root))
			return;		/* fails view-in-subsel-from test */

    /* Verify that subselect has no byhead nodes */
    if (!x100 && opa_subsel_byheadchk(inner_root))
	return;

    if (ocnt + icnt > 8 && ocnt < 8)
			return;		/* resulting from is too big */
    if (!opa_notin_analyze(global, outer_root, bopp->pst_left))
			return;		/* fails outer operand check */
    if (!opa_notin_analyze(global, inner_root, inner_root->pst_left->pst_right))
			return;		/* fails inner operand check (subselect
					** RESDOM) */
    /* Borrow "not exists" check rtne to determine if there are nested
    ** subselects. This isn't allowed. */
    opa_notex_analyze(global, outer_root, inner_root,
	    inner_root->pst_right, &outer, &inner, &equcorr, &nested_subsel);
    if (!x100 && nested_subsel) return;

    opcode = bopp->pst_sym.pst_value.pst_s_op.pst_opno;

    correlated =  ((inner & outer) || 
	(allow_antijoin && opmeta == PST_CARD_ANY && 
	    opcode == global->ops_cb->ops_server->opg_eq));

    if (!correlated && (opmeta == PST_CARD_ANY ||
					opmeta == PST_CARD_ALL))
    {
	PST_QNODE	*rsdmp, *subsp;
	ADI_OP_ID	aggop;

	/* Transform non-correlated, quantified subqueries into compare
	** with aggregates:
	** > ALL => > max, >= ALL => >= max, <= ALL => <= min, < ALL => < min
	** > ANY => > min, >= ANY => >= min, <= ANY => <= max, < ANY => < max.
	**
	** And <> ANY/= ALL are weird and require special treatment.
	**
	** And = ANY/<> ALL fall off the bottom and are handled elsewhere.
	*/
	if (opmeta == PST_CARD_ANY &&
	    opcode == global->ops_cb->ops_server->opg_ne)
	{
	    if (underor)
		return;			/* these don't work under ORs */

	    opa_neany_transform(global, bopp);
	    /* Re-lace AGHEADs to pick up recent additions */
	    opa_subsel_agh_lace(global, outer_root, TRUE);
	    *gotone = TRUE;
	    return;
	}
	else if (opmeta == PST_CARD_ALL &&
	    opcode != global->ops_cb->ops_server->opg_ne)
	{
	    if (underor)
		return;			/* these don't work under ORs */

	    opa_noncorrall_transform(global, outer_root, root, nodep);
	    /* Re-lace AGHEADs to pick up recent additions */
	    opa_subsel_agh_lace(global, outer_root, TRUE);
	    *gotone = TRUE;
	    return;
	}
	else if (opcode != global->ops_cb->ops_server->opg_ne)
	{
	    if (underor)
		return;			/* these don't work under ORs */

	    /* Just the ANY <, <=, >=, > comparisons left to do. */
	    if (opcode == global->ops_cb->ops_server->opg_le || 
		    opcode == global->ops_cb->ops_server->opg_lt)
		aggop = global->ops_cb->ops_server->opg_max;
	    else aggop = global->ops_cb->ops_server->opg_min;

	    /* The next bunch of code saves the RESDOM from the subselect,
	    ** makes an AGHEAD under the RESDOM (by copying the SUBSEL), 
	    ** makes an AOP under the AGHEAD to perform the aggregation, 
	    ** and moves the subselect where clause under the AGHEAD. */
	    subsp = bopp->pst_right;

	    rsdmp = subsp->pst_left;
	    varp = rsdmp->pst_right;
	    workp = subsp->pst_right;
	
	    subsp->pst_right = subsp->pst_left = (PST_QNODE *) NULL;
	    rsdmp->pst_right = subsp;
	    opv_copytree(global, &rsdmp->pst_right);
	    subsp->pst_left = rsdmp;
	    rsdmp->pst_right->pst_sym.pst_type = PST_AGHEAD;
	    rsdmp->pst_right->pst_right = workp;
	    rsdmp->pst_right->pst_left = opa_makeop(global, PST_AOP, aggop,
			varp, (PST_QNODE *) NULL);

	    if (opmeta == PST_CARD_ALL && 
			varp->pst_sym.pst_dataval.db_datatype < 0)
	    {
		PST_QNODE	*aghp, *agh1p;

		/* Subquery is nullable - now we must add tests to assure 
		** all values are non-null. Replicate the aggregate twice,
		** one is count(expr), the other is count(*), and stick
		** "count(expr) = count(*)" on rhs of subselect. 
		*/
		aghp = rsdmp->pst_right;
		opv_copytree(global, &aghp);
		aghp->pst_left->pst_sym.pst_value.pst_s_op.pst_opno =
			global->ops_cb->ops_server->opg_count;

		opa_gresolve(global, &aghp->pst_left);

		agh1p = aghp;
		opv_copytree(global, &agh1p);
		agh1p->pst_left->pst_left = (PST_QNODE *) NULL;
		agh1p->pst_left->pst_sym.pst_value.pst_s_op.pst_opno =
			global->ops_cb->ops_server->opg_scount;

		opa_gresolve(global, &aghp->pst_left);

		subsp->pst_right = opa_makeop(global, PST_BOP,
		    global->ops_cb->ops_server->opg_eq, aghp, agh1p);
	    }
	    else subsp->pst_right = opv_qlend(global);

	    bopp->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_CARD_01R;
	    /* Re-lace AGHEADs to pick up recent additions */
	    opa_subsel_agh_lace(global, outer_root, TRUE);
	    *gotone = TRUE;
	    return;			/* that's all for these guys */
	}
    }

    /* If antijoin and "<compop> ALL ..." where <compop> isn't "<>",
    ** transform into a "not exists". */
    if (allow_antijoin && opmeta == PST_CARD_ALL && 
		opcode != global->ops_cb->ops_server->opg_ne)
    {
	opa_corrall_transform(global, nodep);
	opa_notex_transform(global, nodep, header, outer_root, root,
		bopp->pst_left, gotone, allow_antijoin, underor);
	return;
    }

    /* Subselect passes the test, now do the transform:
    **   - add "subselect_table.TID is null" to where clause, or, if X100,
    **	flag as "antijoin"
    **   - and the bopp comparison to subselect where clause
    **   - subselect where clause becomes outer select on clause
    **   - subselect tables become inner to outer tables
    **   - outer tables become outer to subselect tables
    **   - subselect pst_tvrm is ORed to outer select pst_tvrm
    */

    varno = BTnext((i4)-1, (PTR)&inner_root->pst_sym.pst_value.pst_s_root.
	pst_tvrm, BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* subselect table varno */
    ojoinid = (*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid;

    if (!allow_antijoin || underor)
    {
	ADI_OP_ID	opid;

	if (underor && opmeta == PST_CARD_ANY)
	    opid = global->ops_cb->ops_server->opg_isnotnull;
	else opid = global->ops_cb->ops_server->opg_isnull;
	(*nodep) = opp = opv_opnode(global, PST_UOP, opid, ojoinid);
					/* make UOP (is[not]null) to AND to
					** outer where clause */
	opp->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					/* stick TID var under isnull */
	opp->pst_right = NULL;		/* finish up is null pred. */

	/* Have to resolve the new "is null". Normally, opa_resolve does
	** this for OPF. But we don't have a subquery structure yet, and
	** the subquery is required by opa_resolve. So we'll use the new
	** opa_gresolve() call instead. */

	opa_gresolve(global, nodep);
    }
    else (*nodep) = opv_qlend(global);

    /* Stick AND on root rhs to address (on left) the new oj ON clause. */
    andp = root->pst_right = opa_makeop(global, PST_AND, (ADI_OP_ID) -1, 
	    opa_makeop(global, PST_AND, (ADI_OP_ID) -1, bopp, 
		    inner_root->pst_right), root->pst_right);
    workp = andp->pst_left;		/* (for convenience of reference) */

    /* "... x in (select ..." might not have a WHERE clause. */
    if (andp->pst_left->pst_right == (PST_QNODE *) NULL)
	andp->pst_left->pst_right = opv_qlend(global);

    bopp->pst_right = inner_root->pst_left->pst_right;
					/* suck subsel resdom expr under 
					** "="/"<>" BOP */

    if (!allow_antijoin || (bopp->pst_sym.pst_value.pst_s_op.pst_opno ==
	    global->ops_cb->ops_server->opg_ne && opmeta == PST_CARD_ALL))
    {
	/* "<> ALL"/"NOT IN" becomes the antijoin of "= ANY". */
	bopp->pst_sym.pst_value.pst_s_op.pst_opno = 
				global->ops_cb->ops_server->opg_eq;
					/* make it an "=" for on clause */
	opmeta = PST_CARD_ANY;
	notin = TRUE;
    }
   
    opa_gresolve(global, &root->pst_right->pst_left->pst_left); /*bopp*/

    ojmapp = (OPL_BMOJ **) NULL;
    if (allow_antijoin)
    {
	/* Under an OR becomes weird subquery join. */
	if (underor)
	    ojmapp = &global->ops_goj.opl_subqjmap;
	/* Correlated ANY becomes semijoin. */
	else if (opmeta == PST_CARD_ANY && !notin)
	    ojmapp = &global->ops_goj.opl_semijmap;
	else if (notin)
	{
	    /* NOT IN beomes antijoin on "=" test. */
	    ojmapp = &global->ops_goj.opl_antijmap;

	    /* Complement the <compop> (again if it started as a "NOT ANY"). */
	    status = adi_fidesc(global->ops_adfcb, fidesc->adi_cmplmnt, &fidesc);
	    if (DB_FAILURE_MACRO(status))
		opx_verror( status, E_OP0781_ADI_FIDESC,
		    global->ops_adfcb->adf_errcb.ad_errcode); /* error if
			** the function instance descriptor cannot be found */

	    bopp->pst_sym.pst_value.pst_s_op.pst_opno = fidesc->adi_fiopid;
	    bopp->pst_sym.pst_value.pst_s_op.pst_opinst = fidesc->adi_finstid;
	    bopp->pst_sym.pst_value.pst_s_op.pst_fdesc = fidesc;
	}
    }
    bopp->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;

    /* Call replaces some common code. */
    opa_subsel_makeoj(global, header, workp, root,
		outer_root, inner_root, ojmapp);

    *gotone = TRUE;			/* tell the outside world */

}

/*{
** Name: opa_notin_analyze	- analyze "not in"/"<> all" subselect for
**	transformation potential
**
** Description:
**      This function examines the select list of the subselect for a 
**	column in the subselect from list, and the "not in"/"<>" comparand for 
**	a column in the outer select.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	root		ptr to root node of outer/inner select 
**	opp		ptr to comparand being checked
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE - if operand expression contains VAR in from clause
**	    FALSE - otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-aug-99 (inkdo01)
**	    Written.
**	25-oct-02 (inkdo01)
**	    Disallow "not in"s when both comparands are expressions (creates
**	    CP join instead of more efficient join).
**	30-mar-04 (hayke02)
**	    Back out the previous change and fix INGSRV 1957, bug 108930 in
**	    opz_fboth() instead. This change fixes problem INGSRV 2732, bug
**	    111899.
[@history_template@]...
*/
static bool
opa_notin_analyze(
	OPS_STATE	   *global,
	PST_QNODE	   *root,
	PST_QNODE	   *opp)
	
{


    /* This recursive function searches an operand expression (from the
    ** "not in"/"<>" comparison) to determine if it contains a column 
    ** reference to a table in the corresponding root/subsel node. 
    **
    ** The function is a big loop which switches on the operand node 
    ** type, looking for a PST_VAR which maps onto the appropriate
    ** from clause.
    */

    while (opp)
     switch (opp->pst_sym.pst_type) 
    {

      case PST_BOP:
	if (opa_notin_analyze(global, root, opp->pst_left))
		return(TRUE);		/* recurse down left side */

	opp = opp->pst_right;		/* iterate down right side */
	break;

      case PST_NOT:
      case PST_UOP:
	opp = opp->pst_left;		/* just iterate on NOT & UOP */
	break;

      case PST_CONST:
	return(TRUE);

      case PST_VAR:
	/* Check if column covers either outer or inner select. */
	if (BTtest((u_i2)opp->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    return(TRUE);
	else return(FALSE);

      default:
	return(FALSE);			/* all others return empty */

    }
    return(FALSE);

}

/*{
** Name: opa_demorgan	- push NOT through predicate expression as per DeMorgan
**
** Description:
**      This function applies DeMorgan transforms on all but "exists" 
**	predicates.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	notpp		ptr to ptr to NOT being pushed
**	nodepp		ptr to ptr to node that NOT is being pushed through
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    modifies parse tree to apply deMorgan transforms
**
** History:
**      10-dec-2009 (dougi)
**	    Written.
**	03-Feb-2010 (kiria01)
**	    Cater for degenerate AND/OR nodes
**	24-Feb-2011 (kiria01)
**	    Include EXISTS & NOT EXISTS
*/
static void
opa_demorgan(
	OPS_STATE	   *global,
	PST_QNODE	   **notpp,
	PST_QNODE	   **nodepp)

{
    PST_QNODE		*notp;
    ADI_FI_DESC		*fidesc;
    DB_STATUS		status;
    i4			opmeta;

    while (nodepp && *nodepp)
    {
	/* Simply a switch to perform the required transforms. */
	switch ((*nodepp)->pst_sym.pst_type)
	{
	case PST_NOT:
	    /* Dumb as it may seem to do "not not ...", it is easy to fix.
	    ** Just eat both "not"s and return. */
	    *notpp = (*nodepp)->pst_left;
	    return;

	case PST_UOP:
	    if ((*nodepp)->pst_sym.pst_value.pst_s_op.pst_opno == 
			global->ops_cb->ops_server->opg_exists)
	    {
		(*nodepp)->pst_sym.pst_value.pst_s_op.pst_opno =
			global->ops_cb->ops_server->opg_nexists;
		*notpp = *nodepp;
		return;
	    }
	    if ((*nodepp)->pst_sym.pst_value.pst_s_op.pst_opno == 
			global->ops_cb->ops_server->opg_nexists)
	    {
		(*nodepp)->pst_sym.pst_value.pst_s_op.pst_opno =
			global->ops_cb->ops_server->opg_exists;
		*notpp = *nodepp;
		return;
	    }
	    /* Other UOPs (e.g. is null) drop through. */

	case PST_BOP:
	    /* Find complement and replace original. */
	    status = adi_fidesc(global->ops_adfcb, (*nodepp)->pst_sym.pst_value.
				pst_s_op.pst_opinst, &fidesc);
	    if (DB_FAILURE_MACRO(status))
		    opx_verror(status, E_OP0781_ADI_FIDESC,
					global->ops_adfcb->adf_errcb.ad_errcode);

	    if (fidesc->adi_cmplmnt == ADI_NOFI)
		return;			/* shouldn't really happen */

	    status = adi_fidesc(global->ops_adfcb, fidesc->adi_cmplmnt, &fidesc);
	    if (DB_FAILURE_MACRO(status))
		opx_verror(status, E_OP0781_ADI_FIDESC,
				global->ops_adfcb->adf_errcb.ad_errcode);

	    /* Got the complement - overlay original BOP/UOP. */
	    (*nodepp)->pst_sym.pst_value.pst_s_op.pst_opno = fidesc->adi_fiopid;
	    (*nodepp)->pst_sym.pst_value.pst_s_op.pst_opinst = fidesc->adi_finstid;
	    (*nodepp)->pst_sym.pst_value.pst_s_op.pst_fdesc = fidesc;

	    /* If quantified BOP, flip the quantification. */
	    opmeta = (*nodepp)->pst_sym.pst_value.pst_s_op.pst_opmeta;
	    if (opmeta == PST_CARD_ANY)
		(*nodepp)->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_CARD_ALL;
	    else if (opmeta == PST_CARD_ALL)
		(*nodepp)->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_CARD_ANY;

	    /* Finally, replace NOT with UOP/BOP. */
	    *notpp = (*nodepp);
	    return;

	case PST_AND:
	case PST_OR:
	    /* NOT a AND b ==> NOT a OR NOT b and 
	    ** NOT a OR b ==> NOT a AND NOT b.
	    ** However, check for and drop degenerate
	    ** AND/OR nodes */
	    if (!(*nodepp)->pst_left)
	    {
		/* Drop node & adopt right */
		*nodepp = (*nodepp)->pst_right;
		continue;
	    }
	    if (!(*nodepp)->pst_right)
	    {
		/* Drop node & adopt left */
		*nodepp = (*nodepp)->pst_left;
		continue;
	    }
	    (*nodepp)->pst_sym.pst_type = ((*nodepp)->pst_sym.pst_type == PST_AND) ?
					PST_OR : PST_AND;	/* flip pst_type */

	    /* Push NOT below left and right side. */
	    notp = (*notpp);
	    (*notpp) = (*nodepp);
	    notp->pst_left = (*nodepp)->pst_left;
	    (*notpp)->pst_left = notp;
	    (*notpp)->pst_right = opa_makeop(global, PST_NOT, (ADI_OP_ID) -1, 
	    			(*notpp)->pst_right, (PST_QNODE *) NULL);
	    opa_demorgan(global, &(*notpp)->pst_left, &(*notpp)->pst_left->pst_left);
	    /* Tail recurse:
	    ** opa_demorgan(global, &(*notpp)->pst_right, 
	    **				&(*notpp)->pst_left->pst_right);*/
	    nodepp = &(*notpp)->pst_right->pst_left;
	    notpp = &(*notpp)->pst_right;
	    continue;

	default:
	    return;
	}		/* end of node type switch */
    }
}


/*{
** Name: opa_subsel_exists	- search for EXISTS subqueries
**
** Description:
**      Thus function searches a where clause parse tree fragment for a
**	correlated EXISTS subquery that it can flatten.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to parse tree fragment being examined
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	allow_antijoin	flag to allow transform to antijoin.
**	underor		bool indicating whether we are under a OR
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-Apr-2011 (kiria01) m1538
**	    Pulled out of opa_subsel_underor
*/
static void
opa_subsel_exists(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	bool		   *gotone,
	bool		   allow_antijoin,
	bool		   underor)

{
    PST_QNODE *inner = (*nodep)->pst_left;
    u_i4 state;
    OPV_GBMVARS varmap;

    if (!allow_antijoin ||
	inner == NULL ||
	inner->pst_sym.pst_type != PST_SUBSEL)
	return;

    /* In addition, if in AGG where clause do not flatten if a COUNT aggregate is
    ** present */
    if (root->pst_sym.pst_type == PST_AGHEAD &&
	    opa_subsel_agh_count_present(global, root))
	return;

    state = opa_cosubquery_full(global, outer_root, inner, 0, TRUE, &varmap);

    if (underor)
    {
	/* Must be correlated. */
	if ((state & (COVAR_X_SEEN|COVAR_SS_SEEN|COVAR_HAS_UNION|
			    COVAR_NONEQUIJOIN|COVAR_HAS_COUNT|
			COVAR_EQUIJOIN|COVAR_COREL)) ==
			(COVAR_EQUIJOIN|COVAR_COREL))
	{
	    PST_QNODE *wrkp, **wrkpp;
	    OPV_IGVARS	varno;
	    DB_STATUS	status;
	    DB_ERROR	error;

	    /* Do it - attach rhs of subquery to containing query, mark as
	    ** "oj" and replace "EXISTS' with appropriate "isnull" test. */
	    wrkp = inner->pst_right;

	    /* Find point of insertion (at tail of ANDs). */
	    for (wrkpp = &root->pst_right; (*wrkpp) != (PST_QNODE *) NULL &&
			(*wrkpp)->pst_sym.pst_type == PST_AND; 
			wrkpp = &(*wrkpp)->pst_right)
		/*SKIP*/;

	    (*wrkpp) = opa_makeop(global, PST_AND, (ADI_OP_ID) -1, wrkp, (*wrkpp));
	    (*nodep)->pst_sym.pst_value.pst_s_op.pst_opno = 
			    global->ops_cb->ops_server->opg_isnotnull;
	    varno = BTnext(-1, (char *)&inner->pst_sym.pst_value.pst_s_root.
		pst_tvrm, BITS_IN(inner->pst_sym.pst_value.pst_s_root.pst_tvrm));
	    (*nodep)->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					    /* stick TID var under isnull */
	    opa_gresolve(global, nodep);

	    opa_subsel_makeoj(global, header, wrkp, root,
		    outer_root, inner, &global->ops_goj.opl_subqjmap);
	    *gotone = TRUE;
	}
    }
    else /* not under OR */
    {
	/* To do this as a semi join we must be cleanly correlated (not under OR)
	** between inner and outer AND not including vars from outer-outers */
	/* USE_SEMIJOIN excludes all semi joins but those in update/delete
	** which require it to order rows in the presence of an IN subquery. */
	if ((global->ops_qheader->pst_mode == PSQ_DELETE ||
		global->ops_qheader->pst_mode == PSQ_REPLACE) &&
		(state & (COVAR_X_SEEN|COVAR_SS_SEEN|COVAR_HAS_UNION|
			    COVAR_NONEQUIJOIN|COVAR_HAS_COUNT|
			COVAR_EQUIJOIN|COVAR_COREL)) ==
			(COVAR_EQUIJOIN|COVAR_COREL))
	{ 
	    *nodep = inner->pst_right;
	    /* Mark as semi join */
	    opa_subsel_makeoj(global, header, *nodep, root,
		outer_root, inner, &global->ops_goj.opl_semijmap);
	    *gotone = TRUE;
	}
    }
}


/*{
** Name: opa_subsel_count_movepreds - move predicates from COUNT subquery
**	derived table to cointaining query WHERE
**
** Description:
**	This function searches the subquery WHERE clause for predicates that
**	only reference containing query tables and moves them to the 
**	containing query WHERE clause. Leaving them undermines the COUNT
**	transform.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodepp		ptr to ptr to derived table WHERE clause fragment
**	root		ptr to root node of outer select 
**	rootantimap	ptr to NOT of containing query var map
**	varmap		ptr to variable map of qeruy fragment being analyzed
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-apr-2011 (dougi) m1502, m1880
**	    Written to support count aggs in =/IN subqueries.
*/
static void
opa_subsel_count_movepreds(
	OPS_STATE	*global,
	PST_QNODE	**nodepp,
	PST_QNODE	*root,
	OPV_GBMVARS	*rootantimap,
	OPV_GBMVARS	*varmap)

{
    OPV_GBMVARS	local_map;
    PST_QNODE	*andp;

    if ((*nodepp) == (PST_QNODE *) NULL)
	return;

    switch ((*nodepp)->pst_sym.pst_type) {
      case PST_AND:
	/* Check left side for moving. */
	MEfill(sizeof(local_map), 0, (char *)&local_map);
	opa_subsel_count_movepreds(global, &(*nodepp)->pst_left, root, 
		rootantimap, &local_map);
	if (!BTallzeros((char *)&local_map, PST_NUMVARS))
	{
	    /* AND vars of fragment to NOT of root. If none are left
	    ** over, fragment only references root and should be moved. */
	    BTand(PST_NUMVARS, (char *)rootantimap, (char *)&local_map);
	    if (BTallzeros((char *)&local_map, PST_NUMVARS))
	    {
		/* Move the subtree under an AND in the root. */
		root->pst_right = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
		    (*nodepp)->pst_left, root->pst_right);
		(*nodepp)->pst_left = opv_qlend(global);
	    }
	}

	/* Then check right side for moving. */
	MEfill(sizeof(local_map), 0, (char *)&local_map);
	opa_subsel_count_movepreds(global, &(*nodepp)->pst_right, root, 
		rootantimap, &local_map);
	if (!BTallzeros((char *)&local_map, PST_NUMVARS))
	{
	    /* AND vars of fragment to NOT of root. If none are left
	    ** over, fragment only references root and should be moved. */
	    BTand(PST_NUMVARS, (char *)rootantimap, (char *)&local_map);
	    if (BTallzeros((char *)&local_map, PST_NUMVARS))
	    {
		/* Move the subtree under an AND in the root. */
		root->pst_right = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
		    (*nodepp)->pst_right, root->pst_right);
		(*nodepp)->pst_right = opv_qlend(global);
	    }

	}
	break;

      case PST_VAR:
	BTset((*nodepp)->pst_sym.pst_value.pst_s_var.pst_vno, varmap);
	/* drop to return */
      case PST_CONST:
	return;

      default:
	/* Recurse. */
	if ((*nodepp)->pst_left != (PST_QNODE *) NULL)
	    opa_subsel_count_movepreds(global, &(*nodepp)->pst_left, root, 
		rootantimap, varmap);
	if ((*nodepp)->pst_right != (PST_QNODE *) NULL)
	    opa_subsel_count_movepreds(global, &(*nodepp)->pst_right, root, 
		rootantimap, varmap);
    }	/* end of switch */

}


/*{
** Name: opa_subsel_countpreds - build grouping list for COUNT subquery and
**	massage WHERE clause
**
** Description:
**	This function searches the subquery WHERE clause for correlation
**	predicates and builds a RESDOM list from the columns. It then 
**	massages the predicates and moves them to the outer query ON clause.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	root		ptr to root node of outer select 
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-2011 (dougi) m1502, m1880
**	    Written to support count aggs in =/IN subqueries.
*/
static void
opa_subsel_countpreds(
	OPS_STATE	*global,
	PST_QNODE 	*root, 
	PST_QNODE	**rsdmp, 
	PST_QNODE	*aghp, 
	PST_QNODE	**sqrhs, 
	PST_J_ID	joinid,
	i4		rngno)

{
    PST_QNODE	*andp, *newrsdmp, *lvarp, *rvarp, *tptr;
    DB_ATT_ID	attid;
    bool	lo, ro;

    /* This function descends the parse tree fragment from the subquery
    ** WHERE clause - iterating on left, recursing on right - looking for 
    ** correlation predicates. The column from the subquery side of the
    ** correlation is added to the RESDOM list of grouping columns, the
    ** predicate is modifed to reference the column in the derived table
    ** and the predicate is moved to the outer query WHERE clause. */

    for ( ; ; )
    {
	switch((*sqrhs)->pst_sym.pst_type) {

	  case PST_AND:
	    if ((*sqrhs)->pst_right != (PST_QNODE *) NULL)
		opa_subsel_countpreds(global, root, rsdmp, aghp,
			&(*sqrhs)->pst_right, joinid, rngno);
	    if ((*sqrhs)->pst_left == (PST_QNODE *) NULL)
		return;

	    /* Set up LHS and iterate. */
	    sqrhs = &(*sqrhs)->pst_left;
	    break;

	  case PST_BOP:
	    /* Check for correlation predicate. To be in this location
	    ** (under an AND or the subquery root), this must be a comparison
	    ** predicate. So all we need to do is check if both operands
	    ** are PST_VARs, one from subquery, one from containing query. */
	    if ((lvarp = (*sqrhs)->pst_left)->pst_sym.pst_type != PST_VAR ||
		(rvarp = (*sqrhs)->pst_right)->pst_sym.pst_type != PST_VAR)
		return;				/* not both PST_VARs */

	    lo = BTtest((u_i2)lvarp->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    ro = BTtest((u_i2)rvarp->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);

	    if (lo && ro)
	    {
		/* Left and right columns are both from outer query. This
		** is weird, but legal and the predicate should be moved 
		** outside. */
		tptr = (*sqrhs);
	    }
	    else if (!lo && !ro)
		return;				/* both from sq - do nothing */
	    else
	    {
		/* This is a correlation predicate. Copy the inner PST_VAR
		** to the grouping list & fiddle the original to the new
		** pst_vno/pst_atno. */
		newrsdmp = opv_resdom(global, (lo) ? rvarp : lvarp, 
			(lo) ? &rvarp->pst_sym.pst_dataval : 
					&lvarp->pst_sym.pst_dataval);
		if ((*rsdmp)->pst_sym.pst_type == PST_RESDOM)
		    newrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno =
			(*rsdmp)->pst_sym.pst_value.pst_s_rsdm.pst_rsno+1;
		else newrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 1;
		newrsdmp->pst_left = (*rsdmp);
		(*rsdmp) = newrsdmp;

		attid.db_att_id = newrsdmp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsno;
		tptr = opv_varnode(global, &newrsdmp->pst_sym.pst_dataval,
			rngno, &attid);
		if (lo)
		    (*sqrhs)->pst_right = tptr;
		else (*sqrhs)->pst_left = tptr;
		tptr = (*sqrhs);
		(*sqrhs) = opv_qlend(global);		/* remove BOP */
	    }

	    /* The PST_BOP is now moved into the outer query ON clause. */
	    andp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1, tptr,
			root->pst_right);
	    root->pst_right = andp;
	    tptr->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
	    return;

	  default:
	    /* All the rest just return. */
	    return;
	}

    }

}


/*{
** Name: opa_subsel_countsq - flatten =/IN subquery with COUNT in select list
**
** Description:
**      Thus function confirms the suitability of a subquery comparison
**	(=/IN subquery) in which there is a COUNT aggregate in the SELECT list.
**	If it is a qualifying subquery, it is flattened into an outer join
**	that preserves the COUNT 0 case for the transformed query.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to parse tree fragment being examined
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-2011 (dougi) m1502, m1880
**	    Written to support count aggs in =/IN subqueries.
*/
static void
opa_subsel_countsq(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	bool		   *gotone)

{
    PST_QNODE	*aghp, *countp, *andp, *varp, *rsdmp, *byhp, *newroot, *sqrhs;
    PST_RNGENTRY **tempptr;
    PST_J_ID	joinid = (*nodep)->pst_right->pst_sym.pst_value.pst_s_root.
					pst_ss_joinid;
    OPV_GBMVARS varmap, rootantimap;
    i4		rngno, rngno1;
    u_i4	state;
    DB_ATT_ID	attid;

    /* This transform effectively changes:
    ** "select * from t1 where c2 in 
    **	    (select count(*) from t2 where t1.c1 = t2.c1)" to:
    ** "select t1.* from t1 left join (select c1, count(*) x from t2 
    **	    group by c1) y on t1.c1 = y.c1 where t1.c2 = ifnull(x, 0)"
    ** This flattens the subquery into a much more efficient join while
    ** retaining the semantics of count which would produce a potentially
    ** matching row from the subquery with a count of 0, whereas a more
    ** basic flattening would skip those rows. */
    aghp = (*nodep)->pst_right->pst_left->pst_right;	/* AGHEAD under sq */
    sqrhs = aghp->pst_right;				/* sq WHERE clause */
    if (sqrhs == (PST_QNODE *) NULL || sqrhs->pst_sym.pst_type == PST_QLEND)
	return;						/* uncorrelated */

    state = opa_cosubquery_full(global, outer_root, (*nodep)->pst_right, 
			0, FALSE, &varmap);

    /* Verify the subquery is one we can flatten. */
    if (!(state & (COVAR_X_SEEN|COVAR_HAS_UNION|
			    COVAR_NONEQUIJOIN|
			COVAR_EQUIJOIN|COVAR_COREL)))
	return;

    rsdmp = opv_qlend(global);
    rsdmp->pst_sym.pst_type = PST_TREE;			/* init RESDOMs */

    /* Copy existing range table and allocate/init entry for derived table
    ** introduced to compute count. */
    rngno = global->ops_qheader->pst_rngvar_count;
    if (rngno + 1 >= PST_NUMVARS)
        opx_error(E_OP0202_VAR_OVERFLOW);   /* report error if no parser range
                                            ** variables are available */
    tempptr = (PST_RNGENTRY **)opu_memory(global, (i4)((rngno + 1)* 
						sizeof(PST_RNGENTRY *)));
    MEcopy((PTR) global->ops_qheader->pst_rangetab, 
            (rngno * sizeof(PST_RNGENTRY *)), (PTR)tempptr);
    global->ops_qheader->pst_rangetab = tempptr; 
    global->ops_qheader->pst_rngvar_count++;
    global->ops_qheader->pst_rangetab[rngno] =
        (PST_RNGENTRY *)opu_memory(global, (i4)sizeof(PST_RNGENTRY));
    MEfill(sizeof(PST_RNGENTRY), 0, 
	(char *)global->ops_qheader->pst_rangetab[rngno]);
    global->ops_qheader->pst_rangetab[rngno]->pst_rgtype = PST_DRTREE;

    /* Set new OJ id in range table. */
    for (rngno1 = -1; (rngno1 = BTnext(rngno1, (char *)&root->pst_sym.
	    pst_value.pst_s_root.pst_tvrm, BITS_IN(PST_J_MASK))) >= 0; )
	BTset((i4)joinid, (char *)&header->pst_rangetab[rngno1]->pst_outer_rel);
    BTset((i4)joinid, (char *)&header->pst_rangetab[rngno]->pst_inner_rel);

    BTset(rngno, (char *)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
						/* flag ref to derived table */
    outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc++;

    global->ops_qheader->pst_rangetab[rngno]->pst_rgroot = newroot =
	(*nodep)->pst_right;
    newroot->pst_sym.pst_type = PST_ROOT;	/* change node type */
    byhp = aghp->pst_left = opv_resdom(global, aghp->pst_left, 
		&aghp->pst_sym.pst_dataval);
    byhp->pst_sym.pst_type = PST_BYHEAD;
    newroot->pst_left = opv_resdom(global, aghp, &aghp->pst_sym.pst_dataval);

    /* Replace subquery with ifnull() in original query and tidy BOP. */
    attid.db_att_id = -1;
    (*nodep)->pst_right = opa_makeop(global, PST_BOP, ADI_IFNUL_OP,
	opv_varnode(global, &aghp->pst_sym.pst_dataval, rngno, &attid),
	opv_i1const(global, 0));
    varp = (*nodep)->pst_right->pst_left;	/* save count col to update */
    (*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
    
    /* Build list of grouping columns for derived table and massage WHERE
    ** clauses at the same time, then hook it all together. */
    opa_subsel_countpreds(global, root, &rsdmp, aghp, &aghp->pst_right,
		joinid, rngno);
    varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
		newroot->pst_left->pst_sym.pst_value.pst_s_rsdm.pst_rsno =
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno+1;
    byhp->pst_left = rsdmp;

    /* Make a copy of the grouping column RESDOM list
    ** and hang it from the 1st RESDOM of the derived table. */
    opv_copytree(global, &rsdmp);
    newroot->pst_left->pst_left = rsdmp;

    /* It is possible that the subquery has predicates that only reference
    ** outer query tables - and they mess up the transform. Look for those
    ** and move them to the outer query. */
    MEcopy((char *)&root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	sizeof(rootantimap), (char *)&rootantimap);
    BTnot(PST_NUMVARS, (char *)&rootantimap);
    opa_subsel_count_movepreds(global, &newroot->pst_left->pst_right->pst_right,
		root, &rootantimap, (OPV_GBMVARS *) NULL);
    (*gotone) = TRUE;
}


/*{
** Name: opa_subsel_eq_any - search for a=ANY(SELECT ..." or "a IN(SELECT ..."
**
** Description:
**      Thus function searches a where clause parse tree fragment for a
**	correlated IN/"= ANY" subquery that it can flatten. We do this in such a
**	manner as to annotate the OJ structures to reflect a semi join or if under
**	an OR, we get a sub-query join.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to parse tree fragment being examined
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	allow_antijoin	flag to allow transform to antijoin.
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-Apr-2011 (kiria01) m1538
**	    Pulled out of opa_subsel_underor
*/
static void
opa_subsel_eq_any(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	bool		   *gotone,
	bool		   allow_antijoin,
	bool		   underor)

{
    PST_QNODE *inner = (*nodep)->pst_right;
    u_i4 state;
    OPV_GBMVARS varmap;

    if (!allow_antijoin ||
	    inner == NULL ||
	    inner->pst_sym.pst_type != PST_SUBSEL)
	return;

    /* Check for correlation. */
    state = opa_cosubquery_full(global, outer_root, *nodep, 0, TRUE, &varmap);

    /* If the SS result is a single value aggregate then drop the ANY */
    if (state & (COVAR_SIMPAGG_IN_TL))
    {
	(*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_CARD_01R;
	*gotone = TRUE;
    }
    else if (underor)
    {
	/* Check correlation (includes link). The link might not be a genuine
	** correlation e.g. 'p1' = ANY(SELECT ...) */
	if ((state & (COVAR_X_SEEN|COVAR_SS_SEEN|COVAR_HAS_UNION|
			COVAR_COREL)) ==
			(COVAR_COREL))
	{
	    PST_QNODE *lnodep = (*nodep)->pst_left;
	    PST_QNODE *wrkp, **wrkpp;
	    OPV_IGVARS	varno;
	    DB_STATUS	status;
	    DB_ERROR	error;
	    /* Do it - attach correlation "x = y" predicate to rhs of subquery,
	    ** attach "new" rhs of subquery to rhs of containing query, nark as
	    ** "oj" and replace "x = (SELECT" with appropriate "isnull" test. */
	    wrkp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1,
		    opa_makeop(global, PST_BOP, global->ops_cb->ops_server->opg_eq,
			lnodep, inner->pst_left->pst_right), inner->pst_right);
	    opa_gresolve(global, &wrkp->pst_left);

	    /* Find point of insertion (at tail of ANDs). */
	    for (wrkpp = &root->pst_right; (*wrkpp) != (PST_QNODE *) NULL &&
		(*wrkpp)->pst_sym.pst_type == PST_AND; 
		wrkpp = &(*wrkpp)->pst_right);

	    (*wrkpp) = opa_makeop(global, PST_AND, (ADI_OP_ID) -1, wrkp, (*wrkpp));
        	
	    (*nodep)->pst_sym.pst_type = PST_UOP;		/* change BOP to UOP */
	    (*nodep)->pst_sym.pst_value.pst_s_op.pst_opno = 
			    global->ops_cb->ops_server->opg_isnotnull;
	    varno = BTnext(-1, (char *)&inner->pst_sym.pst_value.pst_s_root.
		pst_tvrm, BITS_IN(inner->pst_sym.pst_value.pst_s_root.pst_tvrm));
	    (*nodep)->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					    /* stick TID var under isnull */
	    (*nodep)->pst_right = (PST_QNODE *) NULL;
	    opa_gresolve(global, nodep);

	    /* Note the SubqJoin map! */
	    opa_subsel_makeoj(global, header, wrkp, root,
		    outer_root, inner, &global->ops_goj.opl_subqjmap);
	    *gotone = TRUE;
	}
    }
    else /* not under OR */
    {
	/* To do this as a semi join we must be either correlated
	** inner to outer OR uncorrelated but with a non-constant link operator.
	** So there should certainly be no externals present but anything else goes. */
	/* USE_SEMIJOIN excludes all semi joins but those in update/delete
	** which require it to order rows in the presence of an IN subquery. */
	if ((global->ops_qheader->pst_mode == PSQ_DELETE ||
		global->ops_qheader->pst_mode == PSQ_REPLACE) &&
		(state & (COVAR_X_SEEN|COVAR_SS_SEEN|COVAR_HAS_UNION|
			COVAR_NONEQUIJOIN|COVAR_EQUIJOIN|COVAR_COREL)) ==
			(COVAR_EQUIJOIN|COVAR_COREL))
	{
	    PST_QNODE **wrkpp;
    	
	    /* Change the expr = ANY(SELECT y ...)" to expr = y predicate in situ
	    ** not forgetting to clear opmeta field and setting the join ID to that of the
	    ** subselect we're removing. */
	    (*nodep)->pst_right = inner->pst_left->pst_right;
	    (*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
		    opa_subsel_joinid(global, *nodep, inner->pst_sym.pst_value.pst_s_root.pst_ss_joinid);

	    /* Attach WHERE clause of subquery to RHS of containing query,
	    ** Find point of insertion (at tail of ANDs). */
	    for (wrkpp = &root->pst_right; (*wrkpp) != (PST_QNODE *) NULL &&
		(*wrkpp)->pst_sym.pst_type == PST_AND; 
		wrkpp = &(*wrkpp)->pst_right);

	    *wrkpp = opa_makeop(global, PST_AND, (ADI_OP_ID) -1, *wrkpp, inner->pst_right);
        	
	    /* Finally mark as a SemiJoin. */
	    opa_subsel_makeoj(global, header, (*wrkpp)->pst_right, root,
		    outer_root, inner, &global->ops_goj.opl_semijmap);
	    *gotone = TRUE;
	}
    }
}


/*{
** Name: opa_subsel_search	- search for "not exists" subselect
**
** Description:
**      Thus function searches a where clause parse tree fragment for 
**	a "not exists" subselect.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to parse tree fragment being examined
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT,
**			PST_SUBSEL or PST_AGHEAD)
**	allow_antijoin	flag to allow transform to antijoin.
**	underor		bool indicating whether we are under a OR
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-aug-99 (inkdo01)
**	    Written.
**	18-jan-05 (inkdo01)
**	    Exclude "not in"/"<> ALL" subselects with nullable subselect 
**	    comparand. OJ doesn't handle nulls correctly. This could be 
**	    handled by including an OR'ed is null test to ON clause.
**	13-may-05 (inkdo01)
**	    Fix some sloppy keying of previous change.
**	16-may-05 (inkdo01)
**	    Changes to allow not exists transform under PST_AGHEAD.
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**      13-apr-07 (huazh01)
**          after transforming nodes under PST_AND nodes, further search
**          the updated tree and see if there are more nodes that are
**          suitable to be transformed into OJ. This fixes b117988.  
**	13-mar-08 (hayke02)
**	    Extend the 18-jan-05 (inkdo01) fix for bug 113772 to the other
**	    (left) side of the "not in"/"<> ALL" subselect. This change
**	    fixes bug 120034.
**	6-nov-2009 (dougi)
**	    Attempt to recursively descend through nested subselects - flatten
**	    on the way back out.
**	10-dec-2009 (dougi)
**	    Add code to apply deMorgan now, rather than waiting for opagen.
**	    Delayed deMorgan interferes with some flattening done here.
**	18-feb-2011 (dougi)
**	    Reset root for PST_AGHEADs, too.
**	16-mar-2011 (dougi)
**	    Don't lose rhs of an op for recursion.
*/
static bool
opa_subsel_search(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	bool		   *gotone,
	bool		   allow_antijoin,
	bool		   underor,
	bool		   undernotex_in)

{
    PST_QNODE	*nodep1, **rnodepp;
    ADI_OP_ID	opno;
    bool        local_gotone = FALSE;
    bool	local_underor = underor;
    bool	local_undernot = undernotex_in;
    bool	rslt = TRUE;

    if ((*nodep) == (PST_QNODE *) NULL)
	return(TRUE);

    /* Recurse through where clause fragment and on the way back out, 
    ** search for one of the magic sequences of PST_NOT->PST_UOP (exists)->
    ** PST_SUBSEL (the NOT EXISTS case), PST_NOT->PST_BOP (=)->PST_SUBSEL 
    ** (the NOT = ANY case) or PST_BOP (<>)->PST_SUBSEL (the <> ALL case - 
    ** same as NOT = ANY). For X100 queries (in fact, when allow_antijoin
    ** is set), other quantified queries are also transformed. ORs screw
    ** things up, so encountering ORs or SUBSELs will reset various things
    ** so we only flatten a subselect into a conjunct. */

    /* First do a pre-deMorgan. */
    if ((*nodep)->pst_sym.pst_type == PST_NOT)
	opa_demorgan(global, nodep, &(*nodep)->pst_left);

    /* We currently disallow NOT EXISTS nested under another NOT EXISTS.
    ** We are capabale, however, of handling such nesting as long as each
    ** subquery correlates to the immediately containing  query. However,
    ** that exception is not yet implemented. */
    if ((*nodep)->pst_sym.pst_type == PST_UOP &&
	(*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_nexists)
    {
	OPV_GBMVARS varmap;
	if (local_undernot)
	{
	    u_i4 state = opa_cosubquery_full(global, root, (*nodep)->pst_left, 0,
				TRUE, &varmap);
	    if (state & COVAR_X_SEEN)
		return(FALSE);
	}
     	local_undernot = TRUE;
    }

    /* This heads off flattening "=" subqueries here if there's no benefit. */
    if ((*nodep)->pst_sym.pst_type == PST_BOP &&
	((*nodep)->pst_right->pst_sym.pst_type == PST_SUBSEL ||
	(*nodep)->pst_left->pst_sym.pst_type == PST_SUBSEL) &&
	((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno !=
			global->ops_cb->ops_server->opg_eq ||
	(*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_CARD_ANY))
    {
	local_undernot = TRUE;
    }

    if ((*nodep)->pst_sym.pst_type == PST_OR)
	local_underor = TRUE;

    if ((*nodep)->pst_sym.pst_type == PST_SUBSEL)
    {
	/* reset with ROOT/SUBSEL as root */
	rslt = opa_subsel_search(global, &(*nodep)->pst_left, header, *nodep,
		*nodep, &local_gotone, allow_antijoin, FALSE, local_undernot);
	if (!rslt)
	    return rslt;
	rslt = opa_subsel_search(global, &(*nodep)->pst_right, header, *nodep,
		*nodep, &local_gotone, allow_antijoin, FALSE, local_undernot);
    }
    else if ((*nodep)->pst_sym.pst_type == PST_AGHEAD)
	rslt = opa_subsel_search(global, &(*nodep)->pst_right, header, root,
		*nodep, &local_gotone, allow_antijoin, FALSE, local_undernot);
					/* reset with AGHEAD as root */
    else if ((*nodep)->pst_sym.pst_type == PST_CONST)
	/* Avoid any potential INLIST chains */
	return TRUE;

    else		/* Keep chugging with current settings */
    {
	/* Things can get swapped around (especially for NOT EXISTS transforms)
	** so we keep track of what's on the right side for the 2nd call. */
	if ((*nodep)->pst_right != NULL)
	    rnodepp = &(*nodep)->pst_right;
	else
	    rnodepp = NULL;

	if ((*nodep)->pst_left != NULL)
	    rslt = opa_subsel_search(global, &(*nodep)->pst_left, header, outer_root, 
		root, &local_gotone, allow_antijoin, local_underor,
		local_undernot);
	if (!rslt)
	    return(rslt);

	if (rnodepp != NULL)
	    rslt = opa_subsel_search(global, rnodepp, header, outer_root, 
		root, &local_gotone, allow_antijoin, local_underor,
		local_undernot);
    }

    if ((underor && !allow_antijoin) || !rslt)
	return(rslt);			/* don't analyze if we're a disjunct 
					** and NOT X100 */

    switch ((*nodep)->pst_sym.pst_type) {
      case PST_UOP:
	/* Look for magic sequence. [NOT EXISTS(SUBSEL ...)] */
	if (((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_nexists ||
		(*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_exists &&
		allow_antijoin && undernotex_in) &&
	    ((*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN ||
		allow_antijoin) &&
	    (nodep1 = (*nodep)->pst_left) &&
	    nodep1->pst_sym.pst_type == PST_SUBSEL &&
	    nodep1->pst_sym.pst_value.pst_s_root.pst_union.pst_next == NULL)
	{
	    /* Ta, da! Now call analyzer to determine if this one can
	    ** be transformed, and do it. */
	    opa_notex_transform(global, nodep, header, outer_root, 
		root, nodep1, gotone, allow_antijoin, underor);
	}
	else if ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_exists)
	    opa_subsel_exists(global, nodep, header, outer_root, 
		    root, gotone, allow_antijoin, underor);

	if (*gotone && root->pst_sym.pst_type == PST_AGHEAD)
	    opa_subsel_agh_common(root);
	return(TRUE);

      case PST_BOP:
	if (allow_antijoin 
	    && 
	    (*nodep)->pst_right->pst_sym.pst_type == PST_SUBSEL
	    &&
	    (*nodep)->pst_right->pst_left->pst_sym.pst_type == PST_RESDOM
	    &&
	    (*nodep)->pst_right->pst_left->pst_right->pst_sym.pst_type == 
			PST_AGHEAD
	    &&
	    (*nodep)->pst_right->pst_left->pst_right->pst_left->
			pst_sym.pst_type == PST_AOP
	    &&
	    ((opno = (*nodep)->pst_right->pst_left->pst_right->pst_left->
			pst_sym.pst_value.pst_s_op.pst_opno) ==
		global->ops_cb->ops_server->opg_count ||
		opno == global->ops_cb->ops_server->opg_scount))
	    opa_subsel_countsq(global, nodep, header, outer_root, root, gotone);
	else if ((((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
	    global->ops_cb->ops_server->opg_ne
	    &&
	    (*nodep)->pst_right->pst_sym.pst_dataval.db_datatype > 0
	    &&
	    (*nodep)->pst_left->pst_sym.pst_dataval.db_datatype > 0)
			|| allow_antijoin)
	    &&
	    ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ALL
	    ||
	    (*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ANY
	    &&
	    ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno !=
			global->ops_cb->ops_server->opg_eq || undernotex_in) &&
	    allow_antijoin)
	    &&
	    ((*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN
			|| allow_antijoin)
	    &&
	    (*nodep)->pst_right->pst_sym.pst_type == PST_SUBSEL 
	    &&
	    (*nodep)->pst_right->pst_sym.pst_value.pst_s_root.
						pst_union.pst_next == NULL)
	{
	    /* This one is "... c1 NOT IN (select ..." which is the same as
	    ** "... c1 <> ALL (select ..." or if antijoin transforms are
	    ** allowed, "... c1 <any compop> <any qop> (select ...". */
	    opa_notin_transform(global, nodep, header, outer_root, 
		root, (*nodep), gotone, allow_antijoin, underor);
	}
	else if ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_eq &&
		(*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ANY)
	    opa_subsel_eq_any(global, nodep, header, outer_root, 
		    root, gotone, allow_antijoin, underor);

	if (*gotone && root->pst_sym.pst_type == PST_AGHEAD)
	    opa_subsel_agh_common(root);
	return(TRUE);

    }    /* end of switch */

    return(TRUE);
}


/*{
** Name: opa_subsel_to_oj	- perform NOT EXISTS/NOT IN subselect
**	to oj transformations
**
** Description:
**      Thus function examines the current query's parse tree for 
**	qualifying NOT EXISTS, NOT IN and <> ALL subselects, and oversees
**	their transformation to equivalent outer joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**
** Outputs:
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-aug-99 (inkdo01)
**	    Written.
**	16-may-05 (inkdo01)
**	    Changes to support transform even under AGHEADs.
**	16-mar-06 (dougi)
**	    Aggregate queries with multiple agg funcs in the SELECT list
**	    generate a WHERE clause per agg func and that confuses the 
**	    transform. Only increment the OJ joinid once.
**	27-oct-2008 (dougi)
**	    Also search derived table parse trees for flattening potential.
**	24-Feb-2011 (kiria01)
**	    Include WITH elements & views along with the derived tables for
**	    flattening potential.
*/
VOID
opa_subsel_to_oj(
	OPS_STATE	   *global,
	bool		   *gotone)

{
    PST_QTREE	*header = global->ops_qheader;
    PST_QNODE	*outer_root, **nodep;
    i4		i;
    bool	local_gotone;


    /* This function simply loops over the union'ed selects in the query's
    ** parse tree, calling the transformation functions for each. */
    *gotone = FALSE;			/* init */

    opa_subsel_agh_lace(global, header->pst_qtree, TRUE);
    do {		/* while (local_gotone) */
	for (nodep = &header->pst_qtree; 
	    (outer_root = *nodep) && outer_root->pst_sym.pst_type == PST_ROOT;
	    nodep = &outer_root->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    local_gotone = FALSE;

	    opa_subsel_search(global, nodep, header, 
			outer_root, outer_root, &local_gotone,
			(global->ops_gmask & OPS_X100)!=0, FALSE, FALSE);

	    if (local_gotone)
		*gotone = TRUE;
	}
    } while (local_gotone);

    opa_subsel_agh_lace(global, header->pst_qtree, FALSE);
    /* Now look for derived tables that can be transformed. */
    for (i = 0; i < header->pst_rngvar_count; i++)
    {
	PST_RNGENTRY *rng = header->pst_rangetab[i];
	if (rng && (
		rng->pst_rgtype == PST_RTREE ||
		rng->pst_rgtype == PST_DRTREE ||
		rng->pst_rgtype == PST_WETREE))
	{
	    for (nodep = &rng->pst_rgroot;
		    (outer_root = *nodep) && outer_root->pst_sym.pst_type == PST_ROOT;
		    nodep = &outer_root->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	    {
		opa_subsel_agh_lace(global, outer_root->pst_right, TRUE);
		opa_subsel_search(global, nodep, header, 
			outer_root, outer_root, gotone,
			(global->ops_gmask & OPS_X100)!=0, FALSE, FALSE);
		opa_subsel_agh_lace(global, outer_root->pst_right, FALSE);
	    }
	}
    }
}

/*{
** Name: opa_cosubquery_full - check for corelation in subquery
**
** Description:
**      This routine is an enhanced version of opa_cosubquery. It is intended
**	to be able to be more fussy about the form of correlation present. It
**	is able to distinquish simple restrictions from correlations and other
**	useful facts like the presence of disjuncts, unflattened sub-queries
**	or presence of COUNT aggregates.
**    
**
** Inputs:
**      global		ptr to global state variable OPS_STATE
**	outer		Outer root node
**	inner		Inner root node
**	break_state	State bits that if set can curtail scan
**	blocking_or	Boolean to contol whether we allow for correlation
**			beneath OR nodes.
**
** Outputs:
**      varmap		ptr to from list map which currently
**			describes the variable scope
**
**      Returns:	completion state bit map with bit values:
**	    COVAR_I_SEEN	An Inner VAR referenced
**	    COVAR_O_SEEN	An Outer VAR referenced
**	    COVAR_X_SEEN	An eXternal VAR referenced
**	    COVAR_SS_SEEN	An unflattened SS found
**	    COVAR_RESTRICT	Restriction on Inner found
**	    COVAR_COREL		f(I) OP g(O) or f(O) OP g(I)
**	    COVAR_EQUIJOIN	COVAR_COREL & has EQUI
**	    COVAR_NONEQUIJOIN	COVAR_COREL & has NONEQUI
**	    COVAR_RESTRICTO	Restriction on Outer found
**	    COVAR_CORELO	f(O) OP g(X) or f(X) OP g(O)
**	    COVAR_EQUIJOINO	COVAR_CORELO & has EQUI
**	    COVAR_NONEQUIJOINO	COVAR_CORELO & has NONEQUI
**	    COVAR_RESTRICTX	Restriction on eXternal found
**	    COVAR_CORELX	f(I) OP g(X) or f(X) OP g(I)
**	    COVAR_EQUIJOINX	COVAR_CORELX & has EQUI
**	    COVAR_NONEQUIJOINX	COVAR_CORELX & has NONEQUI
**	    COVAR_COREL_MIX	f(I,O) OP g(O,I)
**	    COVAR_HAS_COUNT	COUNT seen
**	    COVAR_HAS_UNION	SS has a union
**	    COVAR_MESS		Correlation is complex
**	    COVAR_SIMPAGG_IN_TL	Agg in target list (ungrouped)
**	    COVAR_CONST_TL	Target list has no VAR nodes
**	    COVAR_OR_ACTIVE	A correlation was masked by OR
**				and blocking_or requested
**      Exceptions:
**	    none
** Side Effects:
**
** History:
**      24-Mar-2011 (kiria01)
**          Created for finer control of sub-query investigation.
*/
static u_i4
opa_cosubquery_full(
	OPS_STATE	*global,
	PST_QNODE	*outer,
	PST_QNODE	*inner,
	u_i4		break_state,
	bool		blocking_or,
	OPV_GBMVARS	*varmap)
{
    PST_QNODE **nodep, *subsel = outer, *psubsel = NULL, bop, *bopp = NULL;
    i4 depth = 0;
    bool descend = TRUE;
    bool correl = FALSE;
    bool underor = FALSE;
    bool ssunderor = FALSE;
    OPV_STK stk;
    u_i4 state[MAX_NESTING];

    state[0] = 0;
    OPV_STK_INIT(stk, global);

    memset(varmap, 0, sizeof(OPV_GBMVARS));

    if (inner->pst_sym.pst_type == PST_BOP)
    {
	/* If passed the BOP node above a SUBSEL, add a dummy BOP node into
	** the mix to augment the correlations which makes this test look like
	** how it look after the subselect were merged in. */
	if (inner->pst_right->pst_sym.pst_type != PST_SUBSEL)
	    /* We have a problem ... */
	    opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
			"opmeta conflict jt");
	bop.pst_left = inner->pst_left;
	bop.pst_right = inner->pst_right->pst_left->pst_right;
	bop.pst_sym.pst_type = PST_BOP;
	bop.pst_sym.pst_dataval = inner->pst_sym.pst_dataval;
	bop.pst_sym.pst_len = inner->pst_sym.pst_len;
	bop.pst_sym.pst_value.pst_s_op = inner->pst_sym.pst_value.pst_s_op;
	bopp = &bop;

	inner = inner->pst_right;
    }
    nodep = &inner;

    if (nodep && *nodep)
    while (nodep && (state[depth] & break_state) == 0)
    {
	register PST_QNODE *p, *node = *nodep;
	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && (node->pst_left || node->pst_right))
	{
	    /* Increment depth counters for children */
	    switch (node->pst_sym.pst_type)
	    {
	    case PST_SUBSEL:
	    case PST_ROOT:
		psubsel = subsel;
		subsel = node;
		correl = FALSE;
		depth++;
		state[depth] = 0;
		if (depth != 0)
		    /* If there are states that should follow from
		    ** outer scope, mask them in now. (None at present)*/
		    state[depth] = state[depth-1] & (0);
		if (depth > 1)
		    state[0] |= COVAR_SS_SEEN;
		/* Include any unions into the mix */
		if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    state[depth] |= COVAR_HAS_UNION;
		    opv_push_nodep(&stk,
			&node->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    /* Mark that the top node needs descending */
		    opv_push_nodep(&stk, OPV_DESCEND_MARK);
		}
		/* Delay node evaluation */
		opv_push_nodep(&stk, nodep);
		if (depth == 1 && bopp)
		{
		    /* Add in the fixed up BOP if any as it too may have a bearing
		    ** on correlation but we place this now to be processed while
		    ** SUBSEL (just put on stack) active & after its WHERE list. */
		    opv_push_nodep(&stk, &bopp);
		    /* Mark that the top node needs descending */
		    opv_push_nodep(&stk, OPV_DESCEND_MARK);
		}
		break;
	    case PST_WHLIST:
	    case PST_OR:
		state[depth] |= COVAR_UNDEROR;
		/*FALLTHROUGH*/
	    default:
		/* Delay node evaluation */
		opv_push_nodep(&stk, nodep);
		break;
	    }
	    if (node->pst_left)
	    {
		if (node->pst_right)
		{
		    /* Delay RHS */
		    opv_push_nodep(&stk, &node->pst_right);
		    if ((node->pst_right->pst_right || node->pst_right->pst_left) &&
			node->pst_right->pst_sym.pst_type != PST_VAR)
			/* Mark that the top node needs descending */
			opv_push_nodep(&stk, OPV_DESCEND_MARK);
		}
		nodep = &node->pst_left;
		continue;
	    }
	    else if (node->pst_right)
	    {
		nodep = &node->pst_right;
		continue;
	    }
	}

	/*** Each node will be passed here once, in sequence, bottom-up, left to right ***/
	switch (node->pst_sym.pst_type)
	{
	case PST_WHLIST:
	case PST_OR:
	    /* Need to see if still under a disjunct */
	    p = opv_antecedant_by_4types(&stk, NULL,
				PST_OR, PST_WHLIST, PST_SUBSEL, PST_AGHEAD);
	    if (!p || p->pst_sym.pst_type != PST_OR &&
				p->pst_sym.pst_type != PST_WHLIST)
	    {
		state[depth] &= ~COVAR_UNDEROR;
	    }
	    break;
        case PST_VAR:
            BTset(node->pst_sym.pst_value.pst_s_var.pst_vno, varmap);
	    break;
	case PST_AOP:
	    if (node->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_count ||
		 node->pst_sym.pst_value.pst_s_op.pst_opno ==
		    global->ops_cb->ops_server->opg_scount)
		state[depth] |= COVAR_HAS_COUNT;
	    break;
	case PST_BOP:
	    {
		PST_J_MASK *msk, lvars, rvars, vars, from;
		PST_J_MASK *i_tvrm = &subsel->pst_sym.pst_value.pst_s_root.pst_tvrm;
		PST_J_MASK *o_tvrm = &psubsel->pst_sym.pst_value.pst_s_root.pst_tvrm;
		OPV_STK vstk;
		bool varseen = FALSE;
		/* If operator is not a comparison there will be nothing to do so
		** break out if that is the case. */
		if (!node->pst_sym.pst_value.pst_s_op.pst_fdesc)
		{
		    ADI_OPINFO opinfo;
		    if (adi_op_info(global->ops_adfcb,
					node->pst_sym.pst_value.pst_s_op.pst_opno, &opinfo))
			break;
		    if (opinfo.adi_optype != ADI_COMPARISON)
			break;
		}
		else if (node->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype != ADI_COMPARISON)
		    break;

		/* We have a comparison - setup var maps for left and right sides
		** by doing a simple inline top-down scan of the sub-trees */
		OPV_STK_INIT(vstk, global);
		for (p = node->pst_left, msk = &lvars; ; p = node->pst_right, msk = &rvars)
		{
		    memset(msk, 0, sizeof(*msk));
		    while (p)
		    {
			switch (p->pst_sym.pst_type)
			{
			default:
			    if (p->pst_left)
			    {
				if (p->pst_right)
				    opv_push_node(&vstk, p->pst_right);
				p = p->pst_left;
				continue;
			    }
			    else if (p = p->pst_right)
				continue;
			    break;
			case PST_VAR:
			    BTset(p->pst_sym.pst_value.pst_s_var.pst_vno, msk);
			    varseen = TRUE;
			    break;
			case PST_CONST:
			case PST_AGHEAD:
			    /* Don't descend inlist constants or agheads */
			    break;
			case PST_ROOT:
			case PST_SUBSEL:
			    /* Descend target list and mark that unflattened SS found.
			    ** As the order of traversal of nodes is usually to action
			    ** from the bottom up, seeing these will mean something
			    ** not flattened. */
			    state[depth] |= COVAR_SS_SEEN;
			    break;
			}
			p = opv_pop_node(&vstk);
		    }
		    if (msk == &rvars)
			/* Both sub-trees done */
			break;
		}
		if (!varseen)
		    break;

		BTor3(BITS_IN(PST_J_MASK), &lvars, &rvars, &vars);
		
		if (BTintersects(i_tvrm, &vars, BITS_IN(PST_J_MASK)))
		{
		    state[depth] |= COVAR_I_SEEN;
		    if (BTsubset(&vars, i_tvrm, BITS_IN(PST_J_MASK)))
		    {
			/* Wholly inner restriction - not a join */
			state[depth] |= COVAR_RESTRICT;
			break;
		    }
		}
		if (BTintersects(o_tvrm, &vars, BITS_IN(PST_J_MASK)))
		{
		    state[depth] |= COVAR_O_SEEN;
		    if (BTsubset(&vars, o_tvrm, BITS_IN(PST_J_MASK)))
		    {
			/* Wholly outer restriction - not a join */
			state[depth] |= COVAR_RESTRICTO;
			break;
		    }
		}

		if (blocking_or && (state[depth]&COVAR_UNDEROR))
		{
		    state[0] |= COVAR_OR_ACTIVE;
		}
		/* Given that we now have the var maps determined for this
		** comparison operator - determine the correlation facts.
		** Try the usual: o=i OR i=o */
		else if ((state[depth] & (COVAR_I_SEEN|COVAR_O_SEEN)) ==
			    (COVAR_I_SEEN|COVAR_O_SEEN) && (
			BTsubset(&lvars, o_tvrm, BITS_IN(PST_J_MASK)) &&
			BTsubset(&rvars, i_tvrm, BITS_IN(PST_J_MASK)) ||
			BTsubset(&lvars, i_tvrm, BITS_IN(PST_J_MASK)) &&
			BTsubset(&rvars, o_tvrm, BITS_IN(PST_J_MASK))))
		{
		    /* Clean correlation */
		    if (node->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_eq)
			state[depth] |= COVAR_EQUIJOIN;
		    else
			state[depth] |= COVAR_NONEQUIJOIN;
		    state[depth] |= (COVAR_COREL|COVAR_I_SEEN|COVAR_O_SEEN);
		    break;
		}

		BTor3(BITS_IN(PST_J_MASK), o_tvrm, i_tvrm, &from);
		if (BTsubset(&vars, &from, BITS_IN(PST_J_MASK)))
		{
		    if (node->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_eq)
			state[depth] |= COVAR_EQUIJOIN;
		    else
			state[depth] |= COVAR_NONEQUIJOIN;
		    /* The comparison is mixed so the sense indeterminate but
		    ** we can determine the join direction. We have found that
		    ** all the vnos are present in the from list of the current
		    ** or its parent */
		    if (!blocking_or || (~state[depth]&COVAR_UNDEROR))
			state[depth] |= (COVAR_COREL|COVAR_COREL_MIX);
		    break;
		}
		/* At least 1 var is outside the outer - check the forms */
		state[depth] |= COVAR_X_SEEN;
		if ((state[depth] & (COVAR_I_SEEN|COVAR_O_SEEN)) == 0)
		{
		    /* A var specified from outside outer AND no vars from the
		    ** inner or outer - a simple restriction (a bit out of place) */
		    state[depth] |= COVAR_RESTRICTX;
		    break;
		}
		if (blocking_or && (state[depth]&COVAR_UNDEROR))
		{
		    /* If OR is to degrade correlations then we have recorded the
		    ** fact that an external has been seen but we will disregard
		    ** correlation for this. */
		    break;
		}
		if ((state[depth] & (COVAR_I_SEEN|COVAR_O_SEEN))==COVAR_I_SEEN && (
		    BTsubset(&lvars, i_tvrm, BITS_IN(PST_J_MASK)) &&
		    !BTintersects(&rvars, i_tvrm, BITS_IN(PST_J_MASK)) ||
		    BTsubset(&rvars, i_tvrm, BITS_IN(PST_J_MASK)) &&
		    !BTintersects(&lvars, i_tvrm, BITS_IN(PST_J_MASK))))
		{
		    /* Inner correlated with external */
		    if (node->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_eq)
			state[depth] |= COVAR_EQUIJOINX;
		    else
			state[depth] |= COVAR_NONEQUIJOINX;
		    state[depth] |= COVAR_CORELX;
		    break;
		}
		if ((state[depth] & (COVAR_I_SEEN|COVAR_O_SEEN))==COVAR_O_SEEN && (
		    BTsubset(&lvars, o_tvrm, BITS_IN(PST_J_MASK)) &&
		    !BTintersects(&rvars, o_tvrm, BITS_IN(PST_J_MASK)) ||
		    BTsubset(&rvars, o_tvrm, BITS_IN(PST_J_MASK)) &&
		    !BTintersects(&lvars, o_tvrm, BITS_IN(PST_J_MASK))))
		{
		    /* Inner correlated with external */
		    if (node->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_eq)
			state[depth] |= COVAR_EQUIJOINO;
		    else
			state[depth] |= COVAR_NONEQUIJOINO;
		    state[depth] |= COVAR_CORELO;
		    break;
		}
		state[depth] |= COVAR_MESS;
	    }
	    break;
        case PST_SUBSEL:
	    /*FALLTHROUGH*/
        case PST_ROOT:
	    depth--;
	    /* If there are states that should follow from
	    ** inner scope, mask them in now. (None at present)*/
	    state[depth] |= (state[depth+1]&(0));
	    subsel = psubsel;
	    psubsel = opv_antecedant_by_2types(&stk, NULL, PST_SUBSEL, PST_ROOT);
	    BTclearmask(BITS_IN(*varmap),
		&node->pst_sym.pst_value.pst_s_root.pst_tvrm, varmap);
	    break;
        case PST_AGHEAD:
	    BTclearmask(BITS_IN(*varmap),
		&node->pst_sym.pst_value.pst_s_root.pst_tvrm, varmap);
	    if (~state[depth] & COVAR_IN_WHERE|COVAR_GROUPED)
		state[depth] |= COVAR_SIMPAGG_IN_TL;
	    break;
	case PST_BYHEAD:
	    state[depth] |= COVAR_GROUPED;
	    break;
	case PST_RESDOM:
    	    p = opv_parent_node(&stk, NULL);
	    if (p && p->pst_sym.pst_type != PST_RESDOM)
		/* Finished this levels tgtlist - onto WHERE clause*/
		state[depth] |= COVAR_IN_WHERE;
	default:
	    break;
	}
	nodep = opv_pop_nodep(&stk);
	if (descend = (nodep == OPV_DESCEND_MARK))
	    nodep = opv_pop_nodep(&stk);
    }
    return state[0]|state[1];
}

/*{
** Name: opa_subsel_agh_lace - lace common agheads together
**
** Description:
**      This routine links together all the aghead blocks that share
**	the same WHERE clause. In essence it should be 1 WHERE clause
**	but due to the way we represent the AGHEAD structures, each gets
**	its own copy. This is ok in principle but when flattening the
**	the WHERE clause generally, it will mean potentially flattening
**	it once per aggregate. This is not just undesirable to do this
**	more than once but has other problems if done severally one at a
**	time - especially if the first updates the tree, the later copies
**	will fail to flatten due to the from masks beeing different.
**
**	What we do is chain all the AGHEADs together that share the common
**	WHERE (equivalent to being directly clustered under the same
**	SUBSEL)
**
**	NOTE: This routine is intended as a temporary step to get the tree
**	laced as required. Ideally, this should be done in the parser and
**	in such a way that this list identify the n-AGHEADs that point to
**	the same where clause. The list can then be used to replicate the
**	tree later if need be. Thus during flattening we may operate on 
**	the WHERE clause once avoiding the aforementioned problems and
**	inefficiencies.
**    
**
** Inputs:
**      global		ptr to global state variable OPS_STATE
**	root		Root node
**	enable		Boolean to determine whether we are lacing or
**			unlacing and replicating.
**
** Outputs:
**
**      Returns:
**	    none
**      Exceptions:
**	    none
** Side Effects:
**
** History:
**      24-Mar-2011 (kiria01)
**          Created to aid common WHERE processing of multiple aggregates.
*/
static void
opa_subsel_agh_lace(
	OPS_STATE	*global,
	PST_QNODE	*root,
	bool		enable)
{
    PST_QNODE **nodep;
    i4 depth = 0, fp = 0;
    bool descend = TRUE;
    OPV_STK stk;
    OPV_STK_CMP cmp;
    u_i2 frames[MAX_NESTING];
    PST_QNODE *agh[2*MAX_NESTING];

    OPV_STK_INIT(stk, global);
    OPV_STK_CMP_INIT(cmp, global);

    nodep = &root;

    if (nodep && *nodep)
    while (nodep)
    {
	register PST_QNODE **p, *node = *nodep;
	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && (node->pst_left || node->pst_right))
	{
	    /* Increment depth counters for children */
	    switch (node->pst_sym.pst_type)
	    {
	    case PST_SUBSEL:
	    case PST_ROOT:
		/* On the way down, entering a new SELECT scope, clear the
		** list head pointer for aggregates of this select - they 
		** all share a common WHERE clause. */
		frames[++depth] = fp;
		/* Include any unions into the mix */
		if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    opv_push_nodep(&stk,
			&node->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    /* Mark that the top node needs descending */
		    opv_push_nodep(&stk, OPV_DESCEND_MARK);
		}
		break;
	    default:
		break;
	    }
	    /* Delay node evaluation */
	    opv_push_nodep(&stk, nodep);
	    if (node->pst_left)
	    {
		if (node->pst_right)
		{
		    /* Delay RHS */
		    opv_push_nodep(&stk, &node->pst_right);
		    if ((node->pst_right->pst_right || node->pst_right->pst_left) &&
			node->pst_right->pst_sym.pst_type != PST_CONST &&
			node->pst_right->pst_sym.pst_type != PST_VAR)
			/* Mark that the top node needs descending */
			opv_push_nodep(&stk, OPV_DESCEND_MARK);
		}
		nodep = &node->pst_left;
		continue;
	    }
	    else if (node->pst_right)
	    {
		nodep = &node->pst_right;
		continue;
	    }
	}

	/*** Each node will be passed here once, in sequence, bottom-up, left to right ***/
	switch (node->pst_sym.pst_type)
	{
        case PST_SUBSEL:
        case PST_ROOT:
	    fp = frames[depth--];
	    break;
        case PST_AGHEAD:
	    if (enable)
	    {
		i4 i;

		/* agh[frames[depth]..fp-1] is the current list of distinct where
		** clause fragments under this SUBSEL. These should all be the same but
		** we'll not trust that fact. What we do trust is that the common where
		** fragments are identically structured or co-incident.
		** Need to see if current matches any and if so link into the
		** relevant list. */
		for (i = frames[depth]; i < fp; i++)
		{
		    if (agh[i]->pst_right == node->pst_right ||
			!opv_qtree_compare(&cmp, &agh[i]->pst_right, &node->pst_right, FALSE))
		    {
			/* Does match so link in. */
			p = &agh[i]->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
			node->pst_sym.pst_value.pst_s_root.pst_union.pst_next = *p;
			*p = node;
			break;
		    }
		}
		if (i >= fp)
		{
		    /* No match with given list */
		    node->pst_sym.pst_value.pst_s_root.pst_union.pst_next = node;
		    agh[fp++] = node;
		}
	    }
	    else if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	    {
		PST_QNODE *q = node->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
		/* We are unlacing - and replicating the trees.
		** We've skipped the first node - it will have the primary copy */
		while (q && q != node)
		{
		    PST_QNODE *tmp = q->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
		    /* Clear the link node */
		    q->pst_sym.pst_value.pst_s_root.pst_union.pst_next = NULL;
		    /* Only if the right pointers match do we need to make a copy */
		    if (q->pst_right == node->pst_right)
			/* Need to split off a copy */
			opv_copytree(global, &q->pst_right);
		    q = tmp;
		}
		node->pst_sym.pst_value.pst_s_root.pst_union.pst_next = NULL;
	    }
	    break;
	default:
	    break;
	}
	nodep = opv_pop_nodep(&stk);
	if (descend = (nodep == OPV_DESCEND_MARK))
	    nodep = opv_pop_nodep(&stk);
    }
}

/*{
** Name: opa_subsel_agh_common - Apply the shared transformation
**
** Description:
**      This routine applies the shared transformation to all of the
**	AGHEADs. It does so by simply pointing each AGHEAD in the cyclic
**	list to the transformed where clause and propagating the FROM list
**	to match. In essence it should be 1 WHERE clause anyway and for the
**	remainder of processing it will be so until undone by another call
**	fleshed these out again by a call to opa_subsel_agh_lace to disable
**	the cross links (the cyclic list) and to restore the separate trees.
**
**	If the agh has not been linked by opa_subsel_agh_lace(enable) then
**	this function is a no-op.
**
** Inputs:
**	agh		AGHEAD that has just had it WHERE clause updated.
**
** Outputs:
**      Returns:
**	    none
**      Exceptions:
**	    none
** Side Effects:
**	Will update all AGHEADs to refer to the same WHERE clause fragment.
**
** History:
**      24-Mar-2011 (kiria01)
**          Created to aid common WHERE processing of multiple aggregates.
*/
static void
opa_subsel_agh_common(
	PST_QNODE *agh)
{
    if (agh->pst_sym.pst_type == PST_AGHEAD)
    {
	PST_QNODE *p;

	/* Traverse the cyclic list updating the AGHEAD WHEREs to be common and the
	** FROM list to match that augmented. */
	for (p = agh->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
		    p && p != agh;
		    p = p->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    p->pst_right = agh->pst_right;
	    p->pst_sym.pst_value.pst_s_root.pst_tvrm =
		    agh->pst_sym.pst_value.pst_s_root.pst_tvrm;
	}
    }
}

/*{
** Name: opa_subsel_agh_count_present - Check common agh for COUNT
**
** Description:
**      This routine checks the peer aggregates to see if any are COUNT. It
**	does so by simply traversing each AGHEAD in the cyclic list to look
**	at it's AOP node 
**
**	If the agh has not been linked by opa_subsel_agh_lace(enable) then
**	this function just looks at the agh in question.
**
** Inputs:
**      global		ptr to global state variable OPS_STATE
**	agh		AGHEAD that is in scope.
**
** Outputs:
**      Returns:
**	    boolean
**      Exceptions:
**	    none
** Side Effects:
**	none.
**
** History:
**      24-Mar-2011 (kiria01)
**          Created to aid common WHERE processing of multiple aggregates.
*/
static bool
opa_subsel_agh_count_present(
	OPS_STATE *global,
	PST_QNODE *agh)
{
    if (agh->pst_sym.pst_type == PST_AGHEAD)
    {
	PST_QNODE *p = agh;

	/* Traverse the cyclic list examining each AOP looking for a COUNT */
	while (p)
	{
	    PST_QNODE *aop = p->pst_left;
	    if (aop && aop->pst_sym.pst_type == PST_BYHEAD)
		aop = aop->pst_right;
	    if (aop && aop->pst_sym.pst_type == PST_AOP && (
		    aop->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_count ||
		    aop->pst_sym.pst_value.pst_s_op.pst_opno ==
				global->ops_cb->ops_server->opg_scount))
		    return TRUE;
	    p = p->pst_sym.pst_value.pst_s_root.pst_union.pst_next;

	    if (p == agh)
		break;
	}
    }
    return FALSE;
}
