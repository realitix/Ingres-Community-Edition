/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : Editlsco.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenInges Configuration Manager                                       //
//    Author   : Emmanuel Blattes - UK Sotheavut                                       //
//                                                                                     //
//                                                                                     //
//    Purpose  : list control customized for components list in cbf                    //
//               derived from CuEditableListCtrl                                       //
//                                                                                     //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/

//
// CuEditableListCtrlComponent window
#ifndef LYOUTCTL_COMPONENT_HEADER
#define LYOUTCTL_COMPONENT_HEADER

#include "editlsct.h" // base class

class CuEditableListCtrlComponent : public CuEditableListCtrl
{
private:
  void OnOpenCellEditor(UINT nFlags, CPoint point);

public:
	CuEditableListCtrlComponent();
	
  // virtual interface to afx_msg OnXyzDlgOK methods
  virtual LRESULT ManageEditDlgOK (WPARAM wParam, LPARAM lParam);
	virtual LRESULT ManageEditSpinDlgOK (WPARAM wParam, LPARAM lParam);

	// Overrides
	//
	virtual ~CuEditableListCtrlComponent();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlComponent)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:

	//{{AFX_MSG(CuEditableListCtrlComponent)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditNumberDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditSpinDlgOK (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComboDlgOK (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif

