/*
**Copyright (c) 2008 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <pc.h>
#include    <st.h>
#include    <cs.h>
#include    <bt.h>
#include    <me.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>

#include    <rdf.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>
#include    <lk.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <scf.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>
#include    <gca.h>
#include    <x100.h>
#include    <cui.h>

/**
**
**  Name: QEAX100.C	- execute X100 queries.
**
**  Description:
**      X100 algebraic queries are passed to X100 for execution and 
**	batches of result rows are returned to Ingres. Also, DDL queries
**	involving X100 tables are sent here for coordination between 
**	Ingres and X100.
**
**          qea_x100	- pass calls to X100 to return sets of result rows
**	    qea_x100_crtable - coordinate execution of CREATE TABLE for 
**		X100 tables
**	    qea_x100_crconstraint - coordinate execution of primary key,
**		unique and foreign key constraints on X100 tables
**	    qea_x100_drconstraint - coordinate execution of DROP statements 
**		on constraints of X100 tables
**	    qea_x100_crindex - coordinate execution of CREATE INDEX statements 
**		on X100 tables (including cluster definition, et al)
**
**
**  History:
**      28-aug-2008 (dougi)
**	    Written for preliminary X100 support.
**      28-Sep-2009 (gefei01)
**          Support CREATE INDEX on X100 table.
**      05-Oct-2009 (gefei01)
**          qea_x100_crindex() checks existing index.
**	10-Mar-2010 (kiria01)
**	    Pull together parameter blocks for error handling.
**	22-Sep-2010 (kschendel)
**	    Re-type ivw session state ptr, eliminate casts here.
**	    Drop "commit" flag from x100 CB, all done via qet now.
**     28-sep-2010 (hweho01)
**         Add me.h to resolve MEcopy on Windows.
**	28-Oct-2010 (kschendel) SIR 124544
**	    Changes for new dmu characteristics structure.
**      27-jan-2011 (stial01)
**          qea_x100_combineck_cb() lock table for IVW syscall
**      03-feb-2011 (stial01)
**          Change x100_resrow_format i4->i8 (for tuple counts)
**      07-feb-2011 (stial01) (Mantis 1318)
**          qea_x100_update_reltups: don't RDF_INVALIDATE after reltups update
**          Added dmtcb param to qea_x100_update_reltups (open cursor, if any)
**      22-feb-2011 (stial01) (Mantis 1520)
**          qea_create_x100_tbl() Dont allow vectorwise tables in iidbdb
**      3-mar-2011 (stial01)
**          Undo previous change, and allow vectorwise tables in iidbdb
**      18-mar-2011 (stial01) m1595,m1707,m1748
**	    qea_x100_rowcount_cb: don't RDF_INVALIDATE after reltups update
**	25-May-2011 (kschendel) m1260
**	    Core params now a substructure, fix everywhere.
**	    Check for QEF interrupt after error return from X100.
**	21-June-2011 (stial01) m2015
**	    qea_x100_crconstraint() SQL92 case translate ref_key_name
**	    qea_x100_syscall put dbxlate into X100_SYSCALL_PARAM
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/*    Static declarations */

typedef struct _QEA_X100_KEY_ENTRY
{
    DB_ATT_NAME  key_attr_name;
    i4           key_order;
} QEA_X100_KEY_ENTRY;

typedef struct _QEA_X100_CRIX_CB_PARM
{
    QEF_CB   *cls_qef_cb;
    QEU_CB   *cls_qeu_cb;
} QEA_X100_CRIX_CB_PARAM;

static DB_STATUS
qea_x100_crtable_int(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEF_CB		*qef_cb,
QEU_CB		*pqeu_cb,
DB_ERROR	*err_blk);

static DB_STATUS
qea_create_x100_tbl(
	DMU_CB	*dmu,
	QEF_RCB	*qef_rcb,
	QEF_CB	*qef_cb,
	QEF_AHD	*action,
	CS_SID	sessid,
	DB_ERROR *err_blk);

static DB_STATUS
qea_x100_drtable(
QEF_CB		*qef_cb,
QEU_CB		*qeu_cb,
i8		*row_count,
DB_ERROR	*error_block,
i4		caller);

static DB_STATUS
qea_x100_crindex(
QEU_CB		*qeu_cb,
QEF_CB		*qef_cb,
DB_ERROR	*err_blk);

static DB_STATUS
qea_x100_altable(
QEU_CB		*qeu_cb,
QEF_CB		*qef_cb,
DB_ERROR	*err_blk);

static void 
qea_x100_join_index_cb(
    void       *param,
    char       *tablename,
    char       *indexcolumnname,
    char       *fkname);

static void
qea_x100_sort_order_cb(
    void         *param,
    char         *tablename,
    char         *sortordername,
    X100_COLLIST  order_columns,
    int           order_count);

static void
qea_x100_rowcount_cb(
    X100_SYSCALL_PARAM	*param,
    char	table[DB_TAB_MAXNAME],
    char	table_owner[DB_OWN_MAXNAME],
    i8		new_cnt);

static i4
qea_x100_combineck_cb(
    X100_SYSCALL_PARAM	*param,
    char	tabname[DB_TAB_MAXNAME],
    char	table_owner[DB_OWN_MAXNAME]);

static DB_STATUS
qea_x100_getdbname(
	QEF_CB	*qef_cb,
	DB_DB_NAME	*dbname);

static VOID
qea_x100_defconst(
	DMF_ATTR_ENTRY	*attrp,
	char		*defval);

static DB_STATUS qea_x100_params(
	QEF_AHD		*act,
	QEA_X100	*x100cb,
	QEE_DSH		*dsh);


/*{
** Name: QEA_X100	- call X100 to execute X100 query.
**
** Description:
**	Using parameter list in DSH, call X100 to execute query in 
**	X100 algebra and return sets of result rows in SCF-provided
**	buffer arrays. Note that, though output buffers are "chained"
**	using QEF_DATA structures, the actual row buffers are contiguous
**	and form an array as allocated in SCF. This function depends on
**	the fact that the buffers are contiguous.
**
**	If the query plan is cached and parameterized (as indicated by a
**	non-null ahd-x100-params), we'll take the OPF-generated algebra
**	text and insert the actual query parameters, before execution.
**
**	Sort-of-FIXME note:  X100 queries aren't supported in DBP's yet,
**	but if/when they are, it would probably be wise to implement some
**	sort of actual-query-text reuse, in case the query is inside a
**	loop.
**
** Inputs:
**      action				fetch action item
**      qef_rcb                         the qef request block
**	function			reset flags, not used
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0015_NO_MORE_ROWS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-aug-2008 (dougi)
**	    Written for preliminary X100 support.
**      17-mar-2009 (gefei01)
**          Support X100 DELETE.
**	24-Apr-2009 (kiria01)
**	    Extend to include X100 MODIFY
**	25-aug-2009 (dougi)
**	    And finally X100 APPEND (both insert ... select and single row).
**	8-dec-2009 (dougi)
**	    Set x100cb->commit, as needed.
**	6-may-2010 (dougi)
**	    Indicate formal completion of query by X100.
**	12-May-2010 (kiria01) m617 & m615
**	    Don't fixup AHD_X100_DMLOPS status when a warning was returned.
**	    Doing so upsets the callback logic in qeq_query resulting in
**	    disabling multi-statement transactions.
**	30-june-2010 (dougi)
**	    Added code to detect query error in append after CTAS, then 
**	    explicitly drop table from VectorWise.
**	27-sep-2010 (dougi)
**	    Drop some init logic. It's in qee_cract() now.
**	2-Dec-2010 (kschendel)
**	    Add cancel / abort check.  Remove CTAS check, no longer needed
**	    now that Ingres and VW agree on savepoints and such.
**	4-Jan-2011 (kschendel)
**	    Add call to DMF (DMT_GTT_UPD) when doing an updating query
**	    against a global temporary table.
**	17-Mar-2011 (kschendel) m1541
**	    Allow parameterized repeated queries:  do param substitution
**	    into the prototype algebra here, before execution.
*/
DB_STATUS
qea_x100(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
i4		function )
{
    GLOBALREF QEF_S_CB *Qef_s_cb;
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    QEE_DSH	*dsh	= (QEE_DSH *)qef_cb->qef_dsh;
    QEF_DATA	*output;
    QEA_X100	*x100cb = (QEA_X100 *)dsh->dsh_cbs[action->
					qhd_obj.qhd_x100Query.ahd_x100_cb];
    GCA_TUP_DESCR  *resrow_format;
    ULM_RCB      ulm;
    i4 		bufs_used;
    i4          del_cnt = 0;
    DB_STATUS	status = E_DB_OK, locstatus;

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    /* Normally this stuff would be done down in the QP of an Ingres
    ** query.  Since they ain't no such thing when it's a (pure) VW query,
    ** check here -- it might be a callback retrieving a large dataset.
    */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    /* Copy and init. */
    x100cb->core.session = qef_cb->qef_ivwsesscb;
    x100cb->core.adfcb = qef_cb->qef_adf_cb;
    *(x100cb->x100_errorp) = 0;
    if (qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;
    x100cb->x100_text = action->qhd_obj.qhd_x100Query.ahd_x100_text;
    x100cb->x100_textlen = action->qhd_obj.qhd_x100Query.ahd_x100_textlen;
    x100cb->dsh = dsh;
    if (action->qhd_obj.qhd_x100Query.ahd_x100_params != NULL)
    {
	status = qea_x100_params(action, x100cb, dsh);
	if (status != E_DB_OK)
	{
	    qef_rcb->error = dsh->dsh_error = 
				x100cb->core.adfcb->adf_errcb.ad_dberror;
	    return status;
	}
    }

    /* Here is where the call goes: */

    if (status = x100_query(x100cb))
    {
	dsh->dsh_error.err_code = *(x100cb->x100_errorp);
	if (dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS)
	    qef_rcb->error = dsh->dsh_error = 
				x100cb->core.adfcb->adf_errcb.ad_dberror;
    }

    dsh->dsh_qef_rowcount = dsh->dsh_qef_count = 
			*(x100cb->x100_resrow_countp);

    if (status == E_DB_OK
      && action->qhd_obj.qhd_x100Query.ahd_x100_flag & AHD_X100_DMLOPS)
    {
	DMT_CB	*dmt_cb = (DMT_CB *)dsh->dsh_cbs[action->qhd_obj.qhd_x100Query.ahd_dmtix];
	i8	row_delta;

	/* Make sure resrow_count is i8 */
	resrow_format = x100cb->x100_resrow_format;

	if (resrow_format->gca_col_desc[0].gca_attdbv.db_length == 8)
	    row_delta = *(i8 *)(x100cb->x100_rowbuf_addr);
	else
	{
	    /* unexpected resrow_format */
	    TRdisplay("qea_x100() unexpected resrow_format %d %d\n",
		resrow_format->gca_col_desc[0].gca_attdbv.db_length,
		resrow_format->gca_col_desc[0].gca_attdbv.db_datatype);
	    row_delta = *(i4 *)(x100cb->x100_rowbuf_addr);
	}

        /* Save the deleted row count */
        dsh->dsh_qef_rowcount = dsh->dsh_qef_targcount = dsh->dsh_qef_count
                              = row_delta;
	if (action->qhd_obj.qhd_x100Query.ahd_x100_flag & AHD_X100_DELETE)
	    row_delta = -row_delta;

        if (status == E_DB_OK)
            /* For DELETE, a second call to x100_query is
             * needed as a formality to help X100.
             * However, it is called only when the first call
             * went through OK.
             */
           status = x100_query(x100cb);

	if (row_delta != 0 && dmt_cb->dmt_id.db_tab_base < 0)
	{
	    locstatus = dmf_call(DMT_GTT_UPD, dmt_cb);
	}
	/* Update row count for delete, insert. */
	if (action->qhd_obj.qhd_x100Query.ahd_x100_flag & (AHD_X100_DELETE |
				AHD_X100_APPEND))
	    locstatus = qea_x100_update_reltups(qef_rcb, dmt_cb, &dmt_cb->dmt_id, 
				row_delta);
    }

    qef_rcb->qef_rowstat = 0;

    if (status != E_DB_OK || qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
	x100cb->x100_flags |= QEA_X100_QUERYDONE;
					/* indicate X100 is done with q */

    /* If interrupt signalled while waiting for x100, notice it now, and
    ** set the proper QEF-ish error saying "interrupted".
    */
    if (status != E_DB_OK)
	if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	    return (E_DB_ERROR);
#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif
    return (status);
}


/*{
** Name: QEA_X100_DBU	-Perform X100 DBU operation.
**
** Description:
**      Based on the operation code in QEU_CB, call other
**      qea_x100 routines to perform X100 DBU operations.
**      Currently supported operations are DROP TABLE and
**      CREATE INDEX.
**
** Inputs:
**     qef_cb                          QEF session control block
**     qeu_cb                          General utility control block
**
** Outputs:
**     err_blk                         Error block
**
** Returns:
**     E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     28-Sep-2009 (gefei01)
**         Creation.
**	7-july-2010 (dougi)
**	    Add "alter table".
*/
DB_STATUS
qea_x100_dbu(
QEF_CB    *qef_cb,
QEU_CB    *qeu_cb,
DB_ERROR  *err_blk)
{
    DB_STATUS  status = E_DB_OK;

    if (qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;

    switch (qeu_cb->qeu_d_op)
    {
        case DMU_CREATE_TABLE:
            /* perform CREATE TABLE (for global temp tables). */
	    qef_cb->qef_rcb->qef_cb = qef_cb;	/* weird as this looks, we
						** need it down the road */
	    if (qef_cb->qef_rcb->qef_sess_id == 0)
		qef_cb->qef_rcb->qef_sess_id = qeu_cb->qeu_d_id;
	    status = qea_x100_crtable_int((QEF_AHD *) NULL, (QEF_RCB *) NULL,
			qef_cb, qeu_cb, err_blk);
	    break;

        case DMU_DESTROY_TABLE:
            /* perform DROP TABLE */
            status = qea_x100_drtable(qef_cb, qeu_cb, &qeu_cb->qeu_count,
                                      err_blk, ( i4  )QEF_EXTERNAL );
            break;

        case DMU_INDEX_TABLE:
            /* Perform CREATE INDEX */
            status = qea_x100_crindex(qeu_cb, qef_cb, err_blk);
            break;

	case DMU_ALTER_TABLE:
	    /* Perform ALTER TABLE. */
	    status = qea_x100_altable(qeu_cb, qef_cb, err_blk);
	    break;

        default:
            /* Unknown DBU operation. */
            status = E_DB_ERROR;
    }

    return status;
}


/*{
** Name: QEA_X100_CRTABLE - QEF entry point for X100 CREATE TABLE
**
** Description:
**	Perform actions necessary to define an X100 table from Ingres.
**	This involves populating Ingres catalogs, then calling X100
**	so that it may populate its catalogs and setup required structures.
**
** Inputs:
**      action				CREATE TABLE action item
**      qef_rcb                         QEF request block
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-july-2010 (dougi)
**	    Written as external wrapper for X100 CREATE TABLE.
*/
DB_STATUS
qea_x100_crtable(
QEF_AHD		*action,
QEF_RCB		*qef_rcb)

{
    QEU_CB	*qeu_cb = (QEU_CB *)action->qhd_obj.qhd_DDLstatement.ahd_qeuCB;


    /* Just pass it on to function that does the work. This allows for 
    ** separate call for DGTT. */
    return(qea_x100_crtable_int(action, qef_rcb, qef_rcb->qef_cb, qeu_cb,
				&qef_rcb->error));
}


/*{
** Name: QEA_X100_CRTABLE_INT - coordinate execution of CREATE TABLE
**
** Description:
**	Perform actions necessary to define an X100 table from Ingres.
**	This involves populating Ingres catalogs, then calling X100
**	so that it may populate its catalogs and setup required structures.
**	Can be called from qeq_query() (for persistent tables) or qef_call()
**	(for a DBU call for IVW global temp tables).
**
** Inputs:
**      action				CREATE TABLE action item
**      qef_rcb                         QEF request block
**      qef_cb				QEF control block
**	qeu_cb				QEU control block
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-may-2009 (dougi)
**	    Written for unified X100 DDL support.
**	5-june-2010 (dougi)
**	    Copy newly assigned table ID for CTAS from native Ingres.
**	9-july-2010 (dougi)
**	    Add code to allow call with action or with qeu_cb to implement 
**	    DGTT for IVW tables.
*/
static DB_STATUS
qea_x100_crtable_int(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEF_CB		*qef_cb,
QEU_CB		*pqeu_cb,
DB_ERROR	*err_blk)

{
    QEU_CB	*qeu_cb;
    DMU_CB	*dmu_cb;
    DMT_CB	*dmt_cb;
    QEF_VALID	*vl;
    QEE_DSH	*dsh;
    DB_TAB_ID	table_id;
    DB_STATUS	status = E_DB_OK;
    CS_SID	sessid;
    i8		rowcount;
    i4		caller;

    if (qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;

    if (action != (QEF_AHD *) NULL)
	qeu_cb = (QEU_CB *)action->qhd_obj.qhd_DDLstatement.ahd_qeuCB;
    else qeu_cb = pqeu_cb;
    dmu_cb = (DMU_CB *)qeu_cb->qeu_d_cb;

    /* CREATE TABLE of X100 table consists of call to qeu_dbu() to do
    ** Ingres catalog changes, followed by call to X100 to perform X100
    ** changes. */
    caller = (qeu_cb->qeu_d_op ==
	DMU_ALTER_TABLE ? QEF_EXTERNAL : QEF_INTERNAL);
    sessid = (qeu_cb->qeu_d_id) ? qeu_cb->qeu_d_id : qef_rcb->qef_sess_id;

    status = qeu_dbu(qef_cb, qeu_cb, 1, &table_id,
	&rowcount, err_blk, caller);
    if (DB_FAILURE_MACRO(status))
	return(status);

    qeu_cb->qeu_count = -1;		/* inhibit "(0 rows)" message */

    /* Now call X100 for the other half of the operation. */
    status = qea_create_x100_tbl(dmu_cb, qef_rcb, qef_cb,
	(action) ? action->ahd_next : (QEF_AHD *) NULL, sessid, err_blk);

    if (qef_rcb)
    {
	qef_rcb->qef_rowcount = rowcount;
	STRUCT_ASSIGN_MACRO(*err_blk, qef_rcb->error);
    }
    /* Copy table_id to DMT_CB in case this is CTAS. */
    dsh = (QEE_DSH *)qef_cb->qef_dsh;
    if (dsh && (vl = action->qhd_obj.qhd_DDLstatement.ahd_alt))
    {
	dmt_cb = (DMT_CB *) dsh->dsh_cbs[vl->vl_dmf_cb];
	if (dmt_cb)
	    STRUCT_ASSIGN_MACRO(table_id, dmt_cb->dmt_id);
    }

    return(status);
}

/*{
** Name: QEA_X100_DRTABLE - coordinate execution of DROP TABLE
**
** Description:
**	Perform actions necessary to drop an X100 table from Ingres.
**	This involves dropping Ingres catalog definitions, then calling
**	X100 to do likewise.
**
**	Note the special casing for temp tables.  DMF has to be able
**	to drop X100 temp tables anyway, so it does the X100 side work
**	(via a recursive QEF-call to droptt.)
**
** Inputs:
**      qef_rcb                         QEF request block
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-may-2009 (dougi)
**	    Written for unified X100 DDL support.
**	21-oct-2009 (dougi)
**	    Explicitly load dbname for x100 protocols.
**	13-Sep-2010 (hweho01)
**	    Avoid SEGV in x100_drop_table(), setup the database name    
**	    in table param control block.   
**	20-sep-2010 (dougi)
**	    Add support for temp tables.
**	6-Jan-2011 (kschendel)
**	    DMF has to control TT dropping, rearrange code here.
*/
static DB_STATUS
qea_x100_drtable(
QEF_CB		*qef_cb,
QEU_CB		*qeu_cb,
i8		*row_count,
DB_ERROR	*error_block,
i4		caller)

{
    X100_DROP_TABLE_PARAM	x100cb;
    DMU_CB	*dmu_cb = (DMU_CB *)qeu_cb->qeu_d_cb;
    DB_TAB_ID	table_id;
    DB_STATUS	status = E_DB_OK, savestat;
    DB_DB_NAME	dbname;

    /* DROP TABLE of X100 table consists of call to qeu_dbu() to do
    ** Ingres catalog changes, followed by call to X100 to perform X100
    ** changes.  If it's a temp table, though, the DBU call takes
    ** care of everything.  (because DMF needs to be able to drop
    ** X100 temp tables autonomously for rollbacks.)
    */
    status = qeu_dbu(qef_cb, qeu_cb, 0, (DB_TAB_ID *) NULL,
	row_count, error_block, caller);
    if (dmu_cb->dmu_tbl_id.db_tab_base < 0)
	return status;

    savestat = status;

    status = qea_x100_getdbname(qef_cb, &dbname);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Now call X100 for the other half of the operation. */
    x100cb.core.session = qef_cb->qef_ivwsesscb;
    x100cb.core.adfcb = qef_cb->qef_adf_cb;
    x100cb.core.dbname = dbname.db_db_name;
    x100cb.tablename = dmu_cb->dmu_table_name.db_tab_name;

    if (status = x100_drop_table(&x100cb))
    {
	DB_ERROR *er = &x100cb.core.adfcb->adf_errcb.ad_dberror;
	i4 error;
	qef_error(er->err_code, er->err_data, status, &error, error_block, 0);
    }

    return((savestat == E_DB_OK) ? status : savestat);
}

/*
** Name: QEA_X100_DROPTT - Drop X100 side of session temporary table
**
** Description:
**	If an IVW session temp table is being dropped, DMF does an
**	up-facility call here to drop the X100 side.  DMF needs to
**	be able to do this since the drop might be fired by a rollback,
**	and DMF is the only facility that knows exactly when and which
**	tables are dropped by that rollback.
**
**	Note that this might be a recursive QEF call!  Normal drop
**	table of an IVW session temp will call X100-dbu which calls
**	x100-drtable which calls qeu-dbu which calls DMF which calls
**	here. (!!) As noted above, it's done this way because DMF
**	needs to be able to do autonomous IVW TT drops anyway.
**
** Inputs:
**	qef_cb			QEF session control block
**	qef_rcb			QEF request cb, tabname_p filled in
**	error_block		DB_ERROR place to put errors
**
** Outputs:
**	returns E_DB_OK or error with qef_error called
**
** Edit History:
**	6-Jan-2011 (kschendel)
**	    Invent as part of getting temp tables to work like traditional
**	    Ingres temp tables.
*/

DB_STATUS
qea_x100_droptt(QEF_CB *qefcb, QEF_RCB *rcb, DB_ERROR *error_block)
{
    DB_DB_NAME	dbname;
    DB_STATUS	status;
    DB_TAB_NAME	newname;
    X100_DROP_TABLE_PARAM	x100cb;

    status = qea_x100_getdbname(qefcb, &dbname);
    if (DB_FAILURE_MACRO(status))
	return(status);
    x100cb.core.session = qefcb->qef_ivwsesscb;
    x100cb.core.adfcb = qefcb->qef_adf_cb;
    /* session temp - twiddle the name. */
    memcpy(newname.db_tab_name, "Ivw_sess_temp_", sizeof("Ivw_sess_temp_"));
    memcpy(&newname.db_tab_name[14], rcb->qef_tabname_p->db_tab_name,
		DB_TAB_MAXNAME-14);
    x100cb.tablename = newname.db_tab_name;
    x100cb.core.dbname = dbname.db_db_name;

    if ((status = x100_drop_table(&x100cb)) != 0)
    {
	DB_ERROR *er = &x100cb.core.adfcb->adf_errcb.ad_dberror;
	i4 error;
	qef_error(er->err_code, er->err_data, status, &error, error_block, 0);
    }
    return status;

} /* qea_x100_droptt */

/*{
** Name: QEA_X100_CRCONSTRAINT - coordinate execution of constraint definition
**	on X100 tables
**
** Description:
**	Perform actions necessary to define a primary key, unique or foreign
**	key constraint involving X100 tables. Ingres is called to populate
**	its catalogs and return a constraint name, then X100 is called to
**	complete the constraint definition.
**
** Inputs:
**      action				CREATE CONSTRAINT action item
**      qef_rcb                         QEF request block
**	dsh				ptr to DSH
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-may-2009 (dougi)
**	    Written for unified X100 DDL support.
**     28-Sep-2009 (gefei01)
**          Enable x100_create_key.
**	31-mar-2010 (dougi)
**	    Match "close stream" with "open stream".
**	2-Nov-2010 (kschendel)
**	    The constrained-table-attr row points to a DMT_ATT_ENTRY
**	    pointer list, not a DMF_ATTR_ENTRY pointer list.
**	    The code got away with this up until Alison's change to the
**	    DMT_ATT_ENTRY struct...
*/
DB_STATUS
qea_x100_crconstraint(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh)

{
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    X100_CREATE_KEY	x100cb;
    QEF_CREATE_INTEGRITY_STATEMENT	*crintp;
    DMT_ATT_ENTRY	**attrpp;
    ULM_RCB		*ulmp;
    PST_COL_ID		*colidp;
    DB_TAB_NAME		consname;
    DB_STATUS	status = E_DB_OK;
    i4		i, j, error;

    if (qef_rcb->qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_rcb->qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;

    /* To start, we just send this on to qeaddl for execution. Later, 
    ** we'll follow up with a call to X100.
    ** Most of the qea_createIntegrity code executed is common but
    ** there are 2 X100 specifics to note: Firstly, only the 'CLEANUP'
    ** pass will be run through (mostly) and secondly, the ulm stream
    ** remains unclosed and we must see to it that it is closed */

    status = qea_createIntegrity(action, qef_rcb, dsh);

    crintp = &action->qhd_obj.qhd_createIntegrity;
    x100cb.core.session = qef_rcb->qef_cb->qef_ivwsesscb;
    x100cb.core.adfcb = qef_rcb->qef_cb->qef_adf_cb;
    /*x100cb.tablename set later */

    MEcopy(dsh->dsh_row[crintp->qci_internalConstraintName], 
	sizeof(DB_TAB_NAME), (char *)&consname);

    ulmp = (ULM_RCB *)dsh->dsh_row[crintp->qci_ulm_rcb];	

    x100cb.key_count = BTcount((char *)&crintp->qci_integrityTuple->
				dbi_columns, DB_MAX_COLS);
    x100cb.key_name = (char *)&consname.db_tab_name[0];
    if (crintp->qci_integrityTuple->dbi_consflags & CONS_PRIMARY)
	x100cb.key_type = X100_KEY_PRIMARY;
    else if (crintp->qci_integrityTuple->dbi_consflags & CONS_UNIQUE)
	x100cb.key_type = X100_KEY_UNIQUE;
    else x100cb.key_type = X100_KEY_FOREIGN;

    if (x100cb.key_type == X100_KEY_FOREIGN)
    {
	x100cb.ref_table = &crintp->qci_ref_tabname.db_tab_name[0];
	ulmp->ulm_psize = 1+5+2+8+8+1+10;	/* +10 for slop / extra */
	status = qec_malloc(ulmp);
	if (status != E_DB_OK)
	{
	    error = ulmp->ulm_error.err_code;
	    qef_error(error, 0L, status, &error, &dsh->dsh_error, 0 );
	    goto common_exit;
	}
	x100cb.ref_key_name = (char *)ulmp->ulm_pptr;
	STprintf(ulmp->ulm_pptr, "$%5.5s_u%08x%08x",
		crintp->qci_ref_tabname.db_tab_name,
		crintp->qci_ref_depend_consid.db_tab_base,
		crintp->qci_ref_depend_consid.db_tab_index);
	if (*qef_cb->qef_dbxlate & CUI_ID_REG_U)
	    cui_f_idxlate(ulmp->ulm_pptr, STlength(ulmp->ulm_pptr), *qef_cb->qef_dbxlate);
    }
    x100cb.tablename = crintp->qci_cons_tabname.db_tab_name;

	/* Prepare to copy constrained column names. */
    attrpp = (DMT_ATT_ENTRY **)dsh->dsh_row[crintp->
				qci_cnstrnedTabAttributeArray];
    colidp = (PST_COL_ID *)crintp->qci_cons_collist;

    /* Last step is to copy list of constrained column names.
    ** These were retrieved via dmt-show by the createIntegrity action.
    ** Note that dmt-show null-terminates these names, fortunately.
    */
    ulmp->ulm_psize = x100cb.key_count * sizeof(char *);
    status = qec_malloc(ulmp);
    if (status != E_DB_OK)
    {
	error = ulmp->ulm_error.err_code;
	qef_error(error, 0L, status, &error, &dsh->dsh_error, 0 );
	goto common_exit;
    }
    x100cb.columns = (X100_COLLIST) ulmp->ulm_pptr;

    for (i = 0; i < x100cb.key_count; i++, colidp = colidp->next)
	x100cb.columns[i] = attrpp[colidp->col_id.db_att_id]->att_nmstr;
    if ( (status = x100_create_key(&x100cb)) )
	dsh->dsh_error = x100cb.core.adfcb->adf_errcb.ad_dberror;

common_exit:
    /* Release memory stream as promised */
    ulm_closestream(ulmp);
    return status;
}

/*{
** Name: QEA_X100_DRCONSTRAINT - coordinate execution of DROP CONSTRAINT
**	on X100 tables
**
** Description:
**	Perform actions necessary to drop a constraint defined on an X100
**	table. X100 and Ingres are called in turn to delete the definition
**	and structure of the constraint.
**
** Inputs:
**      action				DROP CONSTRAINT action item
**      qef_rcb                         QEF request block
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-may-2009 (dougi)
**	    Written for unified X100 DDL support.
**     28-Sep-2009 (gefei01)
**         Enable x100_drop_key.
*/
DB_STATUS
qea_x100_drconstraint(
QEF_AHD		*action,
QEF_RCB		*qef_rcb)

{
    X100_DROP_KEY	x100cb;
    QEF_DROP_INTEGRITY	*drintp;
    DB_STATUS		status = E_DB_OK;
    i4			error;

    if (qef_rcb->qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_rcb->qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;

    /* Start by doing Ingres stuff. */
    status = qea_d_integ(action, qef_rcb);
    if (status != E_DB_OK)
	return(status);

    /* Now setup X100 stuff and issue X100 call. */
    drintp = &action->qhd_obj.qhd_drop_integrity;
    x100cb.core.session = qef_rcb->qef_cb->qef_ivwsesscb;
    x100cb.core.adfcb = qef_rcb->qef_cb->qef_adf_cb;
    x100cb.tablename = NULL;

    if ( (status = x100_drop_key(&x100cb)) )
    {
	qef_rcb->error = x100cb.core.adfcb->adf_errcb.ad_dberror;
    }
    return status;
}

/*{
** Name: QEA_X100_CRINDEX - coordinate execution of CREATE INDEX on X100
**	tables
**
** Description:
**	Perform actions necessary to execute CREATE INDEX on an X100 table.
**	This is much more involved than traditional CREATE INDEX (e.g. on a 
**	native Ingres table). The X100 CREATE INDEX is responsible for 
**	defining a clustering hierarchy and that requires ordering the columns
**	of potentially numerous tables, as well as building the join index
**	foreign key column of corresponding referential relationships. 
**
**	Calls will be made to X100, followed by calls to Ingres to add
**	join index columns as necessary, and update iirelation and iiattribute
**	catalogs to document the orders of the affected tables.
**
** Inputs:
**      qeu_cb				CREATE INDEX QEU_CB pointer
**      qef_cb                          QEF session control block
**
** Outputs:
**     None.
**
** Returns:
**     E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      6-may-2009 (dougi)
**	    Written for unified X100 DDL support.
**     28-Sep-2009 (gefei01)
**         Implemented CREATE INDEX on X100 table.
**     05-Oct-2009 (gefei01)
**         Check existing index.
**	27-apr-2010 (dougi)
**	    Add support for tp qe83.
**	23-sep-2010 (dougi)
**	    IVW indexes are now recorded in the Ingres catalogs like native
**	    Ingres indexes.
*/
static DB_STATUS
qea_x100_crindex(
QEU_CB		*qeu_cb,
QEF_CB		*qef_cb,
DB_ERROR	*err_blk)

{
    GLOBALREF QEF_S_CB  *Qef_s_cb;
    X100_CLUSTER_INDEX   x100cb;
    DMU_CB              *dmu_cb = (DMU_CB *)qeu_cb->qeu_d_cb;
    DMT_TBL_ENTRY        table;
    DB_TAB_ID		table_id;
    DMT_SHW_CB           dmt_shw_cb;
    ULM_RCB              ulm;
    QEA_X100_KEY_ENTRY **attrpp;
    QEA_X100_CRIX_CB_PARAM cb_param;
    DB_DB_NAME		dbname;
    i4                   col_cnt = dmu_cb->dmu_key_array.ptr_in_count;
    i4                   i, dummy;
    i8			 dummy_rowcount;
    DB_STATUS            status = E_DB_OK;
    i4                   error;

    /* Get the information of the table where the cluster index starts. */
    dmt_shw_cb.length = sizeof(DMT_SHW_CB);
    dmt_shw_cb.type = DMT_SH_CB;
    dmt_shw_cb.dmt_db_id = qeu_cb->qeu_db_id;
    dmt_shw_cb.dmt_session_id = qeu_cb->qeu_d_id;
    dmt_shw_cb.dmt_flags_mask = DMT_M_TABLE;
    dmt_shw_cb.dmt_table.data_address = (char *)&table;
    dmt_shw_cb.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
    dmt_shw_cb.dmt_char_array.data_in_size = 0;
    dmt_shw_cb.dmt_char_array.data_out_size  = 0;
    dmt_shw_cb.dmt_tab_id.db_tab_base = dmu_cb->dmu_tbl_id.db_tab_base;
    dmt_shw_cb.dmt_tab_id.db_tab_index = dmu_cb->dmu_tbl_id.db_tab_index;

    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
    if (status != E_DB_OK)
        return status;

    /* Only one index allowed on the same table. */
    if (table.tbl_index_count > 0)
    {
      qef_error(E_QE0312_X100_INDEX_EXISTS, 0L, E_DB_FATAL, &error,
                err_blk, 1,
                psf_trmwhite(DB_TAB_MAXNAME, table.tbl_name.db_tab_name),
                table.tbl_name.db_tab_name);

      return status; 
    } 

    status = qea_x100_getdbname(qef_cb, &dbname);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Initialize X100 cluster index control block */
    x100cb.core.session = qef_cb->qef_ivwsesscb;
    x100cb.core.dbname = (PTR)&dbname;
    x100cb.core.adfcb = qef_cb->qef_adf_cb;
    x100cb.tablename = table.tbl_name.db_tab_name;
    x100cb.index_name = dmu_cb->dmu_index_name.db_tab_name;
    x100cb.order_count = col_cnt;

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);

    if ((status = qec_mopen(&ulm)) != E_DB_OK)
        return status;

    ulm.ulm_psize = col_cnt * sizeof(char *);

    status = qec_malloc(&ulm);
 
    if (status != E_DB_OK)
      goto exit;

    /* Set up order columns */
    x100cb.order_columns = (X100_COLLIST)ulm.ulm_pptr;

    attrpp = (QEA_X100_KEY_ENTRY **)dmu_cb->dmu_key_array.ptr_address;

    for (i = 0; i < col_cnt; i++)
      x100cb.order_columns[i] = (char *)&attrpp[i]->key_attr_name.db_att_name[0];

    /* Set up X100 cluster index callback */
    x100cb.create_join_index_callback = &qea_x100_join_index_cb;
    x100cb.create_sort_order_callback = &qea_x100_sort_order_cb;

    cb_param.cls_qef_cb = qef_cb;
    cb_param.cls_qeu_cb = qeu_cb;
    x100cb.callback_param = (void *)&cb_param;
    x100cb.enable_joinindex = (i4 *)1;

    /* Call X100 to create the cluster index */
    if (status = x100_cluster_index(&x100cb))
    {
	DB_ERROR *er = &x100cb.core.adfcb->adf_errcb.ad_dberror;
	i4 error;
	qef_error(er->err_code, er->err_data, status, &error, err_blk, 0);
    }
    else
    {
	/* And call DMF to do Ingres catalog definitions, too. */
	status = qeu_dbu(qef_cb, qeu_cb, 1, &table_id,
	    &dummy_rowcount, err_blk, QEF_EXTERNAL);
	if (DB_FAILURE_MACRO(status))
	{
	    i4	error;
	    qef_error(err_blk->err_code, err_blk->err_data, status, 
				&error, err_blk, 0);
	}
    }

exit:
    ulm_closestream(&ulm);

    return status;
}

/*{
** Name: QEA_X100_ALTABLE - coordinate execution of ALTER TABLE on X100
**	tables
**
** Description:
**	Perform actions necessary to execute ALTER TABLE ... ADD/DROP column.
**	This involves passing the operation to X100 for execution, then
**	calling qeu_dbu() to perform any accomapnying Ingres catalog work.
**
** Inputs:
**      qeu_cb				CREATE INDEX QEU_CB pointer
**      qef_cb                          QEF session control block
**
** Outputs:
**     None.
**
** Returns:
**     E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      7-july-2009 (dougi)
**	    Written for unified X100 DDL support.
**	17-sep-2010 (dougi)
**	    Tidy the error handling.
*/
static DB_STATUS
qea_x100_altable(
QEU_CB		*qeu_cb,
QEF_CB		*qef_cb,
DB_ERROR	*err_blk)

{
    GLOBALREF QEF_S_CB  *Qef_s_cb;
    QEF_RCB	*qef_rcb = qef_cb->qef_rcb;
    DMU_CB	*dmu_cb;
    DMF_ATTR_ENTRY *attrp;
    X100_ADD_COLUMN_PARAM  x100ac;
    X100_DROP_COLUMN_PARAM x100dc;
    DB_DB_NAME	dbname;
    DB_STATUS	status;
    i4		i, caller, dummy;

    /* ALTER TABLE of X100 table consists of call to qeu_dbu() to do
    ** Ingres catalog changes, followed by call to X100 to perform X100
    ** changes. */
    caller = (qeu_cb->qeu_d_op ==
	DMU_ALTER_TABLE ? QEF_EXTERNAL : QEF_INTERNAL);

    status = qeu_dbu(qef_cb, qeu_cb, 0, (DB_TAB_ID *) NULL,
	&qef_rcb->qef_rowcount, &qef_rcb->error, caller);

    if (DB_FAILURE_MACRO(status))
    {
	DB_ERROR *er = &qef_rcb->error;
	i4	error;
	qef_error(er->err_code, er->err_data, status, &error, err_blk, 0);
	return(status);
    }

    /* X100 call - setup and do it. */
    dmu_cb = (DMU_CB *)qeu_cb->qeu_d_cb;

    attrp = (DMF_ATTR_ENTRY *)((DMF_ATTR_ENTRY **)dmu_cb->dmu_attr_array.
		ptr_address)[0];

    if (dmu_cb->dmu_action == DMU_ALT_ADDCOL)
    {
	char	defval[100];
	x100ac.core.session = qef_cb->qef_ivwsesscb;
	x100ac.core.adfcb = qef_cb->qef_adf_cb;
	x100ac.core.dbname = NULL;
	x100ac.tablename = dmu_cb->dmu_table_name.db_tab_name;
	x100ac.column_name = (char *)&attrp->attr_name.db_att_name;
	x100ac.attr_entry = attrp;
	x100ac.default_value = &defval[0];
	if (ult_check_macro(&qef_cb->qef_trace, 
				QEF_DISABLE_X100_COMP, &dummy, &dummy))
	    x100ac.enable_compression = 0;
	else x100ac.enable_compression = 1;
	qea_x100_defconst(attrp, x100ac.default_value);

	status = x100_add_column(&x100ac);
    }
    else if (dmu_cb->dmu_action == DMU_ALT_DROPCOL)
    {
	x100dc.core.session = qef_cb->qef_ivwsesscb;
	x100dc.core.adfcb = qef_cb->qef_adf_cb;
	x100ac.core.dbname = NULL;
	x100dc.tablename = dmu_cb->dmu_table_name.db_tab_name;
	x100dc.column_name = (char *)&attrp->attr_name.db_att_name;

	status = x100_drop_column(&x100dc);
    }

    if (status != E_DB_OK)
    {
	DB_ERROR *er = &qef_cb->qef_adf_cb->adf_errcb.ad_dberror;
	i4	error;
	qef_error(er->err_code, er->err_data, status, &error, err_blk, 0);
    }

    return(status);
}

/*{
** Name: QEA_X100_SYSCALL	- Perform IVW "syscall"
**
** Description:
**      This function calls X100 with the "syscall" operator.
**
** Inputs:
**     qef_cb                          QEF session control block
**     qeu_cb                          General utility control block
**
** Outputs:
**     err_blk                         Error block
**
** Returns:
**     E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     8-mar-2010 (dougi)
**         Written.
**     12-mar-2010 (michal)
**         actual x100_syscall() call added
**     14-may-2010 (michal)
**         added update_count callback parameter
**	18-aug-2010 (dougi)
**	    Added combine table verification callback.
*/
DB_STATUS
qea_x100_syscall(
QEF_CB    *qef_cb,
DB_ERROR  *err_blk)
{
    DB_DB_NAME		dbname;
    X100_SYSCALL_PARAM x100cb;
    QEF_RCB	*qef_rcb = qef_cb->qef_rcb;
    DB_STATUS	status = E_DB_OK;

    if (qef_cb->qef_sess_flags & QEF_SP_IN_TXN)
	qef_cb->qef_sess_flags |= QEF_IVW_SINCE_SP;

    status = qea_x100_getdbname(qef_cb, &dbname);
    if (DB_FAILURE_MACRO(status))
	return(status);

    x100cb.callname = qef_rcb->qef_sysccmd;
    x100cb.callparam = qef_rcb->qef_syscargs;
    x100cb.core.session = qef_cb->qef_ivwsesscb;
    x100cb.core.adfcb = qef_cb->qef_adf_cb;
    x100cb.core.dbname = dbname.db_db_name;
    x100cb.cb_qef_rcb = (PTR)qef_rcb;
    x100cb.update_count = qea_x100_rowcount_cb;
    x100cb.table_access = qea_x100_combineck_cb;
    x100cb.dbxlate = *qef_cb->qef_dbxlate;

    status = x100_syscall(&x100cb);
    if (DB_FAILURE_MACRO(status))
    {
	DB_ERROR *er = &x100cb.core.adfcb->adf_errcb.ad_dberror;
	i4	error;
	qef_error(er->err_code, er->err_data, status, &error, err_blk, 0);
	error = E_QE0122_ALREADY_REPORTED;
    }
    return status;
}


/*
** Name: QEA_X100_BEGIN_SESSION - send "begin session" call to X100.
**
** Description:
**      This function calls X100 with "begin session".
**
** Inputs:
**     qef_cb                          QEF session control block
**
** Outputs:
**
** Returns:
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     25-aug-2010 (dougi)
**         Written.
*/
DB_STATUS
qea_x100_begin_session(
QEF_CB    *qef_cb)

{
    X100_SESSION_PARAM	x100sp;
    DB_DB_NAME	dbname;
    DB_STATUS	status;

    status = qea_x100_getdbname(qef_cb, &dbname);
    if (DB_FAILURE_MACRO(status) || dbname.db_db_name[0] == 0 ||
	dbname.db_db_name[0] == ' ')
	return(status);

    /* Fill in session param. */
    x100sp.ing_sessid = qef_cb->qef_ses_id;
    x100sp.core.adfcb = qef_cb->qef_adf_cb;
    x100sp.core.dbname = dbname.db_db_name;
    
    status = x100_session_begin(&x100sp);

    if (status != E_DB_OK)
	return (status);

    qef_cb->qef_ivwsesscb = x100sp.core.session;

    return(status);
}


/*
** Name: QEA_X100_END_SESSION - send "end session" call to X100.
**
** Description:
**      This function calls X100 with "end session".
**
** Inputs:
**     qef_cb                          QEF session control block
**
** Outputs:
**
** Returns:
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     25-aug-2010 (dougi)
**         Written.
**	22-Sep-2010 (kschendel)
**	    Null out state pointer in qef-cb.  "Even paranoids have enemies."
**	1-Sep-2011 (kschendel) b125689
**	    Move nulling out of state pointer to before the session-end call,
**	    even more paranoia.
*/
DB_STATUS
qea_x100_end_session(
QEF_CB    *qef_cb)

{
    DB_STATUS status;
    X100_SESSION_PARAM	x100sp;

    /* Fill in session param. */
    x100sp.core.session = qef_cb->qef_ivwsesscb;
    x100sp.core.adfcb = NULL;
    x100sp.core.dbname = NULL;
    x100sp.ing_sessid = qef_cb->qef_ses_id;

    qef_cb->qef_ivwsesscb = NULL;
    status = x100_session_end(&x100sp);
    return (status);
}


/*
** Name: qea_create_x100_tbl -Physical creation of X100 table.
**
** Description:
**	This routine creates X100 table by calling X100 server.
**      The sanity check and catalog update for CREATE TABLE
**      is done by dmf_call(DMU_CREATE_TABLE) beforehand.
**
** Inputs:
**	dmu			DMU_CB of the DBU action
**	action			ptr to action header (and possible chain
**				of key constraints)
**
** Outputs:
**	Returns:
**
**	E_DB_OK			Not X100 table or X100 table creation successful
**	E_DB_ERROR	        otherwise
**
**
** History:
**      11-Feb-2009 (gefei01)
**          CREATE TABLE for X100.
**	7-may-2009 (dougi)
**	    Moved from qeus.c to qeax100.c.
**	5-nov-2009 (dougi)
**	    Add support for enable_minmax, enable_compression parms.
**	10-feb-2010 (dougi)
**	    Add logic to identify compound columns from compound keys.
**	31-mar-2010 (dougi)
**	    Match "close stream" with "open stream".
**	9-july-2010 (dougi)
**	    Add support for IVW temp tables.
**	15-sep-2010 (dougi)
**	    Add support for "pax" tables (structure = vectorwise_row).
*/
static DB_STATUS
qea_create_x100_tbl(
    DMU_CB	*dmu,
    QEF_RCB	*qef_rcb,
    QEF_CB	*qef_cb,
    QEF_AHD	*action,
    CS_SID	sessid,
    DB_ERROR	*err_blk)
{
    X100_CREATE_TABLE_PARAM   x100ct;
    QEF_CREATE_INTEGRITY_STATEMENT *crintp;
    i4			i, j, k, dummy, ccount;
    i1			*col_vector, recheck;
    ULM_RCB		ulm;
    DB_STATUS		status;
    DB_TAB_NAME		newname;
    DB_DB_NAME		dbname;
    bool		freestream = FALSE;
    i4			error;

    if (dmu->dmu_chars.dmu_struct <= DB_STDING_STORE_MAX)
	/* Not an X100 table. Nothing to do. */
	return E_DB_OK;

    /* get database name */
    status = qea_x100_getdbname(qef_cb, &dbname);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Now we know we are creating an X100 table */

    /* initialize X100_CREATE_TABLE_PARAM */
    x100ct.core.session = qef_cb->qef_ivwsesscb;
    x100ct.core.adfcb = qef_cb->qef_adf_cb;
    x100ct.core.dbname = dbname.db_db_name;
    if (BTtest(DMU_TEMP_TABLE, dmu->dmu_chars.dmu_indicators))
    {
	/* Modify name before sending it to X100. */
	MEcopy("Ivw_sess_temp_", sizeof("Ivw_sess_temp_"), (char *)&newname);
	MEcopy((char *)&dmu->dmu_table_name, sizeof(DB_TAB_NAME)-14, 
	    (char *)&newname.db_tab_name[14]);
	x100ct.tablename = newname.db_tab_name;
	x100ct.temporary_table = 1;
    }
    else 
    {
	x100ct.tablename = dmu->dmu_table_name.db_tab_name;
	x100ct.temporary_table = 0;
    }

    if (dmu->dmu_chars.dmu_struct == DB_X100RW_STORE)
	x100ct.pax = 1;
    else x100ct.pax = 0;
    x100ct.enable_minmax = 1;
    x100ct.enable_compression = 1;
    x100ct.comp_array = (i1 *) NULL;
    if (ult_check_macro(&qef_cb->qef_trace, 
				QEF_DISABLE_X100_MINMAX, &dummy, &dummy))
	x100ct.enable_minmax = 0;
    if (ult_check_macro(&qef_cb->qef_trace, 
				QEF_DISABLE_X100_COMP, &dummy, &dummy))
	x100ct.enable_compression = 0;

    if (dmu->dmu_attr_array.ptr_address &&
        dmu->dmu_attr_array.ptr_size == sizeof(DMF_ATTR_ENTRY))
    {
        x100ct.attr_entry = (DMF_ATTR_ENTRY**) dmu->dmu_attr_array.ptr_address;
        x100ct.attr_count = dmu->dmu_attr_array.ptr_in_count;
    }
    else
        /* must have at least one attribute. Verified by dmu_create()    
         * already, but double check here.
         */
        return E_DB_ERROR;

    if (action)
    {
	/* Check for key constraints and build compound column descriptor. */
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);

	if ((status = qec_mopen(&ulm)) != E_DB_OK)
            return status;

	ulm.ulm_psize = x100ct.attr_count * sizeof(i2);
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	    return(status);
	freestream = TRUE;

	col_vector = x100ct.comp_array = (i1 *)ulm.ulm_pptr;
	for (i = 0; i < x100ct.attr_count; i++)
	    col_vector[i] = 0;

	/* Now loop over actions in search of key constraints. */
	for (j = 1; action; action = action->ahd_next)
	 if (action->ahd_atype == QEA_X100_CRCONSTRAINT &&
	    MEcmp((char *)&action->qhd_obj.qhd_createIntegrity.qci_cons_tabname,
		(char *)x100ct.tablename, sizeof(DB_TAB_NAME)) == 0)
	 {
	    crintp = &action->qhd_obj.qhd_createIntegrity;
	    if ((ccount = BTcount((char *)&crintp->qci_integrityTuple->
			dbi_columns,x100ct.attr_count)) <= 1)
		continue;			/* only compound keys */

	    recheck = 0;
	    for (i = -1; (i = BTnext(i, (char *)&crintp->qci_integrityTuple->
			dbi_columns, x100ct.attr_count)) >= 0; )
	    {
		recheck = col_vector[i-1];
		col_vector[i-1] = j;
	    }
	    if (recheck)
	    {
		/* Overlapping keys - if the overlapped only has one column
		** left - get rid of it. */
		for (i = 0, k = 0; i < x100ct.attr_count; i++)
		 if (col_vector[i] == recheck)
		    k++;

		/* If only 1 left of the recheck setting, change to 0. */
		if (k == 1)
		 for (i = 0; i < x100ct.attr_count; i++)
		  if (col_vector[i] == recheck)
		  {
		    col_vector[i] = 0;
		    break;
		  }
	    }
	    j++;				/* next compound key */
	 }
    }

    if (status = x100_create_table(&x100ct))
    {
	DB_ERROR *er = &x100ct.core.adfcb->adf_errcb.ad_dberror;
	qef_error(er->err_code, er->err_data, status, &error, err_blk, 0);
    }
    if (freestream)
	ulm_closestream(&ulm);
    return status;
}


/*{
** Name: QEA_X100_JOIN_INDEX_CB - X100 CREATE INDEX callback to Ingres
**
** Description:
**      Callback from X100 to Ingres to add join index columns
**      and update the IIREFREL catalog to document the newly added
**      columns for the corrsponding referential relationships.
**
** Inputs:
**      params                          callback parameter
**      tablename                       table to add the join index foreign key
**      indexcolumnname                 foreign key column
**      fkname                          constraint name
**
** Outputs:
**     None.
**
** Returns:
**     None.
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     28-Sep-2009 (gefei01)
**         Implemented X100 join index callback.
**	8-sep-2010 (dougi)
**	    Fiddle refingixcol to tell RDF to fill it in and don't add
**	join index column to Ingres anymore.
*/
static void 
qea_x100_join_index_cb(
    void       *param,
    char       *tablename,
    char       *indexcolumnname,
    char       *fkname)
{
    GLOBALREF QEF_S_CB      *Qef_s_cb;
    QEA_X100_CRIX_CB_PARAM  *cb_param = (QEA_X100_CRIX_CB_PARAM *)param;
    QEF_CB                  *cls_qef_cb = cb_param->cls_qef_cb;    
    QEU_CB                  *cls_qeu_cb = cb_param->cls_qeu_cb;
    QEF_RCB                 *qef_rcb = cls_qef_cb->qef_rcb;
    QEU_CB                   qeu_cb;
    QEU_CB                   read_qeu, *rqeu = &read_qeu;
    QEF_DATA		     qef_data;
    DB_REFREL                IIREFRELtuple;
    QEU_QUAL_PARAMS          qparams;
    DB_STATUS                status;
    i4                       error;
    i4                       len1, len2;

    /* Initialize data structures for join index callback. */

    /* Update IIREFREL */
    
    /* Set up QEU_CB */
    rqeu->qeu_type = QEUCB_CB;
    rqeu->qeu_length = sizeof(QEUCB_CB);
    rqeu->qeu_tab_id.db_tab_base = DM_B_REFREL_TAB_ID;
    rqeu->qeu_tab_id.db_tab_index = DM_I_REFREL_TAB_ID;

    rqeu->qeu_db_id = cls_qeu_cb->qeu_db_id;
    rqeu->qeu_lk_mode = DMT_IX;
    rqeu->qeu_access_mode = DMT_A_WRITE;
    rqeu->qeu_flag = 0;
    rqeu->qeu_mask = 0;

    /* Open IIREFREL for lookup and update */
    status = qeu_open(cls_qef_cb, rqeu);
    if (status != E_DB_OK)
        return;

    rqeu->qeu_tup_length = sizeof(DB_REFREL);

    /* Reset to QEU_NOREP after the first qeu_get */
    rqeu->qeu_getnext = QEU_REPO; 
    rqeu->qeu_count = 1;
    rqeu->qeu_klen = 0;
    rqeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;
    rqeu->qeu_flag = 0;
    rqeu->qeu_input = rqeu->qeu_output = &qef_data;
    qef_data.dt_next = NULL;
    qef_data.dt_size = sizeof(DB_REFREL);
    qef_data.dt_data = (PTR)&IIREFRELtuple; 

    /* Now loop through IIREFREL */
    for (;;)
    {
        rqeu->qeu_qual = 0;
        rqeu->qeu_qual = 0;

        /* Get a row from IIREFREL */
        status = qeu_get(cls_qef_cb, rqeu);
        if (status != E_DB_OK)
        {
            error = rqeu->error.err_code;
            if (error == E_QE0015_NO_MORE_ROWS )
                status = E_DB_OK;
            break;
        }
        else if (rqeu->qeu_getnext == QEU_REPO)
            rqeu->qeu_getnext = QEU_NOREPO;

        /* Compare the constraint name for a matching record */
        len1 = cui_trmwhite(sizeof(DB_CONSTRAINT_NAME),
                            (PTR)IIREFRELtuple.dbrr_relname.db_constraint_name);
        len2 = cui_trmwhite(sizeof(DB_CONSTRAINT_NAME),
                            (PTR)fkname);

        if (len1 == len2 &&
            MEcmp((PTR)IIREFRELtuple.dbrr_relname.db_constraint_name,
                  (PTR)fkname,
                  len1)==0)
        {
            /* Found the record to update */

            /* Set the referencing column no. to 0 to induce RDF to
	    ** fill it in. OPF always sees it as the last column in the table
	    ** and fixing it now wouldn't allow ADD/DROP columns later. */
            IIREFRELtuple.dbrr_refingixcol = 0;
            rqeu->qeu_qual = 0;
            rqeu->qeu_qarg = 0;
            rqeu->qeu_f_qual = 0;
            rqeu->qeu_f_qarg = 0;

            /* Update IIREFREL */
            status = qeu_replace(cls_qef_cb, rqeu);

            /* We are done. */
            break;
        }
    }

    /* Close IIREFREL */
    (void)qeu_close(cls_qef_cb, rqeu);

    return;
}


/*{
** Name: QEA_X100_SORT_ORDER_CB - X100 sort order callback to Ingres
**
** Description:
**      Callback from X100 to Ingres to update core catalogues
**      (IIRELATION and IIATTRIBUTE) to document the orders of the
**      affected tables. The update is performed through the follwoing
**      internal MODIFY command:
**      MODIFY <tablename> to X100IX on <order_columns>
**
** Inputs:
**      params                          callback parameter
**      tablename                       table to be sorted
**      sortordername                   sort order name
**      order_columns                   columns to be sorted on
**      order_count                     number of order_columns
**
** Outputs:
**     None.
**
** Returns:
**     None.
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     28-Sep-2009 (gefei01)
**         Implemented X100 sort order callback.
*/
static void
qea_x100_sort_order_cb(
    void         *param,
    char         *tablename,
    char         *sortordername,
    X100_COLLIST  order_columns,
    int           order_count)
{
    GLOBALREF QEF_S_CB      *Qef_s_cb;
    QEA_X100_CRIX_CB_PARAM  *cb_param = (QEA_X100_CRIX_CB_PARAM *)param;
    QEF_CB                  *cls_qef_cb = cb_param->cls_qef_cb;
    QEU_CB                  *cls_qeu_cb = cb_param->cls_qeu_cb;
    QEF_RCB                 *qef_rcb = cls_qef_cb->qef_rcb;
    QEU_CB                   qeu_cb;
    DMU_CB                   dmu_cb;
    ULM_RCB                  ulm;
    DMT_SHW_CB               dmt_shw_cb;
    DMT_TBL_ENTRY            table;
    DMF_ATTR_ENTRY         **attrs;
    DMU_KEY_ENTRY          **keys;
    DB_TAB_ID                table_id;
    DB_STATUS                status;
    i4                       i;

    /* We don't do anything here anymore. */
    return;

    /* First check for join index column - we don't update iirelation/
    ** iiattribute for them. This check could be more precise were we to
    ** load all the columns with DMT_SHOW, then loop over to assure there
    ** isn't a user-defined column with the same name. */
    if (order_count == 1 && MEcmp((char *)order_columns[0], "_j", 2) == 0)
	return;

    /* Initialize data structures for sort order callback. */
    MEfill(sizeof(QEU_CB), 0, &qeu_cb);
    MEfill(sizeof(DMU_CB), 0, &dmu_cb);
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid = (PTR)NULL;
    ulm.ulm_flags = ULM_PRIVATE_STREAM;

    if ((status = qec_mopen(&ulm)) != E_DB_OK)
      return;

    /* Update IIRELATION and IIATTRIBUTE through the following
     * internal MODIFY command:
     * MODIFY <tablename> TO x100ix ON <order_columns[0]>, ...,
     * <order_columns[n-1];
     * n == order_count
     */

    /* Initialize a QEU_CB to pass to qeu_dbu() */
    qeu_cb.qeu_type     = QEUCB_CB;
    qeu_cb.qeu_length   = sizeof(QEU_CB);
    qeu_cb.qeu_d_op     = DMU_MODIFY_TABLE;
    qeu_cb.qeu_ascii_id = QEUCB_ASCII_ID;
    qeu_cb.qeu_db_id    = cls_qeu_cb->qeu_db_id;
    qeu_cb.qeu_d_id     = cls_qeu_cb->qeu_d_id;
    qeu_cb.qeu_d_cb     = (void *)&dmu_cb;

    /* Initialize a DMU_CB to pass to qeu_dbu() */
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(DMU_CB);
    dmu_cb.dmu_db_id = cls_qeu_cb->qeu_db_id;
    MEcopy((PTR)tablename, sizeof(DB_TAB_NAME),
           (PTR)dmu_cb.dmu_table_name.db_tab_name);
    dmu_cb.ascii_id = DMU_ASCII_ID;

    /* initialize dmu_tab_id and table owner */
    dmt_shw_cb.type = DMT_SH_CB;
    dmt_shw_cb.length = sizeof(DMT_SHW_CB);
    dmt_shw_cb.dmt_char_array.data_in_size = 0;
    dmt_shw_cb.dmt_char_array.data_out_size = 0;
    dmt_shw_cb.dmt_tab_id.db_tab_base =
         dmt_shw_cb.dmt_tab_id.db_tab_index = 0;
    dmt_shw_cb.dmt_flags_mask = DMT_M_TABLE | DMT_M_NAME;
    dmt_shw_cb.dmt_db_id = qeu_cb.qeu_db_id;
    dmt_shw_cb.dmt_session_id = qeu_cb.qeu_d_id;
    dmt_shw_cb.dmt_table.data_address = (PTR) &table;
    dmt_shw_cb.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
    dmt_shw_cb.dmt_char_array.data_address = (PTR)NULL;
    dmt_shw_cb.dmt_char_array.data_in_size = 0;
    dmt_shw_cb.dmt_char_array.data_out_size  = 0;
    STRUCT_ASSIGN_MACRO(((DMU_CB *)cls_qeu_cb->qeu_d_cb)->dmu_owner,
                                   dmt_shw_cb.dmt_owner);
    MEcopy((PTR)tablename, sizeof(DB_TAB_NAME),
           dmt_shw_cb.dmt_name.db_tab_name); 

    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
    if (status != E_DB_OK)
        goto exit;

    dmu_cb.dmu_tbl_id.db_tab_base  = table.tbl_id.db_tab_base;
    dmu_cb.dmu_tbl_id.db_tab_index = table.tbl_id.db_tab_index;
    STRUCT_ASSIGN_MACRO(table.tbl_owner, dmu_cb.dmu_owner);

    /* Initialize table name */
    MEcopy((PTR)tablename, sizeof(DB_TAB_NAME),
           (PTR)dmu_cb.dmu_table_name.db_tab_name);

    ulm.ulm_psize = sizeof(DB_LOC_NAME);
    status = qec_malloc(&ulm);
    if (status != E_DB_OK)
        goto exit;

    /* Initialize dmu_location */
    dmu_cb.dmu_location.data_address = ulm.ulm_pptr;
    dmu_cb.dmu_location.data_in_size = sizeof(DB_LOC_NAME);

    MEcopy((PTR)DB_DEFAULT_NAME, sizeof(DB_DEFAULT_NAME),
           dmu_cb.dmu_location.data_address);

    /* Indicate storage structure in dmu characteristics */
    dmu_cb.dmu_chars.dmu_struct = DB_X100IX_STORE;
    BTset(DMU_STRUCTURE, dmu_cb.dmu_chars.dmu_indicators);
    dmu_cb.dmu_action = DMU_ACT_STORAGE;

    /* Initialize dmu_attr_array */
    dmu_cb.dmu_attr_array.ptr_in_count = order_count;
    dmu_cb.dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);
    ulm.ulm_psize
        = dmu_cb.dmu_attr_array.ptr_in_count * sizeof(DMF_ATTR_ENTRY *);
    status = qec_malloc(&ulm);
    if (status != E_DB_OK)
        goto exit;

    dmu_cb.dmu_attr_array.ptr_address = (PTR)ulm.ulm_pptr;
    attrs = (DMF_ATTR_ENTRY **)dmu_cb.dmu_attr_array.ptr_address;
 
    i=0;
    while ( i < order_count)
    {
      ulm.ulm_psize = sizeof(DMF_ATTR_ENTRY);
      status = qec_malloc(&ulm);
      if (status != E_DB_OK)
        goto exit;
    
      attrs[i] = (DMF_ATTR_ENTRY *)ulm.ulm_pptr;

      MEfill(sizeof(DMF_ATTR_ENTRY), 0, attrs[i]);
      
      MEcopy((PTR)order_columns[i], sizeof(DB_ATT_NAME),
	     (PTR)attrs[i]->attr_name.db_att_name);
      i++;
    }   

    /* Initialize key array */
    dmu_cb.dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
    ulm.ulm_psize = order_count * sizeof(DMU_KEY_ENTRY *);
    status = qec_malloc(&ulm);
    if (status != E_DB_OK)
      goto exit;
    
    dmu_cb.dmu_key_array.ptr_address = (PTR)ulm.ulm_pptr;
    dmu_cb.dmu_key_array.ptr_in_count = order_count;
    keys = (DMU_KEY_ENTRY **)dmu_cb.dmu_key_array.ptr_address;
 
    for (i = 0; i < order_count; i++)
    {
        ulm.ulm_psize = sizeof(DMU_KEY_ENTRY);
        status = qec_malloc(&ulm);
        if (status != E_DB_OK)
            goto exit;

        keys[i] = (DMU_KEY_ENTRY *)ulm.ulm_pptr;

        MEfill(sizeof(DMU_KEY_ENTRY), 0, keys[i]);

        MEcopy((PTR)order_columns[i], sizeof(DB_ATT_NAME),
               (PTR)keys[i]->key_attr_name.db_att_name);

        keys[i]->key_order = DMU_ASCENDING;
    }

    /* Call qeu_dbu() to do the MODIFY */
    status = qeu_dbu(cls_qef_cb, &qeu_cb, 1, &table_id,
                     &qef_rcb->qef_rowcount, &qef_rcb->error,
                     QEF_EXTERNAL);

 exit:
    ulm_closestream(&ulm);

    return;
}


/*{
** Name: QEA_X100_ROWCOUNT_CB - X100 row count update callback to Ingres
**
** Description:
**      Callback function from "call vectorwise" combine function which
**	passes the new row counts of affected tables. This function updates
**	the corresponding iirelation row to show the new row counts.
**
** Inputs:
**      params                          ptr to original syscall parm
**
** Outputs:
**     None.
**
** Returns:
**     None.
**
** Exceptions:
**     none
**
** Side Effects:
**     Updates iirelation rows with new row counts.
**
** History:
**     22-may-2010 (dougi)
**         written.
**	7-sep-2010 (dougi)
**	    Add support for i8 reltups.
**	15-dec-2010 (dougi)
**	    Change to handle new row count replace function.
*/
static void
qea_x100_rowcount_cb(
    X100_SYSCALL_PARAM	*param,
    char		table[DB_TAB_MAXNAME],
    char		table_owner[DB_OWN_MAXNAME],
    i8			new_cnt)

{
    QEF_RCB		*qef_rcb = (QEF_RCB *) param->cb_qef_rcb;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    RDF_CB		rdf_invcb;
    RDR_RB		*rdf_inv_rb = &rdf_invcb.rdf_rb;
    DMU_CB		dmu_cb;
    DMT_SHW_CB		dmt_show;
    DMT_TBL_ENTRY	tbl_info;
    DB_STATUS		status;
    i4			error;
    bool		commitit = FALSE;

    /*
    ** Do a show on the table to get the table ID for subsequent reltups
    ** update.
    */
    dmt_show.type = DMT_SH_CB;
    dmt_show.length = sizeof(DMT_SHW_CB);
    dmt_show.dmt_session_id = qef_rcb->qef_sess_id;
    dmt_show.dmt_db_id = qef_rcb->qef_db_id;
    dmt_show.dmt_char_array.data_in_size = 0;
    dmt_show.dmt_char_array.data_out_size  = 0;
    dmt_show.dmt_tab_id.db_tab_base =
         dmt_show.dmt_tab_id.db_tab_index = 0;
    dmt_show.dmt_flags_mask = (DMT_M_TABLE | DMT_M_NAME);
    dmt_show.dmt_table.data_address = (PTR) &tbl_info;
    dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
    dmt_show.dmt_char_array.data_address = (PTR)NULL;
    dmt_show.dmt_char_array.data_in_size = 0;
    dmt_show.dmt_char_array.data_out_size  = 0;

    /* Locate by table name. */
    MEcopy(table, sizeof(dmt_show.dmt_name.db_tab_name),
			(char *)&dmt_show.dmt_name.db_tab_name);
    MEcopy(table_owner ? table_owner :
			(char *)&qef_rcb->qef_cb->qef_user.db_own_name,
			DB_OWN_MAXNAME,
			(char *)&dmt_show.dmt_owner.db_own_name);

    status = dmf_call(DMT_SHOW, &dmt_show);
    if (DB_FAILURE_MACRO(status))    
    {
	if (dmt_show.error.err_code == E_DM0054_NONEXISTENT_TABLE ||
	    dmt_show.error.err_code == E_DM0114_FILE_NOT_FOUND)
	    qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
	else
	    qef_error(dmt_show.error.err_code, 0L, status, &error,
		&qef_rcb->error, 0);
	return;
    }

    if (((DMT_TBL_ENTRY *)dmt_show.dmt_table.data_address)->tbl_storage_type
			<= DB_STDING_STORE_MAX)
    {
	/* Error - it ain't IVW. */
	return;
    }

    /* Initialize data structures for sort order callback. */
    MEfill(sizeof(DMU_CB), 0, &dmu_cb);

    /* Update IIRELATION and IIATTRIBUTE through the following
    ** internal pseudo-MODIFY command:
    ** MODIFY <tablename> reltups = <n>; */

    /* Initialize a DMU_CB to pass to qeu_dbu() */
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(DMU_CB);
    dmu_cb.dmu_db_id = qef_rcb->qef_db_id;
    dmu_cb.ascii_id = DMU_ASCII_ID;

    STRUCT_ASSIGN_MACRO(tbl_info.tbl_id, dmu_cb.dmu_tbl_id);

    /* Tell modify what it should be doing */
    dmu_cb.dmu_action = DMU_ACT_REP_RELTUPS;
    dmu_cb.dmu_tup_cnt = new_cnt;

    /* do we need a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	qef_rcb->qef_modifier    = QEF_SSTRAN;
	qef_rcb->qef_flag        = 0;
	commitit = TRUE;
	status = qet_begin(qef_cb);
	if (DB_FAILURE_MACRO(status))
	    return;
    }
    dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;

    status = dmf_call(DMU_MODIFY_TABLE, &dmu_cb);
   
    if (commitit)
	status = qet_commit(qef_cb);

    qeu_rdfcb_init((PTR)&rdf_invcb, qef_cb);
    STRUCT_ASSIGN_MACRO(dmt_show.dmt_tab_id, rdf_inv_rb->rdr_tabid);


}

/*{
** Name: QEA_X100_COMBINECK_CB - X100 combine table verification callback
**	to QEF
**
** Description:
**      Callback function from "call vectorwise" combine function which
**	verifies existence of table and necessary user authorizations.
**
** Inputs:
**	param				ptr to original syscall parm
**	tabname				name of table to be verified
**
** Outputs:
**     None.
**
** Returns:
**	0	- if table doesn't exist or if user has auth.
**	-1	- otherwise.
**
** Exceptions:
**     none
**
** History:
**	18-aug-2010 (dougi)
**	    Written.
**	25-jan-2011 (dougi)
**	    Verify tables are VectorWise. Otherwise someone could use non-ingres
**	    table under unauth. userid to bypass check.
*/
static i4
qea_x100_combineck_cb(
    X100_SYSCALL_PARAM	*param,
    char	tabname[DB_TAB_MAXNAME],
    char		table_owner[DB_OWN_MAXNAME])

{
    QEF_RCB		*qef_rcb = (QEF_RCB *) param->cb_qef_rcb;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    RDF_CB		rdfcb;
    RDR_RB		*rdfrb = &rdfcb.rdf_rb;
    RDR_INFO		rdrinfo;
    RDR_INFO		*rdfinfo;
    QEF_DATA		*qp;
    DB_PROTECTION	*ptup;
    i4			tcount;
    DB_STATUS		status;
    i4			error;
    bool		perm_found, app, del, upd, ret;
    DB_TAB_ID		tabid;


    /* Look for table name under table qualifier (if any) or session user. */
    qeu_rdfcb_init((PTR)&rdfcb, qef_cb);
    rdfcb.rdf_info_blk = (RDR_INFO *)NULL;

    MEcopy((char *)&tabname[0], sizeof(DB_TAB_NAME), (char *)&rdfrb->rdr_name);
    MEcopy(table_owner ? table_owner : (char *)&qef_cb->qef_user, sizeof(DB_OWN_NAME), 
					(char *)&rdfrb->rdr_owner);
    rdfrb->rdr_types_mask = RDR_RELATION | RDR_BY_NAME;
    status = rdf_call(RDF_GETDESC, (PTR)&rdfcb);

    if (status != E_DB_OK)
    {
	if (!table_owner && rdfcb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    /* Now try the DBA as qualifier. */
	    MEcopy((char *)qef_cb->qef_dba, sizeof(DB_OWN_NAME), 
					(char *)&rdfrb->rdr_owner);
	    rdfrb->rdr_types_mask = RDR_RELATION | RDR_BY_NAME;
	    status = rdf_call(RDF_GETDESC, (PTR)&rdfcb);
	}

	/* If still an error - give up. */
	if (status != E_DB_OK)
	{
	    /* If table really not found, return -2 so VW can send
	    ** "not found" error. */
	    if (rdfcb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		return(-2);
	    else return(-1);
	}
    }
	    
    rdfinfo = rdfcb.rdf_info_blk;

    /* 1st check is to verify that this is a VectorWise table. Otherwise, 
    ** someone could spoof a VW table with a native Ingres table. */
    if (rdfinfo && rdfinfo->rdr_rel && 
		rdfinfo->rdr_rel->tbl_storage_type <= DB_STDING_STORE_MAX)
	return(-2);			/* if native, return "no table" */

    /* We should now have the table. If table oiwner is session user, we're
    ** ok. Otherwise, proceed to get the corresponding iiprotect tuples. */
    tabid = rdfrb->rdr_tabid;
    if (MEcmp((char *)&qef_cb->qef_user, (char *)&rdfrb->rdr_owner,
		sizeof(DB_OWN_NAME)) == 0)
    {
	status = rdf_call(RDF_UNFIX, &rdfcb);
        status = qea_x100_lock_table(qef_rcb, &tabid);
	if (DB_FAILURE_MACRO(status))
	    return(-1);
	return(0);
    }

    rdfrb->rdr_types_mask = RDR_PROTECT;	/* to get iiprotect tuples */
    rdfrb->rdr_update_op = RDR_OPEN;
    rdfrb->rdr_qrymod_id = 0;
    rdfrb->rdr_qtuple_count = 20;

    perm_found = app = del = upd = ret = FALSE;	/* init flags */

    /* With 20 at a time, look for our guy and the right auths. */
    while((!perm_found) && rdfcb.rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, (PTR)&rdfcb);

	rdfinfo = rdfcb.rdf_info_blk;
	rdfrb->rdr_update_op = RDR_GETNEXT;

	if (status != E_DB_OK)
	{
	    switch(rdfcb.rdf_error.err_code)
	    {
	      case E_RD0011_NO_MORE_ROWS:
		status = E_DB_OK;
		break;

	      case E_RD0013_NO_TUPLE_FOUND:
		status = E_DB_OK;
		continue;

	      default:
		break;
	    }
	}

	/* Scan the tuples. */
	for (qp = rdfinfo->rdr_ptuples->qrym_data, tcount = 0;
	    (!perm_found) && tcount < rdfinfo->rdr_ptuples->qrym_cnt; 
	    qp = qp->dt_next, tcount++)
	{
	    ptup = (DB_PROTECTION *)qp->dt_data;

	    if (ptup->dbp_gtype == DBGR_PUBLIC ||
		(ptup->dbp_gtype == DBGR_USER && 
		    MEcmp((char *)&qef_cb->qef_user, (char *)&ptup->dbp_owner,
		    sizeof(DB_OWN_NAME)) == 0) ||
		(ptup->dbp_gtype == DBGR_GROUP && 
		    MEcmp((char *)qef_cb->qef_group, (char *)&ptup->dbp_owner, 
		    sizeof(DB_OWN_NAME)) == 0) ||
		(ptup->dbp_gtype == DBGR_APLID && 
		    MEcmp((char *)qef_cb->qef_aplid, (char *)&ptup->dbp_owner, 
		    sizeof(DB_OWN_NAME)) == 0))
	    {
		if (ptup->dbp_popset & DB_RETRIEVE)
		    ret = TRUE;
		if (ptup->dbp_popset & DB_REPLACE)
		    upd = TRUE;
		if (ptup->dbp_popset & DB_APPEND)
		    app = TRUE;
		if (ptup->dbp_popset & DB_DELETE)
		    del = TRUE;
	    }
	    perm_found = ret && upd && app && del;
	}
    }

    /* Close iiprotect, if it has been opened. */
    if (rdfrb->rdr_rec_access_id != NULL)
    {
	rdfrb->rdr_update_op = RDR_CLOSE;
	status = rdf_call(RDF_READTUPLES, &rdfcb);
    }

    status = rdf_call(RDF_UNFIX, &rdfcb);

    if (perm_found)
    {
        status = qea_x100_lock_table(qef_rcb, &tabid);
	if (DB_FAILURE_MACRO(status))
	    return(-1);
	return(0);
    }
    else return(-1);

}

/*{
** Name: QEA_X100_GETDBNAME	- get name of current database
**
** Description:
**      This function calls SCF to get the current database name as
**	needed by X100. Most X100 calls have dbname available, but a couple
**	(create index, create clustered index) don't.
**
** Inputs:
**	dbname			ptr to were dbname is returned
**
** Outputs:
**     None.
**
** Returns:
**     None.
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     30-oct-2009 (dougi)
**         Written.
**     15-Oct-2010 (hweho01)
**         Avoid getting garbage as the return status in caller routine
**         on Windows, need to return E_DB_OK if there is no error.
*/
static DB_STATUS
qea_x100_getdbname(
	QEF_CB	*qef_cb,
	DB_DB_NAME	*dbname)

{
    SCF_CB              scf_cb;
    SCF_SCI             sci_list[1];

    DB_STATUS		status;
    i4			error;

    /*
    ** This is annoying, but since this call originates in psy_kview()
    ** and not scs_sequencer(), we must explicitly look for the dbname.
    */
    scf_cb.scf_length       = sizeof (SCF_CB);
    scf_cb.scf_type	    = SCF_CB_TYPE;
    scf_cb.scf_facility     = DB_QEF_ID;
    scf_cb.scf_session      = qef_cb->qef_ses_id;
    scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_code    = SCI_DBNAME;
    sci_list[0].sci_length  = sizeof(*dbname);
    sci_list[0].sci_aresult = (char *) dbname;
    sci_list[0].sci_rlength = NULL;

    status = scf_call(SCU_INFORMATION, &scf_cb);
    if (status != E_DB_OK)
    {
	error = E_QE022F_SCU_INFO_ERROR;
	status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	return(status);
    }
    else	
       return( E_DB_OK ); 

}

/*{
** Name: QEA_X100_DEFCONST	- generate X100 syntax for constant
**
** Description:
**      This function generates X100 syntax to define the default value
**	of an added column as required by the X100 interface.
**
** Inputs:
**	attrp			ptr to descriptor of column being added
**	defval			ptr to string into which syntax is built
**
** Outputs:
**     None.
**
** Returns:
**     None.
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**     15-july-2010 (dougi)
**         Written.
*/
static VOID
qea_x100_defconst(
	DMF_ATTR_ENTRY	*attrp,
	char		*defval)

{
    DB_DT_ID	dtype = abs(attrp->attr_type);

    /* Just switch on type and generate the strings - NULL or 0 for now. */
    if (attrp->attr_defaultID.db_tab_base == DB_DEF_NOT_SPECIFIED ||
	attrp->attr_defaultID.db_tab_base == DB_DEF_ID_NULL)
	switch (dtype) {
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_NCHR_TYPE:
	  case DB_NVCHR_TYPE:
	  case DB_BYTE_TYPE:
	  case DB_VBYTE_TYPE:
	    STprintf(defval, " null(str(' '))");
	    break;
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_DEC_TYPE:
	    STprintf(defval, " null(sint('0'))");
	    break;
	  case DB_ADTE_TYPE:
	  case DB_DTE_TYPE:
	    STprintf(defval, " null(date('0'))");
	    break;
	  case DB_INYM_TYPE:
	    STprintf(defval, " null(intervalym('0'))");
	    break;
	  case DB_INDS_TYPE:
	    STprintf(defval, " null(intervalds('0'))");
	    break;
	  default:
	    STprintf(defval, " null(uchr('0'))");
	    break;
	}
    else switch (dtype) {
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_NCHR_TYPE:
	  case DB_NVCHR_TYPE:
	  case DB_BYTE_TYPE:
	  case DB_VBYTE_TYPE:
	    STprintf(defval, " str(' ')");
	    break;
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_DEC_TYPE:
	    STprintf(defval, " sint('0')");
	    break;
	  case DB_ADTE_TYPE:
	  case DB_DTE_TYPE:
	    STprintf(defval, " date('0')");
	    break;
	  case DB_INYM_TYPE:
	    STprintf(defval, " intervalym('0')");
	    break;
	  case DB_INDS_TYPE:
	    STprintf(defval, " intervalds('0')");
	    break;
	  default:
	    STprintf(defval, " uchr('0')");
	    break;
	}


}

/*{
** Name: update_reltups	- update reltups for X100 tables after CTAS/ins. into
**
** Description:
**	For X100 CTAS & insert ... select, update catalog row count.
**
** Inputs:
**	qef_rcb				QEF control block for current query.
**	dmr_cb				control block used by the last dmf_call.
**	row_delta			i4 count of added rows
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**      13-dec-2010 (dougi)
**          Created.
*/
DB_STATUS
qea_x100_update_reltups(
QEF_RCB *qef_rcb,
DMT_CB	  *dmt_cb,
DB_TAB_ID *tabid,
i8 row_delta)
{
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    RDF_CB	rdf_invcb;
    RDR_RB	*rdf_inv_rb = &rdf_invcb.rdf_rb;
    DMU_CB	dmu_cb;
    DB_STATUS	status;


    if (tabid->db_tab_base < 0)
	return(E_DB_OK);		/* don't try for temp tables */

    MEfill(sizeof(DMU_CB), 0, &dmu_cb);

    /* Update IIRELATION and IIATTRIBUTE through the following
    ** internal pseudo-MODIFY command:
    ** MODIFY <tablename> reltups += <n>; */

    /* Initialize a DMU_CB to pass to dmu_modify(). */
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(DMU_CB);
    dmu_cb.dmu_db_id = qef_rcb->qef_db_id;
    dmu_cb.ascii_id = DMU_ASCII_ID;

    /* Tell MODIFY what we want it to do */
    dmu_cb.dmu_action = DMU_ACT_UPD_RELTUPS;
    dmu_cb.dmu_tup_cnt = row_delta;
    dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;
    STRUCT_ASSIGN_MACRO(*tabid, dmu_cb.dmu_tbl_id);

    /* If we have a cursor open on dmu_tbl_id, pass the dmt_record_access_id */
    if (dmt_cb)
	dmu_cb.dmu_record_access_id = dmt_cb->dmt_record_access_id;
    else
	dmu_cb.dmu_record_access_id = NULL;

    status = dmf_call(DMU_MODIFY_TABLE, &dmu_cb);
    if (status != E_DB_OK)
	return (status);
}


/*{
** Name: qea_x100_lock_table - Get an intent table lock on an x100 table
**
** Description:
**      Get intent table lock on an x100 table (to block concurrent DDL)
**      Open a cursor on table, this will do the lock request
**
** Inputs:
**	qef_rcb				QEF control block for current query.
**      tabid                           x100 table id
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**      27-Jan-2011 (stial01)
**          Created.
*/
DB_STATUS
qea_x100_lock_table(
QEF_RCB			*qef_rcb,
DB_TAB_ID		*tabid)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    DB_STATUS		status;
    DMT_CB		dmtcb;
    i4			error_code;

    /* Return if session temp */
    if (tabid->db_tab_base < 0)
	return (E_DB_OK);

    /* We should already be in transaction when this routine is called */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	qef_rcb->qef_modifier    = QEF_MSTRAN;
	qef_rcb->qef_flag        = 0;
	status = qet_begin(qef_cb);
	if (DB_FAILURE_MACRO(status))
	    return;
    }
	    
    /*
    ** Open table to get an intent table lock to block concurrent DDL 
    ** Close immediately so we dont have to keep track of open cursors.
    */
    dmtcb.type         = DMT_TABLE_CB;
    dmtcb.length       = sizeof(DMT_CB);
    dmtcb.dmt_db_id = qef_rcb->qef_db_id;
    dmtcb.dmt_tran_id = qef_cb->qef_dmt_id;
    dmtcb.dmt_access_mode = DMT_A_READ;
    dmtcb.dmt_lock_mode = DMT_IS;
    dmtcb.dmt_flags_mask = 0;
    dmtcb.dmt_id = *tabid;
    dmtcb.dmt_sequence = 0;
    dmtcb.dmt_update_mode = DMT_U_DIRECT;
    dmtcb.dmt_char_array.data_address = 0;
    dmtcb.dmt_char_array.data_in_size = 0;

    /*
    ** DMT_OPEN/DMT_CLOSE a cursor on the vw table (to get a lock)
    */
    status = dmf_call(DMT_OPEN, &dmtcb);

    /* Got the table lock, close the cursor */
    if (status == E_DB_OK)
	status = dmf_call(DMT_CLOSE, &dmtcb);

    if (DB_FAILURE_MACRO(status))
    {
	qef_error(dmtcb.error.err_code, 0L, status, 
		&error_code, &qef_rcb->error, 0);
    }

    return (status);
}

/*
** Name: qea_x100_params -- Insert actual-parameters into X100 query text
**
** Description:
**	Handling repeated parameterized queries is actually quite a bit
**	easier for X100 actions than for regular Ingres actions.  All
**	we need to do is insert the actual-values into the X100 algebra
**	text, unlike an Ingres action where there is all sorts of DSH row
**	and pointer fiddling, and variable length string handling, etc.
**	It's a lot easier to just print text into another text stream.
**
**	The new algebra text is created in the DSH qstream, which is the
**	ordinary DSH stream for non-cached queries (which probably shouldn't
**	get here anyway), but is the runtime query specific stream for
**	repeated queries.  It may be necessary to open the qstream.
**	If the action is inside a loop (which can't happen as of this
**	writing, March 2011), each execution will generate a new copy of
**	the text -- this should probably be fixed to reuse old text areas,
**	when the time comes.
**
**	The resulting query text is stored into the runtime x100-cb
**	passed by the caller.
**
**	Keep in mind throughout that the param_no in the insert-point
**	array is one-origin (as it's derived from PST_CONST's pst_parm_no),
**	as is the data pointer in the dsh_param array;  but the qp_params
**	array is zero origin.
**
** Inputs:
**	action			The action header (should be type QEA_X100_QRY)
**	x100cb			Runtime X100 control-block
**
** Outputs:
**	Returns E_DB_OK or error status with adfcb error info filled in;
**	x100cb text pointers set to new text.
**
** History:
**	17-Mar-2011 (kschendel) m1541
**	    Written for repeated parameterized queries.
*/

/* First, a couple helper routines to copy text to the output.
** If the text doesn't fit, the output area is reallocated to be larger.
** qea_100_move moves a specific number of bytes, qea_x100_movestr
** moves a null-terminated string.
*/
static DB_STATUS
qea_x100_move(QEA_X100 *x100cb, const char *str, i4 len)
{
    /* See if text fits in what's left of the new-text area.  If not,
    ** allocate a new larger one.
    */
    if (x100cb->x100_textleft < len)
    {
	char *new;
	char *old = x100cb->x100_text;
	i4 newlen = x100cb->x100_textlen - x100cb->x100_textleft + len + 1000;
	DB_STATUS status;
	ULM_RCB ulm;

	ulm = Qef_s_cb->qef_d_ulmcb;
	ulm.ulm_streamid = x100cb->dsh->dsh_qstreamid;
	ulm.ulm_psize = newlen;
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	{
	    /* Caller will pick up error from adfcb */
	    x100cb->core.adfcb->adf_errcb.ad_dberror = ulm.ulm_error;
	    return E_DB_ERROR;
	}
	new = x100cb->x100_text = (char *) ulm.ulm_pptr;
	memcpy(new, old, x100cb->x100_textlen - x100cb->x100_textleft);
	x100cb->x100_textleft = newlen - (x100cb->x100_textlen - x100cb->x100_textleft);
	x100cb->x100_textlen = newlen;
    }
    /* Move in the string, adjust counter */
    memcpy(x100cb->x100_text + (x100cb->x100_textlen - x100cb->x100_textleft),
		str, len);
    x100cb->x100_textleft -= len;
    return E_DB_OK;
}

/* adt routine expects void, if memory expansion fails, we'll fall over! */
static void
qea_x100_movestr(void *x100cb, const char *str)
{
    (void) qea_x100_move((QEA_X100 *)x100cb, str, strlen(str));
    return;
}

static DB_STATUS
qea_x100_params(QEF_AHD *act, QEA_X100 *x100cb, QEE_DSH *dsh)
{
    char *origbase;			/* Start of unsubstituted query text */
    char *p;
    DB_DATA_VALUE dv, *dvp;
    DB_STATUS status;
    i4 datatype;
    i4 extra;
    i4 origlen;				/* Unsubstituted query length */
    QEF_QP_CB *qp = dsh->dsh_qp_ptr;
    QEF_X100_PARAM_INSERT *ins;		/* An insert-here entry */
    QEF_X100_QRY *x100act;		/* Less typing */
    ULM_RCB ulm;			/* ULM request block */

    x100act = &act->qhd_obj.qhd_x100Query;
    origbase = x100act->ahd_x100_text;
    origlen = x100act->ahd_x100_textlen;

    /* First, make a quick pass thru the substitution-point array and
    ** make an estimate of how large the substituted text will be.  The
    ** area can expand, but it's better if we get it right the first time.
    */
    extra = 0;
    for (ins = &x100act->ahd_x100_params[0];
	 ins < &x100act->ahd_x100_params[x100act->ahd_x100_numparams];
	 ++ins)
    {
	/* All we need is a SWAG here, don't need exactness */
	dvp = qp->qp_params[ins->param_no-1];
	datatype = abs(dvp->db_datatype);
	switch (datatype)
	{
	case DB_INT_TYPE:
	    extra += 15;
	    break;
	case DB_FLT_TYPE:
	    extra += 25;
	    break;
	case DB_DEC_TYPE:
	    extra += dvp->db_length*2 + 10;
	    break;
	case DB_DTE_TYPE:
	case DB_ADTE_TYPE:
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	case DB_INDS_TYPE:
	case DB_INYM_TYPE:
	    extra += 25;
	    break;
	case DB_CHA_TYPE:
	case DB_CHR_TYPE:
	case DB_VCH_TYPE:
	case DB_TXT_TYPE:
	case DB_BYTE_TYPE:
	case DB_VBYTE_TYPE:
	case DB_LTXT_TYPE:
	    extra += dvp->db_length + 10;
	    break;
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	    extra += dvp->db_length/2 + 10;
	    break;
	default:
	    extra += 10;
	    break;
	}
    }
    extra += 100;	/* Better to waste a few bytes than reallocate */

    /* Allocate initial try at new text area, also opening dsh qstream if
    ** it's not open yet.   Note, qec_mopen indirectly zeros ulm_psize... (!)
    */
    ulm = Qef_s_cb->qef_d_ulmcb;
    x100cb->x100_textlen = x100cb->x100_textleft =
		ulm.ulm_psize = origlen + extra;
    ulm.ulm_streamid = dsh->dsh_qstreamid;
    if (ulm.ulm_streamid == NULL)
    {
	/* Open the execution-specific stream if not open yet */
	ulm.ulm_flags = ULM_OPEN_AND_PALLOC;
	status = qec_mopen(&ulm);	/* Open a PRIVATE stream */
	if (status == E_DB_OK)
	    dsh->dsh_qstreamid = ulm.ulm_streamid;
    }
    else
    {
	status = qec_malloc(&ulm);
    }
    if (status != E_DB_OK)
    {
	/* Caller will pick up error from adfcb */
	x100cb->core.adfcb->adf_errcb.ad_dberror = ulm.ulm_error;
	return (E_DB_ERROR);
    }
    x100cb->x100_text = (char *) ulm.ulm_pptr;

    /* Now loop through the insert-points and stick in the parameter values */

    p = origbase;
    for (ins = &x100act->ahd_x100_params[0];
	 ins < &x100act->ahd_x100_params[x100act->ahd_x100_numparams];
	 ++ins)
    {
	/* Copy text to next insert-point */
	if (ins->offset > (p-origbase))
	{
	    status = qea_x100_move(x100cb, p, ins->offset - (p-origbase) + 1);
	    if (status != E_DB_OK)
		return (status);
	    p = origbase + ins->offset;
	}
	/* Prepare db-data-value pointing at actual data.  If it's a counted-
	** string type, fiddle the DBV to reflect the string value length
	** rather than the original length.  These types are VLUP's, variable
	** length user parameters.  Fortunately they are much simpler to
	** handle when just printing the value as opposed to wedging into
	** a compiled expression!
	** The length is set to one more than the count would indicate, in
	** case the value is nullable.  (If it isn't nullable, no harm is
	** done.)  The sequencer arranges for the null indicator to be at
	** the end of the string value.  (see adg-vlup-setnull.)
	*/
	dv = *qp->qp_params[ins->param_no-1];
	dv.db_data = dsh->dsh_param[ins->param_no];
	datatype = abs(dv.db_datatype);
	if (datatype == DB_VCH_TYPE
	  || datatype == DB_TXT_TYPE
	  || datatype == DB_VBYTE_TYPE
	  || datatype == DB_LTXT_TYPE)
	{
	    dv.db_length = ((DB_TEXT_STRING *)dv.db_data)->db_t_count + DB_CNTSIZE + 1;
	}
	else if (datatype == DB_NVCHR_TYPE)
	{
	    DB_NVCHR_STRING *s = (DB_NVCHR_STRING *) dv.db_data;
	    u_i2 len = s->count;	/* ucs2's, not bytes */
	    dv.db_length = len*sizeof(UCS2) + DB_CNTSIZE + 1;
	}

	status = adt_x100_print(qea_x100_movestr, x100cb, x100cb->core.adfcb,
			&dv, ins->nulltype);
	if (status != E_DB_OK)
	    return (status);

	/* Advance past insert point */
	++p;
    }

    /* Output anything that might be left, plus trailing null */
    status = qea_x100_move(x100cb, p, origlen - (p-origbase) + 1);

    /* Note, textlen is "too large" at this point, but nothing else uses it.
    ** Just leave it alone for the present.
    */
    return status;

} /* qea_x100_params */
