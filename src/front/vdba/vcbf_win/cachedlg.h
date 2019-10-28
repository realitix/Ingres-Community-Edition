/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachedlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog-DBMS Cache page                                       //
//                                                                                     //
//    History  :                                                                       //
//      13-Dec-2010 (drivi01)                                                          //
//         Port the solution to x64. Clean up the warnings.                            //
//         Clean up datatypes. Port function calls to x64.                             //
****************************************************************************************/

#if !defined(AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
#define AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachedlg.h : header file
//
#include "cachefrm.h"	// Frame support for the splitter
#include "cachevi1.h"	// View CvDbmsCacheViewLeft
#include "cachevi2.h"	// View CvDbmsCacheViewRight


/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCache dialog

class CuDlgDbmsCache : public CDialog
{
protected:

public:
	CuDlgDbmsCache(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	CvDbmsCacheViewLeft*  GetLeftPane (); 
	CvDbmsCacheViewRight* GetRightPane();
	// Dialog Data
	//{{AFX_DATA(CuDlgDbmsCache)
	enum { IDD = IDD_DBMS_PAGE_CACHE };
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDbmsCache)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CfDbmsCacheFrame* m_pCacheFrame;

	// Generated message map functions
	//{{AFX_MSG(CuDlgDbmsCache)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnValidateData (WPARAM wParam, LPARAM lParam);
	//
	// Generic button: You must spesify the action of what you expected !!!
	afx_msg LRESULT OnButton1 (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnButton2 (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnButton3 (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
