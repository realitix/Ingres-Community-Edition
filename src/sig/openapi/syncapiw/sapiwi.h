/*
** Copyright (c) 2004 Actian Corporation
*/
# ifndef SAPIWI_H_INCLUDED
# define SAPIWI_H_INCLUDED
# include <iiapi.h>
# include <sapiw.h>

/**
** Name:	sapiwi.h - synchronous API wrapper internal functions
**
** Description:
**	Include file for internal functions to interface with the OpenIngres
**	API.
##
## History:
##	27-apr-2000 (abbjo03)
##		Created.
**/

# define E_SW0001_INTERNAL_ERROR	0xEA0001


extern II_PTR IISW_internalError;


IIAPI_STATUS IIsw_getDescriptor(II_PTR stmtHandle,
	IIAPI_GETDESCRPARM *gdescParm, IIAPI_GETEINFOPARM *errParm);
IIAPI_STATUS IIsw_putParms(II_PTR stmtHandle, II_INT2 parmCount,
	IIAPI_DATAVALUE *parmData, IISW_CALLBACK callback, II_PTR closure,
	IIAPI_GETEINFOPARM *errParm);
IIAPI_STATUS IIsw_putParmsRepeated(II_PTR stmtHandle, II_BOOL define,
	II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_DATAVALUE *parmData, SW_REPEAT_QUERY_ID *qryId, II_PTR *qryHandle,
	IIAPI_GETEINFOPARM *errParm);
IIAPI_STATUS IIsw_queryQry(II_PTR connHandle, II_PTR *tranHandle,
	IIAPI_QUERYTYPE queryType, II_CHAR *queryText, II_BOOL params,
	II_PTR *stmtHandle, IIAPI_GETEINFOPARM *errParm);
IIAPI_STATUS IIsw_setDescriptor(II_PTR stmtHandle,
	II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_GETEINFOPARM *errParm);
II_VOID IIsw_wait(IIAPI_GENPARM *genParm);

# endif
