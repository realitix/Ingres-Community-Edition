/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : selogdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               Blatte Emmanuel (Custom Implemenations)                               //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Log File) of Security                          //
//               See the CONCEPT.DOC for more detail  
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //                                 //
****************************************************************************************/
#if !defined(SELOGDLG_HEADER)
#define SELOGDLG_HEADER

// selogdlg.h : header file
//
#include "editlsct.h"


class CuEditableListCtrlAuditLog : public CuEditableListCtrl
{
public:
	CuEditableListCtrlAuditLog();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlAuditLog();
	virtual LRESULT ManageEditDlgOK (WPARAM wParam, LPARAM lParam)        {return OnEditDlgOK(wParam, lParam);}      

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlAuditLog)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlAuditLog)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnEditDlgOK (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};









/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityLogFile dialog

class CuDlgSecurityLogFile : public CDialog
{
public:
	CuDlgSecurityLogFile(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSecurityLogFile)
	enum { IDD = IDD_SECURITY_PAGE_LOGFILE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSecurityLogFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	CuEditableListCtrlAuditLog m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgSecurityLogFile)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnButton1   (WPARAM wParam, LPARAM lParam);     // Not Used	
	afx_msg LRESULT OnButton2   (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnButton3   (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnButton4   (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnButton5   (WPARAM wParam, LPARAM lParam);     // Not Used
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnValidateData (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif
