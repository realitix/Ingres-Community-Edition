/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <cui.h>
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
#define        OPV_PARSER      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPVPARSER.C - init global range table element from parser's table
**
**  Description:
**      Routine to initialize the global range table element from the parser's 
**      table 
**
**  History:  
**      1-jul-86 (seputis)  
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      10-aug-98 (stial01)
**          opv_parser() Remove code to invalidate indexes. The invalidate
**          table that follows will do the job. Invalidate indexes by table id
**          can get E_OP008E_RDF_INVALIDATE, E_RD0045_ULH_ACCESS
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	08-Dec-2010 (kiria01) SIR 124690
**	    Propagate collation ID
**	05-apr-2011 (stial01) m1748
**	    Remove code that was copying rdr_refrel from one RDR_INFO block
**	    to another. (might copy from private to system infoblk)
**	    Don't RDF_INVALIDATE if zero rows (GETDESC now gets updated stats)
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
static i4 opv_tproc(
	OPS_STATE *global,
	OPV_IGVARS gvar,
	OPV_GRV *grvp,
	PST_RNGENTRY *rngentry);
bool opv_parser(
	OPS_STATE *global,
	OPV_IGVARS gvar,
	OPS_SQTYPE sqtype,
	bool rdfinfo,
	bool psf_table,
	bool abort);

/*{
** Name: opv_tproc	- Load RDF defs for table procedure range table entry
**
** Description:
**      This function allocates and formats simulated RDF structures for
**	a table procedure, including result column descriptors that are
**	used to resolve "column" references to the procedure.
**
** Inputs:
**	global		- ptr to global state structure
**	gvar		- index into global range table
**	grvp		- ptr to OPV_GRV of table procedure
**
** Outputs:
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	17-april-2008 (dougi)
**	    Written for table procedures(semi-cloned from pst_stproc).
**	15-dec-2008 (dougi) BUG 121381
**	    Fix computation of result column offsets.
*/
static DB_STATUS
opv_tproc(
	OPS_STATE	*global,
	OPV_IGVARS	gvar,
	OPV_GRV		*grvp,
	PST_RNGENTRY	*rngentry)

{
    DMT_ATT_ENTRY	**attarray, *attrp;
    DMT_ATT_ENTRY	**parmarray, **rescarray;
    DMT_TBL_ENTRY	*tblptr;
    RDR_INFO		*rdrinfop;
    QEF_DATA		*qp;
    RDF_CB		*rdf_cb;
    RDR_RB		*rdf_rb;
    DB_PROCEDURE	*dbp;
    DB_DBP_NAME		proc_name;
    DB_OWN_NAME		proc_owner;
    i4			parameterCount, rescolCount, resrowWidth;
    u_i4		created;

    i4			i, j, offset;
    DB_STATUS           status;
    i4			dmt_mem_size;
    char		*nextname;
    i4			n;
    i4			attr_nametot;

    /* First retrieve iiprocedure row using id from range table entry. */
    rdf_cb = &global->ops_rangetab.opv_rdfcb;
    rdf_rb = &rdf_cb->rdf_rb;

    STRUCT_ASSIGN_MACRO(rngentry->pst_rngvar, rdf_rb->rdr_tabid);
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    /*
    ** need to set rdf_info_blk to NULL for otherwise RDF assumes that we
    ** already have the info_block
    */
    rdf_cb->rdf_info_blk = (RDR_INFO *) NULL;

    status = rdf_call(RDF_GETINFO, rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) opx_verror(status, E_OP0004_RDF_GETDESC,
                                rdf_cb->rdf_error.err_code);
    }
    dbp = rdf_cb->rdf_info_blk->rdr_dbp;

    /* Before proceeding - assure this is the same proc that we think it is. */
    if (rngentry->pst_timestamp.db_tab_high_time != dbp->db_created)
    {
	/* If not, bounce back to SCF to re-parse query. */
	opx_vrecover(E_DB_ERROR, E_OP008F_RDF_MISMATCH, 
					rdf_cb->rdf_error.err_code);
    }

    /* Save procedure stuff for later. */
    parameterCount = dbp->db_parameterCount;
    rescolCount = dbp->db_rescolCount;
    resrowWidth = dbp->db_resrowWidth;
    created = dbp->db_created;
    STRUCT_ASSIGN_MACRO(dbp->db_dbpname, proc_name);
    STRUCT_ASSIGN_MACRO(dbp->db_owner, proc_owner);

    /* Allocate attr descriptors and address from ptr arrays. */

    i = dbp->db_parameterCount + dbp->db_rescolCount;

    /*
    ** Alloc maximum name size for each parameter name
    **
    ** (Right now we don't have total parameter name size in iiprocedure.
    ** In the future it would be nice to add to add iiprocedure.parmnametot, 
    ** similar to iirelation.relattnamtot).
    ** 
    ** If this memory allocation is a problem... we could try allocating
    **  (DB_OLDMAXNAME_32 + 1) * i;
    ** and realloc/reinit parameter names if this isn't big enough
    */
    attr_nametot = (DB_ATT_MAXNAME + 1) * i;

    /* 1 extra ptr because array is 1-origin */
    dmt_mem_size = ((i+1) * sizeof(DMT_ATT_ENTRY *))
	+ (i * sizeof(DMT_ATT_ENTRY))
	+ attr_nametot;

    attarray = (DMT_ATT_ENTRY **) opu_memory(global, dmt_mem_size);
    attarray[0] = (DMT_ATT_ENTRY *)NULL;
    attrp = (DMT_ATT_ENTRY *)&attarray[i+1];
    nextname = (char *)(attrp + i);

    /* Set up attr pointer arrays for both parms and result columns. */
    for (j = 1; j <= i; j++, attrp++)
    {
	attarray[j] = attrp;
	MEfill(sizeof(DMT_ATT_ENTRY), (u_char)0, (char *)attrp);
    }

    rescarray = attarray;
    parmarray = &attarray[rescolCount+1];

    /* Load iiprocedure_parameter rows for both parms and result cols. */
    rdf_rb->rdr_types_mask     = 0;
    rdf_rb->rdr_2types_mask    = RDR2_PROCEDURE_PARAMETERS;
    rdf_rb->rdr_instr          = RDF_NO_INSTR;

    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
    rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
    rdf_cb->rdf_error.err_code = 0;

    /*
    ** must set rdr_rec_access_id since otherwise RDF will barf when we
    ** try to RDR_OPEN
    */
    rdf_rb->rdr_rec_access_id  = NULL;

    while (rdf_cb->rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	rdf_rb->rdr_update_op = RDR_GETNEXT;

	/* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	** E_DB_WARN that would be missed.
	*/
	if (status != E_DB_OK)
	{
	    switch(rdf_cb->rdf_error.err_code)
	    {
		case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;
		    continue;

		default:
		    opx_error(E_OP0013_READ_TUPLES);
		    break;
	    }	    /* switch */
	}	/* if status != E_DB_OK */

	/* For each dbproc parameter tuple */
	for (qp = rdf_cb->rdf_info_blk->rdr_pptuples->qrym_data, j = 0;
	    j < rdf_cb->rdf_info_blk->rdr_pptuples->qrym_cnt;
	    qp = qp->dt_next, j++)
	{
	    DB_PROCEDURE_PARAMETER *param_tup =
		(DB_PROCEDURE_PARAMETER *) qp->dt_data;
	    if (i-- == 0)
	    {
		opx_error(E_OP0013_READ_TUPLES);
	    }
	    if (param_tup->dbpp_flags & DBPP_RESULT_COL)
	    {
		attrp = rescarray[param_tup->dbpp_number];
		attrp->att_number = param_tup->dbpp_number;
	    }
	    else 
	    {
		attrp = parmarray[param_tup->dbpp_number-1];
		attrp->att_flags = DMT_F_TPROCPARM;
		attrp->att_number = param_tup->dbpp_number +
					dbp->db_rescolCount;
	    }

	    /* Compute blank stripped length of attribute name */
	    n = cui_trmwhite(DB_PARM_MAXNAME, param_tup->dbpp_name.db_att_name);
	    attrp->att_nmstr = nextname;
	    attrp->att_nmlen = n;
	    cui_move(n, param_tup->dbpp_name.db_att_name, '\0', n + 1, nextname);
	    nextname += (n + 1);

	    attrp->att_type = param_tup->dbpp_datatype;
	    attrp->att_width = param_tup->dbpp_length;
	    attrp->att_prec = param_tup->dbpp_precision;
	    attrp->att_collID = param_tup->dbpp_collID;
	    attrp->att_offset = param_tup->dbpp_offset;
	}
    }

    /* Reset result column offsets to remove effect of parms. */
    offset = rescarray[1]->att_offset;
    for (j = 1; j <= rescolCount; j++)
	rescarray[j]->att_offset -= offset;

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    opx_error(E_OP0013_READ_TUPLES);
	}
    }

    /* now unfix the dbproc description */
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    status = rdf_call(RDF_UNFIX, rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	opx_error(E_OP008D_RDF_UNFIX);
    }

    /* Allocate table descriptor. */
    tblptr = (DMT_TBL_ENTRY *) opu_memory(global, sizeof(DMT_TBL_ENTRY));

    /* Allocate RDR_INFO block. */
    rdrinfop = (RDR_INFO *) opu_memory(global, sizeof(RDR_INFO)+sizeof(PTR));

    /* Now format them all. */

    MEfill(sizeof(DMT_TBL_ENTRY), (u_char) 0, tblptr);
    MEcopy((char *)&proc_name.db_dbp_name, sizeof(DB_DBP_NAME), 
				(char *)&tblptr->tbl_name.db_tab_name);
    STRUCT_ASSIGN_MACRO(proc_owner, tblptr->tbl_owner);
    MEfill(sizeof(DB_LOC_NAME), (u_char)' ', (char *)&tblptr->tbl_location);
    tblptr->tbl_attr_count = rescolCount + parameterCount;
    tblptr->tbl_attr_nametot = attr_nametot;
    tblptr->tbl_width = resrowWidth;
    tblptr->tbl_date_modified.db_tab_high_time = created;
    tblptr->tbl_storage_type = DB_TPROC_STORE;
    /* Load cost parameters (if there). */
    tblptr->tbl_record_count = (dbp->db_estRows) ? dbp->db_estRows : DBP_ROWEST;
    tblptr->tbl_page_count = (dbp->db_estCost) ? dbp->db_estCost : DBP_DIOEST;
    tblptr->tbl_pgsize = 2048;

    /* All the other DMT_TBL_ENTRY fields are being left 0 until 
    ** something happens that suggests other values. */

    /* Finally fill in the RDR_INFO structure. */
    MEfill(sizeof(RDR_INFO), (u_char) 0, rdrinfop);
    rdrinfop->rdr_rel = tblptr;
    rdrinfop->rdr_attr = rescarray;
    rdrinfop->rdr_no_attr = tblptr->tbl_attr_count;
    rdrinfop->rdr_attnametot = tblptr->tbl_attr_nametot;
    rdrinfop->rdr_dbp = dbp;
    grvp->opv_relation = rdrinfop;

    BTset((i4)gvar, (char *)&global->ops_rangetab.opv_mrdf); /* indicate
						** that RDF info is fixed */
    grvp->opv_gmask |= OPV_TPROC;
    global->ops_gmask |= OPS_TPROCS;		/* show query has table procs */

    return (E_DB_OK);
}

/*{
** Name: opv_unique_colset	- load info about unique column sets
**
** Description:
**      This function loads constraint definitions, searching for UNIQUE/
**	PRIMARY KEYs, then allocates and fills an array with the corresponding
**	column sets (if any).
**
** Inputs:
**	global		- ptr to global state structure
**	grvp		- ptr to OPV_GRV of current table
**	rdfcb		- ptr to RDF control block
**
** Outputs:
**	Returns:
**	    update OPV_GRV structure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	13-may-2009 (dougi)
**	    Written to record info about unique column sets (initially for X100
**	    but will also benefit native Ingres).
**	13-may-2010 (dougi)
**	    Reverted May 12 faulty change and added a few comments.
**	15-june-2010 (dougi)
**	    Faulty code skipped key columns at end of tables.
*/
static VOID
opv_unique_colset(
	OPS_STATE	*global,
	OPV_GRV		*grvp,
	RDF_CB		*rdfcb)
{
    DB_INTEGRITY	*integ_tup;
    QEF_DATA		*datap;
    DB_STATUS		status = E_DB_OK;
    i2			*vectorp;
    i4			tupcount;
    i4			vecsize = 0;
    i4			i, j;


    /* Set up RDF block, open and start reading. */
    rdfcb->rdf_rb.rdr_types_mask = RDR_INTEGRITIES;
    rdfcb->rdf_rb.rdr_update_op = RDR_OPEN;
    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    rdfcb->rdf_rb.rdr_qtuple_count = RDF_MAX_QTCNT;

    /* I'm assuming all the integrities for a given table can be read with
    ** a single call. But if this isn't true, another loop will be required
    ** to drive it through a series of RDR_GETNEXT calls. */
    status = rdf_call(RDF_READTUPLES, (PTR)rdfcb);
    if (status != E_DB_OK)
    {

    }

    if (rdfcb->rdf_info_blk->rdr_ituples == (RDD_QDATA *) NULL)
	return;

    /* opv_ucount is the number of sets of unique columns, NOT the number of 
    ** entries in opv_uvector[]! For each opv_ucount increment, there will be
    ** a set of opv_uvector[] entries (1 to n) terminated by a -1. */
    for (tupcount = 0, datap = rdfcb->rdf_info_blk->rdr_ituples->qrym_data; 
		tupcount < rdfcb->rdf_info_blk->rdr_ituples->qrym_cnt;
		tupcount++, datap = datap->dt_next)
    {
	integ_tup = (DB_INTEGRITY *)datap->dt_data;
	if (!(integ_tup->dbi_consflags & (CONS_UNIQUE | CONS_PRIMARY)))
	    continue;

	grvp->opv_ucount++;
	vecsize += (1 + BTcount((char *)&integ_tup->dbi_columns, 
				(grvp->opv_relation->rdr_no_attr+1)));
    }

    /* Column 0 (TID) is always unique, so make space for it. */
    grvp->opv_ucount++; vecsize += 2;

    vectorp = (i2 *)opu_memory(global, vecsize * sizeof(i2));
    vectorp[0] = 0; vectorp[1] = -1;		/* Start with TID */

    /* Another pass over the constraints. */
    for (tupcount = 0, datap = rdfcb->rdf_info_blk->rdr_ituples->qrym_data, 
		i = 2; tupcount < rdfcb->rdf_info_blk->rdr_ituples->qrym_cnt;
		tupcount++, datap = datap->dt_next)
    {
	integ_tup = (DB_INTEGRITY *)datap->dt_data;
	if (!(integ_tup->dbi_consflags & (CONS_UNIQUE | CONS_PRIMARY)))
	    continue;

	for (j = -1; (j = BTnext(j, (char *)&integ_tup->dbi_columns, 
			(grvp->opv_relation->rdr_no_attr+1))) >= 0; i++)
	    vectorp[i] = j;

	vectorp[i++] = -1;
    }

    grvp->opv_uvector = vectorp;

}

/*{
** Name: opv_parser	- init global range table element given parser varno
**
** Description:
**      This routine will initialize the global range table element in OPF
**      corresponding to the PSF range table element.
**
** Inputs:
**      global                          ptr to global range table
**      gvar                            element in parser range table
**                                      which is referenced in query
**
** Outputs:
**      global->ops_rangetab.opv_base[gvar] initialize corresponding element
**                                      in optimizer range table
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-jul-86 (seputis)
**          initial creation
**	6-nov-88 (seputis)
**          change RDF invalidation to include all indexes on base relation
**          since this is only notification that OPF gets that its' time
**          stamp is out of date
**      25-sep-89 (seputis)
**          - made addition to timestamp check, to refresh if this is a
**          multi-variable query and the tuple count is zero
**	9-nov-89 (seputis)
**          - added OPA_NOVID initialization for b8160, and corrected
**	    sep 25 fix for timestamps
**	12-jan-90 (seputis)
**	    - detect table ID's which are the same for different range vars
**	26-feb-91 (seputis)
**	    - add improved diagnostics for b35862
**      31-dec-91 (seputis)
**          - flush cache entry if tuple count is zero and more than one
**          range table entry, since aggregate queries and flattened queries
**          are not getting handled.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	12-apr-93 (ed)
**	    - fix bug 50673, relax range variable check for larger OPF table
**      7-dec-93 (ed)
**          b56139 - add OPZ_TIDHISTO to mark tid attributes which
**          need histograms,... needed since target list is traversed
**          earlier than before
**      16-feb-95 (inkdo01)
**          b66907 - check for explicit refs to inconsistent tabs/idxes
**      23-feb-95 (wolf) 
**          Recent addition of MEcopy call should have been accompanied by
**	    an include of me.h
**      10-aug-98 (stial01)
**          opv_parser() Remove code to invalidate indexes. The invalidate
**          table that follows will do the job. Invalidate indexes by table id
**          can get E_OP008E_RDF_INVALIDATE, E_RD0045_ULH_ACCESS
**	31-oct-1998 (nanpr01)
**	    Reset the rdfcb->infoblk ptr before checking the error code.
**	18-june-99 (inkdo01)
**	    Init opv_ttmodel for temp table model histogram feature.
**	19-Jan-2000 (thaju02)
**	    relation descriptor may be out of date and contain a stale 
**	    secondary index count, use rdfcb->rdf_info_block->rdr_no_index 
**	    which reflects the number of index entries in the rdr_indx 
**	    array. (b100060)
**	17-Jan-2004 (schka24)
**	    Rename RDR_BLD_KEY to RDR_BLD_PHYS.
**	3-Sep-2005 (schka24)
**	    Remove if-0'ed out section that included a ref to a member
**	    that is going away (opv_ghist).
**	17-Nov-2005 (schka24)
**	    Don't propagate RDF invalidates that we cause.  Our RDF cache
**	    is out of date but that's not other servers' problem.
**	14-Mar-2007 (kschendel) SIR 122513
**	    Light "partitioned" flag if partitioned table seen.
**	18-april-2008 (dougi)
**	    Add support for table procedures.
**	17-dec-2008 (dougi)
**	    Add column comparison stats and referential relationship column
**	    maps to retrieved catalog info.
**	8-Jul-2010 (wanfr01) b123949
**	    If rdf_gdesc failed with an error, don't use the column
**	    information - it may not be fully initialized.
**	9-july-2010 (dougi)
**	    Add support for X100 temp tables.
**	21-sep-2010 (dougi)
**	    Set up X100 table order (from indexes).
**	15-feb-2011 (dougi)
**	    Remove constraint of > 1 table for updating defs. Row counts are 
**	    important even for single table queries.
**	16-feb-2011 (dougi)
**	    Compensate for possible RDF bug in setting rdr_refrel.
**	10-Mar-2011 (kschendel)
**	    Ensure no garbage 2types bits are set - no symptoms known.
**	18-Mar-2011 (kschendel)
**	    Don't wap around rdr_keys for "indexed" VW tables here, let RDF
**	    do the evil deeds.  (Thus ensuring that there's sufficient
**	    keys memory allocated!)
**	7-Apr-2011 (kschendel) SIR 125253
**	    Don't ask for unique-colsets for system catalogs, won't be any.
**	    Attempting to scan iiintegrities is a problem during createdb!
*/
bool
opv_parser(
	OPS_STATE          *global,
	OPV_IGVARS	   gvar,
	OPS_SQTYPE         sqtype,
	bool		   rdfinfo,
	bool               psf_table,
	bool               abort)
{
    OPV_GRT             *gbase;	    /* ptr to base of global range table */
    i4			stgetp, i;

# ifdef E_OP0387_VARNO
    if ((gvar < 0) || ((gvar >= PST_NUMVARS) && (gvar >= OPV_MAXVAR)))
	opx_error(E_OP0387_VARNO ); /* range var out of range - 
                                    ** consistency check */
# endif

    gbase = global->ops_rangetab.opv_base; /* get base of global range table
                                    ** ptr to array of ptrs */
    if ( !gbase->opv_grv[gvar] )
    {
	/* if global range variable element has not been allocated */
	OPV_GRV             *grvp;	    /* ptr to new range var element */
	
        if (global->ops_rangetab.opv_gv <= gvar)
            global->ops_rangetab.opv_gv = gvar+1; /* update the largest range
                                            ** table element assigned so far
                                            */
        grvp = (OPV_GRV *) opu_memory(global, sizeof( OPV_GRV ) ); /* save
                                            ** and allocate ptr to global
                                            ** var */
	/* Explicitly zero out the grv entry */
	MEfill(sizeof(*grvp), (u_char)0, (PTR)grvp);
	grvp->opv_qrt = gvar;		    /* save index to
                                            ** parser range table element */
        grvp->opv_created = sqtype;         /* save global range table type */
	grvp->opv_gvid = OPA_NOVID;	    /* if this base table was implicitly
					    ** referenced then the view id of the
					    ** explicitly reference view will be
					    ** saved */
        grvp->opv_siteid = OPD_NOSITE;      /* initialize distributed site location */
	grvp->opv_same = OPV_NOGVAR;	    /* used to map tables with similiar
					    ** IDs */
	grvp->opv_compare = OPV_NOGVAR;	    /* used to map tables with similiar
					    ** IDs */
	grvp->opv_ttmodel = NULL;	    /* RDR_INFO ptr for temp table model
					    ** histograms */
	gbase->opv_grv[gvar] = grvp;	    /* place into table */

        /* get RDF information about the table */
        if (rdfinfo)
        {
	    RDF_CB             *rdfcb;	    /* ptr to RDF control block which
                                            ** has proper db_id and sessionid
                                            ** info */
	    PST_RNGENTRY       *rngentry;   /* ptr to parse tree range entry
					    ** ONLY VALID if psf_table!! */
	    RDR_INFO	       *tmpr;
	    DB_STATUS          status;      /* RDF return status */

	    i4	       ituple;

            rdfcb = &global->ops_rangetab.opv_rdfcb;
	    if (psf_table)
	    {
		rngentry = global->ops_qheader->pst_rangetab[gvar];

		/* Snag table procedures and handle them elsewhere. */
		if (rngentry->pst_rgtype == PST_TPROC)
		{
		    if (opv_tproc(global, gvar, grvp, rngentry) == E_DB_OK)
			return(FALSE);
		    else return(TRUE);
		}

		STRUCT_ASSIGN_MACRO(rngentry->pst_rngvar,
			rdfcb->rdf_rb.rdr_tabid); /* need 
						** table id from parser's table */
		if (rngentry->pst_rgtype == PST_SETINPUT)
		    grvp->opv_gmask |= OPV_SETINPUT; /* is this the set input
						    ** parameter for a procedure
						    */
		rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION | RDR_INDEXES |
		    RDR_ATTRIBUTES | RDR_BLD_PHYS; /* get relation info 
						** - The optimizer uses attribute
						** info in query tree directly 
						** but it is needed to be requested
						** since the RDF uses attr info to
						** build RDR_BLK_KEY info.  The
						** attribute info does not need to
						** requested if RDF is changed.
						** - ask for indexes so that
						** invalidating the cache can also
						** invalidate any indexes in the
						** cache
						*/
		rdfcb->rdf_rb.rdr_2types_mask = 0;
		if (rngentry->pst_rngvar.db_tab_base > DM_SCATALOG_MAX)
		    rdfcb->rdf_rb.rdr_2types_mask = (RDR2_COLCOMPARE | 
								RDR2_REFREL);
						/* col compare stats and ref
						** relationship defs, too */

	    }
	    rdfcb->rdf_info_blk = NULL;     /* get new ptr to info
                                            ** associated with global var */
            status = rdf_call( RDF_GETDESC, (PTR)rdfcb);
	    grvp->opv_relation = rdfcb->rdf_info_blk; /* save ptr to
                                            ** new info block */
	    if ((status != E_RD0000_OK)
		&&
		(rdfcb->rdf_error.err_code != E_RD026A_OBJ_INDEXCOUNT) /* this
					    ** is a check for distributed
					    ** index information, which could
					    ** be inconsistent but should not
					    ** cause the query to be aborted
					    ** it will cause indexes to be
					    ** avoided */
		)
	    {
		gbase->opv_grv[gvar] = NULL;
		if (abort)
		    opx_verror( status, E_OP0004_RDF_GETDESC,
			rdfcb->rdf_error.err_code);
		else
		{
		    return (TRUE);	/* indicate failure to get RDF
				    ** descriptor */
		}
	    }
            BTset( (i4)gvar, (char *)&global->ops_rangetab.opv_mrdf); /* indicate
                                            ** that RDF information is fixed */

	    /* Check table and all associated indexes to see if there is a
	    ** statement level index associated with this variable
	    */
	    if (grvp->opv_relation->rdr_rel->tbl_2_status_mask
		& DMT_STATEMENT_LEVEL_UNIQUE)
		grvp->opv_gmask |= OPV_STATEMENT_LEVEL_UNIQUE;
	    for( ituple = grvp->opv_relation->rdr_no_index;
		ituple-- ;)
	    {
		if (grvp->opv_relation->rdr_indx[ituple]->idx_status
		    & DMT_I_STATEMENT_LEVEL_UNIQUE)
		    grvp->opv_gmask |= OPV_STATEMENT_LEVEL_UNIQUE;
	    }
	    if (psf_table)
	    {	/* check if timestamp matches and invalidate RDF cache
                ** date of last modify do not match, this is only done
		** for tables passed by PSF, the other tables should
		** be dependent on the PSF time stamp */
		DB_TAB_TIMESTAMP	*timeptr; /* ptr to last modify
                                            ** date that RDF has cached */
		DB_TAB_TIMESTAMP	*psftimeptr; /* ptr to last modify
					    ** date which parser used for the
                                            ** table */
		psftimeptr = &rngentry->pst_timestamp;
		timeptr = &grvp->opv_relation->rdr_rel->tbl_date_modified;
                if (timeptr->db_tab_high_time != psftimeptr->db_tab_high_time
                    || timeptr->db_tab_low_time != psftimeptr->db_tab_low_time)
		{
		    PTR save_fcb = rdfcb->rdf_rb.rdr_fcb;

		    /* Don't propagate this invalidate to other DBMS servers.
		    ** There's no reason to think that they are as out of
		    ** date as we are.  (plus this might be a session temp
		    ** which is strictly local!)
		    */

		    rdfcb->rdf_rb.rdr_fcb = NULL;
		    status = rdf_call( RDF_INVALIDATE, (PTR)rdfcb);
		    rdfcb->rdf_rb.rdr_fcb = save_fcb;
# ifdef E_OP008E_RDF_INVALIDATE
		    if (status != E_RD0000_OK)
			opx_verror( E_DB_ERROR, E_OP008E_RDF_INVALIDATE,
			    rdfcb->rdf_error.err_code);
# endif
		    status = rdf_call( RDF_GETDESC, (PTR)rdfcb);
		    grvp->opv_relation = rdfcb->rdf_info_blk; /* save ptr to
					    ** new info block */
		    if (status != E_RD0000_OK)
		    {
			gbase->opv_grv[gvar] = NULL;
			opx_verror( E_DB_ERROR, E_OP0004_RDF_GETDESC,
			    rdfcb->rdf_error.err_code);
		    }
		    timeptr = &grvp->opv_relation->rdr_rel->tbl_date_modified;
		    if (timeptr->db_tab_high_time != psftimeptr->db_tab_high_time
			||
			timeptr->db_tab_low_time != psftimeptr->db_tab_low_time
			)
			opx_vrecover( E_DB_ERROR, E_OP008F_RDF_MISMATCH,
			    rdfcb->rdf_error.err_code); /* PSF timestamp is
					    ** still out of date, so tell
                                            ** SCF to reparse the query */
		}
		{   /* search thru existing tables to discover if any
		    ** tables have the same table ID */
		    OPV_IGVARS	    gvno;
		    DB_TAB_ID	    *tabidp;
		    OPV_IGVARS	    target_vno;

		    tabidp = &rngentry->pst_rngvar;
		    target_vno = OPV_NOGVAR;

		    for (gvno = 0; gvno < OPV_MAXVAR; gvno++)
		    {
			OPV_GRV	    *grv1p;
			grv1p = gbase->opv_grv[gvno];
			if (grv1p && grv1p->opv_relation)
			{
			    DB_TAB_ID   *gtabidp;

			    if (gvno == gvar)
				continue;
			    gtabidp = &grv1p->opv_relation->rdr_rel->tbl_id;
			    if ((tabidp->db_tab_base == gtabidp->db_tab_base)
				&&
				(tabidp->db_tab_index == gtabidp->db_tab_index)
				)
			    {	/* found 2 table ID's which are identical */
				global->ops_gmask |= OPS_IDSAME;
				if (target_vno == OPV_NOGVAR)
				{   /* map all table id's vars to the lowest
				    ** global range number of the group */
				    if (gvno > gvar)
					target_vno = gvar;
				    else
					target_vno = gvno;
				}
				grv1p->opv_same = target_vno;
				grvp->opv_same = target_vno;
				if (target_vno != gvar)
				    break;
			    }
			}
		    }
		}
	    }
	    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		opd_addsite(global, grvp); /* add site information if a
					** distributed thread */
	    /* Check for partitioned table, turns on additional
	    ** analysis after enumeration.
	    */
	    if (grvp->opv_relation != NULL
	      && grvp->opv_relation->rdr_parts != NULL)
		global->ops_gmask |= OPS_PARTITION;
            if (grvp->opv_relation && grvp->opv_relation->rdr_rel->
                   tbl_2_status_mask & DMT_INCONSIST)
                                        /* check for inconsistent table
                                        ** (due to partial back/recov) */
            {
               OPT_NAME     tabname;
               i2           i;
             
               MEcopy(&grvp->opv_relation->rdr_rel->tbl_name, 
                         sizeof(grvp->opv_relation->rdr_rel->tbl_name),
                         &tabname);           /* table name is msg token */
               for (i = sizeof(grvp->opv_relation->rdr_rel->tbl_name);
                      i > 1 && tabname.buf[i-1] == ' '; i--);
               tabname.buf[i] = 0;            /* lop blanks off name */
	       opx_1perror(E_OP009A_INCONSISTENT_TAB,(PTR)&tabname);  
            }

	    stgetp  = grvp->opv_relation->rdr_rel->tbl_storage_type;

	    /* Call func to search for (and fill in) info about unique/
	    ** primary key constraints. Then we'll have info about sets
	    ** of unique columns in table.
	    ** FIXME ideally would only do this for tables that have the
	    ** info, but apparently there's no way to tell, and RDF doesn't
	    ** cache has-uniqueness so this is an iiintegrities hit each time.
	    ** FOR NOW ONLY DO THIS FOR VW TABLES since we don't use the
	    ** info for ingres tables.
	    */
	    if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	      && grvp->opv_relation->rdr_rel->tbl_id.db_tab_base > DM_SCATALOG_MAX
	      && stgetp > DB_STDING_STORE_MAX /* FIXME REMOVE LATER */)
		opv_unique_colset(global, grvp, rdfcb);

	    /* Look for X100 temp tables and fiddle their names. */
	    if (grvp->opv_relation->rdr_rel->tbl_temporary &&
		stgetp > DB_STDING_STORE_MAX)
	    {
		DMT_TBL_ENTRY	*tblptr;
		RDR_INFO	*rdrinfop;
		DB_TAB_NAME	newname;

		/* Allocate replacement table descriptor, copy the RDF
		** version over, then build new name and copy it back. 
		** If we altered RDF version in place, the altered name
		** would persist.
		** Note: we expect that the original RDF atts, names, etc
		** will persist longer than our new INFO!
		*/
		rdrinfop = (RDR_INFO *) opu_memory(global, sizeof(RDR_INFO));
		tblptr = (DMT_TBL_ENTRY *) opu_memory(global, 
						sizeof(DMT_TBL_ENTRY));
		MEcopy((char *)grvp->opv_relation, sizeof(RDR_INFO),
		    (char *)rdrinfop);
		MEcopy((char *)grvp->opv_relation->rdr_rel,
		    sizeof(DMT_TBL_ENTRY), (char *)tblptr);
		grvp->opv_relation = rdrinfop;
		grvp->opv_relation->rdr_rel = tblptr;

		MEcopy("Ivw_sess_temp_", sizeof("Ivw_sess_temp_"), 
				(char *)&newname);
		MEcopy((char *)&grvp->opv_relation->rdr_rel->tbl_name.
			db_tab_name, sizeof(newname)-14, 
		    (char *)&newname.db_tab_name[14]);
		MEcopy((char *)&newname, sizeof(newname),
		    (char *)&grvp->opv_relation->rdr_rel->tbl_name);
	    }
        }
	else
	    grvp->opv_relation = NULL;          /* set to NULL if not used */
    }
    return(FALSE);
}
