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
#define        OPO_CORDERING      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

static OPO_ISORT
opo_aggregate_ordering(OPS_SUBQUERY *outer_sq,
	OPS_SUBQUERY *aggr_sq,
	OPV_VARS *outer_varp);

/**
**
**  Name: OPOCORDR.C - create multi-attribute ordering
**
**  Description:
**      Create an ordering given a list of equivalence classes, this 
**      will be used to create an ordering available from a BTREE 
**      in an ORIG node.
**
**
**
**  History:    
**      9-dec-87 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	04-Dec-1995 (murf)
**		Added NO_OPTIM for sui_us5 as a SEGV occurred when the help
**		command was issued within the sql session. The OS is
**		Solaris 2.4 and the compiler is 3.0.1.
**	5-jan-97 (inkdo01)
**	    Fixed 87509 by adding OPB_SUBSEL_CNST flag in opo_pr and testing
**	    for it in opo_kordering.
**	25-feb-97 (rosga02) Removed NO_OPTIM for sui_us5
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
OPO_ISORT opo_gnumber(
	OPS_SUBQUERY *subquery);
void opo_iobase(
	OPS_SUBQUERY *subquery);
OPO_ISORT opo_cordering(
	OPS_SUBQUERY *subquery,
	OPO_EQLIST *keyp,
	bool copykey);
void opo_makey(
	OPS_SUBQUERY *subquery,
	OPB_MBF *mbfp,
	i4 count,
	OPO_EQLIST **keylistp,
	bool tempmem,
	bool noconst);
void opo_pr(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
void opo_orig(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
void opo_rsort(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
OPO_ISORT opo_kordering(
	OPS_SUBQUERY *subquery,
	OPV_VARS *varp,
	OPE_BMEQCLS *eqcmap);

/*{
** Name: opo_gnumber	- get a multi-attribute ordering number
**
** Description:
**      Get the next available multi-attribute ordering number
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    OPO_ISORT - empty multi-attribute ordering slot
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-dec-87 (seputis)
**          initial creation
[@history_template@]...
*/
OPO_ISORT
opo_gnumber(
	OPS_SUBQUERY       *subquery)
{
    /* check if ordering slot is available */
    if (!(subquery->ops_msort.opo_sv % OPO_BITMAP))
    {	/* if a boundary has been reached then a new set of slots needs to
        ** be allocated */
	OPO_ST		    *stp;
	stp = (OPO_ST *)opn_memory(subquery->ops_global, 
	    (i4)((subquery->ops_msort.opo_sv + OPO_BITMAP)*sizeof(OPO_SORT)));
	MEcopy((PTR)subquery->ops_msort.opo_base, 
	    subquery->ops_msort.opo_sv * sizeof(stp->opo_stable[0]),
	    (PTR)stp);
	subquery->ops_msort.opo_base = stp; /* FIXME - use old ptr
				    ** for something */
    }
    return (subquery->ops_msort.opo_sv++);
}

/*{
** Name: opo_iobase	- init ordering array of ptrs to descriptors
**
** Description:
**      If the sort ordering array of descriptors is not initialized 
**      then this routine will intialize it.
**
** Inputs:
**      subquery                        ptr to subquery to be analyzed
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-feb-88 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opo_iobase(
	OPS_SUBQUERY	*subquery)
{
    OPO_SORT          *sortp;
    if (subquery->ops_msort.opo_base)
	return;				/* return if already initialized */
    subquery->ops_msort.opo_sv = 0;
    subquery->ops_msort.opo_maxsv = 0;
    subquery->ops_msort.opo_base = (OPO_ST *)opn_memory(subquery->ops_global, 
	    (i4)(OPO_BITMAP*sizeof(sortp)));
}

/*{
** Name: opo_cordering	- create multi-attribute ordering
**
** Description:
**      Create a multi-attribute ordering for a BTREE orig node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      keyp                            ptr to array of equivalence classes
**	copykey				TRUE if keyp should be copied
**
** Outputs:
**	Returns:
**	    OPO_ISORT - number of multi-attribute ordering
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-dec-87 (seputis)
**          initial creation
**      5-jan-93 (ed)
**          do not create a multi-attribute ordering for a single
**          equivalence class
**	15-sep-93 (ed)
**	    add copy key parameter
[@history_template@]...
*/
OPO_ISORT
opo_cordering(
	OPS_SUBQUERY       *subquery,
	OPO_EQLIST	   *keyp,
	bool		    copykey)
{
    OPO_ISORT           ordering;	/* number used to represent
					** multi-attribute ordering */
    OPO_SORT		*orderp;        /* ptr to multi-attribute
					** ordering descriptor */
    OPO_ST              *obase;

    if (keyp->opo_eqorder[0] == OPE_NOEQCLS)
	return(OPE_NOEQCLS);		/* check for the null case */
    if (keyp->opo_eqorder[1] == OPE_NOEQCLS)
        return(keyp->opo_eqorder[0]);   /* check for the single attribute case */
    obase = subquery->ops_msort.opo_base;
    if (!obase)
	opo_iobase(subquery);		/* initialize the ordering table
					** base */
    {	/* search existing list for the multi-attribute ordering */
	i4		maxorder;

	maxorder = subquery->ops_msort.opo_sv;
	for( ordering = 0; ordering < maxorder; ordering++)
	{
	    orderp = obase->opo_stable[ordering];
	    if ( orderp->opo_stype == OPO_SEXACT)
	    {
		OPE_IEQCLS	*eqclsp;
		OPE_IEQCLS	*tempeqclsp;
		
		eqclsp = &orderp->opo_eqlist->opo_eqorder[0];
		tempeqclsp = &keyp->opo_eqorder[0];
		while ((*eqclsp) == (*tempeqclsp))
		{
		    eqclsp++;
		    tempeqclsp++;
		    if (((*eqclsp) == OPE_NOEQCLS)
			&&
			((*tempeqclsp) == OPE_NOEQCLS))
			return((OPO_ISORT)(ordering+subquery->ops_eclass.ope_ev));
					    /* multi-attribute ordering found
					    ** in list */
		}
	    }
	}
    }
    ordering = opo_gnumber(subquery);	/* get the next multi-attribute
					** ordering number */
    orderp = (OPO_SORT *)opn_memory(subquery->ops_global, (i4)sizeof(*orderp));
    orderp->opo_stype = OPO_SEXACT;
    orderp->opo_bmeqcls = (OPE_BMEQCLS *)opn_memory(subquery->ops_global, (i4)sizeof(OPE_BMEQCLS));
    MEfill(sizeof(OPE_BMEQCLS), (u_char)0, (PTR)orderp->opo_bmeqcls);
    orderp->opo_eqlist = keyp;
    {
	OPO_ISORT	*isortp;
	OPO_ISORT	maxsize;
	isortp = &keyp->opo_eqorder[0];
	for(maxsize = 1; *isortp != OPE_NOEQCLS ; isortp++)
	{
	    maxsize++;
	    BTset((i4)*isortp, (char *)orderp->opo_bmeqcls);
	}
	if (copykey)
	{   /* stack was used to build ordering so copy it into
	    ** opf memory */
	    maxsize = maxsize * sizeof(OPO_ISORT);
	    keyp = (OPO_EQLIST *)opn_memory(subquery->ops_global,
		(i4)maxsize);
	    MEcopy((PTR)orderp->opo_eqlist, maxsize, (PTR)keyp);
	    orderp->opo_eqlist = keyp;
	}
    }
    subquery->ops_msort.opo_base->opo_stable[ordering] = orderp;
    return((OPO_ISORT)(ordering+subquery->ops_eclass.ope_ev));
}


/*{
** Name: opo_makey	- create multi-attribute keylist
**
** Description:
**      Routine will create a multi-attribute keylist, terminated
**      by an OPE_NOEQLCS marker.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      mbfp                            ptr to boolean factor descriptor
**      count                           number of attribute elements to
**                                      consider
**      tempmem                         use enumeration memory if true
**                                      otherwise non-enumeration memory
**      noconst                         remove constant equivalence classes
**                                      if true
**
** Outputs:
**	Returns:
**	    OPO_ISORT *, ptr to an ordering list
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-88 (seputis)
**	    initial creation
**      5-jan-93 (ed)
**          bug 48049 add parameters to allow creation of equivalence class lists
**          which eliminate constant equivalence classes.
**	31-mar-03 (hayke02)
**	    Break out of the orderattr loop if we find an outer join on clause
**	    constant predicate - we cannot use this, or subsequent, attributes
**	    for ordering. This change fixes bug 109683.
[@history_line@]...
[@history_template@]...
*/
VOID
opo_makey(
	OPS_SUBQUERY       *subquery,
	OPB_MBF            *mbfp,
	i4                count,
	OPO_EQLIST         **keylistp,
	bool               tempmem,
	bool               noconst)
{   /* multi-attribute ordering */
    i4                     orderattr;   /* used to index array of ordering
				    ** attributes */
    OPZ_AT                 *abase;  /* base of array of ptrs to joinop
				    ** attribute elements */
    OPO_EQLIST		   *keylist;
    i4                     target;  /* index into target array */

    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to
					** joinop attribute elements */
    if (tempmem)
        keylist = (OPO_EQLIST *)opn_memory(subquery->ops_global,
            (i4) (sizeof(OPO_ISORT) * (count+1)));
    else
        keylist = (OPO_EQLIST *)opu_memory(subquery->ops_global,
            (i4) (sizeof(OPO_ISORT) * (count+1))); /* for now place this into
                                    ** non-enumeration memory since it will
                                    ** typically get placed into OPV_VARS
                                    ** structure, otherwise dangling ptrs
                                    ** into deleted memory streams may be
                                    ** created */
    *keylistp = keylist;
    target = 0;
    for (orderattr = 0; orderattr < count; orderattr++)
    {
	bool	ojconst = FALSE;	
        keylist->opo_eqorder[target] = abase->opz_attnums[mbfp->opb_kbase->
            opb_keyorder[orderattr].opb_attno]->opz_equcls;
        if (!noconst
            ||
            !subquery->ops_bfs.opb_bfeqc
            ||
            !BTtest((i4)keylist->opo_eqorder[target],
                (char *)subquery->ops_bfs.opb_bfeqc)
            )
            target++;               /* non-constant equivalence found */
	else if (subquery->ops_oj.opl_base)
	{
	    OPB_IBF	bfi;

	    for (bfi = 0; bfi < subquery->ops_bfs.opb_bv; bfi++)
	    {
		OPB_BOOLFACT	*bfp;

		bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi];
		if ((bfp->opb_eqcls == keylist->opo_eqorder[target]) &&
		    (bfp->opb_ojid >= 0))
		{
		    ojconst = TRUE;
		    break;
		}
	    }
	    if (ojconst)
		break;
	}
    }
    keylist->opo_eqorder[target] = OPE_NOEQCLS;   /* terminate eqcls list */
}

/*{
** Name: opo_ordmap	- map ordering from one OPS_SUBQUERY to another
**
** Description:
**      Routine will take an ordering from one OPS_SUBQUERY (typically a
**	virtual table) and create the same ordering in another.
**
** Inputs:
**      subquery                        ptr to target subquery
**	vtsqp				ptr to virtual table subquery
**	eqcno				order to map
**	varno				variable number of virtual table
**
** Outputs:
**	Returns:
**	    eqcno of new ordering
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
static OPE_IEQCLS
opo_ordmap(
	OPS_SUBQUERY	*subquery,
	OPS_SUBQUERY	*vtsqp,
	OPE_IEQCLS	eqcno,
	OPV_IVARS	varno)
{
    PST_QNODE		*rsdmp;
    OPE_BMEQCLS		eqcmap1, eqcmap2, *mapp;
    OPE_EQCLIST		*eqclp;
    OPZ_ATTS		*attrp;
    OPZ_IATTS		attno;
    i4			i;
    bool		found, allthere;

    /* Make map from order (if it is single EQC). Traverse bit map
    ** mapping each EQC in turn from source OPS_SUBQUERY to target. */
    MEfill(sizeof(eqcmap1), 0, (char *)&eqcmap1);
    MEfill(sizeof(eqcmap2), 0, (char *)&eqcmap2);
    if (eqcno < vtsqp->ops_eclass.ope_ev)
    {
	BTset(eqcno, (char *)&eqcmap1);
	mapp = &eqcmap1;
    }
    else
    {
	mapp = vtsqp->ops_msort.opo_base->opo_stable[eqcno - 
		vtsqp->ops_eclass.ope_ev]->opo_bmeqcls;
	eqcno = BTnext(-1, (char *)mapp, vtsqp->ops_eclass.ope_ev);
    }
    allthere = TRUE;

    /* Loop over EQCs in virtual table order. */
    for (eqcno = -1;(eqcno = BTnext(eqcno, (char *)mapp, 
			vtsqp->ops_eclass.ope_ev)) >= 0 && allthere; )
    {
	eqclp = vtsqp->ops_eclass.ope_base->ope_eqclist[eqcno];
	found = FALSE;
	
	/* Next pair of loops is over virtual table joinop attrs 
 	** in EQC, then over RESDOMs in virtual table, looking for
 	** joinops on rhs. */
	for (attno = -1; (attno = BTnext(attno, (char *)&eqclp->
	    ope_attrmap, vtsqp->ops_attrs.opz_av)) >= 0 && !found; )
	 for (rsdmp = vtsqp->ops_root->pst_left;
	     rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM && !found;
		rsdmp = rsdmp->pst_left)
	 {
	    if (rsdmp->pst_right->pst_sym.pst_type == PST_VAR &&
		rsdmp->pst_right->pst_sym.pst_value.pst_s_var.
		    pst_atno.db_att_id == attno &&
		(attrp = vtsqp->ops_attrs.opz_base->opz_attnums[attno])
		    ->opz_varnm == rsdmp->pst_right->pst_sym.pst_value.
		    pst_s_var.pst_vno)
	    {
		/* Found original PST_VAR for joinop. Final loop 
 		** traverses current SUBQUERY joinop list looking for 
		** vt RESDOM and sets its EQC in eqcmap. */
		for (i = 0;
			i < subquery->ops_attrs.opz_av && !found; i++)
		{
		    attrp = subquery->ops_attrs.opz_base->
						opz_attnums[i];
		    if (attrp->opz_varnm == varno &&
			attrp->opz_attnm.db_att_id == rsdmp->
		    	pst_sym.pst_value.pst_s_rsdm.pst_rsno)
		    {
			BTset(attrp->opz_equcls, (char *)&eqcmap2);
			found = TRUE;
			break;
		    }
		}
	    }
	}

	if (!found)
	    allthere = FALSE;
    }

    if (allthere)
	return(opo_fordering(subquery, &eqcmap2));
    else return(OPE_NOEQCLS);
}

/*{
** Name: opo_clustorig	- record cluster orderings for an ORIG node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of clusterings in table.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to CO node for base table
**
** Outputs:
**	Returns:
**	    cop		- updated to contain cluster orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
**	27-apr-2010 (dougi)
**	    Don't take REUSE subquery clustering.
**	6-jan-2011 (dougi)
**	    Add unique cols to clustered set.
*/
static VOID
opo_clustorig(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPV_GRV		*grvp;
    OPV_VARS		*varp;
    OPV_IVARS		varno;

    DMT_REFREL		*rrarrayp;
    DB_REFREL		*rrp = (DB_REFREL *) NULL;
    RDD_KEY_ARRAY	*keyp;

    OPZ_ATTS		*attrp;
    OPZ_IATTS		attno;

    OPO_ISORT		clvector[10];
    OPE_BMEQCLS		eqcmap, *mapp;
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_EQCLIST		*eqclp;

    i4			i, j, clcount;
    bool		allthere, found;


    /* Set up first, then determine if ORIG is for a "virtual table". If
    ** so, it will inherit clusterings from the root node of its CO-tree.
    ** If it is a simple base table (or index), it gets clusterings from
    ** its natural order and, for X100, the foreign key for a join index
    ** column, if it is sorted on the join index. */
    varno = cop->opo_union.opo_orig;
    varp = subquery->ops_vars.opv_base->opv_rt[varno];
    grvp = varp->opv_grv;
    cop->opo_clusteqcs = (OPO_ISORT *) NULL;
    cop->opo_clcount = 0;
    clcount = 0;

    if (grvp->opv_gquery != (OPS_SUBQUERY *) NULL &&
		grvp->opv_gquery->ops_sqtype != OPS_REUSE)
    {
	OPS_SUBQUERY	*vtsqp = grvp->opv_gquery;
	OPO_CO		*vcop = vtsqp->ops_bestco;
	PST_QNODE	*rsdmp;

	/* Virtual table - translate its clusterings (if any) into orderings
	** for current subquery (NOTE: reuse subqueries have no implied order/
	** clustering). */
	if (vcop == (OPO_CO *) NULL || vcop->opo_clcount == 0)
	    return;				/* nowt to do */

	/* Loop over clusterings of virtual table - check each for 
	** presence in current SUBQUERY's EQC list. */
	for (i = 0; i < vcop->opo_clcount; i++)
	{
	    eqcno = vcop->opo_clusteqcs[i];
	    clvector[clcount] = opo_ordmap(subquery, vtsqp, eqcno, varno);

	    /* If this is a valid order, increment clcount. */
	    if (clvector[clcount] >= 0)
		clcount++;
	}
		
	/* If clcount is non-zero, allocate that many OPO_ISORTs, address them
	** from opo_clusteqcs and fill in from clvector. Note the lazy use of an
	** array to hold the various column sets. Surely noone will have more
	** than 10 sets of unique columns in the same table!?! */
	if (clcount > 0)
	{
	    cop->opo_clcount = clcount;
	    cop->opo_clusteqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
			clcount * sizeof(OPO_ISORT));
	    for (i = 0; i < clcount; i++)
		cop->opo_clusteqcs[i] = clvector[i];
	}
	return;
    }

    /* Base table - if X100 and ordered on join index column, it will be 
    ** clustered on the corresponding foreign key column(s). */
  
    if (cop->opo_ordeqc == OPE_NOEQCLS && grvp && grvp->opv_relation &&
	(grvp->opv_relation->rdr_rel->tbl_storage_type == DMT_X100CL_TYPE ||
	grvp->opv_relation->rdr_rel->tbl_storage_type == DMT_X100IX_TYPE) &&
	(keyp = grvp->opv_relation->rdr_keys) != (RDD_KEY_ARRAY *) NULL &&
	keyp->key_count == 1)
    {
	for (i = 0, rrarrayp = varp->opv_grv->opv_relation->rdr_refrel;
			rrarrayp && i < rrarrayp->refrel_cnt; i++)
	{
	    rrp = &rrarrayp->refrel_array[i];
	    if (rrp->dbrr_refingixcol == keyp->key_array[0])
		break;
	}
	if (rrarrayp == (DMT_REFREL *) NULL || i >= rrarrayp->refrel_cnt)
	    rrp = (DB_REFREL *) NULL;

	/* rrp is either NULL or addresses the referential relationship
	** on which table is clustered. */
    }

    /* If table is ordered, it is clustered on the same thing. */
    if ((eqcno = cop->opo_ordeqc) >= 0)
    {
	clvector[clcount++] = cop->opo_ordeqc;

	/* If ORIG is ordered on a join index column, it is also clustered on
	** the foreign key. Look it up and add it to the clusterings. */
	if (eqcno < subquery->ops_eclass.ope_ev &&
	    ((eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno])->
			ope_mask & OPE_JOINIX))
	{
	    for (attno = -1; (attno = BTnext(attno, (char *)&eqclp->ope_attrmap,
			subquery->ops_attrs.opz_av)) >= 0; )
	     if ((attrp = subquery->ops_attrs.opz_base->opz_attnums[attno])->
			opz_varnm == varno)
		break;

	    if (attno >= 0)
	     for (i = 0, rrarrayp = varp->opv_grv->opv_relation->rdr_refrel;
			rrarrayp && i < rrarrayp->refrel_cnt; i++)
	     {
		rrp = &rrarrayp->refrel_array[i];
		if (attrp->opz_attnm.db_att_id == rrp->dbrr_refingixcol)
		    break;
	     }
	    if (attno < 0 || rrarrayp == (DMT_REFREL *) NULL ||
		i >= rrarrayp->refrel_cnt)
		rrp = (DB_REFREL *) NULL;

	    /* rrp is either NULL or addresses the referential relationship
	    ** on which table is clustered. */
	}
    }

    /* Merge unique column sets - table is clustered on them by definition. */
    for (i = 0; i < cop->opo_ucount; i++)
    {
	for (j = 0, found = FALSE; j < clcount && !found; j++)
	 if (cop->opo_ueqcs[i] == clvector[j])
	    found = TRUE;

	if (!found)
	    clvector[clcount++] = cop->opo_ueqcs[i];
    }

    /* If there is a clustering referential relationship, verify that all
    ** foreign key columns are in the joinop array, create ordering and 
    ** save. */
    if (rrp)
    {
	/* Verify that all foreign key columns are in joinop array,
	** then create ordering and save. */
	MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	for (i = 0, allthere = TRUE; i < rrp->dbrr_colcount && allthere; i++)
	{
	    for (j = 0; j < subquery->ops_attrs.opz_av; j++)
	    {
		if ((attrp = subquery->ops_attrs.opz_base->
				opz_attnums[j])->opz_varnm != varno ||
		    attrp->opz_attnm.db_att_id != 
				rrp->dbrr_refingcols[i])
		    continue;
		BTset(attrp->opz_equcls, (char *)&eqcmap);
		break;			/* found it, on to next */
	    }
	    if (j >= subquery->ops_attrs.opz_av)
		allthere = FALSE;
	}

	if (allthere)
	    clvector[clcount++] = opo_fordering(subquery, &eqcmap);
    }
    
    /* If clcount is non-zero, allocate that many OPO_ISORTs, address them
    ** from opo_clusteqcs and fill in from clvector. Note the lazy use of an
    ** array to hold the various column sets. Surely noone will have more
    ** than 10 sets of unique columns in the same table!?! */
    if (clcount > 0)
    {
	cop->opo_clcount = clcount;
	cop->opo_clusteqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
			clcount * sizeof(OPO_ISORT));
	for (i = 0; i < clcount; i++)
	    cop->opo_clusteqcs[i] = clvector[i];
    }


}

/*{
** Name: opo_clustpr_rsort	- record cluster orderings for a PR or RSORT 
**	node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of clusters still active in a PR or RSORT node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_PR/RSORT CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain cluster orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
VOID
opo_clustpr_rsort(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPO_SORT		*orderp;
    i4			i, clcount;
    OPO_ISORT		clvector[10];

    /* Loop over entries in subordinate COnode & retain those
    ** for which EQCs are in this node, too. */
    cop->opo_clcount = cop->opo_outer->opo_clcount;
    cop->opo_clusteqcs = cop->opo_outer->opo_clusteqcs;	/* copy to be lazy */

    if (cop->opo_clcount <= 0)
	return;						/* nothing to do */

    for (i = 0, clcount = 0; i < cop->opo_clcount; i++)
    {
	if ((eqcno = cop->opo_clusteqcs[i]) >= maxeqcls)
	{
	    orderp = subquery->ops_msort.opo_base->opo_stable[eqcno-maxeqcls];
	    if (BTsubset((char *)orderp->opo_bmeqcls, 
			(char *)&cop->opo_maps->opo_eqcmap, maxeqcls))
		clvector[clcount++] = eqcno;
	}
	else if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
	    clvector[clcount++] = eqcno;
    }

    /* If we didn't copy them all, must reallocate and condense. Otherwise,
    ** just leave DB_PR addressing same list as below. */
    if (clcount < cop->opo_clcount)
    {
	cop->opo_clcount = clcount;
	if (clcount > 0)
	{
	    cop->opo_clusteqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
				clcount * sizeof(OPO_ISORT));
	    for (i = 0; i < clcount; i++)
		cop->opo_clusteqcs[i] = clvector[i];
	}
	else cop->opo_clusteqcs = (OPO_ISORT *) NULL;
    }
}

/*{
** Name: opo_clustsj	- record cluster orderings for a SJ node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of clusters still active in a SJ node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_SJ CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain cluster orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
VOID
opo_clustsj(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPO_CO		*outerp = cop->opo_outer;
    OPE_EQCLIST		*eqclsp;
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPO_SORT		*orderp;
    i4			i, j, clcount;
    OPO_ISORT		clvector[10];

    /* Initially, we will propagate clusterings from the left/outer 
    ** side of the join. And only for X100 - native Ingres should be
    ** revisited at a later date. X100 joins tend to retain ordering/
    ** clustering properties of the left side. Clustering columns must
    ** still be in join EQC map, though. */
    cop->opo_clcount = 0;			/* init */
    cop->opo_clusteqcs = (OPO_ISORT *) NULL;

    if (!(subquery->ops_global->ops_gmask & OPS_X100))
	return;

    /* Loop over left/outer & see if ordering is still in join output. */
    for (i = 0, clcount = 0; i < outerp->opo_clcount; i++)
    {
	if ((eqcno = outerp->opo_clusteqcs[i]) >= maxeqcls)
	{
	    orderp = subquery->ops_msort.opo_base->opo_stable[eqcno-maxeqcls];
	    if (BTsubset((char *)orderp->opo_bmeqcls, 
			(char *)&outerp->opo_maps->opo_eqcmap, maxeqcls))
		clvector[clcount++] = eqcno;	/* add cluster to node */
	}
	else if (BTtest(eqcno, (char *)&outerp->opo_maps->opo_eqcmap))
	    clvector[clcount++] = eqcno;	/* add cluster to node */
    }

    /* Allocate opo_clusteqcs and copy (if any) (but only for X100 just yet). */
    if (clcount > 0 && (subquery->ops_global->ops_gmask & OPS_X100))
    {
	cop->opo_clusteqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
				clcount * sizeof(OPO_ISORT));
	MEcopy((char *)&clvector, clcount*sizeof(OPO_ISORT), 
				(char *)cop->opo_clusteqcs);
	cop->opo_clcount = clcount;
    }
    else
    {
	cop->opo_clcount = 0;
	cop->opo_clusteqcs = (OPO_ISORT *) NULL;
    }
}

/*{
** Name: opo_FDorig	- record FD orderings for an ORIG node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of FDs in table.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to CO node for base table
**
** Outputs:
**	Returns:
**	    cop		- updated to contain FD orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
**	1-feb-2010 (dougi)
**	    TIDs are NEVER dependent variables.
**	24-mar-2010 (dougi)
**	    Avoid FDs with "-1" as ordering.
*/
static VOID
opo_FDorig(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPV_GRV		*grvp;
    OPV_VARS		*varp;
    OPV_IVARS		varno;

    OPZ_ATTS		*attrp;
    OPE_EQCLIST		*eqcp;
    OPZ_IATTS		attno;

    OPO_ISORT		FDivector[30];
    OPO_ISORT		FDdvector[30];
    OPE_BMEQCLS		eqcmap;
    OPE_IEQCLS		eqcno, tideqc, maxeqcls = subquery->ops_eclass.ope_ev;

    i4			i, j, k, FDcount;
    bool		allthere;


    /* Set up first, see if this one even has FDs, then look at ORIG node
    ** to see if FD columns are in its eqcmap. FDs originate in unique
    ** constraints defined on base tables and on GROUP BY lists used to
    ** create aggregate virtual tables. */
    varno = cop->opo_union.opo_orig;
    varp = subquery->ops_vars.opv_base->opv_rt[varno];
    grvp = varp->opv_grv;
    tideqc = OPE_NOEQCLS;
    cop->opo_FDieqcs = cop->opo_FDdeqcs = (OPO_ISORT *) NULL;
    cop->opo_FDcount = 0;
    if (grvp == (OPV_GRV *) NULL)
	return;					/* no work to do */

    /* Separate virtual tables (results of some other query fragment) and
    ** base tables. */
    if (grvp->opv_gquery != (OPS_SUBQUERY *) NULL)
    {
	OPS_SUBQUERY	*vtsqp = grvp->opv_gquery;
	OPO_CO		*vcop = vtsqp->ops_bestco;
	PST_QNODE	*rsdmp;
	OPZ_IATTS	jixatt = -1;
	OPE_IEQCLS	eqcno1, eqcno2, jixeqc = OPE_NOEQCLS;
	OPE_BMEQCLS	depmap;
	OPZ_BMATTS	gbmap;
	/* Initially, we're interested in aggregate subqueries - there is
	** one FD - GROUP BY columns are independent, aggregate function 
	** results are dependent (actually, if subquery has FD in which 
	** some GB cols are DERIVED, they are also dependent).
	**
	** Next step is to migrate other existing FDs from subquery. */
	if (vtsqp->ops_sqtype != OPS_FAGG && vtsqp->ops_sqtype != OPS_RFAGG &&
		vtsqp->ops_sqtype != OPS_HFAGG)
	{
	    if (vtsqp->ops_sqtype != OPS_REUSE)
		return;			/* for now - later more */

	    /* Move FDs from reuse sq to here. */
	    for (i = 0, FDcount = 0; i < vcop->opo_FDcount; i++)
	    {
		/* For each FD from top CO-node of REUSE subquery, map 
		** independent and dependent EQCs to corresponding EQCs in
		** current subquery and create new orders. */
		eqcno1 = opo_ordmap(subquery, vtsqp, 
					vcop->opo_FDieqcs[i], varno);
		if (eqcno1 >= 0)
		    eqcno2 = opo_ordmap(subquery, vtsqp, 
					vcop->opo_FDdeqcs[i], varno);
		if (eqcno1 >= 0 && eqcno2 >= 0)
		{
		    FDivector[FDcount] = eqcno1;
		    FDdvector[FDcount++] = eqcno2;
		}
	    }
	}
	else
	{
	    /* Aggregate subquery - GROUP BY columns define the single FD. */
	    MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	    MEfill(sizeof(depmap), 0, (char *)&depmap);
	    MEfill(sizeof(gbmap), 0, (char *)&gbmap);

	    /* Build map of GB columns from virtual table. */
	    for (rsdmp = vtsqp->ops_byexpr; 
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
	    {
		BTset(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
				(char *)&gbmap);
		if (rsdmp->pst_right->pst_sym.pst_type == PST_VAR)
		{
		    attrp = vtsqp->ops_attrs.opz_base->opz_attnums[rsdmp->
			pst_right->pst_sym.pst_value.pst_s_var.pst_atno.
					db_att_id];
		    if (vtsqp->ops_eclass.ope_base->ope_eqclist[attrp->
			opz_equcls]->ope_mask & (OPE_JOINIX | OPE_JOINIXNOREF))
			jixatt = rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		}
	    }

	    for (i = 0; i < subquery->ops_attrs.opz_av; i++)
	    {
		/* Look for joinops of this subquery that originate in the 
		** agg subquery & classify them as independent or dependent. */
		if ((attrp = subquery->ops_attrs.opz_base->opz_attnums[i])->
				opz_varnm != cop->opo_union.opo_orig)
		    continue;		/* only those in virtual table */

		/* 1st byexpr entry is highest numbered GROUP BY column. */
		if (attrp->opz_attnm.db_att_id > vtsqp->ops_byexpr->pst_sym.
				pst_value.pst_s_rsdm.pst_rsno)
		    BTset(attrp->opz_equcls, (char *)&depmap);
		else 
		{
		    BTset(attrp->opz_equcls, (char *)&eqcmap);
		    BTclear(attrp->opz_attnm.db_att_id, (char *)&gbmap);
					/* remove fromn GROUP BY map */
		}
		if (jixatt >= 0 && jixatt == attrp->opz_attnm.db_att_id)
		    jixeqc = attrp->opz_equcls;

		/* Remember TID EQC (if there). */
		if (attrp->opz_attnm.db_att_id == DB_IMTID)
		    tideqc = attrp->opz_equcls;
	    }

	    /* If not all GB columns are referenced in ORIG node, there's
	    ** no dependency. */
	    if (BTcount((char *)&gbmap, BITS_IN(gbmap)))
		FDcount = 0;
	    else
	    {
		FDivector[0] = opo_fordering(subquery, &eqcmap);
		FDdvector[0] = opo_fordering(subquery, &depmap);
		if (FDivector[0] >= 0 && FDdvector[0] >= 0)
		    FDcount = 1;
		else FDcount = 0;		/* this is possible in non-
						** correlated agg subqueries */
	    }

	    /* If there is a join index amongst the grouping columns, it
	    ** defines an FD on the rest. */
	    if (jixeqc > OPE_NOEQCLS)
	    {
		BTor(maxeqcls, (char *)&depmap, (char *)&eqcmap);
		BTclear(jixeqc, (char *)&eqcmap);
		FDivector[FDcount] = jixeqc;
		FDdvector[FDcount++] = opo_fordering(subquery, &eqcmap);
	    }

	    /* Might later merge lower FDs to move DERIVED GROUP BY columns 
	    ** to dependant side of FD. */
	}

    }

    else 
    {
	/* Real base table. */
	for (i = 0, j = 0, FDcount = 0; i < grvp->opv_ucount; i++)
	{
	    /* Base table - loop over global variable's sets of FDs (unique
	    ** constraints). For each, check if columns are in local variable's 
	    ** EQC list. */
	    MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);

	    /* Loop over columns of this unique set - look up corr. joinop
	    ** variable, then see if it is in CO-node's EQC map. */
	    for (allthere = TRUE; (attno = grvp->opv_uvector[j]) >= 0; j++)
	    {
		for (k = 0; k < subquery->ops_attrs.opz_av && allthere; k++)
		{
		    attrp = subquery->ops_attrs.opz_base->opz_attnums[k];
		    if (attrp->opz_attnm.db_att_id != attno || 
			attrp->opz_varnm != varno)
			continue;

		    /* Remember TID EQC (if there). */
		    if (attrp->opz_attnm.db_att_id == DB_IMTID)
			tideqc = attrp->opz_equcls;

		    /* Found joinop for current FD column. Is it in node? */
		    eqcno = attrp->opz_equcls;
		    if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
			BTset(eqcno, (char *)&eqcmap);
		    else allthere = FALSE;
		    break;			/* in any event, loop's done */
		}
		if (k >= subquery->ops_attrs.opz_av)
		    allthere = FALSE;		/* fell off end - didn't find
						** column */
	    }

	    /* If allthere, we have a candidate to call fordering with. */
	    j++;				/* start of next */
	    if (!allthere)
		continue;

	    FDivector[FDcount] = opo_fordering(subquery, &eqcmap);
	    BTxor(subquery->ops_eclass.ope_ev, 
		(char *)&cop->opo_maps->opo_eqcmap, (char *)&eqcmap);
	    if (tideqc > OPE_NOEQCLS)
		BTclear((i4)tideqc, (char *)&eqcmap);
	    FDdvector[FDcount++] = opo_fordering(subquery, &eqcmap);
	    if (FDcount >= 30)
		break;				/* surely this is enough! */
	}

	/* Finally, loop through attrs looking for a join index. It forms
	** a FD with the corresponding foreign key. */
	for (i = 0; i < subquery->ops_attrs.opz_av; i++)
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[i];
	    if (attrp->opz_varnm != varno)
		continue;
	    eqcp = subquery->ops_eclass.ope_base->ope_eqclist[attrp->
								opz_equcls];
	    if (!(eqcp->ope_mask & (OPE_JOINIX | OPE_JOINIXNOREF)) ||
			eqcp->ope_joinrep == (OPE_BMEQCLS *) NULL)
		continue;			/* skip unless join index */

	    /* Got a join index - set up FD. */
	    FDivector[FDcount] = attrp->opz_equcls;
	    FDdvector[FDcount++] = opo_fordering(subquery, eqcp->ope_joinrep);
	    break;				/* quit loop - only 1 jix */
	}
    }

    /* If FDcount is non-zero, allocate that many OPO_ISORTs, address them
    ** from opo_FDi/deqcs and fill in from FDi/dvector. Note the lazy use of
    ** arrays to hold the various column sets. Surely noone will have more
    ** than 10 sets of unique columns in the same table!?! */
    if (FDcount > 0)
    {
	cop->opo_FDcount = FDcount;
	cop->opo_FDieqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
			2 * FDcount * sizeof(OPO_ISORT));
	cop->opo_FDdeqcs = &cop->opo_FDieqcs[FDcount];
	for (i = 0; i < FDcount; i++)
	{
	    cop->opo_FDieqcs[i] = FDivector[i];
	    cop->opo_FDdeqcs[i] = FDdvector[i];
	}
    }


}

/*{
** Name: opo_FDpr_rsort	- record FD orderings for a PR or RSORT node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of FDs still active in a PR or RSORT node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_PR/RSORT CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain FD orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
static VOID
opo_FDpr_rsort(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_BMEQCLS		eqcmap;
    OPO_SORT		*orderp;
    i4			i, j, FDcount;
    OPO_ISORT		FDivector[30];
    OPO_ISORT		FDdvector[30];

    /* Loop over entries in subordinate COnode & retain those
    ** for which EQCs are in this node, too. */
    cop->opo_FDcount = cop->opo_outer->opo_FDcount;
    cop->opo_FDieqcs = cop->opo_outer->opo_FDieqcs;	/* copy to be lazy */
    cop->opo_FDdeqcs = cop->opo_outer->opo_FDdeqcs;

    if (cop->opo_FDcount <= 0)
	return;						/* nothing to do */

    for (i = 0, FDcount = 0; i < cop->opo_FDcount; i++)
    {
	FDivector[i] = FDdvector[i] = OPE_NOEQCLS;
	/* Check independent cols first - they must all exist in result. */
	if ((eqcno = cop->opo_FDieqcs[i]) >= maxeqcls)
	{
	    orderp = subquery->ops_msort.opo_base->opo_stable[eqcno-maxeqcls];
	    if (BTsubset((char *)orderp->opo_bmeqcls, 
			(char *)&cop->opo_maps->opo_eqcmap, maxeqcls))
		FDivector[i] = eqcno;
	}
	else if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
	    FDivector[i] = eqcno;

	/* Then check dependent cols - they need only overlap result. */
	if ((eqcno = cop->opo_FDdeqcs[i]) >= maxeqcls)
	{
	    MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[eqcno-
		maxeqcls]->opo_bmeqcls, sizeof(eqcmap), (char *)&eqcmap);
	    BTand(maxeqcls, (char *)&cop->opo_maps->opo_eqcmap, (char *)&eqcmap);
	    if (BTcount((char *)&eqcmap, maxeqcls))
		FDdvector[i] = opo_fordering(subquery, &eqcmap);
	}
	else if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
	    FDdvector[i] = eqcno;

	if (FDivector[i] >= 0 && FDdvector[i] >= 0)
	    FDcount++;
	if (FDcount >= 30)
	    break;			/* surely this is enough! */
    }

    /* If we didn't copy them all, must reallocate and condense. Otherwise,
    ** just leave DB_PR addressing same list as below. */
    if (FDcount < cop->opo_FDcount)
    {
	if (FDcount > 0)
	{
	    cop->opo_FDieqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
				2 * FDcount * sizeof(OPO_ISORT));
	    cop->opo_FDdeqcs = &cop->opo_FDieqcs[FDcount];
	    for (i = 0, j = 0; i < cop->opo_FDcount; i++)
	     if (FDivector[i] >= 0 && FDdvector[i] >= 0)
	     {
		/* Only if both independent and dependent columns are here. */
		cop->opo_FDieqcs[j] = FDivector[i];
		cop->opo_FDdeqcs[j++] = FDdvector[i];
	     }
	}
	else cop->opo_FDieqcs = cop->opo_FDdeqcs = (OPO_ISORT *) NULL;
	cop->opo_FDcount = FDcount;
    }
}

/*{
** Name: opo_FDsj	- record FD orderings for a SJ node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of FDs still active in a SJ node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_SJ CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain FD orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-may-2009 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
**	2-nov-2010 (dougi)
**	    Use knowledge of referential relationships to add more FDs.
*/
VOID
opo_FDsj(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPE_BMEQCLS		eqcmap, eqcmap1, eqcmap2;
    OPE_IEQCLS		eqcno, jeqc, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_EQCLIST		*eqclsp;
    OPO_CO		*dcop;

    i4			i, j, k, FDcount = 0;
    OPO_ISORT		FDivector[60];
    OPO_ISORT		FDdvector[60];


    cop->opo_FDieqcs = cop->opo_FDdeqcs = (OPO_ISORT *) NULL;	/* init */
    cop->opo_FDcount = 0;

    /* Join FDs are all the input FDs. We'll copy the input FDs (from both 
    ** sides of the join) where the independent EQCs are covered by the join
    ** and at least one dependent EQC is also in the join. Then we'll look
    ** for independent EQCs of one FD that are dependent FDs of another. 
    ** Such FDs can be consolidated. */

    /* First, loop over input FDs, copying those that qualify. */
    /* Outer first. */
    for (i = 0; i < cop->opo_outer->opo_FDcount; i++)
    {
	FDivector[FDcount] = cop->opo_outer->opo_FDieqcs[i];
	FDdvector[FDcount++] = cop->opo_outer->opo_FDdeqcs[i];
	if (FDcount >= 60)
	    break;			/* surely that's enough */
    }
    /* Then inner. */
    for (i = 0; i < cop->opo_inner->opo_FDcount; i++)
    {
	FDivector[FDcount] = cop->opo_inner->opo_FDieqcs[i];
	FDdvector[FDcount++] = cop->opo_inner->opo_FDdeqcs[i];
	if (FDcount >= 60)
	    break;			/* surely that's enough */
    }

    /* Now check for join EQC mapping a referential relationship. There's
    ** an FD from the join cols of the referencing side to all columns of
    ** the referenced side of the join. */
    goto skip_sjeqc;		/* opo_sjeqc is NOT reliable at this point
				** If it is multi-attr, it will be recomputed
				** and may be different later. It can also
				** get a SEGV referencing ops_msort.opo_base,
				** so we'll skip this code for now. */
    jeqc = cop->opo_sjeqc;
    if (jeqc < maxeqcls)
    {
	MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	BTset(jeqc, (char *)&eqcmap);
    }
    else
	MEcopy((char *)subquery->ops_msort.opo_base->
	    opo_stable[jeqc-maxeqcls]->opo_bmeqcls, sizeof(eqcmap),
	    (char *)&eqcmap);

    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)&eqcmap, maxeqcls)) >= 0; )
    {
	eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];
	jeqc = eqcno;
	if (eqclsp->ope_mask & OPE_REPJOINIX)
	    jeqc = BTnext(-1, (char *)eqclsp->ope_repjoin, maxeqcls);
	else if (!(eqclsp->ope_mask & OPE_JOINIX))
	    continue;
	if (jeqc < 0)
	    continue;

	/* Got one. Determine referenced from referencing and set all
	** referenced EQCs as the dependents. */
	if (jeqc != eqcno)
	    eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[jeqc];
	if (BTsubset((char *)eqclsp->ope_rjrefed, 
		(char *)cop->opo_outer->opo_variant.opo_local->opo_bmvars, 
		subquery->ops_vars.opv_rv))
	    dcop = cop->opo_outer;
	else if (BTsubset((char *)eqclsp->ope_rjrefed, 
		(char *)cop->opo_inner->opo_variant.opo_local->opo_bmvars, 
		subquery->ops_vars.opv_rv))
	    dcop = cop->opo_inner;
	else continue;

	FDivector[FDcount] = eqcno;
	MEcopy((char *)&dcop->opo_maps->opo_eqcmap, sizeof(eqcmap1), 
			(char *)&eqcmap1);

	/* Remove independent EQC before recording dependents. */
	BTclear(eqcno, (char*)&eqcmap1);
	FDdvector[FDcount++] = opo_fordering(subquery, &eqcmap1);
    }

skip_sjeqc:
    /* Search the work vectors for independent EQC sets that appear 
    ** in the dependent EQCs of some other FD. */
    for (i = 0; i < FDcount; i++)
     for (j = 0; j < FDcount; j++)
     {
	if (i == j || FDivector[i] == OPE_NOEQCLS || 
		FDivector[j] == OPE_NOEQCLS)
	    continue;

	/* Load independent and dependent EQC sets to work maps. */
	if ((eqcno = FDivector[i]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	    BTset(eqcno, (char *)&eqcmap);
	}
	else MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, sizeof(eqcmap),
		(char *)&eqcmap);

	if ((eqcno = FDivector[j]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmap2), 0, (char *)&eqcmap2);
	    BTset(eqcno, (char *)&eqcmap2);
	}
	else MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, sizeof(eqcmap2),
		(char *)&eqcmap2);

	if ((eqcno = FDdvector[j]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmap1), 0, (char *)&eqcmap1);
	    BTset(eqcno, (char *)&eqcmap1);
	}
	else MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, sizeof(eqcmap1),
		(char *)&eqcmap1);

	if (BTsubset((char *)&eqcmap, (char *)&eqcmap1, maxeqcls))
	{
	    /* FD i independent EQCs are subset of FD j dependent EQCs. 
	    ** Merge i & j dependent EQCs into j, but keep i because its
	    ** independent vars may be referenced explictly when j's aren't.
	    ** Also, remove i's independent EQCs from j's dependents. */
	    if ((eqcno = FDdvector[i]) < maxeqcls)
	    {
		MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
		BTset(eqcno, (char *)&eqcmap);
	    }
	    else MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, sizeof(eqcmap),
		(char *)&eqcmap);
	    BTnot(maxeqcls, (char *)&eqcmap2);
	    BTor(maxeqcls, (char *)&eqcmap, (char *)&eqcmap1);
	    BTand(maxeqcls, (char *)&eqcmap2, (char *)&eqcmap1);
	    FDdvector[j] = opo_fordering(subquery, &eqcmap1);
	}
     }

    /* If there are any FDs after all that, allocate & populate opo_FDeqcs.
    ** In the process, we'll throw away those in which the independent EQCs
    ** aren't covered by this node's eqcmap. */
    if (FDcount > 0)
    {
	/* Quickly consolidate the list by checking for dups. */
	for (i = 0, k = 0; i < FDcount-1; i++)
	 for (j = i+1; j < FDcount; j++)
	  if ((FDivector[i] == FDivector[j] && FDdvector[i] == FDdvector[j]) ||
			FDdvector[i] == -1)
	  {
	    FDivector[i] = -1;
	    k++;
	    break;
	  }

	cop->opo_FDieqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
				2 * (FDcount - k) * sizeof(OPO_ISORT));
	cop->opo_FDdeqcs = &cop->opo_FDieqcs[FDcount-k];
	for (i = 0, j = 0; i < FDcount; i++)
	{
	    if ((eqcno = FDivector[i]) < 0)
		continue;		/* skip consolidated entries */

	    if (FDivector[i] < maxeqcls)
	    {
		MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
		BTset(eqcno, (char *)&eqcmap);
	    }
	    else MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, sizeof(eqcmap),
		(char *)&eqcmap);
	    if (!BTsubset((char *)&eqcmap, (char *)&cop->opo_maps->opo_eqcmap,
					maxeqcls))
		continue;		/* skip the uncovered ones */
	    cop->opo_FDieqcs[j] = FDivector[i];
	    cop->opo_FDdeqcs[j++] = FDdvector[i];
	}
	cop->opo_FDcount = j;
    }
}

/*{
** Name: opo_uorig	- record uniqueness orderings for an ORIG node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of unique EQCs in table.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to CO node for base table
**
** Outputs:
**	Returns:
**	    cop		- updated to contain uniqueness orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-june-2010 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
static VOID
opo_uorig(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPV_GRV		*grvp;
    OPV_VARS		*varp;
    OPV_IVARS		varno;

    OPZ_ATTS		*attrp;
    OPZ_IATTS		attno;

    OPO_ISORT		uvector[10];
    OPE_BMEQCLS		eqcmap;
    OPE_IEQCLS		eqcno, tideqc, maxeqcls = subquery->ops_eclass.ope_ev;

    i4			i, j, k, ucount;
    bool		allthere;


    /* Set up first, see if this one even has unique constraints, then look 
    ** at ORIG node to see if unique columns are in its eqcmap. Unique column 
    ** sets originate in unique constraints defined on base tables and on 
    ** GROUP BY lists used to create aggregate virtual tables. */
    varno = cop->opo_union.opo_orig;
    varp = subquery->ops_vars.opv_base->opv_rt[varno];
    grvp = varp->opv_grv;
    tideqc = OPE_NOEQCLS;
    cop->opo_ueqcs = (OPO_ISORT *) NULL;
    cop->opo_ucount = 0;
    if (grvp == (OPV_GRV *) NULL)
	return;					/* no work to do */

    /* Separate virtual tables (results of some other query fragment) and
    ** base tables. */
    if (grvp->opv_gquery != (OPS_SUBQUERY *) NULL)
    {
	OPS_SUBQUERY	*vtsqp = grvp->opv_gquery;
	OPO_CO		*vcop = vtsqp->ops_bestco;
	PST_QNODE	*rsdmp;
	OPZ_IATTS	jixatt = -1;
	OPE_IEQCLS	eqcno1, eqcno2, jixeqc = OPE_NOEQCLS;
	OPZ_BMATTS	gbmap;
	if (vtsqp->ops_sqtype != OPS_FAGG && vtsqp->ops_sqtype != OPS_RFAGG &&
		vtsqp->ops_sqtype != OPS_HFAGG)
	{
	    if (vtsqp->ops_sqtype != OPS_REUSE)
		return;			/* for now - later more */

	    /* In a REUSE subquery, map the EQCs from its uniquenesses to
	    ** current subquery. */
	    for (i = 0, ucount = 0; i < vcop->opo_ucount; i++)
	    {
		eqcno1 = opo_ordmap(subquery, vtsqp, 
					vcop->opo_ueqcs[i], varno);
		if (eqcno1 >= 0)
		    uvector[ucount++] = eqcno1;
	    }
	}
	else
	{
	    /* Aggregate subquery - GROUP BY columns define the single 
	    ** uniqueness. */
	    MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	    MEfill(sizeof(gbmap), 0, (char *)&gbmap);

	    /* Build map of GB columns from virtual table. */
	    for (rsdmp = vtsqp->ops_byexpr; 
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
	    {
		if (rsdmp->pst_right->pst_sym.pst_type == PST_VAR)
		{
		    attrp = vtsqp->ops_attrs.opz_base->opz_attnums[rsdmp->
			pst_right->pst_sym.pst_value.pst_s_var.pst_atno.
					db_att_id];
		    if (vtsqp->ops_eclass.ope_base->ope_eqclist[attrp->
			opz_equcls]->ope_mask & (OPE_JOINIX | OPE_JOINIXNOREF))
			jixatt = rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		}
		BTset(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
				(char *)&gbmap);
	    }

	    for (i = 0; i < subquery->ops_attrs.opz_av; i++)
	    {
		/* Look for joinops of this subquery that are GROUP BY
		** columns of the agg subquery and save their EQCs. */
		if ((attrp = subquery->ops_attrs.opz_base->opz_attnums[i])->
				opz_varnm != cop->opo_union.opo_orig)
		    continue;		/* only those in virtual table */

		/* 1st byexpr entry is highest numbered GROUP BY column. */
		if (attrp->opz_attnm.db_att_id <= vtsqp->ops_byexpr->pst_sym.
				pst_value.pst_s_rsdm.pst_rsno)
		{
		    BTset(attrp->opz_equcls, (char *)&eqcmap);
		    BTclear(attrp->opz_attnm.db_att_id, (char *)&gbmap);
					/* remove fromn GROUP BY map */
		}
		if (jixatt >= 0 && jixatt == attrp->opz_attnm.db_att_id)
		    jixeqc = attrp->opz_equcls;
	    }

	    /* If not all GB columns are referenced in ORIG node, there's
	    ** no uniqueness. */
	    if (BTcount((char *)&gbmap, BITS_IN(gbmap)))
		ucount = 0;
	    else
	    {
		uvector[0] = opo_fordering(subquery, &eqcmap);
		ucount = 1;
	    }
	    if (jixeqc > OPE_NOEQCLS)
		uvector[ucount++] = jixeqc;
	}

    }
    else for (i = 0, j = 0, ucount = 0; i < grvp->opv_ucount; i++)
    {
	/* Base table - loop over global variable's sets of unique
	** constraints. For each, check if columns are in local variable's 
	** EQC list, then copy them. */
	MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);

	/* Loop over columns of this unique set - look up corr. joinop
	** variable, then see if it is in CO-node's EQC map. */
	for (allthere = TRUE; (attno = grvp->opv_uvector[j]) >= 0; j++)
	{
	    for (k = 0; k < subquery->ops_attrs.opz_av && allthere; k++)
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[k];
		if (attrp->opz_attnm.db_att_id != attno || 
			attrp->opz_varnm != varno)
		    continue;

		/* Found joinop for current unique column. Is it in node? */
		eqcno = attrp->opz_equcls;
		if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
		    BTset(eqcno, (char *)&eqcmap);
		else if (subquery->ops_bfs.opb_bfeqc == (OPE_BMEQCLS *) NULL || 
			!BTtest(eqcno, (char *)subquery->ops_bfs.opb_bfeqc))
		    allthere = FALSE;
		break;				/* in any event, loop's done */
	    }
	    if (k >= subquery->ops_attrs.opz_av)
		allthere = FALSE;		/* fell off end - didn't find
						** column */
	}

	/* If allthere, we have a candidate to call fordering with. */
	j++;					/* start of next */
	if (!allthere)
	    continue;

	uvector[ucount++] = opo_fordering(subquery, &eqcmap);

	/* Now see if this is multi-attr constraint. If so, is there another
	** to be gained by removing the constants? */
	if (subquery->ops_bfs.opb_mbfeqc == (OPE_BMEQCLS *) NULL ||
	    (k = BTcount((char *)&eqcmap, (i4)maxeqcls)) <= 1)
	    continue;				/* skip this heuristic */

	BTand((i4)maxeqcls, (char *)subquery->ops_bfs.opb_mbfeqc, 
							(char *)&eqcmap);
	if (BTcount((char *)&eqcmap, (i4)maxeqcls) < k)
	    uvector[ucount++] = opo_fordering(subquery, &eqcmap);
    }

    /* If ucount is non-zero, allocate that many OPO_ISORTs, address them
    ** from opo_ueqcs and fill in from uvector. Note the lazy use of work
    ** arrays to hold the various column sets. Surely noone will have more
    ** than 10 sets of unique columns in the same table!?! */
    if (ucount > 0)
    {
	cop->opo_ucount = ucount;
	cop->opo_ueqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
			ucount * sizeof(OPO_ISORT));
	for (i = 0; i < ucount; i++)
	    cop->opo_ueqcs[i] = uvector[i];
    }

}

/*{
** Name: opo_upr_rsort	- record uniqueness orderings for a PR or RSORT node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of uniquenesses still active in a PR or RSORT node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_PR/RSORT CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain uniqueness orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-june-2010 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
*/
VOID
opo_upr_rsort(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_BMEQCLS		eqcmap;
    OPO_SORT		*orderp;
    i4			i, j, ucount;
    OPO_ISORT		uvector[10];

    /* Loop over entries in subordinate COnode & retain those
    ** for which EQCs are in this node, too. */
    cop->opo_ucount = cop->opo_outer->opo_ucount;
    cop->opo_ueqcs = cop->opo_outer->opo_ueqcs;	/* copy to be lazy */

    if (cop->opo_ucount <= 0)
	return;						/* nothing to do */

    for (i = 0, ucount = 0; i < cop->opo_ucount; i++)
    {
	uvector[i] = OPE_NOEQCLS;

	/* Check columns in each subordinate uniqueness - they must also 
	** exist in result. */
	if ((eqcno = cop->opo_ueqcs[i]) >= maxeqcls)
	{
	    orderp = subquery->ops_msort.opo_base->opo_stable[eqcno-maxeqcls];
	    if (BTsubset((char *)orderp->opo_bmeqcls, 
			(char *)&cop->opo_maps->opo_eqcmap, maxeqcls))
		uvector[i] = eqcno;
	}
	else if (BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap))
	    uvector[i] = eqcno;

	if (uvector[i] >= 0)
	    ucount++;
    }

    /* If we didn't copy them all, must reallocate and condense. Otherwise,
    ** just leave DB_PR addressing same list as below. */
    if (ucount < cop->opo_ucount)
    {
	if (ucount > 0)
	{
	    cop->opo_ueqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
				ucount * sizeof(OPO_ISORT));
	    for (i = 0, j = 0; i < cop->opo_ucount; i++)
	     if (uvector[i] >= 0)
		cop->opo_ueqcs[j] = uvector[i];
	}
	else cop->opo_ueqcs = (OPO_ISORT *) NULL;
	cop->opo_ucount = ucount;
    }
}

/*{
** Name: opo_usj	- record uniqueness orderings for a SJ node
**
** Description:
**      Routine will create a vector of multi-attribute lists representing
**	sets of uniquenesses still active in a SJ node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop				ptr to DB_SJ CO node
**
** Outputs:
**	Returns:
**	    cop		- updated to contain uniqueness orderings (if any)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-june-2010 (dougi)
**	    Created for X100, but potentially useful in native Ingres.
**	13-dec-2010 (dougi)
**	    Rework logic. Wasn't right first time around.
**	11-jan-2011 (dougi)
**	    Recognize uniqueness of join index EQCs.
**	4-apr-2011 (dougi)
**	    Make sure join EQC applies to current join inputs.
**	27-apr-2011 (dougi)
**	    Fix bad deref into multi-attr ordering array.
*/
VOID
opo_usj(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)
{
    OPO_CO		*outer;
    OPE_BMEQCLS		jeqcmap, *jmapp = &jeqcmap;
    OPE_EQCLIST		*jeqclp, *jeqclp1;
    OPE_IEQCLS		maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_IEQCLS		jeqc = cop->opo_sjeqc, ueqc, jixeqc;
    OPV_IVARS		maxvars = subquery->ops_vars.opv_rv;
    i4			i, ucount;
    OPO_ISORT		uvector[10];
    bool		omatch, imatch;

    ucount = cop->opo_ucount = 0;				/* init */
    cop->opo_ueqcs = (OPO_ISORT *) NULL;
    if (jeqc < maxeqcls && jeqc >= 0)
	jeqclp = subquery->ops_eclass.ope_base->ope_eqclist[jeqc];
    else jeqclp = (OPE_EQCLIST *) NULL;

    /* If this is a semi- or anti-join, all outer constraints still hold
    ** because each outer row can appear no more than once. */
    if (cop->opo_variant.opo_local && 
	(cop->opo_variant.opo_local->opo_mask & (OPO_SEMIJOIN | OPO_ANTIJOIN)))
    {
	if (cop->opo_variant.opo_local->opo_mask & OPO_REVJOIN)
	    outer = cop->opo_inner;
	else outer = cop->opo_outer;

	for (i = 0; i < outer->opo_ucount; i++)
	 uvector[ucount++] = outer->opo_ueqcs[i];

	return;
    }

    /* If join maps a referential relationship, any uniquenesses from the
    ** child source apply to the result, too (because each child row joins
    ** to no more than 1 parent row). But if this is a REPJOINIX, make sure
    ** the chosen EQC matches both sides of the join. */
    jixeqc = -1;
    if (jeqclp)
    {
	if ((jeqclp->ope_mask & OPE_REPJOINIX) && jeqclp->ope_repjoin)
	{
	    for (jixeqc = -1; (jixeqc = BTnext(jixeqc, (char *)jeqclp->
		ope_repjoin, maxeqcls)) >= 0; )
	    {
		jeqclp1 = subquery->ops_eclass.ope_base->ope_eqclist[jixeqc];
		if ((BTsubset((char *)jeqclp1->ope_rjrefed, (char *)cop->
		      opo_outer->opo_variant.opo_local->opo_bmvars, maxvars) &&
		    BTsubset((char *)jeqclp1->ope_rjrefing, (char *)cop->
		      opo_inner->opo_variant.opo_local->opo_bmvars, maxvars)) ||
		    (BTsubset((char *)jeqclp1->ope_rjrefed, (char *)cop->
		      opo_inner->opo_variant.opo_local->opo_bmvars, maxvars) &&
		    BTsubset((char *)jeqclp1->ope_rjrefing, (char *)cop->
		      opo_outer->opo_variant.opo_local->opo_bmvars, maxvars)))
		    break;		/* stop when inner/outer covered */
	    }
	}
	else if (jeqclp->ope_mask & OPE_JOINIX)
	    jixeqc = jeqc;
    }

    if (jixeqc >= 0)
    {
	jeqclp = subquery->ops_eclass.ope_base->ope_eqclist[jixeqc];
	if (BTsubset((char *)jeqclp->ope_rjrefed, 
		(char *)cop->opo_outer->opo_variant.opo_local->opo_bmvars, 
		subquery->ops_vars.opv_rv))
	    outer = cop->opo_inner;
	else outer = cop->opo_outer;

	for (i = 0; i < outer->opo_ucount; i++)
	 uvector[ucount++] = outer->opo_ueqcs[i];
    }
	
    /* Otherwise, if the join EQC is the same or a subset of a uniqueness 
    ** constraint on either join source, that constraint holds on the join
    ** result. */
    {
	for (i = 0, omatch = FALSE; i < cop->opo_outer->opo_ucount && !omatch;
		i++)
	 if (cop->opo_outer->opo_ueqcs[i] == jeqc)
	    omatch = TRUE;

	for (i = 0, imatch = FALSE; i < cop->opo_inner->opo_ucount && !imatch;
		i++)
	 if (cop->opo_inner->opo_ueqcs[i] == jeqc)
	    imatch = TRUE;

	/* At least one of the inner or outer source must have a uniqueness
	** that matches the join EQC. */
	if (!omatch && !imatch)
	    return;

	/* If both have join EQC as uniqueness, so does the result. */
	if (omatch && imatch)
	    uvector[ucount++] = jeqc;

	/* Then, any other uniqueness that contains the join EQC as a subset
	** also holds for the result. */
	if (jeqc < maxeqcls)
	{
	    MEfill(sizeof(jeqcmap), 0, (char *)jmapp);
	    BTset(jeqc, (char *)jmapp);
	}
	else jmapp = subquery->ops_msort.opo_base->
				opo_stable[jeqc-maxeqcls]->opo_bmeqcls;

	if (imatch)
	 for (i = 0; i < cop->opo_outer->opo_ucount; i++)
	  if ((ueqc = cop->opo_outer->opo_ueqcs[i]) >= maxeqcls &&
		BTsubset((char *)jmapp, (char *)subquery->ops_msort.opo_base->
			opo_stable[ueqc-maxeqcls]->opo_bmeqcls, maxeqcls))
	    uvector[ucount++] = ueqc;

	if (omatch)
	 for (i = 0; i < cop->opo_inner->opo_ucount; i++)
	  if ((ueqc = cop->opo_inner->opo_ueqcs[i]) >= maxeqcls &&
		BTsubset((char *)jmapp, (char *)subquery->ops_msort.opo_base->
			opo_stable[ueqc-maxeqcls]->opo_bmeqcls, maxeqcls))
	    uvector[ucount++] = ueqc;
      }

    /* If ucount is non-zero, allocate that many OPO_ISORTs, address them
    ** from opo_ueqcs and fill in from uvector. Note the lazy use of work
    ** arrays to hold the various column sets. Surely noone will have more
    ** than 10 sets of unique columns in the same table!?! */
    if (ucount > 0)
    {
	cop->opo_ucount = ucount;
	cop->opo_ueqcs = (OPO_ISORT *)opu_memory(subquery->ops_global,
			ucount * sizeof(OPO_ISORT));
	for (i = 0; i < ucount; i++)
	    cop->opo_ueqcs[i] = uvector[i];
    }

}

/*{
** Name: opo_pr	- find ordering for project-restrict node
**
** Description:
**      Given a project restrict node, find the ordering which can be 
**      used to avoid sort nodes by parents.  If the ordering was
**      DB_BTRE_STORE, then the sortedness of the storage structure
**      can be assumed.  Ignore whether constant equivalence classes
**      are embedded, since this will be taken care of in opo_combine.
**      opo_ordeqc will contain an ordering only if sortedness is
**      used, but in the DB_ORIG node, any ordering which can be used
**      for keying will be stored.  This will work since a DB_ORIG
**      node will never be used as an outer for a join directly (i.e.
**      a project restrict will be on top of it).  If the ordering is
**      an inner then this implies keying will be used (FIXME check
**      for physical scan of inner).
**          Note that for a DB_BTRE_STORE, if a set of explicit TIDs are 
**      found then the attributes are not ordered, so check for this case.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to Project-Restrict node
**
** Outputs:
**      [@param_name@] [@comment_text@]#41$
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      10-feb-89 (seputis)
**          mark PR node as sorted only if usable ordering is available
**      24-apr-89 (seputis)
**          place only eqc required by parent into ordering, b5411
**      15-aug-89 (seputis)
**          modified interface to opn_prcost for b6538
**	16-may-90 (seputis)
**	    - fix lint errors 
**	20-jun-90 (seputis)
**	    - add support for top sort node removal
**      15-dec-91 (seputis)
**          - fix b40402 - incorrect answer with corelated group by aggregates
**	28-jan-92 (seputis)
**	    - fix 41939 - top sort node removal optimization when constant
**	    equivalence classes exist
**	28-jan-92 (seputis)
**	    - fix 40923 - remove parameter to opn_prcost
**      29-apr-92 (seputis)
**          - b43969 - check boundary condition to avoid overwriting stack
**      11-may-92 (seputis)
**          - check for out of bounds array access earlier in processing
**	    to prevent read segv on unix.
**          - check for constants in by list and treat aggregate result as
**	    a constant if this is true.
**	4-aug-93 (ed)
**	    - fix bug 51642, 52901, constant eqcls map was not consistently
**	    updated
**	5-jan-97 (inkdo01)
**	    Flag when there's a BF constant eqclass originating in the result
**	    of a single valued subselect.
**	27-mar-04 (inkdo01)
**	    Change to prevent assumption of presortedness on partitioned
**	    structured tables.
**	20-jan-2009 (dougi)
**	    Support DB_X100CL_STORE as another pre-sorted structure.
**	22-jan-2010 (dougi)
**	    And DB_X100IX_STORE is also sort of the same.
**	20-dec-10 (smeke01) b124316
**	    Add new utility function opo_aggregate_ordering() that looks for the
**	    key ordering on an RFAGG or FAGG aggregate subquery on a btree table
**	    and translates the ordering into an equivalence class ordering in
**	    terms of equivalence classes known to the outer subquery. Add a new
**	    conditional section in opo_pr() to call opo_aggregate_ordering()
**	    under the appropriate conditions.
*/
VOID
opo_pr(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    OPO_CO		*orig_cop;
    OPV_GRV		*grvp;		/* ptr to global range var descriptor
					** for ORIG node */
    OPV_VARS		*varp;		/* pointer to range variable
					** element associated with leaf */
    orig_cop = cop->opo_outer;		/* must be a DB_ORIG node on the outer */
    if (cop->opo_sjpr == DB_PR)
    {
	opo_FDpr_rsort(subquery, cop);
	opo_upr_rsort(subquery, cop);
	opo_clustpr_rsort(subquery, cop);	    /* propagate clusters */
    }

    varp = subquery->ops_vars.opv_base->opv_rt[orig_cop->opo_union.opo_orig];
    grvp = varp->opv_grv;
    if ((orig_cop->opo_storage == DB_HEAP_STORE)
	&&
	grvp
	&&
	grvp->opv_gquery
	&&
	(   (grvp->opv_created == OPS_RFAGG)
	    ||
	    (grvp->opv_created == OPS_FAGG)
	)
	&&
	grvp->opv_gquery->ops_bestco->opo_outer &&
	grvp->opv_gquery->ops_bestco->opo_outer->opo_storage == DB_BTRE_STORE &&
	grvp->opv_gquery->ops_bestco->opo_storage == DB_SORT_STORE)
    {
	/*
	** Since the aggregate subquery took its input from a btree table the
	** output from the aggregate subquery (our input) could be sorted on
	** some or all of the attributes.
	*/
	cop->opo_ordeqc = opo_aggregate_ordering(subquery, grvp->opv_gquery, varp);

	if (cop->opo_ordeqc == OPE_NOEQCLS)
	    cop->opo_storage = DB_HEAP_STORE;
    }
    else
    if ((orig_cop->opo_storage == DB_HEAP_STORE)
	&&
	grvp
	&&
	grvp->opv_gquery 
	&& 
	(   (grvp->opv_created == OPS_RFAGG)
	    ||
	    (grvp->opv_created == OPS_FAGG)
	))
    {   /* get the ordering available from the sorted by list of the
	** function aggregate */

	PST_QNODE	    *byexprp;
	i4		    sort_count;
	OPO_EQLIST	    templist;
	OPZ_AT		    *abase;
	OPO_ISORT	    agg_ordering; /* sorted ordering provide by an aggregate */
	OPE_IEQCLS	    agg_eqcls;	/* eqcls of the aggregate result */
	bool		    check_constant; /* for rare special case of correlated group by
				    ** aggregate subselect, the link back attributes are
				    ** not available */

	check_constant= FALSE;
	agg_ordering = OPE_NOEQCLS;
	abase = subquery->ops_attrs.opz_base;
	sort_count = 0;
	if (grvp->opv_gquery->ops_agg.opa_fbylist)
	    byexprp = grvp->opv_gquery->ops_agg.opa_fbylist;
	else
	    byexprp = grvp->opv_gquery->ops_byexpr;
	for (;
	    byexprp && (byexprp->pst_sym.pst_type == PST_RESDOM); 
	    byexprp = byexprp->pst_left)
	{
	    OPZ_IATTS	    attr;
	    OPZ_DMFATTR	    dmfattr;

	    dmfattr = byexprp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    attr = opz_findatt(subquery, orig_cop->opo_union.opo_orig, dmfattr);
	    if (attr < 0)
	    {
		sort_count=0;	    /* this part of ordering is not used */
		continue;
	    }
	    if (sort_count >= DB_MAXKEYS)
		break;		    /* do not write over stack, exit if
				    ** more than 30 elements in by list */
	    templist.opo_eqorder[sort_count] = 
		abase->opz_attnums[attr]->opz_equcls;
	    if (subquery->ops_bfs.opb_bfeqc
		&& 
		BTtest ((i4)templist.opo_eqorder[sort_count],
		    (char *)subquery->ops_bfs.opb_bfeqc))
	    {
		check_constant = TRUE;
		sort_count--; /* ignore equivalence classes
			** which are constant since they will
			** always be sorted */
	    }
	    sort_count++;
	}
	{
	    OPO_ISORT	    count1;
	    OPO_ISORT	    count2;
	    /* the default sort order is inverted from this list in OPC
	    ** so invert the list to be used for ordering, FIXME there is
	    ** a build in assumption that OPC/QEF will sort the resdoms
	    ** in this order */
	    for (count1 = sort_count-1, count2=0; 
		count1 > count2; count1--, count2++)
	    {
		OPO_ISORT	    save;
		save = templist.opo_eqorder[count1];
		templist.opo_eqorder[count1] = templist.opo_eqorder[count2];
		templist.opo_eqorder[count2] = save;
	    }
	}
	for (byexprp = grvp->opv_gquery->ops_root->pst_left;
	    byexprp && (byexprp->pst_sym.pst_type == PST_RESDOM); 
	    byexprp = byexprp->pst_left)
	{   /* find set of attributes which are aggregate results, since
	    ** they are unique with respect to the by list, they can be
	    ** considered sorted, FIXME, - for now do not consider aggregates
	    ** evaluated together, just get the first AOP value since otherwise
	    ** a "set of eqcls" could be used and the rest of the optimizer should
	    ** be able to handle it, but it is not clear this is true at this time.
	    */
	    if (byexprp->pst_right->pst_sym.pst_type == PST_AOP)
	    {
		OPZ_IATTS	    attr1;
		OPZ_DMFATTR	    dmfattr1;

		dmfattr1 = byexprp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		attr1 = opz_findatt(subquery, orig_cop->opo_union.opo_orig, dmfattr1);
		if (attr1 >= 0)
		{
		    agg_eqcls = abase->opz_attnums[attr1]->opz_equcls;
		    if (sort_count == 0)
		    {   /* if all by list elements are constants then this
			** implies there is only one partition so that this
			** aggregate result is also single valued */
			if (check_constant)
			{
			    BTset((i4)agg_eqcls, (char *)subquery->ops_bfs.opb_bfeqc);
			    BTclear((i4)agg_eqcls, (char *)subquery->ops_bfs.opb_mbfeqc);
			    subquery->ops_bfs.opb_mask |= OPB_SUBSEL_CNST;
							/* indicate a subselect-
							** generated constant */
			}
		    }
		    else if (!subquery->ops_bfs.opb_bfeqc
			    || 
			    !BTtest ((i4)agg_eqcls,
				(char *)subquery->ops_bfs.opb_bfeqc))
			break;
		}
	    }
	}
	if ((sort_count < DB_MAXKEYS)
	    &&
	    byexprp 
	    && 
	    (byexprp->pst_sym.pst_type == PST_RESDOM))
	    templist.opo_eqorder[sort_count++] = agg_eqcls;
	if (sort_count == 1)
	    agg_ordering = templist.opo_eqorder[0];
	else if (sort_count > 1)
	{
	    /* multi-attribute can provide a useful ordering */
	    i4		    size;
	    OPO_EQLIST	    *sortp1;
	    /* create an ordering for this storage structure */
	    size = sizeof(OPO_ISORT) * (sort_count + 1);
	    sortp1 = (OPO_EQLIST *)opu_memory(subquery->ops_global, (i4)size);
	    MEcopy( (PTR)&templist, size, (PTR)sortp1);
	    sortp1->opo_eqorder[sort_count] = OPE_NOEQCLS;
				    /* terminate list */
	    agg_ordering = opo_cordering(subquery, sortp1, FALSE); /* find
				** the exact ordering associated with this
				** list */
	}
	if (agg_ordering != OPE_NOEQCLS)
	    /* sortedness is provided by the function aggregate */
	    cop->opo_ordeqc = agg_ordering;
	else
	    cop->opo_storage = DB_HEAP_STORE; /* no ordering provided */
    }
    else if ((orig_cop->opo_storage == DB_BTRE_STORE)
	||
	(orig_cop->opo_storage == DB_ISAM_STORE)
        ||
	(orig_cop->opo_storage == DB_HASH_STORE)
        ||
	(orig_cop->opo_storage == DB_X100CL_STORE)
        ||
	(orig_cop->opo_storage == DB_X100IX_STORE)
        ||
	(cop->opo_storage == DB_SORT_STORE)) /* exact keys can convert
					** ordering into a sorted order */
    {	/* both BTREE and ISAM provide some ordering, with the difference
        ** being that BTREE will be converted into DB_SORT_STORE, since
        ** the ordering can be guaranteed, whereas ISAM will be
        ** considered DB_HEAP_STORE, which can be used for partial
        ** sort merges */
	OPB_SARG            sargtype;   /* not used */
	OPO_BLOCKS	    pblk;       /* not used */
	OPH_HISTOGRAM       *hp;        /* not used */
	bool		    pt_valid;   /* not used */
	OPO_ISORT	    ordering;   /* this is the ordering which can be
					** assumed after keying has been done in
					** the relation, - OPE_NOEQLCS if no
					** ordering can be assumed */
	bool		    imtid;      /* TRUE if implicit tid equality has
                                        ** been found, so ordering will be
					** on TID */
	OPO_BLOCKS	    dio;	/* dummy variable for opn_prcost */
	OPO_CPU		    cpu;	/* dummy variable for opn_prcost */
	
	(VOID) opn_prcost (subquery, orig_cop, &sargtype, &pblk, &hp, &imtid,
	    TRUE, &ordering, &pt_valid, &dio, &cpu); /* only calculate 
					** the ordering which
					** exists after keying */
	cop->opo_ordeqc = ordering;
	if ((	(orig_cop->opo_storage != DB_HASH_STORE) /* hash may only introduce
					** orderings if constant look ups are used
                                        ** otherwise no ordering should be
					** considered */
		&&
		!imtid
					/* exit if implicit TID key since
                                        ** ordering will be on this rather
					** than BTREE key */
	    )
	    &&
	    (	(orig_cop->opo_storage != DB_ISAM_STORE)
		||
		(ordering == OPE_NOEQCLS)
					/* at least part of the key was used
                                        ** in an exact ordering so do not
                                        ** override with the entire key since
                                        ** DB_SORT_STORE would not be TRUE for
					** the entire key */
	    )
	    &&
	    (	!(varp->opv_grv->opv_relation->rdr_parts)))
	{
	    if (varp->opv_mbf.opb_count == 1)
	    {
		cop->opo_ordeqc = subquery->ops_attrs.opz_base->
		    opz_attnums[varp->opv_mbf.opb_kbase->
			opb_keyorder[0].opb_attno]->opz_equcls; /* get the
						** equivalence class associated
						** with the single attribute
						** ordering */
                if (subquery->ops_bfs.opb_bfeqc
                    &&
                    BTtest((i4)cop->opo_ordeqc,
                        (char *)subquery->ops_bfs.opb_bfeqc))
                    cop->opo_ordeqc = OPE_NOEQCLS;
	    }
	    else 
	    {   /* ordering was already created previously */
                if(!varp->opv_mbf.opb_prmaorder)
                {
                    opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
                        &varp->opv_mbf.opb_prmaorder, FALSE, TRUE); /* get the
                                                ** equivalence class list for the
                                                ** ordering */
                }
                cop->opo_ordeqc = opo_cordering(subquery,
                    varp->opv_mbf.opb_prmaorder, FALSE);
	    }
	    if ((orig_cop->opo_storage == DB_BTRE_STORE ||
		 orig_cop->opo_storage == DB_X100CL_STORE ||
		 orig_cop->opo_storage == DB_X100IX_STORE)
		&&
		(cop->opo_ordeqc != OPE_NOEQCLS))
		cop->opo_storage = DB_SORT_STORE;  /* mark node as sorted on the
						** ordering equivalence class if we
						** can guarantee it */
	}
        if (cop->opo_ordeqc == OPE_NOEQCLS)
            cop->opo_storage = DB_HEAP_STORE;
        else
	{
	    OPE_IEQCLS	    ordeqc;
	    OPE_IEQCLS	    maxeqcls;

	    maxeqcls = subquery->ops_eclass.ope_ev;
	    ordeqc = cop->opo_ordeqc;
	    if (ordeqc < maxeqcls)
	    {
		if (!BTtest((i4)ordeqc, (char *)&cop->opo_maps->opo_eqcmap))
		    cop->opo_ordeqc = OPE_NOEQCLS; /* do not return ordering
						** if it is not available in
						** output of CO node */
	    }
	    else
	    {	/* this is a multi-attribute ordering so check that all
		** attributes are available for the parent */
		OPO_SORT	    *sortp;
		sortp = subquery->ops_msort.opo_base->opo_stable[ordeqc-maxeqcls];
		if (!BTsubset((char *)sortp->opo_bmeqcls, (char *)&cop->opo_maps->opo_eqcmap,
		     (i4)maxeqcls))
		{   /* need to remove equivalence classes not available to
		    ** the parent, but skipping over the constant equivalence
		    ** classes which are not available since these should not
		    ** effect the ordering */
		    OPO_EQLIST		   *keylist;
		    OPE_IEQCLS		   first_eqcls;
		    i4			   att_count;
		    OPE_IEQCLS		   *masterp;
		    OPE_IEQCLS		   *eqclsp;

		    keylist = (OPO_EQLIST *)NULL;
		    att_count = 0;
		    for (masterp = &sortp->opo_eqlist->opo_eqorder[0];
			*masterp != OPE_NOEQCLS; masterp++) 
		    {
			if (subquery->ops_bfs.opb_bfeqc
			    &&
			    BTtest((i4)(*masterp), (char *)subquery->ops_bfs.opb_bfeqc))
			    continue;	    /* if this eqcls is part of a constant
					    ** class then we can ignore it and
					    ** continue , b41939 */
			if (!BTtest((i4)(*masterp), (char *)&cop->opo_maps->opo_eqcmap))
				break;	    /* the remainder of the
					    ** ordering is not useful to the
					    ** parent since this eqcls is not
					    ** returned */
			att_count++;
			if (att_count == 1)
			    first_eqcls = *masterp;	    /* only one eqcls is found */
			else
			{
			    if (att_count == 2)
			    {   /* allocate memory for multi-attribute ordering */
				i4			   maxelement;
				maxelement = BTcount((char *)sortp->opo_bmeqcls, (i4)maxeqcls);
				keylist = (OPO_EQLIST *)opu_memory(subquery->ops_global, 
				    (i4) (sizeof(OPO_ISORT) * (maxelement+1))); /* FIXME - 
								** for now place this into
								** non-enumeration memory, it is safer */
				eqclsp = &keylist->opo_eqorder[0];
				*(eqclsp++)=first_eqcls;
			    }
			    *(eqclsp++) = *masterp; /* this eqcls is useful so
						    ** transfer to the ordering */
			}
		    }
		    if (att_count == 0)
		        /* more than one eqcls in the final ordering */
			cop->opo_ordeqc = OPE_NOEQCLS;
		    else if (att_count == 1)
		    	/* exactly one eqcls left */
			cop->opo_ordeqc = first_eqcls;
		    else
		    {
		    	/* a multi-attribute ordering still exists */
			*(eqclsp++) = OPE_NOEQCLS;
			cop->opo_ordeqc = opo_cordering(subquery, keylist, FALSE);
		    }
		}
	    }
	}
    }
}

/*{
** Name: opo_orig	- create key order
**
** Description:
**      Create an equivalence class descriptor for the keying attributes.
**      This is an ordered list of equivalence classes.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to DB_ORIG node
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
**      23-feb-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
VOID
opo_orig(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    if (cop->opo_sjpr == DB_ORIG)
    {
	opo_FDorig(subquery, cop);	    /* check for func dependencies */
	opo_uorig(subquery, cop);	    /* ... and record uniquenesses */
    }

    if
    (
	cop->opo_sjpr == DB_ORIG
	&&
	cop->opo_ordeqc >= 0
    )
    {
	OPV_VARS               *varp;       /* pointer to range variable
					    ** element associated with leaf */
	OPZ_IATTS		attno;

	varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];

	if (varp->opv_mbf.opb_count == 1 && varp->opv_mbf.opb_kbase &&
	    (attno = varp->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno) >= 0)
	{
	    cop->opo_ordeqc = subquery->ops_attrs.opz_base->
		opz_attnums[attno]->opz_equcls; /* get the equivalence class 
					    ** associated with the single 
					    ** attribute ordering */
	}
	else 
	{   /* ordering was already created previously */
	    if(!varp->opv_mbf.opb_maorder)
	    {
		opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
                    &varp->opv_mbf.opb_maorder, FALSE, FALSE); /* get the equivalence
					    ** class list for the ordering */
	    }
	    cop->opo_ordeqc = opo_cordering(subquery, 
		varp->opv_mbf.opb_maorder, FALSE);
	}
    }
    if (cop->opo_sjpr == DB_ORIG)
	opo_clustorig(subquery, cop);	    /* now do clusterings, too */
}

/*{
** Name: opo_rsort	- sort attributes in the given order
**
** Description:
**      This is a sort node placed in the query plan.  Currently,
**      it will be used to process only the top sort node, but
**      could latter be used to describe other sorts, used to
**      flatten queries, and evaluate aggregates.  Sort nodes
**      which are used for joins are not specified by this node
**      since the join node itself specifies whether a sort needs
**      to be performed. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to top sort node
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
**      23-feb-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
VOID
opo_rsort(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
#ifdef OPM_FLATAGGON
    if (subquery->ops_bestco == cop)
#endif
	cop->opo_ordeqc = OPE_NOEQCLS;	    /* FIXME - currently top sort
					    ** nodes are not in the cost
					    ** model, need to fix this */

    if (cop->opo_sjpr == DB_RSORT)
    {
	opo_clustpr_rsort(subquery, cop);   /* propagate clusters */
	opo_FDpr_rsort(subquery, cop);	    /* ... and FDs */
	opo_upr_rsort(subquery, cop);	    /* ... and uniquenesses */
    }
}

/*{
** Name: opo_kordering	- produce a key ordering
**
** Description:
**      Given the available equivalence classes from the outer determine 
**      a key ordering which can be used.
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      eqcmap                          ptr to available equivalence classes
**                                      from the outer
**      mbfp                            ptr to description of the key
**
** Outputs:
**	Returns:
**	    OPO_ISORT - key ordering of OPE_NOEQCLS if no keying can be done
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-mar-88 (seputis)
**          initial creation
**	5-jan-97 (inkdo01)
**	    Added check for single-valued subselect constant eqclass and
**	    1st col covered only by constant eqclass. Fixes bug 87509.
**	23-dec-04 (inkdo01)
**	    Mixed collation eqc's can't be used for key joins.
**	17-Jun-08 (kibro01) b111578
**	    Extend fix for 87509 to include hash table indices where we
**	    have to be able to guarantee every part of the index
**	20-jan-2009 (dougi)
**	    Support DB_X100CL_STORE as another pre-sorted structure.
**	22-jan-2010 (dougi)
**	    And DB_X100IX_STORE is also sort of the same.
*/
OPO_ISORT
opo_kordering(
	OPS_SUBQUERY       *subquery,
	OPV_VARS           *varp,
	OPE_BMEQCLS        *eqcmap)
{
    i4                  count;	    /* number of attributes from outer which
                                    ** can be used in the key */
    i4                  mincount;   /* minimum number of attributes which
                                    ** need to be found to make keying
                                    ** allowable */
    OPO_ISORT           *sortp;     /* ptr to current equivalence class in
                                    ** key list */
    bool		useless;    /* TRUE if a constant equivalence class
				    ** is found in the keylist but not
				    ** found in the eqcmap */
    switch (varp->opv_tcop->opo_storage)
    {
    case DB_HEAP_STORE:
	return(OPE_NOEQCLS);	    /* heap cannot be used for keying so this
                                    ** will be a physical join, i.e. cartesean
                                    ** product */
    case DB_RTRE_STORE:
    {	/* check first if this is the inner of a possible spatial join */
	/* Note that this doesn't deal with the possibility of several 
	** base tabs which all spatial join to the same inner. In such a 
	** case, there would be several spatial join eqc's associated
	** with the current inner, and this little loop would always
	** pick the first, regardless of cost characteristics. */
	OPE_IEQCLS	eqc;
	for (eqc = -1; (eqc = BTnext((i4)eqc, (char *)eqcmap,
		subquery->ops_eclass.ope_ev)) >= 0;)
	 if (subquery->ops_eclass.ope_base->ope_eqclist[eqc]->ope_mask
		& OPE_SPATJ && BTtest((i4)eqc, (char *)&varp->opv_maps.
		opo_eqcmap))	    /* got a spatial join eqclass in the
				    ** current variable - use this order */
	  return(eqc);
    }	/* if no match, let it drop through (though that oughtn't to happen) */

    case DB_BTRE_STORE:
    case DB_ISAM_STORE:
    case DB_SORT_STORE:
    case DB_X100CL_STORE:
    case DB_X100IX_STORE:
    {	/* any prefix of the available equivalence classes can be used */
	mincount = 1;		    /* at least one equivalence class
                                    ** must match */
	break;
    }
    case DB_HASH_STORE:
    {
	mincount = varp->opv_mbf.opb_mcount; /* all the attributes in the
                                    ** key must be found */
	break;
    }
    default:
	return(OPE_NOEQCLS);        /* FIXME - need consistency check */
    }

    if(!varp->opv_mbf.opb_maorder)
    {
	opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
            &varp->opv_mbf.opb_maorder, FALSE, FALSE); /* get the equivalence
				    ** class list for the ordering */
    }
    if (varp->opv_mbf.opb_maorder->opo_eqorder[0] == OPE_NOEQCLS)
	return(OPE_NOEQCLS);	    /* if no ordering of equivalence
                                    ** classes exists, then keying cannot be
                                    ** used */

    {
	bool		found;	    /* TRUE if attribute is from outer set of
				    ** equilvance classes */
	bool		key_useful; 

	count = 0;
	useless = FALSE;
	key_useful = FALSE;
	for(sortp = &varp->opv_mbf.opb_maorder->opo_eqorder[0];
	    (*sortp != OPE_NOEQCLS) 
	    && 
	    (   (found = BTtest((i4)*sortp, (char *)eqcmap)) /* check the number of attributes
					** which exist in the key from the outer*/
		||
		(
		    subquery->ops_bfs.opb_bfeqc /* or check if the attribute
					** is a constant value */
		    &&
		    BTtest((i4)*sortp, (char *)subquery->ops_bfs.opb_bfeqc)
		)
	    );
	    sortp++)
	{
	    if (subquery->ops_eclass.ope_base->ope_eqclist[*sortp]->
			ope_mask & OPE_MIXEDCOLL)
		continue;	    /* skip eqc's that are mixed collations */

	    key_useful |= found;
	    if (!found)
	    {
		/* if 1st key is only covered by a constant, AND there's a 
		** subselect constant, we can't guarantee the 1st col is covered
		** (87509)
		** Extend fix for 87509 to include hash table indices where we
		** have to be able to guarantee every part of the index
		** (kibro01) b111578
		*/
		if ((count == 0 || varp->opv_tcop->opo_storage == DB_HASH_STORE)
			 && subquery->ops_bfs.opb_mask & OPB_SUBSEL_CNST)
				return(OPE_NOEQCLS);

		useless = TRUE;	    /* at least one eqcls was not in the bitmap
				    ** which means it was available only because
				    ** it was a constant equivalence class */
	    }
	    count++;		    /* add number of eqcls found */
	}
	if (!key_useful)
	    return( OPE_NOEQCLS);   /* key join is made only of constant equivalence
				    ** classes and none are from the outer */
    }

    if (count >= mincount)
    {
	if (count == 1)
	    return((OPO_ISORT)varp->opv_mbf.opb_maorder->opo_eqorder[0]);/*
				    ** only one eqcls so return it */
	if ((count == varp->opv_mbf.opb_mcount)
	    &&
	    !useless)
	    return(opo_cordering(subquery, varp->opv_mbf.opb_maorder, FALSE)); /*
				    ** all the attributes are used so the
                                    ** existing list is applicable */
	{   /* partial key is used so need to allocate a new list */
	    OPO_EQLIST	    *newlist;
	    opo_makey(subquery, &varp->opv_mbf, count,
                &newlist, TRUE, FALSE);         /* get the equivalence
				    ** class list for the ordering */
	    return(opo_cordering(subquery, newlist, FALSE)); /* FIXME -wasted 
                                    ** memory, see if newlist exists first */
	}

    }
    else
	return(OPE_NOEQCLS);
}

/*{
** Name: opo_aggregate_ordering	- get the ordering from an aggregate subquery
**
** Description:
**      Find the key ordering if any on a given aggregate subquery and return
**      the equivalence class ordering for this in terms of equivalence classes
**      known to the outer subquery.
**
**	This function expects the
**
**      Future development:
**      Currently this function expects the aggregate subquery to be on
**      a BTREE file with at least one column ordering. Other ordered input
**      could be used with appropriate checks.
**
** Inputs:
**      outer_sq		ptr to outer subquery variable
**      aggr_sq			ptr to aggregate subquery variable
**      outer_varp		ptr to var used by outer subquery for the
**				aggregate input
**
** Outputs:
**	Returns:
**	    OPO_ISORT - eqc ordering, or OPE_NOEQCLS if none identified.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Dec-10 (smeke01) b124613
**          Initial creation
[@history_line@]...
[@history_template@]...
*/
static OPO_ISORT
opo_aggregate_ordering(
	OPS_SUBQUERY *outer_sq,
	OPS_SUBQUERY *aggr_sq,
	OPV_VARS *outer_varp)
{
    OPV_VARS *aggr_varp;
    OPO_EQLIST eqlist;
    PST_QNODE *qnodep;
    OPO_ISORT ordering;
    i4 i;

    /* The aggregate subquery must be an OPS_RFAGG or OPS_FAGG on a
    ** BTREE table with at least one column ordering */
    if ( !((aggr_sq->ops_sqtype == OPS_RFAGG) || (aggr_sq->ops_sqtype == OPS_FAGG)) ||
	!aggr_sq->ops_bestco->opo_outer ||
	aggr_sq->ops_bestco->opo_outer->opo_storage != DB_BTRE_STORE ||
	aggr_sq->ops_bestco->opo_storage != DB_SORT_STORE ||
	!(aggr_varp = aggr_sq->ops_vars.opv_base->opv_rt[aggr_sq->ops_bestco->opo_outer->opo_union.opo_orig]) ||
	!aggr_varp->opv_mbf.opb_count)
    {
	return OPE_NOEQCLS;
    }

    for (i = 0; i < DB_MAXKEYS; i++)
	eqlist.opo_eqorder[i] = OPE_NOEQCLS;

    /* Run through the sort order ...  */
    for(i = 0; i < aggr_varp->opv_mbf.opb_count && i < DB_MAXKEYS - 1; i++)
    {
	PST_QNODE *qnodep;
	OPZ_IATTS aggr_attno;

	/* ... grabbing the attno from the sort order ... */
	aggr_attno = aggr_varp->opv_mbf.opb_kbase->opb_keyorder[i].opb_attno;

	/* ... running down through the aggregate target list ...  */
	for (qnodep = aggr_sq->ops_root->pst_left; qnodep->pst_left;
	    qnodep = qnodep->pst_left)
	{
	    if (qnodep->pst_sym.pst_type == PST_TREE)
		break;

	    if (qnodep->pst_sym.pst_type != PST_RESDOM ||
		!qnodep->pst_right ||
		!(qnodep->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT) ||
		qnodep->pst_right->pst_sym.pst_type != PST_VAR)
		continue;

	    /* ... looking for the first VAR that has a matching attno. ... */
	    if (qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == aggr_attno)
	    {
		i4 j, outer_av;
		OPZ_IATTS outer_attno;
		OPZ_ATTS *outer_attrp;
		OPZ_AT *outer_abase;

		outer_abase = outer_sq->ops_attrs.opz_base;
		outer_av = outer_sq->ops_attrs.opz_av;

		/* ... Grab the ntargno from the associated resdom node to use
		** to identify the attribute in the outer subquery's attribute table ... */
		outer_attno = qnodep->pst_sym.pst_value.pst_s_rsdm.pst_ntargno;

		/* ... run through the attributes table for the outer subquery ... */
		for (j = 0; j < outer_av; j++)
		{
		    outer_attrp = outer_abase->opz_attnums[j];
		    /* ... looking for a match on var and db_att_id ... */
		    if (outer_varp->opv_gvar == outer_attrp->opz_gvar &&
			outer_attno == outer_attrp->opz_attnm.db_att_id)
		    {
			/* ... if match, store the eqc number from the outer
			** attribute table in the correct sort position in
			** the eqc ordering array */
			eqlist.opo_eqorder[i] = outer_attrp->opz_equcls;
			break;
		    }
		}
		break;
	    }
	}
    }

    if (eqlist.opo_eqorder[0] == OPE_NOEQCLS)
	ordering = OPE_NOEQCLS;
    else
    {
	if (eqlist.opo_eqorder[1] == OPE_NOEQCLS)
	{
	    /* Single attribute ordering - just use the eqc number */
	    ordering = eqlist.opo_eqorder[0];
	}
	else
	{
	    /* Multi attribute ordering - find or create an ordering
	    ** number for it */
	    ordering = opo_cordering(outer_sq, &eqlist, TRUE);
	}
    }

    return ordering;
}
