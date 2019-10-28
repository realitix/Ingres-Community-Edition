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
#define        OPV_PCJNBL      TRUE
#define        OPV_IJNBL      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

static int const0 = 0;

/**
**
**  Name: OPVIJNBL.C - determine if a relations are connected by joins
**
**  Description:
**      Routines will determine if relations are connected by joins
**      and update the opv_joinable matrix.  The equivalence class map
**      for the relations are also setup.
**
**  History:
**      8-may-86 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	7-apr-04 (inkdo01)
**	    Added opv_pcjnbl for partitioned table/parallel query processing.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
static void opv_connected(
	OPS_SUBQUERY *subquery,
	OPV_MVARS *connect,
	OPV_IVARS var,
	OPV_BMVARS *ignore);
static void opv_jtables(
	OPS_SUBQUERY *subquery);
static void opv_ntconnected(
	OPS_SUBQUERY *subquery,
	OPV_MVARS *connect,
	i4 *part_count,
	OPV_BMVARS *ignore);
static bool opl_ojcartprod(
	OPS_SUBQUERY *subquery,
	OPL_OUTER *outerp,
	OPV_BMVARS *tvmap);
static void opv_spjoin(
	OPS_SUBQUERY *subquery);
void opv_pcjnbl(
	OPS_SUBQUERY *subquery);
void opv_ijnbl(
	OPS_SUBQUERY *subquery);

/*{
** Name: opv_connected	- add new variables to the connected map
**
** Description:
**      This routine is called recursively when all the variables connected
**      to the selected var can be added to the connected map
**
** Inputs:
**	subquery			ptr to subquery being analyzed
**      var                             variable to add to the connected
**                                      map of relations
**	ignore				if not NULL then this is a map of
**					relations which should not participate
**					in the connected calculations
**
** Outputs:
**      connect                         ptr to map of all relations which
**                                      are currently connected
**                                      updated with all new variables that
**                                      "var" is connected to
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-may-86 (seputis)
**          initial creation
**	5-jun-91 (seputis)
**	    fix for problem involving relations with no TIDs
[@history_line@]...
*/
static VOID
opv_connected(
	OPS_SUBQUERY	   *subquery,
	OPV_MVARS	   *connect,
	OPV_IVARS          var,
	OPV_BMVARS	   *ignore)
{
    OPV_IVARS           joinvar;    /* variable which will be tested against
                                    ** the current var for joinability
                                    */
    OPV_VARS            *var_ptr;   /* ptr to current var which will be used
                                    ** to attempt to include other vars in
                                    ** the connected set
                                    */

    var_ptr = subquery->ops_vars.opv_base->opv_rt[var];
    for ( joinvar = subquery->ops_vars.opv_prv; joinvar--;)
    {
	if (var_ptr->opv_joinable.opv_bool[joinvar]
	    &&
	    (!connect->opv_bool[joinvar]))
	{   /* a new variable has been found which is joinable so use
            ** recursion to find all variables which can be joined to it
            */
	    if (ignore
		&&
		BTtest((i4)joinvar, (char *)ignore))
	    {	/* found a relation in the set to be ignored */
		if (ignore == subquery->ops_keep_dups)
		    subquery->ops_mask |= OPS_NTCARTPROD; /* mark this flag
				    ** which indicates this condition
				    ** occurred */
		continue;
	    }
	    connect->opv_bool[joinvar] = TRUE;
	    opv_connected( subquery, connect, joinvar, ignore);
	}
    }
}

/*{
** Name: opv_jtables	- initialize the joinable tables
**
** Description:
**      Init the arrays indicating which tables are joinable to one another
**      directly.
**
** Inputs:
**      subquery                        subquery being analyzed
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
**      8-jun-87 (seputis)
**          initial creation
**      1-apr-94 (ed)
**          b60125 - cart prod removed incorrectly
[@history_line@]...
[@history_template@]...
*/
static VOID
opv_jtables(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS		rangex;	/* x coordinate - range variable */
    OPV_IVARS		rangey;	/* y coordinate - range variable */
    OPV_RT              *vbase; /* ptr to base of array of ptrs to range
				** variable elements
				*/
    i4                  maxbeqcls; /* number of bytes in equivalence class
                                ** map that may have data - used to reduce
                                ** number of bytes needed to be compared */
    OPE_IEQCLS		maxeqcls; /* number of defined equivalence classes */

    maxeqcls = subquery->ops_eclass.ope_ev;
    if ( (maxbeqcls = (maxeqcls / 8) + 1) > OPE_BITMAPEQCLS)
	maxbeqcls = OPE_BITMAPEQCLS;

    vbase = subquery->ops_vars.opv_base;/* base array of ptr to range variables
   					*/
    for ( rangex = subquery->ops_vars.opv_rv; rangex-- ;)
    {
	OPE_BMEQCLS	*xeqcmap;
	xeqcmap = &vbase->opv_rt[rangex]->opv_maps.opo_eqcmap;
	if (!BTcount((char *)xeqcmap, (i4)maxeqcls))
	    vbase->opv_rt[rangex]->opv_mask |= OPV_NOATTRIBUTES;
	for (rangey = subquery->ops_vars.opv_rv; rangey--;)
	{
	    char               *rangexmap;  /* ptr to map of equivalence classes
					    ** referenced by range var x
					    */
	    char               *rangeymap;  /* ptr to map of equivalence classes
					    ** referenced by range var y
					    */
	    i4		       mapsize;     /* number of bytes left in map to be
					    ** tested for intersection
					    */

	    rangexmap = (char *)xeqcmap;
	    rangeymap = (char *)&vbase->opv_rt[rangey]->opv_maps.opo_eqcmap;
	    for (mapsize = maxbeqcls; mapsize--;)
		/* compare maps to find if there is an intersection - which
		** means that an equivalence class is in common so the
		** relations are joinable
		*/
		if (*rangexmap++ & *rangeymap++)
		{
		    break;		/* equivalence class in common */
		}
	    if (mapsize >= 0)
		vbase->opv_rt[rangex]->opv_joinable.opv_bool[rangey] = TRUE; /*
					** set TRUE if equivalence class is in
					** in common
					*/
	}
    }
}

/*{
** Name: opv_ntconnected	- check for set of connected relations
**
** Description:
**      Check for a set of connected relations, and count the number
**	of non-connected partitions of tables.  If a map of tables to
**	be ignored is supplied, then the test for connectedness will
**	not use those tables
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
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
**      5-jun-91 (seputis)
**          initial creation to correct duplicate semantic problems
**      12-jun-91 (seputis)
**          increment part_count to fix duplicate semantic problem
[@history_template@]...
*/
static VOID
opv_ntconnected(
	OPS_SUBQUERY       *subquery,
	OPV_MVARS          *connect,
	i4                *part_count,
	OPV_BMVARS         *ignore)
{
    OPV_IVARS	    maxvar;
    OPV_IVARS	    nodup_var;

    maxvar = subquery->ops_vars.opv_prv;
    MEfill( maxvar*sizeof(connect->opv_bool[0]), (u_char) 0, (PTR) connect );
    *part_count=0;
    for (nodup_var = -1;
	(nodup_var = BTnext((i4)nodup_var, (char *)subquery->ops_remove_dups,
	    (i4)maxvar))>=0;
	)
    {
	if (!connect->opv_bool[nodup_var])
	{
	    connect->opv_bool[nodup_var] = TRUE;
	    (*part_count)++;
	    opv_connected( subquery, connect,
		nodup_var, subquery->ops_keep_dups);
				    /* check each variable which is joinable
				    ** to range var 0 and use recursion
				    ** until "connect" does not change
				    */
	}
    }
}

/*{
** Name: opl_ojcartprod	- check for cart prod in outer joins
**
** Description:
**      The routine will check for a cart prod in the outer join in
**	order to avoid "no query plan found" errors since the outer join
**      relation placement routines will conflict with the cart prod
**      placement routines in opn_jintersect
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      11-feb-93 (ed)
**          initial creation
[@history_template@]...
*/
static bool
opl_ojcartprod(
    OPS_SUBQUERY	    *subquery,
    OPL_OUTER		    *outerp,
    OPV_BMVARS		    *tvmap)
{
    OPV_IVARS	    maxvar;
    OPV_IVARS	    varno;
    OPV_BMVARS	    excluded;	/* map of variables which
				** should be excluded from
				** the connectedness test */
    OPV_MVARS	    connect;	/* map of variables which
				** are connected to one another
				*/

    MEcopy((PTR)tvmap, sizeof(excluded), (PTR)&excluded);
    BTnot((i4)BITS_IN(excluded), (char *)&excluded);
    maxvar = subquery->ops_vars.opv_prv;
    MEfill( maxvar*sizeof(connect.opv_bool[0]), (u_char) 0, (PTR) &connect );
    varno = BTnext((i4)-1, (char *)tvmap, (i4)maxvar);
    if (varno >= 0)
    {
	connect.opv_bool[varno] = TRUE;
	opv_connected( subquery, &connect, varno, &excluded);
				/* check each variable which is joinable
				** to range var 0 and use recursion
				** until "connect" does not change
				*/
	for (varno=(-1); (varno = BTnext((i4)varno, (char *)tvmap,
	    (i4)maxvar))>=0;)
	{
	    if (!connect.opv_bool[varno])
	    {	/* variable not connected so mark this outer join
		** as requiring a cart prod lower in the plan */
		return(TRUE);
	    }
	}
    }
    return(FALSE);
}

/*{
** Name: opv_spjoin	- look for possible spatial joins
**
** Description:
**      Look for possible spatial joins:
**	- Boolean factor flagged as SPATJ
**	- one of the cols has Rtree index defined on it.
**
**	Build special eqclass & stick in Rtree column and other primary table
**	column. Show Rtree index joinable to other primary table. This triggers
**	later consideration of "key join" from primary to Rtree, then TID join
**	from Rtree to other primary.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
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
**	29-may-96 (inkdo01)
**	    initial creation.
[@history_line@]...
*/
static VOID
opv_spjoin(
	OPS_SUBQUERY       *subquery)
{
    OPB_IBF		bfi;
    OPB_BOOLFACT	*bfp;
    PST_QNODE		*func;		/* ptr to spatial function */
    PST_QNODE		*var1;		/* ptrs to PST_VAR func operands */
    PST_QNODE		*var2;
    PST_QNODE		*var;

    bool		first_time;


    /* This function loops over the Boolean factors, looking for spatial join
    ** tests. The PST_VAR nodes under the spatial functions are used to search
    ** for useful Rtree indexes, and the world unfolds from there */

    for (bfi = 0, bfp = subquery->ops_bfs.opb_base->opb_boolfact[0];
	 bfi < subquery->ops_bfs.opb_bv;
	 bfi++, bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi])
     if (bfp->opb_mask & OPB_SPATJ)	/* "spatfunc(c1, c2) = 1"
					** or "c1 spatfunc c2" */
     {
	func = bfp->opb_qnode;		/* see if function or infix notation */
	if (func->pst_sym.pst_type == PST_BOP &&
	    func->pst_sym.pst_value.pst_s_op.pst_opno ==
	     subquery->ops_global->ops_cb->ops_server->opg_eq)
	 func = func->pst_left;		/* function notation - get left subtr */
        if (func->pst_sym.pst_type != PST_BOP ||
		(var1 = func->pst_left) == NULL ||
		var1->pst_sym.pst_type != PST_VAR ||
	 	(var2 = func->pst_right) == NULL ||
		var2->pst_sym.pst_type != PST_VAR)
	    return;		/* this can't happen - so leave if it does */

	/* We have a spatial join factor. Now look for a useful Rtree. */
	first_time = TRUE;

	for ( ; ; )	/* something to break from */
	{
	/* This loop is executed once with var1, var2 as from above, then a second
	** time with them in reverse order. This allows detection of useful Rtrees
	** on either or both operands of the spatial join function. */

	    OPV_IVARS	rti;
	    OPV_VARS	*varp;

	    for (rti = subquery->ops_vars.opv_prv,
		 varp = subquery->ops_vars.opv_base->opv_rt[rti];
		 rti < subquery->ops_vars.opv_rv; rti++,
		 varp = subquery->ops_vars.opv_base->opv_rt[rti])
	     if (varp->opv_index.opv_poffset == var1->pst_sym.pst_value.pst_s_var.
			pst_vno && varp->opv_mask & OPV_SPATF
					/* curr index is Rtree on var1 basetab */
		&& BTtest((i4)subquery->ops_attrs.opz_base->opz_attnums
		    [var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->
		    opz_equcls, (char *)&varp->opv_maps.opo_eqcmap)
					/* and it indexes var1's column */
		)
	    {
		OPE_IEQCLS	eqc, oldeqc;
		OPZ_IATTS	att2, att3;
		OPZ_ATTS	*att2p;
		OPV_VARS	*var2p;

		/* We have what we came for - a spatial join boolfact with an Rtree
		** secondary on one of the columns. Now create a special eqclass,
		** stuff the var2 column and the var1 Rtree column into it, and
		** set the var1 table's joinable entry for the var2 Rtree */

		var2p = subquery->ops_vars.opv_base->opv_rt[var2->pst_sym.
				pst_value.pst_s_var.pst_vno];
		var2p->opv_joinable.opv_bool[rti] = TRUE;	/* set joinable */
		att2 = var2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
						/* "other" col joinop atno */
		att2p = subquery->ops_attrs.opz_base->opz_attnums[att2];
		oldeqc = att2p->opz_equcls;	/* save eqc - neweqcls overlays it */
		eqc = ope_neweqcls(subquery, att2);
		att2p->opz_equcls = oldeqc;	/* restore "real" eqclass */
		att2p->opz_mask |= OPZ_SPATJ;	/* show sp join involvement */
		att3 = BTnext((i4)-1, (char *)&varp->opv_attrmap,
			sizeof(varp->opv_attrmap));	/* indxd col attrno */
		subquery->ops_attrs.opz_base->opz_attnums[att3]->opz_mask
			|= OPZ_SPATJ;		/* again flag sp join */
		BTset((i4)att3, (char *) &subquery->ops_eclass.ope_base->
				ope_eqclist[eqc]->ope_attrmap);
						/* eqcls gets both joinop attrs */
		BTset((i4)eqc, (char *)&varp->opv_maps.opo_eqcmap);
		BTset((i4)eqc, (char *)&var2p->opv_maps.opo_eqcmap);
						/* add eqc to rt entries */
		subquery->ops_eclass.ope_base->ope_eqclist[eqc]->ope_mask |=
			OPE_SPATJ;		/* flag new eqclass */
	    }
	    if (!first_time) break;		/* termination condition */

	    var = var1; var1 = var2; var2 = var;  /* swap for 2nd iteration */
	    first_time = FALSE;			/* terminate next time */
	}	/* end of var1, var2 loop */
    }	    /* end of boolfact loop */

}	/* end of opv_spjoin */


/*{
** Name: opv_pcjnbl	- look for partitioning compatible joins
**
** Description:
**      Examine range table and joinable arrays for partitioned tables that
**	join one another on their partitioning columns. In addition, the
**	number of logical partitions in one partitioning scheme must be
**	the same or an integral multiple of the other.
**
**	If the conditions are met, a OPV_PCJDESC structure is built for
**	each table, indicating the joins and columns. Such joins have great
**	query optimization potential, since the rows of each partition of
**	one table must join only to rows in the matching partition(s) of
**	the other table. This reduces the size of the operations and
**	allows the possibility of doing the whole join on several threads
**	in parallel.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
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
**	6-apr-04 (inkdo01)
**	    Written for partitioned table support.
**	27-apr-04 (inkdo01)
**	    Minor fix for identification of PC joins.
**	14-Mar-2007 (kschendel) SIR 122513
**	    Remove pc-join flag, need more global indication of
**	    partitioning for pc-agg analysis.
**	29-May-2007 (kschendel) SIR 122513
**	    Allow difference in column length/precision/nullability
**	    if the rule is range or list, as long as the breaks
**	    compare OK.  For hash partitioning, the number of bytes
**	    hashed affects the result, so columns have to be identical
**	    (excluding null indicator byte) for hash.
*/
VOID
opv_pcjnbl(
	OPS_SUBQUERY       *subquery)
{
    OPV_VARS		*var1p, *var2p;
    i4			i, j, k, l, m, n, o;
    i4			len1, len2;
    DB_PART_DEF		*part1p, *part2p;
    DB_PART_DIM		*pdim1p, *pdim2p;
    DB_PART_LIST	*pcol1p, *pcol2p;
    OPZ_ATTS		*att1p, *att2p;
    OPZ_AT		*zbase = subquery->ops_attrs.opz_base;
    OPZ_IATTS		v1cols[32], v2cols[32], col1, col2;
    OPE_IEQCLS		peqcls, eqcs[32];

    OPV_PCJDESC		*pcj1p, *pcj2p;

    bool		nogood, match;


    /* Step 1: Loop over range tables looking for pairs of partitioned
    ** tables that join one another. */
    for (i = 0; i < subquery->ops_vars.opv_prv-1; i++)
    {
	var1p = subquery->ops_vars.opv_base->opv_rt[i];
	if (var1p->opv_grv == NULL ||
	    var1p->opv_grv->opv_relation == NULL ||
	    (part1p = var1p->opv_grv->opv_relation->rdr_parts) == NULL)
	    continue;		/* skip un-partitioned tables */
	for (k = 0; k < part1p->ndims &&
		part1p->dimension[k].distrule != DBDS_RULE_AUTOMATIC; k++);
	if (k < part1p->ndims)
	    continue;		/* skip automatic partitioned tables */

	for (j = i+1; j < subquery->ops_vars.opv_prv; j++)
	{
	    var2p = subquery->ops_vars.opv_base->opv_rt[j];
	    if (var2p->opv_grv == NULL ||
		var2p->opv_grv->opv_relation == NULL ||
		(part2p = var2p->opv_grv->opv_relation->rdr_parts) == NULL)
		continue;	/* skip un-partitioned tables */
	    if (var2p->opv_joinable.opv_bool[i] == FALSE)
		continue;	/* skip if not joined to outer */
	    for (k = 0; k < part2p->ndims &&
		part2p->dimension[k].distrule != DBDS_RULE_AUTOMATIC; k++);
	    if (k < part2p->ndims)
		continue;	/* skip automatic partitioned tables */

	    /* We now have 2 joined partitioned tables.
	    ** Step 2: check for potential partitioning compatibility. */
	    for (k = 0; k < part1p->ndims; k++)
	     for (l = 0; l < part2p->ndims; l++)
	    {
		pdim1p = &part1p->dimension[k];
		pdim2p = &part2p->dimension[l];
		if (pdim1p->distrule != pdim2p->distrule)
		    continue;	/* partitioning types must be same */
		if (pdim1p->ncols != pdim2p->ncols)
		    continue;	/* column counts must be same */
		if (pdim1p->distrule != DBDS_RULE_HASH &&
		    pdim1p->nbreaks != pdim2p->nbreaks)
		    continue;	/* value break counts must be same */
		if (pdim1p->nparts != pdim2p->nparts &&
		    (pdim1p->distrule != DBDS_RULE_HASH ||
		     (pdim1p->nparts > pdim2p->nparts &&
		      pdim1p->nparts % pdim2p->nparts != 0) ||
		     (pdim2p->nparts > pdim1p->nparts &&
		      pdim2p->nparts % pdim1p->nparts != 0)))
		    continue;	/* partitioning cardinalities must be
				** compatible - list, range must be
				** equal, hash must be even multiple */

		/* Step 3: check the join columns and the partitioning
		** columns. */
		for (m = 0; m < 32; m++)
		    v1cols[m] = v2cols[m] = -1;	/* init col arrays */

		for (m = 0, nogood = FALSE; m < pdim1p->ncols && !nogood; m++)
		{
		    pcol1p = &pdim1p->part_list[m];
		    col1 = v1cols[m] = pcol1p->att_number;

		    /* Look for this column in table's ref'ed atts. */
		    for (n = -1; (n = BTnext(n, (char *)&var1p->opv_attrmap,
			subquery->ops_attrs.opz_av)) >= 0; )
			if ((att1p = zbase->opz_attnums[n])->
					opz_attnm.db_att_id == col1)
			    break;
		    if (n < 0)
		    {
			/* Part. col not referenced in query. */
			nogood = TRUE;
			break;
		    }

		    peqcls = att1p->opz_equcls;

		    /* Look in 2nd var for same eqcls. */
		    for (n = -1, match = FALSE; !match &&
			(n = BTnext(n, (char *)&var2p->opv_attrmap,
			    subquery->ops_attrs.opz_av)) >= 0; )
			if ((att2p = zbase->opz_attnums[n])->opz_equcls
				== peqcls)
			    for (o = 0; o < pdim2p->ncols && !match; o++)
			    {
				pcol2p = &pdim2p->part_list[o];
				col2 = v2cols[m] = pcol2p->att_number;
				if ((att2p = zbase->opz_attnums[n])->
				    opz_attnm.db_att_id == col2)
				    match = TRUE;
			    }
		    if (!match)
		    {
			nogood = TRUE;
			break;		/* didn't find match in 2nd var */
		    }

		    /* Insist on identical types outside of nullability.
		    ** This might be too strict, but it's safe and I
		    ** don't really want to think through the implications,
		    ** especially for range.
		    */
		    if (abs(pcol1p->type) != abs(pcol2p->type))
		    {
			nogood = TRUE;
			break;
		    }

		    if (pdim1p->distrule == DBDS_RULE_HASH)
		    {
			/* For hash rule, columns have to be identical
			** (excluding null indicator).  Nullable vs not-
			** nullable of otherwise identical type is OK,
			** thanks to the way hashprep treats things.
			*/
			len1 = (pcol1p->type > 0) ? pcol1p->length :
							pcol1p->length-1;
			len2 = (pcol2p->type > 0) ? pcol2p->length :
							pcol2p->length-1;
			if (len1 != len2 ||
			  pcol1p->precision != pcol2p->precision)
			{
			    nogood = TRUE;
			    break;
			}
		    }

		    /* We now have partitioning columns from each table
		    ** that are joined to each other. Record them in the
		    ** temp structure and get next column. */
		    eqcs[m] = peqcls;
		}

		if (nogood)
		    continue;		/* failed to find match */

		if (pdim1p->distrule != DBDS_RULE_HASH &&
			!(adt_partbreak_compare(subquery->ops_global->ops_adfcb,
				pdim1p, pdim2p)))
		    continue;		/* non-hash partitioning and
					** breaks aren't identical */

		/* If we get here, we have a pair of partition dimensions
		** that map to join predicates on the tables. Build
		** descriptions and add them to the 2 variables. */
		pcj1p = (OPV_PCJDESC *)opu_memory(subquery->ops_global,
		    sizeof(OPV_PCJDESC) + pdim1p->ncols * sizeof(OPE_IEQCLS));
		pcj2p = (OPV_PCJDESC *)opu_memory(subquery->ops_global,
		    sizeof(OPV_PCJDESC) + pdim1p->ncols * sizeof(OPE_IEQCLS));

		pcj1p->jn_dim1 = k;	/* part. dimension no. (this var) */
		pcj1p->jn_dim2 = l;	/* part. dimension no. (other var) */
		pcj1p->jn_var1 = i;	/* this var */
		pcj1p->jn_var2 = j;	/* joined var */
		for (m = 0; m < pdim1p->ncols; m++)
		    pcj1p->jn_eqcs[m] = eqcs[m];  /* equijoin eqcs */
		pcj1p->jn_eqcs[m] = -1;	/* marker at end of list */
		pcj1p->jn_next = var1p->opv_pcjdp;
		var1p->opv_pcjdp = pcj1p;  /* link to var description */

		pcj2p->jn_dim1 = l;
		pcj2p->jn_dim2 = k;
		pcj2p->jn_var1 = j;	/* this var */
		pcj2p->jn_var2 = i;	/* other var */
		for (m = 0; m < pdim1p->ncols; m++)
		    pcj2p->jn_eqcs[m] = eqcs[m];
		pcj2p->jn_eqcs[m] = -1;
		pcj2p->jn_next = var2p->opv_pcjdp;
		var2p->opv_pcjdp = pcj2p;

	    }	/* partition dimension pairs loop */
	}	/* var2p loop */
    }		/* var1p loop */

}


/*{
** Name: opv_x100_joinsetup	- prepare X100 query with join index info
**
** Description:
**	For X100 queries, search for equijoins that map to referential
**	relationships (hence, X100 join indices) and modify query to include
**	join index columns (referenced table __tid__ in referencing table and
**	derived __tid__ in referenced table).
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    resulting query structure is adorned with join index info where
**	    relevant
**
** History:
**	13-jan-2009 (dougi)
**	    Written for X100 support.
**	17-may-2009 (dougi)
**	    Modified to permit iirefrel to be keyed on refingrelid/x - helps
**	    support clustering.
**	23-apr-2010 (dougi)
**	    Changes to make use of refrels even when pairs of cols are not
**	    equal, but are "constant".
**	6-jan-2011 (dougi)
**	    Add join indexes to attr/EQC lists even if not useable in current
**	    subquery. They may be useful in a containing sq.
**	16-feb-2011 (dougi)
**	    Protect against missing rdr_refrel.
*/
VOID
opv_x100_joinsetup(
	OPS_SUBQUERY       *subquery)

{
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPV_VARS	*refedvar, *refingvar, *vtrefingvar;
    DMT_REFREL	*rrarrayp;
    DB_REFREL	*rrp;
    PST_QNODE	*rsdmp, *varp;
    OPS_SUBQUERY *vtsubq;
    OPZ_AT	*abase = subquery->ops_attrs.opz_base;
    OPZ_ATTS	*attrp, *refedattrp, *refingattrp, *vtrefingattrp;
    OPZ_IATTS	refed, refing, vtrefingatt;
    DB_DATA_VALUE tid_dv;
    OPE_EQCLIST *eqcp, *eqcp1;
    OPE_IEQCLS	eqcarray[16], eqcno, saveeqc;
    RDD_KEY_ARRAY *keyptr;
    i4		i, j, k, l, m, n, savemask;
    i2		varcount = subquery->ops_vars.opv_prv;
    DB_STATUS	status = E_DB_OK;
    bool	match, gotone = FALSE, nojoinix, jixavail, vtrefing;


    if (!(subquery->ops_global->ops_gmask & OPS_X100))
	return;					/* only for X100 queries */

    /* Init. tid_dv. */
    tid_dv.db_data = (char *) NULL;
    tid_dv.db_datatype = DB_INT_TYPE;
    tid_dv.db_length = DB_TID8_LENGTH;
    tid_dv.db_prec = 0;
    tid_dv.db_collID = -1;

    /* Loop over range table. Outer loop is referencing table, inner is
    ** referenced, because rdr_refrel array describes relationships that a
    ** table is member of. */
    for (i = 0; i < varcount; i++)
    {
	jixavail = FALSE;
	vtrefing = FALSE;
	refingvar = vbase->opv_rt[i];

	if (refingvar->opv_grv && (vtsubq = refingvar->opv_grv->opv_gquery) &&
	    (vtsubq->ops_sqtype == OPS_HFAGG ||
	     vtsubq->ops_sqtype == OPS_RFAGG ||
	     vtsubq->ops_sqtype == OPS_FAGG))
	{
	    /* Virtual table resulting from aggregation - check further
	    ** to see if grouped on join index. */
	    for (rsdmp = vtsubq->ops_byexpr;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	     if ((varp = rsdmp->pst_right) &&
		varp->pst_sym.pst_type == PST_VAR &&
		varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id >= 0)
	     {
		vtrefingattrp = vtsubq->ops_attrs.opz_base->opz_attnums[varp->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
		if (vtrefingattrp->opz_equcls >= 0 &&
		    (eqcp = vtsubq->ops_eclass.ope_base->
			ope_eqclist[vtrefingattrp->opz_equcls])->ope_mask &
				(OPE_JOINIX | OPE_JOINIXNOREF) &&
		    !(eqcp->ope_mask & (OPE_REFNOJOINIX)))
		{
		    vtrefingvar = vtsubq->ops_vars.opv_base->
					opv_rt[vtrefingattrp->opz_varnm];
		    if (vtrefingvar->opv_grv->opv_relation &&
			vtrefingvar->opv_grv->opv_relation->rdr_refrel)
		    {
			vtrefing = TRUE;
			vtrefingatt = rsdmp->pst_sym.pst_value.
							pst_s_rsdm.pst_rsno;
		    }
		    break;
		}
	     }
	    if (!vtrefing)
		continue;
	}
	else if (refingvar->opv_grv == NULL ||
	    refingvar->opv_grv->opv_relation == NULL ||
	    refingvar->opv_grv->opv_relation->rdr_refrel == NULL ||
	    refingvar->opv_grv->opv_relation->rdr_refrel->refrel_cnt == 0)
	    continue;			/* must be a REAL relation and be
					** referencing in at least 1 refrel */
	jixavail = TRUE;

	/* For each variable, look for vars it joins to. */
	for (j = 0; j < varcount; j++)
	{
	    if (refingvar->opv_joinable.opv_bool[j] == FALSE)
		continue;			/* only want those that join */
	    if (i == j)
		continue;			/* no self-joins */

	    refedvar = vbase->opv_rt[j];
	    if (refedvar->opv_grv == NULL ||
		refedvar->opv_grv->opv_relation == NULL)
		continue;			/* must be a REAL relation */

	    /* These two join - now look for iirefrel between the tables. */
	    if (vtrefing)
		rrarrayp = vtsubq->ops_vars.opv_base->opv_rt[vtrefingattrp->
			opz_varnm]->opv_grv->opv_relation->rdr_refrel;
	    else rrarrayp = refingvar->opv_grv->opv_relation->rdr_refrel;
	    for (k = 0; k < rrarrayp->refrel_cnt; k++)
	    {
		match = FALSE;
		rrp = &rrarrayp->refrel_array[k];
		if (MEcmp((char *)&rrp->dbrr_refedtabid,
		    (char *)&refedvar->opv_grv->opv_relation->rdr_rel->tbl_id,
				sizeof(DB_TAB_ID)) != 0)
		    continue;			/* no match - skip to next */

		/* Tables match this refrel - now check that all pairs of
		** referenced/referencing columns are also in the same EQC. */
		for (l = n = 0, match = TRUE;
			l < rrp->dbrr_colcount && match; l++)
		{
		    refedattrp = refingattrp = (OPZ_ATTS *) NULL;

		    /* Loop over joinop's looking for match. */
		    for (m = 0; m < subquery->ops_attrs.opz_av; m++)
		    {
			attrp = abase->opz_attnums[m];
			if (attrp->opz_varnm == i &&
			    !vtrefing &&
			    attrp->opz_attnm.db_att_id ==
						rrp->dbrr_refingcols[l])
			    refingattrp = attrp;	/* got ref'ing */
			else if (attrp->opz_varnm == i &&
			    vtrefing)
			{
			    /* Must search virtual table for corresponding
			    ** column and hope it matches dbrr_refingcols[]. */
			    for (rsdmp = vtsubq->ops_root->pst_left;
				rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
				rsdmp = rsdmp->pst_left)
			     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
					attrp->opz_attnm.db_att_id &&
				(varp = rsdmp->pst_right)->pst_sym.pst_type ==
					PST_VAR &&
				vtsubq->ops_attrs.opz_base->opz_attnums[varp->
					pst_sym.pst_value.pst_s_var.pst_atno.
					db_att_id]->opz_attnm.db_att_id ==
					rrp->dbrr_refingcols[l])
			     {
				refingattrp = attrp;
				break;
			     }
			}
			else if (attrp->opz_varnm == j &&
			    attrp->opz_attnm.db_att_id ==
						rrp->dbrr_refedcols[l])
			    refedattrp = attrp;	/* got ref'ed */
			if (refedattrp  && refingattrp)
			{
			    /* One of each means this one's covered. Save
			    ** the EQC and leave loop. */
			    eqcarray[l] = refedattrp->opz_equcls;
			    break;
			}
		    }
		    if (refedattrp == (OPZ_ATTS *) NULL ||
			refingattrp == (OPZ_ATTS *) NULL)
			match = FALSE;		/* missing col is no match */
		    else if (refedattrp->opz_equcls != refingattrp->opz_equcls)
		    {
			/* If both columns are "constant", this one doesn't
			** count. */
			if (subquery->ops_bfs.opb_bfeqc &&
			    BTtest((i4)refedattrp->opz_equcls,
				(char *)subquery->ops_bfs.opb_bfeqc) &&
			    BTtest((i4)refingattrp->opz_equcls,
				(char *)subquery->ops_bfs.opb_bfeqc))
			    n++;
			else match = FALSE;	/* otherwise no match */
		    }
		}

		if (match)
		    break;		/* take 1st refrel between tables */
	    }

	    if (match)
	    {
		/* These two have a referential relationship. Adorn plan
		** with info to allow it to be used. */
		jixavail = FALSE;

		if (rrp->dbrr_refingixcol < 0 || n > 0)
		{
		    /* No joinix or constant but unequal keys make
		    ** it unuseable. */
		    nojoinix = TRUE;
		    refing = -1;
		}
		else nojoinix = FALSE;

		/* Mini-hack to avoid incorrect setting of OPE_MORETHANJOIN. */
		saveeqc = attrp->opz_equcls;
		savemask = subquery->ops_eclass.ope_base->ope_eqclist[saveeqc]
					->ope_mask;

		/* Check for pre-existence of attrs. */
		for (l = 0, eqcno = refed = refing = -1;
			l < subquery->ops_attrs.opz_av; l++)
		{
		    attrp = abase->opz_attnums[l];
		    if (attrp->opz_varnm == i && !nojoinix &&
			attrp->opz_attnm.db_att_id == rrp->dbrr_refingixcol)
		    {
			refing = l;
			eqcno = attrp->opz_equcls;
		    }
		    else if (attrp->opz_varnm == j &&
			attrp->opz_attnm.db_att_id == 0)
		    {
			refed = l;
			eqcno = attrp->opz_equcls;
		    }
		}

		/* Add new attrs, as required. */
		if (refing == -1 && !nojoinix)
		    refing = opz_addatts(subquery, i, (vtrefing) ?
			vtrefingatt : rrp->dbrr_refingixcol, &tid_dv);
		if (refed == -1)
		    refed = opz_addatts(subquery, j, 0, &tid_dv);

		subquery->ops_eclass.ope_base->ope_eqclist[saveeqc]->ope_mask =
			savemask;	/* restore original ope_mask */

		/* Add EQC, as required. */
		if (eqcno == -1)
		    eqcno = ope_neweqcls(subquery, refed);

		/* Hook __tid__ (in referenced table) and referencing
		** table join index column into same EQC. We still note
		** joins across referential relationships, even in the
		** absence of join indexes. They allow later detection of
		** functional dependencies and mergejoin potential. */
		refedattrp = abase->opz_attnums[refed];
		if (!nojoinix)
		{
		    refingattrp = abase->opz_attnums[refing];
		    refingattrp->opz_mask |= OPZ_X100JOINIX;
		}
		else refingattrp = (OPZ_ATTS *) NULL;
		refedattrp->opz_mask |= OPZ_X100TID;
						/* flag for cross compiler */
		ope_addeqcls(subquery, eqcno, refed, FALSE);
		if (!nojoinix)
		    ope_addeqcls(subquery, eqcno, refing, FALSE);
						/* add refed/ing col to EQC */
		eqcp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];
		eqcp->ope_mask |= (OPE_JOINIX | OPE_HIST_NEEDED);/* flag EQC */
		if (nojoinix)
		    eqcp->ope_mask |= OPE_REFNOJOINIX;

		if (eqcp->ope_rjrefed == (OPV_BMVARS *) NULL)
		{
		    eqcp->ope_rjrefed = (OPV_BMVARS *)opu_memory(subquery->
			ops_global, 2 * sizeof(OPV_BMVARS)+sizeof(OPE_BMEQCLS));
		    eqcp->ope_rjrefing = &eqcp->ope_rjrefed[1];
		    eqcp->ope_joinrep = (OPE_BMEQCLS *)&eqcp->ope_rjrefing[1];
		    MEfill(2*sizeof(OPV_BMVARS)+sizeof(OPE_BMEQCLS),
					0, (char *)eqcp->ope_rjrefed);
		}
		BTset(i, (char *)eqcp->ope_rjrefing);
		BTset(j, (char *)eqcp->ope_rjrefed);

		if (nojoinix)
		{
		    /* Record referencing table for nojoinix EQCs. */
		    if (eqcp->ope_nojoinix == (OPV_BMVARS *) NULL)
		    {
			eqcp->ope_nojoinix = (OPV_BMVARS *)opu_memory(subquery->
			    ops_global, sizeof(OPV_BMVARS));
			MEfill(sizeof(OPV_BMVARS), 0,
						(char *)eqcp->ope_nojoinix);
		    }
		    BTset(i, (char *)eqcp->ope_nojoinix);
		}

		for (l = 0; l < rrp->dbrr_colcount; l++)
		 if (subquery->ops_bfs.opb_bfeqc && BTtest((i4)eqcarray[l],
				(char *)subquery->ops_bfs.opb_bfeqc))
		    continue;			/* skip constant cols */
		 else BTset(eqcarray[l], (char *)eqcp->ope_joinrep);

		/* Add join index EQC to tables. */
		BTset((i4)eqcno, (char *)&refedvar->opv_maps.opo_eqcmap);

		if (!nojoinix)
		{
		    BTset((i4)eqcno, (char *)&refingvar->opv_maps.opo_eqcmap);
		    refedvar->opv_tcop->opo_ordeqc = eqcno; /* ref'ed table
						** order is now __tid__ */

		    /* Fiddle refed/refing tables' MBF stuff. */
		    refedvar->opv_mbf.opb_count = 1;
		    if (refedvar->opv_mbf.opb_kbase == (OPB_KA *) NULL)
		        refedvar->opv_mbf.opb_kbase =
				(OPB_KA *)opu_memory(subquery->ops_global,
                		(i4)(sizeof(OPB_ATBFLST)));
		    refedvar->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno =
							refed;

		    /* Only update referencing table MBF if it is clustered
		    ** on the current referenced table. */
		    if (vtrefing ||
			(keyptr = refingvar->opv_grv->opv_relation->
					rdr_keys)->key_count == 1 &&
			keyptr->key_array[0] ==
					refingattrp->opz_attnm.db_att_id)
		    {
			refingvar->opv_mbf.opb_count = 1;
			if (refingvar->opv_mbf.opb_kbase == (OPB_KA *) NULL)
		            refingvar->opv_mbf.opb_kbase =
				(OPB_KA *)opu_memory(subquery->ops_global,
                		(i4)(sizeof(OPB_ATBFLST)));
			refingvar->opv_mbf.opb_kbase->
					opb_keyorder[0].opb_attno = refing;
			refingvar->opv_tcop->opo_ordeqc = eqcno;

			/* Since referencing table is keyed on join index
 			** foreign key column, it is a clustered join. Set
			** clustering ID. */
			if (refingvar->opv_x100clid > 0)
			    refedvar->opv_x100clid = refingvar->opv_x100clid;
			else if (refedvar->opv_x100clid > 0)
			    refingvar->opv_x100clid = refedvar->opv_x100clid;
			else
			{
			    refedvar->opv_x100clid = refingvar->opv_x100clid =
					(++subquery->ops_x100clcnt);
			    subquery->ops_mask2 |= OPS_HINTPASS;
			}
						/* clustered join order is
  						** treated like a hint */
		    }
		}

		gotone = TRUE;		/* at least one JOINIX */

		/* Finally - loop over EQCs of mapped columns and flag that
		** they've been replaced by a join index, and if EQC is only
		** there for join, remove from opv_maps. */
		for (l = 0; l < rrp->dbrr_colcount; l++)
		{
		    eqcp = subquery->ops_eclass.ope_base->ope_eqclist[
				(saveeqc = eqcarray[l])];
		    if (subquery->ops_bfs.opb_bfeqc && BTtest((i4)saveeqc,
				(char *)subquery->ops_bfs.opb_bfeqc))
			continue;			/* skip constant cols */
		    eqcp->ope_mask |= OPE_REPJOINIX;

		    /* Identify JOINIX EQCs that replace REPJOINIX EQCs. */
		    if (eqcp->ope_repjoin == (OPE_BMEQCLS *) NULL)
		    {
			eqcp->ope_repjoin = (OPE_BMEQCLS *)opu_memory(
				subquery->ops_global, sizeof(OPE_BMEQCLS));
			MEfill(sizeof(OPE_BMEQCLS), 0,
						(char *)eqcp->ope_repjoin);
		    }
		    BTset(eqcno, (char *)eqcp->ope_repjoin);

		    if (FALSE && !(eqcp->ope_mask & OPE_MORETHANJOIN))
		    {
			BTclear((i4)saveeqc,
				(char *)&refedvar->opv_maps.opo_eqcmap);
			BTclear((i4)saveeqc,
				(char *)&refingvar->opv_maps.opo_eqcmap);
		    }
		}
	    }
	}

	/* Check if there is a join index on current table and it hasn't
	** already been added to attr/EQC list. */
	if (!jixavail || refingvar->opv_grv->opv_relation == (RDR_INFO *) NULL)
	    continue;			/* no join index in the first place */

	if (subquery->ops_sqtype != OPS_FAGG &&
		subquery->ops_sqtype != OPS_RFAGG &&
		subquery->ops_sqtype != OPS_HFAGG)
	    continue;			/* uninteresting sq type */

	rrarrayp = refingvar->opv_grv->opv_relation->rdr_refrel;
	for (k = 0, jixavail = FALSE; k < rrarrayp->refrel_cnt && !jixavail;
				k++)
	{
	    rrp = &rrarrayp->refrel_array[k];
	    if (rrp->dbrr_refingixcol >= 0)
		jixavail = TRUE;
	}

	if (!jixavail)
	    continue;			/* no join index */

	/* Loop over referencing columns to assure they're all
	** in subquery attribute list. Otherwise, referencing subqueries
	** won't need the join index. */
	for (k = 0; k < rrp->dbrr_colcount; k++)
	{
	    /* Loop over joinop's looking for match. */
	    for (l = 0, match = FALSE; l < subquery->ops_attrs.opz_av &&
						!match; l++)
	    {
		attrp = abase->opz_attnums[l];
		if (attrp->opz_varnm == i &&
		    attrp->opz_attnm.db_att_id ==
					rrp->dbrr_refingcols[k])
		{
		    eqcarray[k] = attrp->opz_equcls;
		    match = TRUE;
		}
	    }
	    if (!match)
		break;			/* missed one - quit search */
	}
	if (!match)
	    continue;			/* failed ref'ing column test */

	/* Finally tests succeeded - add attr & EQC for join index column. */
	refing = opz_addatts(subquery, i, rrp->dbrr_refingixcol, &tid_dv);
	eqcno = ope_neweqcls(subquery, refing);
	refingvar->opv_tcop->opo_ordeqc = eqcno;
	BTset((i4)eqcno, (char *)&refingvar->opv_maps.opo_eqcmap);
	BTset((i4)eqcno, (char *)&subquery->ops_eclass.ope_maps.opo_eqcmap);
	eqcp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];
	eqcp->ope_mask |= (OPE_JOINIXNOREF | OPE_HIST_NEEDED);/* flag EQC */
	eqcp->ope_joinrep = (OPE_BMEQCLS *)opu_memory(subquery->ops_global,
				sizeof(OPE_BMEQCLS));
	MEfill(sizeof(OPE_BMEQCLS), 0, (char *)eqcp->ope_joinrep);
	for (k = 0; k < rrp->dbrr_colcount; k++)
	 if (!(subquery->ops_bfs.opb_bfeqc && BTtest((i4)eqcarray[k],
			(char *)subquery->ops_bfs.opb_bfeqc)))
	 {
	    BTset((i4)eqcarray[k], (char *)eqcp->ope_joinrep);
	    eqcp1 = subquery->ops_eclass.ope_base->ope_eqclist[eqcarray[k]];
	    if (eqcp1->ope_repjoin == (OPE_BMEQCLS *) NULL)
	    {
		eqcp1->ope_repjoin = (OPE_BMEQCLS *)opu_memory(subquery->
			ops_global, sizeof(OPE_BMEQCLS));
		MEfill(sizeof(OPE_BMEQCLS), 0, (char *)eqcp1->ope_repjoin);
	    }
	    BTset((i4)eqcno, (char *)eqcp1->ope_repjoin);
	 }
	if (refingvar->opv_mbf.opb_kbase == (OPB_KA *) NULL)
	    refingvar->opv_mbf.opb_kbase = (OPB_KA *)opu_memory(subquery->
			ops_global, (i4)(sizeof(OPB_ATBFLST)));
	refingvar->opv_mbf.opb_count = 1;
	refingvar->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno = refing;
    }
}


/*{
** Name: opv_x100_pagecount	- perform rough page count estimates for X100
**	tables
**
** Description:
**	For X100 queries, estimate page count for each opv_tcop based on known
**	row count and columns in opo_eqcmap.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    all opv_tcop's for X100 tables will have page counts updated
**
** History:
**	15-june-2009 (dougi)
**	    Written for X100 support.
*/
VOID
opv_x100_pagecount(
	OPS_SUBQUERY       *subquery)

{
    OPZ_IATTS	attno;
    OPZ_ATTS	*attrp;
    OPE_IEQCLS	eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPZ_BMATTS	*attrmap;
    i4		i;
    OPO_TUPLES	rowsize;
    OPO_CO	*cop;

    /* Loop over range table looking for X100 tables. For each, accumulate
    ** "row" size by summing ref'ed column sizes. Compute page count,
    ** assuming 16K page size and 5% fudge. */
    for (i = 0; i < subquery->ops_vars.opv_prv; i++)
    {
	cop = subquery->ops_vars.opv_base->opv_rt[i]->opv_tcop;
	if (cop == (OPO_CO *)NULL)
	    continue;

	for (eqcno = -1, rowsize = 0; (eqcno = BTnext(eqcno,
		(char *)&cop->opo_maps->opo_eqcmap, maxeqcls)) >= 0; )
	{
	    attrmap = &subquery->ops_eclass.ope_base->
					ope_eqclist[eqcno]->ope_attrmap;
	    for (attno = -1; (attno = BTnext(attno, (char *)attrmap,
				subquery->ops_attrs.opz_av)) >= 0; )
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
		if (attrp->opz_varnm == i)
		{
		    rowsize += attrp->opz_dataval.db_length;
		    break;
		}
	    }
	}

	if (rowsize > 0)
	    cop->opo_cost.opo_reltotb = rowsize * cop->opo_cost.opo_tups /
				16384;
    }

}


/*{
** Name: opv_ijnbl	- init boolean matrix TRUE if tables connected by joins
**
** Description:
**	Initialize the opv_joinable array. element (i,j) of this array is TRUE
**	iff relation opv_rt[i] is joinable with opv_rt[j].
**      Note that there is not really a matrix defined but a set of arrays
**      which logically form a matrix.
**
**      The bitmaps used to initialize joinable do not contain settings for
**      any multi-variable function attributes.  A query cannot depend on
**      being joined by a multi-variable function.
**
**      This routine also uses the joinable array to determine if the
**      query is connected and sets opv_cart_prod TRUE if it is not.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**      subquery
**          .opv_vars...opv_joinable    initialized to TRUE for every pair
**                                      of relations connected by a join
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-may-86 (seputis)
**          initial creation
**	15-mar-90 (seputis)
**	    fix bug 20632
**	26-dec-90 (seputis)
**	    check whether cart prods would be useful for a multi-attribute key
**	5-jun-91 (seputis)
**	    - fix STAR access violation, and local problem in which nested
**	    subselect with union views could result in a query plan not
**	    found error
**      7-dec-93 (ed)
**          b56139 - create equivalence classes for unused attributes in
**		union views, since OPC requires this
**	26-mar-94 (ed)
**	    bug 59355 - an outer join can change the result of a resdom
**	    expression from non-null to null, this change needs to be propagated
**	    for union view target lists to all parent query references in var nodes
**	    or else OPC/QEF will not allocate the null byte and access violations
**	    may occur
**	9-jul-97 (inkdo01)
**	    fix to avoid setting SVAR refs into ORIG node eqcmap when in
**	    inner table of outer join (screws up null semantics).
**	14-nov-97 (inkdo01)
**	    Slight correction to above to avoid bug 87211 (seems we can
**	    somehow have both OPZ_OJSVAR and opz_ojid == OPL_NOOUTER).
**	16-aug-05 (inkdo01)
**	    Add support of multi-EQC attributes (from OJs).
**	10-mar-06 (hayke02)
**	    Set OPL_OOJCARTPROD in opl_mask if we have a cart prod in the
**	    outer (opl_ovmap) of the outer join. This change fixes bug 115599.
[@history_line@]...
*/
VOID
opv_ijnbl(
	OPS_SUBQUERY       *subquery)
{
    OPV_RT              *vbase; /* ptr to base of array of ptrs to range
				** variable elements
				*/
    bool		correlated; /* TRUE if one correlated joinop attbribute
                                ** was found */
    OPE_IEQCLS		maxeqcls;
    OPS_STATE		*global;

    global = subquery->ops_global;
    vbase = subquery->ops_vars.opv_base;/* base array of ptr to range variables
   					*/

    maxeqcls = subquery->ops_eclass.ope_ev;
    correlated = FALSE;

    {
	/* map all equivalence classes associated with the variable */
	OPZ_IATTS              attr;	/* joinop attribute number being
                                        ** analyzed */
	OPZ_AT                 *abase;  /* ptr to base of array of ptrs to
                                        ** joinop attributes
                                        */
        OPZ_FT                 *fbase;  /* ptr to base of array of ptrs to
                                        ** function attribute elements */

        abase = subquery->ops_attrs.opz_base;
        fbase = subquery->ops_funcs.opz_fbase;

	for (attr = subquery->ops_attrs.opz_av; attr--;)
	{
	    OPZ_ATTS           *attrp;	/* ptr to joinop attribute being
                                        ** analyzed */
	    attrp = abase->opz_attnums[attr];
	    if (attrp->opz_func_att != OPZ_NOFUNCATT)
	    {
		OPZ_FATTS	*fap;
		fap = fbase->opz_fatts[attrp->opz_func_att];
		if (global->ops_gmask & OPS_NULLCHANGE)
		{   /* check all var nodes since the underlying temp
		    ** table resdom may have changed to NULL */
		    if (fap->opz_function)
			ope_aseqcls(subquery, (OPE_BMEQCLS *)NULL,
			    fap->opz_function); /* this routine has the
					** side effect of relabelling the
					** var nodes when necessary */
		}
		if (fap->opz_mask & OPZ_OJSVAR && fap->opz_ojid != OPL_NOOUTER)
					/* (not clear how we can have
					** OPL_NOOUTER, but we can and it
					** used to cause bug 87211) */
		{
		    OPL_OUTER	*outerp;
		    outerp = subquery->ops_oj.opl_base->opl_ojt[fap->opz_ojid];
		    if (outerp->opl_idmap == NULL ||
			BTnext((i4)-1, (PTR)outerp->opl_idmap,
			subquery->ops_oj.opl_lv) < 0)
		     fap->opz_mask &= (~OPZ_OJSVAR);
		}
		if (fap->opz_type == OPZ_MVAR || fap->opz_type == OPZ_SVAR
		    && fap->opz_mask & OPZ_OJSVAR)
			continue;	/* do not map multi-variable function
                                        ** attributes since original relations
                                        ** cannot use these for joins. Also
					** leave SVARs with ref to inner var
					** of OJ. Can't eval 'til join itself
                                        */
	    }
	    if (attrp->opz_attnm.db_att_id == OPZ_CORRELATED)
	    {	/* do not include OPZ_CORRELATED attributes yet to determine
		** the joinability, since a cartesean product may be required
                ** to evaluate the correlated subselect
                **    e.g. select r.a from r,s where
		**    r.a > (select t.b from t where r.a = t.c AND s.b=t.d)
                ** - the above query is really cartesean product if it has not
                ** been flattened, but it would be connected by the virtual
                ** table represented the subselect if CORRELATED variables
                ** are allowed */
		/* can add one equivalence class, so that above problem with
		** two equivalence class is avoided, otherwise all SEJOIN queries
		** will be cartesean products */
		OPE_IEQCLS	co_eqcls;
		OPE_BMEQCLS	*co_bmeqcls;
		OPZ_IATTS	maxattr;
		OPV_IVARS	co_var;
		OPZ_IATTS	co_attr;
		OPE_ET		*ebase;

		ebase = subquery->ops_eclass.ope_base;
		co_var = OPV_NOVAR;
		maxattr = subquery->ops_attrs.opz_av;
		co_bmeqcls = &vbase->opv_rt[attrp->opz_varnm]->opv_maps.opo_eqcmap;
		for (co_eqcls = -1; ((co_eqcls = BTnext((i4)co_eqcls, (char *)co_bmeqcls, maxeqcls))>=0)
				    &&
				    (co_var == OPV_NOVAR)
				    ;)
		{
		    OPZ_BMATTS	    *co_bmattr;
		    co_bmattr = &ebase->ope_eqclist[co_eqcls]->ope_attrmap;
		    for (co_attr = -1; (co_attr = BTnext((i4)co_attr, (char *)co_bmattr, maxattr))>=0;)
			if (abase->opz_attnums[co_attr]->opz_varnm != attrp->opz_varnm)
			{
			    if (abase->opz_attnums[co_attr]->opz_attnm.db_att_id != OPZ_CORRELATED)
			    {
				co_var = abase->opz_attnums[co_attr]->opz_varnm;
				break;
			    }
			}
		}
                correlated = TRUE;
		if (co_var != OPV_NOVAR)
		    continue;		/* one equivalence class with another variable
					** only can be connected for the jintersect tests */
	    }
            if (attrp->opz_equcls < 0)
	    {
		if (subquery->ops_gentry >= 0 &&
			global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry]
					->opv_gmask & OPV_UVPROJECT)
		    (VOID)ope_neweqcls(subquery, attr);	/* add equivalence which is
					** not used (so do not add to opv_eqcmp) but
					** is still needed for OPC which assumes all
					** attributes have equivalence classes */
		else
		    opx_error(E_OP0498_NO_EQCLS_FOUND);
	    }
	    else
	    {
		BTset(  (i4)attrp->opz_equcls,
		    (char *)&vbase->opv_rt[attrp->opz_varnm]->opv_maps.opo_eqcmap); /* set
                                        ** equivalence class map associated
                                        ** relation */
		BTor((i4)maxeqcls, (char *)&attrp->opz_eqcmap,
			(char *)&vbase->opv_rt[attrp->opz_varnm]->
						opv_maps.opo_eqcmap);
					/* OR in possible multi-EQCs */
	    }
	}
    }

    if ((subquery->ops_vars.opv_rv <= 1) && (!subquery->ops_enum))
	return;                         /* return if no join to be found */

    /* use the variable equivalence class maps just created to set the
    ** joinable array
    */

    opv_jtables (subquery);		/* init the arrays indicating which
                                        ** tables are joinable */

    if (subquery->ops_bfs.opb_mask & OPB_GOTSPATJ) opv_spjoin(subquery);
					/* fiddle eqclasses, joinables for
					** Rtree-able spatial joins */

    {	/* - check whether a cartesian products exists i.e. is query connected?
        ** using the joinable array
	*/
	OPV_MVARS              connect; /* map of variables which
                                        ** are connected to one another
                                        */
	OPV_IVARS	       maxvar;	/* number of variables in the range
                                        ** table
                                        */

	maxvar = subquery->ops_vars.opv_prv;
	MEfill( maxvar*sizeof(connect.opv_bool[0]),
	    (u_char) 0, (PTR) &connect );
	connect.opv_bool[0] = TRUE;	/* relation is connected to itself */
	opv_connected( subquery, &connect,
	    (OPV_IVARS)0, (OPV_BMVARS *)NULL); 	/* check each variable which
					** is joinable
                                        ** to range var 0 and use recursion
                                        ** until "connect" does not change
                                        */

	if (correlated)
	{
	    /* now map the correlated attributes */
	    OPZ_IATTS              coattr;	/* joinop attribute number being
					    ** analyzed */
	    OPZ_AT                 *abase1;  /* ptr to base of array of ptrs to
					    ** joinop attributes
					    */

	    abase1 = subquery->ops_attrs.opz_base;

	    for (coattr = subquery->ops_attrs.opz_av; coattr--;)
	    {
		OPZ_ATTS           *cattrp;	/* ptr to joinop attribute being
					    ** analyzed */
		cattrp = abase1->opz_attnums[coattr];
		if (cattrp->opz_attnm.db_att_id == OPZ_CORRELATED)
		{
		    BTset(  (i4)cattrp->opz_equcls,
			(char *)&vbase->opv_rt[cattrp->opz_varnm]->opv_maps.opo_eqcmap); /* set
					    ** equivalence class map associated
					    ** relation */
		    BTor((i4)maxeqcls, (char *)&cattrp->opz_eqcmap,
			(char *)&vbase->opv_rt[cattrp->opz_varnm]->
						opv_maps.opo_eqcmap);
					/* OR in possible multi-EQCs */
		    connect.opv_bool[cattrp->opz_varnm] = TRUE; /* if correlated
					** then it will connect to an existing
					** relation */
		}
	    }
	}
	while (maxvar--)		/* check result of connectivity test*/
	{
	    if (!connect.opv_bool[maxvar])
	    {	/* variable which is not connected has been found so
                ** a cartesian product will be produced
                */
		subquery->ops_vars.opv_cart_prod = TRUE;
		break;
	    }
	}
	if (correlated)
	    opv_jtables( subquery );	/* init the joinable arrays again so
                                        ** that opn_jintersect will operate
                                        ** correctly */
	if (global->ops_cb->ops_check
	    &&
	    opt_strace( global->ops_cb, OPT_F052_CARTPROD))
		subquery->ops_vars.opv_cart_prod = TRUE; /* trace flag indicates
					** that cartesean product space space
					** is desired */
    }
    if (subquery->ops_vars.opv_prv >= 3)
    {	/* check whether cart-prods would be useful to fully use a multi-attribute
	** key, e.g. a case in which several small tables are joined to a
	** large table, as in the metaphor tests */
	OPV_IVARS	maxvar1;
	OPV_IVARS	varno;

	maxvar1 = subquery->ops_vars.opv_prv-1;
	for (varno = subquery->ops_vars.opv_rv; --varno >= 0;)
	{
	    /* for each variable, or index, set up the primary variables
	    ** in the partition, except for the primary variable being analyzed
	    ** so that a cart prod can be checked */
	    OPV_VARS	    *varp;
	    bool	    joinable;
	    varp = vbase->opv_rt[varno];
	    if (varp->opv_mbf.opb_mcount > 1)
	    {	/* multi-attribute key found */
		OPN_STLEAVES	    partition;
		OPV_IVARS	    tempvarno;
		OPV_IVARS	    nextvar;
		OPV_IVARS	    skipvar;

		if (varno <= maxvar1)
		    skipvar = varno;
		else
		    skipvar = varp->opv_index.opv_poffset;
		nextvar = 0;
		for (tempvarno = 0; tempvarno <= maxvar1; tempvarno++)
		{
		    if (tempvarno == skipvar)
			continue;
		    partition[nextvar++] = tempvarno;
		}
		(VOID)opn_cartprod(subquery, partition, maxvar1, varno, &joinable);
					/* this routine will set masks in
					** OPV_VARS of varno, to indicate that
					** cartesean products are useful */
	    }
	}

    }
    {	/* use equivalence class maps to determine if special case checking
	** of secondary indexes is required */
	if (subquery->ops_mask & OPS_MULTI_INDEX)
	{   /* if the second index was found then scan all the existing
	    ** indexes to see if any attributes are in common and mark
	    ** both indexes as needing checks to determine that multi-attribute
	    ** join tuple counts are not too restrictive on implicit tid joins */
	    OPV_IVARS	    bvar;	/* base relation to check */

	    for (bvar = subquery->ops_vars.opv_prv; --bvar >=0;)
	    {
		if (vbase->opv_rt[bvar]->opv_mask & OPV_SECOND)  /* check that at least two
					** indexes where defined on this primary */
		{   /* found primary variable so check all indexes against it */
		    OPV_IVARS	    ivar1;  /* first index to check */
		    for (ivar1 = subquery->ops_vars.opv_rv; --ivar1>=0;)
		    {	/* get a map of all the equivalences class which are
			** defined in secondary indexes */
			OPV_VARS	*ivar1p; /* ptr to first index descriptor */
			ivar1p = vbase->opv_rt[ivar1];
			if ((ivar1p->opv_index.opv_poffset == bvar)
			    &&
			    (ivar1 != bvar))
			{   /* found first index, now find second index on this table */
			    OPV_IVARS	    ivar2;	/* second index to check */
			    for (ivar2 = ivar1; --ivar2>=0;)
			    {	/* get a map of all the equivalences class which are
				** defined in secondary indexes */
				OPV_VARS    *ivar2p;	/* ptr to second index descriptor */
				ivar2p = vbase->opv_rt[ivar2];
				if ((ivar2p->opv_index.opv_poffset == bvar)
				    &&
				    (ivar2 != bvar))
				{
				    OPE_BMEQCLS	    eqcmap;
				    MEcopy((PTR)&ivar1p->opv_maps.opo_eqcmap, sizeof(eqcmap),
					(PTR)&eqcmap);
				    BTand((i4)maxeqcls, (char *)&ivar2p->opv_maps.opo_eqcmap,
					(char *)&eqcmap);
				    if (BTcount((char *)&eqcmap, (i4)maxeqcls ) >= 2)
				    {	/* the TID equivalence class is in common, but
					** any more should trigger checks on these indexes */
					ivar1p->opv_mask |= OPV_CBF;
					ivar2p->opv_mask |= OPV_CBF;
					subquery->ops_mask |= OPS_CBF;
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }
    if (subquery->ops_keep_dups
	&&
	(   BTcount((char *)subquery->ops_remove_dups, (i4)subquery->ops_vars.opv_prv)
	    >= 2
	))
    {
	OPV_MVARS           connect2;	/* map of variables which
                                        ** are connected to one another
                                        */

	subquery->ops_mask |= OPS_NOTIDCHECK;	/* need to check placement of relations
					** to avoid plans in which a sort node cannot remove
					** duplicates from the appropriate relations */
	opv_ntconnected(subquery, &connect2, &subquery->ops_pcount, subquery->ops_keep_dups);
	if (subquery->ops_pcount < 2)
	{   /* at least 2 partitions exist which are not connected
	    ** check if these relations are connected to a relation which
	    ** has no TIDs and need duplicates kept */
	    subquery->ops_mask &= (~OPS_NTCARTPROD); /* reset flag so that
					** cart-prods are not checked */
	}
    }
    if (subquery->ops_oj.opl_base)
    {	/* if outer joins exist then check for special case cart prod of
	** inner relations, and mark those outer joins which require
	** special handling in the joinability hueristic */
	OPL_IOUTER	ojid;
	for (ojid = subquery->ops_oj.opl_lv; ojid--;)
	{

	    OPL_OUTER	    *outerp;

	    outerp = subquery->ops_oj.opl_base->opl_ojt[ojid];
	    if (outerp->opl_ivmap
		&&
		opl_ojcartprod(subquery, outerp, outerp->opl_ivmap))
		outerp->opl_mask |= OPL_OJCARTPROD;
	    if (outerp->opl_ovmap
		&&
		opl_ojcartprod(subquery, outerp, outerp->opl_ovmap))
		outerp->opl_mask |= OPL_OOJCARTPROD;
	    if (outerp->opl_bvmap
                &&
                opl_ojcartprod(subquery, outerp, outerp->opl_bvmap))
                outerp->opl_mask |= OPL_BOJCARTPROD;
	}
    }
    {	/* check if constant equivalence classes can be implied from
	** an aggregate */
	OPV_IVARS	varno2;

	for (varno2 = subquery->ops_vars.opv_prv; --varno2 >= 0;)
	{
	    OPV_VARS	    *varp2;
	    OPV_GRV	    *grvp;		/* ptr to global range var descriptor
						** for ORIG node */
	    varp2 = vbase->opv_rt[varno2];
	    grvp = varp2->opv_grv;
	    if (grvp
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
		    attr = opz_findatt(subquery, varno2, dmfattr);
		    if (attr < 0)
		    {
			sort_count=0;	    /* this part of ordering is not used */
			continue;
		    }
		    if (subquery->ops_bfs.opb_bfeqc
			&&
			BTtest ((i4)abase->opz_attnums[attr]->opz_equcls,
			    (char *)subquery->ops_bfs.opb_bfeqc))
		    {
			check_constant = TRUE;
			sort_count--; /* ignore equivalence classes
				** which are constant since they will
				** always be sorted */
		    }
		    sort_count++;
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
			attr1 = opz_findatt(subquery, varno2, dmfattr1);
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
	    }
	}
    }
}
