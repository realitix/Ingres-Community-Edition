/*
**Copyright (c) 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <sl.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSYX100.C	- Functions to interface to X100 for IVW
**
**  Description:
**      This file contains the functions that call directly from PSF to the
**	IVW X100 interface
**
**          psy_syscall - passes IVW syscall to QEF to be passed on to the X100
**	interface.
**
**
**  History:    
**      8-mar-2010 (dougi)
**	    Written to support IVW "syscall" statement.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/*{
** Name: psy_syscall	- send IVW "syscall" to QEF for execution
**
** Description:
**	IVW syscall command is passed, along with optional argument 
**	string, to QEF function that interfaces with X100.
**
** Inputs:
**      psy_cb			Ptr to PSY_CB containing IVWCALL particulars
**	sess_cb			Ptr to PSF session control block.
**
** Outputs:
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-mar-2010 (dougi)
**	    Written to support IVW "syscall" statement.
**	24-may-2011 (stial01) m1627
**	    Check for any open cursors on x100 tables
*/

DB_STATUS
psy_syscall(
	PSY_CB		*psy_cb,
	PSS_SESBLK	*sess_cb)
{
    DB_STATUS		status, locstat;
    QEF_RCB		qef_rcb;
    i4			err_code;
    DB_ERROR		*err_blk;
    
    err_blk = &psy_cb->psy_error;
    qef_rcb.qef_length	= sizeof(QEF_RCB);
    qef_rcb.qef_type	= QEFRCB_CB;
    qef_rcb.qef_ascii_id	= QEFRCB_ASCII_ID;
    qef_rcb.qef_sess_id	= sess_cb->pss_sessid;
    qef_rcb.qef_db_id	= sess_cb->pss_dbid;
    qef_rcb.qef_eflag	= QEF_EXTERNAL;
    qef_rcb.qef_sysccmd = psy_cb->psy_sysccmd;
    qef_rcb.qef_syscargs = psy_cb->psy_syscargs;

    /* Check for open cursor on x100 table */
    if (sess_cb->pss_numcursors && psq_any_vwtab_cursor(sess_cb, err_blk) )
    {
	psf_error(9492, 0L, PSF_USERERR, &err_code, err_blk, 0);
	return(E_DB_ERROR);
    }

    if (status = qef_call(QEU_IVWCALL, ( PTR ) &qef_rcb))
    {
	err_blk->err_code = E_PS0001_USER_ERROR;
    }

    return(status);
}
