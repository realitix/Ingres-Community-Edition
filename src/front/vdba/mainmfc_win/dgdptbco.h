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
#ifndef DOMPROP_TABLE_COLUMNS_HEADER
#define DOMPROP_TABLE_COLUMNS_HEADER

#include "domseri.h"

class CuDlgDomPropTableColumns : public CDialog
{
public:
    CuDlgDomPropTableColumns(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropTableColumns)
    enum { IDD = IDD_DOMPROP_TABLE_COLUMNS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // columns list control
    CuListCtrlCheckmarks  m_cListColumns;
    CImageList            m_ImageList;
    CImageList            m_ImageCheck;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropTableColumns)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void  AddTblColumn(CuTblColumn* pTblColumn);

private:
  CuDomPropDataTableColumns m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropTableColumns)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_TABLE_COLUMNS_HEADER
