/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : cacheprm.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog, Sub-Page (Parameter) of DBMS Cache page
**               See the CONCEPT.DOC for more detail 
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#include "stdafx.h"
#include "vcbf.h"
#include "cacheprm.h"
#include "crightdg.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCacheParameter dialog


CuDlgDbmsCacheParameter::CuDlgDbmsCacheParameter(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDbmsCacheParameter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDbmsCacheParameter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDbmsCacheParameter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDbmsCacheParameter)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDbmsCacheParameter, CDialog)
	//{{AFX_MSG_MAP(CuDlgDbmsCacheParameter)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnButton4)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnButton5)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCacheParameter message handlers

void CuDlgDbmsCacheParameter::OnDestroy() 
{
	VCBF_GenericDestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgDbmsCacheParameter::OnInitDialog() 
{
	VCBF_CreateControlGeneric (m_ListCtrl, this, &m_ImageList, &m_ImageListCheck);
	m_ListCtrl.SetGenericForCache();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDbmsCacheParameter::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}

void CuDlgDbmsCacheParameter::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}


LRESULT CuDlgDbmsCacheParameter::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CuDlgCacheTab
	ASSERT_VALID (pParent2);
	CWnd* pParent3 = pParent2->GetParent ();    // CvDbmsCacheViewRight
	ASSERT_VALID (pParent3);
	CWnd* pParent4 = pParent3->GetParent ();    // CSplitterWnd
	ASSERT_VALID (pParent4);
	CWnd* pParent5 = pParent4->GetParent ();    // CfDbmsCacheFrame
	ASSERT_VALID (pParent5);
	CWnd* pParent6 = pParent5->GetParent ();    // CuDlgDmbsCache
	ASSERT_VALID (pParent6);
	CWnd* pParent7 = pParent6->GetParent ();    // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent8 = pParent7->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent8);


	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent8)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent8)->GetDlgItem (IDC_BUTTON2);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);
	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);
	
	try 
	{
		//
		// Do something low-level: Retrieve data and display it.
		int  iIndex    = 0;
		BOOL bContinue = TRUE;
		GENERICLINEINFO  data;
			
		m_ListCtrl.HideProperty ();
		VCBF_GenericDestroyItemData (&m_ListCtrl);
		m_ListCtrl.DeleteAllItems();
		memset (&data, 0, sizeof (data));

		bContinue = VCBFllGetFirstGenCacheLine(&data);
		while (bContinue)
		{
			VCBF_GenericAddItem (&m_ListCtrl, &data, iIndex);
			bContinue = VCBFllGetNextGenCacheLine(&data);
			iIndex++;
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgDbmsCacheParameter::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("CuDlgDbmsCacheParameter::OnUpdateData has caught exception... \n");
	}
	return 0L;
}

LRESULT CuDlgDbmsCacheParameter::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_ListCtrl.HideProperty();
	return 0L;
}

LRESULT CuDlgDbmsCacheParameter::OnButton1 (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsCacheParameter::OnButton1 (EditValue)...\n");
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 1;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_ListCtrl.EditValue (iIndex, iNameCol, rCell);
	}
	return 0;
}


LRESULT CuDlgDbmsCacheParameter::OnButton2(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsCacheParameter::OnButton2 (Restore)...\n");
	UINT nState = 0;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.HideProperty();
	GENERICLINEINFO* pData = (GENERICLINEINFO*)m_ListCtrl.GetItemData(iIndex);
	try
	{
		if (!pData)
			return 0;
		VCBFllCacheResetGenValue (pData);
		VCBF_GenericSetItem (&m_ListCtrl, pData, iIndex);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgDbmsCacheParameter::OnButton2 has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0;
}


LRESULT CuDlgDbmsCacheParameter::OnButton3(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsParameter::OnButton3...\n");
	return 0;
}

LRESULT CuDlgDbmsCacheParameter::OnButton4(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsParameter::OnButton4...\n");
	return 0;
}

LRESULT CuDlgDbmsCacheParameter::OnButton5(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsCacheParameter::OnButton5...\n");
	return 0;
}

void CuDlgDbmsCacheParameter::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Recallculate)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CuDlgCacheTab
	ASSERT_VALID (pParent2);
	CWnd* pParent3 = pParent2->GetParent ();    // CvDbmsCacheViewRight
	ASSERT_VALID (pParent3);
	CWnd* pParent4 = pParent3->GetParent ();    // CSplitterWnd
	ASSERT_VALID (pParent4);
	CWnd* pParent5 = pParent4->GetParent ();    // CfDbmsCacheFrame
	ASSERT_VALID (pParent5);
	CWnd* pParent6 = pParent5->GetParent ();    // CuDlgDmbsCache
	ASSERT_VALID (pParent6);
	CWnd* pParent7 = pParent6->GetParent ();    // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent8 = pParent7->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent8);


	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent8)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent8)->GetDlgItem (IDC_BUTTON2);

	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}
