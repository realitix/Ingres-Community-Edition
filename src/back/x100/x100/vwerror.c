/*
** Copyright (c) 2010 Actian Corporation
*/

#include <compat.h>

/* The whole thing is IVW only */
#if defined(conf_IVW)

#include <gl.h>
#include <iicommon.h>
#include <cs.h>
#include <er.h>
#include <cm.h>
#include <me.h>
#include <st.h>
#include <gca.h>
#include <adf.h>
#include <ulf.h>
#include <dbdbms.h>
#include <dmf.h>
#include <scf.h>
#include <usererror.h>
#include <generr.h>
#include <qefmain.h>
#include <sqlstate.h>
#include <x100.h>
#include <x100client.h>
#include <x100/vwlog.h>

/**
**
**  Name: VWERROR.C -	    contains on the support routines necessary for
**			    error-handling in VWF.
**
**  Description:
**      This file contains all the routines necessary to handle errors in
**	VWF in the Ingres manner. 
**
**          vwErrorFcn() -	format an error msg for an VWF error code
**				and translate it to the appropriate DB_STATUS.
**
**
**  History:
**      02-Mar-2010 (kiria01)
**          Initial creation.
**      16-feb-2011 (stial01)
**          Reset error code if error already reported to user or errlog.
**          Also we must send user 4328, 4330 errors here since they have args
**/
    

/*
**  Defines of other constants.
*/

/*  Define the maximum number of error parameters allowed. */
#define			    MAX_PARMS	6


/*{
** Name: aduErrorFcn()	-   format an error message for an VWF error code and
**			    translate to appropriate DB_STATUS code.
**
** Description:
**        This routine is used to format an error message for an VWF error
**	code and then translates the error code to a DB_STATUS code to
**	be returned.  Note, the return status in this procedure is overloaded.
**	It is intended to map an VWF error code to the appropriate DB_STATUS
**	error code.  However, if an error occurs in executing this routine,
**	it will be signaled by returning an E_DB_ERROR code.  This case
**	can be determined by looking at the contents of the ad_errcode field
**	in the VWF error block, adf_errcb.
**	  This routine will also format VWF warning messages.
**
** Inputs:
**	cb				Pointer to an VWF control block.
**	  cb->adfcb			Pointer to an ADF session control block.
**	    .adf_errcb			VWF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.  If <= 0, no
**					error message will be formatted.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.  If
**					NULL, no error message will be
**					formatted.
**      x100_errorcode                  The VWF error code to format a message
**					for and translate to a DB_STATUS code.
**					Arbitrary status codes may be revectored
**					in this routine.
**	FileName			Pointer to filename of the adu_error
**					macro making this call.
**	LineNumber			Line number within that file.
**	pcnt				This is a count of the optional
**					parameters which should be formated
**					into the error message.  If no
**					parameters are necessary, this should
**					be zero.
**	[p1 ... p6]			Optional parameter pairs. The only
**					supported type is null terminated string
**
** Outputs:
**	  adfcb				Pointer to an VWF session control block.
**	    .adf_errcb			VWF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen <= 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		VWF error code for the error.
**		.ad_errclass		Signifies the VWF error class.
**		.ad_usererr		If .ad_errclass is VWF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an VWF error code or a user-error code.
**		.ad_sqlstate		This field is set to the SQLSTATE status
**					code this error maps to.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**		.adf_dberror
**		    .err_code		Duplicates ad_errcode
**		    .err_data		0
**		    .err_file		FileName
**		    .err_line		LineNumber
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adfcb.adf_errcb.ad_errcode to determine
**	    the VWF error code.  The following is a list of possible VWF error
**	    codes that can be returned by this routine:
**
**	    E_VW0000_OK			If the VWF error was actually OK.
**	    E_AD1021_BAD_ERROR_PARAMS	If the too many parameter arguments
**					are passed in or an odd number of
**					parameter arguments is passed in.
**	    E_AD1025_BAD_ERROR_NUM	The VWF error code passed in was
**					unknown to adu_error().
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-Mar-2010 (kiria01)
**          Initial creation.
**	24-Feb-2011 (kiria01)
**	    Tie MAX_PARMS to MAX_X100_ERROR_PARAMS to avoid
**	    referencing bad stack locations.
**	13-Jun-2011 (drivi01) Mantis 1406
**		Fix case statement for E_VW1005_FLTUNDERFLOW
**		by adding ret_status = E_DB_ERROR so that
**		correct status is reflected upon return from
**		vw_translate_err.
**	01-Aug-2011 (bonro01) SIR 125458
**	    Update cs_elog calls inline with sc0e_putAsFcn changes.
*/


static ADF_CB *get_adfcb(VW_CORE_PARMS	    *cb)
{
    struct X100SessionState *state;
    state = cb->session;
    if (state == NULL)
    {
	CS_SID sid;
	CSget_sid(&sid);
	state = GET_X100_SCB(sid);
    }
    return x100_session_get_adfcb(state);
}

static CS_SID get_sessid(VW_CORE_PARMS	    *cb)
{
    if (cb->session == NULL)
    {
	CS_SID sid;
	CSget_sid(&sid);
	return sid;
    }
    return x100_session_get_sessid(cb->session);
}

/*VARARGS3*/
DB_STATUS
vwErrorFcn(
struct X100SessionState *session,
i4         	    vw_errorcode,
PTR		    FileName,
i4		    LineNumber,
i4		    pcnt,
		    ...)
{
    DB_STATUS           ret_status = E_DB_OK;
    DB_STATUS           uleret = ret_status;
    DB_SQLSTATE		sqlstate;
    i4			ulecode;
    char		errbuf[DB_ERR_SIZE];
    i4			msglen;
    i4			num_params;
    i4			psize[MAX_PARMS];
    PTR			pvalue[MAX_PARMS];
    bool		user_local = TRUE;
    i4			log_flag = ULE_LOG; /* By default capture to log */
    ADF_ERROR		*adf_errcb;
    va_list		ap;
    DB_ERROR		dbError;

    if (!session || !x100_session_get_adfcb(session))
	return (ret_status);

    adf_errcb = &x100_session_get_adfcb(session)->adf_errcb;

    if (vw_errorcode == E_AD0000_OK)
    {
	CLRDBERR(&adf_errcb->ad_dberror);
	adf_errcb->ad_errcode = vw_errorcode;
	return ret_status;
    }

    /*
    ** Plant caller's source information in the ADF_SCB's
    ** DB_ERROR.
    **
    ** We guarantee that err_code will match
    ** what ends up in adf_errcb.ad_errcode.
    */
    adf_errcb->ad_dberror.err_code = vw_errorcode;
    adf_errcb->ad_dberror.err_data = vw_errorcode;
    adf_errcb->ad_dberror.err_file = FileName;
    adf_errcb->ad_dberror.err_line = LineNumber;

    /* Set the ADF error block's fields correctly */

    /* By default, set the error class to "user error".
    ** This is done because all of the internal errors in the switch
    ** stmt below perform the same action and it can easily be reset
    ** there for internal errors.
    */
    adf_errcb->ad_errclass  = ADF_USER_ERROR;
    adf_errcb->ad_errcode   = vw_errorcode;

    switch (vw_errorcode)
    {
    /*
    ** VW errors to be displayed to the user as is AND logged to errlog.log
    */
    case E_VW1006_GENERIC: /* Generic error */
    case E_VW1007_ERR_OOM: /* Out of memory. */
    case E_VW1010_BKLD_STATE_ERROR:
    case E_VW1011_BKLD_PUSH_DATA_ERROR:
    case E_VW1012_SIMPLE_QRY_ERROR:
    case E_VW1013_PKEY_ERROR:
    case E_VW1014_FKEY_ERROR:
    case E_VW1016_PARSE: /* "Parse error in %0 at line %1, column %2: %3."*/
    case E_VW1027_REWRITER: /* "Rewriter error." */
    case E_VW1028_BUILDER: /* "Error building operator tree." */
    case E_VW1029_OPERATORTREE: /* "Error in operator tree." */
    case E_VW1030_INVALIDSYSCALL: /* "Invalid system call '%s'." */
    case E_VW1035_CONNECTION: /* "Cannot connect to VectorWise server." */
    case E_VW1036_NETWORK: /*  "VectorWise server network communication error." */
    case E_VW1037_INTERNAL: /* "Internal VectorWise error: %0." */
    case E_VW1038_CHECKPOINT: /* "Error in x100 checkpoint: %0." */
    case E_VW1039_SINGLEWRITER: /* "Failed to hold single writer lock - abort write query." */
    case E_VW1040_TABLE_EXISTS: /* "Table '%0' already exists." */
    case E_VW1041_TABLE_NOT_EXISTS: /* "Table '%0' does not exist." */
    case E_VW1042_KEY_EXISTS: /* "Key '%0' already exists." */
    case E_VW1043_KEY_NOT_EXISTS: /* "Key '%0' on table '%1' does not exist." */
    case E_VW1044_INDEX_EXISTS: /* "Index '%0' already exists." */
    case E_VW1045_INDEX_NOT_EXISTS: /* "Index '%0' on table '%1' does not exist." */
    case E_VW1046_CS_EXISTS: /* "Columnspace '%0' already exists." */
    case E_VW1047_CS_NOT_EXISTS: /* "Columnspace '%0' does not exist." */
    case E_VW1048_COL_NOT_FOUND: /* "Column '%0' in table '%1', not found." */
    case E_VW1049_HAS_PRIMARY: /* "Table '%0' already has primary index." */
    case E_VW1054_IDX_TYPE_EXISTS: /* "Table '%1' already has %0 index." */
    case E_VW1057_WRITE: /* "Failed writing block." */
    case E_VW1058_WRITELOG: /* "Failed writing log." */
    case E_VW1059_WRITESORT: /* "Failed writing during disk spilling sort." */
    case E_VW1060_WRITECHECKPOINT: /* "Failed writing during checkpointing." */
    case E_VW1061_READ: /* "Failed reading block." */
    case E_VW1062_READLOG: /* "Failed reading log." */
    case E_VW1063_READSORT: /* "Failed reading during disk spilling sort." */
    case E_VW1064_CHECKPOINTDELIMITER: /* "Unexepected delimiter '%0' at position %1." */
    case E_VW1065_CHECKPOINTTABLE: /* "Table '%0' not found during checkpointing." */
    case E_VW1066_CHECKPOINTINSMAIN: /* "Table '%0' cannot be inserted during checkpointing as it is the main table." */
    case E_VW1067_CHECKPOINTEXISTS: /* "Table '%0' cannot be used for checkpointiing multiple times in one system call." */
    case E_VW1068_TABLESTRUCT: /* "Table '%0' and table '%1' not equal in structure." */
    case E_VW1069_APPEND_IDX_NOT_SUP: /* "Appending to non empty tables with indexes not supported." */
    case E_VW1073_CHECKPOINTNOSPACE: /* "Checkpointing cannot be executed because not enough free space in the columnspace." */
    case E_VW1085_ADCOL_UPD_CONFLICT: /* "Cannot add or drop column for tables with in-memory updates. Please checkpoint the table first."*/
    case E_VW1086_COLUMN_EXISTS: /* "Column '%0' already exists." */
    case E_VW1087_CHECKPOINT_APPEND: /* "Combine/checkpointing failed on appending data." */
    case E_VW1088_CHECKPOINT_CONSTRAINT: /* "Combine/checkpointing failed on constraint violation." */
    case E_VW1089_COMBINE_START_FRESH: /* "The current transaction contains uncommitted changes, to continue this operation first commit or abort." */ 
    case E_VW1090_DPCOL_IDX_CONFLICT: /* "Column cannot be dropped because of indices, drop index first before dropping column." */
    case E_VW1092_BLK_VERSION: /* Block version verification failed. Data is not compatible with this release of database. */
    case E_VW1093_INVALIDSYSCALL_ARGS: /* "Invalid system call arguments." */
    case E_VW1094_WRITE_LOCK_OTHERS: /* "Failed to acquire global write lock. Abort or commit all concurrent update transactions and try again." */
    case E_VW1095_WRITE_LOCK_SELF: /* "There are uncommitted changes in this transaction, abort or commit to continue." */
    case E_VW1096_WRITE_LOCK_HOLD: /* "Global write lock is being held. Try to unlock it first." */
    case E_VW1097_CYCLIC_FK_NOT_SUP: /* "Cyclic foreign key references are not allowed." */
    case E_VW1098_SERVER_LIMIT: /* "Active VectorWise databases limit reached." */
    case E_VW1099_SERVER_START: /* "Cannot start VectorWise server." */
	adf_errcb->ad_usererr = vw_errorcode;
	ret_status = E_DB_ERROR;
	break;

    /*
    ** VW specific user errors that don't need logging to errlog.log
    */
    case E_VW1026_CAST: /* "Error casting '%0' from type '%1' to type '%2'." */
    case E_VW1033_PKDELVIOLATION: /* "Cannot DELETE because the values are still used." */
    case E_VW1034_FKINSVIOLATION: /* "Cannot INSERT because the values do not match referenced table." */
    case E_VW1050_INCORRECT_REF_KEY: /* "Incorrect referenced key type." */
    case E_VW1051_JOIN_NULLABLE: /* "Join index on nullable columns not allowed." */
    case E_VW1052_KEY_NON_EMPTY: /* "Key on non-empty table not allowed." */
    case E_VW1053_IDX_NON_EMPTY: /* "Index on non-empty table not allowed." */
    case E_VW1055_SPARSE_JOIN_INV_COL: /* "First column in sparse index should be join index, if one is defined." */
    case E_VW1056_ERR_DML_JI_NOT_SUP: /* "DML operations on tables with join indexes not supported." */
    case E_VW1071_FKUPDVIOLATION: /* "Cannot UPDATE because the new values do not match referenced table." */
    case E_VW1072_PKUPDVIOLATION: /* "Cannot UPDATE because the old values are still used." */
    case E_VW1074_DROP_INC_JI: /* "Cannot drop table with incoming join index." */
    case E_VW1076_INVALID_ARG: /* "Invalid argument type for operation '%0'." */
    case E_VW1077_UNICODE_CONVERSION: /* "Unicode conversion error." */
    case E_VW1078_NO_TYPE_CONVERSION: /* "No conversion defined for result type." */
    case E_VW1079_DECOVF_ERROR: /* "Packed decimal overflow." */
    case E_VW1080_INVALID_EXTRACT: /* "Invalid parameters to extract." */
    case E_VW1082_NULL_TO_NONNULL: /* "An attempt to place a null value in a non-nullable datatype." */
    case E_VW1083_UPD_IDX_NOT_SUP: /* "An attemt to modify an indexed attribute (which defines sort order)." */
    case E_VW1084_NO_ACCESS_TO_TABLE: /* "You do not have any privilege to access table '%0c'." */
    case E_VW1091_JI_PARENT_MODIFY: /* "Cannot modify sort key of joinindex parent." */
    /* concurrent transactions conflicts */
    case E_VW1101_CONFL_CREATE_DUP_NAME:
    case E_VW1102_CONFL_DROPTBL_DDL:
    case E_VW1103_CONFL_CREATEKEY_DDL:
    case E_VW1104_CONFL_DROPKEY_DDL:
    case E_VW1105_CONFL_CREATEIDX_DDL:
    case E_VW1106_CONFL_DROPIDX_DDL:
    case E_VW1107_CONFL_ADDCOL_DDL:
    case E_VW1108_CONFL_DROPCOL_DDL:
    case E_VW1109_CONFL_APPEND_DDL:
    case E_VW1110_CONFL_UPDATE_DDL:
    case E_VW1111_CONFL_CKT_DDL:
    case E_VW1112_CONFL_COMB_DDL:
    case E_VW1113_CONFL_DROPTBL_DML:
    case E_VW1114_CONFL_CREATEKEY_DML:
    case E_VW1115_CONFL_DROPKEY_DML:
    case E_VW1116_CONFL_CREATEIDX_DML:
    case E_VW1117_CONFL_DROPIDX_DML:
    case E_VW1118_CONFL_ADDCOL_DML:
    case E_VW1119_CONFL_DROPCOL_DML:
    case E_VW1120_CONFL_APPEND_DML:
    case E_VW1121_CONFL_UPDATE_DML:
    case E_VW1122_CONFL_CKT_DML:
    case E_VW1123_CONFL_COMB_DML:
    case E_VW1124_CONFL_CREATEFK_REFDDL:
    case E_VW1125_CONFL_CREATEJI_REFDDL:
    case E_VW1126_CONFL_UPDATE_UPDATE:
    case E_VW1127_CONFL_UNIQ:
    case E_VW1128_CONFL_FK:
    case E_VW1131_CONFL_UPDATE_CKT:
	adf_errcb->ad_usererr = vw_errorcode;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

      /* 
      ** Error codes that get mapped to a user
      ** error code in ERUSF.MSG follow.
      ** NOTE if we map a E_VW error to a E_US error that has args ..
      ** the error MUST be sent to the user here (user_local = TRUE)  
      ** We should generally be able to send any user error here,
      ** except for 4500, 4501 Duplicate key .. detected
      ** which should be returned for the caller to handle
      */
    case E_VW1009_QRY_SYN_ERR:
	adf_errcb->ad_usererr	= 9374;
	ret_status = E_DB_ERROR;
	break;

    case E_VW1001_INTDIVBYZERO:
	adf_errcb->ad_usererr	= E_US1069_INTDIV_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1002_INTOUTOFRANGE:
	adf_errcb->ad_usererr	= E_US1068_INTOVF_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1003_FLTDIVBYZERO:
	adf_errcb->ad_usererr	= E_US106B_FLTDIV_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1004_FLTOVERFLOW:
	adf_errcb->ad_usererr	= E_US106A_FLTOVF_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1005_FLTUNDERFLOW:
	adf_errcb->ad_usererr	= E_US106C_FLTUND_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1008_QRY_BAD_DT_ERROR:
	adf_errcb->ad_usererr = 5565;
	ret_status = E_DB_ERROR;
	user_local = 0; /* Do log but not to user */
	break;

    case E_VW1015_KILL:
	/* This one used to indicate interrupt, get out now without logging */
	return E_DB_ERROR;

    case E_VW1017_INVALIDDOMAIN: 
	/* "Argument outside of domain of math function." */
	adf_errcb->ad_usererr = 4209;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1018_INVALIDNUMERIC: 
	/* "String cannot be converted to numeric." */
	adf_errcb->ad_usererr = 4111;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1019_INVALIDTIME: 
	/* "'%0' is not a valid time value." */
	adf_errcb->ad_usererr = 4326;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1020_INVALIDTIMESTAMP: 
	/* "'%0' is not a valid timestamp value." */
	adf_errcb->ad_usererr = 4328;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1021_INVALIDINTERVALYM: 
	/* "'%0' is not a valid value for interval year to month." */
	adf_errcb->ad_usererr = 4330;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1022_INVALIDINTERVALDS: 
	/* "'%0' is not a valid value for interval day to second." */
	adf_errcb->ad_usererr = 4331;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1023_INVALIDDATE: 
	/* "'%0' is not a valid date/time value." */
	adf_errcb->ad_usererr = 4302;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1024_INVALIDMONEY: 
	/* "'%0' is not a valid money value." */
	adf_errcb->ad_usererr = 4400;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;

    case E_VW1025_INVALIDBOOL: 
	/* "'%0' is not a valid boolean value." */
	adf_errcb->ad_usererr = E_AD106A_INV_STR_FOR_BOOL_CAST;
	ret_status = E_DB_ERROR;
	log_flag = 0; /* Don't log */
	break;
	
    case E_VW1031_COMMIT: 
	/* "Error committing transaction." */
	adf_errcb->ad_usererr = E_VW1031_COMMIT;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1032_INSUNIQUECONSVIOLAT:
	/* "Duplicate unique key detected." */
	adf_errcb->ad_usererr = 4500; /* "Duplicate key on INSERT detected." */
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;

    case E_VW1070_UPDUNIQUECONSVIOLAT:
	/* "Duplicate unique key detected." */
	adf_errcb->ad_usererr = 4501; /* "Duplicate key on UPDATE detected." */
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;
    case E_VW1075_DATE_DIV_ANSI_INTV:
	/* "Invalid interval division." */
	adf_errcb->ad_usererr = E_US10F2_DATE_DIV_ANSI_INTV; /* "Cannot mix ANSI intervals in division." */
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;
    case E_VW1081_INTOVERFLOW:
	adf_errcb->ad_usererr	= E_US1068_INTOVF_ERROR;
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;
    case E_VW1129_MNYOVF_ERROR:
	adf_errcb->ad_usererr	= 4401; /* exceeded the maximum money value allowed. */
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;
    case E_VW1130_MNYUDF_ERROR:
	adf_errcb->ad_usererr	= 4402; /* value specified is less than the minimal legal value. */
	ret_status = E_DB_ERROR;
	user_local = log_flag = 0; /* Don't log */
	break;
    case E_VW9997_UNMAPPED_ERROR:
    case E_VW9998_INTERNAL_ERROR:
    case E_VW9999_INTERNAL_ERROR:
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	ret_status = E_DB_ERROR;
	break;

    default:
	/* Call adu_error() to handle an unknown VWF error code. */
	return (aduErrorFcn(x100_session_get_adfcb(session), E_AD1025_BAD_ERROR_NUM, 
			  adf_errcb->ad_dberror.err_file,
			  adf_errcb->ad_dberror.err_line,
			  2,
			  (i4) sizeof(vw_errorcode),
			  (i4 *) &vw_errorcode));
    }


    /* Format the error msg */
    if (pcnt > MAX_PARMS)
    {
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	adf_errcb->ad_errcode	= E_AD1021_BAD_ERROR_PARAMS;
	vw_errorcode		= E_AD1021_BAD_ERROR_PARAMS;
	pcnt			= 0;
    }

    /* If this is a user error, we want to format the user error message
    ** and not the internal ADF error number.
    */
    CLRDBERR(&dbError);
    if (adf_errcb->ad_errclass == ADF_USER_ERROR)
	vw_errorcode =adf_errcb->ad_usererr;
    SETDBERR(&dbError, 0, vw_errorcode);

    /* Extract variable parameters */
    va_start(ap, pcnt);
    for (num_params = 0; num_params < MAX_PARMS; num_params++)
    {
	if (num_params < pcnt)
	{
	    pvalue[num_params] = (PTR)va_arg(ap, PTR);
	    psize[num_params] = STlen(pvalue[num_params]);
	}
	else
	{
	    pvalue[num_params] = NULL;
	    psize[num_params] = 0;
	}
    }
    va_end( ap );

    uleret = uleFormat(&dbError, 0, NULL, log_flag, &sqlstate,
			errbuf, (i4) sizeof(errbuf), &msglen, &ulecode,
			num_params,
			psize[0], pvalue[0],
			psize[1], pvalue[1],
			psize[2], pvalue[2],
			psize[3], pvalue[3],
			psize[4], pvalue[4]);
    /*
    ** If ule_format failed, we probably can't report any error.
    ** Instead, just propagate the error up to the user.
    */
    if (uleret != E_DB_OK)
    {
	STprintf(errbuf, "Error message corresponding to %d (%x) \
	    not found in INGRES error file", vw_errorcode, vw_errorcode);
	msglen = STlength(errbuf);
	uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
	    errbuf, msglen, 0, &ulecode, 0);
	/*ret_status = ulecode;*/
    }

    if (adf_errcb->ad_errclass == ADF_USER_ERROR &&
	user_local)
    {
	SCF_CB scf_cb;
	DB_STATUS scf_status;
	scf_cb.scf_length = sizeof(scf_cb);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_QEF_ID;
	scf_cb.scf_session = x100_session_get_sessid(session);
	scf_cb.scf_nbr_union.scf_local_error = vw_errorcode;

	STRUCT_ASSIGN_MACRO(sqlstate, scf_cb.scf_aux_union.scf_sqlstate);
	scf_cb.scf_len_union.scf_blength = msglen;
	scf_cb.scf_ptr_union.scf_buffer = errbuf;
	scf_status = scf_call(SCC_ERROR, &scf_cb);
	if (scf_status != E_DB_OK && !ret_status)
	{
	    ret_status = scf_cb.scf_error.err_code;
	}
	else
	{
	    /* This user error already reported to user */
	    vw_errorcode = E_QE0025_USER_ERROR;
	}
    }
    else if (log_flag && adf_errcb->ad_errclass == ADF_INTERNAL_ERROR)
    {
	/* This internal error already reported to errlog */
	vw_errorcode = E_QE000B_ADF_INTERNAL_FAILURE;
    }

    adf_errcb->ad_dberror.err_code = vw_errorcode;

    return ret_status;
}

/* Non-variadic function form */
static DB_STATUS
vw_errorNV(
struct X100SessionState *session,
i4		x100_errorcode,
i4		pcnt,
		    ...)
{
    PTR		pn[MAX_PARMS];
    i4		i;
    va_list	ap;

    va_start( ap, pcnt );
    for ( i = 0; i < pcnt && i < MAX_PARMS; i++ )
    {
        pn[i] = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    return vwErrorFcn(session, x100_errorcode, "", 0, pcnt, pn[0], pn[1], pn[2]);
}

DB_STATUS
vwTranslateErr(
struct X100SessionState *session,
	void *_x100err)
{
    char		errbuf[DB_ERR_SIZE];
    i4 vw_errorcode;
    struct X100Error *x100err = _x100err;

    if (x100err == NULL)
	return vwErrorFcn(session, E_VW1035_CONNECTION, "", 0, 0);

    switch (x100err->code)
    {
    /* Allow for the remapping of arbitrary statuses if not of the VW facility:
    ** initial values are purely temporary for test. */ 
    case -1:	    vw_errorcode = E_VW1009_QRY_SYN_ERR; break;
    case X100_ERR_INTDIVBYZERO:
	/*"Integer division by zero." */
	vw_errorcode = E_VW1001_INTDIVBYZERO; break;
    case X100_ERR_INTOUTOFRANGE:
	/* "Integer '%0c' out of range.") */
	vw_errorcode = E_VW1002_INTOUTOFRANGE; break;
    case X100_ERR_FLTDIVBYZERO:
	/* "Floating point division by zero." */
	vw_errorcode = E_VW1003_FLTDIVBYZERO; break;
    case X100_ERR_FLTOVERFLOW:
	/* "Floating point overflow." */
	vw_errorcode = E_VW1004_FLTOVERFLOW; break;
    case X100_ERR_FLTUNDERFLOW:
	/*"Floating point underflow." */
	vw_errorcode = E_VW1005_FLTUNDERFLOW; break;
    case X100_ERR_GENERIC:
	/* Generic error */
	vw_errorcode = E_VW1006_GENERIC; break;
    case X100_ERR_OOM:
	/*"Out of memory."*/
	vw_errorcode = E_VW1007_ERR_OOM; break;
    case X100_ERR_KILL:
	/* We use this one to show a user abort / interrupt.
	** QEF will handle VW1015 specially.
	*/
	vw_errorcode = E_VW1015_KILL; break;
    case X100_ERR_PARSE:
	/* "Parse error in %0 at line %1, column %2: %3."*/
	vw_errorcode = E_VW1016_PARSE; break;
    case X100_ERR_INVALIDDOMAIN:
	/* "Argument outside of domain of math function." E_US1071_4209 */
	vw_errorcode = E_VW1017_INVALIDDOMAIN; break;
    case X100_ERR_INVALIDNUMERIC:
	/* "String '%0' cannot be converted to numeric." E_US100F_4111 */
	vw_errorcode = E_VW1018_INVALIDNUMERIC; break;
    case X100_ERR_INVALIDTIME:
	/* "'%0' is not a valid time value." E_US10E6_4326 */
	vw_errorcode = E_VW1019_INVALIDTIME; break;
    case X100_ERR_INVALIDTIMESTAMP:
	/* "'%0' is not a valid timestamp value." E_US10E8_4328 */
	vw_errorcode = E_VW1020_INVALIDTIMESTAMP; break;
    case X100_ERR_INVALIDINTERVALYM:
	/* "'%0' is not a valid value for interval year to month." E_US10EA_4330 */
	vw_errorcode = E_VW1021_INVALIDINTERVALYM; break;
    case X100_ERR_INVALIDINTERVALDS:
	/* "'%0' is not a valid value for interval day to second." E_US10EB_4331 */
	vw_errorcode = E_VW1022_INVALIDINTERVALDS; break;
    case X100_ERR_INVALIDDATE:
	/* "'%0' is not a valid date/time value." E_US10CE_4302 */
	vw_errorcode = E_VW1023_INVALIDDATE; break;
    case X100_ERR_INVALIDMONEY:
	/* "'%0' is not a valid money value." E_US1130_4400 */
	vw_errorcode = E_VW1024_INVALIDMONEY; break;
    case X100_ERR_INVALIDBOOL:
	/* "'%0' is not a valid boolean value." E_AD106A_INV_STR_FOR_BOOL_CAST */
	vw_errorcode = E_VW1025_INVALIDBOOL; break;
    case X100_ERR_CAST:
	/* "Error casting '%0' from type '%1' to type '%2'." E_WT0037_CAST_ERR */
	vw_errorcode = E_VW1026_CAST; break;
    case X100_ERR_REWRITER:
	/* "Rewriter error." */
	vw_errorcode = E_VW1027_REWRITER; break;
    case X100_ERR_BUILDER:
	/* "Error building operator tree." */
	vw_errorcode = E_VW1028_BUILDER; break;
    case X100_ERR_OPERATORTREE:
	/* "Error in operator tree." */
	vw_errorcode = E_VW1029_OPERATORTREE; break;
    case X100_ERR_INVALIDSYSCALL:
	/* "Invalid system call '%s'." */
	vw_errorcode = E_VW1030_INVALIDSYSCALL; break;
    case X100_ERR_COMMIT:
	/* "Error committing transaction." */
	vw_errorcode = E_VW1031_COMMIT; break;
    case X100_ERR_INSUNIQUECONSVIOLATION:
	/* "Duplicate unique key detected." */
	vw_errorcode = E_VW1032_INSUNIQUECONSVIOLAT; break;
    case X100_ERR_PKDELVIOLATION:
	/* "Cannot DELETE because the values are still used." */
	vw_errorcode = E_VW1033_PKDELVIOLATION; break;
    case X100_ERR_FKINSVIOLATION:
	/* "Cannot INSERT because the values do not match referenced table." */
	vw_errorcode = E_VW1034_FKINSVIOLATION; break;
    case X100_ERR_CONNECTION:
	/* "Cannot connect to VectorWise server." */
	vw_errorcode = E_VW1035_CONNECTION; break;
    case X100_ERR_SERVER_LIMIT:
	/* "Active VectorWise databases limit reached." */
	vw_errorcode = E_VW1098_SERVER_LIMIT; break;
    case X100_ERR_SERVER_START:
	/* "Cannot start VectorWise server." */
	vw_errorcode = E_VW1099_SERVER_START; break;
    case X100_ERR_NETWORK:
	/*  "VectorWise server network communication error." */
	vw_errorcode = E_VW1036_NETWORK; break;
    case X100_ERR_CHECKPOINT:
	/* "Checkpointing failed." */
	vw_errorcode = E_VW1038_CHECKPOINT; break;
    case X100_ERR_INTERNAL:
	/* "Internal VectorWise error: %0." */
	vw_errorcode = E_VW1037_INTERNAL; break;
    case X100_ERR_SINGLEWRITER:
	/* "Failed to hold single writer lock - abort write query." */
	vw_errorcode = E_VW1039_SINGLEWRITER; break;
    case X100_ERR_TABLE_EXISTS:
	/* "Table '%0' already exists." */
	vw_errorcode = E_VW1040_TABLE_EXISTS; break;
    case X100_ERR_TABLE_NOT_EXISTS:
	/* "Table '%0' does not exist." */
	vw_errorcode = E_VW1041_TABLE_NOT_EXISTS; break;
    case X100_ERR_KEY_EXISTS:
	/* "Key '%0' already exists." */
	vw_errorcode = E_VW1042_KEY_EXISTS; break;
    case X100_ERR_KEY_NOT_EXISTS:
	/* "Key '%0' on table '%1' does not exist." */
	vw_errorcode = E_VW1043_KEY_NOT_EXISTS; break;
    case X100_ERR_INDEX_EXISTS:
	/* "Index '%0' already exists." */
	vw_errorcode = E_VW1044_INDEX_EXISTS; break;
    case X100_ERR_INDEX_NOT_EXISTS:
	/* "Index '%0' on table '%1' does not exist." */
	vw_errorcode = E_VW1045_INDEX_NOT_EXISTS; break;
    case X100_ERR_CS_EXISTS:
	/* "Columnspace '%0' already exists." */
	vw_errorcode = E_VW1046_CS_EXISTS; break;
    case X100_ERR_CS_NOT_EXISTS:
	/* "Columnspace '%0' does not exist." */
	vw_errorcode = E_VW1047_CS_NOT_EXISTS; break;
    case X100_ERR_COLUMN_EXISTS:
	/* "Column '%0' already exists." */
	vw_errorcode = E_VW1086_COLUMN_EXISTS; break;
    case X100_ERR_COL_NOT_FOUND:
	/* "Column '%0' in table '%1', not found." */
	vw_errorcode = E_VW1048_COL_NOT_FOUND; break;
    case X100_ERR_HAS_PRIMARY:
	/* "Table '%0' already has primary index." */
	vw_errorcode = E_VW1049_HAS_PRIMARY; break;
    case X100_ERR_INCORRECT_REF_KEY:
	/* "Incorrect referenced key type." */
	vw_errorcode = E_VW1050_INCORRECT_REF_KEY; break;
    case X100_ERR_JOIN_NULLABLE:
	/* "Join index on nullable columns not allowed." */
	vw_errorcode = E_VW1051_JOIN_NULLABLE; break;
    case X100_ERR_KEY_NON_EMPTY:
	/* "Key on non-empty table not allowed." */
	vw_errorcode = E_VW1052_KEY_NON_EMPTY; break;
    case X100_ERR_IDX_NON_EMPTY:
	/* "Index on non-empty table not allowed." */
	vw_errorcode = E_VW1053_IDX_NON_EMPTY; break;
    case X100_ERR_IDX_TYPE_EXISTS:
	/* "Table '%1' already has %0 index." */
	vw_errorcode = E_VW1054_IDX_TYPE_EXISTS; break;
    case X100_ERR_SPARSE_JOIN_INV_COL:
	/* "First column in sparse index should be join index, if one is defined." */
	vw_errorcode = E_VW1055_SPARSE_JOIN_INV_COL; break;
    case X100_ERR_DML_JI_NOT_SUP:
	/* "DML operations on tables with join indexes not supported." */
	vw_errorcode = E_VW1056_ERR_DML_JI_NOT_SUP; break;
    case X100_ERR_MODIFY_SK_NOT_SUP:
	/* "An attemt to modify an indexed attribute (which defines sort order)." */
	vw_errorcode =  E_VW1083_UPD_IDX_NOT_SUP; break;
    case X100_ERR_WRITE:
	/* "Failed writing block." */
	vw_errorcode = E_VW1057_WRITE; break;
    case X100_ERR_WRITELOG:
	/* "Failed writing log." */
	vw_errorcode = E_VW1058_WRITELOG; break;
    case X100_ERR_WRITESORT:
	/* "Failed writing during disk spilling sort." */
	vw_errorcode = E_VW1059_WRITESORT; break;
    case X100_ERR_WRITECHECKPOINT:
	/* "Failed writing during checkpointing." */
	vw_errorcode = E_VW1060_WRITECHECKPOINT; break;
    case X100_ERR_READ:
	/* "Failed reading block." */
	vw_errorcode = E_VW1061_READ; break;
    case X100_ERR_READLOG:
	/* "Failed reading log." */
	vw_errorcode = E_VW1062_READLOG; break;
    case X100_ERR_READSORT:
	/* "Failed reading during disk spilling sort." */
	vw_errorcode = E_VW1063_READSORT; break;
    case X100_ERR_CHECKPOINTDELIMITER:
	/* "Unexepected delimiter '%0' at position %1." */
	vw_errorcode = E_VW1064_CHECKPOINTDELIMITER; break;
    case X100_ERR_CHECKPOINTTABLE:
	/* "Table '%0' not found during checkpointing." */
	vw_errorcode = E_VW1065_CHECKPOINTTABLE; break;
    case X100_ERR_CHECKPOINTINSMAIN:
	/* "Table '%0' cannot be inserted during checkpointing as it is the main table." */
	vw_errorcode = E_VW1066_CHECKPOINTINSMAIN; break;
    case X100_ERR_CHECKPOINTEXISTS:
	/* "Table '%0' cannot be used for checkpointiing multiple times in one system call." */
	vw_errorcode = E_VW1067_CHECKPOINTEXISTS; break;
    case X100_ERR_TABLESTRUCT:
	/* "Table '%0' and table '%1' not equal in structure." */
	vw_errorcode = E_VW1068_TABLESTRUCT; break;
    case X100_ERR_APPEND_IDX_NOT_SUP:
	/* "Appending to non empty tables with indexes not supported." */
	vw_errorcode = E_VW1069_APPEND_IDX_NOT_SUP; break;
    case X100_ERR_UPDUNIQUECONSVIOLATION:
	/* "Duplicate unique key detected." */
	vw_errorcode = E_VW1070_UPDUNIQUECONSVIOLAT; break;
    case X100_ERR_FKUPDVIOLATION:
	/* "Cannot UPDATE because the new values do not match referenced table." */
	vw_errorcode = E_VW1071_FKUPDVIOLATION; break;
    case X100_ERR_PKUPDVIOLATION:
	/* "Cannot UPDATE because the old values are still used." */
	vw_errorcode = E_VW1072_PKUPDVIOLATION; break;
    case X100_ERR_CHECKPOINTNOSPACE:
	/* "Checkpointing cannot be executed because not enough free space in the columnspace." */
	vw_errorcode = E_VW1073_CHECKPOINTNOSPACE;
	break;
    case X100_ERR_DROP_INC_JI:
	/* "Cannot drop table with incoming join index." */
	vw_errorcode = E_VW1074_DROP_INC_JI;
	break;
    case X100_ERR_ADCOL_PDT_CONFLICT:
	/* "Cannot adddrop column for table with updates." */
	vw_errorcode = E_VW1085_ADCOL_UPD_CONFLICT; 
	break;
    case X100_ERR_DPCOL_IDX_CONFLICT:
	/* "Cannot adddrop column for table with updates." */
	vw_errorcode = E_VW1090_DPCOL_IDX_CONFLICT;
	break;
    case X100_ERR_CHECKPOINT_APPEND:
	/* "Cannot adddrop column for table with updates." */
	vw_errorcode = E_VW1087_CHECKPOINT_APPEND;
	break;
    case X100_ERR_CHECKPOINT_CONSTRAINT:
	vw_errorcode = E_VW1088_CHECKPOINT_CONSTRAINT;
	break;
    case X100_ERR_COMBINE_START_FRESH:
	vw_errorcode = E_VW1089_COMBINE_START_FRESH;
	break;
    case X100_ERR_DATE_DIV_ANSI_INTV:
	/* "Invalid interval division." */
	vw_errorcode = E_VW1075_DATE_DIV_ANSI_INTV;
	break;
    case X100_ERR_INVALID_ARG:
	/* "Invalid argument type for operation '%0'."*/
	vw_errorcode = E_VW1076_INVALID_ARG;
	break;
    case X100_ERR_UNICODE_CONVERSION:
	/* "Unicode conversion error." */
	vw_errorcode = E_VW1077_UNICODE_CONVERSION;
	break;
    case X100_ERR_NO_TYPE_CONVERSION:
	/* "No conversion defined for result type." */
	vw_errorcode = E_VW1078_NO_TYPE_CONVERSION;
	break;
    case X100_ERR_DECOVF_ERROR:
	/* "Packed decimal overflow." */
	vw_errorcode = E_VW1079_DECOVF_ERROR;
	break;
    case X100_ERR_INVALID_EXTRACT:
	/* "Invalid parameters to extract." */
	vw_errorcode = E_VW1080_INVALID_EXTRACT;
	break;
    case X100_ERR_INTOVERFLOW:
	/* "Integer overflow." */
	vw_errorcode = E_VW1081_INTOVERFLOW; break;
	break;
    case X100_ERR_NULL_TO_NONNULL:
	/* "An attempt to place a null value in a non-nullable datatype." */
	vw_errorcode = E_VW1082_NULL_TO_NONNULL; break;
	break;
    case X100_ERR_JI_PARENT_MODIFY:
	/* "Cannot modify sort key of joinindex parent."*/
	vw_errorcode = E_VW1091_JI_PARENT_MODIFY;
	break;
    case X100_ERR_BLK_VERSION:
	vw_errorcode = E_VW1092_BLK_VERSION;
	break;
    case X100_ERR_INVALIDSYSCALL_ARGS:
	vw_errorcode = E_VW1093_INVALIDSYSCALL_ARGS;
	break;
    case X100_ERR_WRITE_LOCK_OTHERS:
	vw_errorcode = E_VW1094_WRITE_LOCK_OTHERS;
	break;
    case X100_ERR_WRITE_LOCK_SELF:
	vw_errorcode = E_VW1095_WRITE_LOCK_SELF;
	break;
    case X100_ERR_WRITE_LOCK_HOLD:
	vw_errorcode = E_VW1096_WRITE_LOCK_HOLD;
	break;
    case X100_ERR_CYCLIC_FK_NOT_SUP:
	vw_errorcode = E_VW1097_CYCLIC_FK_NOT_SUP;
	break;
    case X100_ERR_CONFLICT_CREATE_DUP_NAME:
	/* "Error committing transaction: conflict creating table - table name already in use." */
	vw_errorcode = E_VW1101_CONFL_CREATE_DUP_NAME; 
	break;
    case X100_ERR_CONFLICT_DROPTBL_DDL:
	/* "Error committing transaction: conflict dropping table - table dropped or table schema has changed." */
	vw_errorcode = E_VW1102_CONFL_DROPTBL_DDL;
	break;
    case X100_ERR_CONFLICT_CREATEKEY_DDL: 
	/* "Error committing transaction: conflict creating key - table dropped or table schema has changed." */
	vw_errorcode = E_VW1103_CONFL_CREATEKEY_DDL;
	break;
    case X100_ERR_CONFLICT_DROPKEY_DDL:
	/* "Error committing transaction: conflict dropping key - table dropped or table schema has changed." */
	vw_errorcode = E_VW1104_CONFL_DROPKEY_DDL;
	break;
    case X100_ERR_CONFLICT_CREATEIDX_DDL:
	/* "Error committing transaction: conflict creating index - table dropped or table schema has changed." */
	vw_errorcode = E_VW1105_CONFL_CREATEIDX_DDL;
	break;
    case X100_ERR_CONFLICT_DROPIDX_DDL:
	/* "Error committing transaction: conflict dropping index - table dropped or table schema has changed." */
	vw_errorcode = E_VW1106_CONFL_DROPIDX_DDL;
	break;
    case X100_ERR_CONFLICT_ADDCOL_DDL:
	/* "Error committing transaction: conflict adding column - table dropped or table schema has changed." */
	vw_errorcode = E_VW1107_CONFL_ADDCOL_DDL;
	break;
    case X100_ERR_CONFLICT_DROPCOL_DDL:
	/* "Error committing transaction: conflict dropping column - table dropped or table schema has changed." */
	vw_errorcode = E_VW1108_CONFL_DROPCOL_DDL;
	break;
    case X100_ERR_CONFLICT_APPEND_DDL:
	/* "Error committing transaction: conflict appending to table - table dropped or table schema has changed." */
	vw_errorcode = E_VW1109_CONFL_APPEND_DDL;
	break;
    case X100_ERR_CONFLICT_UPDATE_DDL:
	/* "Error committing transaction: conflict updating table - table dropped or table schema has changed." */
	vw_errorcode = E_VW1110_CONFL_UPDATE_DDL;
	break;
    case X100_ERR_CONFLICT_CKT_DDL: 
	/* "Error committing transaction: conflict checkpointing - table dropped or table schema has changed." */
	vw_errorcode = E_VW1111_CONFL_CKT_DDL;
	break;
    case X100_ERR_CONFLICT_COMB_DDL:
	/* "Error committing transaction: conflict combining - table dropped or table schema has changed." */
	vw_errorcode = E_VW1112_CONFL_COMB_DDL;
	break;
    case X100_ERR_CONFLICT_DROPTBL_DML:
	/* "Error committing transaction: conflict dropping table - concurrent append or update." */
	vw_errorcode = E_VW1113_CONFL_DROPTBL_DML;
	break;
    case X100_ERR_CONFLICT_CREATEKEY_DML:
	/* "Error committing transaction: conflict creating key - concurrent append or update." */
	vw_errorcode = E_VW1114_CONFL_CREATEKEY_DML;
	break;
    case X100_ERR_CONFLICT_DROPKEY_DML:
	/* "Error committing transaction: conflict dropping key - concurrent append or update." */
	vw_errorcode = E_VW1115_CONFL_DROPKEY_DML;
	break;
    case X100_ERR_CONFLICT_CREATEIDX_DML:
	/* "Error committing transaction: conflict creating index - concurrent append or update." */
	vw_errorcode = E_VW1116_CONFL_CREATEIDX_DML;
	break;
    case X100_ERR_CONFLICT_DROPIDX_DML:
	/* "Error committing transaction: conflict dropping index - concurrent append or update." */
	vw_errorcode = E_VW1117_CONFL_DROPIDX_DML;
	break;
    case X100_ERR_CONFLICT_ADDCOL_DML:
	/* "Error committing transaction: conflict adding column - concurrent append or update." */
	vw_errorcode = E_VW1118_CONFL_ADDCOL_DML;
	break;
    case X100_ERR_CONFLICT_DROPCOL_DML:
	/* "Error committing transaction: conflict dropping column - concurrent append or update." */
	vw_errorcode = E_VW1119_CONFL_DROPCOL_DML;
	break;
    case X100_ERR_CONFLICT_APPEND_DML:
	/* "Error committing transaction: conflict appending to table - concurrent append or update." */
	vw_errorcode = E_VW1120_CONFL_APPEND_DML;
	break;
    case X100_ERR_CONFLICT_UPDATE_DML:
	/* "Error committing transaction: conflict updating table - concurrent append." */
	vw_errorcode = E_VW1121_CONFL_UPDATE_DML;
	break;
    case X100_ERR_CONFLICT_CKT_DML:
	/* "Error committing transaction: conflict checkpointing - concurrent append." */
	vw_errorcode = E_VW1122_CONFL_CKT_DML;
	break;
    case X100_ERR_CONFLICT_COMB_DML:
	/* "Error committing transaction: conflict combining - concurrent append or update." */
	vw_errorcode = E_VW1123_CONFL_COMB_DML;
	break;
    case X100_ERR_CONFLICT_CREATEFK_REFDDL:
	/* "Error committing transaction: conflict creating foreign key - referenced table dropped or table schema has changed." */
	vw_errorcode = E_VW1124_CONFL_CREATEFK_REFDDL;
	break;
    case X100_ERR_CONFLICT_CREATEJI_REFDDL:
	/* "Error committing transaction: conflict creating joinindex - referenced table dropped or table schema has changed." */
	vw_errorcode = E_VW1125_CONFL_CREATEJI_REFDDL;
	break;
    case X100_ERR_CONFLICT_UPDATE_UPDATE:
	/* "Error committing transaction: updates conflict with updates in another transaction." */
	vw_errorcode = E_VW1126_CONFL_UPDATE_UPDATE;
	break;
    case X100_ERR_CONFLICT_UNIQ:
	/* "Error committing transaction: unique constraint conflict with another transaction." */
	vw_errorcode = E_VW1127_CONFL_UNIQ;
	break;
    case X100_ERR_CONFLICT_FK:
	/* "Error committing transaction: foreign key constraint conflict with another transaction." */
	vw_errorcode = E_VW1128_CONFL_FK;
	break;
    case X100_ERR_MNYOVF_ERROR:
	/* "Exceeded the maximum money value allowed." */
	vw_errorcode = E_VW1129_MNYOVF_ERROR;
	break;
    case X100_ERR_MNYUDF_ERROR:
	/* "Exceeded the minimum money value allowed." */
	vw_errorcode = E_VW1130_MNYUDF_ERROR;
	break;
    case X100_ERR_CONFLICT_UPDATE_CKT:
	/* "Error committing transaction: updates conflict with recently committed background checkpointing process." */
	vw_errorcode = E_VW1131_CONFL_UPDATE_CKT;
	break;
    default:
	STprintf(errbuf, "0x%08x", x100err->code);
	return vwErrorFcn(session, E_VW9997_UNMAPPED_ERROR, "", 0, 1, errbuf);
    } /* switch */
#if MAX_X100_ERROR_PARAMS == 3 && MAX_X100_ERROR_PARAMS <= MAX_PARMS
    return vwErrorFcn(session, vw_errorcode, "", 0,
		x100err->param_count, 
		x100err->params[0],
		x100err->params[1],
		x100err->params[2]);
#else
#error MAX_X100_ERROR_PARAMS != 3 || MAX_X100_ERROR_PARAMS > MAX_PARMS
#endif
}

#endif /* conf_IVW */
