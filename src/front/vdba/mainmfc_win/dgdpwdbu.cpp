/*
**  Copyright (c) 2005-2010 Actian Corporation. All Rights Reserved.
*/

/*
**
** History:
**
** 13-Dec-2010 (drivi01)
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/
// DgDpWdbu.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwdbu.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  //#include "monitor.h"
  #include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgDomPropIceDbuser, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbuser dialog


CuDlgDomPropIceDbuser::CuDlgDomPropIceDbuser()
	: CFormView(CuDlgDomPropIceDbuser::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceDbuser)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceDbuser::~CuDlgDomPropIceDbuser()
{
}

void CuDlgDomPropIceDbuser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceDbuser)
	DDX_Control(pDX, IDC_COMMENT, m_edComment);
	DDX_Control(pDX, IDC_ALIASS, m_edAlias);
	DDX_Control(pDX, IDC_ICEUSERNAME, m_edName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceDbuser, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceDbuser)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbuser diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceDbuser::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceDbuser::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbuser message handlers

void CuDlgDomPropIceDbuser::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LRESULT CuDlgDomPropIceDbuser::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LRESULT CuDlgDomPropIceDbuser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      //case FILTER_DOM_SYSTEMOBJECTS:
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH_DETAIL:
      if (m_Data.m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
        break;    // need to update
      else
        return 0; // no need to update
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  ResetDisplay();
  // Get info on the object
  ICEDBUSERDATA IceDbuserParams;
  memset (&IceDbuserParams, 0, sizeof (IceDbuserParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get ICE Detail Info
  //
  lstrcpy ((LPTSTR)IceDbuserParams.UserAlias, (LPCTSTR)lpRecord->objName);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            OT_ICE_DBUSER,
                            &IceDbuserParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceDbuser tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csUserAlias = IceDbuserParams.UserAlias;
  m_Data.m_csComment   = IceDbuserParams.Comment;
  m_Data.m_csUserName  = IceDbuserParams.User_Name;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LRESULT CuDlgDomPropIceDbuser::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceDbuser") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceDbuser* pData = (CuDomPropDataIceDbuser*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceDbuser)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LRESULT CuDlgDomPropIceDbuser::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceDbuser* pData = new CuDomPropDataIceDbuser(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceDbuser::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edAlias.SetWindowText(m_Data.m_csUserAlias);
  m_edComment.SetWindowText(m_Data.m_csComment);
  m_edName.SetWindowText(m_Data.m_csUserName);
}

void CuDlgDomPropIceDbuser::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edAlias.SetWindowText(_T(""));
  m_edComment.SetWindowText(_T(""));
  m_edName.SetWindowText(_T(""));

  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
