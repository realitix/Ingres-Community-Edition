/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : editlsgn.h, Header File
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Special Owner draw List Control(Editable)
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#ifndef EDITLISTCONTROLSTARTUPSETTING_HEADER
#define EDITLISTCONTROLSTARTUPSETTING_HEADER
#include "editlsct.h"


class CuEditableListCtrlStartupSetting : public CuEditableListCtrl
{
public:
	CuEditableListCtrlStartupSetting();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlStartupSetting();

	virtual LRESULT ManageEditDlgOK (WPARAM wParam, LPARAM lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual LRESULT ManageEditNumberDlgOK (WPARAM wParam, LPARAM lParam)  {return OnEditNumberDlgOK(wParam, lParam);}
	virtual LRESULT ManageEditSpinDlgOK (WPARAM wParam, LPARAM lParam)    {return OnEditSpinDlgOK(wParam, lParam);}  
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlStartupSetting)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:

	//{{AFX_MSG(CuEditableListCtrlStartupSetting)
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

