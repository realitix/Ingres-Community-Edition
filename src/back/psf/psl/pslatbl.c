/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <er.h>
#include    <me.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <bt.h>
#include    <dbdbms.h>
#include    <dmf.h>	/* needed for psfparse.h */
#include    <adf.h>	/* needed for psfparse.h */
#include    <ddb.h>	/* needed for psfparse.h */
#include    <ulf.h>	/* needed for qsf.h, rdf.h and pshparse.h */
#include    <qsf.h>	/* needed for psfparse.h and qefrcb.h */
#include    <qefrcb.h>	/* needed for psfparse.h */
#include    <psfparse.h>
#include    <dmtcb.h>	/* needed for rdf.h and pshparse.h */
#include    <qu.h>	    /* needed for pshparse.h */
#include    <rdf.h>	    /* needed for pshparse.h */
#include    <psfindep.h>    /* needed for pshparse.h */
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <cui.h>
#include    <opfcb.h>
#include    <ulm.h>
#include    <dmrcb.h>
#include    <dmucb.h>	    
#include    <qefmain.h>
#include    <qefqeu.h>

/*
** Name: PSLATBL.C:	this file contains functions used in semantic actions
**			for the ALTER TABLE (SQL) statement
**
** Description:
**	this file contains functions used by the SQL grammar in
**	parsing and subsequent processing of the ALTER TABLE statement
**
**	Routines:
**	    psl_alter_table	- actions for alter_table
**				  (check existing data, then create constraint)
**	    psl_alt_tbl		- actions for alt_tbl (make sure table exists)
**	    psl_d_cons		- semantic action for ALTER TABLE DROP
**				  CONSTRAINT
**	    psl_alt_tbl_rename	- actions for ALTER TABLE RENAME TO.
**	    psl_alt_tbl_col	- actions for ALTER TABLE ADD/DROP COLUMN,
**				  alloc. and populate QEU and DMU ctl blks.
**	    psl_alt_tbl_col_add	- actions for ALTER TABLE ADD COLUMN, check
**				  col does not exist, null/default values etc.
**	    psl_alt_tbl_col_drop- actions for ALTER TABLE DROP COLUMN, check
**				  that col exists.
**	    psl_alt_tbl_rename - actions for ALTER TABLE RENAME.
**	    psl_alt_tbl_col_rename- actions for ALTER TABLE RENAME COLUMN.
**
**  History:
**      28-nov-92 (rblumer)    
**          created.
**      20-feb-93 (rblumer)    
**          take out "not-implemented" error code, and allow to go thru OPF.
**      05-mar-93 (rblumer)    
**          add psl_check_existing_data function, plus its supporting
**          functions psl_check_foreignkey, psl_check_checkcons, and
**          psl_execute_data_check.
**	13-mar-93 (andre)
**	    wrote psl_d_cons
**	21-mar-93 (rblumer)
**	    take 'unimplemented' error message out of psl_alter_table
**	    and let statement go to OPF and QEF; added psq_cb to
**	    psl_check_existing_data and add CREATE SCHEMA optimization.
**	24-mar-93 (rblumer)
**	    fixed typo in optimization in psl_check_existing_data.
**	30-mar-93 (rblumer)
**	    set PSQ_NO_CHECKPERMS flag so parser does not check permissions for
**	    internal query in psl_execute_data_check.
**	26-apr-93 (rblumer)
**	    in psl_check_foreignkey, use unique correlation names instead of
**	    table names to qualify column names; otherwise we get ambiguous
**	    references when checking self-referential constraints.
**	    Added function headers/descriptions to several functions.
**	28-apr-93 (rickh)
**	    Changed some ifs to ifdefs.
**	08-jun-93 (rblumer)
**	    in psl_execute_data_check, set QEU_WANT_USER_XACT to tell QEF to
**	    start a user transaction if we are currently in an internal xact.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**      12-Jul-1993 (fred)
**          Disallow mucking with extension tables.  Treat the same as
**          system catalogs.
**	16-jul-93 (rblumer)
**	    in psl_check_existing_data, change PST_EMPTY_REFERENCING_TABLE to
**	    PST_EMPTY_TABLE, and combine with CREATE SCHEMA short-cut.
**	    Also, changed psl_d_cons to use pst_ddl_header function.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	10-sep-93 (rblumer)
**	    SET the PST_VERIFY_CONS_HOLDS bit in pst_createIntegrityFlags.
**	    This tells QEF to verify whether the constraint holds on existing
**	    data in the table.  This used to be done here in PSF (as a hack),
**	    but has now been moved to QEF (where it belongs and where it can
**	    easily be made part of the user transaction).  Thus I have
**	    REMOVED psl_check_existing_data() and all its supporting functions:
**	    psl_execute_data_check, handle_qef_error, psl_check_checkcons,
**	    and psl_check_foreignkey.
**	23-sep-93 (rblumer)
**	    removed prototype for psl_check_existing_data.
**	10-nov-93 (anitap)
**	    Changes for verifydb for FIPS constraints. Do not give error when
**	    $ingres tries to alter another user's table. This is to enable
**	    verifydb to "alter table drop constraint" on a table owned by 
**	    another user. Changes in psl_alt_tbl().
**	18-apr-94 (rblumer)
**	    interface of psl_verify_cons() changed to accept PSS_RNGTAB/DMU_CB
**	    pair instead of PTR.
**      06-mar-2000 (stial01)
**          psl_alt_tbl_col() Init dmu_location,dmu_rawextnm (B100756)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	01-Mar-2001 (jenjo02)
**	    Remove references to obsolete DB_RAWEXT_NAME, dmu_rawextnm.
**	15-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	18-April-2005 (inifa01) INGSRV2907 b112687
**	    In pls_alter_table() added invalidattion of table relation cache
**	    once the constraint being created has been verified to prevent 
**	    creation of duplicate primary keys due to stale cache info being 
**	    checked.
**      21-Nov-2008 (hanal04) Bug 121248
**          If we are invalidating a table referenced in a range variable
**          we should pass the existing pss_rdrinfo reference if there is one.
**          Furthermore, we need RDF to clear down our reference to the 
**          rdf_info_blk if it is freed.
**      30-Oct-2009 (gefei01)
**          ALTER TABLE ADD COLUMN and DROP CONSTRAINT unsupported on
**          X100 tables.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	6-Jun-2010 (kschendel) b123923
**	    Allow alter table alter column with lob only if type doesn't change.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	14-Oct-2010 (kschendel) SIR 124544
**	    Change how we store DMU characteristics
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      04-mar-2011 (stial01) m1665
**          Error E_US24FC_9468 for alter add column/constraint for x100.
**      11-may-2011 (stial01) m1545
**          Check for unsupported VectorWise datatypes
**          Disallow alter column for VectorWise tables (m1650)
**      24-May-2011 (horda03) b125118
**          Allow IDENTITY column attribute to be dropped.
**       4-Jul-2011 (hanal04) SIR 125458
**          Add DB_ERROR support (__FILE__, __LINE__) to PSF and to user
**          error functions in SCF.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**      18-aug-2011 (stial01) b125675, m2347
**          psl_alt_tbl_col_drop() Check for alter drop 'tid' column
*/

/* TABLE OF CONTENTS */
i4 psl_alter_table(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_RNGTAB *rngvar,
	PSS_CONS *cons_list);
i4 psl_alt_tbl(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_OBJ_NAME *tbl_spec,
	PSS_RNGTAB **rngvarp);
i4 psl_d_cons(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_RNGTAB *rngvar,
	char *cons_name,
	bool drop_cascade);
i4 psl_alt_tbl_col(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_alt_tbl_col_drop(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DB_ATT_NAME attname,
	bool cascade);
i4 psl_alt_tbl_col_add(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DB_ATT_NAME attname,
	PSS_CONS *cons_list,
	i4 altopts);
i4 psl_alt_tbl_col_rename(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DB_ATT_NAME attname,
	DB_ATT_NAME *newattname);
i4 psl_alt_tbl_rename(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_OBJ_NAME *newTabName);
static i4 psl_atbl_partcheck(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DMT_ATT_ENTRY *dmt_attr);
static i4 psl_atbl_alter_lob(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DMT_ATT_ENTRY *dmt_att,
	DMF_ATTR_ENTRY *dmu_att);

/*
** Name: psl_alter_table  - verify that constraint for an ALTER TABLE statement
**                          is correctly specified, and verify that existing
**                          data in the table does not violate the constraint
**
** Description:	    
**          Calls psl_verify_cons to verify that columns (and tables) used in a
**          constraint really exist, and build a PST_CREATE_INTEGRITY statement
**          node for the constraint.  Also, check existing data in the table
**          being altered, to verify that the constraint holds on the existing
**          data.  If constraints does not hold, return an error.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	      ptr to mem stream for allocating statement nodes
**	psq_cb		    PSF request CB
**	    psq_mode	    query mode
**	rngvar		    range table entry of table being altered
**	cons_list	    constraint info block for constraint being added
**
** Outputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	      root of this stream points to head of query tree
**	psq_cb		    PSF request CB
**         psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	28-nov-92 (rblumer)
**	    written
**	05-mar-93 (rblumer)
**	    take out code handling returned stmt list from 
**	    psl_check_existing_data, as it will no longer return a list.
**	21-mar-93 (rblumer)
**	    take out 'unimplemented' error and let stmt go to OPF and QEF.
**	07-may-93 (andre)
**	    interface of psl_verify_cons() changed to accept psq_cb instead of
**	    psq_mode and &psq_error
**	10-sep-93 (rblumer)
**	    SET the PST_VERIFY_CONS_HOLDS bit in pst_createIntegrityFlags.
**	    This tells QEF to verify whether the constraint holds on existing
**	    data in the table.  This used to be done here in PSF (as a hack),
**	    but has now been moved to QEF (where it belongs and where it can
**	    easily be made part of the user transaction).
**	18-apr-94 (rblumer)
**	    interface of psl_verify_cons() changed to accept PSS_RNGTAB/DMU_CB
**	    pair instead of PTR.
**      18-April-2005 (inifa01) INGSRV2907 b112687
**          In pls_alter_table() added invalidattion of table relation cache
**          once the constraint being created has been verified to prevent
**          creation of duplicate primary keys due to stale cache info being
**          checked.
**	9-Nov-2010 (kschendel)
**	    Change verify-cons call again.
*/
DB_STATUS
psl_alter_table(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_RNGTAB	*rngvar,
		PSS_CONS	*cons_list)
{
    PST_STATEMENT  *cr_integ_stmt;  /* pointer to CREATE_INTEGRITY stmt */
    DB_STATUS      status;
    DB_ERROR       *err_blk = &psq_cb->psq_error;
    RDF_CB rdf_cb;    
    /* verify that the constraint info is correct
    ** and convert it into a CREATE INTEGRITY statement
    */
    status = psl_verify_cons(sess_cb, psq_cb,
			     (struct _DMU_CB *) NULL, rngvar,
			     cons_list, &cr_integ_stmt);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* since this is an alter table of an existing table,
    ** set the bit to indicate that we must verify that the
    ** constraint holds on existing data in the table.
    **
    ** but let's optimize this, and not set the bit when we know the table is
    ** empty (i.e. don't set it when we are in an execute-immediate statement
    ** and we know there is no data in the table.  This happens when creating
    ** referential constraints in a CREATE SCHEMA statement or when creating
    ** self-referential constraints in a CREATE TABLE statement.)
    */
    if (   (psq_cb->psq_info == (PST_INFO *) NULL) 
	|| (~psq_cb->psq_info->pst_execflags & PST_EMPTY_TABLE)) 
    {
	cr_integ_stmt->pst_specific.pst_createIntegrity.pst_createIntegrityFlags
	    |= PST_VERIFY_CONS_HOLDS;
    }

    /* allocate the proc header for the query tree
    ** and set the root of the QSF object to the head of the tree
    */
    status = pst_ddl_header(sess_cb, cr_integ_stmt, err_blk);

    if (DB_FAILURE_MACRO(status))
	return(status);
    /* Invalidate base table information from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
                         rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
       (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    
    return(E_DB_OK);
    
} /* end psl_alter_table */



/*
** Name: psl_alt_tbl  - make sure table exists;
** 			returns pointer to range entry describing the table.
**
** Description:	    
**          Calls normal rngent() functions to verify the table exists.
**          Range entry describing the table is returned in rngvarp;
**          a memory stream is also opened for the query.
**          This is used in the constraint productions.
**          
**          Returns an error if table found is actually a view or index,
**          or if the table is not owned by the current user.
**          Also returns an error for system catalogs (unless user has catalog
**          update privilege).
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	      tells whether this is a distributed thread or not
**	    pss_auxrng	      passed to rngent functions
**	    pss_user	      name of current user
**	    pss_ses_flag      holds PSS_CATUPD flag
**	    pss_lineno	      used for error messages
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode
**	tbl_spec	    holds table name and owner name (if specified)
**
** Outputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	      points to newly opened memory stream
**	psq_cb		    PSF request CB
**         psq_error	    will be filled in if an error occurs
**	rngvarp		    returned range table entry of table being altered
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	opens memory stream
**
** History:
**
**	28-nov-92 (rblumer)
**	    written
**	20-apr-93 (rblumer)
**	    return error if trying to alter an index.
**	04-may-93 (andre)
**	    place the table being altered at scope specified by
**	    sess_cb->pss_qualdepth (rather than -1) which will spare us having
**	    to provide special treatment to <column reference>s of form
**	    [schema.]table.column inside CHECK constraint
**	07-may-93 (andre)
**	    set sess_cb->pss_resrng to point at the range entry for the table
**	    being "altered".  This will greatly simplify the task of figuring
**	    out the table name, owner, etc when producing constraint-specific
**	    error messages, for instance
**      12-Jul-1993 (fred)
**          Disallow mucking with extension tables.  Treat the same as
**          system catalogs.
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with 
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	10-nov-93 (anitap)
**	    Changes for verifydb for FIPS constraints. Do not give error when
**	    $ingres tries to alter another user's table. This is to enable
**	    verifydb to "alter table drop constraint" on a table owned by 
**	    another user.
**	14-jun-06 (toumi01)
**	    Allow session temporary tables to be referenced without the
**	    "session." in DML. This eliminates the need for this Ingres-
**	    specific extension, which is a hinderance to app portability.
**	    If psl_rngent returns E_DB_INFO it found a session temporary
**	    table, which is invalid in this context, so return an error.
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl_rngent for WITH support.
*/
DB_STATUS
psl_alt_tbl(
	    PSS_SESBLK 	  *sess_cb,
	    PSQ_CB	  *psq_cb,
	    PSS_OBJ_NAME  *tbl_spec,
	    PSS_RNGTAB	  **rngvarp)
{
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    i4	err_code, err_no;
    i4	mask;
    i4	mask2;
    PSS_RNGTAB	*rngvar;
    i4		rngvar_info;

    /* "alter table" is not allowed in distributed yet 
     */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	(VOID) psf_error(5118L, (DB_ERROR *)NULL, PSF_USERERR, 
			 &err_code, err_blk, 1,
			 sizeof(ERx("ALTER TABLE"))-1, ERx("ALTER TABLE"));
	return (E_DB_ERROR);
    }
    
    /* make sure that table exists and get range entry 
     */
    /* the call to psl_[o]rngent will fix the table info in RDF and store it
    ** in the range table (so it can be used in CHECK constraint parsing);
    ** it will be unfixed automatically during normal cleanup 
    ** when the query is finished (or aborted due to error).
    */
    if (tbl_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
    {
	status = psl_orngent(&sess_cb->pss_auxrng, sess_cb->pss_qualdepth, 
			            tbl_spec->pss_orig_obj_name,
			     &tbl_spec->pss_owner,
			     &tbl_spec->pss_obj_name, sess_cb, FALSE, rngvarp,
			     psq_cb->psq_mode, err_blk, &rngvar_info);
    }
    else
    {
	status = psl_rngent(&sess_cb->pss_auxrng, sess_cb->pss_qualdepth, 
			           tbl_spec->pss_orig_obj_name,
			    &tbl_spec->pss_obj_name, sess_cb, FALSE, rngvarp,
			    psq_cb->psq_mode, err_blk, &rngvar_info, NULL);
	if (status == E_DB_INFO)	/* oops, we found a session.table */
	{
	    (VOID) psf_error(E_PS0BD2_NOT_SUPP_ON_TEMP, (DB_ERROR *)NULL, 
                PSF_USERERR, &err_code, err_blk, 1,
		sizeof("ALTER TABLE")-1, "ALTER TABLE");
	    return(E_DB_ERROR);
	}

    }
    if (DB_FAILURE_MACRO(status))
	return (status);

    rngvar = *rngvarp;
    mask   = rngvar->pss_tabdesc->tbl_status_mask;
    mask2   = rngvar->pss_tabdesc->tbl_2_status_mask;

    /* check for constraints on views and indexes,
    **     or on tables not owned by the user;
    ** if either case is found, return an error.
    */
    err_no = 0L;
    if ((MEcmp(rngvar->pss_ownname.db_own_name,
	      sess_cb->pss_user.db_own_name, DB_OWN_MAXNAME) != 0) &&
	(MEcmp(rngvar->pss_ownname.db_own_name,		
	      sess_cb->pss_sess_owner.db_own_name, DB_OWN_MAXNAME) != 0) &&
	((sess_cb->pss_ses_flag & PSS_INGRES_PRIV) == 0))
	
	err_no = 2117L;	
    else 
      if ((mask & DMT_VIEW) || (mask & DMT_IDX))
	err_no = E_PS0496_CANT_ALTER_VIEW;
	
    if (err_no)
    {
	/*
	** let user know if name supplied by the user was resolved to a
	** synonym
	*/
	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    psl_syn_info_msg(sess_cb, rngvar, tbl_spec, rngvar_info,
			     sizeof(ERx("ALTER TABLE"))-1, ERx("ALTER TABLE"),
			     err_blk);
	}				      

	(void) psf_error(err_no, (DB_ERROR *)NULL, PSF_USERERR, 
                 &err_code, err_blk, 1,
		 psf_trmwhite(DB_TAB_MAXNAME, rngvar->pss_tabname.db_tab_name),
		 rngvar->pss_tabname.db_tab_name);
	return (E_DB_ERROR);
    }

    /* if a DBMS catalog (and not an extended catalog) 
    ** and user does not have catalog update permission, return error.
    */
    if ((mask & DMT_CATALOG || mask2 & DMT_TEXTENSION) 
	&& (~mask & DMT_EXTENDED_CAT) && (~sess_cb->pss_ses_flag & PSS_CATUPD))
    {
	/*
	** let user know if name supplied by the him was resolved to a
	** synonym
	*/
	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    psl_syn_info_msg(sess_cb, rngvar, tbl_spec, rngvar_info, 
			     sizeof(ERx("ALTER TABLE"))-1, ERx("ALTER TABLE"),
			     err_blk);
	}
	
	(VOID) psf_error(3498L, (DB_ERROR *)NULL, PSF_USERERR, 
                         &err_code, err_blk, 2, 
			 sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			 psf_trmwhite(sizeof(DB_TAB_NAME),
				      (char *) &rngvar->pss_tabname),
			 (char *) &rngvar->pss_tabname);
	return (E_DB_ERROR);
    }

    /* since schema name and table name check out OK,
    ** open the memory stream for allocating the query tree.
    */

    if (psq_cb->psq_mode == PSQ_ALTERTABLE)
    {
	status = psf_mopen(sess_cb, QSO_QTREE_OBJ, 
		          &sess_cb->pss_ostream, err_blk);
		       
	if (DB_FAILURE_MACRO(status))
	    return (status);
	sess_cb->pss_stk_freelist = NULL;
	sess_cb->pss_object = (PTR) 0;
	sess_cb->pss_save_qeucb = (PTR) 0;
    }

    sess_cb->pss_resrng = rngvar;

    return(E_DB_OK);

} /* end psl_alt_tbl */

/*
** Name: psl_d_cons	- implement semantic action for
**			  alter_table: alt_tbl DROP CONSTRAINT generic_ident
**			  production
**
** Description:
**	this function will verify that a constraint with specified name exists
**	on the specified table and build a PST_STATEMENT structure describing
**	the constraint to be dropped
**
** Input:
**	sess_cb		    PSF sesion CB
**	psq_cb		    PSF request CB
**	rngvar		    range variable describing the table a constraint on
**			    which is to be dropped
**	cons_name	    NULL-terminated constraint name
**	drop_cascade	    TRUE if cascading destruction was (implicitly or
**			    explicitly) specified; FALSE otherwise
**	
** Output:
**	sess_cb
**	    pss_ostream	    if successful, root of the QSF object allocated
**			    using pss_ostream will point at a PST_PROCEDURE
**			    node with a single PST_DROP_INTEGRITY_TYPE statement
**	psq_cb
**	    psq_error	    filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	memory will be allocated
**
** History
**	13-mar-93 (andre)
**	    written
**	16-jul-93 (rblumer)
**	    replaced code allocating procedure node with call to
**	    existing procedure pst_ddl_header.
**	10-aug-93 (andre)
**	    removed cause of compiler warning
**	6-may-2009 (dougi)
**	    Tag "drop constraint" for X100 tables.
**      30-Oct-2009 (gefei01)
**          ALTER TABLE DROP CONSTRAINT is unsupported on X100 table.
*/
DB_STATUS
psl_d_cons(
    PSS_SESBLK	    *sess_cb,
    PSQ_CB	    *psq_cb,
    PSS_RNGTAB	    *rngvar,
    char	    *cons_name,
    bool	    drop_cascade)
{
    DB_STATUS			    status;
    DB_INTEGRITYIDX		    integidx;
    RDF_CB			    rdf_cb;
    RDR_RB			    *rdf_rb = &rdf_cb.rdf_rb;
    PST_STATEMENT		    *snode;
    PST_DROP_INTEGRITY		    *drop_integ;
    struct PST_DROP_CONS_DESCR_	    *drop_cons;
    i4			    err_code;

    if (rngvar->pss_tabdesc->tbl_storage_type > DB_STDING_STORE_MAX)
    {
        /* ALTER TABLE DROP CONSTRAINT is unsupported on X100 table. */
        (void)psf_error(9469L, 0L, PSF_USERERR, &err_code,
                        &psq_cb->psq_error, 1,
                        psf_trmwhite(DB_TAB_MAXNAME,
                                     rngvar->pss_tabname.db_tab_name),
                        rngvar->pss_tabname.db_tab_name);

        return E_DB_ERROR;
    }
    
    /*
    ** first we call RDF to verify that a constraint with specified name existis
    ** in the schema to which the table belongs
    ** 
    ** @FIXME - strictly speaking, it would be nice to verify that the
    **		constraint really was defined on this table, but this will have
    **		to wait 'till we have more time
    ** @END_OF_FIXME
    */

    pst_rdfcb_init(&rdf_cb, sess_cb);
    
    rdf_rb->rdr_2types_mask = RDR2_CNS_INTEG;
    rdf_rb->rdr_qrytuple    = (PTR) &integidx;
    STmove(cons_name, ' ', sizeof(rdf_rb->rdr_name.rdr_cnstrname),
	(char *) &rdf_rb->rdr_name.rdr_cnstrname);
    STRUCT_ASSIGN_MACRO(rngvar->pss_ownname, rdf_rb->rdr_owner);

    status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	{
	    /*
	    ** constraint with specified name does not exist in the specified
	    ** schema
	    */
	    (VOID) psf_error(E_PS048A_NO_SUCH_CONSTRAINT, (DB_ERROR *)NULL, 
                PSF_USERERR, &err_code, &psq_cb->psq_error, 2, 
                (i4) STlength(cons_name), (PTR) cons_name,
		psf_trmwhite(sizeof(rngvar->pss_ownname),
		    rngvar->pss_ownname.db_own_name),
		(PTR) rngvar->pss_ownname.db_own_name);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error,
		&psq_cb->psq_error);
	}

	return(status);
    }

    /*
    ** allocate and populate a PST_DROP_INTEGRITY statement node
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_STATEMENT),
	(PTR *) &snode, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) snode);

    snode->pst_mode = psq_cb->psq_mode;
    if (rngvar->pss_tabdesc->tbl_storage_type <= DB_STDING_STORE_MAX)
        snode->pst_type = PST_DROP_INTEGRITY_TYPE;
    else snode->pst_type = PST_X100_DROP_INTEGRITY_TYPE;

    drop_integ = &snode->pst_specific.pst_dropIntegrity;

    drop_integ->pst_flags = PST_DROP_ANSI_CONSTRAINT;
    if (drop_cascade)
	drop_integ->pst_flags |= PST_DROP_CONS_CASCADE;

    drop_cons = &drop_integ->pst_descr.pst_drop_cons_descr;

    STRUCT_ASSIGN_MACRO(rngvar->pss_tabname,	drop_cons->pst_cons_tab_name);
    STRUCT_ASSIGN_MACRO(rngvar->pss_ownname,	drop_cons->pst_cons_schema);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid,	drop_cons->pst_cons_tab_id);
    STRUCT_ASSIGN_MACRO(integidx.dbii_consname, drop_cons->pst_cons_name);
    STRUCT_ASSIGN_MACRO(integidx.dbii_consschema,
			drop_cons->pst_cons_schema_id);

    /* allocate and populate PST_PROCEDURE node */
    status = pst_ddl_header(sess_cb, snode, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* finita la comedia */
    return(E_DB_OK);
}

/*
** Name: psl_alt_tbl_col  - allocate and populate the DMU control block for
** 			    use by ALTER TABLE ADD / DROP COLUMN / RENAME 
**			    TABLE/COLUMN.
**
** Description:	    
**	
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	      tells whether this is a distributed thread or not
**	    pss_auxrng	      passed to rngent functions
**	    pss_user	      name of current user
**	    pss_ses_flag      holds PSS_CATUPD flag
**	    pss_lineno	      used for error messages
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode
**
** Outputs:
**	dmu_cb 		    ptr to DMU session CB
**	                      pointed to thru qeu_cb->qeu_d_cb    
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	22-jul-96 (ramra01 for bryanp)
**		Created.  Based loosely on psl_ct10_crt_tbl_kwd.
**	26-Feb-2004 (schka24)
**	    Set nparts in the new dmu cb.
**	11-sep-2009 (dougi)
**	    Don't allow for X100 tables.
**      30-oct-2009 (gefei01)
**          ALTER TABLE ADD COLUMN is unsupported on X100 table.
**	17-Nov-2009 (kschendel) SIR 122890
**	    resjour values expanded, fix here.
**	30-Mar-2009 (inkdo01, gupsh01)
**	    Add support for "rename column"
**	7-july-2010 (dougi)
**	    Remove X100 errors in prep for support of ALTER TABLE.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Change the way the DMU CB is set up.
*/
DB_STATUS
psl_alt_tbl_col(
	       PSS_SESBLK *sess_cb,
	       PSQ_CB	  *psq_cb)
{
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    PSS_RNGTAB	*rngvar;
    QEU_CB	*qeu_cb;
    DMU_CB	*dmu_cb;
    i4	err_code;
    bool	x100tab;
   

    rngvar = (PSS_RNGTAB *) sess_cb->pss_resrng;

    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_GATEWAY)
    {
       (void) psf_error(5557L, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, err_blk, 1,
			psf_trmwhite(DB_TAB_MAXNAME,
				     rngvar->pss_tabname.db_tab_name),
			rngvar->pss_tabname.db_tab_name);
       return (E_DB_ERROR);
    }

    if (rngvar->pss_tabdesc->tbl_status_mask & (DMT_CATALOG | DMT_EXTENDED_CAT))
    {
       (void) psf_error(9347L, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, err_blk, 1,
                        psf_trmwhite(DB_TAB_MAXNAME,
                                     rngvar->pss_tabname.db_tab_name),
                        rngvar->pss_tabname.db_tab_name);
       return (E_DB_ERROR);
    }

    x100tab = (rngvar->pss_tabdesc->tbl_storage_type > DB_STDING_STORE_MAX);

    /* allocate QEU_CB for ALTER_TABLE and init the header */

    /* The generated tree is useless, because only alter tbl 
    ** constraint mechanism uses it, so close the QTREE memory stream
    ** to delete memory. While the alter table column xlates to a
    ** DDL action.
    */
    status = psf_mclose(sess_cb, &sess_cb->pss_ostream, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
        return (status);
    sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;

    if ((psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN || 
        psq_cb->psq_mode == PSQ_ATBL_ALTER_COLUMN) && !x100tab)
       status = pst_crt_tbl_stmt(sess_cb, PSQ_CREATE, DMU_ALTER_TABLE,
    			         &psq_cb->psq_error);
    else if (psq_cb->psq_mode == PSQ_ATBL_RENAME_TABLE)
       status = pst_rename_stmt(sess_cb, PSQ_ATBL_RENAME_TABLE, DMU_ALTER_TABLE,
    			         &psq_cb->psq_error);
    else if (psq_cb->psq_mode == PSQ_ATBL_RENAME_COLUMN)
       status = pst_rename_stmt(sess_cb, PSQ_ATBL_RENAME_COLUMN, DMU_ALTER_TABLE,
    			         &psq_cb->psq_error);
    else
       status = psl_qeucb(sess_cb, DMU_ALTER_TABLE, &psq_cb->psq_error);

    if (status != E_DB_OK)
       return (status);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;

    /* allocate a DMU control block */

    if (qeu_cb->qeu_d_cb == NULL)
    {
       status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CB),
			   (PTR *) &dmu_cb, err_blk);
       if (DB_FAILURE_MACRO(status))
          return (status);
    }

    if (x100tab)
	qeu_cb->qeu_d_op = DMU_ALTER_TABLE;

    qeu_cb->qeu_d_cb = (PTR) dmu_cb;

    MEfill(sizeof(DMU_CB), 0, (PTR) dmu_cb);

    /* fill in the DMU control block header */
    dmu_cb->type = DMU_UTILITY_CB;
    dmu_cb->length = sizeof(DMU_CB);
    dmu_cb->dmu_db_id = sess_cb->pss_dbid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dmu_cb->dmu_owner);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, dmu_cb->dmu_tbl_id);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabname, dmu_cb->dmu_table_name);
    dmu_cb->dmu_nphys_parts = rngvar->pss_tabdesc->tbl_nparts;

    /* allocate attribute entry.  Only 1 (or 2 for RENAME COLUMN) required 
    ** as ALTER TABLE ADD/DROP COLUMN only supports altering one column 
    ** per statement. Not needed for alter table rename to ... statements.
    */

    if (psq_cb->psq_mode != PSQ_ATBL_RENAME_TABLE)
    {
      status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DMF_ATTR_ENTRY *) * 2,
			(PTR *) &dmu_cb->dmu_attr_array.ptr_address,
			err_blk);
      if (DB_FAILURE_MACRO(status))
       return (status);

    dmu_cb->dmu_attr_array.ptr_in_count = 0;
    dmu_cb->dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

    }
    else
    {
      dmu_cb->dmu_attr_array.ptr_in_count = 0;
      dmu_cb->dmu_attr_array.ptr_size = 0;
    }

    if (psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN)
	dmu_cb->dmu_action = DMU_ALT_ADDCOL;
    if (psq_cb->psq_mode == PSQ_ATBL_DROP_COLUMN)
	dmu_cb->dmu_action = DMU_ALT_DROPCOL;
    if (psq_cb->psq_mode == PSQ_ATBL_ALTER_COLUMN)
	dmu_cb->dmu_action = DMU_ALT_ALTERCOL;
    if (psq_cb->psq_mode == PSQ_ATBL_RENAME_TABLE)
	dmu_cb->dmu_action = DMU_ALT_TBL_RENAME;
    if (psq_cb->psq_mode == PSQ_ATBL_RENAME_COLUMN)
	dmu_cb->dmu_action = DMU_ALT_COL_RENAME;

    /* "cascade" is initially OFF thanks to zeroing the DMU CB */

    /* Allocate the location entries.  Assume DM_LOC_MAX */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    DM_LOC_MAX * sizeof(DB_LOC_NAME),
	    (PTR *) &dmu_cb->dmu_location.data_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb->dmu_location.data_in_size = 0;    /* Start with 0 locations */

    return(E_DB_OK);

} /* end psl_alt_tbl_col */

/*
** Name: psl_alt_tbl_col_drop - populate attribute entry in array for column   
** 			        to be dropped.                           
**
** Description:	    
**		
**      Builds a DMU_UTILITY_CB describing the column to be dropped.
**
**              alt_tbl DROP COLUMN newcolname drop_behavior
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	      tells whether this is a distributed thread or not
**	    pss_auxrng	      passed to rngent functions
**	    pss_user	      name of current user
**	    pss_ses_flag      holds PSS_CATUPD flag
**	    pss_lineno	      used for error messages
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode
**
** Outputs:
**	dmu_cb 		    ptr to DMU session CB
**	                      pointed to thru qeu_cb->qeu_d_cb    
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	22-jul-96 (ramra01 for bryanp)
**		Created.  
**	16-Oct-96 (ramra01)
**	   Restrict dropping the last column from the table
**	   when using alter table drop column command. 
**	15-nov-2006 (dougi)
**	    Don't allow drop of partitioning column.
**	7-july-2010 (dougi)
**	    Add support for X100 tables.
*/
DB_STATUS
psl_alt_tbl_col_drop(
	             PSS_SESBLK    *sess_cb,
	             PSQ_CB        *psq_cb,
		     DB_ATT_NAME   attname,
		     bool	   cascade)
{
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    QEU_CB	*qeu_cb;
    DMU_CB	*dmu_cb;
    DMF_ATTR_ENTRY **dmu_attr;
    DMT_ATT_ENTRY  *dmt_attr;
    RDF_CB	rdf_cb;
    i4	err_code;

    PSS_RNGTAB  *rngvar;

    rngvar = (PSS_RNGTAB *) sess_cb->pss_resrng;
    

    if (rngvar->pss_tabdesc->tbl_storage_type > DB_STDING_STORE_MAX)
	psq_cb->psq_mode = PSQ_X100_ALTABLE;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    dmu_attr = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

    dmt_attr = pst_coldesc(sess_cb->pss_resrng, 
		attname.db_att_name, DB_ATT_MAXNAME);

    if (dmt_attr == (DMT_ATT_ENTRY *) NULL)
    {
       (VOID) psf_error(2100L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
       			&psq_cb->psq_error, 4,
			(i4)sizeof(sess_cb->pss_lineno), 
			&sess_cb->pss_lineno,
			psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &sess_cb->pss_resrng->pss_tabname),
			(char *) &sess_cb->pss_resrng->pss_tabname,
			psf_trmwhite(sizeof(DB_OWN_NAME),
				    (char *) &sess_cb->pss_resrng->pss_ownname),
			(char *) &sess_cb->pss_resrng->pss_ownname,
			psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &attname),
			(char *) &attname);

       return (E_DB_ERROR);
    }

    if (rngvar->pss_tabdesc->tbl_attr_count <= 1)
    {
        (void) psf_error(3858L, (DB_ERROR *)NULL, PSF_USERERR, 
                         &err_code, err_blk, 0);
        return (E_DB_ERROR);
    }

    /* Can't drop 'tid' column */
    if (STcompare( dmt_attr->att_nmstr,
	      ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid")) == 0)
    {
        (void) psf_error(2012L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		dmt_attr->att_nmlen, dmt_attr->att_nmstr);
        return (E_DB_ERROR);
    }

    /* Verify column isn't used for partitioning. */
    status = psl_atbl_partcheck(sess_cb, psq_cb, dmt_attr);
    if (status != E_DB_OK)
       return (status);

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMF_ATTR_ENTRY), 
			(PTR *) &dmu_attr[0], err_blk);

    if (status != E_DB_OK)
       return (status);

    STmove((char *) &attname, ' ', sizeof(DB_ATT_NAME),
   	   (char *) &(dmu_attr[0])->attr_name);

    dmu_attr[0]->attr_type = dmt_attr->att_intlid;
    dmu_attr[0]->attr_size = dmt_attr->att_number;
    dmu_cb->dmu_attr_array.ptr_in_count = 1;

    if (cascade)
	BTset(DMU_CASCADE, dmu_cb->dmu_chars.dmu_indicators);

    /* Invalidate base table information from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
   			 rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
       (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return (status);
}

/*
** Name: psl_alt_tbl_col_add  - populate attribute entry in array for column   
** 			        to be added.                             
**
** Description:	    
**      Builds a DMU_UTILITY_CB describing the column to be added or altered.
**
**              alt_tbl ADD COLUMN newcolname typedesc
** 		or
**              alt_tbl ALTER COLUMN newcolname typedesc
**
**      The column name and type description will be filled in by the
**      newcolname and typedesc productions, which are shared with CREATE TABLE.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	      tells whether this is a distributed thread or not
**	    pss_auxrng	      passed to rngent functions
**	    pss_user	      name of current user
**	    pss_ses_flag      holds PSS_CATUPD flag
**	    pss_lineno	      used for error messages
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode
**
** Outputs:
**	dmu_cb 		    ptr to DMU session CB
**	                      pointed to thru qeu_cb->qeu_d_cb    
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	22-jul-96 (ramra01 for bryanp)
**		Created.  
**	17-dec-96 (inkdo01)
**	    Added code to place actual colno into pst_cons_list of create
**	    integrity parse tree structure (so iirule gets built right).
**	25-April-2003 (inifa01) Bug 110137 INGSRV 2233
**	    'alter table <table name> add <column name> <column type> with 
**	    default not null unique' causes 'E_SC0206 internal error' and a 
**	    SEGV at cui_idunorm() on a table where a column had previously 
**	    been dropped.
**	    We were computing and setting col_id.db_att_id to its attintl_id
**	    instead of its att_id. Fix: compute 'colno' based on 'tbl_attr_count'
**	    instead of 'tbl_attr_high_colno'.
**	13-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	19-Oct-2005 (penga03)
**	    Added check for alter table alter column that does not exist.
**	15-nov-2006 (dougi)
**	    Don't allow alter on partitioning column.
**	13-aug-2008 (dougi)
**	    Add altopts parm for identity columns.
**      15-Jan-2008 (horda03) Bug 121513
**          Don't change psq_mode to PSQ_CREATE, a syntax error will cause the wrong error
**          message to be displayed. Use US error 2032 rather than 2013 to report a
**          duplicate column error.
**	22-Mar-2010 (toumi01) SIR 122403
**	    Add encflags and encwid for encryption.
**	7-july-2010 (dougi)
**	    Add support for X100 tables.
**	9-Nov-2010 (kschendel)
**	    Change verify-cons call again.
**      24-may-2011 (horda03) b125118
**          altops = 3 implies ALTER TABLE ALTER COLUMN DROP IDENTITY
**          so remove the IDENTITY flags and set the default setting for the column
**          to the default setting.
*/
DB_STATUS
psl_alt_tbl_col_add(
	             PSS_SESBLK    *sess_cb,
	             PSQ_CB        *psq_cb,
		     DB_ATT_NAME   attname,
		     PSS_CONS	   *cons_list,
		     i4		   altopts)
{
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    QEU_CB	*qeu_cb;
    DMU_CB	*dmu_cb;
    DMF_ATTR_ENTRY **dmu_attr;
    DMT_ATT_ENTRY  *dmt_attr;
    PST_STATEMENT  *stmt_list;
    PST_CREATE_INTEGRITY *cr_integ;
    DB_COLUMN_BITMAP *integ_cols;
    RDF_CB	rdf_cb;
    i4	err_code;
    i4		colno;
    PSS_RNGTAB  *rngvar;
    PSQ_MODE	orig_psq_mode = psq_cb->psq_mode;

    rngvar = (PSS_RNGTAB *) sess_cb->pss_resrng;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    if (rngvar->pss_tabdesc->tbl_storage_type > DB_STDING_STORE_MAX)
    {
	i4 x100_error = 0;
	if (cons_list)
	{
	    /*
	    ** X100 cant add column and constraint in a single alter command 
	    ** Issue separate alter table add column, alter table add constraint
	    */
	    x100_error = 9468;
	}
	else if (psq_cb->psq_mode == PSQ_ATBL_ALTER_COLUMN)
	{
	    x100_error = 9470;
	}
	if (x100_error)
	{
	    (void)psf_error(x100_error, 0L, PSF_USERERR, &err_code,
                        &psq_cb->psq_error, 1,
                        psf_trmwhite(DB_TAB_MAXNAME,
                                     rngvar->pss_tabname.db_tab_name),
                        rngvar->pss_tabname.db_tab_name);
	    return (E_DB_ERROR);
	}
	psq_cb->psq_mode = PSQ_X100_ALTABLE;
    }

    dmu_attr = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

    dmt_attr = pst_coldesc(sess_cb->pss_resrng, 
			attname.db_att_name, DB_ATT_MAXNAME);

    if ((psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN) &&
	(dmt_attr != (DMT_ATT_ENTRY *) NULL))
    {
       (VOID) psf_error(2032L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
                        sizeof( "ALTER TABLE"), "ALTER TABLE",
			psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &attname),
			(char *) &attname);

       return (E_DB_ERROR);
    }
    else if (psq_cb->psq_mode == PSQ_ATBL_ALTER_COLUMN)
    {
	if (dmt_attr == (DMT_ATT_ENTRY *) NULL)
	{
	    (VOID) psf_error(2100L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
       			&psq_cb->psq_error, 4,
			(i4)sizeof(sess_cb->pss_lineno), 
			&sess_cb->pss_lineno,
			psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &sess_cb->pss_resrng->pss_tabname),
			(char *) &sess_cb->pss_resrng->pss_tabname,
			psf_trmwhite(sizeof(DB_OWN_NAME),
				    (char *) &sess_cb->pss_resrng->pss_ownname),
			(char *) &sess_cb->pss_resrng->pss_ownname,
			psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &attname),
			(char *) &attname);

	    return (E_DB_ERROR);
	}
	else if (altopts != PSL_ALT_COL_NONE)
	{
	    if ((dmt_attr->att_flags & (DMU_F_IDENTITY_ALWAYS |
					DMU_F_IDENTITY_BY_DEFAULT)) == 0)
	    {
		(VOID) psf_error(E_PS04B9_ALTER_IDENTITY, (DB_ERROR *)NULL, 
                    PSF_USERERR, &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &attname),
		    (char *) &attname);
		return(E_DB_ERROR);
	    }

	    /* Copy DMT_ATT_ENTRY contents to DMF_ATTR_ENTRY - then new
	    ** identity flag. */
	    (*dmu_attr)->attr_type = dmt_attr->att_type;
	    (*dmu_attr)->attr_size = dmt_attr->att_width;
	    (*dmu_attr)->attr_precision = dmt_attr->att_prec;
	    (*dmu_attr)->attr_flags_mask = dmt_attr->att_flags;
	    STRUCT_ASSIGN_MACRO(dmt_attr->att_defaultID, 
					(*dmu_attr)->attr_defaultID);
	    (*dmu_attr)->attr_collID = dmt_attr->att_collID;
	    (*dmu_attr)->attr_geomtype = dmt_attr->att_geomtype;
	    (*dmu_attr)->attr_srid = dmt_attr->att_srid;
	    (*dmu_attr)->attr_encflags = dmt_attr->att_encflags;
	    (*dmu_attr)->attr_encwid = dmt_attr->att_encwid;
	    (*dmu_attr)->attr_defaultTuple = NULL;
	    (*dmu_attr)->attr_seqTuple = NULL;
	    switch (altopts) {
	      case PSL_ALT_COL_GENALWAYS:
		(*dmu_attr)->attr_flags_mask &= ~DMU_F_IDENTITY_BY_DEFAULT;
		(*dmu_attr)->attr_flags_mask |= DMU_F_IDENTITY_ALWAYS;
		break;

	      case PSL_ALT_COL_GENDEFAULT:
		(*dmu_attr)->attr_flags_mask &= ~DMU_F_IDENTITY_ALWAYS;
		(*dmu_attr)->attr_flags_mask |= DMU_F_IDENTITY_BY_DEFAULT;
		break;

	      case PSL_ALT_COL_DROPIDENTITY:
		(*dmu_attr)->attr_flags_mask &= ~(DMU_F_IDENTITY_ALWAYS|DMU_F_IDENTITY_BY_DEFAULT);
		status = psl_2col_ingres_default( sess_cb, *dmu_attr, err_blk );
		if (status != E_DB_OK)
		   return (status);
		break;
	    }
	}

	/* Verify column isn't used for partitioning. */
	status = psl_atbl_partcheck(sess_cb, psq_cb, dmt_attr);
	if (status != E_DB_OK)
	    return (status);

	/* verify that if LOBs are involved, it's ok */
	status = psl_atbl_alter_lob(sess_cb, psq_cb, dmt_attr, *dmu_attr);
	if (status != E_DB_OK)
	    return (status);
    }

    /*
    ** For X100 tables ensure that collation is set to UCS_BASIC 
    ** and that datatype is valid VectorWise datatype
    */
    if (rngvar->pss_tabdesc->tbl_storage_type > DB_STDING_STORE_MAX &&
	    (orig_psq_mode == PSQ_ATBL_ALTER_COLUMN
	    || orig_psq_mode == PSQ_ATBL_ADD_COLUMN))
    {
	status = psl_check_x100_att(sess_cb, psq_cb, 
		&dmu_cb->dmu_table_name, *dmu_attr);
	if (status)
	    return (E_DB_ERROR);
    }

    if (cons_list != (PSS_CONS *) NULL)
    {
       status = psl_verify_cons(sess_cb, psq_cb, dmu_cb,
				(PSS_RNGTAB *) sess_cb->pss_resrng,
				cons_list, &stmt_list);

       if (DB_FAILURE_MACRO(status))
	  return(status);

	/* This bit of code repairs both the Create Integrity structure 
	** sent to QEF for execution, and the iiintegrity tuple which is
	** stored on the catalog. Because we're pretending to be creating
	** a one column table here (rather than adding a col to the 
	** existing table), the column number recorded in the integrity
	** definition is 1, rather than "n+1" (where "n" is the number of
	** columns already in the table). This code fixes that. 
	*/
       colno = 1;
       cr_integ = &stmt_list->pst_specific.pst_createIntegrity;
       integ_cols = &cr_integ->pst_integrityTuple->dbi_columns;
       BTclear(colno, (char *) integ_cols);
       colno = (sess_cb->pss_resrng->pss_tabdesc->tbl_attr_count) + 1;
       BTset(colno, (char *) integ_cols);
       cr_integ->pst_cons_collist->col_id.db_att_id = colno;

       /* allocate the proc header for the query tree
       ** and set the root of the QSF object to the head of the tree
       */

       status = pst_attach_to_tree(sess_cb, stmt_list, err_blk);

       if (DB_FAILURE_MACRO(status))
	   return(status);
    }

    if (psq_cb->psq_mode != PSQ_X100_ALTABLE)
	psq_cb->psq_mode = PSQ_ALTERTABLE;

    /* Invalidate base table information from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
   			 rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
       (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return (status);
}

/*
** Name: psl_alt_tbl_col_rename - populate attribute entry in array 
**				for column to be renamed and the new 
**				column name.                             
**
** Description:	    
**      Builds a DMU_UTILITY_CB describing the column to be renamed.
**
**              alt_tbl RENAME COLUMN newcolname typedesc
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	      tells whether this is a distributed thread or not
**	    pss_auxrng	      passed to rngent functions
**	    pss_user	      name of current user
**	    pss_ses_flag      holds PSS_CATUPD flag
**	    pss_lineno	      used for error messages
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode
**	attname		    Name of the attribute to be renamed.
**	newattname	    New name of the attribute.
**
** Outputs:
**	dmu_cb 		    ptr to DMU session CB
**	                      pointed to thru qeu_cb->qeu_d_cb    
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	10-apr-2010 (gupsh01)
**	    Created.  
**	21-apr-2010 (gupsh01)
**	    Fixed the variable declaration for attr0 and attr1.
*/
DB_STATUS
psl_alt_tbl_col_rename(
	             PSS_SESBLK    *sess_cb,
	             PSQ_CB        *psq_cb,
		     DB_ATT_NAME   attname,
		     DB_ATT_NAME   *newattname)
{
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    QEU_CB	*qeu_cb;
    DMU_CB	*dmu_cb;
    DMF_ATTR_ENTRY **dmu_attr;
    DMT_ATT_ENTRY  *dmt_attr;
    DMT_ATT_ENTRY  *newdmt_attr;
    RDF_CB	rdf_cb;
    i4		err_code;
    PST_PROCEDURE 	*proc_node; 
    PST_STATEMENT  	*rename_stmt; 
    PST_RENAME		*pst_rename; 
    DB_ATT_ID		att_id;
    DMF_ATTR_ENTRY	*attr0;
    DMF_ATTR_ENTRY 	*attr1;
    
    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    dmu_attr = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

    dmt_attr = pst_coldesc(sess_cb->pss_resrng, 
		attname.db_att_name, DB_ATT_MAXNAME);

    if (dmt_attr == (DMT_ATT_ENTRY *) NULL)
    {
       	/* Column to be renamed does not exist */ 
	(VOID) psf_error(2100L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
       			&psq_cb->psq_error, 4,
			(i4)sizeof(sess_cb->pss_lineno), 
			&sess_cb->pss_lineno,
			psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &sess_cb->pss_resrng->pss_tabname),
			(char *) &sess_cb->pss_resrng->pss_tabname,
			psf_trmwhite(sizeof(DB_OWN_NAME),
				    (char *) &sess_cb->pss_resrng->pss_ownname),
			(char *) &sess_cb->pss_resrng->pss_ownname,
			psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &attname),
			(char *) &attname);

	return (E_DB_ERROR);
    }

    newdmt_attr = pst_coldesc(sess_cb->pss_resrng, 
			newattname->db_att_name, DB_ATT_MAXNAME);

    if ( newdmt_attr != (DMT_ATT_ENTRY *) NULL ) 
    {
       /* Column with the same name already exists */
       (VOID) psf_error(2032L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
                        sizeof( "ALTER TABLE"), "ALTER TABLE",
			psf_trmwhite(sizeof(DB_ATT_NAME), (char *)newattname),
			(char *) newattname);

       return (E_DB_ERROR);
    }

    /* Verify column isn't used for partitioning. */
    status = psl_atbl_partcheck(sess_cb, psq_cb, dmt_attr);
    if (status != E_DB_OK)
        return (status);

    /* NOW Init the dummy attribute for passing the new column name.*/
    attr0 = dmu_attr[0];
    attr1 = dmu_attr[1];

    /* Copy the name of the attribute. */
    MEcopy(newattname->db_att_name, (u_i4)DB_ATT_MAXNAME, attr1->attr_name.db_att_name);

    attr1->attr_type = attr0->attr_type = dmt_attr->att_type;     
    attr1->attr_size = attr0->attr_size = dmt_attr->att_number;     
    attr1->attr_precision = attr0->attr_precision = dmt_attr->att_prec;   
    attr1->attr_flags_mask = attr0->attr_flags_mask = dmt_attr->att_flags;     
    attr1->attr_defaultTuple = attr0->attr_defaultTuple = NULL;
    attr1->attr_seqTuple = attr0->attr_seqTuple = NULL;   
    attr1->attr_collID = attr0->attr_collID = dmt_attr->att_collID; 
    attr1->attr_srid   = attr0->attr_srid = dmt_attr->att_srid;    
    attr1->attr_geomtype = attr0->attr_geomtype = dmt_attr->att_geomtype; 

    proc_node = (PST_PROCEDURE *) sess_cb->pss_qsf_rcb.qsf_root;
    rename_stmt = proc_node->pst_stmts;
    pst_rename = &rename_stmt->pst_specific.pst_rename;

    att_id.db_att_id = dmt_attr->att_intlid;

    STRUCT_ASSIGN_MACRO(att_id, 	pst_rename->pst_rnm_col_id);
    STRUCT_ASSIGN_MACRO(attname, 	pst_rename->pst_rnm_old_colname);
    STRUCT_ASSIGN_MACRO(*newattname, 	pst_rename->pst_rnm_new_colname);
    STRUCT_ASSIGN_MACRO(attname, 	pst_rename->pst_rnm_old_colname);
    STRUCT_ASSIGN_MACRO(*newattname, 	pst_rename->pst_rnm_new_colname);

    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, pst_rename->pst_rnm_owner);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname, pst_rename->pst_rnm_old_tabname);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname, pst_rename->pst_rnm_new_tabname);

    (*dmu_attr)->attr_defaultTuple = NULL;
    (*dmu_attr)->attr_seqTuple = NULL;
    psq_cb->psq_mode = PSQ_ALTERTABLE;

    /* Invalidate base table information from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
   			 rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
       (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return (status);
}

/*
** Name: psl_alt_tbl_rename - handles rename table action.
**
** Description:	    
**	Populate the new table name in the DMU_CB control block and the
**	PST_RENAME statement.
**
**		alt_tbl RENAME TO obj_spec
**		rename_tbl TO obj_spec
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_lineno	      used for error messages
**	    
**	psq_cb		    PSF request CB
**	newTabName	    New name of the table.
**
** Outputs:
**	dmu_cb 		    ptr to DMU session CB
**	                      pointed to thru qeu_cb->qeu_d_cb    
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	10-apr-2010 (gupsh01) SIR 123444
**	    Created.  
*/
DB_STATUS
psl_alt_tbl_rename(
	             PSS_SESBLK    *sess_cb,
	             PSQ_CB        *psq_cb,
		     PSS_OBJ_NAME  *newTabName)
{
    DB_STATUS		status;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    RDF_CB		rdf_cb;
    PST_PROCEDURE 	*proc_node;
    PST_STATEMENT  	*rename_stmt;
    PST_RENAME		*pst_rename;

    proc_node = (PST_PROCEDURE *) sess_cb->pss_qsf_rcb.qsf_root;
    rename_stmt = proc_node->pst_stmts;
    pst_rename = &rename_stmt->pst_specific.pst_rename;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;


    if ((psq_cb->psq_mode != PSQ_ATBL_RENAME_TABLE) &&
    	(pst_rename->pst_rnm_type != PST_ATBL_RENAME_TABLE))
    {
	return (E_DB_ERROR); 	/* not the right mode for this routine */
    }

    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, pst_rename->pst_rnm_owner);
    STRUCT_ASSIGN_MACRO(newTabName->pss_obj_name, pst_rename->pst_rnm_new_tabname);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname, pst_rename->pst_rnm_old_tabname);

    /* Fill the new table name in the DMU control block */
    STRUCT_ASSIGN_MACRO(newTabName->pss_obj_name, dmu_cb->dmu_newtab_name);

    psq_cb->psq_mode = PSQ_ALTERTABLE;

    /* Invalidate base table information from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
   			 rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
       (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return (status);
}

/*
** Name: psl_atbl_partcheck	- checks for drop/alter of partitioning column
**
** Description:	    
**      If target table is partitioned, searches partitioning column for
**	target column and issues error, if found.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_resrng	      target table descriptor
**	psq_cb		    PSF request CB
**
** Outputs:
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	15-nov-2006 (dougi)
**	    Written to validate partitioned tables.
*/
static DB_STATUS
psl_atbl_partcheck(
	PSS_SESBLK    *sess_cb,
	PSQ_CB        *psq_cb,
	DMT_ATT_ENTRY *dmt_attr)

{
    PSS_RNGTAB	*rngvar;
    i4		err_code;
    i4		i, j;


    /* First see if table is partitioned. */
    if ((rngvar = sess_cb->pss_resrng)->pss_rdrinfo->rdr_parts == NULL ||
	rngvar->pss_rdrinfo->rdr_parts->ndims < 1)
	return(E_DB_OK);

    /* Now loop over partitions and columns looking for our guy. */
    for (i = 0; i < rngvar->pss_rdrinfo->rdr_parts->ndims; i++)
     for (j = 0; j < rngvar->pss_rdrinfo->rdr_parts->dimension[i].ncols; j++)
      if (rngvar->pss_rdrinfo->rdr_parts->dimension[i].part_list[j].
				att_number == dmt_attr->att_number)
      {
	/* Got a match - not allowed. */
	(VOID) psf_error(2131L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
       			&psq_cb->psq_error, 4,
			(i4)sizeof(sess_cb->pss_lineno), 
			&sess_cb->pss_lineno,
			psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname,
			psf_trmwhite(sizeof(DB_OWN_NAME),
				    (char *) &rngvar->pss_ownname),
			(char *) &rngvar->pss_ownname,
			psf_trmwhite(sizeof(DB_ATT_NAME), 
				dmt_attr->att_nmstr),
			dmt_attr->att_nmstr);

	return (E_DB_ERROR);
      }

    return(E_DB_OK);
}

/*
** Name: psl_atbl_alter_lob - Check for LOB's and alter column
**
** Description:
**	Alter table alter column is not permitted if either the new or old
**	types are LOB types -- *unless* the type change is innocuous in the
**	sense that underlying etab segments are untouched.  This basically
**	means a null alter (e.g. long varchar -> long varchar), or
**	(the important one) an SRID-changing alter.
**
**	It's plausible to allow long varchar <--> long byte here, as
**	the two are essentially equivalent;  however the underlying
**	etab segment types are different, and allowing this would lead
**	to a discrepancy between the base table and the etab.  I don't
**	thing anything checks, and this restriction could be relaxed
**	if necessary / desirable.
**
**	The reason that LOB types aren't alter-able is that DMF can't
**	deal with it on-the-fly.  Consider a long varchar -> varchar alter,
**	if a row were re-written with the (new) varchar value, something
**	has to delete the etab rows belonging to the original row value.
**	There is no linkage to detect or allow that.
**
** Inputs:
**	sess_cb		Session parser control block
**	psq_cb		Query parse control block
**	dmt_att		Pointer to current attribute description
**	dmu_att		Pointer to new attribute description
**
** Outputs:
**	Returns E_DB_OK or error status.
**
** History:
**	6-Jun-2010 (kschendel) b123923
**	    Prevent alter table alter column with lobs, except for degenerate
**	    and SRID cases.
*/

static DB_STATUS
psl_atbl_alter_lob(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	DMT_ATT_ENTRY *dmt_att, DMF_ATTR_ENTRY *dmu_att)
{
    DB_STATUS status;
    i4 err_code;
    i4 old_bits, new_bits;	/* Data type info bits */

    status = adi_dtinfo(sess_cb->pss_adfcb, dmt_att->att_type, &old_bits);
    if (status != E_DB_OK)
	return (status);
    status = adi_dtinfo(sess_cb->pss_adfcb, dmu_att->attr_type, &new_bits);
    if (status != E_DB_OK)
	return (status);
    if ((old_bits & AD_PERIPHERAL) == 0 && (new_bits & AD_PERIPHERAL) == 0)
	return (E_DB_OK);

    /* One or the other is a blob.  If both are, check the types to make
    ** sure they are the same.  If they are, allow the alter.  Any other
    ** combination rejects the alter.
    */
    if (old_bits & AD_PERIPHERAL && new_bits & AD_PERIPHERAL
      && (dmt_att->att_type == dmu_att->attr_type
      || abs(dmt_att->att_type) == DB_GEOM_TYPE
      || abs(dmu_att->attr_type) == DB_GEOM_TYPE))
	return (E_DB_OK);

    (void) psf_error(3859, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
                     &psq_cb->psq_error, 0);
    return (E_DB_ERROR);
}
