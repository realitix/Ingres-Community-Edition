/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : staprdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Parameter) of Star                             //
//               See the CONCEPT.DOC for more detail                                   // 					
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/

#if !defined(STAPRDLG_HEADER)
#define STAPRDLG_HEADER

// staprdlg.h : header file
//
#include "editlsgn.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarParameter dialog

class CuDlgStarParameter : public CDialog
{
public:
	CuDlgStarParameter(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStarParameter)
	enum { IDD = IDD_STAR_PAGE_PARAMETER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStarParameter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlGeneric m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgStarParameter)
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

#endif
