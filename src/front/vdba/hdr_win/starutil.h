/********************************************************************
**
**  Copyright (C) 2005-2011 Actian Corporation. All Rights Reserved.
**
**  Source : starutil.h
**
**  History:
**
**  13-Dec-2010 (drivi01)
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**
********************************************************************/

// starutil.h : interface for cpp entry points (dialogs, ...)

#ifndef _STARINTERFACE_INCLUDED
#define _STARINTERFACE_INCLUDED

#define LINK_TABLE      1
#define LINK_VIEW       2
#define LINK_PROCEDURE  3

//
// First Section : "extern "C" functions
//
#ifdef __cplusplus
extern "C"
{
#endif

#include "dbaset.h"

INT_PTR DlgRegisterTableAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcTblName, LPCSTR srcOwnerName);
INT_PTR DlgRegisterViewAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcViewName, LPCSTR srcOwnerName);
INT_PTR DlgRegisterProcAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcProcName, LPCSTR srcOwnerName);
INT_PTR DlgStarLocalTable(HWND hwnd, LPTABLEPARAMS lptbl);

#ifdef __cplusplus
}
#endif


//
// Second Section : pure c++ functions
//
#ifdef __cplusplus
BOOL IsLocalNodeName(LPCTSTR nodeName, BOOL bCanContainServerClass);
CString GetLocalHostName();
CString VerifyStarSqlError(LPCTSTR csDestNodeName, LPCTSTR csCurrentNodeName);
#endif


#endif  // _STARINTERFACE_INCLUDED
