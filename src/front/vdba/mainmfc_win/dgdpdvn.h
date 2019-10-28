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
#ifndef DOMPROP_STARVIEW_N_HEADER
#define DOMPROP_STARVIEW_N_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri.h"
#include "domilist.h"

class CuDlgDomPropViewNative : public CDialog
{
public:
    CuDlgDomPropViewNative(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropViewNative)
    enum { IDD = IDD_DOMPROP_STARVIEW_N };
    // NOTE: the ClassWizard will add data members here
    CEdit	m_edText;
    CuListCtrlCheckmarks  m_clistCtrl;
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropViewNative)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  int GetImageIndexFromType(int componentType);

private:
  CuDomPropDataView m_Data;

  CuDomImageList m_ImageList;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropViewNative)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_STARVIEW_N_HEADER
