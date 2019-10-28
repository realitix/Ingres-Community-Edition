/*
**  Copyright (c) 2004 Actian Corporation
*/

/*
** Name: 256bmp.cpp : The Ingres bitmap of the High Availability Setup Wizard
** 
** Description:
** 	Displays the Ingres bitmap that appears on each property page of the
** 	High Availability Setup Wizard.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
*/

#include "stdafx.h"
#include "wincluster.h"
#include "256bmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WORD DibNumColors (VOID FAR * pv)
{
	INT bits;
	LPBITMAPINFOHEADER lpbi= ((LPBITMAPINFOHEADER)pv);
	LPBITMAPCOREHEADER lpbc= ((LPBITMAPCOREHEADER)pv);
	
	if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
	{
		if (lpbi->biClrUsed != 0)
			return (WORD)lpbi->biClrUsed;
		bits = lpbi->biBitCount;
	}
	else
		bits = lpbc->bcBitCount;
	
	switch (bits)
	{
	case 1:
		return 2;
	case 4:
		return 16;
	case 8:
		return 256;
	default:
		return 0;
	}
}

WORD PaletteSize (VOID FAR * pv)
{
	
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)pv;
	WORD NumColors = DibNumColors(lpbi);
	if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
		return (WORD)(NumColors * sizeof(RGBTRIPLE));
	else
		return (WORD)(NumColors * sizeof(RGBQUAD));
}

HPALETTE CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
	LOGPALETTE *pPal;
	HPALETTE hpal = NULL;
	WORD nNumColors;
	BYTE red;
	BYTE green;
	BYTE blue;
	WORD i;
	RGBQUAD FAR *pRgb;
	if (!lpbi)
		return NULL;
	
	if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
		return NULL;
	/* Get a pointer to the color table and the number of colors in it */
	pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->biSize);
	nNumColors = DibNumColors(lpbi);
	
	if (nNumColors)
	{
		/* Allocate for the logical palette structure */
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
		if (!pPal)
			return NULL;
		pPal->palNumEntries = nNumColors;
		pPal->palVersion = 0x300;
		/* 
		Fill in the palette entries from the DIB color table and create a logical color palette.
		*/
		for (i = 0; i < nNumColors; i++)
		{
			pPal->palPalEntry[i].peRed = pRgb[i].rgbRed;
			pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
			pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;
		}
		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
	}
	else if (lpbi->biBitCount == 24)
	{
		/* 
		A 24 bitcount DIB has no color table entries so, set the number of to the maximum value (256).
		*/
		nNumColors = 256;
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
		if (!pPal)
			return NULL;
		pPal->palNumEntries = nNumColors;
		pPal->palVersion = 0x300;
		red = green = blue = 0;
		for (i = 0; i < pPal->palNumEntries; i++)
		{
			pPal->palPalEntry[i].peRed = red;
			pPal->palPalEntry[i].peGreen = green;
			pPal->palPalEntry[i].peBlue  = blue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;
			if (!(red += 32))
				if (!(green += 32))
					blue += 64;
		}
		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
	}
	return hpal;
}

/////////////////////////////////////////////////////////////////////////////
// C256bmp

C256bmp::C256bmp()
{
}

C256bmp::~C256bmp()
{
}


BEGIN_MESSAGE_MAP(C256bmp, CStatic)
//{{AFX_MSG_MAP(C256bmp)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C256bmp message handlers

BOOL C256bmp::OnEraseBkgnd(CDC* pDC) 
{
	return CStatic::OnEraseBkgnd(pDC);
}

void C256bmp::OnPaint() 
{
	CPaintDC dc(this); /* device context for painting */
	
	BITMAPINFO *pb=GetBitmap();
	if (pb)
	{
		BITMAPINFOHEADER *pbh=( BITMAPINFOHEADER*)pb;
		HPALETTE hpal=CreateBIPalette (pbh);
		LPSTR pbits = (LPSTR)pbh + (WORD)pbh->biSize + PaletteSize(pbh);
		HPALETTE holdpal=::SelectPalette(dc.m_hDC,hpal,FALSE);
		if (!hSystemPalette) hSystemPalette=holdpal;
		::RealizePalette(dc.m_hDC);
		::SetDIBitsToDevice(dc.m_hDC,0,0,pbh->biWidth,pbh->biHeight,0,0,0,pbh->biHeight,pbits,pb,DIB_RGB_COLORS);
		::SelectPalette(dc.m_hDC,holdpal,FALSE);
		::DeleteObject(hpal);
	}
	ValidateRect(0);
}

BITMAPINFO * C256bmp::GetBitmap()
{
	BITMAPINFO *pb = NULL;
	CString s; GetWindowText(s); 
	int bitmapNum=atoi(s);
	HRSRC hrsrc=FindResource(AfxGetInstanceHandle(),MAKEINTRESOURCE(bitmapNum),RT_BITMAP);
	if (hrsrc)
	{
		HGLOBAL h=LoadResource(AfxGetInstanceHandle(),hrsrc);
		if (h)
		{
			pb=(BITMAPINFO *) LockResource(h);
		}
	}
	return pb;
}

void C256bmp::GetBitmapDimension(CPoint &pt)
{
	BITMAPINFOHEADER *pbh=( BITMAPINFOHEADER*)GetBitmap();
	if (pbh)
	{
		pt.x=pbh->biWidth;
		pt.y=pbh->biHeight;
	}
	else
	{
		pt.x=pt.y=0;
	}
	
}
