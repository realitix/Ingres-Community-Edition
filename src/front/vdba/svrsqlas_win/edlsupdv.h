/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : edlsupdv.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 3)
**               Editable List control to provide the update value
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(EDLSUPDV_HEADER)
#define EDLSUPDV_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"


class CuEditableListCtrlSqlWizardUpdateValue : public CuEditableListCtrl
{
public:
	enum {COLUMN, VALUE};
	CuEditableListCtrlSqlWizardUpdateValue();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlSqlWizardUpdateValue)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LRESULT ManageEditDlgOK       (WPARAM wParam, LPARAM lParam) {return OnEditDlgOK(wParam, lParam);}      
	//
	// Implementation
public:
	CaLLQueryInfo m_queryInfo;
	virtual ~CuEditableListCtrlSqlWizardUpdateValue();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlSqlWizardUpdateValue)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditAssistant (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(EDLSUPDV_HEADER)
