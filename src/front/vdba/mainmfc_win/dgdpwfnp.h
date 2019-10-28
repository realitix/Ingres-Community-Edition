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
#ifndef DOMPROP_ICE_FACETANDPAGE_HEADER
#define DOMPROP_ICE_FACETANDPAGE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceFacetAndPage Form View

class CuDlgDomPropIceFacetAndPage : public CFormView
{
protected:
  CuDlgDomPropIceFacetAndPage();
  DECLARE_DYNCREATE(CuDlgDomPropIceFacetAndPage)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceFacetAndPage)
	enum { IDD = IDD_DOMPROP_ICE_FACETNPAGE };
	CStatic	m_stFilename;
	CStatic	m_stLocation;
	CStatic	m_stPath;
	CStatic	m_stSessionCache;
	CStatic	m_stPreCache;
	CStatic	m_stPermCache;
	CEdit	m_edPath;
	CButton	m_stGroupCache;
	CEdit	m_edLocation;
	CEdit	m_edFilename;
	CEdit	m_edDoc;
	CButton	m_chkPublic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceFacetAndPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceFacetAndPage m_Data;

// Implementation
private:
  void ShowHideControls();
protected:
  virtual ~CuDlgDomPropIceFacetAndPage();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceFacetAndPage)
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

#endif  // DOMPROP_ICE_FACETANDPAGE_HEADER
