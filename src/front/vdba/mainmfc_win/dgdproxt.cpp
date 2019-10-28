/*
**  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpRoXT.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_GRANTEDTBL into two statements to avoid compiler
**	    error.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
** 10-Aug-2011 (frima01) Bug 125297
**   Pass on OT_ROLE to DOMGetFirstObject via bufComplim. The function
**   can then avoid confusion with tables granted to non-existing 
**   users or groups of the same name as the role in question.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdproxt.h"
#include "vtree.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int aGrantedType[NBTBLGRANTED] = {
                                         OTR_GRANTEE_SEL_TABLE,
                                         OTR_GRANTEE_INS_TABLE,
                                         OTR_GRANTEE_UPD_TABLE,
                                         OTR_GRANTEE_DEL_TABLE,
                                         OTR_GRANTEE_REF_TABLE,
                                         OTR_GRANTEE_CPI_TABLE,
                                         OTR_GRANTEE_CPF_TABLE,
                                        };
#define LAYOUT_NUMBER   ( NBTBLGRANTED + 3 )

CuDlgDomPropRoleXGrantedTbl::CuDlgDomPropRoleXGrantedTbl(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropRoleXGrantedTbl::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropRoleXGrantedTbl)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropRoleXGrantedTbl::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropRoleXGrantedTbl)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropRoleXGrantedTbl::AddRoleXGrantedTbl (CuGrantedTbl* pGrantedTbl)
{
  ASSERT (pGrantedTbl);
  BOOL  bSpecial = pGrantedTbl->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name
  index  = m_cListCtrl.InsertItem (nCount, pGrantedTbl->GetStrName(), 0);

  // Owner - parent Db
  m_cListCtrl.SetItemText (index, 1, (LPCSTR)pGrantedTbl->GetOwnerName());
  m_cListCtrl.SetItemText (index, 2, (LPCSTR)pGrantedTbl->GetParentName());

  for (int cpt = 0; cpt < NBTBLGRANTED; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+3, _T(""));
    else {
      if (pGrantedTbl->GetFlag(aGrantedType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+3, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+3, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on granted object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD_PTR)pGrantedTbl);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropRoleXGrantedTbl, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropRoleXGrantedTbl)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRoleXGrantedTbl message handlers

void CuDlgDomPropRoleXGrantedTbl::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropRoleXGrantedTbl::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_TABLE);
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T(""),           FALSE, 80 + 16+4 },   // 16+4 for image
    { _T(""),           FALSE, 80 },
    { _T(""),           FALSE, 80 },
    { _T("Select"),     TRUE,  -1 },
    { _T("Insert"),     TRUE,  -1 },
    { _T("Update"),     TRUE,  -1 },
    { _T("Delete"),     TRUE,  -1 },
    { _T("Reference"),  TRUE,  -1 },
    { _T("Copy Into"),  TRUE,  -1 },
    { _T("Copy From"),  TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_OWNER));//_T("Owner")
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_PARENT_DB));
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropRoleXGrantedTbl::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LRESULT CuDlgDomPropRoleXGrantedTbl::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LRESULT CuDlgDomPropRoleXGrantedTbl::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  // cast received parameters
  int nNodeHandle = (int)wParam;
  LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
  ASSERT (nNodeHandle != -1);
  ASSERT (pUps);

  // ignore selected actions on filters
  switch (pUps->nIpmHint)
  {
    case 0:
      case FILTER_DOM_SYSTEMOBJECTS:  // can be system objects (replicator dd_xyz)
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_TABLE)
        return 0L;
      break;
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListCtrl);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();

  //
  // Get list of granted objects
  //
  m_Data.m_uaRoleXGrantedTbl.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  storeint(bufComplim,OT_ROLE);

  // Xref return code
  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBTBLGRANTED; index++) {
    iret =  DOMGetFirstRelObject(nNodeHandle,
                                 aGrantedType[index],
                                 1,                           // level,
                                 aparentsTemp,                // Temp mandatory!
                                 pUps->pSFilter->bWithSystem, // bwithsystem
                                 NULL,                        // base owner
                                 NULL,                        // other owner
                                 aparentsResult,
                                 buf,
                                 bufOwner,
                                 bufComplim);
     if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
       bError = TRUE;
       continue;
    }
    else {
      while (iret == RES_SUCCESS) {
        CuGrantedTbl grantedTbl((const char *)buf,                // obj name
                              (const char *)bufOwner,           // obj owner
                              (const char *)aparentsResult[0],  // obj parent
                              aGrantedType[index]               // grant type
                              );
        CuMultFlag *pRefGrantedTbl = m_Data.m_uaRoleXGrantedTbl.Find(&grantedTbl);
        if (pRefGrantedTbl)
          m_Data.m_uaRoleXGrantedTbl.Merge(pRefGrantedTbl, &grantedTbl);
        else
          m_Data.m_uaRoleXGrantedTbl.Add(grantedTbl);

        iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuGrantedTbl grantedTbl1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaRoleXGrantedTbl.Add(grantedTbl1);
  }

  // Manage no granted object
  if (m_Data.m_uaRoleXGrantedTbl.GetCount() == 0)
  {
    /* "<No GrantedTbl>" */
    CuGrantedTbl grantedTbl12(VDBA_MfcResourceString (IDS_E_NO_GRANTEDTBL));
    m_Data.m_uaRoleXGrantedTbl.Add(grantedTbl12);
  }

  ASSERT (m_Data.m_uaRoleXGrantedTbl.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LRESULT CuDlgDomPropRoleXGrantedTbl::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataRoleXGrantedTbl") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataRoleXGrantedTbl* pData = (CuDomPropDataRoleXGrantedTbl*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataRoleXGrantedTbl)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LRESULT CuDlgDomPropRoleXGrantedTbl::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataRoleXGrantedTbl* pData = new CuDomPropDataRoleXGrantedTbl(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropRoleXGrantedTbl::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = (int)m_Data.m_uaRoleXGrantedTbl.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuGrantedTbl* pGrantedTbl = m_Data.m_uaRoleXGrantedTbl[cnt];
    AddRoleXGrantedTbl(pGrantedTbl);
  }
  // No Initial sort
}

void CuDlgDomPropRoleXGrantedTbl::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuGranted::CompareNames, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuGranted::CompareOwners, 0L);
      break;
    case 2:
      m_cListCtrl.SortItems(CuGranted::CompareParents, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 3);
      break;
  }
  *pResult = 0;
}


void CuDlgDomPropRoleXGrantedTbl::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
