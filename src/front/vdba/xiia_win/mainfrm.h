/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : mainfrm.h : interface of the CfMainFrame class
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 07-Feb-2001 (uk$so01)
**    Created
** 13-Dec-2010 (drivi01)
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**
**/

#if !defined(AFX_MAINFRM_H__4E271B48_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__4E271B48_FCE9_11D4_873E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "view.h"

class CfMainFrame : public CFrameWnd
{
public:
	CfMainFrame();
protected: 
	DECLARE_DYNAMIC(CfMainFrame)


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CvChildView    m_wndView;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	//}}AFX_MSG
	afx_msg LRESULT OnImportAssistant(WPARAM w, LPARAM l);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__4E271B48_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
