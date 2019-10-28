/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : stadupco.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Duplicatable component branchs
**
** History:
**
** 04-Mar-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 20-Jun-2001 (noifr01)
**    (bug 105065) renamed the 2 subtabs in the "status" right pane tab
** 30-Oct-2003 (noifr01)
**    upon cleanup after massive ingres30->main propagation, added CR 
**    after traling }, which was causing problems in propagations
** 25-Jul-2007 (drivi01)
**	  Due to the fact that ivm will use winstart on Vista instead of
**    ingstart/ingstop, some of the tabs are irrelevant now.
**	  This change will remove/disable those tabs.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#include "stdafx.h"
#include "ivm.h"
#include "mgrfrm.h"
#include "stadupco.h"
#include "ivmdisp.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusDuplicatableComponent dialog


CuDlgStatusDuplicatableComponent::CuDlgStatusDuplicatableComponent(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusDuplicatableComponent::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusDuplicatableComponent)
	m_bUnreadCriticalEvent = FALSE;
	m_bStarted = FALSE;
	m_bStartedAll = FALSE;
	m_bStartedMore = FALSE;
	//}}AFX_DATA_INIT
	m_pPage1 = NULL;
	m_pPage2 = NULL;
	m_pCurrentPage = NULL;
}


void CuDlgStatusDuplicatableComponent::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusDuplicatableComponent)
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Check(pDX, IDC_CHECK1, m_bUnreadCriticalEvent);
	DDX_Check(pDX, IDC_CHECK5, m_bStarted);
	DDX_Check(pDX, IDC_CHECK6, m_bStartedAll);
	DDX_Check(pDX, IDC_CHECK7, m_bStartedMore);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusDuplicatableComponent, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusDuplicatableComponent)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED, OnComponentChanged)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusDuplicatableComponent message handlers

void CuDlgStatusDuplicatableComponent::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgStatusDuplicatableComponent::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//
	// Construct the Tab Control:
	// to be moved to the resource file - currently locked by (bigger) concurrent change
	TCHAR tchszTab [2][48] = {_T("Start/Stop Event History"), _T("Start/Stop Output Log")};

	CRect   r;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	for (int i=0; i<2; i++)
	{
		if (theApp.m_bVista && i == 1)
			break;
		item.pszText = (LPTSTR)tchszTab[i];
		m_cTab1.InsertItem (i, &item);
	}

	try
	{
		m_pPage1 = new CuDlgStatusInstallationTab1(&m_cTab1);
		m_pPage2 = new CuDlgStatusInstallationTab2(&m_cTab1);
		m_pPage1->Create (IDD_STATUS_INSTALLATION_TAB1,&m_cTab1);
		if (!theApp.m_bVista)
		m_pPage2->Create (IDD_STATUS_INSTALLATION_TAB2,&m_cTab1);
		m_pCurrentPage = m_pPage1;
		m_cTab1.SetCurSel (0);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(CuDlgStatusDuplicatableComponent::OnInitDialog()): \nFailed to create the Tab of historiy of events"));
		return FALSE;
	}

	DisplayPage();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgStatusDuplicatableComponent::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	CRect r, rDlg;
	m_cTab1.GetWindowRect (r);
	ScreenToClient (r);
	GetClientRect (rDlg);
	r.right = rDlg.right - r.left;
	r.bottom = rDlg.bottom  - r.left;
	m_cTab1.MoveWindow (r);
	DisplayPage();
}

void CuDlgStatusDuplicatableComponent::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	DisplayPage();
	*pResult = 0;
}

void CuDlgStatusDuplicatableComponent::DisplayPage()
{
	CRect r;
	int nSel = m_cTab1.GetCurSel();

	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->ShowWindow (SW_HIDE);
	switch (nSel)
	{
	case 0:
		m_pCurrentPage = m_pPage1;
		break;
	case 1:
		m_pCurrentPage = m_pPage2;
		break;
	default:
		m_pCurrentPage = m_pPage1;
		break;
	}
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->MoveWindow (r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}

LRESULT CuDlgStatusDuplicatableComponent::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	try
	{
		CaTreeComponentItemData* pItem = NULL;
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		if (pPageInfo)
			pItem = pPageInfo->GetIvmItem();
		//
		// Result from the Refresh by the Worker thread of Main application <CappIvmApplication>
		if (wParam == 0 && lParam == 0)
		{
			CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pManagerFrame);
			if (!pManagerFrame)
				return 0L;
			CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
			if (!pTreeView)
				return 0L;
			CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
			HTREEITEM hSelected = cTree.GetSelectedItem();
			if (!hSelected)
				return 0L;
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
			pPageInfo = pItem->GetPageInformation();
		}
		if (!pItem)
			return 0L;

		m_bUnreadCriticalEvent = (pItem->m_treeCtrlData.AlertGetCount() > 0)? TRUE: FALSE;
		m_bStarted = FALSE;
		m_bStartedAll = FALSE;
		m_bStartedMore = FALSE;
		int nStatus = pItem->GetItemStatus();
		switch (nStatus)
		{
		case COMP_NORMAL:
			break;
		case COMP_START:
			m_bStarted = TRUE;
			m_bStartedAll = TRUE;
			break;
		case COMP_STARTMORE:
			m_bStarted = TRUE;
			m_bStartedMore = TRUE;
			m_bStartedAll = FALSE;
			break;
		case COMP_HALFSTART:
			m_bStarted = TRUE;
			m_bStartedAll = FALSE;
			break;
		default:
			break;
		}
		UpdateData (FALSE);
		//
		// Activate the Last start/stop output:
		if (theApp.IsComponentStartStop())
		{
			m_cTab1.SetCurSel(1);
			DisplayPage();
		}
		m_pPage1->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);
		m_pPage2->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);
		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(CuDlgStatusDuplicatableComponent::OnUpdateData): \nFailed to refresh the Data"));
	}

	return 0L;
}


LRESULT CuDlgStatusDuplicatableComponent::OnComponentChanged (WPARAM wParam, LPARAM lParam)
{
	OnUpdateData (0, 0);
	return 0;
}


//
// There are new events, so refresh the pane:
LRESULT CuDlgStatusDuplicatableComponent::OnNewEvents (WPARAM wParam, LPARAM lParam)
{
	//
	// Construct the filter:
	CaTreeComponentItemData* pItem;
	CaEventFilter filter;
	CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pManagerFrame);
	if (!pManagerFrame)
		return 0;
	CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
	if (!pTreeView)
		return 0;
	CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return 0;
	pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
	CaPageInformation* pPageInfo = pItem? pItem->GetPageInformation(): NULL;

	if (pPageInfo && pItem)
	{
		OnUpdateData (0, (LPARAM)pPageInfo);
	}
	return 0L;
}

LRESULT CuDlgStatusDuplicatableComponent::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LRESULT CuDlgStatusDuplicatableComponent::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStatusDuplicatableComponent::OnFind \n");
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		return (long)m_pCurrentPage->SendMessage (WMUSRMSG_FIND, wParam, lParam);
	}

	return 0L;
}
