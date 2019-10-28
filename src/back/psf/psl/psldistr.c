/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <cui.h>
#include    <adf.h>
#include    <adfops.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
/*
** Name: PSLDISTR.C:	this file contains semantic actions for the
**			register [table|view] statement as well as a
**			function to evaluate constant functions in
**			SQL queries..
**
** Description:
**	This module is used by the distributed thread for processing
**	the REGISTER and CREATE LINK statements.  It is also used by
**	the distributed CREATE TABLE to generate text for iiregistrations.
**	The function to evaluate dbmsinfo() and other constant functions
**	is also in this module (for want of a better place to put it.)
**
**	Routines:
**	    psl_rg1_reg_distr_tv  - semantic actions for reg_distr_tv production
**	    psl_rg2_reg_distr_idx - semantic actions for reg_distr_idx product'n
**	    psl_rg3_reg_tvi	  - semantic actions for reg_kwd and reg_idx
**				    productions
**	    psl_rg4_regtext	  - generate and save text for iiregistrations
**	    psl_rg5_ldb_spec	  - semantic actions for ldb_spec: production
**	    psl_rg6_link_col_list - semantic actions for the link_col_list prod
**	    psl_ds1_dircon	  - semantic actions for DIRECT CONNECT
**	    psl_ds2_dir_exec_immed- semantic actions for DIRECT EXEC IMMED
**	    psl_proc_func	  - semantic action for scalar_func rule.
**
** History:
**	04-jun-1992 (barbara)
**	    Created as part of the Sybil project of merging Star code
**	    into the 6.5 rplus line.  The functions consist of code extracted
**	    from the productions for the REGISTER, CREATE LINK and CREATE
**	    TABLE statements (from the 6.4 Star grammar); also from the
**	    scalar_func rule.
**	03-aug-1992 (barbara)
**	    Completed support for Sybil.  Added psl_rg6_link_col_list; extended
**	    other functions to handle CREATE LINK.  Fixed various bugs arising
**	    from preliminary Star testing.
**	13-jan-1993 (barbara)
**	    Added return values to functions that were previously falling
**	    off the end w/out returns.
**	10-feb-93 (rblumer)
**	    check for new DISTCREATE query mode when setting up ddl_info
**	01-mar-93 (barbara)
**	   Added delimited id support for Star.
**	29-jun-93 (andre)
**	    #included CV.H, ME.H, and ST.H
**      12-Jul-1993 (fred)
**          Disallow registration of extension tables.  No useful purpose...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	26-nov-93 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to psy_owner whose type has
**	    changed.
**	16-mar-94 (kirke)
**	    Typo: cb->pss_q_buf in psl_rg2_reg_distr_idx() should really be
**	    sess_cb->pss_qbuf.  This is inside a CMbytecnt macro so single
**	    byte builds don't notice it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    pst_ldbtab_desc().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	7-Feb-2010 (kschendel) b124962
**	    Fix psl-proc-func for ANSI current-time keywords (COPY defaults).
**	    Fix a few MEcopy -> memcpy while I'm in here.
**       4-Jul-2011 (hanal04) SIR 125458
**          Add DB_ERROR support (__FILE__, __LINE__) to PSF and to user
**          error functions in SCF.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/

/* TABLE OF CONTENTS */
i4 psl_rg1_reg_distr_tv(
	PSS_SESBLK *sess_cb,
	PSS_OBJ_NAME *reg_name,
	i4 ldb_obj_type,
	PSS_Q_XLATE *xlated_qry,
	PSQ_CB *psq_cb);
i4 psl_rg2_reg_distr_idx(
	PSS_SESBLK *sess_cb,
	char *reg_name,
	PSS_Q_XLATE *xlated_qry,
	DB_ERROR *err_blk);
i4 psl_rg3_reg_tvi(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_rg4_regtext(
	PSS_SESBLK *sess_cb,
	i4 ldb_obj_type,
	PSS_Q_XLATE *xlated_qry,
	PSQ_CB *psq_cb);
i4 psl_rg5_ldb_spec(
	PSS_SESBLK *sess_cb,
	char *name,
	i4 qmode,
	DB_ERROR *err_blk);
i4 psl_rg6_link_col_list(
	char *colname,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_ds1_dircon(
	PSQ_CB *psq_cb,
	char *name1,
	char *name2,
	bool is_distrib);
i4 psl_ds2_dir_exec_immed(
	char *name,
	bool is_distrib,
	char *exec_arg,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_proc_func(
	PSS_SESBLK *sess_cb,
	PSF_MSTREAM *stream,
	i4 op_code,
	PST_QNODE *arg,
	PST_QNODE **newnode,
	DB_ERROR *err_blk);

/*
** Name: psl_rg1_reg_distr_tv	- semantic actions for reg_distr_tv production
**
** Description:
**	The reg_distr_tv production is the highest-level production for the
**	Star REGISTER and CREATE LINK statements.  It does various semantic
**	checking for the statement.  It enters info about the registration
**	into the QED_DDL_INFO block.  It generates text for iiregistrations
**	and saves inside the QED_DDL_INFO block.  For REGISTER WITH REFRESH
**	it allocates and fills in a PSY_CB.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    memory stream
**	    pss_stmt_flags  statement specific flags
**			      PSS_REG_AS_NATIVE indicates that registration
**			      is NATIVE (as opposed to LINK)
**	    pss_ses_flag    session specific flags
**			      PSS_CATUPD indicates session running with
**			      catalog update privilege
**	    pss_auxrng	    range table
**	    pss_object	    points to a QED_DDL_INFO block.
**	    pss_distr_sflags  WITH-clause info
**	reg_name	    name of table to be registered (possible with owner)
**	ldb_obj_type	    type of underlying LDB object
**	xlated_qry	    for query text buffering
**	psq_cb		    ptr to parser control block.
**	  
** Outputs:
**	sess_cb
**	    pss_object	    for REGISTER WITH REFRESH, will be changed to
**			    point to a PSY_CB.  The QED_DDL_INFO will be
**			    hung off the PSY_CB.
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**	    psq_mode	    reset if REGISTER WITH REFRESH
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	04-jun-92 (barbara)
**	    Mostly copied from the Star 6.4 SQL grammar
**	06-jul-92 (barbara)
**	    Allow REGISTER TABLE [owner.]reg_tab_name for consistency
**	    with gateway register statement.  Change name of function
**	    in keeping with changed names of rules in grammar.
**	15-oct-92 (barbara)
**	    Fixed up buggy comparison of "$ingres" to pss_ownname.
**	16-nov-92 (barbara)
**	    If registered object exists and is a catalog, users with
**	    catalog update privilege should get regular error message about
**	    table already existing.
**	16-nov-92 (teresa)
**	    Added support for registered procedures.
**	14-dec-92 (tam)
**	    Changed call to psl0_rngent() so that rdf will search
**	    both table and procedure name space.
**	05-jan-93 (tam)
**	    Changed error reporting to cater for reg proc.
**	12-jan-93 (tam)
**	    Remapped error 2036L to E_PS1206_REGISTER to cater for REGISTER
**	    PROCEDURE syntax.
**	01-mar-93 (barbara)
**	    Delimited id support for Star.  Force a mapping of column names
**	    when the DDB is case insensitive and the LDB is case sensitive.
**	22-apr-93 (barbara)
**	    When checking for system catalogs, do case insensitive comparisons
**	    for "ii" prefix and "$ingres".
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use case insensitive "ii" when detecting catalog name.
**	28-may-93 (barbara)
**	    Removed most of the change of 01-mar-93.  That change made
**	    PSF provide a mapping of LDB->DDB column names for the table
**	    being registered.  QEF was to use this list to map names obtained
**	    from iitables, etc. into iidd_tables, etc.  But that scheme didn't
**	    work for statements like CREATE foo AS SELECT * FROM otherfoo.  In
**	    that case PSF cannot provide the mapping because it has no
**	    information on the column names of foo because it hasn't been
**	    created yet.  So the mapping logic (except for user-mapped column
**	    names) now resides in QEF.
**      12-Jul-1993 (fred)
**          Disallow registration of extension tables.  No useful purpose...
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with 
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	02-sep-93 (barbara)
**	    Case translation of owner names when looking up LDB object:  Do not
**	    set flag PSS_GET_TBLCASE unless an owner name has been specified on
**	    the FROM clause.  If there is no owner name, RDF will supply one
**	    from a catalog lookup and place it in qed_d2_obj_owner.  But since
**	    RDF is called twice from pst_ldbtab_desc, the PSS_GET_TBLCASE
**	    causes the second call to RDF to case translate (sometimes
**	    incorrectly) the valid user name RDF supplied on the first call.
**	05-nov-93 (barbara)
**	    When registering objects in the name of "$ingres", use the
**	    pss_cat_owner field instead of hard-coded "$ingres" because
**	    of case considerations. (bug 56670)
**	23-jun-97 (nanpr01)
**	    Fixup the error parameters.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent for WITH support.
*/
DB_STATUS
psl_rg1_reg_distr_tv(
	PSS_SESBLK	*sess_cb,
	PSS_OBJ_NAME	*reg_name,
	i4		ldb_obj_type,
	PSS_Q_XLATE	*xlated_qry,
	PSQ_CB		*psq_cb)
{
    PSS_RNGTAB		*resrange;
    DB_TAB_NAME		tabname;
    QED_DDL_INFO	*ddl_info	= (QED_DDL_INFO *) sess_cb->pss_object;
    DD_2LDB_TAB_INFO	*ldb_tab_info	= ddl_info->qed_d6_tab_info_p;
    char		*rngvar_name;
    i4			rngvar_info;
    i4			lookup_mask;
    char		*ch2 = reg_name->pss_orig_obj_name 
				+ CMbytecnt(reg_name->pss_orig_obj_name);
    i4			tbls_to_lookup = PSS_USRTBL;
    i4	        err_code;
    DB_STATUS		status;
    DB_ERROR		*err_blk = &psq_cb->psq_error;

    /*
    ** If "WITH REFRESH" was specified, we may NOT have
    ** "REGISTER TABLE|VIEW|PROCEDURE", 'column_list', "FROM source", 
    ** or "AS NATIVE"
    */
    if (   sess_cb->pss_distr_sflags & PSS_REFRESH
	&& (   
		/* with-clause bits present */
	       sess_cb->pss_distr_sflags &
			(PSS_NODE|PSS_DB|PSS_DBMS|PSS_LDB_TABLE)
	    || ldb_obj_type > 0
	    || ldb_tab_info->dd_t6_mapped_b
	    || sess_cb->pss_stmt_flags & PSS_REG_AS_NATIVE)
	)
    {
	(VOID) psf_error(E_PS1206_REGISTER, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, err_blk, 2, (i4) sizeof(sess_cb->pss_lineno),
			&(sess_cb->pss_lineno), sizeof("REFRESH") - 1, "REFRESH");
	return (E_DB_ERROR);
    }

    /*
    ** if AS NATIVE was specified, we MUST have REGISTER TABLE, and we
    ** should not have column mapping
    */
    if (   sess_cb->pss_stmt_flags & PSS_REG_AS_NATIVE
	&& (ldb_obj_type != DD_2OBJ_TABLE || ldb_tab_info->dd_t6_mapped_b)
	)
    {
	(VOID) psf_error(E_PS1206_REGISTER, (DB_ERROR *)NULL, PSF_USERERR, 
                &err_code, err_blk, 2, (i4) sizeof(sess_cb->pss_lineno),
		&(sess_cb->pss_lineno), sizeof("NATIVE") - 1, "NATIVE");
	return (E_DB_ERROR);
    }

    if (!CMcmpnocase(reg_name->pss_orig_obj_name, "i") &&
	!CMcmpnocase(ch2, "i"))
    {
	if (~sess_cb->pss_ses_flag & PSS_CATUPD)
    	/*
    	** can only have registrations beginning with 'II' if running with
    	** system catalog update privilege.
    	*/
    	{
	    if (ldb_obj_type == DD_5OBJ_REG_PROC)
	    {
		(VOID) psf_error(2432L, (DB_ERROR *)NULL, PSF_USERERR, 
                    &err_code, err_blk, 1, 
                    STlength(reg_name->pss_orig_obj_name),
                    reg_name->pss_orig_obj_name);
	    }
	    else
	    {
	    	(VOID) psf_error(5116L, (DB_ERROR *)NULL, PSF_USERERR, 
                    &err_code, err_blk, 3, sizeof("REGISTER") - 1, "REGISTER",
		    STlength(reg_name->pss_orig_obj_name),
		    reg_name->pss_orig_obj_name,
		    STlength(SystemCatPrefix), SystemCatPrefix);
	    }

	    return (E_DB_ERROR);
	}

	/*
	** If user name was specified it should be "$ingres" (or "$INGRES")
	*/
	if (   reg_name->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA
	    && (STncasecmp("$ingres", (PTR)&reg_name->pss_owner, 7) != 0))
	{
	    char 	*c;

	    if (ldb_obj_type == DD_5OBJ_REG_PROC)
	    {
		c = "REGISTER PROCEDURE";
	    }
	    else
	    {
		c = "REGISTER TABLE";
	    }

	    (VOID) psf_error(E_PS0421_CRTOBJ_NOT_OBJ_OWNER, (DB_ERROR *)NULL,
                PSF_USERERR, &err_code, &psq_cb->psq_error, 2,
		STlength(c), c,
		psf_trmwhite(sizeof(DB_OWN_NAME),
		    (char *) &reg_name->pss_owner),
		&reg_name->pss_owner);
	    return (E_DB_ERROR);
	}
	/* 
	** 8/7/90 (andre)
	** if registering an object with a name starting with "ii"
	** change the name of the Owner of the DDB object to be
	** created to $ingres
	*/
	memcpy(ddl_info->qed_d2_obj_owner, sess_cb->pss_cat_owner,
		sizeof(DD_OWN_NAME));

	/* Lookup "ii" tables as $ingres only */

	tbls_to_lookup = PSS_INGTBL;
    }
    else
    {
	/*
	** If name was qualified, it'd better be the same as the user.
	*/
	if (   reg_name->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA
	    && MEcmp((PTR) &reg_name->pss_owner, (PTR) &sess_cb->pss_user,
		sizeof(DB_OWN_NAME)))
	{
	    char	*c;

	    if (ldb_obj_type == DD_5OBJ_REG_PROC)
	    {
		c = "REGISTER PROCEDURE";
	    }
	    else
	    {
		c = "REGISTER TABLE";
	    }
	    (VOID) psf_error(E_PS0421_CRTOBJ_NOT_OBJ_OWNER, (DB_ERROR *)NULL,
                PSF_USERERR, &err_code, &psq_cb->psq_error, 2,
		STlength(c), c,
		psf_trmwhite(sizeof(DB_OWN_NAME),
		    (char *) &reg_name->pss_owner),
		&reg_name->pss_owner);
	    return (E_DB_ERROR);
	}
    }

    if (sess_cb->pss_distr_sflags & PSS_REFRESH)
    {
	lookup_mask = 0;
	rngvar_name = reg_name->pss_orig_obj_name;;
	psq_cb->psq_mode = PSQ_REREGISTER;
    }
    else
    {
	/* PST_FULLSEARCH searches table and procedure name space */
	lookup_mask = PST_SHWNAME | PST_CHECK_EXIST | PST_FULLSEARCH;
	rngvar_name = "";
    }

    STmove(reg_name->pss_orig_obj_name, ' ', sizeof(tabname),
			tabname.db_tab_name);

    status = psl0_rngent(&sess_cb->pss_auxrng, -1, rngvar_name, &tabname,
	sess_cb, TRUE, &resrange, psq_cb->psq_mode, err_blk,
	tbls_to_lookup, &rngvar_info, lookup_mask, NULL);


    if (sess_cb->pss_distr_sflags & PSS_REFRESH)
    {
	DD_1LDB_INFO		*save_ptr;
	RDR_INFO		*rdr_info;
	DD_OBJ_DESC		*obj_desc;
	DD_2LDB_TAB_INFO	*refr_ldb_tab_info;

	if (err_blk->err_code == E_PS0903_TAB_NOTFOUND)
	{
	    (VOID) psf_error(2753L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
                err_blk, 2, sizeof("REGISTER ... WITH REFRESH") - 1,
		"REGISTER ... WITH REFRESH",
		STlength(reg_name->pss_orig_obj_name),
		reg_name->pss_orig_obj_name);
	    return (E_DB_ERROR);
	}
        else if (status != E_DB_OK)
	{
	    return (status);
	}

	rdr_info = resrange->pss_rdrinfo;
	obj_desc = rdr_info->rdr_obj_desc;

	/* VIEW may not be REFRESHed */
	if (obj_desc->dd_o6_objtype == DD_3OBJ_VIEW)
	{
	    (VOID) psf_error(2043L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
	        err_blk, 1,STlength(reg_name->pss_orig_obj_name),
		reg_name->pss_orig_obj_name);
	    return (E_DB_ERROR);
	}

	refr_ldb_tab_info = &obj_desc->dd_o9_tab_info;

	/*
	** Check if schema changed in a way that precludes refreshing:
	** 1) Some columns have been deleted or changed         OR
	** 2) new columns were added and the DDB column names
	**    were mapped
	*/
	if (   rdr_info->rdr_local_info &
	    	(RDR_L2_SUBSET_SCHEMA | RDR_L3_SCHEMA_MISMATCH)
	    ||
	       refr_ldb_tab_info->dd_t6_mapped_b &&
	       ~rdr_info->rdr_local_info & RDR_L0_NO_SCHEMA_CHANGE
	   )
	{
	    (VOID) psf_error(2044L, (DB_ERROR *)NULL, PSF_USERERR, 
                             &err_code, err_blk,0);
		return (E_DB_ERROR);
	}

	/* If stats found on LDB, update DDB stats from LDB stats */
	if (rdr_info->rdr_local_info & RDR_L4_STATS_EXIST)
	{
	    ddl_info->qed_d10_ctl_info |= QED_1DD_DROP_STATS;
	}

	/* 
	** Copy LDB_TAB_INFO and nested LDB_INFO of object being
	** refreshed because it will be used for new registration.
	*/
	save_ptr  = ldb_tab_info->dd_t9_ldb_p;
	STRUCT_ASSIGN_MACRO(*refr_ldb_tab_info, *ldb_tab_info);
	STRUCT_ASSIGN_MACRO(*refr_ldb_tab_info->dd_t9_ldb_p, *save_ptr);
	ldb_tab_info->dd_t9_ldb_p = save_ptr;

	/*
	** If column names were mapped, we need to store attribute
	** entries.
	*/

	if (ldb_tab_info->dd_t6_mapped_b)
	{
	    i4			col_cnt;
	    DD_COLUMN_DESC	**from_ldb_col_desc;
	    DD_COLUMN_DESC	**to_ldb_col_desc;
	    DMT_ATT_ENTRY	**from_ddb_att_entry;
	    DD_COLUMN_DESC	**to_ddb_col_desc;

	    /* pointers to accomodate LDB object attribute entries */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	        (ldb_tab_info->dd_t7_col_cnt +1) * sizeof(DD_COLUMN_DESC *),
	        (PTR *) &ldb_tab_info->dd_t8_cols_pp, err_blk);

	    if (status != E_DB_OK)
	        return (status);

	    /*
	    ** allocate space for LDB table column descriptors, copy them
	    ** from the RDF-provided structure;  also allocate space for and
	    ** fill in DDB column descriptors based on info found in the RDR
	    ** info block
	    */

	    from_ldb_col_desc	= refr_ldb_tab_info->dd_t8_cols_pp + 1;
	    to_ldb_col_desc	= ldb_tab_info->dd_t8_cols_pp + 1;
	    from_ddb_att_entry	= rdr_info->rdr_attr + 1;
	    to_ddb_col_desc	= ddl_info->qed_d4_ddb_cols_pp + 1;

	    for (col_cnt = ldb_tab_info->dd_t7_col_cnt;
		 col_cnt > 0;
		 col_cnt--, from_ldb_col_desc++, to_ldb_col_desc++,
		 from_ddb_att_entry++, to_ddb_col_desc++)
	    {
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		    sizeof(DD_COLUMN_DESC), (PTR *) to_ldb_col_desc, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}

		/* Copy LDB column descriptors */
		STRUCT_ASSIGN_MACRO((**from_ldb_col_desc),
				    (**to_ldb_col_desc));

		status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		    sizeof(DD_COLUMN_DESC), (PTR *) to_ddb_col_desc, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}

		/* Copy attribute info from the RDF info block */
		cui_move((*from_ddb_att_entry)->att_nmlen,
		    (*from_ddb_att_entry)->att_nmstr, ' ',
		    sizeof(DD_ATT_NAME),
		    (PTR) ((*to_ddb_col_desc)->dd_c1_col_name));

		(*to_ddb_col_desc)->dd_c2_col_ord =
		    (*from_ddb_att_entry)->att_number;
	    }
	}
	else
	{
	    ldb_tab_info->dd_t8_cols_pp = (DD_COLUMN_DESC **) NULL;
	    ddl_info->qed_d4_ddb_cols_pp = (DD_COLUMN_DESC **) NULL;
	}

	/* Copy DDB object name */
	memcpy(ddl_info->qed_d1_obj_name, tabname.db_tab_name,
		sizeof(DD_TAB_NAME));

	/* Store number of columns */
	ddl_info->qed_d3_col_count = ldb_tab_info->dd_t7_col_cnt;

	/* Store id of the DDB object being refreshed */
	STRUCT_ASSIGN_MACRO(obj_desc->dd_o3_objid, ddl_info->qed_d7_obj_id);

	/* store the type of the object being refreshed */
	ddl_info->qed_d8_obj_type = obj_desc->dd_o6_objtype;

    }
    else
    {
	i4	mask = 0;

	if (   status != E_DB_OK 
	    && err_blk->err_code != E_PS0903_TAB_NOTFOUND)
	{
	    return (status);
	}

	if (resrange)
	{
	    /*
	    ** Make sure base table isn't a system table.
	    */
	    if (   (resrange->pss_tabdesc->tbl_status_mask & DMT_CATALOG
		    || resrange->pss_tabdesc->tbl_2_status_mask
		              & DMT_TEXTENSION)
		&& ~sess_cb->pss_ses_flag & PSS_CATUPD)
	    {
		(VOID) psf_error(2009L, (DB_ERROR *)NULL, PSF_USERERR, 
                    &err_code, err_blk, 1, 
                    STtrmwhite(reg_name->pss_orig_obj_name),
		    reg_name->pss_orig_obj_name);
		return (E_DB_ERROR);
	    }
	    /*
	    ** or registration already owned by the current user
	    */
	    else if (   !MEcmp((PTR) &resrange->pss_ownname,
			(PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME))
		     || sess_cb->pss_ses_flag & PSS_CATUPD)
	    {
		(VOID) psf_error(2010L, (DB_ERROR *)NULL, PSF_USERERR, 
                    &err_code, err_blk, 1,
                    STtrmwhite(reg_name->pss_orig_obj_name),
		    reg_name->pss_orig_obj_name);
		return (E_DB_ERROR);
	    }
	}
	/*
	** if 'FROM source' was not specified, default to link_name
	*/
	if (!(sess_cb->pss_distr_sflags & PSS_LDB_TABLE))
	{
	    STmove(reg_name->pss_orig_obj_name, ' ', sizeof(DD_TAB_NAME),
				ldb_tab_info->dd_t1_tab_name);
	}

	/* store REGISTERed object name for QEF */
	STmove(reg_name->pss_orig_obj_name, ' ', sizeof(DD_OBJ_NAME),
			ddl_info->qed_d1_obj_name);

	/* if the LDB table name was not quoted, let RDF case translate
	** it according to LDB's case translation semantics.
	*/
	if (sess_cb->pss_distr_sflags & PSS_DELIM_TBLNAME)
	{
	    mask |= PSS_DELIM_TBL;
	}
	if (   ~sess_cb->pss_distr_sflags & PSS_QUOTED_TBLNAME)
	{
	    mask |= PSS_GET_TBLCASE;
	}
	if (sess_cb->pss_distr_sflags & PSS_DELIM_OWNNAME)
	{
	    mask |= PSS_DELIM_OWN;
	}
	if (   ~sess_cb->pss_distr_sflags & PSS_QUOTED_OWNNAME
	    && STskipblank(ldb_tab_info->dd_t2_tab_owner,
				sizeof(ldb_tab_info->dd_t2_tab_owner))
						!= (char *)NULL
	   )
	{
	    mask |= PSS_GET_OWNCASE;
	}
	if (ldb_obj_type == DD_5OBJ_REG_PROC)
	{
	    mask |= PSS_REGPROC;
	}
	if (*sess_cb->pss_dbxlate & CUI_ID_DLM_M)
	{
	    mask |= PSS_DDB_MIXED;
	}

	/* try to get LDB table description */
	status = pst_ldbtab_desc(sess_cb, ddl_info, &sess_cb->pss_ostream,
				mask, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	/*
	** either REGISTER <obj_name> was entered, or REGISTER <obj_type>
	** was entered, and this is not the type of the LDB object being
	** registered.  In the former case, we will flag an error if the
	** LDB object is an INDEX, since one MUST use REGISTER INDEX to
	** register an index.  In the latter case we will flag an error.
	*/

	if (ldb_obj_type != ldb_tab_info->dd_t3_tab_type)
	{
	    /*
	    ** REGISTER [TABLE|VIEW|PROCEDURE] was entered, but the LDB object 
	    ** is an INDEX
	    */
	    if (ldb_tab_info->dd_t3_tab_type == DD_4OBJ_INDEX)
	    {
		(VOID) psf_error(2037L, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, err_blk,1, psf_trmwhite(sizeof(DD_TAB_NAME),
				     ldb_tab_info->dd_t1_tab_name),
			ldb_tab_info->dd_t1_tab_name);
		return (E_DB_ERROR);
	    }

	    /* REGISTER TABLE|VIEW|PROCEDURE was entered */
	    else if (ldb_obj_type > 0)     
	    {
		char	*reg_word;
		char	*type;

		switch (ldb_obj_type)
		{
		case DD_2OBJ_TABLE:
		    reg_word = "TABLE";
		    break;
		case DD_3OBJ_VIEW:
		    reg_word = "VIEW";
		    break;
		case DD_5OBJ_REG_PROC:
		    reg_word = "PROCEDURE";
		    break;
		}
		switch (ldb_tab_info->dd_t3_tab_type)
		{
		case DD_2OBJ_TABLE:
		    type = "TABLE";
		    break;
		case DD_3OBJ_VIEW:
		    type = "VIEW";
		    break;
		case DD_5OBJ_REG_PROC:
		    type = "PROCEDURE";
		    break;
		}

		if (ldb_tab_info->dd_t3_tab_type == DD_5OBJ_REG_PROC)
		{
                    (VOID) psf_error(E_PS1202_REG_WRONGTYPE, (DB_ERROR *)NULL, 
                        PSF_USERERR, 
			&err_code, err_blk, 3, STlength(reg_word), reg_word,
                        psf_trmwhite(sizeof(DD_TAB_NAME),
                                    ldb_tab_info->dd_t1_tab_name),
                        ldb_tab_info->dd_t1_tab_name, STlength(type), type);
		}
		else
		{
		    (VOID) psf_error(2038L, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, err_blk, 3, STlength(reg_word), reg_word,
			psf_trmwhite(sizeof(DD_TAB_NAME),
				     ldb_tab_info->dd_t1_tab_name),
			ldb_tab_info->dd_t1_tab_name, STlength(type), type);
		}
		return (E_DB_ERROR);
	    }
	}

	/* 
	** If this is a register procedure, make sure that the SQL level
	** for the local is at least 6.5
	*/
	if ((ldb_obj_type == DD_5OBJ_REG_PROC) &&
	    (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c4_ingsql_lvl < 605))
	{
	    (VOID) psf_error(E_PS1201_LDB_PROC_LVL_UNSUP, (DB_ERROR *)NULL, 
                        PSF_USERERR, &err_code, err_blk, 1, 
                        psf_trmwhite(sizeof(DD_TAB_NAME),
                                    ldb_tab_info->dd_t1_tab_name),
                        ldb_tab_info->dd_t1_tab_name);
	    return (E_DB_ERROR);
	}

	/*
	** if AS NATIVE was specified, DDB object type must be TABLE.
	** qed_d8_obj_type was initialized as DD_1OBJ_LINK.
	*/
	if (sess_cb->pss_stmt_flags & PSS_REG_AS_NATIVE)
	{
	    ddl_info->qed_d8_obj_type = DD_2OBJ_TABLE;
	}
	
	if (!ldb_tab_info->dd_t6_mapped_b)
	{
	    ldb_tab_info->dd_t8_cols_pp = (DD_COLUMN_DESC **) NULL;
	    ddl_info->qed_d4_ddb_cols_pp = (DD_COLUMN_DESC **) NULL;
	}
    }
    /*
    ** number of LINK columns must be the same as that of LDB table.
    */
    ddl_info->qed_d3_col_count = ldb_tab_info->dd_t7_col_cnt;

    /* store the text of REGISTER query */
    status = psl_rg4_regtext(sess_cb, ldb_obj_type, xlated_qry, psq_cb);

    if (DB_FAILURE_MACRO(status))
	return (status);

    /* store the text of REGISTER query */
    ddl_info->qed_d9_reg_info_p->qed_q4_pkt_p =
					       xlated_qry->pss_q_list.pss_head;

    if (sess_cb->pss_distr_sflags & PSS_REFRESH)
    /* Finally, build PSY_CB to be used when calling QEF */

    {
	PSY_CB		*psy_cb;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSY_CB),
				&sess_cb->pss_object, err_blk);
	if (status != E_DB_OK)
	    return (status);
	    
	status = psf_mroot(sess_cb, &sess_cb->pss_ostream, sess_cb->pss_object, err_blk);
	if (status != E_DB_OK)
	    return (status);

    	psy_cb = (PSY_CB *) sess_cb->pss_object;
	
	/* Fill in control block header */
	psy_cb->psy_length = sizeof(PSY_CB);
	psy_cb->psy_type = PSYCB_CB;
	psy_cb->psy_owner = (PTR)DB_PSF_ID;
	psy_cb->psy_ascii_id = PSYCB_ASCII_ID;

	/* Initialize QSO_OBIDs to indicate no objects. */
	psy_cb->psy_qrytext.qso_handle = (PTR) NULL;
	psy_cb->psy_outtree.qso_handle = (PTR) NULL;
	psy_cb->psy_textout.qso_handle = (PTR) NULL;
	psy_cb->psy_intree.qso_handle  = (PTR) NULL; 

	/*
	** I am conscious of the fact that I am overloading psy_dmucb,
	** but it seems stupid to add another PTR to PSY_CB.
	*/
	psy_cb->psy_dmucb = (PTR) ddl_info;
    }
    return (E_DB_OK);
}

/*
** Name: psl_rg2_reg_distr_idx	- semantic actions for reg_distr_idx production
**				  for registering an index
**
** Description:
**	This routine contains the semantic actions for the reg_distr_idx
**	production.  Note that REGISTER INDEX has not been completely
**	implemented yet.  A user error is issued in psl_rg3_reg_tvi function.
**	See comments below.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    memory stream
**	    pss_object	    points to a QED_DDL_INFO block.
**	reg_name	    name of table to be registered
**	xlated_qry	    for query text buffering
**	  
** Outputs:
**	err_blk	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	04-jun-92 (barbara)
**	    Copied from the Star 6.4 SQL grammar
*/
DB_STATUS
psl_rg2_reg_distr_idx(
	PSS_SESBLK	*sess_cb,
	char		*reg_name,
	PSS_Q_XLATE	*xlated_qry,
	DB_ERROR	*err_blk)
{
    QED_DDL_INFO	*ddl_info     = (QED_DDL_INFO *) sess_cb->pss_object;
    DD_2LDB_TAB_INFO    *ldb_tab_info = ddl_info->qed_d6_tab_info_p;
    i4		err_code;
    DB_STATUS	    	status;
    i4		    	mask = 0;

    /*
    ** Check if the DDB object on which the INDEX is being REGISTERed
    ** exists.  While at it, obtain the site of the LDB index
    ** FIX ME!!!
    ** NOTE: this code needs to be completed (and will be, as soon as we
    ** have enough time and QEF gets around to deciding what it wants from
    ** PSF.  Folowing has to be done:
    ** 1) Check if the DDB object (ON NAME) exists, as does the underlying
    **    LDB object.  ALSO, obtain the site; this may require modifying
    **    semantics of RDR_CHECK_EXIST call to RDF, or, more simply, using
    **    pst_sent() to get all the information (we don't care about
    **    attributes.)  While doing it, we must save store away name of the
    **    DDB object beiong created and the name of its owner, as they would
    **    get overwritten as part of obtaioning the above info.
    **
    ** 2) Once this is done, we must place the name and owner of the DDB
    **    object being created back into DDL_INFO.  We have to obtain info
    **    about the proposed underlying object, in particular we must make
    **    sure that it exsits and is, indeed, an index on the LDB object
    **    corresponding to <NAME>.
    **    We will reuse some info that we got about the (ON NAME) object,
    **    e.g. site, whether it is mapped or not, etc.  Then
    **    (first saving away the mapped_b) we have to pretend that there
    **    is no mapping.  First we need to know how many columns are there
    **    in the local index.
    **    Once this is known we will (depending on what QEF wants) either
    **    allocate space for 2 separate sets of columns or make both
    **    pointers point to one set of columns (if the columns in the DDB
    **    table [NAME] are mapped, column names of the DDB object being
    **    created MUST be mapped to those of the underlying LDB object and
    **    I would expect to treat this as mapped case, but it's up to QEF
    **    to choose the way.
    */
    /*
    ** Check if the DDB object exists, and try to obtain its
    ** underlying object and site.  
    ******************
    */

    /* if 'FROM source' was not specified, default to link_name */
    if (~sess_cb->pss_distr_sflags & PSS_LDB_TABLE)
    {
	STmove(reg_name, ' ', sizeof(DD_TAB_NAME), ldb_tab_info->dd_t1_tab_name);
    }

    /* store REGISTERed object name for QEF */
    STmove(reg_name, ' ', sizeof(DD_OBJ_NAME), ddl_info->qed_d1_obj_name);
	
    /* column names will be the same */
    ldb_tab_info->dd_t8_cols_pp = ddl_info->qed_d4_ddb_cols_pp;

    /* if the LDB owner or table name was not specified or was not quoted,
    ** let RDF uppercase it if IIDBCAPABILITIES indicates that names
    ** must be uppercased
    */
    if (~sess_cb->pss_distr_sflags & PSS_QUOTED_TBLNAME)
    {
	mask |= PSS_GET_TBLCASE;
    }
    if (~sess_cb->pss_distr_sflags & PSS_QUOTED_OWNNAME)
    {
	mask |= PSS_GET_OWNCASE;
    }
    if (*sess_cb->pss_dbxlate & CUI_ID_DLM_M)
    {
	mask |= PSS_DDB_MIXED;
    }

    /* try to get LDB index description */
    status = pst_ldbtab_desc(sess_cb, ddl_info, &sess_cb->pss_ostream, mask, err_blk);

    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /* If the local object is not of type INDEX ==> error */
    if (ldb_tab_info->dd_t3_tab_type != DD_4OBJ_INDEX)
    {
	char	    *type;

	type = (ldb_tab_info->dd_t3_tab_type == DD_2OBJ_TABLE) ? "TABLE"
							       : "VIEW";
	(VOID) psf_error(2038L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		err_blk, 3, sizeof("INDEX") - 1, "INDEX",
		psf_trmwhite(sizeof(DD_TAB_NAME), ldb_tab_info->dd_t1_tab_name),
		ldb_tab_info->dd_t1_tab_name, STlength(type), type);
	return (E_DB_ERROR);
    }
	
    /* number of LINK columns must be the same as that of LDB table. */

    ddl_info->qed_d3_col_count = ldb_tab_info->dd_t7_col_cnt;

    /* store the text of REGISTER INDEX */

    /* FIX_ME: Will need to call psl_rg4_regtext() */

    /* store the text of REGISTER query */
    ddl_info->qed_d9_reg_info_p->qed_q4_pkt_p =
					       xlated_qry->pss_q_list.pss_head;
    return (E_DB_OK);
}


/*
** Name: psl_rg3_reg_tvi	- semantic actions for reg_keywd, reg_idx
**				  and crt_lnk_kwd productions
**
** Description:
**	The reg_keywd, reg_idx and crt_lnk_kwd productions are the first
**	productions of the Star REGISTER TABLE|VIEW, REGISTER PROCEDURE,
**	REGISTER INDEX and CREATE LINK statements.  
**
**	The purpose of this function is to set up data structures for parsing 
**	the rest of the statement.  Note that REGISTER INDEX is not yet 
**	supported in Star and incurs an error.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	  pss_ostream	    memory stream
**	  pss_user	    user name
**	psq_cb		    parser control block
**	  psq_dfltdesc	    points to CDB descriptor
**	  psq_mode	    mode of statement
**			     PSQ_0_CRT_LINK,PSQ_REG_LINK, PSQ_REG_INDEX
**	  
** Outputs:
**	sess_cb
**	  pss_object	    points to an allocated QED_DDL_INFO block.
**	psq_cb		    
**	  psq_error	    will be filled in if an error occurs
**	  psq_ldbdesc	    will point at LDB desc inside QED_DDL_INFO
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	04-jun-92 (barbara)
**	    Copied from the Star 6.4 SQL grammar
**	11-now-92 (teresa)
**	    minor modification to support register procedure
**	07-jan-02 (toumi01)
**	    Replace DD_300_MAXCOLUMN with DD_MAXCOLUMN (1024).
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
*/
DB_STATUS
psl_rg3_reg_tvi(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    i4			err_code;
    DB_STATUS			status;
    QED_DDL_INFO	       *ddl_info;	
    DD_2LDB_TAB_INFO       	*ldb_tab_info;
    QED_QUERY_INFO		*qry_info;
	    
    /* REGISTER [TABLE|VIEW|INDEX] */

    if (psq_cb->psq_mode == PSQ_REG_INDEX)
    {
	/* This is so until QEF gets around to implementing it */
	(VOID) psf_error(2040L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		   &psq_cb->psq_error,0);
	return (E_DB_ERROR);
    }

    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);
    sess_cb->pss_stk_freelist = NULL;

    /* Allocate the QEU *-level info block for REGISTER */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QED_DDL_INFO),
			    &sess_cb->pss_object, &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, sess_cb->pss_object,
			&psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    ddl_info = (QED_DDL_INFO *) sess_cb->pss_object;

    memcpy(ddl_info->qed_d2_obj_owner, sess_cb->pss_user.db_own_name,
		sizeof(DD_OWN_NAME));

    /*
    ** Allocate the column descriptors of the LINK object.  Assume
    ** DD_MAXCOLUMN, although it's probably fewer.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    (DD_MAXCOLUMN+1) * sizeof(DD_COLUMN_DESC *),
	    (PTR *) &ddl_info->qed_d4_ddb_cols_pp, &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    /*
    ** No text will be provided in qed_d5_qry_info_p.  This is
    ** where CREATE TABLE text goes.
    */
    ddl_info->qed_d5_qry_info_p = (QED_QUERY_INFO *) NULL;

    /* allocate space for query info block */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QED_QUERY_INFO),
			    (PTR *) &ddl_info->qed_d9_reg_info_p,
			    &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    qry_info = ddl_info->qed_d9_reg_info_p;

    qry_info->qed_q1_timeout = 0;
    qry_info->qed_q2_lang    = DB_SQL;
    qry_info->qed_q3_qmode   = DD_2MODE_UPDATE;

    /* allocate space for LDB table info block */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_2LDB_TAB_INFO),
			    (PTR *) &ddl_info->qed_d6_tab_info_p,
			    &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    ldb_tab_info = (DD_2LDB_TAB_INFO *) ddl_info->qed_d6_tab_info_p;

    /* Assume that the user name will not be specified */
    MEfill(sizeof(DD_OWN_NAME), ' ', (PTR) ldb_tab_info->dd_t2_tab_owner);
	
    ldb_tab_info->dd_t6_mapped_b = FALSE;  /* assume no mapping is needed */
    ldb_tab_info->dd_t7_col_cnt = 0;

    ddl_info->qed_d8_obj_type = DD_1OBJ_LINK;   /* REGISTERing AS LINK */
	
    /* allocate space for LDB descriptor */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_1LDB_INFO),
			    (PTR *) &ldb_tab_info->dd_t9_ldb_p,
			    &psq_cb->psq_error);

    if (status != E_DB_OK)
	return (status);

    /*
    ** save address of LDB descriptor so it will be filled in when	    
    ** processing WITH clause
    */

    psq_cb->psq_ldbdesc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;
	
    /* copy default LDB description so it may be modified */
    STRUCT_ASSIGN_MACRO(*psq_cb->psq_dfltldb, *psq_cb->psq_ldbdesc);

    /* this flag is set to FALSE for user queries */
    psq_cb->psq_ldbdesc->dd_l1_ingres_b = FALSE;

    /* we may not assume that the LDB id is the same as that of CDB */
    psq_cb->psq_ldbdesc->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;

    return (E_DB_OK);
}

/*
** Name: psl_rg4_regtext	- generate and save text for iiregistrations
**
** Description:
**	The distributed statements REGISTER [WITH REFRESH], CREATE LINK
**	and CREATE TABLE must all save text of the register statement
**	in iiregistrations.  This routine buffers the text into DD_PACKETS.
**	The caller will arrange to pass the text to QEF.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    memory stream
**	    pss_stmt_flags  statement specific flags
**			      PSS_REG_AS_NATIVE indicates that registration
**			      is NATIVE (as opposed to LINK)
**	    pss_distr_sflags  contains info on ldb table_name.
**	    pss_object	    for REG/CRT_LNK stmts points to a QED_DDL_INFO
**			      containing info about distributed and local
**			      object; for CRT_TBL points to a QEU_CB.
**	ldb_obj_type	    type of underlying LDB object
**	xlated_qry	    for query text buffering
**	psq_cb		    ptr to parser control block.
**	    psq_dfltldb	      ptr to LDB descriptor (CDB)
**	    psq_ldbdesc	      ptr to LDB descriptor from WITH-clause
**	  
** Outputs:
**	sess_cb
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	04-jun-92 (barbara)
**	    Written to be shareable by REGISTER, CREATE LINK and CREATE TABLE.
**	07-oct-92 (barbara)
**	    Pass in length to psq_x_add if string to be buffered contains
**	    spaces.
**	14-oct-92 (barbara)
**	    More problems with psq_x_add interface!  Delimiters cannot
**	    contain spaces, i.e., no multi-word delimiters.
**	17-nov-92 (teresa)
**	    Add support for register procedure.
**	07-dec-92 (andre)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	10-feb-93 (rblumer)
**	    check for new DISTCREATE query mode when setting up ddl_info
**	02-sep-93 (barbara)
**	    1. When storing the local owner and object names for iiregistrations
**	       text, there's no need to case translate.  The names were case
**	       translated in place by RDF when it did the LDB object lookup.
**	    2. Star delimited ids: To decide whether or not to delimit, Star now
**	       relies on the presence of the LDB's DB_DELIMITED_CASE capability
**	       instead of its OPEN/SQL_LEVEL.
**	    
*/
DB_STATUS
psl_rg4_regtext(
	PSS_SESBLK	*sess_cb,
	i4		ldb_obj_type,
	PSS_Q_XLATE	*xlated_qry,
	PSQ_CB		*psq_cb)
{
    QED_DDL_INFO	*ddl_info;
    QEU_CB		*qeu_cb;
    DD_2LDB_TAB_INFO	*ldb_tab_info;
    DD_COLUMN_DESC 	**col_names;
    DD_PACKET		*pkt;
    char		*str_obj_type;
    DB_STATUS		status;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    u_char		unorm_buf[DB_MAXNAME *2 +2];
    char		*bufp;
    u_i4		unorm_len=DB_MAXNAME*2+2, name_len;
    bool		support_delims;

    if (   psq_cb->psq_mode == PSQ_CREATE 
	|| psq_cb->psq_mode == PSQ_RETINTO 
	|| psq_cb->psq_mode == PSQ_DISTCREATE)
    {
	qeu_cb = (QEU_CB *)sess_cb->pss_object;
	ddl_info = (QED_DDL_INFO *)qeu_cb->qeu_ddl_info;
    }
    else
    {
	ddl_info = (QED_DDL_INFO *) sess_cb->pss_object;
    }
    ldb_tab_info = ddl_info->qed_d6_tab_info_p;

    if (ldb_obj_type == DD_2OBJ_TABLE)
	str_obj_type = " table";
    else if (ldb_obj_type == DD_3OBJ_VIEW)
	str_obj_type = " view";
    else if (ldb_obj_type == DD_5OBJ_REG_PROC)
	str_obj_type = " procedure";
    else 
	str_obj_type = (char *)NULL;

    xlated_qry->pss_q_list.pss_tail = 
	xlated_qry->pss_q_list.pss_head = (DD_PACKET *) NULL;

    /* For REGISTER, buffer size is equal to catalog row width */
    xlated_qry->pss_buf_size = PSS_CRLINK_BSIZE;

    /* We also need to reserve the first 2 bytes in each buffer */
    xlated_qry->pss_q_list.pss_skip = 2;

    status = psq_x_add(sess_cb, "register", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *) NULL, (char *)NULL, str_obj_type, err_blk);

    if (DB_FAILURE_MACRO(status))
	return (status);

    support_delims = (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
			dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS) ? TRUE:FALSE;
    /*
    ** Buffer registered object's name.  First unnormalize and delimit if
    ** LDB supports that.
    */
    name_len = (u_i4) psf_trmwhite(sizeof(DD_OBJ_NAME),
                                                ddl_info->qed_d1_obj_name);

    if (support_delims)
    {
	unorm_len=DB_OBJ_MAXNAME*2+2;
	status = cui_idunorm((u_char *)ddl_info->qed_d1_obj_name, &name_len,
                                unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
	bufp = (char *) unorm_buf;
	name_len = unorm_len;
    }
    else
    {
	bufp = ddl_info->qed_d1_obj_name;
    }

    status = psq_x_add(sess_cb, bufp, &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			(i4) name_len, " ", " ", (char *)NULL, err_blk);

    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Buffer column list if column names are mapped */

    if (ldb_tab_info->dd_t6_mapped_b)
    {
	i4	i;

	for (i = 1, col_names = &ddl_info->qed_d4_ddb_cols_pp[1];
	     i <= ldb_tab_info->dd_t7_col_cnt; i++, col_names++)
	{
	    char	*str;

	    if (i == 1)
		str = "(";
	    else
		str = " ";

	    name_len = (u_i4) psf_trmwhite(sizeof(DD_ATT_NAME),
				(*col_names)->dd_c1_col_name);

	    if (support_delims)
	    {
		unorm_len=DB_ATT_MAXNAME*2+2;
		status = cui_idunorm((u_char *)(*col_names)->dd_c1_col_name,
			&name_len, unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);

		if (DB_FAILURE_MACRO(status))
		    return (status);
	
		bufp = (char *) unorm_buf;
		name_len = unorm_len;
	    }
	    else
	    {
		bufp = (*col_names)->dd_c1_col_name;
	    }

	    psq_x_add(sess_cb, bufp, &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list, name_len,
		str, ",", (char *)NULL, err_blk);

	    if (DB_FAILURE_MACRO(status))
		return (status);
	}

	/* back over the last comma */
	xlated_qry->pss_q_list.pss_next_ch         -=PSS_0_COMMA_LEN;
	xlated_qry->pss_q_list.pss_tail->dd_p1_len -=PSS_0_COMMA_LEN;

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *) NULL, (char *)NULL, ") ", err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    {
	char	*str;

	if (sess_cb->pss_stmt_flags & PSS_REG_AS_NATIVE)
	{
	    str="as native from ";
	}
	else
	{
	    str="as link from ";
	}
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		    -1, (char *)NULL, (char *)NULL, str, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    /* 
    ** Append owner.table.  Owner and table name must be delimited.
    ** Case translation is not required because RDF has already translated
    ** -- so the table and owner name in ldb_tab_info appear exactly as they
    ** were looked up by RDF.
    **
    ** Unnormalize and delimit owner name if LDB supports that.
    */

    name_len = (u_i4) psf_trmwhite(sizeof(DD_OWN_NAME),
				ldb_tab_info->dd_t2_tab_owner);

    if (support_delims)
    {
	unorm_len=DB_OWN_MAXNAME*2+2;
	status = cui_idunorm((u_char *)ldb_tab_info->dd_t2_tab_owner, &name_len,
                                unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
	unorm_buf[unorm_len] = '\0';	/* pain-in-the-neck psq_x_add */
        status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list, -1,
			(char *)NULL, (char *)NULL, (char *)unorm_buf, err_blk);
    }
    else
    {
    	status = psq_x_add(sess_cb, ldb_tab_info->dd_t2_tab_owner,
			&sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list, name_len,
			"'", "'", (char *)NULL, err_blk);
    }

    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Unnormalize and delimit tablename */

    name_len = (u_i4) psf_trmwhite(sizeof(DD_TAB_NAME),
				ldb_tab_info->dd_t1_tab_name);

    if (support_delims)
    {
	unorm_len=DB_TAB_MAXNAME*2+2;
	status = cui_idunorm((u_char *)ldb_tab_info->dd_t1_tab_name, &name_len,
                                unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
	bufp = (char *)unorm_buf;
 	name_len = unorm_len;
    }
    else
    {
	bufp = ldb_tab_info->dd_t1_tab_name;
    }

    status = psq_x_add(sess_cb, bufp, &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			name_len,
			".", " ", (char *) NULL, err_blk);

    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Determine if the WITH-clause needs to be specified. */
    if (!psq_same_site(psq_cb->psq_dfltldb, psq_cb->psq_ldbdesc))
    {
	char	*str = "with node = ";

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *)NULL, (char *)NULL, str, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psq_x_add(sess_cb, psq_cb->psq_ldbdesc->dd_l2_node_name,
			&sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list,
			(i4) psf_trmwhite(sizeof(DD_NODE_NAME),
				psq_cb->psq_ldbdesc->dd_l2_node_name),
			"'", "'", ", ", err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	str = "database = ";

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *)NULL, (char *)NULL, str, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psq_x_add(sess_cb, psq_cb->psq_ldbdesc->dd_l3_ldb_name,
			&sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list,
			(i4) psf_trmwhite(sizeof(DD_DB_NAME),
				psq_cb->psq_ldbdesc->dd_l3_ldb_name),
			"'", "'", ", ", err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	str = "dbms = ";

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *)NULL, (char *) NULL, str, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psq_x_add(sess_cb, psq_cb->psq_ldbdesc->dd_l4_dbms_name,
		    &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
		    &xlated_qry->pss_q_list,
		    (i4) psf_trmwhite(
			sizeof(psq_cb->psq_ldbdesc->dd_l4_dbms_name),
			psq_cb->psq_ldbdesc->dd_l4_dbms_name),
			"'", "'", " ", err_blk);

	if (DB_FAILURE_MACRO(status))
	    return (status);

    }

    /*
    ** we need to store buffer length in the first 2 bytes of each
    ** packet.
    */
    for (pkt = xlated_qry->pss_q_list.pss_head;
	 pkt != (DD_PACKET *) NULL; pkt = pkt->dd_p3_nxt_p)
    {
	*((u_i2 *) pkt->dd_p2_pkt_p) = (u_i2) (pkt->dd_p1_len - 2);
    }
    return (E_DB_OK);
}

/*
** Name: psl_rg5_ldb_spec	- semantic actions for ldb_spec: production
**
** Description:
**	The ldb_spec production is used on the REGISTER, CREATE LINK,
**	DIRECT CONNECT and DIRECT EXECUTE IMMEDIATE statements.  It is
**	the topmost rule for parsing the with_clause on these statements.
**
**	ldb_spec:				-- no WITH-clause
**		|	WITH NULLWORD		-- not sure why this is allowed
**		|	WITH NAME		-- WITH REFRESH
**		|	WITH ldb_spec_list	-- WITH NODE, etc.
**
**	psl_rg5_ldb_spec is called for the second two rules (WITH NAME and
**	WITH ldb_spec_list)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_distr_sflags  for with-clause validation
**	    pss_ostream	    memory stream
**	name		    ptr to WITH option if WITH NAME rule; NULL otherwise
**	qmode	    	    mode of statement	
**	  
** Outputs:
**	sess_cb
**	    pss_distr_sflags    will be set if REGISTER WITH REFRESH 
**	err_blk 	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	04-jun-92 (barbara)
**	    Copied from 6.4 Star grammar for Sybil.
**	08-oct-92 (barbara)
**	    Added break in case statement.
**	19-nov-92 (tam)
**	    Added support for ldb procedure registration.
*/
DB_STATUS
psl_rg5_ldb_spec(
	PSS_SESBLK	*sess_cb,
	char		*name,
	i4		qmode,
	DB_ERROR	*err_blk)
{
    i4	err_code = 0;
    char 	*c;

    if (name != (char *) NULL)
    {
	/* WITH REFRESH is allowed only for REGISTER */

	/* depending on the query mode, we will produce different message */
	switch (qmode)
	{
	    case PSQ_DIRCON:
	    case PSQ_DIREXEC:
	    {
		err_code = 2079L;
		break;
	    }
	    case PSQ_REG_LINK:
	    {
		if (STcompare("refresh", name))
		{
		    err_code = 2080L;
		}
		else
		{
		    err_code = 0L;
		}
		break;
	    }
	    case PSQ_0_CRT_LINK:
	    {
		err_code = 2087L;
		break;
	    }
	}

	if (err_code != 0L)
	{
	    (VOID) psf_error(err_code, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		err_blk, 1,STlength(name), name);
	    return (E_DB_ERROR);
	}

	sess_cb->pss_distr_sflags |= PSS_REFRESH;
	return (E_DB_OK);
    }

    /* Processing WITH ldb_spec_list rule */

    switch (qmode)
    {
	case PSQ_DIRCON:
	{
	    c = "DIRECT CONNECT";
	    break;
	}

	case PSQ_DIREXEC:
	{
	    c = "DIRECT EXECUTE IMMEDIATE";
	    break;
	}

	case PSQ_0_CRT_LINK:
	{
	    c = "CREATE LINK";
	    break;
	}

	case PSQ_REG_LINK:
	{
	    if (sess_cb->pss_distr_sflags & PSS_REGISTER_PROC)
		c = "REGISTER PROCEDURE";
	    else
	        c = "REGISTER [TABLE|VIEW|INDEX]";
	    break;
	}
    }

    /*
    ** make sure that either none or both of NODE and DATABASE
    ** clauses were found
    */

    if (   (sess_cb->pss_distr_sflags & PSS_NDE_AND_DB)
	&& (~sess_cb->pss_distr_sflags & PSS_NDE_AND_DB))
    {
	(VOID) psf_error(2031L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
			err_blk, 1,STlength(c), c);
	return (E_DB_ERROR);
    }
    return (E_DB_OK);
}

/*
** psl_rg6_link_col_list - semantic actions for the link_col_list prod
**
** Description:
**	The link_col_list production is used on the REGISTER and CREATE LINK,
**	statements.  It parses the optional column list on these statements.
**	This function is called for each column name parsed.  The semantic
**	actions are to save the mapped column name and check that no
**	duplicate column names are encountered.
**
**	link_col_list:	NAME
**		|	link_col_list COMMA NAME
**
** Inputs:
**	colname		    char string containing mapped name
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    memory stream
**	    pss_object	    points to distributed QEF info
**	psq_cb
**	    psq_mode	    mode of statement (PST_0_CRT_LINK,PSQ_REG_LINK)
**	  
** Outputs:
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	03-aug-92 (barbara)
**	    Copied from 6.4 Star grammar for Sybil.
**	11-oct-92 (barbara)
**	    Return immediately if not distributed thread.
*/
DB_STATUS
psl_rg6_link_col_list(
	char		*colname,
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    i4	    err_code;
    QED_DDL_INFO    *ddl_info;
    DD_COLUMN_DESC  **col;
    DB_STATUS	    status;
    i4		    *count;
    i4		    i;
    char	    *newcol;
    char	    *cmnd = (psq_cb->psq_mode == PSQ_REG_LINK)
				    ? "REGISTER [TABLE|VIEW]"
				    : "CREATE LINK";

    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	(VOID) psf_error(E_PS110C_TBL_COLLIST, (DB_ERROR *)NULL, PSF_USERERR, 
                         &err_code, &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    ddl_info = (QED_DDL_INFO *)sess_cb->pss_object;
    count = &ddl_info->qed_d6_tab_info_p->dd_t7_col_cnt;

    /* mapping will be required */
    ddl_info->qed_d6_tab_info_p->dd_t6_mapped_b = TRUE;

    /* allocate new column descriptor */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_COLUMN_DESC),
	(PTR *) &ddl_info->qed_d4_ddb_cols_pp[++*count],
	&psq_cb->psq_error);

    if (status != E_DB_OK)
   	return (status); 

    newcol = ddl_info->qed_d4_ddb_cols_pp[*count]->dd_c1_col_name;
    STmove(colname, ' ', sizeof(DD_ATT_NAME), newcol);

    /* check for duplicate column names */
    for (i = *count -1, col = ddl_info->qed_d4_ddb_cols_pp +i;
	 i > 0; col--, i--)
    {
	if (!MEcmp((PTR) (*col)->dd_c1_col_name,
			(PTR) newcol, sizeof(DD_ATT_NAME)))
	{
	    (VOID) psf_error(2032L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 2,STlength(cmnd), cmnd,
			    STlength(colname), colname);
	    return (E_DB_ERROR);
	}
    }

    return (E_DB_OK);
}

/*
** Name: psl_ds1_dircon	- semantic action for DIRECT CONNECT
**
** Description:
**	The semantic actions for DIRECT CONNECT consist of syntax
**	checking and setting the statement mode for return to SCF.
**
** Inputs:
**	name1		    first word of statement
**	name2		    second word of statement
**	is_distrib	    TRUE if distributed
**	  
** Outputs:
**	psq_cb		    ptr to parser control block.
**	    psq_mode	    Set to DIRECT CONNECT if no error
**	    psq_error	    will be filled in if an error occurs
**
** History:
**	03-aug-1992 (barbara)
**	    Extracted from 6.4 Star grammar for Sybil.
*/
DB_STATUS
psl_ds1_dircon(
	PSQ_CB		*psq_cb,
	char		*name1,
	char		 *name2,
	bool		is_distrib)
{
    i4	err_code;
    char	*c;

    if (STcompare(name1, "direct") == 0)
    {
	if (STcompare(name2, "connect") == 0)
	{
	    psq_cb->psq_mode = PSQ_DIRCON;
	    c = "DIRECT CONNECT";
	}
	else if (STcompare(name2, "disconnect") == 0)
	{
	    psq_cb->psq_mode = PSQ_DIRDISCON;
	    c = "DIRECT DISCONNECT";
	}
	else
	{
	    /* word following "direct" is not "[dis]connect" */
	    (VOID) psf_error(2077L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,STlength(name2), name2);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/* first word is not "direct" <==> ERROR */
	(VOID) psf_error(2076L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1,STlength(name1), name1);
	return (E_DB_ERROR);
    }

    /*
    ** To get this far, we must have seen "DIRECT CONNECT" or
    ** "DIRECT DISCONNECT".  Since they are only allowed in distributed
    ** INGRES, make sure that it is.
    */
    if (is_distrib == FALSE)
    {
	(VOID) psf_error(2086L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,STlength(c), c);
	return (E_DB_ERROR); 
    }

    if (psq_cb->psq_mode == PSQ_DIRDISCON)
    {
	/*
	** we will get here only if NOT in "direct connect" mode.
	** Since there was no "direct connect" to match this
	** "direct disconnect", indicate error.  However, we shall
	** delay reporting an error until we see the rest of the text.
	*/
	(VOID) psf_error(2078L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
			&psq_cb->psq_error,0);
    }

    return (E_DB_OK);
}
/*
** Name: psl_ds2_dir_exec_immed - Semantic actions for DIRECT EXECUTE IMMEDIATE
**
** Description:
**	The user's direct execute immediate text is put in the form of
**	an execute immediate statement and saved in QSF memory for SCF.
**
** Inputs:
**	name		    first word of statement 
**	is_distrib	    TRUE if distributed session
**	exec_arg	    execute immediate argument
**	sess_cb		    ptr to a PSF session CB
**
** Outputs:
**	psq_cb		    ptr to parser control block.
**	    psq_mode	    Set to PSQ_DIREXEC if no error
**	    psq_error	    will be filled in if an error occurs
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    ptr to opened memory stream 
**	    pss_object	    ptr to allocated qry text
**
** History:
**	03-aug-1992 (barbara)
**	    Condensed from 6.4 Star module psqsttxt.c
**	24-sep-1992 (barbara)
**	    Reversed arguments to STcat.
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
*/
DB_STATUS
psl_ds2_dir_exec_immed(
	char		*name,
	bool		is_distrib,
	char		*exec_arg,
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    PSQ_QDESC	*qdesc;
    DB_STATUS	status;
    i4		arg_len;
    i4	err_code;

    if (STcompare(name, "direct") != 0)
    {
	/* first word is not "direct" <==> ERROR */

	(VOID) psf_error(2076L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1, STlength(name), name);
	return (E_DB_ERROR);
    }

    /* "direct execute immediate" is allowed only in distributed */
    if (is_distrib == FALSE)
    {
	(VOID) psf_error(2086L, (DB_ERROR *)NULL, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 1,
		    STlength("DIRECT EXECUTE IMMEDIATE"),
		    "DIRECT EXECUTE IMMEDIATE");
	return (E_DB_ERROR);
    }

    status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, &sess_cb->pss_ostream,
			&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);
    sess_cb->pss_stk_freelist = NULL;

    arg_len = sizeof("execute immediate ") + STlength(exec_arg); 
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, arg_len + sizeof(PSQ_QDESC) +2,
			&sess_cb->pss_object, &psq_cb->psq_error);

    qdesc = (PSQ_QDESC *)sess_cb->pss_object;

    /* Initialize query descriptor */
    qdesc->psq_qrysize = arg_len;
    qdesc->psq_datasize = 0;
    qdesc->psq_dnum = 0;
    qdesc->psq_qrytext = (char *)(qdesc + 1);

    qdesc->psq_qrydata = (DB_DATA_VALUE **) NULL;

    /* Copy direct execute immediate text from input */

    (VOID) STcopy("execute immediate ", qdesc->psq_qrytext);
    (VOID) STcat(qdesc->psq_qrytext, exec_arg);
    (VOID) STcat(qdesc->psq_qrytext, " ");

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, sess_cb->pss_object,
			&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    psq_cb->psq_mode = PSQ_DIREXEC;

    return (E_DB_OK);
}

/*
** Name: psl_proc_func	- semantic actions for scalar_function rule
**
** Description:
**	This function evaluates a pseudo-constant function such as DBMSINFO()
**	or CURRENT_DATE or CURRENT_TIMESTAMP.  This is used in two very
**	different contexts:
**	- if it's a Star query, since different LDBMS's may well have
**	  different values for these things, and we want to use a well
**	  defined value that is globally the same for the query;
**	- if it's COPY, the column default may have to be evaluated if
**	  the default is a constant function.
**
**	Note that for the copy case, it would probably be better to allow
**	a general expression for the column default (as long as it evaluates
**	to a constant, of course).  If we could simply run the default
**	value string through the parser to do ordinary constant folding
**	we could skip this stuff.  (We'd need a flag to tell the parser
**	to evaluate "notvirgin" functions that are normally put off till
**	runtime, of course.)  But for now, we'll limp along with this
**	code.
**
**	Many of the DBMSINFO functions that ask for performance data
**	are evaluated to zero, since there is no good way to do anything
**	more "useful" (such as adding up the values for all LDBMS's).
**
**	If a function is actually evaluated, a PST_CONST node is created
**	to point at the result value.
**
**	For functions other than dbmsinfo() and the special constant
**	functions, the behavior of the distributed and non-distributed
**	threads is the same.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_adfcb	    ADF control block
**	    pss_dba	    dba info
**	stream		    memory stream for allocation
**	op_code	    	    ADF function code
**	arg	    	    QST_QNODE representing arg, if any
**	newnode	    	    address of the ptr to the node to represent
**			    the result of evaluating the constant func.
**	  
**
** Outputs:
**	err_blk		    will be filled in if an error occurs
**	newnode		    address of ptr to new node for evaluated func.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	08-nov-91 (barbara)
**	    Copied from the 6.5 SQL grammar, adding distributed-only
**	    actions from the 6.4 Star SQL grammar.  If I had the time,
**	    I would use a look-up table for the dbmsinfo() arguments
**	    and eliminate the many small pieces of code doing a 
**	    CMcmpnocase().
**	13-apr-93 (robf)
**	    Added handling for session_seclabel() which must be handled by
**	    STAR, not the LDB
**	14-may-93 (barbara)
**	    Fixed dbmsinfo('username').  Misplaced paren was causing only
**	    first letter of username to be returned.
**	10-aug-93 (andre)
**	    fixed cause of compiler warnings
**	15-feb-94 (robf)
**          Added dbmsinfo('maxidle') and dbmsinfo('maxconnect') since these
**          are support in  STAR.
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members
**	December 2009 (stephenb)
**	    Add stream parameter for batch processing.
**	19-Aug-2010 (kschendel) b124282
**	    Let's try again.  Initialize adf_fi_desc, adf_pat_flags, and
**	    especially adf_dv_n members.
**	4-Feb-2011 (kschendel) b124962
**	    Add CURRENT_DATE and friends.
**	    Rework to eliminate bizarre and lengthy dbmsinfo arg evaluation.
*/

/* DBMSINFO variants that actually call DBMSINFO to generate the constant.
** Some of these (e.g. maxidle) don't make much sense to me, but I'm setting
** the table to match the original code.
*/
static const char *dbmsi_call_adf[] =
{
	"dba",
	"_bintim",
	"_et_sec",
	"maxidle",
	"username",
	"_version",
	"database",
	"terminal",
	"language",
	"maxconnect",
	"system_user",
	"server_class",
	"initial_user",
	"session_user",
	"query_language",
	"autocommit_state",
	"transaction_state",
	NULL
};

/* DBMSINFO variants that return zero (i.e. '0').  These are mostly statistics
** things that don't make sense in a distributed environment.
*/
static const char *dbmsi_zero[] =
{
	"_cpu_ms",
	"_dio_cnt",
	"_bio_cnt",
	"dbms_bio",
	"dbms_dio",
	"dbms_cpu",
	"session_id",
	"_pfault_cnt",
	NULL
};

/* We'll turn some things like SESSION_USER into dbmsinfo functions,
** since that is how CX execution does it, and it's the simplest way
** to get there from here.
*/
static const DEFINE_DB_TEXT_STRING(dbmsi_versn,"_version");
static const DEFINE_DB_TEXT_STRING(dbmsi_sessuser, "session_user");
static const DEFINE_DB_TEXT_STRING(dbmsi_sysuser, "system_user");
static const DEFINE_DB_TEXT_STRING(dbmsi_inituser, "initial_user");

DB_STATUS
psl_proc_func(
	PSS_SESBLK	*sess_cb,
	PSF_MSTREAM	*stream,
	i4		op_code,
	PST_QNODE	*arg,
	PST_QNODE	**newnode,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    ADI_FI_TAB	        fi_tab;
    ADF_FN_BLK	        adf_funcblk;
    PST_CNST_NODE	cconst;
    PTR		        result;
    DB_DATA_VALUE       *res_data_value = &adf_funcblk.adf_r_dv;
    i4	        err_code;
    register i4         res_len = 0;    /* length of CHAR or VARCHAR text */
    DB_DATA_VALUE       *arg_val;
    DB_DATA_VALUE	arg_dbmsi;	/* For dbmsinfo hackery */
    bool		call_adf;

    /* Normally we'll call ADF to evaluate the function, but there are
    ** some special cases that don't.
    ** There are also some functions that translate into DBMSINFO and
    ** are easiest done that way.
    */
    call_adf = TRUE;
    arg_val = NULL;
    switch (op_code)
    {
	case ADI_DBMSI_OP: case ADI_BNTIM_OP: case ADI_ETSEC_OP:
	case ADI_CURDATE_OP: case ADI_CURTIME_OP: case ADI_CURTMSTMP_OP:
	case ADI_LCLTIME_OP: case ADI_LCLTMSTMP_OP:
	    break;	    
	case ADI_BIOC_OP: case ADI_CHRD_OP:  case ADI_CHREQ_OP:
	case ADI_CHRRD_OP: case ADI_CHSIZ_OP: case ADI_CHUSD_OP:
	case ADI_CHWR_OP: case ADI_CPUMS_OP: case ADI_DIOC_OP:
	case ADI_PFLTC_OP: case ADI_PHPAG_OP: case ADI_WSPAG_OP:
	case ADI_DBA_OP:   case ADI_USRNM_OP:
	    call_adf = FALSE;
	    break;	    
	case ADI_VERSN_OP:
	    op_code = ADI_DBMSI_OP;
	    arg_dbmsi.db_datatype = DB_VCH_TYPE;
	    arg_dbmsi.db_length = sizeof(dbmsi_versn);
	    arg_dbmsi.db_data = (PTR) &dbmsi_versn;
	    arg_val = &arg_dbmsi;
	    break;
	case ADI_SESSUSER_OP:
	    op_code = ADI_DBMSI_OP;
	    arg_dbmsi.db_datatype = DB_VCH_TYPE;
	    arg_dbmsi.db_length = sizeof(dbmsi_sessuser);
	    arg_dbmsi.db_data = (PTR) &dbmsi_sessuser;
	    arg_val = &arg_dbmsi;
	    break;
	case ADI_SYSUSER_OP:
	    op_code = ADI_DBMSI_OP;
	    arg_dbmsi.db_datatype = DB_VCH_TYPE;
	    arg_dbmsi.db_length = sizeof(dbmsi_sysuser);
	    arg_dbmsi.db_data = (PTR) &dbmsi_sysuser;
	    arg_val = &arg_dbmsi;
	    break;
	case ADI_INITUSER_OP:
	    op_code = ADI_DBMSI_OP;
	    arg_dbmsi.db_datatype = DB_VCH_TYPE;
	    arg_dbmsi.db_length = sizeof(dbmsi_inituser);
	    arg_dbmsi.db_data = (PTR) &dbmsi_inituser;
	    arg_val = &arg_dbmsi;
	    break;
	default:
	    return (E_DB_OK);
    }
    
    /*
    ** get the ADF function description.  We are using our inside
    ** knowledge that there is only 1 function instance.
    */
    if (adi_fitab(sess_cb->pss_adfcb, (ADI_OP_ID) op_code,
	    &fi_tab) != E_DB_OK)
    {
	(VOID) psf_error(E_PS0C05_BAD_ADF_STATUS, (DB_ERROR *)NULL, PSF_INTERR,
			 &err_code, err_blk,0);
	return (E_DB_ERROR);
    }

    /* get the length of the output */
    res_data_value->db_datatype = fi_tab.adi_tab_fi->adi_dtresult;

    /*
    ** initialize ptrs to argument's data_value; for funcs other
    ** than DBMSINFO(), argument is ignored
    ** Leave arg_val alone if already set above.
    */    
    if (arg_val == NULL)
    {
	arg_val = (arg == (PST_QNODE *) NULL || op_code != ADI_DBMSI_OP)
					    ? (DB_DATA_VALUE *) NULL
					    : &arg->pst_sym.pst_dataval;
	if (arg_val != NULL && arg->pst_sym.pst_dataval.db_datatype != DB_VCH_TYPE)
	    arg_val = NULL;
    }


    status = adi_0calclen(sess_cb->pss_adfcb,
	&fi_tab.adi_tab_fi->adi_lenspec, 1, &arg_val,
	res_data_value);

    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0C05_BAD_ADF_STATUS, (DB_ERROR *)NULL, PSF_INTERR,
			 &err_code, err_blk,0);
	return (E_DB_ERROR);
    }
    /* Trick out DBA and USERNAME to return varchar instead of c,
    ** since we'll be doing them by hand anyway.
    */
    if (op_code == ADI_DBA_OP || op_code == ADI_USRNM_OP)
    {
	res_data_value->db_datatype = DB_VCH_TYPE;
	res_data_value->db_length += DB_CNTSIZE;
    }

    /*
    ** ADF function block already has the result's type and length.
    ** Now we allocate space for DB_DATA for result
    */

    status = psf_malloc(sess_cb, stream, res_data_value->db_length,
		    &result, err_blk);
    if (status != E_DB_OK)
	return (status);

    res_data_value->db_data = result;

    if (op_code == ADI_DBMSI_OP)
    {
	char arg_string[20];	/* Enough for longest dbmsinfo arg
				** string plus trailing null
				*/
	const char **argtab;
	DB_TEXT_STRING *str;
	bool move_zero  = FALSE, move_blank = FALSE;

	/*
	** For the time being, if the parameter to dbmsinfo() is not a
	** quoted string, we will return a blank string as a result
	*/
	arg_string[0] = '\0';
	if (arg_val != NULL)
	{
	    str = (DB_TEXT_STRING *) arg_val->db_data;
	    if (str->db_t_count <= sizeof(arg_string)-1)
	    {
		memcpy(&arg_string[0], str->db_t_text, str->db_t_count);
		arg_string[str->db_t_count] = '\0';
	    }
	}
	/* Match up dbmsinfo arg with known possibilities.  Some
	** return constants, some (mostly performance-data related)
	** return zero ('0'), everything else returns blank.
	*/
	argtab = &dbmsi_call_adf[0];
	call_adf = FALSE;
	do
	{
	    if (STcasecmp(arg_string, *argtab) == 0)
	    {
		call_adf = TRUE;
		break;
	    }
	} while (*++argtab != NULL);
	if (! call_adf)
	{
	    argtab = &dbmsi_zero[0];
	    do
	    {
		if (STcasecmp(arg_string, *argtab) == 0)
		{
		    move_zero = TRUE;
		    break;
		}
	    } while (*++argtab != NULL);
	    if (! move_zero)
		move_blank = TRUE;
	}
	/* Set zero or blank into result as appropriate.
	** Note that we know / assume that DBMSINFO returns a varchar.
	*/
	str = (DB_TEXT_STRING *) result;
	if (move_zero)
	{
	    str->db_t_count = 1;
	    str->db_t_text[0] = '0';
	}
	else if (move_blank)
	{
	    str->db_t_count = 0;
	    str->db_t_text[0] = '\0';
	}
    } /* if dbmsinfo */
    else if (op_code == ADI_DBA_OP || op_code == ADI_USRNM_OP)
    {
	/* These have no ADF function call dispatch (they're normally done
	** via by-hand dispatch to adu-dbmsinfo in adeexecute.c).
	** Do them by hand here.  The result type is set to varchar above.
	*/
	DB_TEXT_STRING *str = (DB_TEXT_STRING *) result;
	char *putter, *taker;
	i4 len;

	if (op_code == ADI_DBA_OP)
	{
	    taker = &sess_cb->pss_dba.db_tab_own.db_own_name[0];
	    len = sizeof(sess_cb->pss_dba.db_tab_own.db_own_name);
	}
	else
	{
	    taker = &sess_cb->pss_user.db_own_name[0];
	    len = sizeof(sess_cb->pss_user.db_own_name);
	}
	putter = str->db_t_text;
	res_len = 0;
	while ((res_len + CMbytecnt(taker)) <= len && !CMspace(taker))
	{
	    res_len += CMbytecnt(taker);
	    CMcpyinc(taker, putter);
	}
	str->db_t_count = res_len;
    }
    else if (! call_adf)
    {
	i4 restype = res_data_value->db_datatype;
	i4 reslen = res_data_value->db_length;

	/* Don't think we can get nullable here, but be safe */
	if (restype < 0)
	{
	    restype = -restype;
	    --reslen;
	}
	/* These are all obsolete compatibility COP's such as _cpu_ms,
	** which are now replaced by their equivalent DBMSINFO functions.
	** These are all integer results.
	*/
	if (restype != DB_INT_TYPE)
	{
	    /* Supposed to be integer! */
	    (VOID) psf_error(E_PS0002_INTERNAL_ERROR, (DB_ERROR *)NULL, 
                                PSF_INTERR, &err_code, err_blk,0);
	    return (E_DB_ERROR);
	}
	switch (reslen)
	{
	case 8:
	    *(i8 *) (res_data_value->db_data) = 0;
	    break;
	case 4:
	    *(i4 *) (res_data_value->db_data) = 0;
	    break;
	case 2:
	    *(i2 *) (res_data_value->db_data) = 0;
	    break;
	case 1:
	    *(i1 *) (res_data_value->db_data) = 0;
	    break;
	}
    }

    if (call_adf)
    {
	adf_funcblk.adf_fi_id = fi_tab.adi_tab_fi->adi_finstid;
	adf_funcblk.adf_fi_desc = NULL;
	adf_funcblk.adf_dv_n = 0;
	if (arg_val != NULL)
	{
	    adf_funcblk.adf_dv_n = 1;
	    adf_funcblk.adf_1_dv = *arg_val;
	}
	adf_funcblk.adf_pat_flags = AD_PAT_DOESNT_APPLY;
	status = adf_func(sess_cb->pss_adfcb, &adf_funcblk);
	if (status != E_DB_OK)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS, (DB_ERROR *)NULL, 
                        PSF_INTERR, &err_code, err_blk,0);
	    return (E_DB_ERROR);
	}
    }

    /* Non-parameter text constant node */
    cconst.pst_tparmtype = PST_USER;
    cconst.pst_parm_no = 0;
    cconst.pst_pmspec  = PST_PMNOTUSED;
    cconst.pst_cqlang = DB_SQL;

    /*
    ** The value of the constant is the result of the evaluation
    ** of the function.  Caller gets to coerce the result (if needed)
    ** to a desired type.  (This applies more to COPY, which is calling
    ** by hand, than to Star which is calling via normal parsing.)
    ** For Star, set the "original text" which opf might need.
    */
    switch (res_data_value->db_datatype)
    {
	case DB_VCH_TYPE:
	{
	    char *p;
	    i2		origtxt_len;

	    res_len = ((DB_TEXT_STRING *) (res_data_value->db_data))->db_t_count;
	    /* Orig text is 'result', 2 quotes and a null */
	    origtxt_len = res_len + 2 + 1;

	    res_data_value->db_length = res_len + DB_CNTSIZE;

	    /* allocate space to store "original text */
	    status = psf_malloc(sess_cb, stream, origtxt_len,
				(PTR *)&cconst.pst_origtxt, err_blk);

	    if (status != E_DB_OK)
	    	return (status);

	    p = cconst.pst_origtxt;
	    *p++ = '\'';
	    memcpy(p, ((DB_TEXT_STRING*) result)->db_t_text, res_len);
	    p += res_len;
	    *p++ = '\'';
	    *p++ = EOS;
	    break;
	}
	case DB_INT_TYPE:
	{
	    i4	num;

	    /*
	    ** allocate space to store "original text"
	    ** largest i4 will fit into 12 bytes with some extra
	    ** room left
	    */
	    status = psf_malloc(sess_cb, stream, 12,
				(PTR *)&cconst.pst_origtxt, err_blk);
	    if (status != E_DB_OK)
		return (status);

	    switch (res_data_value->db_length)
	    {
	    case 8:
		num = (i4) (*(i8 *) res_data_value->db_data);
		break;
	    case 4:
		num = *(i4 *) res_data_value->db_data;
		break;
	    case 2:
		num = *(i2 *) res_data_value->db_data;
		break;
	    case 1:
		num = *(i1 *) res_data_value->db_data;
		break;
	    }

	    MEfill(12, ' ', (PTR) cconst.pst_origtxt);
	    CVla(num, cconst.pst_origtxt);
	    break;
	}
	/* FIXME what about the date pseudo-variables?  they are OK with
	** COPY defaults, but not sure about Star.
	*/
    }

    status = pst_node(sess_cb, stream, NULL, NULL,
		PST_CONST, (char *) &cconst, sizeof(cconst),
		res_data_value->db_datatype, 0, res_data_value->db_length,
		(DB_ANYTYPE *) result, newnode, err_blk, 0);

    return (status);
}
