/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : acprcp.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Archiver and Recover Log events
**
** History:
**
** 05-Jul-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#if !defined(ACPRCP_HEADER)
#define ACPRCP_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuDlgArchiverRecoverLog : public CDialog
{
public:
	CuDlgArchiverRecoverLog(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuDlgArchiverRecoverLog)
	enum { IDD = IDD_LOGEVENT_ACPRCP };
	CEdit	m_cEdit1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgArchiverRecoverLog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgArchiverRecoverLog)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(ACPRCP_HEADER)
