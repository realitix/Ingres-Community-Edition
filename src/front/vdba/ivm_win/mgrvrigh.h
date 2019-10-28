/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : mgrvrigh.h , Header File
**
**
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**
**
**    Purpose  : The View that contains the Tab Control
**
**  History:
**	15-Dec-1998 (uk$so01)
**	    Created.
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(AFX_MGRVRIGH_H__63A2E058_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MGRVRIGH_H__63A2E058_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "maintab.h"

class CvManagerViewRight : public CView
{
protected:
	CvManagerViewRight(); // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CvManagerViewRight)

public:
	CuMainTabCtrl* GetTabCtrl(){return m_pMainTab;};

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvManagerViewRight)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CuMainTabCtrl* m_pMainTab;

	virtual ~CvManagerViewRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvManagerViewRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoveEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetHelpID  (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComponentChanged (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewEvents  (WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnReinitializeEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReactivateSelection(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLastNameServerStartedup(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetCreatedPane(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MGRVRIGH_H__63A2E058_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
