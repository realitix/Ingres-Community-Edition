/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : secdlgsy.h : header file
**    Project  : Configuration Manager 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog, Page (System) of Security
**
** History:
**
** 20-jun-2003 (uk$so01)
**    created.
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(AFX_SECDLGSY_H__17EC0061_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
#define AFX_SECDLGSY_H__17EC0061_94D9_11D7_8839_00C04F1F754A__INCLUDED_
#include "editlsgn.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuDlgSecuritySystem : public CDialog
{
public:
	CuDlgSecuritySystem(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSecuritySystem)
	enum { IDD = IDD_SECURITY_PAGE_SYSTEM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSecuritySystem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageListCheck;
	CImageList m_ImageList;
	CuEditableListCtrlGeneric m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgSecuritySystem)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnButton1   (WPARAM wParam, LPARAM lParam);     // Edit Value	
	afx_msg LRESULT OnButton2   (WPARAM wParam, LPARAM lParam);     // Default
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECDLGSY_H__17EC0061_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
