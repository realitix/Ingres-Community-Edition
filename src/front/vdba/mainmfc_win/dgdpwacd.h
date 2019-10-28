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
#ifndef DOMPROP_ICE_ACCESSDEF_HEADER
#define DOMPROP_ICE_ACCESSDEF_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceAccessDef Form View

class CuDlgDomPropIceAccessDef : public CFormView
{
protected:
  CuDlgDomPropIceAccessDef();
  DECLARE_DYNCREATE(CuDlgDomPropIceAccessDef)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceAccessDef)
	enum { IDD = IDD_DOMPROP_ICE_BUNIT_ACCESSDEF };
	CButton	m_chkUpdate;
	CButton	m_chkRead;
	CButton	m_chkDelete;
	CButton	m_chkExecute;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceAccessDef)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceAccessDef m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceAccessDef();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceAccessDef)
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

#endif  // DOMPROP_ICE_ACCESSDEF_HEADER
