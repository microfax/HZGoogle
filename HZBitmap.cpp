// EnBitmap.cpp: implementation of the CHZBitmap class (c) daniel godson 2002.
//
// credits: Peter Hendrix's CPicture implementation for the original IPicture code 
//          Yves Maurer's GDIRotate implementation for the idea of working directly on 32 bit representations of bitmaps 
//          Karl Lager's 'A Fast Algorithm for Rotating Bitmaps' 
// 
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "HZBitmap.h"
#include "HZBitmapProcessors.h"
#include <atlimage.h>
#include "HZColor.h"

#include <afxpriv.h>
#include <unordered_map>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int HIMETRIC_INCH	= 2540;

///////////////////////////////////////////////////////////////////////

CHZBitmapProcessor::CHZBitmapProcessor(BOOL bEnableWeighting) : m_bWeightingEnabled(bEnableWeighting)
{
}

CHZBitmapProcessor::~CHZBitmapProcessor()
{
}

CString CHZBitmapProcessor::GetFunctionName()
{
	return m_strFunctionName;
}

CSize CHZBitmapProcessor::CalcDestSize(CSize sizeSrc) 
{ 
	return sizeSrc; // default
}

BOOL CHZBitmapProcessor::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{ 
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	return TRUE;
}
 
// CHZBitmapProcessor::CalcWeightedColor(...) is inlined in EnBitmap.h

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHZBitmap::CHZBitmap(COLORREF crBkgnd) : m_crBkgnd(crBkgnd)
{
	m_bDesignMode = false;
}

CHZBitmap::~CHZBitmap()
{
	for (int i=0; i<m_arrProcessors.GetSize(); i++) delete m_arrProcessors[i];
	m_arrProcessors.RemoveAll();

	//Detach();
	//DeleteObject();
}

void CHZBitmap::Design()
{
	for (int i=0; i<m_arrProcessors.GetSize(); i++) delete m_arrProcessors[i];
	m_arrProcessors.RemoveAll();
	m_bDesignMode = true;
}

void CHZBitmap::Render()
{
	if (m_bDesignMode)
	{
		ProcessImage(m_arrProcessors);	
		m_bDesignMode = false;
	}
	for (int i=0; i<m_arrProcessors.GetSize(); i++) delete m_arrProcessors[i];
	m_arrProcessors.RemoveAll();
}

void CHZBitmap::Action(CHZBitmapProcessor* pProcessor)
{
	if (m_bDesignMode)
	{
		m_arrProcessors.Add(pProcessor);
	}
	else
	{
		ProcessImage(pProcessor);
		delete pProcessor;
	}
}

void CHZBitmap::RotateImage(int nDegrees, BOOL bEnableWeighting)				{ Action(new CHZBitmapRotator(nDegrees, bEnableWeighting)); }
void CHZBitmap::ShearImage(int nHorz, int nVert, BOOL bEnableWeighting)			{ Action(new CHZBitmapShearer(nHorz, nVert, bEnableWeighting)); }
void CHZBitmap::FadeImage(int nAmount)											{ Action(new CHZBitmapFader(nAmount)); }
void CHZBitmap::SoftenImage()													{ Action(new CHZBitmapBlurrer(1)); }
void CHZBitmap::GrayImage()														{ Action(new CHZBitmapGrayer()); }
void CHZBitmap::MaskImage(COLORREF crMask, bool bAntiAlias)						{ Action(new CHZBitmapMasker(crMask, bAntiAlias)); }
void CHZBitmap::BoxImage(COLORREF crColor, CRect rcRect, int nAmount, int nBorder)			{ Action(new CHZBitmapBoxer(crColor, rcRect, nAmount, nBorder)); }
void CHZBitmap::CutImage(CHZBitmap* pBitmap, bool bInvert, bool bMonochrome)	{ Action(new CHZBitmapCutter(pBitmap, bInvert, bMonochrome)); }
void CHZBitmap::BlitImage(CHZBitmap* pBitmap)									{ Action(new CHZBitmapBlitter(pBitmap)); }
void CHZBitmap::BlurImage(int nAmount)											{ Action(new CHZBitmapHorizontalBlurrer(nAmount)); }
void CHZBitmap::ShadowImage(COLORREF crColor, int nRadius)						{ Action(new CHZBitmapShadower(crColor, nRadius)); }
void CHZBitmap::SharpenImage(int nAmount)										{ Action(new CHZBitmapSharpener(nAmount)); }
void CHZBitmap::ResizeImage(double dFactor)										{ Action(new CHZBitmapResizer(dFactor)); }
void CHZBitmap::ResizeImage(int nWidth, int nHeight)							{ Action(new CHZBitmapStretcher(nWidth, nHeight)); }
void CHZBitmap::ResizeImage(int nWidth, int nHeight, int nFilter)				{ Action(new CHZBitmapResampler(nWidth, nHeight, (CHZBitmapResampler::FILTER)nFilter)); }
void CHZBitmap::FlipImage(BOOL bHorz, BOOL bVert)								{ Action(new CHZBitmapFlipper(bHorz, bVert)); }
void CHZBitmap::NegateImage()													{ Action(new CHZBitmapNegator()); }
void CHZBitmap::ReplaceColor(COLORREF crFrom, COLORREF crTo)					{ Action(new CHZColorReplacer(crFrom, crTo)); }
void CHZBitmap::ShiftImage(int nAngle, int nRadius)								{ Action(new CHZBitmapShifter(nAngle, nRadius)); }
void CHZBitmap::SweepImage(COLORREF crColor, int nAngle, int nRadius)			{ Action(new CHZBitmapSweeper(crColor, nAngle, nRadius)); }
void CHZBitmap::OutlineImage(COLORREF crColor, int nRadius)						{ Action(new CHZBitmapOutliner(crColor, nRadius)); }
void CHZBitmap::ColorizeImage(COLORREF crColor)									{ Action(new CHZBitmapColorizer(crColor)); }
void CHZBitmap::PadImage(int nWidth, int nHeight, int nAlignment)				{ Action(new CHZBitmapPadder(nWidth, nHeight, nAlignment)); }
void CHZBitmap::PadImage(int nWidth, int nHeight, float fXPos, float fYPos)		{ Action(new CHZBitmapPadder(nWidth, nHeight, fXPos, fYPos)); }
void CHZBitmap::CropImage(int x1, int y1, int x2, int y2)						{ Action(new CHZBitmapCropper(x1, y1, x2, y2)); }
void CHZBitmap::ClampImage(bool bMultiply, bool bFlatten, bool bSwap)			{ Action(new CHZBitmapClamper(bMultiply, bFlatten, bSwap)); }
void CHZBitmap::DitherImage(int nColors)										{ Action(new CHZBitmapDitherer(this, nColors)); }





BOOL CHZBitmap::SaveImage(CString strPath)
{
	int width = GetWidth();
	int height = GetHeight();

	if ((width * height) < 20) return FALSE;

	CHZBitmap bmpCopy;
	bmpCopy.CopyImage(this);
	bool bPng = (strPath.Right(3)==L"png");
	if (bPng) bmpCopy.ClampImage(false);
	else bmpCopy.ClampImage(false, true);

	CImage imgComposite;
	imgComposite.Destroy();
	imgComposite.Create(width, height, 32, CImage::createAlphaChannel);
	
	BYTE* outptr = (BYTE*)imgComposite.GetBits();
	int outpitch = imgComposite.GetPitch();

	RGBX* pSrcPixels = bmpCopy.GetDIBits32();

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			RGBX* pRGBSrc = &pSrcPixels[y * width + x];
			*(outptr+outpitch*y+4*x) = pRGBSrc->btBlue;
			*(outptr+outpitch*y+4*x+1) = pRGBSrc->btGreen;
			*(outptr+outpitch*y+4*x+2) = pRGBSrc->btRed;
			*(outptr+outpitch*y+4*x+3) = pRGBSrc->btAlpha;
		}
	}

	delete [] pSrcPixels;

	return imgComposite.Save(strPath);
}

BOOL CHZBitmap::GetImage(CImage* pImage)
{
	try
	{

		HBITMAP hBitmap = (HBITMAP)GetSafeHandle();
		BITMAP BM;
		if (!::GetObject(hBitmap, sizeof(BM), &BM)) return 0;

		int width = BM.bmWidth;
		int height = BM.bmHeight;

		pImage->Destroy();
		pImage->Create(width, height, 32, CImage::createAlphaChannel);
		BYTE* outPtr = (BYTE*)pImage->GetBits();
		int outPitch = pImage->GetPitch();

		RGBX* pPixels = GetDIBits32();

		for (int x=0; x<width;x++)
		for (int y=0; y<height;y++)
		{
			RGBX* pPixel = &pPixels[y * width + x];

			*(outPtr+outPitch*y+4*x) =  pPixel->btBlue;
			*(outPtr+outPitch*y+4*x+1) =  pPixel->btGreen;
			*(outPtr+outPitch*y+4*x+2) =  pPixel->btRed;
			*(outPtr+outPitch*y+4*x+3) = pPixel->btAlpha;
		}

		delete[] pPixels;
	}
	catch (...)
	{
		
		return false;
	}

	return TRUE;
}


BOOL CHZBitmap::CreateImage(int nWidth, int nHeight, bool bTransparent)
{
	BITMAPINFO bi; 
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = nWidth;
	bi.bmiHeader.biHeight = -nHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	
	HDC hDC = ::GetDC(NULL);
	unsigned char* lpBitmapBits;
	HBITMAP bitmap = ::CreateDIBSection(hDC, &bi, DIB_RGB_COLORS,  (VOID**)&lpBitmapBits, NULL, 0);

	if (!bTransparent)
	{
		int pitch = 4*nWidth; // 4 bytes per pixel but if not 32 bit, round pitch up to multiple of 4
		int index;
		for(int x=0; x<nWidth; x=x++)
		{
			for(int y=0; y<nHeight; y++)
			{
				index = y * pitch;
				index += x * 4;
				lpBitmapBits[index + 0] = 255; // blue
				lpBitmapBits[index + 1] = 255; // green
				lpBitmapBits[index + 2] = 255; // red
				lpBitmapBits[index + 3] = 255; // alpha 
				
			}
		}
	}

	::ReleaseDC(NULL, hDC);
	DeleteObject();
	m_hObject=bitmap;

	return NULL;
}


BOOL CHZBitmap::LoadImage(LPCTSTR szImagePath)
{
	CImage imgIn;
	imgIn.Destroy();
	imgIn.Load(szImagePath);

	int width = imgIn.GetWidth();
	int height = imgIn.GetHeight();
	int inPitch = imgIn.GetPitch();
	int nBPP = imgIn.GetBPP();
	bool is32Bit = (imgIn.GetBPP()==32);
	BYTE* inPtr = (BYTE*)imgIn.GetBits();

	CreateImage(width, height, is32Bit);


	if (nBPP == 8)
	{
	}

	int nBits = is32Bit?4:3;

	RGBX* pPixels = new RGBX[width * height];

	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			RGBX* pPixel = &pPixels[y * width + x];
			
			pPixel->btBlue = *(inPtr+inPitch*y+nBits*x);
			pPixel->btGreen = *(inPtr+inPitch*y+nBits*x+1);
			pPixel->btRed = *(inPtr+inPitch*y+nBits*x+2);
			if (nBits==4) pPixel->btAlpha = *(inPtr+inPitch*y+nBits*x+3);

			pPixel->btAlpha = is32Bit ? pPixel->btAlpha : 255;
			pPixel->btRed = INT_ALPHA[pPixel->btRed][pPixel->btAlpha];
			pPixel->btGreen = INT_ALPHA[pPixel->btGreen][pPixel->btAlpha];
			pPixel->btBlue = INT_ALPHA[pPixel->btBlue][pPixel->btAlpha];

		}
	}

	SetBitmapBits(width * height * 4, pPixels);
	delete[] pPixels;
	return TRUE;


}

BOOL CHZBitmap::LoadImage(UINT uIDRes, int width, int height)
{
	HRSRC hRes = FindResourceEx(GetModuleHandle(NULL), _T("BINARY"), MAKEINTRESOURCE(uIDRes), MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL));
	DWORD dwSize = SizeofResource(GetModuleHandle(NULL), hRes);
	HGLOBAL hGlob = LoadResource(GetModuleHandle(NULL), hRes);
	RGBX* pPixels = reinterpret_cast<RGBX*>(::LockResource(hGlob));
	::UnlockResource(hGlob);

	CreateImage(width, height, false);
	SetBitmapBits(width * height * 4, pPixels);
	return TRUE;

}

void CHZBitmap::LoadImage(BYTE* pBuff, int nSize)
{
	IStream* pMemStream = SHCreateMemStream(pBuff, nSize);
	CComPtr<IStream> stream;
	stream.Attach(pMemStream); // Need to Attach, otherwise ownership is not transferred and we leak memory
	CImage imgIn;
	imgIn.Load(stream);
	int width = imgIn.GetWidth();
	int height = imgIn.GetHeight();
	int inPitch = imgIn.GetPitch();
	int nDepth = imgIn.GetBPP();
	BYTE* inPtr = (BYTE*)imgIn.GetBits();

	CreateImage(width, height, true);

	int nBits = nDepth / 8;

	RGBX* pPixels = new RGBX[width * height];

	RGBQUAD arrColors[256];
	if (imgIn.IsIndexed())
	{
		int nColorTableEntries = imgIn.GetMaxColorTableEntries();
		imgIn.GetColorTable(0, nColorTableEntries, arrColors);
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			RGBX* pPixel = &pPixels[y * width + x];

			pPixel->btBlue = *(inPtr + inPitch * y + nBits * x);
			if (nBits > 1)
			{
				pPixel->btGreen = *(inPtr + inPitch * y + nBits * x + 1);
				pPixel->btRed = *(inPtr + inPitch * y + nBits * x + 2);
				if (nBits > 3) pPixel->btAlpha = *(inPtr + inPitch * y + nBits * x + 3);
				else pPixel->btAlpha = 255;
			}
			else
			{
				if (imgIn.IsIndexed())
				{
					pPixel->btGreen = arrColors[pPixel->btBlue].rgbGreen;
					pPixel->btRed = arrColors[pPixel->btBlue].rgbRed;
					pPixel->btBlue = arrColors[pPixel->btBlue].rgbBlue;
				}
				else
				{
					pPixel->btGreen = pPixel->btBlue;
					pPixel->btRed = pPixel->btBlue;
				}
				pPixel->btAlpha = 255;
			}
			//pPixel->btAlpha = 255;//is32Bit?pPixel->btAlpha:255;
			pPixel->btAlpha = pPixel->btAlpha;
			pPixel->btRed = INT_ALPHA[pPixel->btRed][pPixel->btAlpha];
			pPixel->btGreen = INT_ALPHA[pPixel->btGreen][pPixel->btAlpha];
			pPixel->btBlue = INT_ALPHA[pPixel->btBlue][pPixel->btAlpha];
		}
	}

	SetBitmapBits(width * height * 4, pPixels);
	delete[] pPixels;
}


HBITMAP CHZBitmap::ExtractBitmap(IPicture* pPicture, COLORREF crBack)
{
	ASSERT(pPicture);

	if (!pPicture)
		return NULL;

	CBitmap bmMem;
	CDC dcMem;
	CDC* pDC = CWnd::GetDesktopWindow()->GetDC();

	if (dcMem.CreateCompatibleDC(pDC))
	{
		long hmWidth;
		long hmHeight;

		pPicture->get_Width(&hmWidth);
		pPicture->get_Height(&hmHeight);
		
		int nWidth	= MulDiv(hmWidth, pDC->GetDeviceCaps(LOGPIXELSX), HIMETRIC_INCH);
		int nHeight	= MulDiv(hmHeight, pDC->GetDeviceCaps(LOGPIXELSY), HIMETRIC_INCH);

		if (bmMem.CreateCompatibleBitmap(pDC, nWidth, nHeight))
		{
			CBitmap* pOldBM = dcMem.SelectObject(&bmMem);

			if (crBack != -1)
				dcMem.FillSolidRect(0, 0, nWidth, nHeight, crBack);
			
			HRESULT hr = pPicture->Render(dcMem, 0, 0, nWidth, nHeight, 0, hmHeight, hmWidth, -hmHeight, NULL);
			dcMem.SelectObject(pOldBM);
		}
	}

	CWnd::GetDesktopWindow()->ReleaseDC(pDC);

	return (HBITMAP)bmMem.Detach();
}

int CHZBitmap::GetFileType(LPCTSTR szImagePath)
{
	CString sPath(szImagePath);
	sPath.MakeUpper();

	// else
	return 0;
}



BOOL CHZBitmap::ProcessImage(CHZBitmapProcessor* pProcessor) 
{
	CHZBitmapProcessorArray aProcessors;
	aProcessors.Add(pProcessor);
	return ProcessImage(aProcessors);
}


typedef struct colorhist_item 
{
	RGBX color;
	int value;
} *colorhist_vector;


typedef struct box* box_vector;

struct box
{
	int index;
	int colors;
	int sum;
};




static int redcompare(const void* ch1, const void* ch2)
{
	return ((colorhist_vector)ch1)->color.btRed - ((colorhist_vector)ch2)->color.btRed;
}

static int greencompare(const void* ch1, const void* ch2)
{
	return ((colorhist_vector)ch1)->color.btGreen - ((colorhist_vector)ch2)->color.btGreen;
}

static int bluecompare(const void* ch1, const void* ch2)
{
	return ((colorhist_vector)ch1)->color.btBlue - ((colorhist_vector)ch2)->color.btBlue;
}

static int sumcompare(const void* b1, const void* b2)
{
	return ((box_vector)b2)->sum - ((box_vector)b1)->sum;
}

static bool bFlip = true;

void CHZBitmap::QuantizeImage(int nColors, CList<CHZColor, CHZColor&> &colorList)
{
	
	if (!GetSafeHandle()) return;

	int newcolors = max(/*115*/256, nColors); // nColors * 7;
	try
	{
		// retrieve src and final dest sizes
		BITMAP BM;
		if (!GetBitmap(&BM)) return;
		CSize sizeSrc(BM.bmWidth, BM.bmHeight);

		// prepare src and dest bits
		RGBX* pSrcPixels = GetDIBits32();
		if (!pSrcPixels) return;

		// Create histogram
		int colors = 0;
		int sum = 0;
		
		unordered_map<DWORD, int> histogram;
		
		for (int nX = 0; nX < sizeSrc.cx; nX++)
		{
			for (int nY = 0; nY < sizeSrc.cy; nY++)
			{
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
				DWORD col = RGB(pRGBSrc->btRed, pRGBSrc->btGreen, pRGBSrc->btBlue);
				if (histogram.find(col) == histogram.end()) histogram[col] = 1;
				else histogram[col]++;
			}
		}

		delete[] pSrcPixels;

		
		vector<colorhist_item> chv;
		unordered_map<DWORD, int>::iterator itr;
		for (itr = histogram.begin(); itr != histogram.end(); itr++)
		{
			colors++;
			sum += itr->second;
			colorhist_item item;
			item.color = RGBX(COLORREF(itr->first), 0);
			item.value = itr->second;
			chv.push_back(item);
		}

		//assert(colors != sum);

		colorhist_vector colormap;
		box_vector bv;
		register int bi, i;
		int boxes;

		colormap = (colorhist_vector)malloc(sizeof(struct colorhist_item) * newcolors);
		if (colormap == (colorhist_vector)0) return;
		for (i = 0; i < newcolors; ++i) colormap[i].color = RGBX(0,0,0,0);
		if (colors <= newcolors)
		{
			for (i = 0; i < colors; i++)
			{
				colormap[i].color = chv[i].color;
				colorList.AddTail(CHZColor(RGB(chv[i].color.btRed, chv[i].color.btGreen, chv[i].color.btBlue)));
			}
			return;
		}
		bv = (box_vector)malloc(sizeof(struct box) * newcolors);
		if (bv == (box_vector)0) {
			free(colormap);
			return;
		}

		//Set up the initial box.
		bv[0].index = 0;
		bv[0].colors = colors;
		bv[0].sum = sum;
		boxes = 1;

		//Main loop: split boxes until we have enough.

		while (boxes < newcolors)
		{
			register int indx, clrs;
			int sm;
			register int minr, maxr, ming, maxg, minb, maxb, v;
			int halfsum, lowersum;

			//Find the first splittable box.
			
			for (bi = 0; bv[bi].colors < 2 && bi < boxes; ++bi)
				;
			if (bi == boxes)
				break;	// ran out of colors! 
			indx = bv[bi].index;
			clrs = bv[bi].colors;
			sm = bv[bi].sum;

			// Go through the box finding the minimum and maximum of each
			// component - the boundaries of the box.
			
			minr = maxr = chv[indx].color.btRed;
			ming = maxg = chv[indx].color.btGreen;
			minb = maxb = chv[indx].color.btBlue;
			for (i = 1; i < clrs; ++i)
			{
				v = chv[indx + i].color.btRed;
				if (v < minr) minr = v;
				if (v > maxr) maxr = v;
				v = chv[indx + i].color.btGreen;
				if (v < ming) ming = v;
				if (v > maxg) maxg = v;
				v = chv[indx + i].color.btBlue;
				if (v < minb) minb = v;
				if (v > maxb) maxb = v;
			}

						
			if (maxr - minr >= maxg - ming && maxr - minr >= maxb - minb)
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), redcompare);
			else if (maxg - ming >= maxb - minb)
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), greencompare);
			else
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), bluecompare);
			
			/*
			float rl, gl, bl;

			rl = 0.299 * (maxr - minr);
			gl = 0.587 * (maxg - ming);
			bl = 0.114 * (maxb - minb);

			if (rl >= gl && rl >= bl)
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), redcompare);
			else if (gl >= bl)
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), greencompare);
			else
				qsort((char*)&(chv[indx]), clrs, sizeof(struct colorhist_item), bluecompare);
			*/

			
			lowersum = chv[indx].value;
			halfsum = sm / 2;
			for (i = 1; i < clrs - 1; ++i)
			{
				if (lowersum >= halfsum) break;
				lowersum += chv[indx + i].value;
			}

			bv[bi].colors = i;
			bv[bi].sum = lowersum;
			bv[boxes].index = indx + i;
			bv[boxes].colors = clrs - i;
			bv[boxes].sum = sm - lowersum;
			++boxes;
			qsort((char*)bv, boxes, sizeof(struct box), sumcompare);
		}

		if (bFlip) histogram.clear();

		for (bi = 0; bi < boxes; ++bi)
		{
			/*
			register int indx = bv[bi].index;
			register int clrs = bv[bi].colors;
			register long r = 0, g = 0, b = 0;

			for (i = 0; i < clrs; ++i)
			{
				r += chv[indx + i].color.btRed;
				g += chv[indx + i].color.btGreen;
				b += chv[indx + i].color.btBlue;
			}

			r = r / clrs;
			g = g / clrs;
			b = b / clrs;
			
			colormap[bi].color = RGBX(r, g, b, 0);
			colorList.AddTail(CHZColor(RGB(r, g, b)));
			TRACE("%d - %d - %d\n", r, g, b);
			//colorList.AddTail(CHZColor(RGBX(r, g, b,0)));
			*/
			
			/*
			register int indx = bv[bi].index;
			register int clrs = bv[bi].colors;
			register long r = 0, g = 0, b = 0, sum = 0;

			for (i = 0; i < clrs; ++i)
			{
				r += chv[indx + i].color.btRed * chv[indx + i].value;
				g += chv[indx + i].color.btGreen * chv[indx + i].value;
				b += chv[indx + i].color.btBlue * chv[indx + i].value;
				sum += chv[indx + i].value;
			}
			r = r / sum;
			if (r > 255) r = 255;	//avoid math errors
			g = g / sum;
			if (g > 255) g = 255;
			b = b / sum;
			if (b > 255) b = 255;
			colormap[bi].color = RGBX(r, g, b, 0);
			colorList.AddTail(CHZColor(RGB(r, g, b)));
			TRACE("%d - %d - %d\n", r, g, b);
			*/



			register int indx = bv[bi].index;
			register int clrs = bv[bi].colors;
			register long r = 0, g = 0, b = 0;

			RGBX cl;
			int nDelta = 0;
			int nSum = 0;
			for (i = 0; i < clrs; ++i)
			{
				if (chv[indx + i].value > nDelta)
				{
					cl = chv[indx + i].color;
					nSum += chv[indx + i].value;
					nDelta = chv[indx + i].value;
				}
			}
			
			if (bFlip)
			{
				DWORD col = RGB(cl.btRed, cl.btGreen, cl.btBlue);
				histogram[col] = nSum;
			}

			//bFlip = !bFlip;

			colormap[bi].color = cl;
			colormap[bi].value = nSum;
			// Only add if its over 10 in difference

			colorList.AddTail(CHZColor(RGB(cl.btRed, cl.btGreen, cl.btBlue)));
			//TRACE("%d - %d - %d (%d)\n", cl.btRed, cl.btGreen, cl.btBlue, nSum);

		}
		
		if (false)
		{
			CString strPalette = L"<table><tr><th style=\"width:20px\">Actual Color</th><th style=\"width:30%\">Hex</th></tr><tr>\n";

			POSITION pos = colorList.GetHeadPosition();
			while (pos != NULL)
			{
				CString strColor, strColorVal;
				CString strColor2, strColorVal2;

				CHZColor temp = colorList.GetNext(pos);

				strPalette += L"<td style=\"background-color:" + temp.GetName() + L"\">&nbsp;</td><td>" + temp.GetName()
					+ L"&nbsp;</td></tr>\n";

				//strPalette +=  color.GetName() + "\t" + strColor  + "\r\n";
			}

			strPalette += L"</table>\n";

			CFile file;
			file.Open(L"D:\\palette2.htm", CFile::modeCreate | CFile::modeWrite, NULL);
			CT2CA outputString(strPalette, CP_UTF8);
			file.Write(outputString, ::strlen(outputString));
			file.Close();

			HINSTANCE hand = ShellExecute(NULL, L"open", L"D:\\palette2.htm", NULL, NULL, SW_SHOWNORMAL);
		}

		
		POSITION pos1, pos2;

		for (pos1 = colorList.GetHeadPosition(); (pos2 = pos1) != NULL;)
		{
			CHZColor color1 = colorList.GetNext(pos1);
			float* lab1 = color1.GetLab();
			float fDelta = sqrt(
				(lab1[0] * lab1[0]) +
				(lab1[1] * lab1[1]) +
				(lab1[2] * lab1[2]));

			if (fDelta < 0.21f)
			{
				colorList.RemoveAt(pos2);
				continue;
			}
		}

		
		while (colorList.GetCount() > nColors)
		{
			CHZColor cl1, cl2;
			int nSumDelta = 0;
			pos1 = colorList.GetHeadPosition();
			float ndelta = 9999999999999;
			//int ndelta = 9999999999999;

			while (pos1 != NULL)
			{
				CHZColor color1 = colorList.GetNext(pos1);
				POSITION pos2 = colorList.GetHeadPosition();
				while (pos2 != NULL)
				{
					CHZColor color2 = colorList.GetNext(pos2);
					//int ndelta2 = ;
					
					if (COLORREF(color1) != COLORREF(color2))
					{
						/*
						if (color1.GetNearestName() == color2.GetNearestName())
						{
							if (histogram.find(COLORREF(color1))->second > histogram.find(COLORREF(color2))->second)
								cl = color2;
							else cl = color1;
							break;
						}
						*/

						/*
						float* lab1 = color1.GetLab();
						float* lab2 = color2.GetLab();
						float fDelta = sqrt((lab2[0] - lab1[0]) * (lab2[0] - lab1[0]) * 2.0 +
							(lab2[1] - lab1[1]) * (lab2[1] - lab1[1]) +
							(lab2[2] - lab1[2]) * (lab2[2] - lab1[2]));
						*/
						
						//float f1 = color1.GetLabLuminance();
						//float f2 = color2.GetLabLuminance();
						//float fDelta = (max(f1, f2) + 0.05) / (min(f1, f2) + 0.05);

						
						// LAST USED!!!!
						int rmean = (color1.GetRed() + color2.GetRed()) / 2;
						int r = color1.GetRed() - color2.GetRed();
						int g = color1.GetGreen() - color2.GetGreen();
						int b = color1.GetBlue() - color2.GetBlue();
						float fDelta = sqrt((float)(((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
						

						/*
						float* lab1 = color1.GetLab();
						float* lab2 = color2.GetLab();
						float fDelta = sqrt(
							(lab2[1] - lab1[1]) * (lab2[1] - lab1[1]) +
							(lab2[2] - lab1[2]) * (lab2[2] - lab1[2]));
						*/
						//fDelta = fmod(abs(color1.GetHue() - color2.GetHue()), 360);
						//fDelta = (fDelta > 180 ? 360.0 - fDelta : fDelta) / 180.0;
						//fDelta += abs(color1.GetLabLuminance() - color2.GetLabLuminance()) / 2.0;

						if (fDelta < ndelta)
						{
							
							int nSum1 = histogram.find(COLORREF(color1))->second;
							int nSum2 = histogram.find(COLORREF(color2))->second;

							if (nSum1 > nSum2)
							{
								cl1 = color2;
								cl2 = color1;
								nSumDelta = nSum2;
							}
							else
							{
								cl1 = color1;
								cl2 = color2;
								nSumDelta = nSum1;
							}
							
							/*
							if ((lab1[1] * lab1[1] + lab1[2] * lab1[2]) >
								(lab2[1] * lab2[1] + lab2[2] * lab2[2]))
							{

								if (color1.GetRed() > 200 && color1.GetGreen() < 130 && color1.GetBlue() < 130)
								{
									CString strCol1 = color1.GetString();
									CString strCol2 = color2.GetString();
									CString strTemp = strCol1 + strCol2;
								}

								cl1 = color1;
								cl2 = color2;
							}
							else
							{
								cl1 = color2;
								cl2 = color1;
							}
							*/
							ndelta = fDelta;
						}
					}
				}
			}
			POSITION pos = colorList.Find(cl1);
			if (pos)
			{
				colorList.RemoveAt(pos);
				histogram.find(COLORREF(cl2))->second += nSumDelta;
			}
			else break;
			//if (pos != NULL) colorList.RemoveAt(pos);
			
		}

		if (false)
		{
			CString strPalette = L"<table><tr><th style=\"width:20px\">Actual Color</th><th style=\"width:30%\">Hex</th></tr><tr>\n";

			POSITION pos = colorList.GetHeadPosition();
			while (pos != NULL)
			{
				CString strColor, strColorVal;
				CString strColor2, strColorVal2;

				CHZColor temp = colorList.GetNext(pos);

				strPalette += L"<td style=\"background-color:" + temp.GetName() + L"\">&nbsp;</td><td>" + temp.GetName()
					+ L"&nbsp;</td></tr>\n";

				//strPalette +=  color.GetName() + "\t" + strColor  + "\r\n";
			}

			strPalette += L"</table>\n";

			CFile file;
			file.Open(L"D:\\palette3.htm", CFile::modeCreate | CFile::modeWrite, NULL);
			CT2CA outputString(strPalette, CP_UTF8);
			file.Write(outputString, ::strlen(outputString));
			file.Close();

			HINSTANCE hand = ShellExecute(NULL, L"open", L"D:\\palette3.htm", NULL, NULL, SW_SHOWNORMAL);
		}
		free(bv);

		
	}
	catch (...)
	{

	}

}


BOOL CHZBitmap::ProcessImage(CHZBitmapProcessorArray& aProcessors)
{
	
	if (!GetSafeHandle()) return FALSE;
	if (!aProcessors.GetSize()) return TRUE;
	BOOL bRes = TRUE;

	try
	{

		// retrieve src and final dest sizes
		BITMAP BM;
		if (!GetBitmap(&BM)) return FALSE;
		CSize sizeSrc(BM.bmWidth, BM.bmHeight);
		CSize sizeDest(sizeSrc), sizeMax(sizeSrc);

		int nProcessor, nCount = (int)aProcessors.GetSize();
		for (nProcessor = 0; nProcessor < nCount; nProcessor++)
		{
			sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeDest);
			sizeMax = ((sizeMax.cx*sizeMax.cy)>(sizeDest.cx*sizeDest.cy))?sizeMax:sizeDest;
		}

		// prepare src and dest bits
		RGBX* pSrcPixels = GetDIBits32();
		if (!pSrcPixels) return FALSE;
		if (sizeMax.cx<0) return FALSE;

		RGBX* pDestPixels = new RGBX[sizeMax.cx * sizeMax.cy];
		if (!pDestPixels) return FALSE;
		//memset(pDestPixels, 0, sizeMax.cx * sizeMax.cy * 4);
		//Fill(pDestPixels, sizeMax, m_crBkgnd, 0);

		
		sizeDest = sizeSrc;

		// do the processing
		for (nProcessor = 0; bRes && nProcessor < nCount; nProcessor++)
		{
			DWORD dwTicks = GetTickCount();
		
			// if its the second processor or later then we need to copy
			// the previous dest bits back into source.
			// we also need to check that sizeSrc is big enough
			if (nProcessor > 0)
			{
				if ((sizeSrc.cx*sizeSrc.cy)<(sizeDest.cx*sizeDest.cy))
				{
					delete [] pSrcPixels;
					pSrcPixels = new RGBX[sizeDest.cx * sizeDest.cy];
				}

				CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeDest.cy); // default
				FillMemory(pDestPixels, sizeDest.cx * sizeDest.cy * 4, 0);
			}

			sizeSrc = sizeDest;
			sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeSrc);	
			bRes = aProcessors[nProcessor]->ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
			
			TRACE(L"\tProcess\t" + aProcessors[nProcessor]->GetFunctionName() + \
				CString("            ").Mid(aProcessors[nProcessor]->GetFunctionName().GetLength()) + \
				L"\ttook \t%.3f\n", (float)(GetTickCount()-dwTicks)/1000.0f);
			
		}

		int width = sizeDest.cx;
		int height = sizeDest.cy;


		CHZBitmap bmpNew;
		bmpNew.CreateImage(width, height, true);
		bmpNew.SetBitmapBits(width * height * 4, pDestPixels);
		DeleteObject();
		//bRes = Attach(bmpNew.Detach());
		bRes = Attach((HBITMAP)bmpNew.Detach());
		bmpNew.DeleteObject();

		delete [] pSrcPixels;
		delete [] pDestPixels;
	}
	catch (...)
	{

		#ifdef _DEBUG
		AfxMessageBox(L"Error with processors");
		#endif
		bRes = false;
	}
	
	return bRes;
}

RGBX* CHZBitmap::GetDIBits32()
{
	HBITMAP hBitmap = (HBITMAP)GetSafeHandle();
	BITMAP BM;
	if (!::GetObject(hBitmap, sizeof(BM), &BM)) return 0;
	BYTE* bmpBuffer= new BYTE[BM.bmWidthBytes*BM.bmHeight];
	GetBitmapBits(BM.bmWidthBytes*BM.bmHeight, bmpBuffer);
	return (RGBX*)bmpBuffer;
}

BOOL CHZBitmap::PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap)
{
	if (!hBitmap)
		hBitmap = (HBITMAP)GetSafeHandle();

	BITMAP BM;

	if (!::GetObject(hBitmap, sizeof(BM), &BM))
		return FALSE;

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = BM.bmWidth;
	bi.bmiHeader.biHeight = -BM.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32; // 32 bit
	bi.bmiHeader.biCompression = BI_RGB; // 32 bit
	bi.bmiHeader.biSizeImage = BM.bmWidth * 4 * BM.bmHeight; // 32 bit
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	return BM.bmHeight;
}

int CHZBitmap::GetWidth()
{
	HBITMAP hBitmap = (HBITMAP)GetSafeHandle();
	BITMAP BM;
	if (!::GetObject(hBitmap, sizeof(BM), &BM)) return 0;
	return BM.bmWidth;
}

int CHZBitmap::GetHeight()
{
	HBITMAP hBitmap = (HBITMAP)GetSafeHandle();
	BITMAP BM;
	if (!::GetObject(hBitmap, sizeof(BM), &BM)) return 0;
	return BM.bmHeight;
}

BOOL CHZBitmap::CopyImage(HBITMAP hBitmap)
{
	ASSERT (hBitmap);
	if (!hBitmap) return FALSE;
	HBITMAP hbmp = (HBITMAP)::CopyImage(hBitmap,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	//Detach();
	DeleteObject();
	bool res = Attach(hbmp);
	return res;
}

BOOL CHZBitmap::CopyImage(CBitmap* pBitmap)
{
	bool res;
	try
	{
		if (!pBitmap) return FALSE;
		HBITMAP hbmp = (HBITMAP)::CopyImage((HBITMAP)pBitmap->GetSafeHandle(),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
		//Detach();
		DeleteObject();
		res = Attach(hbmp);
	}
	catch (...)
	{
		
	}
	return res;
}


BOOL CHZBitmap::Fill(RGBX* pPixels, CSize size, COLORREF color, int alpha)
{
	if  (!pPixels) return FALSE;
	if (color==-1) color = 0;

	// fill the first line with the color
	RGBX* pLine = &pPixels[0];
	int nSize = 1;

	pLine[0] = RGBX(color, alpha);

	while (1)
	{
		if (nSize > size.cx) break;
		// else
		int nAmount = min(size.cx - nSize, nSize) * 4;
		CopyMemory(&pLine[nSize], pLine, nAmount);
		nSize *= 2;
	}

	// use that line to fill the rest of the block
	int nRow = 1;

	while (1)
	{
		if (nRow > size.cy) break;
		// else
		int nAmount = min(size.cy - nRow, nRow) * size.cx * 4;
		CopyMemory(&pPixels[nRow * size.cx], pPixels, nAmount);
		nRow *= 2;
	}

	return TRUE;
}

IMPLEMENT_SERIAL(CHZBitmap, CBitmap, 1);
void CHZBitmap:: Serialize (CArchive &ar)
{
	if (ar.IsStoring())
	{

		HBITMAP hBitmap = (HBITMAP)GetSafeHandle();
		BITMAP BM;
		int nWidth, nHeight;
		CByteArray arrBytes;

		if (!::GetObject(hBitmap, sizeof(BM), &BM)) 
		{
			nWidth = 0;
			nHeight = 0;
			arrBytes.SetSize(0);
		}
		else
		{
			nWidth = BM.bmWidth;
			nHeight = BM.bmHeight;

			BYTE* bmpBuffer= new BYTE[BM.bmWidthBytes*BM.bmHeight];
			GetBitmapBits(BM.bmWidthBytes*BM.bmHeight, bmpBuffer);
			arrBytes.SetSize(BM.bmWidthBytes*BM.bmHeight);
			::CopyMemory(arrBytes.GetData(), bmpBuffer, BM.bmWidthBytes*BM.bmHeight);
			delete[] bmpBuffer;
		}

		ar << nWidth;
		ar << nHeight;
		arrBytes.Serialize(ar);
		
		/*
		if (IsNull()) return;
		BITMAP hBmp;
		GetBitmap (&hBmp);
		ar << (long) hBmp.bmWidth;
		ar << (long) hBmp.bmHeight;
		ar << (long) hBmp.bmType;
		ar << (long) hBmp.bmWidthBytes;
		ar << (WORD) hBmp.bmPlanes;
		ar << (WORD) hBmp.bmBitsPixel;
		BITMAPINFO *pbmi;
		BITMAPFILEHEADER bmfh;
		HDC hDC = NULL;
		DWORD wHeaderSize;
		void * pBits;
		wHeaderSize = sizeof (BITMAPINFOHEADER);
		pbmi = (PBITMAPINFO) new char [wHeaderSize];
		pbmi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
		pbmi->bmiHeader.biPlanes = hBmp.bmPlanes;
		pbmi->bmiHeader.biBitCount = hBmp.bmBitsPixel;
		pbmi->bmiHeader.biHeight = hBmp.bmHeight;
		pbmi->bmiHeader.biWidth = hBmp.bmWidth;
		pbmi->bmiHeader.biCompression = BI_RGB;
		pbmi->bmiHeader.biClrUsed = 0;
		pbmi->bmiHeader.biClrImportant = 0;
		pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount +31) && ~31)/8 * pbmi->bmiHeader.biHeight;
		hDC =:: CreateCompatibleDC(NULL);
		pBits = new char [pbmi->bmiHeader.biSizeImage];
		HBITMAP hBitmap = (HBITMAP)(this->GetSafeHandle());
		if (!::GetDIBits(hDC, hBitmap, 0, (WORD) pbmi->bmiHeader.biHeight, pBits, pbmi, DIB_RGB_COLORS))
		bmfh.bfType = 0x4d42;
		bmfh.bfReserved1 = 0;
		bmfh.bfReserved2 = 0;
		bmfh.bfSize = sizeof (BITMAPFILEHEADER) + wHeaderSize + pbmi->bmiHeader.biSizeImage;
		bmfh.bfOffBits = (DWORD) sizeof (BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + pbmi->bmiHeader.biClrUsed * sizeof (RGBQUAD);
		ar.Write(&bmfh, sizeof (BITMAPFILEHEADER));
		ar.Write(pbmi, wHeaderSize);
		ar.Write(pBits, pbmi->bmiHeader.biSizeImage);
		delete [] pBits;
		delete [] pbmi;
		*/
	}
	else
	{
	
		CByteArray arrBytes;

		int width, height;
		ar >> width;
		ar >> height;
		arrBytes.Serialize(ar);

		if (width>0 && height>0)
		{
			CreateImage(width, height, true);
			SetBitmapBits(width * height * 4, arrBytes.GetData());
		}
		

		/*
		static BITMAP hBmp;
		ar >> hBmp.bmWidth;
		ar >> hBmp.bmHeight;
		ar >> hBmp.bmType;
		ar >> hBmp.bmWidthBytes;
		ar >> hBmp.bmPlanes;
		ar >> hBmp.bmBitsPixel;
		BITMAPINFO *pbmi;
		BITMAPFILEHEADER bmfh;
		DWORD wHeaderSize;
		void * pBits;
		wHeaderSize = sizeof(BITMAPINFOHEADER);
		pbmi = (PBITMAPINFO) new char [wHeaderSize];
		ar.Read(&bmfh, sizeof (BITMAPFILEHEADER));
		ar.Read(pbmi, wHeaderSize);
		pBits = new char [pbmi-> bmiHeader.biSizeImage];
		ar.Read(pBits, pbmi->bmiHeader.biSizeImage);
		HBITMAP hLoadedBmp =::CreateDIBitmap(CWindowDC(CWnd::GetDesktopWindow()), &pbmi->bmiHeader, CBM_INIT, pBits, pbmi, DIB_RGB_COLORS);
		if(DeleteObject()) Detach();
		Attach(hLoadedBmp);
		delete[] pBits;
		delete[] pbmi;
		*/
	}
}