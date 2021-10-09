// EnBitmap.h: interface for the CHZBitmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)
#define AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include <atlimage.h>

#include "HZColor.h"
////////////////////////////////////////////////////////////////////////////////////
// helper struct. equates to COLORREF

#pragma pack(push)
#pragma pack(1)

struct RGBX
{
public:
	RGBX() : btRed(0), btGreen(0), btBlue(0), btAlpha(0) {};
	RGBX(BYTE red, BYTE green, BYTE blue, BYTE alpha) { btRed = red; btBlue = blue; btGreen = green; btAlpha = alpha; }
	RGBX(COLORREF color, BYTE alpha) { btRed = GetRValue(color); btBlue = GetBValue(color); btGreen = GetGValue(color); btAlpha = alpha; }

	BYTE btBlue;
	BYTE btGreen;
	BYTE btRed;
	BYTE btAlpha;

protected:

public:
	inline BOOL Equals(const RGBX& rgb) { return (btRed == rgb.btRed && btGreen == rgb.btGreen && btBlue == rgb.btBlue); }
	//inline BOOL Equals(RGBX* rgb) { return (btRed == rgb->btRed && btGreen == rgb->btGreen && btBlue == rgb->btBlue); }
	inline BOOL Equals(RGBX* rgb) { return COLORREF(this)==COLORREF(rgb); }
	inline BOOL IsGray() const { return (btRed == btGreen && btGreen == btBlue); }

	RGBX Gray() 
	{
		BYTE btGray = (BYTE)(0.299 * btRed + 0.587 * btGreen + 0.114 * btBlue);
		//BYTE btGray = (btRed*77)+(btGreen*151)+(btBlue*28) >> 8;
		//BYTE btGray = (btBlue + btGreen * 6 + btRed * 3) / 10;
		return RGBX(btGray, btGray, btGray, btAlpha);
	}

};

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////////////
// base class for image processing

class CHZBitmapProcessor 
{

public:
	CHZBitmapProcessor(BOOL bEnableWeighting = FALSE);
	virtual ~CHZBitmapProcessor();

	virtual CSize CalcDestSize(CSize sizeSrc);
	virtual BOOL ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest);

	inline RGBX CalcWeightedColor(RGBX* pPixels, CSize size, double dX, double dY);

	virtual CString GetFunctionName();

protected:
	BOOL m_bWeightingEnabled;
	CString m_strFunctionName;
	
};

typedef CArray<CHZBitmapProcessor*, CHZBitmapProcessor*> CHZBitmapProcessorArray;

//////////////////////////////////////////////////////////////////////////////////////////

class CHZBitmap : public CBitmap  
{
	DECLARE_SERIAL(CHZBitmap)

public:
	CHZBitmap(COLORREF crBkgnd = RGB(255, 255, 255));
	virtual ~CHZBitmap();

	int GetWidth();
	int GetHeight();

	BOOL IsNull() { return (m_hObject==NULL); }
	BOOL CreateImage(int nWidth, int nHeight, bool bTransparent);
	BOOL LoadImage(LPCTSTR szImagePath);
	BOOL LoadImage(UINT uIDRes, int width, int height); 
	void LoadImage(BYTE* pBuff, int nSize);
	BOOL CopyImage(HBITMAP hBitmap);
	BOOL CopyImage(CBitmap* pBitmap);
	BOOL SaveImage(CString strPath);
	BOOL GetImage(CImage* pImage);
	
	void BlitImage(CHZBitmap* pBitmap);
	void RotateImage(int nDegrees, BOOL bEnableWeighting = FALSE); // rotates about centrepoint, +ve (clockwise) or -ve (anticlockwise) from 12 o'clock
	void ShearImage(int nHorz, int nVert, BOOL bEnableWeighting = FALSE); // shears +ve (right, down) or -ve (left, up)
	void GrayImage();
	void BlurImage(int nAmount = 5);
	void ShadowImage(COLORREF crColor, int nRadius);
	void FadeImage(int nAmount = 5);
	void SharpenImage(int nAmount = 5);
	void MaskImage(COLORREF crMask, bool bAntiAlias = false);
	void BoxImage(COLORREF crColor, CRect rcRect, int nAmount, int nBorder = 0);
	void ResizeImage(double dFactor);
	void ResizeImage(int nWidth, int nHeight);
	void ResizeImage(int nWidth, int nHeight, int nFilter);
	void FlipImage(BOOL bHorz, BOOL bVert = 0);
	void NegateImage();
	void ReplaceColor(COLORREF crFrom, COLORREF crTo);
	void CutImage(CHZBitmap* pBitmap, bool bInvert, bool bMonochrome = true);
	void SoftenImage();
	void ShiftImage(int nAngle, int nRadius);
	void SweepImage(COLORREF crColor, int nAngle, int nRadius);
	void OutlineImage(COLORREF crColor, int nRadius);
	void ColorizeImage(COLORREF crColor);
	void PadImage(int nWidth, int nHeight, int nAlignment);
	void PadImage(int nWidth, int nHeight, float fXPos, float fYPos);
	void CropImage(int x1, int y1, int x2, int y2);
	void ClampImage(bool bMultiply, bool bFlatten = false, bool bSwap = false);
	void DitherImage(int nColors);
	void QuantizeImage(int nColors, CList<CHZColor, CHZColor&> &colorList);

	BOOL ProcessImage(CHZBitmapProcessor* pProcessor);
	BOOL ProcessImage(CHZBitmapProcessorArray& aProcessors); // ordered list of processors

	// helpers
	

	RGBX* GetDIBits32();

	void Design();
	void Render();

	static BOOL Fill(RGBX* pPixels, CSize size, COLORREF color, int alpha);

	virtual void Serialize(CArchive& ar);

protected:
	COLORREF m_crBkgnd;
	CHZBitmapProcessorArray m_arrProcessors;

private:
	bool m_bDesignMode;

	void Action(CHZBitmapProcessor* pProcessor);

protected:

	BOOL PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap = NULL);

	static HBITMAP ExtractBitmap(IPicture* pPicture, COLORREF crBack);
	static int GetFileType(LPCTSTR szImagePath);

};

// inline weighting function
inline RGBX CHZBitmapProcessor::CalcWeightedColor(RGBX* pPixels, CSize size, double dX, double dY)
{
	ASSERT (m_bWeightingEnabled);
	 
	// interpolate between the current pixel and its pixel to the right and down
	int nX = (int)dX;
	int nY = (int)dY;
	 
	if (dX < 0 || dY < 0)
	return pPixels[max(0, nY) * size.cx + max(0, nX)]; // closest
	 
	RGBX* pRGB = &pPixels[nY * size.cx + nX]; // current
	 
	double dXFraction = dX - nX;
	double dX1MinusFraction = 1 - dXFraction;

	double dYFraction = dY - nY;
	double dY1MinusFraction = 1 - dYFraction;

	RGBX* pRGBXP = &pPixels[nY * size.cx + min(nX + 1, size.cx - 1)]; // x + 1
	RGBX* pRGBYP = &pPixels[min(nY + 1, size.cy - 1) * size.cx + nX]; // y + 1
	RGBX* pRGBXYP = &pPixels[min(nY + 1, size.cy - 1) * size.cx + min(nX + 1, size.cx - 1)]; // y + 1,x+1
	 
	int nRed = (int)(( dX1MinusFraction * dY1MinusFraction * pRGB->btRed +
	dXFraction * dY1MinusFraction * pRGBXP->btRed +
	dX1MinusFraction * dYFraction * pRGBYP->btRed +
	dXFraction * dYFraction * pRGBXYP->btRed) );

	int nGreen = (int)(( dX1MinusFraction * dY1MinusFraction * pRGB->btGreen +
	dXFraction * dY1MinusFraction * pRGBXP->btGreen +
	dX1MinusFraction * dYFraction * pRGBYP->btGreen +
	dXFraction * dYFraction * pRGBXYP->btGreen) );

	int nBlue = (int)(( dX1MinusFraction * dY1MinusFraction * pRGB->btBlue +
	dXFraction * dY1MinusFraction * pRGBXP->btBlue +
	dX1MinusFraction * dYFraction * pRGBYP->btBlue +
	dXFraction * dYFraction * pRGBXYP->btBlue) );

	int nAlpha = (int)(( dX1MinusFraction * dY1MinusFraction * pRGB->btAlpha +
	dXFraction * dY1MinusFraction * pRGBXP->btAlpha +
	dX1MinusFraction * dYFraction * pRGBYP->btAlpha +
	dXFraction * dYFraction * pRGBXYP->btAlpha) );
	if (nAlpha==254) nAlpha ++;

	/*
	int nAlpha2 = pRGB->btAlpha;
	if (abs(nAlpha-nAlpha2)==1) 
	{
		TRACE2("%i\%i\n",nAlpha,nAlpha2);
		nAlpha = nAlpha2;
	}
	*/

	return RGBX(nRed, nGreen, nBlue, nAlpha);
} 

#endif // !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)
