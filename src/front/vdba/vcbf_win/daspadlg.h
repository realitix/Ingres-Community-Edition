/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : daspadlg.h : header file
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog, Page (Parameter) of DAS Server
** 
** History:
**
** 10-Mar-2004 (uk$so01)
**    Created.
**    SIR #110013 of DAS Server 
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/
#if !defined (_DASPADLG_HEADER_FILE)
#define _DASPADLG_HEADER_FILE

#pragma once

// CuDlgDASVRParameter dialog
#include "editlsgn.h"

class CuDlgDASVRParameter : public CDialog
{
	DECLARE_DYNAMIC(CuDlgDASVRParameter)

public:
	CuDlgDASVRParameter(CWnd* pParent = NULL);   // standard constructor
	virtual ~CuDlgDASVRParameter();
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	enum { IDD = IDD_DASVR_PAGE_PARAMETER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();

	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlGeneric m_ListCtrl;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg LRESULT OnButton1      (WPARAM wParam, LPARAM lParam);     // Edit Value
	afx_msg LRESULT OnButton2      (WPARAM wParam, LPARAM lParam);     // Default
	afx_msg LRESULT OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnValidateData (WPARAM wParam, LPARAM lParam);
};

#endif // _DASPADLG_HEADER_FILE
