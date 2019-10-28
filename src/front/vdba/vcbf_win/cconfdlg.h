/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : configdlg.h, header file                                              //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Special Page of CMainTabDlg (Configure)                               //
//               See the CONCEPT.DOC for more detail                                   //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/

// CConfigDlg dialog
#ifndef CCONFDLG_HEADER
#define CCONFDLG_HEADER

#include "conffrm.h"

class CConfigDlg : public CDialog
{
// Construction
public:
	CConfigFrame * m_pConfDlgFrame;
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;};
	void OnCancel() {return;};
	UINT GetHelpID();

	// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LRESULT OnComponentExit  (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComponentEnter (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateData     (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnValidateData   (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif//#ifndef CCONFDLG_HEADER

