/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : FrmVNode.h, Header file    (MDI Child Frame)                          //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when            //
//               it is not a Docking View.                                             //
//    History:                                                                         //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/
#ifndef FRMVNODE_HEADER
#define FRMVNODE_HEADER
class CFrmVNodeMDIChild : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CFrmVNodeMDIChild)
protected:
	CFrmVNodeMDIChild();           // protected constructor used by dynamic creation
	CToolBar    m_wndToolBar;
public:

    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFrmVNodeMDIChild)
	//}}AFX_VIRTUAL

protected:
	virtual ~CFrmVNodeMDIChild();

	// Generated message map functions
	//{{AFX_MSG(CFrmVNodeMDIChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
  afx_msg LRESULT OnGetNodeHandle(WPARAM uu, LPARAM ll);
  afx_msg LRESULT OnGetMfcDocType(WPARAM uu, LPARAM ll);
	DECLARE_MESSAGE_MAP()
};

#endif
