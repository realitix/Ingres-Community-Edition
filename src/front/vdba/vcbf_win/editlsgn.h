/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : editlsgn.h, Header File  
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Special Owner draw List Control for the Generic Page
**               See the CONCEPT.DOC for more detail
** History:
** xx-Sep-1997 (uk$so01)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#ifndef EDITLISTCONTROLGENERIC_HEADER
#define EDITLISTCONTROLGENERIC_HEADER
#include "editlsct.h"

class CuEditableListCtrlGeneric : public CuEditableListCtrl
{
public:
	CuEditableListCtrlGeneric();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlGeneric();

	virtual LRESULT ManageEditDlgOK (WPARAM wParam, LPARAM lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual LRESULT ManageEditNumberDlgOK (WPARAM wParam, LPARAM lParam)  {return OnEditNumberDlgOK(wParam, lParam);}
	virtual LRESULT ManageEditSpinDlgOK (WPARAM wParam, LPARAM lParam)    {return OnEditSpinDlgOK(wParam, lParam);}  
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	void SetGenericForCache (BOOL bCache = TRUE){m_bCache = bCache;};
	BOOL GetGenericForCache (){return m_bCache;};
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlGeneric)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	BOOL m_bCache; // When this control is used for CACHE, the low-level call is slightly
                   // different. If this member is TRUE, then the control is used for CACHE.

	//{{AFX_MSG(CuEditableListCtrlGeneric)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditDlgCancel (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditNumberDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditNumberDlgCancel (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditSpinDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditSpinDlgCancel (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif

