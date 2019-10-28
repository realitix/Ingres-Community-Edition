/*
** Copyright (c) 2011, 2011 Actian Corporation
**
NO_OPTIM = dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <pc.h>
#include    <st.h>
#include    <me.h>
#include    <cs.h>
#include    <jf.h>
#include    <ck.h>
#include    <cv.h>
#include    <di.h>
#include    <id.h>
#include    <lo.h>
#include    <nm.h>
#include    <si.h>
#include    <tm.h>
#include    <tr.h>
#include    <er.h>
#include    <bt.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgkdef.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm0m.h>
#include    <dmfx100.h>
#include    <x100.h>

/*
**  Name: DMFX100.C - DMF x100 support routines.
**
**  Description:
**	Function prototypes are defined in DMFX100.H.
**
**      This file contains DMF x100 support routines
**
**	  Internal routines:
**
**          x100_terminate - terminate the x100 process associated with a db
**          x100_stall - write_lock the x100 process associated with a db
**          x100_unstall - write_unlock the x100 process associated with a db
**
**  History:
**      23-jun-2011 (stial01)
**          Moved dmf x100 routines here.
**          Updated x100_terminate,x100_stall,x100_unstall to do direct syscall,
**          instead of invoking terminal monitor to issue sql for syscall.
**      28-jun-2011 (stial01)
**          Use DEFINE_DB_TEXT_STRING to define text strings
**	29-Jun-2011 (kiria01)
**	    Drop trailing semicolon from DEFINE_DB_TEXT_STRINGs as it is not
**	    portable to have a null statement in a declaration list
*/


/*
** Name: x100_terminate - terminate x100 server 
**
**
** Description:
**	This routine is called at the beginning of rollforwarddb x100 db
**
** Inputs:
**      jsx
**      dcb
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**   The rollforwarddb will be delayed if a transaction is outstanding and
**   quiescing of the database is required before the terminate 'me'
**   is run. The completion of transactions even though a rollforwarddb is
**   about to be performed is required to ensure calling applications 
**   receive a valid reply.
**
**   Write transactions:
**        Any data written to the Vectorwise table is lost.
**
**   Read transactions:
**        Existing read transactions are stopped. This may cause subsequent
**        or dependent applications that are running to fail.
**
**   These are expected results of running rollforwarddb.
**
** History:
**	29-apr-2011 (stial01) m1700
**	    Created.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/
DB_STATUS
x100_terminate(
    DMF_JSX	*jsx,
    DMP_DCB     *dcb)
{
    STATUS		status;
    STATUS		local_status;
    DB_ERROR		*dberr;
    char		line_buffer[132];
    ADF_CB		adf_cb;
    X100_SESSION_PARAM	x100sp;
    X100_SYSCALL_PARAM	x100cb;
    char		dbnamestr[DB_DB_MAXNAME + 1];
    bool		x100_session = FALSE;
    bool		write_lock = FALSE;
    DEFINE_DB_TEXT_STRING(write_lock_str, "write_lock")
    DEFINE_DB_TEXT_STRING(write_unlock_str, "write_unlock")
    DEFINE_DB_TEXT_STRING(terminate_str, "terminate")
    DEFINE_DB_TEXT_STRING(me_str, "me")

    for ( ; ; )
    {
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: Quiescing Ingres VectorWise engine associated to : %~t\n",
	    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);

	MEmove(sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name, '\0',
	    sizeof(DB_DB_NAME) + 1, dbnamestr);
	STtrmwhite(dbnamestr);

	/* Init/zero fill adf control block */
	MEfill(sizeof(ADF_CB), 0, &adf_cb);

	/* Fill in session param. */
	x100sp.ing_sessid = DB_NOSESSION;
	x100sp.core.adfcb = &adf_cb;
	x100sp.core.dbname = dbnamestr;
    
	status = x100_session_begin(&x100sp);
	if (status)
	    break;

	x100_session = TRUE;

	x100cb.core.session = x100sp.core.session;
	x100cb.core.adfcb = &adf_cb;
	x100cb.core.dbname = dbnamestr;
	x100cb.cb_qef_rcb =  NULL;
	x100cb.update_count = NULL;
	x100cb.table_access = NULL;
	x100cb.callname = (DB_TEXT_STRING *)&write_lock_str;
	x100cb.callparam = (DB_TEXT_STRING *)NULL;

	status = x100_syscall(&x100cb);
	if (status)
	    break;

	write_lock = TRUE;

	x100cb.callname = (DB_TEXT_STRING *)&terminate_str;
	x100cb.callparam = (DB_TEXT_STRING *)&me_str;
	status = x100_syscall(&x100cb);
	if (status)
	    break;

	local_status = x100_session_end(&x100sp);

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: Stopped Ingres VectorWise engine for: %~t\n",
	sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);

	return (E_DB_OK);
    }

    if (write_lock)
    {
	x100cb.callname = (DB_TEXT_STRING *)&write_unlock_str;
	x100cb.callparam = (DB_TEXT_STRING *)NULL;
	status = x100_syscall(&x100cb);
    }

    if (x100_session)
	local_status = x100_session_end(&x100sp);

    return (E_DB_ERROR);
}

/*
** Name: x100_stall - get x100 write_lock
**
**
** Description: get x100 write_lock
**	This routine is called at the beginning of checkpoint x100 db
**
** Inputs:
**	jsx
**	dcb
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**    The ckpdb will be delayed if a transaction is outstanding and
**    quiescing of the database is required.
**
** History:
**	29-apr-2011 (stial01) m1700
**	    Created.
*/
DB_STATUS
x100_stall(
    DMF_JSX	*jsx,
    DMP_DCB     *dcb)
{
    STATUS		status;
    STATUS		local_status;
    DB_ERROR		*dberr;
    char		line_buffer[132];
    ADF_CB		adf_cb;
    X100_SESSION_PARAM	x100sp;
    X100_SYSCALL_PARAM	x100cb;
    char		dbnamestr[DB_DB_MAXNAME + 1];
    bool		x100_session = FALSE;
    DEFINE_DB_TEXT_STRING(write_lock_str, "write_lock")

    for ( ; ; )
    {
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ CPP: Quiescing Ingres VectorWise engine associated to : %~t\n",
	    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);

	MEmove(sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name, '\0',
	    sizeof(DB_DB_NAME) + 1, dbnamestr);
	STtrmwhite(dbnamestr);

	/* Init/zero fill adf control block */
	MEfill(sizeof(ADF_CB), 0, &adf_cb);

	/* Fill in session param. */
	x100sp.ing_sessid = DB_NOSESSION;
	x100sp.core.adfcb = &adf_cb;
	x100sp.core.dbname = dbnamestr;
    
	status = x100_session_begin(&x100sp);
	if (status)
	    break;

	x100_session = TRUE;

	x100cb.core.session = x100sp.core.session;
	x100cb.core.adfcb = &adf_cb;
	x100cb.core.dbname = dbnamestr;
	x100cb.cb_qef_rcb =  NULL;
	x100cb.update_count = NULL;
	x100cb.table_access = NULL;
	x100cb.callname = (DB_TEXT_STRING *)&write_lock_str;
	x100cb.callparam = (DB_TEXT_STRING *)NULL;

	status = x100_syscall(&x100cb);
	break; /* ALWAYS */
    }

    if (x100_session)
	local_status = x100_session_end(&x100sp);

    return (status);
}

  
/*
** Name: x100_unstall - issue x100 write_unlock
**
**
** Description:
**	This routine is called at the end of checkpoint x100 db
**
** Inputs:
**	jsx
**	dcb
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-apr-2011 (stial01) m1700
**	    Created.
*/
DB_STATUS
x100_unstall(
    DMF_JSX	*jsx,
    DMP_DCB     *dcb)
{
    STATUS		status;
    STATUS		local_status;
    DB_ERROR		*dberr;
    char		line_buffer[132];
    ADF_CB		adf_cb;
    X100_SESSION_PARAM	x100sp;
    X100_SYSCALL_PARAM	x100cb;
    char		dbnamestr[DB_DB_MAXNAME + 1];
    bool		x100_session = FALSE;
    DEFINE_DB_TEXT_STRING(write_unlock_str, "write_unlock")

    for ( ; ; )
    {
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ CPP: Unlocking Ingres VectorWise engine associated to : %~t\n",
	    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);

	MEmove(sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name, '\0',
	    sizeof(DB_DB_NAME) + 1, dbnamestr);
	STtrmwhite(dbnamestr);

	/* Init/zero fill adf control block */
	MEfill(sizeof(ADF_CB), 0, &adf_cb);

	/* Fill in session param. */
	x100sp.ing_sessid = DB_NOSESSION;
	x100sp.core.adfcb = &adf_cb;
	x100sp.core.dbname = dbnamestr;
    
	status = x100_session_begin(&x100sp);
	if (status)
	    break;

	x100_session = TRUE;

	x100cb.core.session = x100sp.core.session;
	x100cb.core.adfcb = &adf_cb;
	x100cb.core.dbname = dbnamestr;
	x100cb.cb_qef_rcb =  NULL;
	x100cb.update_count = NULL;
	x100cb.table_access = NULL;
	x100cb.callname = (DB_TEXT_STRING *)&write_unlock_str;
	x100cb.callparam = (DB_TEXT_STRING *)NULL;

	status = x100_syscall(&x100cb);
	break; /* ALWAYS */
    }

    if (x100_session)
	local_status = x100_session_end(&x100sp);

    return (status);
}
