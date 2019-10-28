/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachelst.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Special Owner Draw List Control                                       //
//               Modeless dialog - left pane of page (DBMS Cache)                      //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/
#if !defined(AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_)
#define AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachelst.h : header file
//
#include "calsctrl.h"

class CuCacheCheckListCtrl: public CuListCtrl
{
public:
	CuCacheCheckListCtrl();
	// Overrides
	//
	virtual ~CuCacheCheckListCtrl();

	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	//
	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheList dialog

class CuDlgCacheList : public CDialog
{
// Construction
public:
	CuDlgCacheList(CWnd* pParent = NULL);   
	CuCacheCheckListCtrl* GetCacheListCtrl() {return &m_ListCtrl;}
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgCacheList)
	enum { IDD = IDD_CACHE_LIST };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgCacheList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList  m_ImageList;
	CImageList  m_ImageListCheck;
	CuCacheCheckListCtrl m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgCacheList)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	//
	// Generic button: You must spesify the action of what you expected !!!
	afx_msg LRESULT OnButton1 (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnButton2 (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnButton3 (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_)
