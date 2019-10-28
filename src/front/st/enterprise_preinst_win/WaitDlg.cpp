/*
**  Copyright (c) 2001 Actian Corporation
*/

/*
**  Name: WaitDlg.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	06-Apr-2010 (drivi01)
**	    Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**	    will be unsigned int on x86 and unsigned int64 on x64.
**	    This will cleanup warnings.
**  10-Dec-2010 (drivi01)
**	    Add a call to ConvertTextToIVW() function and set the custom
**	    title on this dialog.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void ConvertTextToIVW(CDialog *dialog);

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog


CWaitDlg::CWaitDlg(UINT uiStrText, UINT uiAVI_Resource /*=IDR_CLOCK_AVI*/, CWnd* pParent /*=NULL*/)
	: CDialog(CWaitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWaitDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_uiAVI_Resource=uiAVI_Resource;
	m_strText.LoadString(uiStrText);
	m_hThreadWait=NULL;
}


void CWaitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CWaitDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	static int bReplaced = 0;

	CWnd *hAniWnd=GetDlgItem(IDC_AVI);
	if(hAniWnd)
	{
		hAniWnd->SendMessage(ACM_OPEN, 0, (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_uiAVI_Resource)));
		hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1), (LPARAM)MAKELONG(0, 0xFFFF));
	}
	
	CWnd *hTextWnd=GetDlgItem(IDC_TEXT);
	if(hTextWnd)
		hTextWnd->SetWindowText(m_strText);
	
    if(m_hThreadWait)
		SetTimer(1, 20000, 0);
	
	if (thePreInstall.m_IngresVW  && !bReplaced)
	{
		CString text;
		ConvertTextToIVW(this);
		bReplaced = 1;
		text.LoadString(IDS_TITLE_INGRESVW);
		SetWindowText(text);	
	}

	CenterWindow(GetDesktopWindow());
	BringWindowToTop();
	ShowWindow(SW_SHOW);
	RedrawWindow();
	
	return TRUE;
}

void CWaitDlg::OnTimer(UINT_PTR nIDEvent) 
{
	if(nIDEvent==1)
	{ 
		DWORD dwRet;
		
		if((m_hThreadWait) && (GetExitCodeThread(m_hThreadWait,&dwRet)))
		{ 
			if(dwRet!=STILL_ACTIVE)
				CDialog::OnCancel();
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}
