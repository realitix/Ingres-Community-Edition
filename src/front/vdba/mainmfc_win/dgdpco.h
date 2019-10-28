/*
**  Copyright (c) 2005-2010 Actian Corporation. All Rights Reserved.
*/

/*
**
** History:
**
** 13-Dec-2010 (drivi01)
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/
#ifndef DOMPROP_CONNECTION_HEADER
#define DOMPROP_CONNECTION_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPTb.h : header file
//

#include "domseri2.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropConnection dialog

class CuDlgDomPropConnection : public CFormView
{
protected:
  CuDlgDomPropConnection();
  DECLARE_DYNCREATE(CuDlgDomPropConnection)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropConnection)
	enum { IDD = IDD_DOMPROP_CONNECTION };
	CButton	m_chkLocalDb;
	CEdit	m_edNode;
	CEdit	m_edType;
	CEdit	m_edNumber;
	CEdit	m_edDescription;
	CEdit	m_edDb;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropConnection)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void  RefreshDisplay();

private:
  CuDomPropDataConnection m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropConnection();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropConnection)
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // DOMPROP_CONNECTION_HEADER
