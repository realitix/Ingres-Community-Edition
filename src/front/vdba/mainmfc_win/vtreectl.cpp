// VtreeCtl.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtreectl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVtreeCtrl

CVtreeCtrl::CVtreeCtrl()
{
}

CVtreeCtrl::~CVtreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CVtreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CVtreeCtrl)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVtreeCtrl message handlers

void CVtreeCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
  HTREEITEM hHit = HitTest(point, &nFlags);
  if (hHit)
    SelectItem (hHit);

	CTreeCtrl::OnRButtonDown(nFlags, point);
}
