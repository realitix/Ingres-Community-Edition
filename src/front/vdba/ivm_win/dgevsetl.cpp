/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : dgevsetl.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog of the left pane of Event Setting Frame
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgevsetl.h"
#include "fevsetti.h"
#include "wmusrmsg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingLeft dialog


CuDlgEventSettingLeft::CuDlgEventSettingLeft(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgEventSettingLeft::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgEventSettingLeft)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgEventSettingLeft::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgEventSettingLeft)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckShowCategories);
	DDX_Control(pDX, IDC_TREE1, m_cTree1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgEventSettingLeft, CDialog)
	//{{AFX_MSG_MAP(CuDlgEventSettingLeft)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckShowCategories)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnDisplayTree)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingLeft message handlers

void CuDlgEventSettingLeft::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgEventSettingLeft::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_MESSAGE_CATEGORY_SETTING, 16, 0, RGB(255,0,255));
	m_cTree1.SetImageList (&m_ImageList, TVSIL_NORMAL);

	m_cCheckShowCategories.SetCheck (1);
	
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	CaTreeMessageState& treeStateView = pFrame->GetTreeStateView();

	BOOL bShowCat = (BOOL)m_cCheckShowCategories.GetCheck();
	UINT nMode = bShowCat? SHOW_CATEGORY_BRANCH: 0;

	treeStateView.Display (&m_cTree1, NULL, nMode);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgEventSettingLeft::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cTree1.m_hWnd))
		return;
	CRect r, rDlg;
	GetClientRect (rDlg);
	m_cTree1.GetWindowRect (r);
	ScreenToClient (r);
	r.left  = rDlg.left;
	r.right = rDlg.right - 1;
	r.bottom= rDlg.bottom- 1;
	m_cTree1.MoveWindow (r);
}


void CuDlgEventSettingLeft::OnCheckShowCategories() 
{
	OnDisplayTree (1, 0);
}

//
// If w = 1 then do not notify the right tree:
LRESULT CuDlgEventSettingLeft::OnDisplayTree (WPARAM w, LPARAM l)
{
	BOOL bShow = (BOOL)m_cCheckShowCategories.GetCheck();
	UINT nMode = bShow? SHOW_CATEGORY_BRANCH: 0;
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	CaTreeMessageState& treeStateView = pFrame->GetTreeStateView();
	CaTreeMessageCategory& treeCategoryView = pFrame->GetTreeCategoryView();

	treeStateView.UpdateTree (&m_cTree1, &treeCategoryView);

	treeStateView.Display (&m_cTree1, NULL, nMode);
	treeStateView.SortItems (&m_cTree1);
	m_cTree1.Invalidate();

	//
	// Notify the right tree to re-display:
	if ((int)w == 1)
		return 0;
	CvEventSettingRight* RightView = pFrame->GetRightView();
	if (RightView)
	{
		CuDlgEventSettingRight* pDlg = RightView->GetDialog();
		if (pDlg)
			pDlg->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)1, l);
	}

	return 0;
}
