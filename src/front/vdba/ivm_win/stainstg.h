/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : stainstg.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Instance (generic)
**
** History:
**
** 04-Mar-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#if !defined(AFX_STAINSTG_H__28F5FDF8_D144_11D2_A2D6_00609725DDAF__INCLUDED_)
#define AFX_STAINSTG_H__28F5FDF8_D144_11D2_A2D6_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dginstt1.h"

class CuDlgStatusInstance : public CDialog
{
public:
	CuDlgStatusInstance(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStatusInstance)
	enum { IDD = IDD_STATUS_INSTANCE };
	CTabCtrl	m_cTab1;
	BOOL	m_bUnreadCriticalEvent;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStatusInstance)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void DisplayPage();

	CuDlgStatusInstallationTab1* m_pPage1;
	CWnd* m_pCurrentPage;

	// Generated message map functions
	//{{AFX_MSG(CuDlgStatusInstance)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComponentChanged (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewEvents (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STAINSTG_H__28F5FDF8_D144_11D2_A2D6_00609725DDAF__INCLUDED_)
