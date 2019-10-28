/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <bt.h>
#include    <cm.h>
#include    <me.h>
#include    <qu.h>
#include    <tr.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>

/**
**
**  Name: PSTHEADER.C - Functions for creating query tree headers.
**
**  Description:
**      This file contains the functions for creating query tree headers.
**	Every query tree contains a header.  The header contains the query
**	mode, the range table for the query tree,
**	a flag telling whether it's a cursor definition "for update", a pointer
**	to the query tree, the sort list for the query tree (if any), the
**	number of repeat query parameters in the tree, the name of the repeat
**	query (if it is such), and the table id (or table name and owner) of
**	the result table, if any.  Other things may be added to the header
**	in the future.
**
**          pst_header - Allocate and fill in a query tree header.
**	    pst_modhdr - Update existing header.
**	    pst_cdb_cat- Query involves catalogs on CDB only
**
**  History:
**      01-apr-86 (jeff)    
**          written
**	18-mar-87 (stec)
**	    Added pst_resjour handling.
**	27-aug-87 (stec)
**	    Added pst_modhdr procedure.
**	02-oct-87 (stec)
**	    Cleaned up code in connection with PST_RESTAB.
**	29-feb-88 (stec)
**	    Check for VLUPs in the target list - temporary fix.
**	22-apr-88 (stec)
**	    pst_modhdr should update the mode in header.
**	24-oct-88 (stec)
**	    Change to code executed when dealing with cb->pss_resrng.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**      24-jul-89 (jennifer)
**          Added a new field to statement header pst_audit to point 
**          to security audit information required by QEF. 
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**	    For a declared global temporary table, the "owner" of the table
**	    is the special internally-generated session schema, not the user.
**	11-jun-92 (barbara)
**	    Merged Star code for Sybil.  Star history:
**	    15-aug-91 (kirke)
**		Removed MEfill() declaration - it's in me.h.
**	    16-mar-92 (barbara)
**		Allocate PST_QRYHDR_INFO structure.  This structure is used
**		to pass table id info to OPC where it is stored as part of the
**		QEP.  PSF uses this part of the QEP to decide if a Star repeat
**		query is shareable.  The method is taken from QUEL repeat
**		queries.
**	09-Nov-92 (rblumer)
**	    Change pst_header to initialize PST_STATEMENT nodes to all zero's
**	    (thus initializing new field pst_statementEndRules).
**	22-dec-92 (rblumer)
**	    Attach statement-level rules to statement from global list.
**	    Initialize new pst_flags field in PST_PROCEDURE nodes.
**	06-apr-93 (smc)
**	    Commented out text following endif.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	05-may-94 (davebf)
**	    Add routine pst_singlesite (called from pst_header) 
**	    to set PST_1SITE for OPF
**      12-apr-95 (rudtr01)
**          Fixed bug 68036.  In pst_cdb_cat(), changed all calls to
**          CMcmpcase() to CMcmpnocase().  Needed for FIPS installations
**          where identifiers are coerced to upper case.  Also tidied
**          the code to conform to the One True Standard.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions regarding ifnull and
**	    aggregates. Making the decision in opf/opa, as is now done, can
**	    cause problems when we detect a "do not flatten" situation but
**	    have already begun the process of query flattening.
**	    Do not flatten query if we have:
**	    - more than one IFNULL with an aggregate operand
**		OR
**	    - one IFNULL with an aggregate operand and another aggregate
**	      operand in the query
**	2-feb-06 (dougi)
**	    Replace pst_rngtree integer flag with PST_RNGTREE single bit.
**	12-Jun-2006 (kschendel)
**	    Fix a null/0 mixup causing compiler warnings.
**	13-May-2009 (kschendel) b122041
**	    Another of the same.
**      30-Oct-2009 (gefei01)
**          Return errors for various SQL features unsupported on X100 table.
**	01-apr-2010 (stial01)
**	    Changes for Long IDs
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	8-Mar-2011 (kschendel) m1586
**	    Rearrange x100 checking to streamline it.
**	24-may-2011 (stial01) m1627
**	    Check for any open cursors on x100 tables
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
i4 pst_header(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	i4 forupdate,
	PST_QNODE *sortlist,
	PST_QNODE *qtree,
	PST_QTREE **tree,
	PST_PROCEDURE **pnode,
	i4 mask,
	PSS_Q_XLATE *xlated_qry);
i4 pst_modhdr(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	i4 forupdate,
	PST_QTREE *header);
bool pst_cdb_cat(
	PSS_RNGTAB *rng_tab,
	PSQ_CB *psq_cb);
static i4 pst_proc_rngentry(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PST_QTREE *header,
	PSS_RNGTAB *usrrange_base,
	i4 idx,
	bool *is_iidd);
void pst_singlesite(
	PSS_RNGTAB *rng_tab,
	i4 *stype,
	DD_LDB_DESC **save_ldb_desc);
static void pst_chk_xtab_upd_const_only(
	PST_QTREE *header);
static bool walk(
	PST_QNODE *qn1);
static DB_STATUS pst_x100_check(
	PSS_RNGTAB *rngbase,
	PST_QTREE *header,
	DB_ERROR *errblk);
static void pst_chk_mult_corr_map(
	PST_QNODE *nodep,
	PST_J_MASK *mapp,
	bool *countp,
	bool *subselp);
static void pst_chk_mult_corr_count(
	PST_QNODE *nodep,
	bool *nocount);
static bool pst_chk_mult_corr(
	PST_QNODE *nodep,
	PST_J_MASK *omap,
	PST_J_MASK *imap,
	bool *nocount);

/* Local convenience macro to decide whether a range variable is an X100
** table.  Input is a PSS_RNGTAB * already determined to be PST_TABLE.
** Return TRUE if IVW storage structure, or if a view containing an X100
** table.  (Views presumably only occur for things that go thru pst-header
** without a preceding qrymod, if there are such things.)
*/
#define PST_IS_X100_MACRO(rng) \
    ( ((rng)->pss_rdrinfo->rdr_rel->tbl_storage_type > DB_STDING_STORE_MAX) \
     || ((rng)->pss_rdrinfo->rdr_rel->tbl_status_mask & DMT_VIEW \
	 && (rng)->pss_rdrinfo->rdr_rel->tbl_2_status_mask & DMT_X100_INVIEW \
	) \
    )

/*{
** Name: pst_header	- Allocate and fill in a query tree header
**
** Description:
**      This function allocates a query tree header and fills it in.  See the
**	description given above for what's in a query tree header.  In addition,
**	this function will do miscellaneous things to the query tree, like add
**	on the PST_TREE and PST_QLEND nodes, and copy the user's range table
**	from the session control block to the query tree's range table.
**
**	In addition, this function sets the QSF root of the query tree to point
**	to the query tree header.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	  pss_ostream			  For memory allocation
**	  pss_usrrange			  User's range table copied to query
**					  tree's range table
**	  pss_auxrng			  Auxiliary range table copied to
**					  query tree's range table for
**					  statements that have gone through
**					  qrymod.
**	  pss_resrng			  Result range variable info copied to
**					  range table.
**	  pss_restab			  Result structure info copied to header
**	  pss_highparm			  Largest parameter number for this tree
**	  pss_crsr			  Ptr to current cursor cb, if any
**	  pss_stmt_flags	    	  word containing various bitmasks:
**	    PSS_AGINTREE		  set if aggregates were found in tree
**	    PSS_SUBINTREE		  set if subselect was found in tree
**	  pss_flattening_flags		bits of info that help us determine 
**					whether a given DML query should be 
**					flattened or, if processing a view 
**					definition, which bits describing 
**					flattening-related characteristics of a
**					view will be set in pst_mask1
**      psq_cb                          Pointer to the user's control block
**	  psq_error			  Place to put error info it error
**					  happens
**	  psq_mode			  Query mode is copied to header.
**      forupdate                       Type of update allowed in cursor
**					    PST_DIRECT - Direct update
**					    PST_DEFER - Deferred update
**					    PST_READONLY - No update allowed
**      sortlist                        Pointer to the sort list, if any
**	qtree				Pointer to query tree to store in header
**	tree				Place to put pointer to header
**	mask				Miscellaneous node info:
**					    PST_1INSERT_TID -  pass to OPF
**					    PST_0FULL_HEADER - stmt/proc node
**	xlated_qry			ptr to structure used to administer
**					translated query text.
**
** Outputs:
**	sess_cb
**	  .pss_ostream			  Memory allocated
**      psq_cb
**	  .psq_error			  Filled in with error information,
**					  if any
**	qtree				Qualification put in conjunctive normal
**					form.  PST_TREE and PST_QLEND nodes
**					attached.
**	tree				Filled in with pointer to header
**	pnode				Filled in with ptr to procedure node
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**	    Sets root of query tree in QSF
**
** History:
**	01-apr-86 (jeff)
**          written
**	02-oct-87 (stec)
**	    Cleaned up code in connection with PST_RESTAB.
**	29-feb-88 (stec)
**	    Check for VLUPs in the target list - temporary fix
**	    for FEs (to be removed).
**	24-oct-88 (stec)
**	    Change to code executed when cb->pss_resrng defined.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**	26-jan-89 (neil)
**	    Add in rule processing tree to the "after" statement list.
**	    This tree was built in qrymod, by psy_rules.
**	14-apr-89 (neil)
**	    Updated old comment.
**	18-apr-89 (andre)
**	    pst_res2 renamed to pst_mask1.
**      24-jul-89 (jennifer)
**          Added a new filed to statement header pst_audit to point 
**          to security audit information required by QEF. 
**	15-nov-89 (andre)
**	    removed loop checking for illegal use of VLUPs as it is now done in
**	    SQL/QUEL grammar where we can make a more intelligent decision as
**	    the context is known.  It was done to fix bug 8638 where the
**	    abovementioned loop caused errors when VLUPs were found in the
**	    target list of subselect in INSERT...subselect.
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	25-jan-91 (andre)
**	    PST_QTREE.pst_res1 has been renamed to pst_info and will be used to
**	    contain an address of PST_QRYHDR_INFO structure.  pst_header() will
**	    be responsible for allocating this structure and initializing it.
**	    pst_header()'s caller may choose to store some meaningful
**	    information in *PST_QTREE.pst_info (e.g. for QUEL shareable repeat
**	    queries) or to leave them alone.
**	    If a tree is being built for an SQL query, set PST_SQL_QUERY bit in
**	    PST_QTREE.pst_mask1
**	08-Nov-91 (rblumer)
**	  merged from 6.4:  25-feb-91 (andre)
**	    make changes related to changes in PST_QTREE structure.
**	  merged from 6.4:  01-mar-91 (andre)
**	    for PSQ_DELCURS use pss_auxrng since pst_header() for DELETE CURSOR
**	    can be called either from PSLSGRAM.YI or from psq_dlcsrrow() where
**	    result table entry (if any) will also be entered into pss_auxrng.
**	2-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**	    For a declared global temporary table, the "owner" of the table
**	    is the special internally-generated session schema, not the user.
**	11-jun-92 (barbara)
**	    Merged Star code for Sybil.  Star history:
**	    07-oct-88 (andre)
**		added new arg - xlated_qry - for processing of distributed
**		queries
**	    01-nov-88 (andre)
**		Added code to send RANGE statement when translating
**		single-site QUEL queries in STAR.
**	    10-feb-89 (andre)
**		replaced hdrtype which was used to indicate whether or not
**		a full header was required with a mask field for greater
**		flexibility.
**	    6-feb-89 (andre)
**		QED_PSF_RANGE-->QED_1PSF_RANGE
**	    16-mar-89 (andre)
**		Save ptr to the last packet in the chain in 
**		qsite_info->pst_last_pkt
**	    05-apr-89 (andre)
**		OR pst_mask1 with PST_CDB_IIDD_TABLES if the query
**		references ONLY iidd_* tables located on CDB with exception
**		of iidd_stats and iidd_histograms.  Also OR pst_mask1 with
**		PST_CATUPD_OK if the user has catalog update privilege.
**	    20-apr-89 (andre)
**		queries referencing iidd_dbconstants will not be going over
**		privileged association.
**	    Because Star's single site query buffering is no longer done
**	    by PSF, some actions related to single-site queries have been
**	    eliminated for Sybil.  Star actions are to set special RDF flag
**	    if query is single-site against CDB catalogs; also to put
**	    distributed-only info into query tree for OPF.
**	05-oct-92 (barbara)
**	    If PSF has not saved up text, set pst_qtext to NULL for OPF.
**	09-Nov-92 (rblumer)
**	    Change code to initialize PST_STATEMENT nodes to all zero's
**	    (thus initializing new field pst_statementEndRules).
**	22-dec-92 (rblumer)
**	    Initialize new pst_flags field in PST_PROCEDURE nodes.
**	04-mar-93 (andre)
**	    when REPLACE CURSOR in QUEL, we will obtain description of the
**	    table/view on which cursor was defined and then let psy_qrymod()
**	    loose on it; description of the cursor's underlying base table will
**	    be placed into the query tree header
**	08-apr-93 (andre)
**	    sess_cb maintains four distinct lists of rules: user-defined
**	    row-level, system-generated row-level, user-defined statement-level,
**	    and system-generated statement-level.  System-generated row-level
**	    and statement-level rules need to be appended to user-defined
**	    row-level and statement-level rules, respectively.  The resulting
**	    two lists will be pointed to by pst_after_stmt and
**	    pst_statementEndRules.  Following this (unless we are processing
**	    PREPARE), we will voalesce row-level and statement-level rules.
**	    This used to be done in psy_rules() but with introduction of rules
**	    on views, it no longer seemed like a very good place to do it.
**	16-sep-93 (robf)
**         Set PST_UPD_ROW_SECLABEL in flags mask if so intructed by caller
**	   Leave pss_audit untouched until assigned to a statement (when
**	   full header is built). This prevents audit info for dbproc
**	   statements getting "lost".
**	17-nov-93 (andre)
**	    now that all the information needed to determine whether a query 
**	    should be flattened is available in sess_cb->pss_flattening_flags, 
**	    we can move code responsible for setting PST_NO_FLATTEN and 
**	    PST_CORR_AGGR bits from various places in PSLCTBL.C, PSLCVIEW.C, and
**	    PSLSGRAM.C into pst_header().  
**
**	    For DML queries (SELECT, DELETE, INSERT, UPDATE, CREATE TABLE AS 
**	    SELECT, DGTT AS SELECT) we will set (if appropriate) PST_NO_FLATTEN
**	    and PST_CORR_AGGR bits.  
**
**	    For PSQ_VIEW, we will translate various bits in pss_flattening_flags
**	    into corresponding bits in qtree->pst_mask1.  
**
**	    For DECLARE CURSOR (psq_cb->psq_mode == PSQ_DEFCURS) we will do 
**	    nothing here - pst_modhdr() will handle setting (if appropriate) 
**	    PST_NO_FLATTEN and PST_CORR_AGGR bits.
** 
**	    For remaining queries, bits in pss_flattening_flags will not even 
**	    be examined - this may need to change if we decide to add support 
**	    for rules that allow subselects (but then lots of other things may 
**	    need to change as well.)
**	6-dec-93 (robf)
**         Set need query syscat flag.
**	21-mar-94 (andre)
**	    fix for bug 58965:
**	    when populating a range entry describing the result range variable 
**	    for INSERT, we need to copy pss_[inner|outer]_rel into 
**	    pst_[inner|outer]_rel
**	05-may-94 (davebf)
**	    Add call to pst_singlesite to set PST_1SITE for OPF
**      23-june-1997(angusm)
**	    1. flag up cross-table updates
**          2. Validate tree for cross-table update query to allow
**          elimination of 'duplicate' rows where update only
**          involves constant values.
**	    (bugs 77040, 79623, 80750)
**	17-jul-2000 (hayke02)
**	    Set PST_INNER_OJ in pst_mask1 if PSS_INNER_OJ has been set in
**	    pss_stmt_flags.
**	26-Mar-2002 (thaju02)
**	    For dgtt as select involving a IN/OR subselect, set 
**	    PST_NO_FLATTEN bit. (B103567)
**	29-nov-2004 (hayke02)
**	    Init (pst_)stype with PST_1SITE rather than PST_NSITE so that STAR
**	    singlesite queries are now unflattened - see opa_1pass(). This
**	    change fixes problem INGSRV 3045, bug 113436.
**	 3-mar-2005 (hayke02)
**	    Set PST_MULT_CORR_ATTRS in pst_mask1 for select/retrieve as well
**	    as for views. We then test for PST_MULT_CORR_ATTRS in opa_1pass()
**	    when deciding whether or not to flatten single site STAR queries.
**	    This change fixes problem INGSTR 70, bug 114009.
**	16-jan-2006 (hayke02)
**	    Set PST_SUBSEL_IN_OR_TREE in pst_mask1 for select/retrieve as well
**	    as for views. We then test for PST_SUBSEL_IN_OR_TREE in opa_1pass()
**	    when deciding whether or not to flatten single site STAR queries.
**	    This change fixes bug 113941.
**	3-feb-06 (dougi)
**	    Stuff hint mask.
**	9-may-06 (dougi)
**	    Add support for trace point ps151 (enables 6.4 ambiguous update).
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	17-jan-2007 (dougi)
**	    Set PST_SCROLL flag for scrollable cursors.
**	12-feb-2007 (dougi)
**	    ... and PST_KEYSET.
**      03-jun-2005 (gnagn01)
**          Set PST_NO_FLATTEN bit for a select with case operator and 
**          aggregate.Bug 113574,INGSRV3073.
**	04-oct-2007 (toumi01) BUG 119250
**	    Fix setting of PST_CDB_IIDD_TABLES to avoid clobbering pst_mask1.
**	    Doing this broke createdb for Star.
**      10-dec-2007 (smeke01) b118405
**          Add AGHEAD_MULTI, IFNULL_AGHEAD and IFNULL_AGHEAD_MULTI handling 
**          to PSQ_VIEW, PSQ_DEFCURS & PSQ_DGTT_AS_SELECT cases. Restructured 
**          PSQ_RETRIEVE etc case so that the setting of pst_mask1 flags 
**          relevant to views is done in one place.
**	26-aug-2008 (dougi)
**	    Check for mixture of X100 and Ingres tables - not allowed.
**	13-feb-2009 (dougi)
**	    X100 update operations are now allowed.
**	13-Aug-2009 (kiria01)
**	    Don't necessarily count views as being non-x100.
**      30-Oct-2009 (gefei01)
**          Return errors for various SQL features unsupported on X100 table.
**	22-apr-2010 (dougi)
**	    Add error for IVW repeat query with host variables (ok with no
**	    host vars).
**	5-june-2010 (dougi)
**	    Drop some superfluous IVW code and set pst_struct (also for IVW).
**	18-aug-2010 (dougi)
**	    Check for "next value" sequence ops in X100 query. Also moved
**	    X100TAB_INQ flag to stmt_flags2.
**	30-sep-2010 (dougi)
**	    Prepare for mixed native Ingres/X100 queries and disallow 
**	    positioned upd/del on IVW tables.
**	16-Mar-2011 (kschendel) m1541
**	    Remove restriction on parameterized repeated IVW queries.
**	21-apr-2011 (dougi) m1457
**	    Unset MULT_CORR_ATTRS if triggered by column in select list 
**	    under EXISTS.
**	28-Apr-2011 (kschendel) m1920
**	    Allow SCROLL cursors for VW tables.  Continue to prohibit KEYSET
**	    (updatable) cursor, though.
**      12-Jul-2012 (hanal04 & horda03) Bug 125497
**          Initialise new pst_fe_lname field.
*/
DB_STATUS
pst_header(
	PSS_SESBLK      *sess_cb,
	PSQ_CB		*psq_cb,
	i4		forupdate,
	PST_QNODE	*sortlist,
	PST_QNODE	*qtree,
	PST_QTREE	**tree,
	PST_PROCEDURE   **pnode,
	i4		mask,
	PSS_Q_XLATE	*xlated_qry)
{
    DB_STATUS           status;
    PST_QTREE		*header;
    register PST_QNODE	*qnode;
    register PSS_RNGTAB	*usrrange, *usrrange_base;
    register i4	i;
    bool		is_iidd = FALSE;
    i4			stype;
    DD_LDB_DESC		*ldb_desc = (DD_LDB_DESC *)NULL;
    extern PSF_SERVBLK  *Psf_srvblk;
    i4		val1;
    i4		val2;
    i4		err_code;

    /* Various things are not allowed if IVW table anywhere in the query.
    ** Note that this includes the target (if updating DML) and views (thanks
    ** to the x100-inview flag from dmf/rdf).
    */
    if (sess_cb->pss_stmt_flags2 & PSS_2_X100TAB_INQ)
    {
	/* X100 table and table procedure are not allowed
	 * in the same query.
	 */
	if (sess_cb->pss_ses_flag & PSS_TPROC_INQ)
	{
	    (VOID)psf_error(9465L,  0L, PSF_USERERR,
                        &err_code, &psq_cb->psq_error, 0);

	    return(E_DB_ERROR);
	}
 
	/* Keyset (updatable scrollable) cursor on VW table is not supported */
	if ((psq_cb->psq_mode == PSQ_DEFCURS) &&
	    (sess_cb->pss_stmt_flags & PSS_KEYSET))
	{
	    (void)psf_error(E_US24F8_9464_NO_KEYSET_VW, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 0);

	    return E_DB_ERROR;
	}

	/* CREATE RULE on X100 table is unsupported. */
	if (psq_cb->psq_mode == PSQ_RULE)
	{
	    (void)psf_error(9467L, 0L, PSF_USERERR, &err_code,
                        &psq_cb->psq_error, 0);

	    return E_DB_ERROR;
	}

	/* X100 table and NEXT VALUE operation (on sequence) aren't allowed
	 * in the same query.
	 */
	if (sess_cb->pss_stmt_flags2 & PSS_2_NEXTVAL_INQ)
	{
	    (VOID)psf_error(9476L,  0L, PSF_USERERR,
                        &err_code, &psq_cb->psq_error, 0);

	    return(E_DB_ERROR);
	}

	/* Positioned upd/del not allowed on IVW tables. */
	if (psq_cb->psq_mode == PSQ_REPCURS ||
	    psq_cb->psq_mode == PSQ_DELCURS)
	{
	    psf_error(9481, 0L, PSF_USERERR, 
			&err_code, &psq_cb->psq_error, 0);
	    return(E_DB_ERROR);
	}

	/* Check for open cursor on x100 table */
	if (sess_cb->pss_numcursors 
		&& psq_any_vwtab_cursor(sess_cb, &psq_cb->psq_error) )
	{
	    psf_error(9492, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 0);
	    return(E_DB_ERROR);
	}

	/* This isn't everything.  More checks below where convenient. */
    }
    else if (sess_cb->pss_stmt_flags2 & PSS_WINDOW_INQ)
    {
	/* Make sure this is a VectorWise query. */
	_VOID_ psf_error(9487, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }
 
    *tree = (PST_QTREE *) NULL;
    *pnode = (PST_PROCEDURE *) NULL;

    /* set initial value of stype.  PST_1SITE may be over-ridden */
    stype = PST_1SITE;
    /*stype = PST_NSITE;*/
    /* Note: preceeding line turns "off" distributed singlesite unflattening.
    ** When it is desired to turn it on and finish debugging it (b58068),
    ** replace with    stype = PST_1SITE */

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_QTREE),
	(PTR *) &header, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
	return (status);
    }

    MEfill(sizeof (PST_QTREE), 0, (PTR) header);

    /* pst_rgtype field in the header range table needs to initialized
    ** to PST_UNUSED value, but the fact that MEfill initializes it
    ** to NULL and PST_UNUSED is 0, allow us to take no specific action.
    */

    /*
    ** Fill in query tree header.
    */

    /* The query mode */
    header->pst_mode = psq_cb->psq_mode;

    /* The version */
    header->pst_vsn = PST_CUR_VSN;

    /* Indicate if any aggregates in tree */
    header->pst_agintree = ((sess_cb->pss_stmt_flags & PSS_AGINTREE) != 0);
    /* indicate subselects */
    header->pst_subintree= ((sess_cb->pss_stmt_flags & PSS_SUBINTREE) != 0);

    header->pst_hintmask = sess_cb->pss_hint_mask;
    sess_cb->pss_hint_mask = 0;
    header->pst_hintcount = sess_cb->pss_hintcount;
    if (header->pst_hintcount == 0)
	header->pst_tabhint = (PST_HINTENT *) NULL;
    else
    {
	/* Allocate space for hint array and copy the whole thing. */
	i = header->pst_hintcount;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
		i * sizeof(PST_HINTENT), (PTR)&header->pst_tabhint,
		&psq_cb->psq_error);
	if (status != E_DB_OK)
	{
	    return (status);
	}
	MEcopy((char *)&sess_cb->pss_tabhint, i * sizeof(PST_HINTENT),
		(char *)header->pst_tabhint);
    }

    /*
    ** Copy range table to query tree.  If the statement has gone through
    ** qrymod, use the auxiliary range table, otherwise use the user's
    ** range table.
    */
    if (psq_cb->psq_qlang == DB_SQL
	||
	psq_cb->psq_mode == PSQ_RETRIEVE || psq_cb->psq_mode == PSQ_RETINTO ||
	psq_cb->psq_mode == PSQ_APPEND   || psq_cb->psq_mode == PSQ_REPLACE ||
	psq_cb->psq_mode == PSQ_DELETE   || psq_cb->psq_mode == PSQ_DEFCURS ||
	psq_cb->psq_mode == PSQ_DELCURS  || psq_cb->psq_mode == PSQ_REPCURS ||
	psq_cb->psq_mode == PSQ_DGTT_AS_SELECT ||
	psq_cb->psq_mode == PSQ_X100_DGTT_AS)
    {
	usrrange_base = sess_cb->pss_auxrng.pss_rngtab;

	/* remember number of entries in the range table */
	header->pst_rngvar_count = sess_cb->pss_auxrng.pss_maxrng + 1;
    }
    else
    {
	usrrange_base = sess_cb->pss_usrrange.pss_rngtab;

	/* remember number of entries in the range table */
	header->pst_rngvar_count = sess_cb->pss_usrrange.pss_maxrng + 1;
    }

    /*
    ** if this is OPEN CURSOR statement or PREPAREd SELECT/RETRIEVE, we don't
    ** really know how many entries there are in the range table (since qrymod
    ** will be performed later), so in such cases we will allocate the maximum
    ** number of pointers, since otherwise we may have problems later on in
    ** pst_modhdr().
    */
    if(psq_cb->psq_mode == PSQ_DEFCURS ||
       (psq_cb->psq_mode == PSQ_RETRIEVE && sess_cb->pss_defqry == PSQ_PREPARE))
    {
	i = PST_NUMVARS;
    }
    else
    {
	i = header->pst_rngvar_count;
    }

    /* for some queries there may be no entries to allocate */
    if (i == 0)
    {
	header->pst_rangetab = (PST_RNGENTRY **) NULL;
    }
    else
    {
	/*
	** allocate and initilize an array of pointers to range entry structure
	*/ 
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, i * sizeof(PST_RNGENTRY *),
	    (PTR *) &header->pst_rangetab, &psq_cb->psq_error);
	if (status != E_DB_OK)
	{
	    return (status);
	}

	for (i--; i >= 0; i--)
	{
	    header->pst_rangetab[i] = (PST_RNGENTRY *) NULL;
	}

	usrrange = usrrange_base;
	for (i = 0; i < PST_NUMVARS; i++, usrrange++)
	{
	    /*
	    ** Only build the range table for those range variables that have
	    ** had range variable numbers assigned to them for this query.
	    ** (and that haven't been recursively built already)
	    */
	    if (usrrange->pss_rgno != -1 && usrrange->pss_used &&
		!header->pst_rangetab[usrrange->pss_rgno])
	    {
		status = pst_proc_rngentry(sess_cb, psq_cb, header, 
		    usrrange_base, i, &is_iidd);
		if (status != E_DB_OK)
		{
		    return(status);
		}

		if (sess_cb->pss_distrib & DB_3_DDB_SESS)
		{
		    /* set stype for use below */
		    pst_singlesite(usrrange, &stype, &ldb_desc);
		}
	    }
	}
	/* Check for IVW tables, set mask1. */
	status = pst_x100_check(usrrange_base, header, &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Update mode indicator */
    header->pst_updtmode = forupdate;

    /* The query tree */
    header->pst_qtree = qtree;

    /*
    ** this field will be used (starting with 6.5) to store ID of a table whose
    ** timestamp must be changed to force invalidation of QEPs which may have
    ** become obsolete as a result of destruction of a view or a permit; it will
    ** also be used to force invalidation of QEPs of database procedures when a
    ** database procedure or a permit on a database procedure gets dropped
    */
    header->pst_basetab_id.db_tab_base =
	header->pst_basetab_id.db_tab_index = (i4) 0;
    
    /* is this a repeat query */
    if (sess_cb->pss_defqry == PSQ_50DEFQRY ||
	sess_cb->pss_defqry == PSQ_DEFQRY ||
	sess_cb->pss_defqry == PSQ_DEFCURS)
    {
	header->pst_mask1 |= PST_RPTQRY;
    }

    /* The sort list */
    if (sortlist != (PST_QNODE *) NULL)
    {
	header->pst_stflag = TRUE;
    }
    else
    {
	header->pst_stflag = FALSE;
    }
    header->pst_stlist = sortlist;

    /* The number of repeat query parameters (add 1 since they start at 0) */
    header->pst_numparm = sess_cb->pss_highparm + 1;

    /*
    ** I don't think this need to be copied for cases other than RETRIEVE
    ** INTO/CREATE AS SELECT, but I'll let it be for the time being
    */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_restab, header->pst_restab);

    /* The result range variable, if any */
    if (sess_cb->pss_resrng != (PSS_RNGTAB *) NULL)
    {
	/* Copy some data from pss_resrng */
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
	    header->pst_restab.pst_restabid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname,
	    header->pst_restab.pst_resname);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_ownname,
	    header->pst_restab.pst_resown);
	header->pst_restab.pst_resvno = sess_cb->pss_resrng->pss_rgno;
	if (PST_IS_X100_MACRO(sess_cb->pss_resrng))
	    header->pst_mask1 |= PST_X100_TARGET;

	/*
	** unlike pst_modhdr(), here we cannot assume that the range entry
	** information has been copied:
	**   for INSERT/APPEND cb->pss_resrng will be pointing at
	**   pss_usrrange->pss_rsrng or pss_auxrange->pss_rsrng (for QUEL and
	**   SQL, respectively), and the above loop does not try to copy these
	**   entries
	*/

	/*
	** allocate a range variable entry and populate it if it has not been
	** allocated (and populated) yet
	*/
	if (header->pst_rangetab[header->pst_restab.pst_resvno] ==
	    (PST_RNGENTRY *) NULL)
	{
	    if ((status = pst_proc_rngentry(sess_cb, psq_cb, header, 
		sess_cb->pss_resrng, -1, &is_iidd)))
		return(status);
	}
	else if (sess_cb->pss_distrib & DB_3_DDB_SESS && !is_iidd)
	{

	    /*
	    ** check if the table is on CDB and has a name starting
	    ** with "iidd_". Queries against these tables will use
	    ** the privileged association.
	    */ 
	    is_iidd = pst_cdb_cat(sess_cb->pss_resrng, psq_cb); 
	}
    }

    /*
    ** On a retrieve into, the table name and owner are in a different place.
    */
    if (header->pst_mode == PSQ_RETINTO)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
				header->pst_restab.pst_resown);
    }
    else if (header->pst_mode == PSQ_DGTT_AS_SELECT ||
	header->pst_mode == PSQ_X100_DGTT_AS)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_sess_owner,
				header->pst_restab.pst_resown);
    }

    /*
    ** Copy the cursor id, if any, to the header.
    ** Copy the update map, if any, to the header.
    */
    if (sess_cb->pss_crsr != (PSC_CURBLK *) NULL)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_blkid, header->pst_cursid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_iupdmap, header->pst_updmap);
    } else if ((sess_cb->pss_defqry == PSQ_DEFCURS) ||
	       (sess_cb->pss_defqry == PSQ_50DEFQRY) ||
	       (sess_cb->pss_defqry == PSQ_DEFQRY))
    {
	STRUCT_ASSIGN_MACRO(psq_cb->psq_cursid, header->pst_cursid);
    }

    /*
    ** If replace statment, set the updmap to contain the updated attributes
    ** All elements in the target list are updateable.
    */
    if (psq_cb->psq_mode == PSQ_REPLACE)
    {
	for (qnode = qtree->pst_left; qnode != (PST_QNODE*) NULL 
			    && qnode->pst_sym.pst_type == PST_RESDOM;
	     qnode = qnode->pst_left)
	{
	    BTset((i4) qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
		(char *) &header->pst_updmap);
	}
    }
    /*
    ** Assume no delete permission given for now, unless this is a delete
    ** command.
    */
    header->pst_delete = psq_cb->psq_mode == PSQ_DELETE;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_QRYHDR_INFO),
	(PTR *) &header->pst_info, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
	return (status);
    }

    header->pst_info->pst_1_usage = 0L;	    /*
					    ** this field may get reset later,
					    ** if there is a need to use the
					    ** structure
					    */ 

    if (is_iidd == TRUE)
	header->pst_mask1 |= PST_CDB_IIDD_TABLES;

    if (sess_cb->pss_stmt_flags & PSS_SCROLL)
    {
	header->pst_mask1 |= PST_SCROLL;	/* scrollable cursor */
	if (sess_cb->pss_stmt_flags & PSS_KEYSET)
	    header->pst_mask1 |= PST_KEYSET;	/* keyset scrollable cursor */
    }

    if (sess_cb->pss_stmt_flags2 & PSS_WINDOW_INQ)
	header->pst_mask1 |= PST_WINDOW_IN_QUERY;	/* window func */

    /* remember language of the query */
    if (sess_cb->pss_lang == DB_SQL)
    {
	header->pst_mask1 |= PST_SQL_QUERY;
    }

    if (mask & PST_1INSERT_TID)
    {
	header->pst_mask1 |= PST_TIDREQUIRED;
    }

    /*
    ** Set qry_syscat flag. Note: This should really only be set
    ** when the query needs it (see qrymod), not just when the user
    ** has it.
    */
    if(psy_ckdbpr(psq_cb, DBPR_QUERY_SYSCAT)==E_DB_OK)
    {
	header->pst_mask1 |= PST_NEED_QRY_SYSCAT;
    }

    /* if user has catalog update privilege, set a bit for OPF */
    if (sess_cb->pss_ses_flag & PSS_CATUPD)
    {
	header->pst_mask1 |= PST_CATUPD_OK;
    }

    if (sess_cb->pss_flattening_flags & PSS_MULT_CORR_ATTRS)
    {
	/* Double check the MULT_CORR flag - parser isn't selective enough. */
	bool	nocount = TRUE;

	if (!pst_chk_mult_corr(header->pst_qtree, (PST_J_MASK *) NULL,
			(PST_J_MASK *) NULL, &nocount))
	    sess_cb->pss_flattening_flags &= ~PSS_MULT_CORR_ATTRS;
    }

    /*
    ** translate bits in sess_cb->pss_flattening_flags according to the mode of 
    ** the query
    */
    switch (psq_cb->psq_mode)
    {
	case PSQ_RETRIEVE:
	{
	    if (sess_cb->pss_defqry == PSQ_PREPARE)
	    {
		/* 
		** if preparing a SELECT, psy_qrymod() has not been called yet,
		** we will leave it up to pst_modhdr() (which gets called as a 
		** part of opening a cursor) to set (if appropriate) 
		** PST_NO_FLATTEN and PST_CORR_AGGR
		*/
		break;
	    }
	}
	case PSQ_RETINTO:
	case PSQ_DELETE:
	case PSQ_REPLACE:
	case PSQ_APPEND:
	case PSQ_DEFCURS:
	case PSQ_DGTT_AS_SELECT:
	case PSQ_X100_DGTT_AS:
	{
	    /*
	    ** do not flatten trees which contain SUBSEL node s.t. it is a
	    ** descenedant of OR node;
	    ** ... the same applies to trees involving ALL ... SELECT
	    ** ... the same applies to trees containing multiple correlated
	    **     references in a tree rooted in EXIST which is a descendant 
	    **     of NOT or a SUBSEL with count() as one of the elements of 
	    **	   the target list
	    ** ... the same applies to queries involving singleton subselect(s)
	    **	   if we were told at server startup that such queires should 
	    **	   not be flattened
            ** ... the same applies to queries with case operator and aggregate
	    */
	    if (   (sess_cb->pss_flattening_flags &
			(  PSS_SUBSEL_IN_OR_TREE | PSS_ALL_IN_TREE 
			 | PSS_MULT_CORR_ATTRS))
		|| (   Psf_srvblk->psf_flags & PSF_NOFLATTEN_SINGLETON_SUBSEL
		    && sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
                || (sess_cb->pss_flattening_flags & PSS_CASE_AGHEAD )
	       )
	    {
		header->pst_mask1 |= PST_NO_FLATTEN;
	    }

            /* b110577, INGSRV2413
            ** ... the same applies to queries with more than one IFNULL
            **     with an aggregate operand OR one IFNULL with an aggregate
            **     operand and another aggregate in the query
            */
            if (( sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD_MULTI )
                || ( sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD
                    && sess_cb->pss_flattening_flags & PSS_AGHEAD_MULTI )
               )
            {
                header->pst_mask1 |= PST_AGHEAD_NO_FLATTEN;
            }
             
	    if (sess_cb->pss_stmt_flags & PSS_XTABLE_UPDATE)
	    {
		header->pst_mask1 |= PST_XTABLE_UPDATE;
	    }
	    else
	    {
		header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	    }

	    /* 
	    ** This drops through to PSQ_VIEW intentionally so as to keep the 
	    ** pst_mask1 code for PSQ_VIEW in line with PSQ_RETRIEVE/PSQ_RETINTO.
	    ** This is so that we make sure that exactly the same flags are 
	    ** stored (and subsequently restored when the view is used) for a 
	    ** view as used for a retrieve. 
	    ** 
	    ** We don't store PST_NO_FLATTEN or PST_AGHEAD_NO_FLATTEN as this is 
	    ** determined from the other flags that we do store. We also don't 
	    ** want to store PST_XTABLE_UPDATE as this is not a flag that could 
	    ** occur on a view.
	    */

	}

	case PSQ_VIEW:
	{
	    if (sess_cb->pss_flattening_flags & PSS_CORR_AGGR)
	    {
		header->pst_mask1 |= PST_CORR_AGGR;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_MULT_CORR_ATTRS)
	    {
		header->pst_mask1 |= PST_MULT_CORR_ATTRS;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
	    {
		header->pst_mask1 |= PST_SINGLETON_SUBSEL;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_SUBSEL_IN_OR_TREE)
	    {
		header->pst_mask1 |= PST_SUBSEL_IN_OR_TREE;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_ALL_IN_TREE)
	    {
		header->pst_mask1 |= PST_ALL_IN_TREE;
	    }

	    if (sess_cb->pss_stmt_flags & PSS_INNER_OJ)
	    {
		header->pst_mask1 |= PST_INNER_OJ;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_AGHEAD_MULTI)
	    {
		header->pst_mask1 |= PST_AGHEAD_MULTI; 
	    }

	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD)
	    {
		header->pst_mask1 |= PST_IFNULL_AGHEAD;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD_MULTI)
	    {
		header->pst_mask1 |= PST_IFNULL_AGHEAD_MULTI;
	    }

	    break;
	}

	default:
	    /* 
	    ** no translation of "flattening" bits needed for remaining types
	    ** of queries (at least for now)
	    */
	    break;
    }

    /*
    ** Do final processing of query tree.  Set variable bit maps.  Count
    ** variables.
    */
    status = pst_trfix(sess_cb, &sess_cb->pss_ostream, qtree, &psq_cb->psq_error);

    /*
    ** Make output pointer point to new header if we got through everything
    ** with no errors.
    */
    if (status == E_DB_OK)
	*tree = header;

    /*
    ** If hdrtype is TRUE a full header has been requested, i.e.,
    ** PROCEDURE and STATEMENT nodes should be generated.
    */
    if (mask & PST_0FULL_HEADER)
    {
	PST_STATEMENT	*snode;
	PST_PROCEDURE   *prnode;

	/* Allocate statement node. */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_STATEMENT),
	    (PTR *) &snode, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	MEfill(sizeof(PST_STATEMENT), 0, (PTR) snode);

	snode->pst_mode = psq_cb->psq_mode;
	snode->pst_type = PST_QT_TYPE;
	snode->pst_specific.pst_tree = header;

	/* Assign row-level and statement-level rules from saved rule lists 
	 */

	/*
	** make pst_after_stmt point at user-defined row-level rules and then
	** append system-generated row-level rules to the end of the list
	*/
	snode->pst_after_stmt        = sess_cb->pss_row_lvl_usr_rules;
	psy_rl_coalesce(&snode->pst_after_stmt, sess_cb->pss_row_lvl_sys_rules);

	/*
	** make pst_statementEndRules point at user-defined statement-level
	** rules and then append system-generated statement-level rules to the
	** end of the list
	*/
	snode->pst_statementEndRules = sess_cb->pss_stmt_lvl_usr_rules;
	psy_rl_coalesce(&snode->pst_statementEndRules,
	    sess_cb->pss_stmt_lvl_sys_rules);

	/*
	** same for "before" rules.
	*/
	snode->pst_before_stmt        = sess_cb->pss_row_lvl_usr_before_rules;
	psy_rl_coalesce(&snode->pst_before_stmt, 
				sess_cb->pss_row_lvl_sys_before_rules);

	snode->pst_before_statementEndRules = 
				sess_cb->pss_stmt_lvl_usr_before_rules;
	psy_rl_coalesce(&snode->pst_before_statementEndRules,
				sess_cb->pss_stmt_lvl_sys_before_rules);

	/*
	** finally, unless we are processing PREPARE, append statement-level
	** rules to the end of the list of row-level rules (after and before)
	*/
	if (sess_cb->pss_defqry != PSQ_PREPARE)
	{
	    psy_rl_coalesce(&snode->pst_after_stmt,
		snode->pst_statementEndRules);
	    psy_rl_coalesce(&snode->pst_before_stmt,
		snode->pst_before_statementEndRules);
	}
	
	sess_cb->pss_row_lvl_usr_rules  =
	sess_cb->pss_row_lvl_sys_rules  =
	sess_cb->pss_stmt_lvl_usr_rules =
	sess_cb->pss_stmt_lvl_sys_rules =
	sess_cb->pss_row_lvl_usr_before_rules  =
	sess_cb->pss_row_lvl_sys_before_rules  =
	sess_cb->pss_stmt_lvl_usr_before_rules =
	sess_cb->pss_stmt_lvl_sys_before_rules = (PST_STATEMENT *) NULL;

	/* Assign the audit information from session pointer. */
	snode->pst_audit = sess_cb->pss_audit;

        sess_cb->pss_audit = (PTR)NULL;

	snode->pst_lineno = sess_cb->pss_lineno;

	/* Allocate procedure node. */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_PROCEDURE),
	    (PTR *) &prnode, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	prnode->pst_mode = (i2) 0;
	prnode->pst_vsn = (i2) PST_CUR_VSN;
	prnode->pst_isdbp = FALSE;
	prnode->pst_flags = 0;
	prnode->pst_stmts = snode;
	prnode->pst_parms = (PST_DECVAR *) NULL;
	STRUCT_ASSIGN_MACRO(header->pst_cursid, prnode->pst_dbpid);
        prnode->pst_fe_lname = 0;

	*pnode = prnode;
    }

    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_LONG_QRY_TREE_TRACE, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree:\n\n\n");
	status = pst_prmdump(qtree, header, &psq_cb->psq_error, DB_PSF_ID);
    }
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_SHORT_QRY_TREE_TRACE, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree:\n\n\n");
	status = pst_1prmdump(qtree, header, &psq_cb->psq_error);
    }

    /*
    ** for distributed INGRES we need to indicate type (by site) of the query
    ** as well as provide location and translated text for single-site queries.
    */

    if (sess_cb->pss_distrib & DB_3_DDB_SESS) 
    {
	PST_DSTATE	*qsite_info;
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_DSTATE),
	                    (PTR *) &qsite_info, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	qsite_info->pst_stype    = stype;
	qsite_info->pst_ldb_desc = (DD_LDB_DESC *) NULL;  /* obsolete? */
	qsite_info->pst_qtext = (DD_PACKET *) NULL;

	/*
	** In some cases we may have something to pass to OPC when
	** processing multi-site queries, e.g. when dealing with
	** unknown options in the WITH clause in
	** CREATE TABLE...AS SELECT;  this information (if any) will be
	** pointed to by xlated_qry->pss_q_list.pss_head
	*/
	if (xlated_qry != (PSS_Q_XLATE *) NULL)
	{
	    qsite_info->pst_qtext = xlated_qry->pss_q_list.pss_head;
	}
		
	header->pst_distr = qsite_info;
    } /* if DISTRIBUTED */

/*
** 6.4 compat mode: duplicate elimination AND loose ambig replace
** checking
*/
    if (header->pst_mask1 & PST_XTABLE_UPDATE)
    {

	if (Psf_srvblk->psf_flags & PSF_AMBREP_64COMPAT ||
	    ult_check_macro(&sess_cb->pss_trace, PSS_AMBREP_64COMPAT,
				&val1, &val2))
	{
	    header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	    header->pst_mask1 |= PST_UPD_CONST;
	}
	else
    	    pst_chk_xtab_upd_const_only(header);

    }
    return (status);
}

/*{
** Name: pst_modhdr - Update an existing tree header.
**
** Description:
**      This function modifies an existing tree header. In SQL "open cursor"
**	statement creates header before psy_qrymod is run. This is sort of
**	necessary, because of the way things are set up in terms of productions.
**	The header needs to be updated after running qrymod algorithm. So to
**	localize the changes this module was written.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	  pss_auxrng			  Auxiliary range table copied to
**					  query tree's range table for
**					  statements that have gone through
**					  qrymod.
**	  pss_resrng			  Result range variable info copied to
**					  range table.
**	  pss_restab			  Result object info copied to header
**	  pss_highparm			  Largest parameter number for this tree
**	  pss_flag			  flag word containing various bitflags:
**	    PSS_AGINTREE		  set if aggregates were found in tree
**	    PSS_SUBINTREE		  set if subselect was found in tree
**	  pss_flattening_flags		bits of info that help us determine 
**					whether a given DML query should be 
**					flattened or, if processing a view 
**					definition, which bits describing 
**					flattening-related characteristics of a
**					view will be set in pst_mask1
**      psq_cb                          Pointer to the user's control block
**	  psq_mode			  Query mode is copied to header.
**	  psq_error			  Place to put error info it error
**					  happens
**      forupdate                       Type of update allowed in cursor
**					    PST_DIRECT - Direct update
**					    PST_DEFER - Deferred update
**					    PST_READONLY - No update allowed
**	tree				Pointer to existing header
**
** Outputs:
**      psq_cb
**	  .psq_error			Filled in with error info, if any.
**	tree				Updated header.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	28-aug-86 (stec)
**          written.
**	22-apr-88 (stec)
**	    pst_modhdr should update the mode in header.
**	24-oct-88 (stec)
**	    Change to code executed for cb->pss_resrng.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	08-nov-91 (rblumer)
**	  merged from 6.4:  25-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    PST_RNGENTRY pointers instead of an array of structures.  It is
**	    possible, that space was not allocated for one or more entries, so
**	    it will be allocated here
**	  merged from 6.4:  11-mar-91 (andre)
**	    Overwrite information in the tree header with that from the PSF
**	    range table without checking if the entries (in the header and PSF
**	    range tables) contain the same information.  Originally, I was
**	    comparing table ids and making sure that the entry does not
**	    represent a non-mergeable view.  As it turns out, all other things
**	    being the same, the timestamp may have changed.  Rather than add one
**	    more check, we will just blindly overwrite the information.
**	16-aug-93 (andre)
**	    copy the inner and the outer relation maps from the pss_auxrng
**	    entries into the header range table entries.  Forgetting to do it
**	    resulted in errors trying to open cursors defined on views with
**	    outer joins
**	18-nov-93 (andre)
**	    added code to set (if appropriate) PST_NO_FLATTEN and PST_CORR_AGGR 
**	    bits in header->pst_mask1; if processing PREPARE SELECT or 
**	    DECLARE CURSOR, we didn't even try to set these bits in pst_header()
**	    since at that point we were yet to pump the tree through qrymod 
**	    which may discover new information which may affect the decision 
**	    whether these bits need to be set
**	jun-26-96 (inkdo01)
**	    Fixed nasty bug which resulted in overlaying other nice folks'
**	    memory with range table entries.
**	1-june-2007 (dougi)
**	    Set PST_KEYSET for scrollable cursors with implicit update. 
**	29-apr-2010 (dougi)
**	    Add checks for X100 tables to enable Quel (& SQL) cursor'ed queries.
**	30-sep-2010 (dougi)
**	    Prepare for mixed native Ingres/X100 queries and disallow 
**	    positioned upd/del on IVW tables.
**	21-apr-2011 (dougi) m1457
**	    Unset MULT_CORR_ATTRS if triggered by column in select list 
**	    under EXISTS.
*/
DB_STATUS
pst_modhdr(
	PSS_SESBLK         *sess_cb,
	PSQ_CB		   *psq_cb,
	i4		   forupdate,
	PST_QTREE	   *header)
{
    DB_STATUS           status = E_DB_OK;
    register PSS_RNGTAB	*usrrange;
    PST_RNGENTRY 	*headrng;
    i4			oldcount;
    PST_RNGENTRY	**oldrangetab;
    DB_TAB_TIMESTAMP	*timestamp;
    register i4	i;
    extern PSF_SERVBLK  *Psf_srvblk;
#ifdef    xDEBUG
    i4		val1;
    i4		val2;
#endif
    i4		err_code;

    /* The query mode */
    header->pst_mode = psq_cb->psq_mode;

    /* Indicate if any aggregates in tree */
    header->pst_agintree = ((sess_cb->pss_stmt_flags & PSS_AGINTREE) != 0);
    /* Indicate if any subselects in tree */
    header->pst_subintree= ((sess_cb->pss_stmt_flags & PSS_SUBINTREE) != 0);

    /*
    ** do not flatten trees which contain SUBSEL node s.t. it is a
    ** descenedant of OR node;
    ** ... the same applies to trees involving ALL ... SELECT
    ** ... the same applies to trees containing multiple correlated
    **     references in a tree rooted in EXIST which is a descendant 
    **     of NOT or a SUBSEL with count() as one of the elements of 
    **	   the target list 
    ** ... the same applies to queries involving singleton subselect(s)
    **	   if we were told at server startup that such queires should 
    **	   not be flattened
    */
    if (sess_cb->pss_flattening_flags & PSS_MULT_CORR_ATTRS)
    {
	/* Double check the MULT_CORR flag - parser isn't selective enough. */
	bool		nocount = TRUE;

	if (!pst_chk_mult_corr(header->pst_qtree, (PST_J_MASK *) NULL,
			(PST_J_MASK *) NULL, &nocount))
	    sess_cb->pss_flattening_flags &= ~PSS_MULT_CORR_ATTRS;
    }

    if (   (sess_cb->pss_flattening_flags &
		(PSS_SUBSEL_IN_OR_TREE | PSS_ALL_IN_TREE | PSS_MULT_CORR_ATTRS))
	|| (   Psf_srvblk->psf_flags & PSF_NOFLATTEN_SINGLETON_SUBSEL
	    && sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
       )
    {
	header->pst_mask1 |= PST_NO_FLATTEN;
    }

    /*
    ** if sess_cb->pss_flattening_flags indicates that the query tree 
    ** contains (subselects involving aggregates in their target lists 
    ** or NOT EXISTS, NOT IN, !=ALL) and correlated attribute 
    ** references OR <expr involving an attrib> !=ALL (<subsel>), set 
    ** PST_CORR_AGGR in pst_mask1
    */
    if (sess_cb->pss_flattening_flags & PSS_CORR_AGGR)
    {
	header->pst_mask1 |= PST_CORR_AGGR;
    }

    /* Implicit update scrollable cursors need to be flagged as KEYSET. */
    if (sess_cb->pss_stmt_flags & PSS_KEYSET)
	header->pst_mask1 |= PST_KEYSET;	/* keyset scrollable cursor */

    /*
    ** We only get here after psy_qrymod() has returned, so use the auxiliary
    ** range table.
    */
    usrrange = &sess_cb->pss_auxrng.pss_rngtab[0];

    /* remember number of entries in the range table */
    oldcount = header->pst_rngvar_count;	/* save old count to avoid
						** overlaying other stge */
    oldrangetab = header->pst_rangetab;		/* & old ptr array addr */
    header->pst_rngvar_count = sess_cb->pss_auxrng.pss_maxrng + 1;
    if (oldcount < header->pst_rngvar_count)
    {					/* allocate/init new ptr array */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY *)*
	    header->pst_rngvar_count, (PTR *)&header->pst_rangetab,
	    &psq_cb->psq_error);
	if (oldcount > 0)
	  MEcopy((PTR)oldrangetab, oldcount*sizeof(PST_RNGENTRY *), 
	    (PTR)header->pst_rangetab);
					/* copy old bits over */
	for (i = oldcount; i < header->pst_rngvar_count; i++)
	    header->pst_rangetab[i] = (PST_RNGENTRY *)NULL;
					/* & clear the rest */
    }

    for (i = 0, usrrange = sess_cb->pss_auxrng.pss_rngtab;
	 i < PST_NUMVARS;
	 i++, usrrange++)
    {
	/*
	** Only build the range table for those range variables that have
	** had range variable numbers assigned to them for this query.
	*/
	if (usrrange->pss_rgno != -1 && usrrange->pss_used)
	{
	    /*
	    ** allocate a range variable entry if it has not been allocated yet
	    */
	    if (header->pst_rangetab[usrrange->pss_rgno] ==
		(PST_RNGENTRY *) NULL)
	    {
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY),
		    (PTR *) header->pst_rangetab + usrrange->pss_rgno,
		    &psq_cb->psq_error);
		if (status != E_DB_OK)
		{
		    return (status);
		}
	    }

	    headrng = header->pst_rangetab[usrrange->pss_rgno];

	    STRUCT_ASSIGN_MACRO(usrrange->pss_tabid, headrng->pst_rngvar);
	    timestamp = &usrrange->pss_tabdesc->tbl_date_modified;
	    STRUCT_ASSIGN_MACRO(*timestamp, headrng->pst_timestamp);
	    headrng->pst_rgtype = usrrange->pss_rgtype;
	    headrng->pst_rgparent = usrrange->pss_rgparent;
	    headrng->pst_rgroot = usrrange->pss_qtree;
	    if (headrng->pst_rgtype == PST_RTREE)
		header->pst_mask1 |= PST_RNGTREE;

	    /*
	    ** copy the range variable name
	    */
	    MEcopy((PTR) usrrange->pss_rgname, DB_TAB_MAXNAME,
		   (PTR) &headrng->pst_corr_name);

	    /* copy the inner and the outer relation maps */
	    STRUCT_ASSIGN_MACRO(usrrange->pss_outer_rel,headrng->pst_outer_rel);
	    STRUCT_ASSIGN_MACRO(usrrange->pss_inner_rel,headrng->pst_inner_rel);
	}
    }
    status = pst_x100_check(sess_cb->pss_auxrng.pss_rngtab, header, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    /* Update mode indicator */
    header->pst_updtmode = forupdate;

    /*
    ** no need to copy sess_cb->pss_restab since it is only used by CREATE TABLE
    ** AS SELECT and RETRIEVE INTO, and you cannot open a cursor for either
    */

    /* The result range variable, if any */
    if (sess_cb->pss_resrng == (PSS_RNGTAB *) NULL)
    {
	header->pst_restab.pst_resvno = -1;
	header->pst_mask1 &= ~PST_X100_TARGET;
    }
    else
    {
	/* copy some data from pss_resrng */
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
	    header->pst_restab.pst_restabid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname,
	    header->pst_restab.pst_resname);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_ownname,
	    header->pst_restab.pst_resown);
	header->pst_restab.pst_resvno = sess_cb->pss_resrng->pss_rgno;
	if (PST_IS_X100_MACRO(sess_cb->pss_resrng))
	    header->pst_mask1 |= PST_X100_TARGET;

	/*
	** all the relevant information has already been copied from the PSF
	** range table entry (in the FOR loop above)
	*/
    }

    /* Copy the update map, if any, to the header. */
    if (sess_cb->pss_crsr != (PSC_CURBLK *) NULL)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_iupdmap, header->pst_updmap);
    }

#ifdef	xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_LONG_QRY_TREE_TRACE, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree after modification:\n\n\n");
	status = pst_prmdump(header->pst_qtree, header,
	    &psq_cb->psq_error, DB_PSF_ID);
    }
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_SHORT_QRY_TREE_TRACE, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree after modification:\n\n\n");
	status = pst_1prmdump(header->pst_qtree, header, &psq_cb->psq_error);
    }
#endif

    return (status);
}


/*{
** Name: pst_cdb_cat - is table an "iidd_*" table on the CDB?
**
** Description:
**      Check if the LDB is the CDB and the local table is prefixed by
**	"iidd_".  If so, return true, otherwise false with the following
**	exception:  there are three CDB "iidd_" tables that return FALSE
**	and these are
**	    iidd_stats, iidd_histograms and iidd_dbconstants
**
** Inputs:
**	rng_tab				Pointer to range table element
**      psq_cb                          Pointer to the user's control block
**	  psq_dfltldb			ptr to CDB descriptor
**
** Outputs:
**
**	Returns:
**	    TRUE		 	CDB table with "iidd_"prefix
**	    FAIL			non-CDB or non-"iidd_" table
**
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	11-jun-92 (barbara)
**	    Extracted out of pst_header for Sybil.
**      12-apr-95 (rudtr01)
**          Fixed bug 68036.  Changed all calls to CMcmpcase() to
**          CMcmpnocase().  Needed for FIPS installations where
**          identifiers are coerced to upper case.  Also tidied
**          the code to conform to the One True Standard.
*/
bool
pst_cdb_cat(
	PSS_RNGTAB	*rng_tab,
	PSQ_CB		*psq_cb)
{

    DD_2LDB_TAB_INFO	*ldb_tab_info =
		  	&rng_tab->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;
    DD_LDB_DESC		*ldb_desc;
    char		*c1 = ldb_tab_info->dd_t1_tab_name;
    char		*c2 = "iidd_";

    for (; (*c2 != EOS && CMcmpnocase(c1,c2) == 0); CMnext(c1), CMnext(c2))
	    ;
	     
    if (*c2 != EOS)
    {
	return (FALSE);
    }

    if (CMcmpnocase(c1, "s") == 0)
	c2 = "stats";
    else if (CMcmpnocase(c1, "h") == 0)
	c2 = "histograms";
    else if (CMcmpnocase(c1, "d") == 0)
	c2 = "dbconstants";
    else
	c2 = (char *) NULL;

    /* if there is a reason to check */
    if (c2 != (char *) NULL)
    {
	for (; (*c2 != EOS && CMcmpnocase(c1,c2) == 0); CMnext(c1), CMnext(c2))
	    ;

	/*
	** if the search was terminated because the search
	** string was over AND there are no more non-blank
	** chars in the name we are checking, this must be
	** either iidd_histograms or iidd_stats or
	** iidd_dbconstants , so return FALSE.
	*/
	if (*c2 == EOS
	    && (c2 - ldb_tab_info->dd_t1_tab_name == sizeof(DD_TAB_NAME)
		|| CMspace(c1)
		)
	    )
	{
	    return (FALSE);
	}
    }

    /* Check site */
    ldb_desc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;

    return (psq_same_site(ldb_desc, psq_cb->psq_dfltldb));
}

/*
** Name: pst_proc_rngentry - copy contents of a range entry in PSF session CB 
**			     into a PST_RNGENTRY
**
** Description:
**	This function will copy contents of a range entry in PSF session CB 
**	into a PST_RNGENTRY and, if appropriate, update various fields in the 
**	query tree header structure
**
** Input:
**	sess_cb			PSF session CB
**	psq_cb			PSF request block
**	header			query tree header
**	usrrange_base		PSF range table array base, or (if idx is
**				-1) the actual entry to copy (ie pss_resrng)
**	idx			Range table index to copy, or -1.
**				If not -1, copy usrrange_base[idx] is the
**				entry to copy to the query tree header.
**	is_iidd			if processing a STAR query, this arg will 
**				contain an address of a variable used to keep 
**				track of whether the query involved a link to a
**				local table with a name starting with iidd_ and
**				which resides on CDB
**
** Output:
**	header
**	    pst_rangetab[usrrange->pss_rgno]
**				will be populated
**	*is_iidd		may be reset to TRUE
**	psq_cb.psq_errror	will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	21-mar-94 (andre)
**	    extracted from pst_header() in the course of fixing bug 58965
**	20-dec-2010 (dougi)
**	    Added post-check for derived tables and when elements so the
**	    pst_rgparent fields of referenced tables can be set correctly.
**	    This was a problem found in IVW but is a generic Ingres issue.
**	31-Dec-2010 (kiria01)
**	    The above change needs to be recursive as it is possible for
**	    rgno to be referenced out of order.
**	11-feb-2011 (dougi)
**	    X100 flags need to be global for mix test to work.
**	8-Mar-2011 (kschendel) m1586
**	    Remove x100 checks, do in one pass after range entries complete.
*/
static DB_STATUS
pst_proc_rngentry(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PST_QTREE	*header,
	PSS_RNGTAB	*usrrange_base,
	i4		idx,
	bool		*is_iidd)
{
    PSS_RNGTAB		*usrrange;
    PST_RNGENTRY	*headrng;
    DB_STATUS		status;

    usrrange = usrrange_base;
    if (idx >= 0)
	usrrange = &usrrange_base[idx];

    /* allocate a range variable entry */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY),
        (PTR *) header->pst_rangetab + usrrange->pss_rgno, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
        return(status);
    }

    headrng = header->pst_rangetab[usrrange->pss_rgno];
    STRUCT_ASSIGN_MACRO(usrrange->pss_tabid, headrng->pst_rngvar);

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
        headrng->pst_dd_obj_type =
    	    usrrange->pss_rdrinfo->rdr_obj_desc->dd_o6_objtype;

        if (!*is_iidd)
        {
            /*
            ** if we have not yet found a reference to a link to a iidd_* table 
            ** residing on CDB, check if this range entry describes one of these
            ** special tables.  Queries against such tables will use the 
            ** privileged association.
            */ 
            *is_iidd = pst_cdb_cat(usrrange, psq_cb); 
        }

    }

    STRUCT_ASSIGN_MACRO(usrrange->pss_tabdesc->tbl_date_modified, 
			headrng->pst_timestamp);
    headrng->pst_rgtype = usrrange->pss_rgtype;
    headrng->pst_rgparent = usrrange->pss_rgparent;
    headrng->pst_rgroot = usrrange->pss_qtree;
    if (headrng->pst_rgtype == PST_RTREE)
	header->pst_mask1 |= PST_RNGTREE;

    /*
    ** copy the range variable name
    */
    MEcopy((PTR) usrrange->pss_rgname, DB_TAB_MAXNAME,
           (PTR) &headrng->pst_corr_name);

    /* copy the inner and the outer relation maps */
    STRUCT_ASSIGN_MACRO(usrrange->pss_outer_rel, headrng->pst_outer_rel);
    STRUCT_ASSIGN_MACRO(usrrange->pss_inner_rel, headrng->pst_inner_rel);

    /* If derived table or when element, set pst_rgparent of tables included
    ** for their use. */
    if (idx >= 0
      && (headrng->pst_rgtype == PST_DRTREE || headrng->pst_rgtype == PST_WETREE))
    {
	PST_QNODE	*rootp = headrng->pst_rgroot;
	i4		i;

	/* Loop over entries in pst_tvrm - then set our rgno in their
	** pst_rgparents. */
	for (i = -1; (i = BTnext(i, (char *)&rootp->pst_sym.pst_value.
		pst_s_root.pst_tvrm, BITS_IN(PST_J_MASK))) >= 0; )
	{
	    /* If tvrm refers to a rangetab that we've not setup yet,
	    ** best do so now out of order. */
	    if (!header->pst_rangetab[i])
	    {
		i4 j;
		for (j = 0; j < PST_NUMVARS; j++)
		{
		    PSS_RNGTAB *rv = usrrange_base + j;
		    if (rv->pss_used && rv->pss_rgno == i)
		    {
			if ((status = pst_proc_rngentry(sess_cb, psq_cb, header,
				usrrange_base, j, is_iidd)))
			    return status;
			break;
		    }
		}
		if (!header->pst_rangetab[i])
		{
		    i4 err_code;
		    psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR,
				    &err_code, &psq_cb->psq_error, 0);
		    return (E_DB_ERROR);
		}
	    }
	    header->pst_rangetab[i]->pst_rgparent = usrrange->pss_rgno;
	}

    }

    return(E_DB_OK);
}

/*{
** Name: pst_singlesite - compare dd_ldb_descs and set pst_stype
**
** Description:
**      Compare this descriptor to others seen and set pst_stype to
**	PST_NSITE if any mismatch.
**
** Inputs:
**	rng_tab				Pointer to range table element
**      stype                           Pointer to i4  holding pst_stype
**	save_ldb_desc			Pointer to ptr to DD_LDB_DESC
**
** Outputs:
**
**	May modify *stype and/or *save_ldb_desc
**
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	19-apr-94 (davebf)
**	    Created.
**	20-jun-94 (davebf)
**	    Enabled gateways
*/
VOID
pst_singlesite(
	PSS_RNGTAB	*rng_tab,
	i4             *stype,
	DD_LDB_DESC	**save_ldb_desc)
{

    DD_2LDB_TAB_INFO	*ldb_tab_info =
		  	&rng_tab->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;
    DD_LDB_DESC		*ldb_desc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;

    if(ldb_desc == (DD_LDB_DESC *)NULL)
	return;

    if(*save_ldb_desc == NULL)
    {
	*save_ldb_desc = ldb_desc;	/* save pointer to first */
	return;
    }

    if(psq_same_site(ldb_desc, *save_ldb_desc))
	return;				/* if equal, leave stype alone */
    else
    {
	*stype = PST_NSITE;		/* else, change to NSITE */
	return;
    }

}
/*{
** Name: pst_chk_xtab_upd_const_only 
**
** Description:
**     	Walk completed query tree, and check if (cross-table
**	update) only applies constant values.
**
**	The Result domain (RSD) nodes start on the LH
**	branch from the ROOT of the tree: at each node,
**	the LH branch will be the next RSD (with a special
**	terminating type at end), and the RH branch will
**	be a node or further subtree indicating the value.
**
**	In order to determine if the xtable update only
**	involves constant values, we must walk down to the
**	leaves of all the value subtrees and check that they
**	are all PST_CONST.
**
**	If we find that the updates only apply constant values
**	to the target, unset the 'cross-table update' flag - 
**	there is no point to doing the checking lower down
**	in DMF. Also, set another flag to indicate to OPF
**	[in opn_halloween()] that a top-level duplicate
**	elimination can be done, reinstating previous
**	behaviour.
**	
** Inputs:
**	header	-	query tree header
** Outputs:
**	none
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	3-july-1997(angusm)
**		created.
*/
static 
void
pst_chk_xtab_upd_const_only( PST_QTREE	*header)
{
    register PST_QNODE	*qtree = header->pst_qtree;
    register PST_QNODE	*qnode;
    bool     notconst = FALSE;

/* PST_UPD_CONST */
    /* 
    outer loop: walk RSD nodes,
    but don't take leftmost
    */

    for (qnode = qtree->pst_left; qnode != (PST_QNODE*) NULL 
	    && qnode->pst_sym.pst_type == PST_RESDOM
	    && qnode->pst_left->pst_sym.pst_type != PST_TREE;
     qnode = qnode->pst_left)
    {
	/* inner loop: check leaves of RH tree at RSD node */
	if (qnode->pst_right != (PST_QNODE  *)NULL)
	{
	    notconst = walk(qnode->pst_right);
	    if (notconst == TRUE)
		break;
	}
    }
/* double negative: notconst FALSE. i.e. update only uses constant values */
    if (notconst == FALSE)
    {
	header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	header->pst_mask1 |= PST_UPD_CONST;
    }
}
/*
**  recursive tree-walk
*/
static
bool
walk(PST_QNODE  *qn1)
{
	bool notconst;

	/* if we've reached leaf, terminate */
	if ((qn1->pst_left == (PST_QNODE*) NULL ) &&
	   (qn1->pst_right == (PST_QNODE*) NULL ) )
	{
		if (qn1->pst_sym.pst_type == PST_VAR)
		    return(TRUE);
		else
		    return(FALSE);
	}
	/* otherwise recurse down the branches */
	if (qn1->pst_left != NULL)
	{
	    notconst = walk(qn1->pst_left);
	    if (notconst == TRUE)
	    	return(notconst);
	}
	if (qn1->pst_right != NULL)
	{
	    notconst = walk(qn1->pst_right);
	    if (notconst == TRUE)
	    	return(notconst);
	}
	return notconst;
}

/*
** Name: pst_x100_check -- Check for X100 (IVW) tables in query
**
** Description:
**
**	This routine takes a quick spin through the range table
**	and sets the X100-query and X100-only flags as needed.  If
**	a mix of X100 and Ingres tables is detected, we issue an error,
**	since that's not allowed at present.
**
**	By scanning the range table array rather than the QTREE header
**	range variable array, we'll exclude the result variable for
**	an INSERT statement, which should not participate.
**
** Inputs:
**	rngbase			PSS_RNGTAB * base of range table array
**	header			PST_QTREE * header to set bits in
**	errblk			Where to put error info
**
** Outputs:
**	Returns E_DB_OK or error status with message issued
**
** History:
**	8-Mar-2011 (kschendel) m1586
**	    Extract code needed in a couple places.
*/

static DB_STATUS
pst_x100_check(PSS_RNGTAB *rngbase, PST_QTREE *header, DB_ERROR *errblk)
{
    bool nonx100;
    i4 err_code;
    PSS_RNGTAB *rng;

    nonx100 = FALSE;
    header->pst_mask1 &= ~(PST_X100_QUERY | PST_X100ONLY_QUERY);
    for (rng = rngbase;
	 rng < &rngbase[PST_NUMVARS];
	 ++rng)
    {
	if (rng->pss_rgno != -1 && rng->pss_used
	   && rng->pss_rgtype == PST_TABLE)
	{
	    if (PST_IS_X100_MACRO(rng))
		header->pst_mask1 |= PST_X100_QUERY;
	    else
		nonx100 = TRUE;
	}
    }
    if (header->pst_mask1 & PST_X100_QUERY)
    {
	if (nonx100)
	{
	    /* Not allowed to mix at present */
	    (void) psf_error(E_PS04F2_X100_MIXED,  0L, PSF_USERERR,
			&err_code, errblk, 0);
	    return(E_DB_ERROR);
	}
	else
	{
	    header->pst_mask1 |= PST_X100ONLY_QUERY;
	}
    }

    return (E_DB_OK);
} /* pst_x100_check */


/*
** Name: pst_chk_mult_corr_map - build map of vars refereenced in subtree
**
** Description:
**
**	This routine descends a parse tree fragment looking for PST_VAR 
**	nodes to update a map of vars referenced in a subtree. It also
**	notes when there are count aggs under SUBSEL nodes for future
**	analysis.
**
** Inputs:
**	nodep		ptr to subtree being examined
**	mmap		ptr to map of vars ref'ed in subtree
**	countp		ptr to flag indicating whether there's a count agg
**			under a SUBSEL in this subtree
**	subselp		ptr to flag indicating whether there's a SUBSEL in
**			this subtree
**
** Outputs:
**	mmap		ptr to possibly updated map of var's in subtree
**	countp		ptr to possibly updated count agg indicator
**	subselp		ptr to possibly updated SUBSEL indicator
**
** History:
**	22-apr-2011 (dougi) m1457
**	    Written.
*/

static void
pst_chk_mult_corr_map(
	PST_QNODE	*nodep,
	PST_J_MASK	*mapp,
	bool		*countp,
	bool		*subselp)

{
    ADI_OP_ID	op;

    /* Simple function that looks for VAR nodes (to update var map), AOPs
    ** (to look for count's) and SUBSELs. */
    if (nodep == (PST_QNODE *) NULL)
	return;

    switch(nodep->pst_sym.pst_type) {
      case PST_VAR:
	BTset(nodep->pst_sym.pst_value.pst_s_var.pst_vno, (char *)mapp);
	return;

      case PST_AOP:
	if ((*subselp) && ((op = nodep->pst_sym.pst_value.pst_s_op.pst_opno)
		== ADI_CNT_OP || op == ADI_CNTAL_OP))
	    (*countp) = TRUE;
	break;

      case PST_SUBSEL:
	(*subselp) = TRUE;
	break;

      case PST_UOP:
	if (((op = nodep->pst_sym.pst_value.pst_s_op.pst_opno) == 
		ADI_EXIST_OP || op == ADI_NXIST_OP) &&
	    nodep->pst_left != (PST_QNODE *) NULL &&
	    nodep->pst_left->pst_sym.pst_type == PST_SUBSEL)
	{
	    /* If this is EXISTS subquery, ignore left side. */
	    pst_chk_mult_corr_map(nodep->pst_left->pst_right, mapp, 
			countp, subselp);
	    return;
	}

      case PST_CONST:
	return;				/* don't recurse */

    }	/* end of switch */

    if (nodep->pst_left != (PST_QNODE *) NULL)
	pst_chk_mult_corr_map(nodep->pst_left, mapp, countp, subselp);
    if (nodep->pst_right != (PST_QNODE *) NULL)
	pst_chk_mult_corr_map(nodep->pst_right, mapp, countp, subselp);

}

/*
** Name: pst_chk_mult_corr_count - look for count agg under SUBSEL
**
** Description:
**	This routine descends a parse tree looking for a COUNT aggregate
**	with nothing else but constants and binary/unary operations. 
**
** Inputs:
**	nodep		ptr to subtree being examined
**	nocount		ptr to flag indicating whether we're under a count agg
**
** Outputs:
**
** Returns:
**
** History:
**	27-apr-2011 (dougi) m1457
**	    Written.
*/
static void 
pst_chk_mult_corr_count(
	PST_QNODE	*nodep,
	bool		*nocount)

{
    ADI_OP_ID	op;

    /* Search left (by iteration) & right (by recursion) subtrees for 
    ** COUNT aggregate and nothing else but BOP/UOP/CONST nodes. */
    for ( ; nodep != (PST_QNODE *) NULL; )
     switch (nodep->pst_sym.pst_type) {
      case PST_CONST:
	return;

      case PST_BOP:
	if (nodep->pst_right != (PST_QNODE *) NULL)
	    pst_chk_mult_corr_count(nodep->pst_right, nocount);
	/* drop through to UOP & iterate on left */

      case PST_UOP:
	nodep = nodep->pst_left;
	break;

      case PST_AOP:
	if ((op = nodep->pst_sym.pst_value.pst_s_op.pst_opno) == ADI_CNT_OP ||
			op == ADI_CNTAL_OP)
	{
	    (*nocount) = FALSE;
	    return;
	}
	/* drop through to default */
	
      default:
	/* Anything else negates any count found. */
	(*nocount) = TRUE;
	return;
     }

}

/*
** Name: pst_chk_mult_corr	- recheck condition for PSS_MULT_CORR_ATTRS
**
** Description:
**
**	This routine descends a parse tree looking for the conditions that
**	result in the PST_MULT_CORR_ATTRS flag being set. Namely, it looks
**	for an "IN/<compop>" subquery with a count agg in the select list
**	that has a correlation predicate to the containing query.
**
**	The check is done because the parser is sloppy and sometimes sets
**	the flag incorrectly (specifically, when the count subquery contains
**	an EXISTS subquery with a column in its select list).
**
** Inputs:
**	nodep		ptr to subtree being examined
**	omap		ptr to map of outer vars
**	imap		ptr to map of inner vars (or NULL)
**	nocount		ptr to flag indicating whether we're under a count agg
**
** Outputs:
**
** Returns:	TRUE if the condition holds, else FALSE
**
** History:
**	21-apr-2011 (dougi) m1457
**	    Written.
*/

static bool 
pst_chk_mult_corr(
	PST_QNODE	*nodep,
	PST_J_MASK	*omap,
	PST_J_MASK	*imap,
	bool		*nocount)

{
    PST_J_MASK	local_omap, local_imap, lmap, rmap, tmap;
    PST_QNODE	*rootp, *subsp, *aghp, *aopp;
    ADI_OP_ID	op;
    bool	lcount, rcount, lsubsel, rsubsel, lo, ro, li, ri;


    /* Descend the tree looking first for a COUNT agg in the SELECT list
    ** of a subquery, then a correlation predicate under the count agg
    ** (correlating what's under the count subquery with what's outside the
    ** subquery). */
    if (nodep == (PST_QNODE *) NULL)
	return(FALSE);


    switch (nodep->pst_sym.pst_type) {
      case PST_ROOT:
	/* Top of parse tree - loop over (possible) union, then for each,
	** copy tvrm & descend rest of parse tree. */
	for (rootp = nodep; rootp != (PST_QNODE *) NULL; 
	    rootp = rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    MEcopy((char *)&rootp->pst_sym.pst_value.pst_s_root.pst_tvrm,
		sizeof(local_omap), (char *)&local_omap);
	    MEfill(sizeof(local_imap), 0, (char *)&local_imap);
	    if (pst_chk_mult_corr(rootp->pst_left, &local_omap, 
			&local_imap, nocount))
		return(TRUE);
	    if (pst_chk_mult_corr(rootp->pst_right, &local_omap, 
			&local_imap, nocount))
		return(TRUE);
	}
	
	/* If we get back here, the answer is FALSE. */
	return(FALSE);

      case PST_SUBSEL:
	/* If we're not under a COUNT, OR pst_tvrm into omap (and remove it
	** on return). */
	if ((*nocount))
	{
	    MEcopy((char *)imap, sizeof(local_imap), (char *)&local_imap);
	    BTor3(PST_NUMVARS, (char *)&nodep->pst_sym.pst_value.pst_s_root.
			pst_tvrm, (char *)omap, (char *)&local_omap);
	}
	else
	{
	    MEcopy((char *)omap, sizeof(local_omap), (char *)&local_omap);
	    BTor3(PST_NUMVARS, (char *)&nodep->pst_sym.pst_value.pst_s_root.
			pst_tvrm, (char *)imap, (char *)&local_imap);
	}

	/* Check subtrees. */
	if (nodep->pst_left != (PST_QNODE *) NULL &&
		pst_chk_mult_corr(nodep->pst_left, &local_omap, &local_imap, 
				nocount))
	    return(TRUE);
	if (nodep->pst_right != (PST_QNODE *) NULL &&
		pst_chk_mult_corr(nodep->pst_right, &local_omap, &local_imap, 
				nocount))
	    return(TRUE);

	/* If we get back here, the answer is FALSE (so far). */
	return(FALSE);

      PST_BOP:
	/* First check for COUNT agg under a <compop> (IN, quantified or
	** singleton - doesn't matter which). */
	lcount = rcount = lsubsel = rsubsel = FALSE;
	if ((*nocount) && nodep->pst_right != (PST_QNODE *) NULL &&
		(subsp = nodep->pst_right)->pst_sym.pst_type == PST_SUBSEL &&
		subsp->pst_left != (PST_QNODE *) NULL &&
		(aghp = subsp->pst_left->pst_right) != (PST_QNODE *) NULL)
	{
	    /* Look for COUNT or COUNT in an otherwise constant expression. */
	    pst_chk_mult_corr_count(aghp, nocount);
	    /* If eligible COUNT is found, descend further, looking for
	    ** correlation. */
	    if (!(*nocount) && pst_chk_mult_corr(aghp, omap, imap, nocount))
		return(TRUE);

	    /* If we get back here, just keep recursing. */
	    (*nocount) = TRUE;
	}
	if (!(*nocount) && nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->
			adi_fitype == ADI_COMPARISON)
	{
	    /* Next code looks for column on one side of BOP from outer
	    ** or inner tables, then a column on other side of BOP from 
	    ** inner or outer tables. That's the condition for a correlation
	    ** predicate. */
	    MEfill(sizeof(lmap), 0, (char *)&lmap);
	    MEfill(sizeof(rmap), 0, (char *)&rmap);
	    pst_chk_mult_corr_map(nodep->pst_left, &lmap, &lcount, &lsubsel);
	    pst_chk_mult_corr_map(nodep->pst_right, &rmap, &rcount, &rsubsel);
	    BTor3(PST_NUMVARS, (char *)&lmap, omap, (char *)&tmap);
	    if (!BTallzeros((char *)&tmap, PST_NUMVARS))
	    {
		BTor3(PST_NUMVARS, (char *)&rmap, imap, (char *)&tmap);
		if (!BTallzeros((char *)&tmap, PST_NUMVARS))
		    return(TRUE);		/* got our condition! */
	    }
	    else
	    {
		BTor3(PST_NUMVARS, (char *)&rmap, omap, (char *)&tmap);
		if (!BTallzeros((char *)&tmap, PST_NUMVARS))
		{
		    BTor3(PST_NUMVARS, (char *)&lmap, imap, (char *)&tmap);
		    if (!BTallzeros((char *)&tmap, PST_NUMVARS))
			return(TRUE);		/* got our condition! */
		}
	    }

	    /* If we get here, just check for COUNT agg and SUBSEL on
	    ** either side (because there was a scalar subquery down there
	    ** somewhere) and check the subtree. */
	    if (lsubsel && lcount && 
		pst_chk_mult_corr(nodep->pst_left, &local_omap, &local_imap, 
				nocount))
		return(TRUE);
	    if (rsubsel && rcount && 
		pst_chk_mult_corr(nodep->pst_right, &local_omap, &local_imap, 
				nocount))
		return(TRUE);
	}
	break;

      case PST_UOP:
	if (((op = nodep->pst_sym.pst_value.pst_s_op.pst_opno) == 
		ADI_EXIST_OP || op == ADI_NXIST_OP) &&
	    nodep->pst_left != (PST_QNODE *) NULL &&
	    nodep->pst_left->pst_sym.pst_type == PST_SUBSEL)
	{
	    /* If this is EXISTS subquery, ignore left side. */
	    if (pst_chk_mult_corr(nodep->pst_left->pst_right, omap, imap,
			nocount))
		return(TRUE);
	    else return(FALSE);
	}
	break;

      case PST_CONST:
      case PST_VAR:
	return(FALSE);			/* cut short - no need to recurse */

    }	/* end of switch */

    /* If we get here, just recurse on subtrees. */
    if (nodep->pst_left != (PST_QNODE *) NULL &&
	pst_chk_mult_corr(nodep->pst_left, omap, imap, nocount))
	return(TRUE);
    if (nodep->pst_right != (PST_QNODE *) NULL &&
	pst_chk_mult_corr(nodep->pst_right, omap, imap, nocount))
	return(TRUE);

    /* If we get here, the answer is FALSE (so far). */
    return(FALSE);
}
