/*
** Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
**
**    Source   : dlgmsbox.cpp, Implementation File
**    Project  : Ingres II / Visual VDBA
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog box that acts like a message box.
**
** History:
**    26-Jan-2000 (uk$so01)
**       (SIR #100175) Created.
**    13-Dec-2010 (drivi01) 
**       Port the solution to x64. Clean up the warnings.
**       Clean up datatypes. Port function calls to x64.
**
**/

#if !defined(AFX_DLGMSBOX_H__A2998043_D3F7_11D3_A337_00C04F1F754A__INCLUDED_)
#define AFX_DLGMSBOX_H__A2998043_D3F7_11D3_A337_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgMessageBox : public CDialog
{
public:
	CxDlgMessageBox(CWnd* pParent = NULL);
	void OnCancel(){return;}
	// Dialog Data
	//{{AFX_DATA(CxDlgMessageBox)
	enum { IDD = IDD_XMESSAGEBOX };
	CStatic	m_cStaticImage;
	CString	m_strStaticText;
	BOOL	m_bNotShow;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgMessageBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgMessageBox)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//
// RETURN IDOK or IDCANCEL
// bContinue will be set to TRUE or FALSE;
INT_PTR MSGContinueBox (LPCTSTR lpszMessage, BOOL& bContinue);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGMSBOX_H__A2998043_D3F7_11D3_A337_00C04F1F754A__INCLUDED_)
