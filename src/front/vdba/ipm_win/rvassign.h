/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rvassign.h, header file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Assignment)
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

#if !defined(AFX_RVASSIGN_H__360829AA_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RVASSIGN_H__360829AA_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"

class CuDlgReplicationServerPageAssignment : public CDialog
{
public:
	CuDlgReplicationServerPageAssignment(CWnd* pParent = NULL); 
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationServerPageAssignment)
	enum { IDD = IDD_REPSERVER_PAGE_ASSIGNMENT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
    CuListCtrl m_cListCtrl;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationServerPageAssignment)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void AddItem (REPLICCDDSASSIGNDATAMIN* pdata);

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationServerPageAssignment)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RVASSIGN_H__360829AA_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
