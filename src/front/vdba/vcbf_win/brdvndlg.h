/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : brdvndlg.h : header file
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : VNode Page of Bridge
** 
** History:
**
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf, Created 
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#if !defined(AFX_BRDVNDLG_H__BF499692_5034_42FF_A084_6C0207BD704A__INCLUDED_)
#define AFX_BRDVNDLG_H__BF499692_5034_42FF_A084_6C0207BD704A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuListCtrlBridgeVnode: public CuListCtrl
{
public:
	CuListCtrlBridgeVnode():CuListCtrl(){}
	~CuListCtrlBridgeVnode(){}

	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);
};

class CuDlgBridgeVNode : public CDialog
{
public:
	CuDlgBridgeVNode(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuDlgBridgeVNode)
	enum { IDD = IDD_BRIDGE_PAGE_VNODE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgBridgeVNode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuListCtrlBridgeVnode m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgBridgeVNode)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnButton1   (WPARAM wParam, LPARAM lParam);     // Add
	afx_msg LRESULT OnButton2   (WPARAM wParam, LPARAM lParam);     // Delete
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRDVNDLG_H__BF499692_5034_42FF_A084_6C0207BD704A__INCLUDED_)
