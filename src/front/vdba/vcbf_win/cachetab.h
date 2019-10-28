/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachetab.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog of the right pane of DBMS Cache                       //
//               It holds thow pages (parameter, derived)                              //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/
#if !defined(AFX_CACHETAB_H__6722F082_3B35_11D1_A201_00609725DDAF__INCLUDED_)
#define AFX_CACHETAB_H__6722F082_3B35_11D1_A201_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachetab.h : header file
//
#include "cacheprm.h"	// Page of Cache (Generic Parameter)
#include "cachedrv.h"	// Page of Cache (Generic Parameter Dependent)

/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheTab dialog
class CuCacheCheckListCtrl;
class CuDlgCacheTab : public CDialog
{
public:
	CuDlgCacheTab(CWnd* pParent = NULL);  
	void DisplayPage (CACHELINEINFO* pData);
	CuCacheCheckListCtrl* GetCacheListCtrl();
	void OnOK() {return;}
	void OnCancel() {return;}
	CDialog* GetCurrentPage(){return m_pCurrentPage;}
	UINT GetHelpID();

	// Dialog Data
	//{{AFX_DATA(CuDlgCacheTab)
	enum { IDD = IDD_CACHE_TAB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgCacheTab)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CDialog*    m_pCurrentPage;
	CuDlgDbmsCacheParameter* m_pCacheParameter;
	CuDlgDbmsCacheDerived*   m_pCacheDerived;

	// Generated message map functions
	//{{AFX_MSG(CuDlgCacheTab)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
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

#endif // !defined(AFX_CACHETAB_H__6722F082_3B35_11D1_A201_00609725DDAF__INCLUDED_)
