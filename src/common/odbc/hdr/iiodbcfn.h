/* 
** Copyright (c) 2004, 2009 Actian Corporation 
*/ 
/** 
** Name: iiodbcfn.h - ODBC CLI function pointer definitions
** 
** Description: 
**   This file contains the function pointer definitions for the ODBC
**   CLI.
** 
** History:
**   15-Jul-2005 (hanje04)
** 	Created for Mac OSX port to keep GLOBALDEF's unique because the
**	mac linker doesn't allow multiple symbol defns.
**   16-June-2006 (Ralph Loen) SIR 116260
**      Added IIDescribeParam().
**   03-Jul-2006 (Ralph Loen) SIR 116326
**      Added IIBrowseConnectW().
**   14-Jul-2006 (Ralph Loen) SIR 116385
**      Added IITablePriviegesW().
**   21-jul-06 (loera01)  SIR 116410
**      Added IIColumnPrivilegesW().
**   21-jul-06 (Ralph Loen)  SIR 116410
**      Add for IIColumnPrivilegesW().
**   30-Jan-2008 (Ralph Loen) SIR 119723
**          Added IISetPos().
**   23-Mar-2009 (Ralph Loen) Bug 121838
**     Added IIGetFunctions().
*/

/*
**  Ingres ODBC driver function entry points.
*/

GLOBALREF IIODBC_DRIVER_FN IIAllocConnect;
GLOBALREF IIODBC_DRIVER_FN IIAllocEnv;
GLOBALREF IIODBC_DRIVER_FN IIAllocStmt;
GLOBALREF IIODBC_DRIVER_FN IIBindCol;
GLOBALREF IIODBC_DRIVER_FN IICancel;
GLOBALREF IIODBC_DRIVER_FN IIColAttribute;
GLOBALREF IIODBC_DRIVER_FN IIConnect;
GLOBALREF IIODBC_DRIVER_FN IIDescribeCol;
GLOBALREF IIODBC_DRIVER_FN IIDisconnect;
GLOBALREF IIODBC_DRIVER_FN IIError;
GLOBALREF IIODBC_DRIVER_FN IIExecDirect;
GLOBALREF IIODBC_DRIVER_FN IIExecute;
GLOBALREF IIODBC_DRIVER_FN IIFetch;
GLOBALREF IIODBC_DRIVER_FN IIFreeConnect;
GLOBALREF IIODBC_DRIVER_FN IIFreeEnv;
GLOBALREF IIODBC_DRIVER_FN IIFreeStmt;
GLOBALREF IIODBC_DRIVER_FN IIGetCursorName;
GLOBALREF IIODBC_DRIVER_FN IINumResultCols;
GLOBALREF IIODBC_DRIVER_FN IIPrepare;
GLOBALREF IIODBC_DRIVER_FN IIRowCount;
GLOBALREF IIODBC_DRIVER_FN IISetCursorName;
GLOBALREF IIODBC_DRIVER_FN IISetParam;
GLOBALREF IIODBC_DRIVER_FN IITransact;
GLOBALREF IIODBC_DRIVER_FN IIColAttributes;
GLOBALREF IIODBC_DRIVER_FN IITablePrivileges;
GLOBALREF IIODBC_DRIVER_FN IIBindParam;
GLOBALREF IIODBC_DRIVER_FN IIColumns;
GLOBALREF IIODBC_DRIVER_FN IIDriverConnect;
GLOBALREF IIODBC_DRIVER_FN IIGetConnectOption;
GLOBALREF IIODBC_DRIVER_FN IIGetData;
GLOBALREF IIODBC_DRIVER_FN IIGetInfo;
GLOBALREF IIODBC_DRIVER_FN IIGetStmtOption;
GLOBALREF IIODBC_DRIVER_FN IIGetTypeInfo;
GLOBALREF IIODBC_DRIVER_FN IIParamData;
GLOBALREF IIODBC_DRIVER_FN IIPutData;
GLOBALREF IIODBC_DRIVER_FN IISetConnectOption;
GLOBALREF IIODBC_DRIVER_FN IISetStmtOption;
GLOBALREF IIODBC_DRIVER_FN IISpecialColumns;
GLOBALREF IIODBC_DRIVER_FN IIStatistics;
GLOBALREF IIODBC_DRIVER_FN IITables;
GLOBALREF IIODBC_DRIVER_FN IIBrowseConnect;
GLOBALREF IIODBC_DRIVER_FN IIColumnPrivileges;
GLOBALREF IIODBC_DRIVER_FN IIDescribeParam;
GLOBALREF IIODBC_DRIVER_FN IIExtendedFetch;
GLOBALREF IIODBC_DRIVER_FN IIForeignKeys;
GLOBALREF IIODBC_DRIVER_FN IIMoreResults;
GLOBALREF IIODBC_DRIVER_FN IINativeSql;
GLOBALREF IIODBC_DRIVER_FN IINumParams;
GLOBALREF IIODBC_DRIVER_FN IIParamOptions;
GLOBALREF IIODBC_DRIVER_FN IIPrimaryKeys;
GLOBALREF IIODBC_DRIVER_FN IIProcedureColumns;
GLOBALREF IIODBC_DRIVER_FN IIProcedures;
GLOBALREF IIODBC_DRIVER_FN IISetPos;
GLOBALREF IIODBC_DRIVER_FN IISetScrollOptions;
GLOBALREF IIODBC_DRIVER_FN IIBindParameter;
GLOBALREF IIODBC_DRIVER_FN IIAllocHandle;
GLOBALREF IIODBC_DRIVER_FN IICloseCursor;
GLOBALREF IIODBC_DRIVER_FN IICopyDesc;
GLOBALREF IIODBC_DRIVER_FN IIEndTran;
GLOBALREF IIODBC_DRIVER_FN IIFetchScroll;
GLOBALREF IIODBC_DRIVER_FN IIFreeHandle;
GLOBALREF IIODBC_DRIVER_FN IIGetConnectAttr;
GLOBALREF IIODBC_DRIVER_FN IIGetDescField;
GLOBALREF IIODBC_DRIVER_FN IIGetDescRec;
GLOBALREF IIODBC_DRIVER_FN IIGetDiagField;
GLOBALREF IIODBC_DRIVER_FN IIGetDiagRec;
GLOBALREF IIODBC_DRIVER_FN IIGetEnvAttr;
GLOBALREF IIODBC_DRIVER_FN IIGetStmtAttr;
GLOBALREF IIODBC_DRIVER_FN IISetConnectAttr;
GLOBALREF IIODBC_DRIVER_FN IISetDescField;
GLOBALREF IIODBC_DRIVER_FN IISetDescRec;
GLOBALREF IIODBC_DRIVER_FN IISetEnvAttr;
GLOBALREF IIODBC_DRIVER_FN IISetStmtAttr;
GLOBALREF IIODBC_DRIVER_FN IIColAttributeW;
GLOBALREF IIODBC_DRIVER_FN IIColAttributesW;
GLOBALREF IIODBC_DRIVER_FN IIBrowseConnectW;
GLOBALREF IIODBC_DRIVER_FN IIConnectW;
GLOBALREF IIODBC_DRIVER_FN IIDescribeColW;
GLOBALREF IIODBC_DRIVER_FN IIErrorW;
GLOBALREF IIODBC_DRIVER_FN IIGetDiagRecW;
GLOBALREF IIODBC_DRIVER_FN IIGetDiagFieldW;
GLOBALREF IIODBC_DRIVER_FN IIExecDirectW;
GLOBALREF IIODBC_DRIVER_FN IIGetStmtAttrW;
GLOBALREF IIODBC_DRIVER_FN IIDriverConnectW;
GLOBALREF IIODBC_DRIVER_FN IISetConnectAttrW;
GLOBALREF IIODBC_DRIVER_FN IIGetCursorNameW;
GLOBALREF IIODBC_DRIVER_FN IIPrepareW;
GLOBALREF IIODBC_DRIVER_FN IISetStmtAttrW;
GLOBALREF IIODBC_DRIVER_FN IISetCursorNameW;
GLOBALREF IIODBC_DRIVER_FN IIGetConnectOptionW;
GLOBALREF IIODBC_DRIVER_FN IIGetInfoW;
GLOBALREF IIODBC_DRIVER_FN IIColumnsW;
GLOBALREF IIODBC_DRIVER_FN IIGetTypeInfoW;
GLOBALREF IIODBC_DRIVER_FN IISetConnectOptionW;
GLOBALREF IIODBC_DRIVER_FN IISpecialColumnsW;
GLOBALREF IIODBC_DRIVER_FN IIStatisticsW;
GLOBALREF IIODBC_DRIVER_FN IITablesW;
GLOBALREF IIODBC_DRIVER_FN IIForeignKeysW;
GLOBALREF IIODBC_DRIVER_FN IINativeSqlW;
GLOBALREF IIODBC_DRIVER_FN IIPrimaryKeysW;
GLOBALREF IIODBC_DRIVER_FN IIProcedureColumnsW;
GLOBALREF IIODBC_DRIVER_FN IIProceduresW;
GLOBALREF IIODBC_DRIVER_FN IISetDescFieldW;
GLOBALREF IIODBC_DRIVER_FN IIGetDescFieldW;
GLOBALREF IIODBC_DRIVER_FN IIGetDescRecW;
GLOBALREF IIODBC_DRIVER_FN IIGetConnectAttrW;
GLOBALREF IIODBC_DRIVER_FN IIGetPrivateProfileString;
GLOBALREF IIODBC_DRIVER_FN IITraceInit;
GLOBALREF IIODBC_DRIVER_FN IIDescribeParam;
GLOBALREF IIODBC_DRIVER_FN IITablePrivilegesW;
GLOBALREF IIODBC_DRIVER_FN IIColumnPrivilegesW;
GLOBALREF IIODBC_DRIVER_FN IISetPos;
GLOBALREF IIODBC_DRIVER_FN IIGetFunctions;
