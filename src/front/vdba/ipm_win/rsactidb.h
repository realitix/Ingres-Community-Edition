/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/


/*
**  Project  : Visual DBA
**  Source   : rsactidb.h, header file
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : Page of a static item Replication.(Sub page of Activity: Per Database)
**             Outgoing, Incoming, Total 
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(AFX_RSACTIDB_H__360829AB_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSACTIDB_H__360829AB_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"


class CuDlgReplicationStaticPageActivityPerDatabase : public CDialog
{
public:

	CuDlgReplicationStaticPageActivityPerDatabase(CWnd* pParent = NULL);  
	CuDlgReplicationStaticPageActivityPerDatabase(int nMode, CWnd* pParent = NULL);  
	void OnOK(){return;}
	void OnCancel(){return;}
	void GetAllColumnWidth (CArray < int, int >& cxArray);
	void SetAllColumnWidth (CArray < int, int >& cxArray);
	void SetScrollPosListCtrl (CSize& size);
	CSize GetScrollPosListCtrl();

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageActivityPerDatabase)
	enum { IDD = IDD_REPSTATIC_PAGE_ACTIVITY_TAB_DATABASE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	enum {MODE_OUTGOING = 1, MODE_INCOMING, MODE_TOTAL};


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageActivityPerDatabase)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	UINT m_nMode; // (1: Outgoing, 2: Incoming, 3: Total)
	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageActivityPerDatabase)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSACTIDB_H__360829AB_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
