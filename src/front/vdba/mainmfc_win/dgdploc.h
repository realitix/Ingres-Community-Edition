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
#ifndef DOMPROP_LOCATION_HEADER
#define DOMPROP_LOCATION_HEADER

#include "domrfsh.h"   // CuDomRefreshParams

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPLoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocation Form View

class CuDlgDomPropLocation : public CFormView
{
protected:
  CuDlgDomPropLocation();
  DECLARE_DYNCREATE(CuDlgDomPropLocation)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropLocation)
	enum { IDD = IDD_DOMPROP_LOCATION };
	CStatic	m_ctrlStaticCheckArea;
	CStatic	m_ctrlStaticPctRaw;
	CEdit	m_ctrlEditPctRaw;
	CButton	m_chkRawArea;
	CEdit	m_editArea;
	CButton	m_chkWork;
	CButton	m_chkJournal;
	CButton	m_chkDump;
	CButton	m_chkDatabase;
	CButton	m_chkCheckpoint;
	//}}AFX_DATA

  CString m_Area;
  BOOL    m_bDatabase;
  BOOL    m_bWork;
  BOOL    m_bJournal;
  BOOL    m_bCheckPoint;
  BOOL    m_bDump;
  int     m_iRawPct;

  // Added for background refresh management
  CuDomRefreshParams m_refreshParams;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropLocation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

// Implementation
protected:
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropLocation)
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

#endif  // DOMPROP_LOCATION_HEADER
