/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : brdpadlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Parameter Page of Bridge                          .                   //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/
#if !defined(AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_)
#define AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// brdpadlg.h : header file
//
#include "editlsgn.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeParameter dialog

class CuDlgBridgeParameter : public CDialog
{
public:
	CuDlgBridgeParameter(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgBridgeParameter)
	enum { IDD = IDD_BRIDGE_PAGE_PARAMETER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgBridgeParameter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlGeneric m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgBridgeParameter)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnButton1      (WPARAM wParam, LPARAM lParam);     // Edit Value	
	afx_msg LRESULT OnButton2      (WPARAM wParam, LPARAM lParam);     // Default
	afx_msg LRESULT OnButton3      (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnButton4      (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnButton5      (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnValidateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_)
