/*
** Copyright (c) 2004 Actian Corporation
*/
# include <stdio.h>
# include <iiapi.h>
# include <sapiw.h>
# include "sapiwi.h"

/**
** Name:	apiproc.c - synchronous API wrapper execute procedure functions
**
** Description:
**	Defines
**		IIsw_execProcedure	- execute database procedure
**		IIsw_execProcDesc	- execute proc returning tuple descr
##
## History:
##	27-apr-2000 (abbjo03)
##		Created.
##	31-mar-2004 (gupsh01)
##		Pass IIAPI_GETINFOPARM as input paramter to variour
##		wrapper functions.
**/

/*{
** Name:	IIsw_execProcedure	- execute database procedure
**
** Description:
**	Executes a database procedure.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**
** Outputs:
**	tranHandle	- transaction handle
**	stmtHandle	- statement handle
**	errParm		- error paramerter block
**
** Returns:
**	Status from call to IIsw_getQueryInfo.
*/
IIAPI_STATUS
IIsw_execProcedure(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
II_LONG			*procReturn,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qinfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_STATUS		status;

	status = IIsw_queryQry(connHandle, tranHandle, IIAPI_QT_EXEC_PROCEDURE,
		NULL, TRUE, stmtHandle, qinfoParm, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);
	status = IIsw_setDescriptor(*stmtHandle, parmCount, parmDesc, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);
	status = IIsw_putParms(*stmtHandle, parmCount, parmData, NULL, NULL,
		errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);
	
	status = IIsw_getQueryInfo(*stmtHandle, qinfoParm, errParm);
	if (status == IIAPI_ST_MESSAGE)
	{
		errParm->ge_errorHandle = qinfoParm->gq_genParm.gp_errorHandle;
		while (TRUE)
		{
			IIapi_getErrorInfo(errParm);
			if (errParm->ge_status == IIAPI_ST_NO_DATA)
				break;
			printf("%s\n", errParm->ge_message);
		}
	}
	if (qinfoParm->gq_mask & IIAPI_GQ_PROCEDURE_RET && procReturn)
		*procReturn = qinfoParm->gq_procedureReturn;
	return (status);
}


/*{
** Name:	IIsw_execProcDesc	- Execute Procedure returning the Tuple Descriptor
**
** Description:
**	Initiate an Execute Procedure.  
**	Ask for the query descriptors.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**
** Outputs:
**	tranHandle	- transaction handle
**	columnCount - count of the number of columns returned in columnData
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**	errParm	- error handle
**
** Returns:
**	IIAPI_ST_SUCCESS	- successfully retrieved row data
**	anything else		- error
*/
IIAPI_STATUS
IIsw_execProcDesc(
II_PTR				connHandle,
II_PTR				*tranHandle,
II_INT2				parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
II_LONG				*columnCount,
IIAPI_DESCRIPTOR	**columnDesc,
II_PTR				*stmtHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_STATUS		status;
	IIAPI_GETDESCRPARM	gdescParm;

	*columnDesc = NULL;
	*columnCount = 0;
	status = IIsw_queryQry(connHandle, tranHandle,
			IIAPI_QT_EXEC_PROCEDURE, NULL, parmCount ? TRUE : FALSE,
			stmtHandle, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);

	if (parmCount)
	{
		status = IIsw_setDescriptor(*stmtHandle, parmCount,
			parmDesc, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
		status = IIsw_putParms(*stmtHandle, parmCount, parmData, NULL,
			NULL, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
	}

	status = IIsw_getDescriptor(*stmtHandle, &gdescParm, errParm);
	if (status == IIAPI_ST_SUCCESS)
	{	
		/* Return the column count and description array */
		*columnCount = gdescParm.gd_descriptorCount;
		*columnDesc = gdescParm.gd_descriptor;
	}

	return (status);
}
