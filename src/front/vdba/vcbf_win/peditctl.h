/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : peditctl.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Special parse edit to allow only a subset of characters               //
****************************************************************************************/
#if !defined(AFX_PEDITCTL_H__2BED42B1_6186_11D1_A225_00609725DDAF__INCLUDED_)
#define AFX_PEDITCTL_H__2BED42B1_6186_11D1_A225_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// Parse Style
// -----------
#define PES_NATURAL         0x0001           // Positive integer
#define PES_LETTERS         0x0002           // Alphabet
#define PES_OTHERCHARS      0x0004           // Other
#define PES_INTEGER         (0x0008|0x0001)	 // Can be negative
#define PES_ALL             0xFFFF           // All chars
// Above the range for normal EN_ messages
// sent to parent when illegal character hit
// return 0 if you want parsed edit to beep
#define PEN_ILLEGALCHAR     0x8000


class CuParseEditCtrl : public CEdit
{
public:
	CuParseEditCtrl();

	//
	// Explicit construction
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	//
	// Subclassed construction
	BOOL SubclassEdit(UINT nID, CWnd* pParent, WORD wParseStyle);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuParseEditCtrl)
	//}}AFX_VIRTUAL
	virtual void OnBadInput();

	//
	// Implementation
protected:
	WORD m_wParseStyle; 

public:
	virtual ~CuParseEditCtrl();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuParseEditCtrl)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PEDITCTL_H__2BED42B1_6186_11D1_A225_00609725DDAF__INCLUDED_)
