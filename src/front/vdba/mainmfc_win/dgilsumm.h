/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgILSumm.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail (Summary) page of Locations.            //              
//                                                                                     //
//    History  :                                                                       //
//      13-Dec-2010 (drivi01)                                                          //
//         Port the solution to x64. Clean up the warnings.                            //
//         Clean up datatypes. Port function calls to x64.                             //
//                                                                                     //
****************************************************************************************/
#ifndef DGILSUMM_HEADER
#define DGILSUMM_HEADER
#include "statfrm.h"
#include "statlege.h"

class CuDlgIpmSummaryLocations : public CDialog
{
public:
	CuDlgIpmSummaryLocations(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmSummaryLocations)
	enum { IDD = IDD_IPMDETAIL_LOCATIONS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuStatisticBarLegendList  m_cListLegend;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmSummaryLocations)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuStatisticBarFrame* m_pDlgFrame;
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmSummaryLocations)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnApply      (WPARAM wParam, LPARAM lParam);
};
#endif
