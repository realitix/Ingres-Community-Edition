/*
**
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : edlsinpr.h, Header File
**
**
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Editable List control to provide to Change the parameter of
**               Installation Branch (System and User)
**
**  History:
**	17-Dec-1998 (uk$so01)
**	    Created.
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(EDLS_PARAMETER_INSTALLATION_HEADER)
#define EDLS_PARAMETER_INSTALLATION_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlInstallationParameter window

class CuEditableListCtrlInstallationParameter : public CuEditableListCtrl
{
public:
	enum {C_PARAMETER, C_VALUE, C_DESCRIPTION};
	CuEditableListCtrlInstallationParameter();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlInstallationParameter)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LRESULT ManageEditDlgOK (WPARAM wParam, LPARAM lParam) { return OnEditDlgOK(wParam, lParam);}
	virtual LRESULT ManageEditNumberDlgOK (WPARAM wParam, LPARAM lParam){ return OnEditNumberDlgOK(wParam, lParam);}
	virtual LRESULT ManageEditSpinDlgOK (WPARAM wParam, LPARAM lParam){return OnEditSpinDlgOK(wParam, lParam);}
	virtual LRESULT ManageComboDlgOK (WPARAM wParam, LPARAM lParam){return OnComboDlgOK(wParam, lParam);}

	void DisplayParameter(int nItem = -1);
	void SetAllowEditing(BOOL bAllow = TRUE){m_bAllowEditing = bAllow;}
	BOOL IsAllowEditing(){return m_bAllowEditing;}

	//
	// Implementation
public:
	virtual ~CuEditableListCtrlInstallationParameter();

	//
	// Generated message map functions
protected:
	BOOL m_bAllowEditing;

	//{{AFX_MSG(CuEditableListCtrlInstallationParameter)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditAssistant (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditNumberDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditNumberDlgCancel (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditSpinDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditSpinDlgCancel (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComboDlgOK (WPARAM wParam, LPARAM lParam);
	
	
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif 
