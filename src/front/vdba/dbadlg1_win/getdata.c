/********************************************************************
//
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : getdata.c
//
//    TOOLS
//
//    History:
//      4-Nov-2010 (drivi01)
//        Port the solution to x64. Clean up the warnings.
//        Clean up datatypes. Port function calls to x64.
********************************************************************/

#include "dba.h"
#include "domdata.h"
#include "main.h"       // for hwndMDIClient
#include "dom.h"

// from main.c, for MAINLIB purposes
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);


// Modified Emb 25/4/95 to accept any document type
int GetMDINodeHandle (HWND hwndMDI)
{
  INT_PTR             docType;
  LPDOMDATA       lpDomData;
  //LPSQLACTDATA    lpSqlActData;
  //LPPROPERTYDATA  lpPropertyData;
  
  docType = QueryDocType(hwndMDI);
  switch (docType) {
    case TYPEDOC_DOM:
      lpDomData = (LPDOMDATA)GetWindowLongPtr (GetVdbaDocumentHandle(hwndMDI), 0);
      if (lpDomData)
          return lpDomData->psDomNode->nodeHandle;
      break;
    case TYPEDOC_SQLACT:
      /* old code, replaced since full mfc with richedit
      lpSqlActData = (LPSQLACTDATA)GetWindowLongPtr (GetVdbaDocumentHandle(hwndMDI), 0);
      if (lpSqlActData)
          return lpSqlActData->nodeHandle;
      */
      // replacement code
      return (int)SendMessage(hwndMDI, WM_USER_GETNODEHANDLE, 0, 0);
      break;
    default:
      return (int)SendMessage(hwndMDI, WM_USER_GETNODEHANDLE, 0, 0);

  }

  return -1; // error!
}

int GetCurMdiNodeHandle ()
{
   // Does not work at WM_INITDIALOG time : HWND hwndParent = GetParent(hwnd);

   HWND hwndParent;

   hwndParent = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   // corrected Emb 30/06/95 to accept any type of mdi doc
   return GetMDINodeHandle (hwndParent);
}


LPDOMDATA GetCurLpDomData ()
{
   // Does not work at WM_INITDIALOG time : HWND hwndParent = GetParent(hwnd);

   HWND hwndParent;
   LPDOMDATA lpDomData;

   hwndParent = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   // corrected emb to return null pointer if current doc is not a dom
   if (QueryDocType(hwndParent) != TYPEDOC_DOM)
      return 0;

   lpDomData = (LPDOMDATA)GetWindowLongPtr (GetVdbaDocumentHandle(hwndParent), 0);
   return lpDomData;
}


BOOL GetMDISystemFlag (HWND hwndMDI)
{
   LPDOMDATA lpDomData;

   // corrected emb to return TRUE if current doc is not a dom
   if (QueryDocType(hwndMDI) != TYPEDOC_DOM)
      return TRUE;

   lpDomData = (LPDOMDATA)GetWindowLongPtr (GetVdbaDocumentHandle(hwndMDI), 0);
   if (lpDomData)
       return lpDomData->bwithsystem;
   return TRUE;
}

BOOL GetSystemFlag ()
{
   // Does not work at WM_INITDIALOG time : HWND hwndParent = GetParent(hwnd);
   HWND hwndParent;
   LPDOMDATA lpDomData;

   hwndParent = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   // corrected emb to return default TRUE if current doc is not a dom
   if (QueryDocType(hwndParent) != TYPEDOC_DOM)
      return TRUE;

   lpDomData = (LPDOMDATA)GetWindowLongPtr (GetVdbaDocumentHandle(hwndParent), 0);
   if (lpDomData)
       return lpDomData->bwithsystem;
   return TRUE;
}


/*
//_____________________________________________________________________________
//                                                                             &
//                                                                             &
// Return object type as :                                                     &
//                                                                             &
// OT_USER, OT_GROUP, OT_ROLE, OT_PUBLIC, OT_UNKNOWN                           &
//                                                                             &
//_____________________________________________________________________________&
*/
int DBA_GetObjectType (LPUCHAR objectname, LPDOMDATA lpDomData)
{
   int     resu;
   int     resulevel;
   int     myType;
   UCHAR   buf  [MAXOBJECTNAME];
   UCHAR   buf2 [MAXOBJECTNAME];
   UCHAR   buf3 [MAXOBJECTNAME];
   /*
   if (x_stricmp ((const char*)objectname, (const char*)lppublicsysstring())  == 0)
       return OT_PUBLIC;
   */
   if (x_stricmp ((const char*)objectname, (const char*)lppublicdispstring()) == 0)
       return OT_PUBLIC;

   resu = DOMGetObject(lpDomData->psDomNode->nodeHandle,
           objectname,          // objectname
           "",                  // objectowner
           OT_GRANTEE,
           0,                   // level
           (void *)0,           // parentstrings
           GetSystemFlag (),
           &myType, &resulevel, (void *)0,
           buf, buf2, buf3);

   if (resu != RES_SUCCESS)
       return OT_UNKNOWN;
   return myType;
}






