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
** 11-Apr-2011 (drivi01)
**    Define fields for static strings Locations,
**    Allocate, Extend, Fill Factor and Allocated Pages.
**    Update index properties for VectorWise index
**    by disabling unrelated controls such as Locations,
**    Extend and etc.
*/
#ifndef DOMPROP_INDEX_HEADER
#define DOMPROP_INDEX_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPusr.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIndex form view

class CuDlgDomPropIndex : public CFormView
{
private:
  BOOL m_bMustInitialize;

protected:
	CuDlgDomPropIndex();
  DECLARE_DYNCREATE(CuDlgDomPropIndex)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIndex)
	enum { IDD = IDD_DOMPROP_INDEX };
	CStatic	m_stPersist;
	CListBox	m_lbLocations;
	CListCtrl	m_clistColumns;
	CEdit	m_edPgSize;
	CEdit	m_edUnique;
	CEdit	m_edParent;
	CEdit	m_edIdxSchema;
	CEdit	m_edFillFact;
	CEdit	m_edExtend;
	CEdit	m_edAllocatedPages;
	CEdit	m_edAlloc;
	CStatic	m_stStruct4;
	CStatic	m_stStruct3;
	CStatic	m_stStruct2;
	CStatic	m_stCompress;
	CStatic	m_stStruct1;
	CEdit	m_edStruct4;
	CEdit	m_edStruct3;
	CEdit	m_edStruct2;
	CEdit	m_edStruct1;
	CEdit	m_edStruct;
	CButton	m_chkPersist;
	CButton	m_chkCompress;
	CStatic m_stAlloc;
	CStatic m_stExtend;
	CStatic m_stFillFact;
	CStatic m_stAllocatedPages;
	CStatic m_stLocations;

	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIndex)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIndex m_Data;

// Implementation
protected:
    virtual ~CuDlgDomPropIndex();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIndex)
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

#endif  // DOMPROP_INDEX_HEADER
