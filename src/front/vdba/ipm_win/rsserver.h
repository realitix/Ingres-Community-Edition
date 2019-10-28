/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsserver.h, header file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a static item Replication.  (Server)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/


#if !defined(AFX_RSSERVER_H__360829A4_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSSERVER_H__360829A4_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"

class CuDlgReplicationStaticPageServer : public CDialog
{
public:
	CuDlgReplicationStaticPageServer(CWnd* pParent = NULL);  
	void OnOK(){return;}
	void OnCancel(){return;}
	void EnableButtons();

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageServer)
	enum { IDD = IDD_REPSTATIC_PAGE_SERVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageServer)
	afx_msg void OnButtonPingAll();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSSERVER_H__360829A4_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
