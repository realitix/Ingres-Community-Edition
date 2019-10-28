// edlduslo.h: interface for the CuEditableListCtrlDuplicateDbSelectLocation class.
//
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "editlsct.h"

class CuEditableListCtrlDuplicateDbSelectLocation : public CuEditableListCtrl
{
public:
	enum {INITIAL_LOC, NEW_LOC};
	CuEditableListCtrlDuplicateDbSelectLocation();
	virtual ~CuEditableListCtrlDuplicateDbSelectLocation();

	LRESULT ManageComboDlgOK      (WPARAM wParam, LPARAM lParam) {return OnComboDlgOK(wParam, lParam);}

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	//void UpdateDisplay (int iItem);

protected:
	void InitSortComboBox();
	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlDuplicateDbSelectLocation)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	//afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComboDlgOK (WPARAM wParam, LPARAM lParam);
	//afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

};

#endif // !defined(AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
