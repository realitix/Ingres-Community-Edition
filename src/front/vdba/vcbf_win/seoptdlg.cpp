/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : seoptdlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog, Page (Options) of Security
**               See the CONCEPT.DOC for more detail
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 16-Mar-2005 (shaha03)
**	  BUG #114089, Display the confirmation check for any tab switch in security, 
**	  if it is edited.Removed security type check.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#include "stdafx.h"
#include "vcbf.h"
#include "seoptdlg.h"
#include "crightdg.h"
#include "wmusrmsg.h"

#include "cbfitem.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityOption dialog

CuDlgSecurityOption::CuDlgSecurityOption(CWnd* pParent)
	: CDialog(CuDlgSecurityOption::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSecurityOption)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgSecurityOption::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSecurityOption)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSecurityOption, CDialog)
	//{{AFX_MSG_MAP(CuDlgSecurityOption)
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
END_MESSAGE_MAP()



LRESULT CuDlgSecurityOption::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	//
	// First of all:
	// manage potential change between left group and right group of property pages
	//
  // Fix Oct 27, 97: lParam is NULL when OnUpdateData() called
  // after activation of "Preferences" tag followed by activation of "Configure" tag
  if (lParam) {
  		CuSECUREItemData *pSecureData = (CuSECUREItemData*)lParam;
  		// terminate old page family
  		pSecureData->LowLevelDeactivationWithCheck();
  		// set page family
  		pSecureData->SetSpecialType(GEN_FORM_SECURE_SECURE);
  		// Initiate new page family
  		BOOL bOk = pSecureData->LowLevelActivationWithCheck();
  		ASSERT (bOk);
  }
	

	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);

	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);

	VCBF_GenericAddItems (&m_ListCtrl);
	return 0L;
}


/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityOption message handlers

void CuDlgSecurityOption::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


void CuDlgSecurityOption::OnDestroy() 
{
	VCBF_GenericDestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}


void CuDlgSecurityOption::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}


BOOL CuDlgSecurityOption::OnInitDialog() 
{
	VCBF_CreateControlGeneric (m_ListCtrl, this, &m_ImageList, &m_ImageListCheck);
	return TRUE;
}



LRESULT CuDlgSecurityOption::OnButton1 (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgSecurityOption::OnButton1 (EditValue)...\n");
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


LRESULT CuDlgSecurityOption::OnButton2(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgSecurityOption::OnButton2.(Restore)..\n");
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
	m_ListCtrl.HideProperty();
	VCBF_GenericResetValue (m_ListCtrl,iIndex);
	return 0;
}


LRESULT CuDlgSecurityOption::OnButton3(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgSecurityOption::OnButton3...\n");
	return 0;
}

LRESULT CuDlgSecurityOption::OnButton4(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgSecurityOption::OnButton4...\n");
	return 0;
}

LRESULT CuDlgSecurityOption::OnButton5(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgSecurityOption::OnButton5...\n");
	return 0;
}


void CuDlgSecurityOption::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Restore)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}
