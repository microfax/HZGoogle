// ImageProcessors.cpp: CHZBitmapProcessor derivations (c) daniel godson 2002.
//
// credits: Karl Lager's 'A Fast Algorithm for Rotating Bitmaps' 
// 
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "HZBitmapProcessors.h"

#include "HZColor.h"

#include <math.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#include <emmintrin.h>
//#include <stdint.h>

#define round(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))
//#define round(x) floorz(x)
//#define avg(x,y) (min(255, x + INT_ALPHA[max(0,y)][255-x]))
//#define bnd(x,y) ((x+y)>310)?avg(x,y):(max(y,x))
//#define bnd(x,y) ((x+y)>320)?255:(max(y,x))
#define bnd(x,y) max(x,y)
//#define bnd(x,y) ((x+y)>400)?255:(max(y,x)) << PREFER THIS, but causes errors on sweep
//#define avg(x,y) ((x+y)>509)?255:(max(y,x))
#define avg(x,y) (min(255, x + INT_ALPHA[max(0,y)][255-x]))
#define rvg(x,y) (min(255, y + INT_ALPHA[max(0,x)][255-y]))
//#define bnd(x,y) (max(max(x,y), x + INT_ALPHA[max(0,y)][255-x]))
//#define avg(x,y) max(x,y)
//#define avg(x,y) min((x==0)?y:x,y)
//#define avg(x,y) (min(255, x + INT_ALPHA[max(0,y)][255-x]))
#define sqr(x) (x*x)
//#define floorz(x) ((int)(x + 32768.) - 32768)
//#define ceilz(x) (32768 - (int)(32768. - x))
#define floorz(x) ((int)(x))
#define ceilz(x) ((int)((x)+1))
//#define floorz(x) floorf(x)
//#define ceilz(x) ceilf(x)
//#define sqrtz(x) sqrtf(x)

#define SQRT_MAGIC_F 0x5f3759df 


#if _WIN32 || _WIN64
	#if _WIN64
	
	inline float sqrtz(const float x)
	{
		//return sqrtf(x);
		const float xhalf = 0.5f*x;
		union // get bits for floating value
		{
			float x;
			int i;
		} u;
		u.x = x;
		u.i = SQRT_MAGIC_F - (u.i >> 1);  // gives initial guess y0
		return x*u.x*(1.5f - xhalf*u.x*u.x);// Newton step, repeating increases accuracy 
	}


	#else

	double inline __declspec (naked) __fastcall sqrtz(double n)
	{
		_asm fld qword ptr [esp+4]
		_asm fsqrt
		_asm ret 8
	} 

	#endif
#endif


/*
double sqrtz(double n)
{
  __asm{
     fld n
     fsqrt
   }
}
*/

//#define avg(x,y) (x&y)+((x^y)>>1)
//#define round(x) ceil(x)



/////////////////////////////////////////////////////////////////////
// CHZBitmapProcessor derivations

const double PI = 3.14159265358979323846;



CHZBitmapRotator::CHZBitmapRotator(int nDegrees, BOOL bEnableWeighting)
	: CHZBitmapProcessor(bEnableWeighting)
{
	// normalize the angle
	while (nDegrees >= 360)
		nDegrees -= 360;

	while (nDegrees < 0)
		nDegrees += 360;

	ASSERT (nDegrees >= 0 && nDegrees < 360);

	m_dRadians = nDegrees * PI / 180;
}

CHZBitmapRotator::CHZBitmapRotator(double dRadians)
{
	m_strFunctionName = "Rotator";
	// normalize the angle
	while (dRadians >= 2 * PI)
		dRadians -= 2 * PI;

	while (dRadians <= 0)
		dRadians += 2 * PI;

	ASSERT (dRadians >= 0 && dRadians < 2 * PI);

	m_dRadians = dRadians;
}

CHZBitmapRotator::~CHZBitmapRotator()
{
}

CSize CHZBitmapRotator::CalcDestSize(CSize sizeSrc)
{
	if (!m_dRadians || !sizeSrc.cx || !sizeSrc.cy)
		return sizeSrc;

	// calculate the four rotated corners
	double dCosA = cos(m_dRadians);
	double dSinA = sin(m_dRadians);

	CPoint ptTopLeft, ptTopRight, ptBottomLeft, ptBottomRight;

	ptTopLeft.x = (int)(-sizeSrc.cx * dCosA / 2 + sizeSrc.cy * dSinA / 2);
	ptTopLeft.y = (int)(sizeSrc.cy * dCosA / 2 - (-sizeSrc.cx) * dSinA / 2);

	ptTopRight.x = (int)(sizeSrc.cx * dCosA / 2 + sizeSrc.cy * dSinA / 2);
	ptTopRight.y = (int)(sizeSrc.cy * dCosA / 2 - sizeSrc.cx * dSinA / 2);

	ptBottomLeft.x = (int)(-sizeSrc.cx * dCosA / 2 + (-sizeSrc.cy) * dSinA / 2);
	ptBottomLeft.y = (int)(-sizeSrc.cy * dCosA / 2 - (-sizeSrc.cx) * dSinA / 2);

	ptBottomRight.x = (int)(sizeSrc.cx * dCosA / 2 + (-sizeSrc.cy) * dSinA / 2);
	ptBottomRight.y = (int)(-sizeSrc.cy * dCosA / 2 - sizeSrc.cx * dSinA / 2);

	// find the max absolute values in each direction
	int nMaxY = max(abs(ptTopLeft.y), max(abs(ptTopRight.y), max(abs(ptBottomLeft.y), abs(ptBottomRight.y))));
	int nMaxX = max(abs(ptTopLeft.x), max(abs(ptTopRight.x), max(abs(ptBottomLeft.x), abs(ptBottomRight.x))));
	
	return CSize((nMaxX + 1) * 2, (nMaxY + 1) * 2);
}

BOOL CHZBitmapRotator::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (!m_dRadians)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		// note: we also need to translate the coords after rotating
		CSize sizeDestOffset(sizeDest.cx / 2 + sizeDest.cx % 2, sizeDest.cy / 2 + sizeDest.cy % 2);
		CSize sizeSrcOffset(sizeSrc.cx / 2 + sizeSrc.cx % 2, sizeSrc.cy / 2 + sizeSrc.cy % 2);

		CRect rSrc(0, 0, sizeSrc.cx - 1, sizeSrc.cy - 1);
		rSrc.OffsetRect(-sizeSrcOffset);

		// note: traversing the src bitmap leads to artifacts in the destination image
		// what we do is to traverse the destination bitmaps and compute the equivalent 
		// source color - credit for this observation goes to Yves Maurer (GDIRotate) 2002
		double dCosA = cos(m_dRadians);
		double dSinA = sin(m_dRadians);

		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			// calc y components of rotation
			double dCosYComponent = (nY - sizeDestOffset.cy) * dCosA;
			double dSinYComponent = (nY - sizeDestOffset.cy) * dSinA;

			double dSrcX = -sizeDestOffset.cx * dCosA + dSinYComponent;
			double dSrcY = dCosYComponent - (-sizeDestOffset.cx * dSinA);

			for (int nX = 0; nX < sizeDest.cx; nX++)
			{
				dSrcX += dCosA;
				dSrcY -= dSinA;

				CPoint ptSrc((int)dSrcX, (int)dSrcY);
				int nPixel = (nY * sizeDest.cx + nX);

				if (rSrc.PtInRect(ptSrc))
				{
					if (!m_bWeightingEnabled)
					{
						ptSrc.Offset(sizeSrcOffset);
						RGBX* pRGBSrc = &pSrcPixels[ptSrc.y * sizeSrc.cx + ptSrc.x];
						
						pDestPixels[nPixel] = *pRGBSrc;
					}
					else
						pDestPixels[nPixel] = CalcWeightedColor(pSrcPixels, sizeSrc, 
															dSrcX + sizeSrcOffset.cx, dSrcY + sizeSrcOffset.cy);
				}
			}
		}
	}

	return bRes;
}

///////

CHZBitmapShearer::CHZBitmapShearer(int nHorz, int nVert, BOOL bEnableWeighting)
	: CHZBitmapProcessor(bEnableWeighting), m_nHorz(nHorz), m_nVert(nVert)
{
	m_strFunctionName = "Shearer";
}

CHZBitmapShearer::~CHZBitmapShearer()
{
}

CSize CHZBitmapShearer::CalcDestSize(CSize sizeSrc)
{
	return CSize(sizeSrc.cx + abs(m_nHorz), sizeSrc.cy + abs(m_nVert));
}

BOOL CHZBitmapShearer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (!m_nHorz && !m_nVert)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		// shears +ve (down) or -ve (up)
		for (int nX = 0; nX < sizeDest.cx; nX++)
		{
			double dYOffset = 0;

			// calc the offset to src Y coord
			if (m_nVert > 0)
				dYOffset = (double)m_nVert * nX / sizeDest.cx;
				
			else if (m_nVert < 0)
				dYOffset = (double)-m_nVert * (sizeDest.cx - nX) / sizeDest.cx;

			// shears +ve (right) or -ve (left)
			for (int nY = 0; nY < sizeDest.cy; nY++)
			{
				double dXOffset = 0;

				// calc the offset to src X coord
				if (m_nHorz < 0)
					dXOffset = (double)-m_nHorz * nY / sizeDest.cy;
				
				else if (m_nHorz > 0)
					dXOffset = (double)m_nHorz * (sizeDest.cy - nY) / sizeDest.cy;

				double dSrcX = nX - dXOffset;
				double dSrcY = nY - dYOffset;

				if ((int)dSrcX >= 0 && (int)dSrcX < sizeSrc.cx && (int)dSrcY >= 0 && (int)dSrcY < sizeSrc.cy)
				{
					if (!m_bWeightingEnabled)
					{
						RGBX* pRGBSrc = &pSrcPixels[(int)dSrcY * sizeSrc.cx + (int)dSrcX];
						pDestPixels[nY * sizeDest.cx + nX] = *pRGBSrc;
					}
					else
						pDestPixels[nY * sizeDest.cx + nX] = CalcWeightedColor(pSrcPixels, sizeSrc, dSrcX, dSrcY);
				}
			}
		}
	}
	
	return bRes;
}

///////

CHZBitmapCutter::CHZBitmapCutter(CHZBitmap* pBitmap, bool bInvert, bool bMonochrome)
{
	m_strFunctionName = "Cutter";
	m_pBitmap = pBitmap;
	m_bInvert = bInvert;
	m_bMonochrome = bMonochrome;
}

CHZBitmapCutter::~CHZBitmapCutter()
{
}

BOOL CHZBitmapCutter::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	if (m_pBitmap == NULL)
	{
		if (m_bInvert) memset(pDestPixels, 128, sizeDest.cx * sizeDest.cy * 4);
		else memset(pDestPixels, 255, sizeDest.cx * sizeDest.cy * 4);
		return TRUE;
	}

	RGBX* pSrcAPixels = m_pBitmap->GetDIBits32();

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrcB = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBSrcA = &pSrcAPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			int alpha = pRGBSrcA->btAlpha;
			if (!m_bInvert) alpha = 255 - alpha;

			if (alpha>0)
			{
				pRGBDest->btRed = INT_ALPHA[pRGBSrcB->btRed][alpha];
				pRGBDest->btGreen = INT_ALPHA[pRGBSrcB->btGreen][alpha];
				pRGBDest->btBlue = INT_ALPHA[pRGBSrcB->btBlue][alpha];
				pRGBDest->btAlpha = INT_ALPHA[(m_bMonochrome?pRGBSrcB->btAlpha:255)][alpha];
			}
		}
	}

	delete [] pSrcAPixels;


	return TRUE;
}


///////

CHZBitmapBlitter::CHZBitmapBlitter(CHZBitmap* pBitmap)
{
	m_strFunctionName = "Blitter";
	m_pBitmap = pBitmap;
}

CHZBitmapBlitter::~CHZBitmapBlitter()
{
}

BOOL CHZBitmapBlitter::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	RGBX* pSrcInPixels = m_pBitmap->GetDIBits32();
	if (!pSrcInPixels) return TRUE;

	int sum = 0;
	for (int i = 0; i < sizeSrc.cx*sizeSrc.cy; i++) 
	{
		sum |= pSrcPixels[i].btBlue;
		sum |= pSrcPixels[i].btGreen;
		sum |= pSrcPixels[i].btRed;
		sum |= pSrcPixels[i].btAlpha;
		if (sum>0) break;
	}
	if (sum == 0) 
	{ 
		CopyMemory(pDestPixels, pSrcInPixels, sizeDest.cx * 4 * sizeDest.cy); 
		delete [] pSrcInPixels;
		return TRUE; 
	}

	int alphaA, alphaB, alphaC, alphaZ;

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrcA = &pSrcInPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBSrcB = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			if ((pRGBSrcA->btAlpha + pRGBSrcB->btAlpha) > 0)
			{
				if (pRGBSrcA->btAlpha == 0 || pRGBSrcB->btAlpha == 0)
				{
					pRGBDest->btBlue = pRGBSrcA->btBlue+pRGBSrcB->btBlue;
					pRGBDest->btGreen = pRGBSrcA->btGreen+pRGBSrcB->btGreen;
					pRGBDest->btRed = pRGBSrcA->btRed+pRGBSrcB->btRed;
					pRGBDest->btAlpha = pRGBSrcA->btAlpha+pRGBSrcB->btAlpha;

					//RGBX* pRGBSrc = (pRGBSrcA->btAlpha == 0)?pRGBSrcB:pRGBSrcA;
					//memcpy(pRGBDest, pRGBSrc, sizeof(RGBX)); // PROFILER CHANGED
					//pRGBDest = pRGBSrcA & pRGBSrcB;
					/*
					pRGBDest->btAlpha = pRGBSrc->btAlpha;
					pRGBDest->btRed = pRGBSrc->btRed;
					pRGBDest->btGreen = pRGBSrc->btGreen;
					pRGBDest->btBlue = pRGBSrc->btBlue;
					*/

				}
				else
				{
					float fdiv = BETA[pRGBSrcB->btAlpha];
					pRGBSrcB->btRed *= fdiv;
					pRGBSrcB->btGreen *= fdiv;
					pRGBSrcB->btBlue *= fdiv;

					alphaA = pRGBSrcA->btAlpha;
					alphaB = pRGBSrcB->btAlpha;
					alphaZ = INT_ALPHA[alphaB][255-alphaA];
					alphaC = min(255, alphaA + alphaZ);

					if (alphaC>0)
					{
						pRGBDest->btAlpha = alphaC;
						pRGBDest->btRed = min(pRGBDest->btAlpha, pRGBSrcA->btRed + INT_ALPHA[pRGBSrcB->btRed][alphaZ]);
						pRGBDest->btGreen = min(pRGBDest->btAlpha, pRGBSrcA->btGreen + INT_ALPHA[pRGBSrcB->btGreen][alphaZ]);
						pRGBDest->btBlue = min(pRGBDest->btAlpha, pRGBSrcA->btBlue + INT_ALPHA[pRGBSrcB->btBlue][alphaZ]);
					}
				}
			}
		}
	}

	delete [] pSrcInPixels;

	return TRUE;
}


///////

CHZBitmapGrayer::CHZBitmapGrayer()
{
	m_strFunctionName = "Grayer";
}

CHZBitmapGrayer::~CHZBitmapGrayer()
{
}

BOOL CHZBitmapGrayer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			*pRGBDest = pRGBSrc->Gray();
		}
	}

	return TRUE;
}

///////


CHZBitmapShadower::CHZBitmapShadower(COLORREF crColor, int nRadius)
{
	m_strFunctionName = "Shadower";
	m_crColor = crColor;
	m_nRadius = nRadius;
}

CHZBitmapShadower::~CHZBitmapShadower()
{
}

void CHZBitmapShadower::BoxBlurH4(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{
	float iarr = 1.0f / (radius + radius + 1);

	for (int i=0; i<sizeSrc.cy; i++)
	{
		int ti = i*sizeSrc.cx;
		int li = ti;
		int ri = ti+radius;

		RGBX* pFV = &pSrcPixels[ti];
		RGBX* pLV = &pSrcPixels[ti+sizeSrc.cx-1];

		int red = (radius + 1) * pFV->btAlpha;
		int j;

		for (j=0; j<radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ti+j];
			red += pRGBX->btAlpha;
		}

		for (j=0; j<=radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ri++];
			red += pRGBX->btAlpha - pFV->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btAlpha = round(red*iarr);
		}

		for (j=radius+1; j<sizeSrc.cx-radius; j++)
		{
			RGBX* pRGBX1 = &pSrcPixels[ri++];
			RGBX* pRGBX2 = &pSrcPixels[li++];
			red += pRGBX1->btAlpha - pRGBX2->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btAlpha = round(red*iarr);
		}

		for (j=sizeSrc.cx-radius; j<sizeSrc.cx; j++)
		{
			RGBX* pRGBX = &pSrcPixels[li++];
			red += pLV->btAlpha - pRGBX->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btAlpha = round(red*iarr);
		}
	}
}


void CHZBitmapShadower::BoxBlurT4(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{

	float iarr = 1.0f / (radius + radius + 1);
	
	for (int i=0; i<sizeSrc.cx; i++)
	{
		int ti = i;
		int li = ti;
		int ri = ti+radius*sizeSrc.cx;

		RGBX* pFV = &pSrcPixels[ti];
		RGBX* pLV = &pSrcPixels[ti+sizeSrc.cx*(sizeSrc.cy-1)];

		int red = (radius + 1) * pFV->btAlpha;
		int j;

		for (j=0; j<radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ti+j*sizeSrc.cx];
			red += pRGBX->btAlpha;
		}

		for (j=0; j<=radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ri];
			red += pRGBX->btAlpha - pFV->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btAlpha = round(red*iarr);
			ri+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}

		for (j=radius+1; j<sizeSrc.cy-radius; j++)
		{
			RGBX* pRGBX1 = &pSrcPixels[ri];
			RGBX* pRGBX2 = &pSrcPixels[li];
			red += pRGBX1->btAlpha - pRGBX2->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btAlpha = round(red*iarr);
			li+=sizeSrc.cx;
			ri+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}

		for (j=sizeSrc.cy-radius; j<sizeSrc.cy; j++)
		{
			RGBX* pRGBX = &pSrcPixels[li];
			red += pLV->btAlpha - pRGBX->btAlpha;
			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btAlpha = round(red*iarr);
			li+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}
	}
}

BOOL CHZBitmapShadower::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	int radius = m_nRadius;
	int sigma = radius;
	int n = 1;

	float wIdeal = sqrt((float)(12*sigma*sigma/n)+1); // Ideal averaging filter width 
	int wl = floor(wIdeal);
	if (wl%2==0) wl--;
	int wu = wl+2;
	int mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
	int m = round(mIdeal);

	radius = ((2<m?wl:wu)-1)/2;
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy);
	BoxBlurH4(pDestPixels, sizeSrc, pSrcPixels, sizeDest, radius);
	BoxBlurT4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, radius);

	radius = ((1<m?wl:wu)-1)/2;
	CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeSrc.cy);
	BoxBlurH4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, radius);
	BoxBlurT4(pDestPixels, sizeSrc, pSrcPixels, sizeDest, radius);

	//radius = ((0<m?wl:wu)-1)/2;
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy);
	//BoxBlurH4(pDestPixels, sizeSrc, pSrcPixels, sizeDest, radius);
	//BoxBlurT4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, radius);
	
	int red = GetRValue(m_crColor);
	int green = GetGValue(m_crColor);
	int blue = GetBValue(m_crColor);

	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			
			pRGBDest->btAlpha = pRGBDest->btAlpha;// * (1.0f + float(radius)/10.0f));

			int alpha =  pRGBDest->btAlpha;

			pRGBDest->btAlpha = pRGBDest->btAlpha;
			pRGBDest->btRed = INT_ALPHA[red][alpha];
			pRGBDest->btGreen = INT_ALPHA[green][alpha];
			pRGBDest->btBlue = INT_ALPHA[blue][alpha];
		}
	}

	return TRUE;
}


///////

CHZBitmapHorizontalBlurrer::CHZBitmapHorizontalBlurrer(int nAmount)
{
	m_strFunctionName = "HorizontalBlurrer";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapHorizontalBlurrer::~CHZBitmapHorizontalBlurrer()
{
}


void CHZBitmapHorizontalBlurrer::BoxBlur4(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	BoxBlurH4(pDestPixels, sizeSrc, pSrcPixels, sizeDest, radius);
	BoxBlurT4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, radius);
}

void CHZBitmapHorizontalBlurrer::BoxBlurH4(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{

	float iarr = 1.0f / (radius + radius + 1);
	
	for (int i=0; i<sizeSrc.cy; i++)
	{
		int ti = i*sizeSrc.cx;
		int li = ti;
		int ri = ti+radius;

		RGBX* pFV = &pSrcPixels[ti];
		RGBX* pLV = &pSrcPixels[ti+sizeSrc.cx-1];

		int red = 0, green = 0, blue = 0, alpha = 0;

		red = (radius + 1) * pFV->btRed;
		green = (radius + 1) * pFV->btGreen;
		blue = (radius + 1) * pFV->btBlue;
		alpha = (radius + 1) * pFV->btAlpha;

		int j;

		for (j=0; j<radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ti+j];
			red += pRGBX->btRed;
			green += pRGBX->btGreen;
			blue += pRGBX->btBlue;
			alpha += pRGBX->btAlpha;
		}

		for (j=0; j<=radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ri++];
			red += pRGBX->btRed - pFV->btRed;
			green += pRGBX->btGreen - pFV->btGreen;
			blue += pRGBX->btBlue - pFV->btBlue;
			alpha += pRGBX->btAlpha - pFV->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);
		}

		for (j=radius+1; j<sizeSrc.cx-radius; j++)
		{
			RGBX* pRGBX1 = &pSrcPixels[ri++];
			RGBX* pRGBX2 = &pSrcPixels[li++];
			red += pRGBX1->btRed - pRGBX2->btRed;
			green += pRGBX1->btGreen - pRGBX2->btGreen;
			blue += pRGBX1->btBlue - pRGBX2->btBlue;
			alpha += pRGBX1->btAlpha - pRGBX2->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);
		}

		for (j=sizeSrc.cx-radius; j<sizeSrc.cx; j++)
		{
			RGBX* pRGBX = &pSrcPixels[li++];
			red += pLV->btRed - pRGBX->btRed;
			green += pLV->btGreen - pRGBX->btGreen;
			blue += pLV->btBlue - pRGBX->btBlue;
			alpha += pLV->btAlpha - pRGBX->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti++];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);
		}
	}
}


void CHZBitmapHorizontalBlurrer::BoxBlurT4(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{

	float iarr = 1.0f / (radius + radius + 1);
	
	for (int i=0; i<sizeSrc.cx; i++)
	{
		int ti = i;
		int li = ti;
		int ri = ti+radius*sizeSrc.cx;

		RGBX* pFV = &pSrcPixels[ti];
		RGBX* pLV = &pSrcPixels[ti+sizeSrc.cx*(sizeSrc.cy-1)];

		int red = 0, green = 0, blue = 0, alpha = 0;

		red = (radius + 1) * pFV->btRed;
		green = (radius + 1) * pFV->btGreen;
		blue = (radius + 1) * pFV->btBlue;
		alpha = (radius + 1) * pFV->btAlpha;

		int j;

		for (j=0; j<radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ti+j*sizeSrc.cx];
			red += pRGBX->btRed;
			green += pRGBX->btGreen;
			blue += pRGBX->btBlue;
			alpha += pRGBX->btAlpha;
		}

		for (j=0; j<=radius; j++)
		{
			RGBX* pRGBX = &pSrcPixels[ri];
			red += pRGBX->btRed - pFV->btRed;
			green += pRGBX->btGreen - pFV->btGreen;
			blue += pRGBX->btBlue - pFV->btBlue;
			alpha += pRGBX->btAlpha - pFV->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);

			ri+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}

		for (j=radius+1; j<sizeSrc.cy-radius; j++)
		{
			RGBX* pRGBX1 = &pSrcPixels[ri];
			RGBX* pRGBX2 = &pSrcPixels[li];
			red += pRGBX1->btRed - pRGBX2->btRed;
			green += pRGBX1->btGreen - pRGBX2->btGreen;
			blue += pRGBX1->btBlue - pRGBX2->btBlue;
			alpha += pRGBX1->btAlpha - pRGBX2->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);

			li+=sizeSrc.cx;
			ri+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}

		for (j=sizeSrc.cy-radius; j<sizeSrc.cy; j++)
		{
			RGBX* pRGBX = &pSrcPixels[li];
			red += pLV->btRed - pRGBX->btRed;
			green += pLV->btGreen - pRGBX->btGreen;
			blue += pLV->btBlue - pRGBX->btBlue;
			alpha += pLV->btAlpha - pRGBX->btAlpha;

			RGBX* pRGBDest = &pDestPixels[ti];
			pRGBDest->btRed = round(red*iarr);
			pRGBDest->btGreen = round(green*iarr);
			pRGBDest->btBlue = round(blue*iarr);
			pRGBDest->btAlpha = round(alpha*iarr);

			li+=sizeSrc.cx;
			ti+=sizeSrc.cx;
		}
	}
}

BOOL CHZBitmapHorizontalBlurrer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);

		DWORD dwTick = GetTickCount();

		float r = m_nAmount - 0.5;
		
		float sigma = m_nAmount - 0.5;
		int n = 3;

		float wIdeal = sqrt((float)(12*sigma*sigma/n)+1); // Ideal averaging filter width 
		int wl = floor(wIdeal);
		if (wl%2==0) wl--;
		int wu = wl+2;
		int mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
		int m = round(mIdeal);

		CUIntArray bxs;
		for (int i=0; i<n; i++)
		{
			bxs.InsertAt(0,i<m?wl:wu);
		}

		BoxBlur4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, (bxs[0]-1)/2);
		BoxBlur4(pDestPixels, sizeSrc, pSrcPixels, sizeDest, (bxs[1]-1)/2);
		BoxBlur4(pSrcPixels, sizeSrc, pDestPixels, sizeDest, (bxs[2]-1)/2);

	}

	return TRUE;
}


///////

CHZBitmapFastBlurrer::CHZBitmapFastBlurrer(int nAmount)
{
	m_strFunctionName = "FastBlurrer";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapFastBlurrer::~CHZBitmapFastBlurrer()
{
}


void CHZBitmapFastBlurrer::BoxBlur3(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default

	BoxBlurH3(pDestPixels, sizeSrc, pSrcPixels, sizeDest, radius);
	BoxBlurT3(pSrcPixels, sizeSrc, pDestPixels, sizeDest, radius);
}

void CHZBitmapFastBlurrer::BoxBlurH3(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{
	for (int i=0; i<sizeSrc.cy; i++)
	{
		for (int j=0; j<sizeSrc.cx; j++)
		{
			int red = 0, green = 0, blue = 0, alpha = 0;

			for (int ix=j-radius; ix<j+radius+1; ix++)
			{
				int x = min(sizeSrc.cx-1, max(0, ix));

				RGBX* pSrc = &pSrcPixels[i*sizeSrc.cx+x];

				red += pSrc->btRed;
				green += pSrc->btGreen;
				blue += pSrc->btBlue;
				alpha += pSrc->btAlpha;

			}

			RGBX* pRGBDest = &pDestPixels[i*sizeSrc.cx+j];
			pRGBDest->btRed = red/(radius+radius+1);
			pRGBDest->btGreen = green/(radius+radius+1);
			pRGBDest->btBlue = blue/(radius+radius+1);
			pRGBDest->btAlpha = alpha/(radius+radius+1);
		}
	}
}


void CHZBitmapFastBlurrer::BoxBlurT3(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest, int radius)
{

	for (int i=0; i<sizeSrc.cy; i++)
	{
		for (int j=0; j<sizeSrc.cx; j++)
		{
			int red = 0, green = 0, blue = 0, alpha = 0;

			for (int iy=i-radius; iy<i+radius+1; iy++)
			{
				int y = min(sizeSrc.cy-1, max(0, iy));

				RGBX* pSrc = &pSrcPixels[y*sizeSrc.cx+j];

				red += pSrc->btRed;
				green += pSrc->btGreen;
				blue += pSrc->btBlue;
				alpha += pSrc->btAlpha;

			}

			RGBX* pRGBDest = &pDestPixels[i*sizeSrc.cx+j];
			pRGBDest->btRed = red/(radius+radius+1);
			pRGBDest->btGreen = green/(radius+radius+1);
			pRGBDest->btBlue = blue/(radius+radius+1);
			pRGBDest->btAlpha = alpha/(radius+radius+1);
		}
	}
}

BOOL CHZBitmapFastBlurrer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);

		DWORD dwTick = GetTickCount();

		int r = m_nAmount - 1;
		
		int sigma = m_nAmount - 1;
		int n = 3;

		float wIdeal = sqrt((float)(12*sigma*sigma/n)+1); // Ideal averaging filter width 
		int wl = floor(wIdeal);
		if (wl%2==0) wl--;
		int wu = wl+2;
		int mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
		int m = round(mIdeal);

		CUIntArray bxs;
		for (int i=0; i<n; i++)
		{
			bxs.Add(i<m?wl:wu);
		}

		BoxBlur3(pSrcPixels, sizeSrc, pDestPixels, sizeDest, (bxs[0]-1)/2);
		BoxBlur3(pDestPixels, sizeSrc, pSrcPixels, sizeDest, (bxs[1]-1)/2);
		BoxBlur3(pSrcPixels, sizeSrc, pDestPixels, sizeDest, (bxs[2]-1)/2);

	}


	return TRUE;
}


///////

CHZBitmapBoxBlurrer::CHZBitmapBoxBlurrer(int nAmount)
{
	m_strFunctionName = "BoxBlurrer";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapBoxBlurrer::~CHZBitmapBoxBlurrer()
{
}

BOOL CHZBitmapBoxBlurrer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);
		
		CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default

		static unsigned int stack_blur8_mul[] =
		{
			512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
			454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
			482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
			437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
			497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
			320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
			446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
			329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
			505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
			399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
			324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
			268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
			451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
			385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
			332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
			289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
		};

		static unsigned int stack_blur8_shr[] =
		{
			 9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
			17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
			20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
			21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
			21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
			22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
			22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
		};

		enum enumColorOrder { R, G, B, A };

		int w = sizeSrc.cx;
		int h = sizeSrc.cy;
		int r = m_nAmount + 1;

		unsigned x = 0, y = 0, xp = 0, yp = 0, i = 0, t = 0;
		unsigned stack_ptr = 0;
		unsigned stack_start = 0;

		unsigned char* src_pix_ptr = NULL;
		unsigned char* dst_pix_ptr = NULL;
		unsigned char* stack_pix_ptr = NULL;
		unsigned long* stack_data_ptr = NULL;
		unsigned long* lpStack = NULL;

		unsigned sum_r = 0;
		unsigned sum_g = 0;
		unsigned sum_b = 0;
		unsigned sum_a = 0;
		unsigned sum_in_r = 0;
		unsigned sum_in_g = 0;
		unsigned sum_in_b = 0;
		unsigned sum_in_a = 0;
		unsigned sum_out_r = 0;
		unsigned sum_out_g = 0;
		unsigned sum_out_b = 0;
		unsigned sum_out_a = 0;

		unsigned wm  = (w - 1);
		unsigned hm  = (h - 1);
		unsigned div = 0;
		unsigned mul_sum = 0;
		unsigned shr_sum = 0;
		unsigned row_addr = 0;
		unsigned stride = 0;

		if (w < 1 || h < 1 || r < 1) return false;
		else if (r > 254) r = 254;

		mul_sum = stack_blur8_mul[r];
		shr_sum = stack_blur8_shr[r];

		div = ((r + r) + 1);

		int iterations = 3;
		while (iterations-- > 0)
		{
			lpStack = new unsigned long[ div ];
			stack_data_ptr = lpStack;
			y = 0;

			while(++y < h)
			{
				sum_r = sum_g = sum_b = sum_a = 
				sum_in_r = sum_in_g = sum_in_b = sum_in_a = 
				sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

				row_addr = (y * w);
				src_pix_ptr = (unsigned char*) (pDestPixels + row_addr);
				i = 0;

				while(++i <= r)
				{
					t = (i + 1);

					stack_pix_ptr = (unsigned char*)(stack_data_ptr + i);

					*(stack_pix_ptr + R) = *(src_pix_ptr + R);
					*(stack_pix_ptr + G) = *(src_pix_ptr + G);
					*(stack_pix_ptr + B) = *(src_pix_ptr + B);
					*(stack_pix_ptr + A) = *(src_pix_ptr + A);

					sum_r += (*(stack_pix_ptr + R) * t);
					sum_g += (*(stack_pix_ptr + G) * t);
					sum_b += (*(stack_pix_ptr + B) * t);
					sum_a += (*(stack_pix_ptr + A) * t);

					sum_out_r += *(stack_pix_ptr + R);
					sum_out_g += *(stack_pix_ptr + G);
					sum_out_b += *(stack_pix_ptr + B);
					sum_out_a += *(stack_pix_ptr + A);

					if (i > 0)
					{
						t = (r + 1 - i);
		
						if (i <= wm) 
						{
							src_pix_ptr += 4; 
						}

						stack_pix_ptr = (unsigned char*)(stack_data_ptr + (i + r));

						*(stack_pix_ptr + R) = *(src_pix_ptr + R);
						*(stack_pix_ptr + G) = *(src_pix_ptr + G);
						*(stack_pix_ptr + B) = *(src_pix_ptr + B);
						*(stack_pix_ptr + A) = *(src_pix_ptr + A);

						sum_r += (*(stack_pix_ptr + R) * t);
						sum_g += (*(stack_pix_ptr + G) * t);
						sum_b += (*(stack_pix_ptr + B) * t);
						sum_a += (*(stack_pix_ptr + A) * t);

						sum_in_r += *(stack_pix_ptr + R);
						sum_in_g += *(stack_pix_ptr + G);
						sum_in_b += *(stack_pix_ptr + B);
						sum_in_a += *(stack_pix_ptr + A);
					}
				}

				stack_ptr = r;
				xp = r;

				if (xp > wm) xp = wm;

				src_pix_ptr = (unsigned char*)(pDestPixels + (xp + row_addr));
				dst_pix_ptr = (unsigned char*)(pDestPixels + row_addr);
				x = 0;

				while(++x < w)
				{
					*(dst_pix_ptr + R) = ((sum_r * mul_sum) >> shr_sum);
					*(dst_pix_ptr + G) = ((sum_g * mul_sum) >> shr_sum);
					*(dst_pix_ptr + B) = ((sum_b * mul_sum) >> shr_sum);
					*(dst_pix_ptr + A) = ((sum_a * mul_sum) >> shr_sum);

					dst_pix_ptr += 4;

					sum_r -= sum_out_r;
					sum_g -= sum_out_g;
					sum_b -= sum_out_b;
					sum_a -= sum_out_a;

					stack_start = (stack_ptr + div - r);

					if (stack_start >= div) 
					{
						stack_start -= div;
					}

					stack_pix_ptr = (unsigned char*)(stack_data_ptr + stack_start);

					sum_out_r -= *(stack_pix_ptr + R);
					sum_out_g -= *(stack_pix_ptr + G);
					sum_out_b -= *(stack_pix_ptr + B);
					sum_out_a -= *(stack_pix_ptr + A);

					if (xp < wm) 
					{
						src_pix_ptr += 4;
						++xp;
					}

					*(stack_pix_ptr + R) = *(src_pix_ptr + R);
					*(stack_pix_ptr + G) = *(src_pix_ptr + G);
					*(stack_pix_ptr + B) = *(src_pix_ptr + B);
					*(stack_pix_ptr + A) = *(src_pix_ptr + A);

					sum_in_r += *(stack_pix_ptr + R);
					sum_in_g += *(stack_pix_ptr + G);
					sum_in_b += *(stack_pix_ptr + B);
					sum_in_a += *(stack_pix_ptr + A);

					sum_r += sum_in_r;
					sum_g += sum_in_g;
					sum_b += sum_in_b;
					sum_a += sum_in_a;

					if (++stack_ptr >= div) 
					{
						stack_ptr = 0;
					}

					stack_pix_ptr = (unsigned char*)(stack_data_ptr + stack_ptr);

					sum_out_r += *(stack_pix_ptr + R);
					sum_out_g += *(stack_pix_ptr + G);
					sum_out_b += *(stack_pix_ptr + B);
					sum_out_a += *(stack_pix_ptr + A);

					sum_in_r -= *(stack_pix_ptr + R);
					sum_in_g -= *(stack_pix_ptr + G);
					sum_in_b -= *(stack_pix_ptr + B);
					sum_in_a -= *(stack_pix_ptr + A);
				}
			}
			

			stride = (w << 2);
			stack_data_ptr = lpStack;
			x = 0;
	
			while(++x < w)
			{
				sum_r = sum_g = sum_b = sum_a = 
				sum_in_r = sum_in_g = sum_in_b = sum_in_a = 
				sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

				src_pix_ptr = (unsigned char*)(pDestPixels + x);
				i = 0;

				while(++i <= r)
				{
					t = (i + 1);
	
					stack_pix_ptr = (unsigned char*)(stack_data_ptr + i);

					*(stack_pix_ptr + R) = *(src_pix_ptr + R);
					*(stack_pix_ptr + G) = *(src_pix_ptr + G);
					*(stack_pix_ptr + B) = *(src_pix_ptr + B);
					*(stack_pix_ptr + A) = *(src_pix_ptr + A);

					sum_r += (*(stack_pix_ptr + R) * t);
					sum_g += (*(stack_pix_ptr + G) * t);
					sum_b += (*(stack_pix_ptr + B) * t);
					sum_a += (*(stack_pix_ptr + A) * t);

					sum_out_r += *(stack_pix_ptr + R);
					sum_out_g += *(stack_pix_ptr + G);
					sum_out_b += *(stack_pix_ptr + B);
					sum_out_a += *(stack_pix_ptr + A);

					if (i > 0)
					{
						t = (r + 1 - i);
		
						if (i <= hm) 
						{
							src_pix_ptr += stride; 
						}

						stack_pix_ptr = (unsigned char*)(stack_data_ptr + (i + r));

						*(stack_pix_ptr + R) = *(src_pix_ptr + R);
						*(stack_pix_ptr + G) = *(src_pix_ptr + G);
						*(stack_pix_ptr + B) = *(src_pix_ptr + B);
						*(stack_pix_ptr + A) = *(src_pix_ptr + A);

						sum_r += (*(stack_pix_ptr + R) * t);
						sum_g += (*(stack_pix_ptr + G) * t);
						sum_b += (*(stack_pix_ptr + B) * t);
						sum_a += (*(stack_pix_ptr + A) * t);

						sum_in_r += *(stack_pix_ptr + R);
						sum_in_g += *(stack_pix_ptr + G);
						sum_in_b += *(stack_pix_ptr + B);
						sum_in_a += *(stack_pix_ptr + A);
					}
				}

				stack_ptr = r;
				yp = r;

				if (yp > hm) yp = hm;

				src_pix_ptr = (unsigned char*)(pDestPixels + (x + (yp * w)));
				dst_pix_ptr = (unsigned char*)(pDestPixels + x);
				y = 0;

				while(++y < h)
				{
					*(dst_pix_ptr + R) = ((sum_r * mul_sum) >> shr_sum);
					*(dst_pix_ptr + G) = ((sum_g * mul_sum) >> shr_sum);
					*(dst_pix_ptr + B) = ((sum_b * mul_sum) >> shr_sum);
					*(dst_pix_ptr + A) = ((sum_a * mul_sum) >> shr_sum);

					dst_pix_ptr += stride;

					sum_r -= sum_out_r;
					sum_g -= sum_out_g;
					sum_b -= sum_out_b;
					sum_a -= sum_out_a;

					stack_start = (stack_ptr + div - r);
					if (stack_start >= div)
					{
						stack_start -= div;
					}

					stack_pix_ptr = (unsigned char*)(stack_data_ptr + stack_start);

					sum_out_r -= *(stack_pix_ptr + R);
					sum_out_g -= *(stack_pix_ptr + G);
					sum_out_b -= *(stack_pix_ptr + B);
					sum_out_a -= *(stack_pix_ptr + A);

					if (yp < hm) 
					{
						src_pix_ptr += stride;
						++yp;
					}

					*(stack_pix_ptr + R) = *(src_pix_ptr + R);
					*(stack_pix_ptr + G) = *(src_pix_ptr + G);
					*(stack_pix_ptr + B) = *(src_pix_ptr + B);
					*(stack_pix_ptr + A) = *(src_pix_ptr + A);

					sum_in_r += *(stack_pix_ptr + R);
					sum_in_g += *(stack_pix_ptr + G);
					sum_in_b += *(stack_pix_ptr + B);
					sum_in_a += *(stack_pix_ptr + A);

					sum_r += sum_in_r;
					sum_g += sum_in_g;
					sum_b += sum_in_b;
					sum_a += sum_in_a;

					if (++stack_ptr >= div) 
					{
						stack_ptr = 0;
					}

					stack_pix_ptr = (unsigned char*)(stack_data_ptr + stack_ptr);

					sum_out_r += *(stack_pix_ptr + R);
					sum_out_g += *(stack_pix_ptr + G);
					sum_out_b += *(stack_pix_ptr + B);
					sum_out_a += *(stack_pix_ptr + A);

					sum_in_r -= *(stack_pix_ptr + R);
					sum_in_g -= *(stack_pix_ptr + G);
					sum_in_b -= *(stack_pix_ptr + B);
					sum_in_a -= *(stack_pix_ptr + A);
				}
			}
			
			delete lpStack;
			lpStack = NULL;
		}
		
		
	/*
		int w = sizeSrc.cx;
		int h = sizeSrc.cy;
		int radius = m_nAmount;// + 1;
		int wm = w-1;
		int hm = h-1;
		int wh = w*h;
		int div = radius+radius+1;

		int *vMIN = new int[max(w,h)];
		int *vMAX = new int[max(w,h)];

		unsigned char *r=new unsigned char[wh];
		unsigned char *g=new unsigned char[wh];
		unsigned char *b=new unsigned char[wh];
		unsigned char *a=new unsigned char[wh];

		unsigned char *dv = new unsigned char[256*div];
		for (int i=0;i<256*div;i++) dv[i]=(i/div);

		int iterations = 3;

		while (iterations-- > 0)
		{

			int yw = 0;
			int yi = 0;

			for (int y=0; y<h; y++)
			{
				int rsum = 0, gsum = 0, bsum = 0, asum = 0;
				for (int i=-radius;i<=radius;i++)
				{
					int p = (yi + min(wm, max(i,0)));

					RGBX* pRGBSrc = &pSrcPixels[p];

					rsum += pRGBSrc->btRed;
					gsum += pRGBSrc->btGreen;
					bsum += pRGBSrc->btBlue;
					asum += pRGBSrc->btAlpha;
				}
				for (int x=0;x<w;x++)
				{
					r[yi]=dv[rsum];
					g[yi]=dv[gsum];
					b[yi]=dv[bsum];
					a[yi]=dv[asum];

					if (y==0)
					{
						vMIN[x] = min(x+radius+1,wm);
						vMAX[x] = max(x-radius,0);
					}
					int p1 = (yw+vMIN[x]);
					int p2 = (yw+vMAX[x]);

					RGBX* pRGBSrc1 = &pSrcPixels[p1];
					RGBX* pRGBSrc2 = &pSrcPixels[p2];

					rsum += pRGBSrc1->btRed - pRGBSrc2->btRed;
					gsum += pRGBSrc1->btGreen - pRGBSrc2->btGreen;
					bsum += pRGBSrc1->btBlue - pRGBSrc2->btBlue;
					asum += pRGBSrc1->btAlpha - pRGBSrc2->btAlpha;

					yi++;
				}
				yw+=w;
			}

			for (int x=0;x<w;x++)
			{
				int rsum = 0, gsum = 0, bsum = 0, asum = 0;
				int yp = -radius*w;
				for(int i=-radius; i<=radius; i++)
				{
					yi = max(0,yp)+x;
					rsum += r[yi];
					gsum += g[yi];
					bsum += b[yi];
					asum += a[yi];
					yp+=w;
				}
	
				yi=x;

				for (int y=0; y<h; y++)
				{
				
					RGBX* pRGBDest = &pDestPixels[yi];

					pRGBDest->btRed = dv[rsum];
					pRGBDest->btGreen = dv[gsum];
					pRGBDest->btBlue = dv[bsum];
					pRGBDest->btAlpha = dv[asum];
				
					if(x==0)
					{
						vMIN[y] = min(y+radius+1,hm)*w;
						vMAX[y] = max(y-radius,0)*w;
					}

					int p1 = x+vMIN[y];
					int p2 = x+vMAX[y];

					rsum += r[p1]-r[p2];
					gsum += g[p1]-g[p2];
					bsum += b[p1]-b[p2];
					asum += a[p1]-a[p2];

					yi+=w;
				}
			}
		}

		delete[] r;
		delete[] g;
		delete[] b;
		delete[] a;

		delete[] vMIN;
		delete[] vMAX;
		delete[] dv;
		*/

	}
	
	return TRUE;
}

///////

CHZBitmapGaussianBlurrer::CHZBitmapGaussianBlurrer(int nAmount)
{
	m_strFunctionName = "GaussianBlurrer";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapGaussianBlurrer::~CHZBitmapGaussianBlurrer()
{
}

BOOL CHZBitmapGaussianBlurrer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);


		DWORD dwTick = GetTickCount();

		int r = m_nAmount;
		int rs = ceil(r * 2.57);

		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			for (int nX = 0; nX < sizeSrc.cx; nX++)
			{
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
				RGBX* pRGBSrc2 = &pSrcPixels[nY * sizeSrc.cx + nX];
				RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

				float red = 0, green = 0, blue = 0, alpha = 0;
				float wsum = 0;

				for(int iy = nY-rs; iy<nY+rs+1; iy++)
				{
					for(int ix = nX-rs; ix<nX+rs+1; ix++) 
					{
						
						float dsq = (ix-nX)*(ix-nX)+(iy-nY)*(iy-nY);
						float wght = exp(-dsq / (2*r*r) ) / (PI*2*r*r);

						int x = min(sizeSrc.cx-1, max(0, ix));
						int y = min(sizeSrc.cy-1, max(0, iy));
						pRGBSrc2 = &pSrcPixels[y * sizeSrc.cx + x];

						red += pRGBSrc2->btRed * wght;
						green += pRGBSrc2->btGreen * wght;
						blue += pRGBSrc2->btBlue * wght;
						alpha += pRGBSrc2->btAlpha * wght;

						wsum += wght;
					}
				}

				pRGBDest->btRed = round(red/wsum);
				pRGBDest->btGreen = round(green/wsum);
				pRGBDest->btBlue = round(blue/wsum);
				pRGBDest->btAlpha = round(alpha/wsum);

			}
		}

	}

	return TRUE;
}

///////



CHZBitmapDitherer::CHZBitmapDitherer(CHZBitmap* pBitmap, int nColors)
{
	m_pBitmap = pBitmap;
	m_nColors = nColors;
}

CHZBitmapDitherer::~CHZBitmapDitherer()
{
}


BOOL CHZBitmapDitherer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT(sizeSrc == sizeDest);

	CList<CHZColor, CHZColor&> lstColors;

	m_pBitmap->QuantizeImage(m_nColors, lstColors);
	
	// Dunno
	//lstColors.AddHead(CHZColor::FromString(L"#000000"));
	//lstColors.AddHead(CHZColor::FromString(L"#FF5555"));
	//lstColors.AddHead(CHZColor::FromString(L"#55FFFF"));
	//lstColors.AddHead(CHZColor::FromString(L"#FFFFFF"));
	

	/*
	lstColors.AddHead(CHZColor::FromString(L"#1a1828"));
	lstColors.AddHead(CHZColor::FromString(L"#4d5a6c"));
	lstColors.AddHead(CHZColor::FromString(L"#6da9e3"));
	lstColors.AddHead(CHZColor::FromString(L"#ffffff"));
	lstColors.AddHead(CHZColor::FromString(L"#eeb333"));
	lstColors.AddHead(CHZColor::FromString(L"#259322"));
	lstColors.AddHead(CHZColor::FromString(L"#b04848"));
	lstColors.AddHead(CHZColor::FromString(L"#5b2e33"));
	*/

	// CGA
	/*
	lstColors.AddHead(CHZColor::FromString(L"#000000"));
	lstColors.AddHead(CHZColor::FromString(L"#ff55ff"));
	lstColors.AddHead(CHZColor::FromString(L"#55ffff"));
	lstColors.AddHead(CHZColor::FromString(L"#ffffff"));
	*/
	/* CGA Full
	lstColors.AddHead(CHZColor::FromString(L"#000000"));
	lstColors.AddHead(CHZColor::FromString(L"#555555"));
	lstColors.AddHead(CHZColor::FromString(L"#0000AA"));
	lstColors.AddHead(CHZColor::FromString(L"#5555FF"));
	lstColors.AddHead(CHZColor::FromString(L"#00AA00"));
	lstColors.AddHead(CHZColor::FromString(L"#55FF55"));
	lstColors.AddHead(CHZColor::FromString(L"#00AAAA"));
	lstColors.AddHead(CHZColor::FromString(L"#55FFFF"));
	lstColors.AddHead(CHZColor::FromString(L"#AA0000"));
	lstColors.AddHead(CHZColor::FromString(L"#FF5555"));
	lstColors.AddHead(CHZColor::FromString(L"#AA00AA"));
	lstColors.AddHead(CHZColor::FromString(L"#FF55FF"));
	lstColors.AddHead(CHZColor::FromString(L"#AA5500"));
	lstColors.AddHead(CHZColor::FromString(L"#FFFF55"));
	lstColors.AddHead(CHZColor::FromString(L"#AAAAAA"));
	lstColors.AddHead(CHZColor::FromString(L"#FFFFFF"));
	*/
	
	//arrColors.SetSize(15);
	// MSX
	/*
	lstColors.AddHead(CHZColor::FromString(L"#010101"));
	lstColors.AddHead(CHZColor::FromString(L"#3eb849"));
	lstColors.AddHead(CHZColor::FromString(L"#74d07d"));
	lstColors.AddHead(CHZColor::FromString(L"#5955e0"));
	lstColors.AddHead(CHZColor::FromString(L"#8076f1"));
	lstColors.AddHead(CHZColor::FromString(L"#b95e51"));
	lstColors.AddHead(CHZColor::FromString(L"#65dbef"));
	lstColors.AddHead(CHZColor::FromString(L"#db6559"));
	lstColors.AddHead(CHZColor::FromString(L"#ff897d"));
	lstColors.AddHead(CHZColor::FromString(L"#ccc35e"));
	lstColors.AddHead(CHZColor::FromString(L"#ded087"));
	lstColors.AddHead(CHZColor::FromString(L"#3aa241"));
	lstColors.AddHead(CHZColor::FromString(L"#b766b5"));
	lstColors.AddHead(CHZColor::FromString(L"#cccccc"));
	lstColors.AddHead(CHZColor::FromString(L"#ffffff"));
	*/

	// VIC-20
	/*
	lstColors.AddHead(CHZColor::FromString(L"#000000"));
	lstColors.AddHead(CHZColor::FromString(L"#ffffff"));
	lstColors.AddHead(CHZColor::FromString(L"#a8734a"));
	lstColors.AddHead(CHZColor::FromString(L"#e9b287"));
	lstColors.AddHead(CHZColor::FromString(L"#772d26"));
	lstColors.AddHead(CHZColor::FromString(L"#b66862"));
	lstColors.AddHead(CHZColor::FromString(L"#85d4dc"));
	lstColors.AddHead(CHZColor::FromString(L"#c5ffff"));
	lstColors.AddHead(CHZColor::FromString(L"#a85fb4"));
	lstColors.AddHead(CHZColor::FromString(L"#e99df5"));
	lstColors.AddHead(CHZColor::FromString(L"#559e4a"));
	lstColors.AddHead(CHZColor::FromString(L"#92df87"));
	lstColors.AddHead(CHZColor::FromString(L"#42348b"));
	lstColors.AddHead(CHZColor::FromString(L"#7e70ca"));
	lstColors.AddHead(CHZColor::FromString(L"#bdcc71"));
	lstColors.AddHead(CHZColor::FromString(L"#ffffb0"));
	*/

	// DawnBringer
	/*
	lstColors.AddHead(CHZColor::FromString(L"#4e4a4e"));
	lstColors.AddHead(CHZColor::FromString(L"#442434"));
	lstColors.AddHead(CHZColor::FromString(L"#30346d"));
	lstColors.AddHead(CHZColor::FromString(L"#854c30"));
	lstColors.AddHead(CHZColor::FromString(L"#346524"));
	lstColors.AddHead(CHZColor::FromString(L"#757161"));
	lstColors.AddHead(CHZColor::FromString(L"#d04648"));
	lstColors.AddHead(CHZColor::FromString(L"#597dce"));
	lstColors.AddHead(CHZColor::FromString(L"#d27d2c"));
	lstColors.AddHead(CHZColor::FromString(L"#6daa2c"));
	lstColors.AddHead(CHZColor::FromString(L"#8595a1"));
	lstColors.AddHead(CHZColor::FromString(L"#d2aa99"));
	lstColors.AddHead(CHZColor::FromString(L"#6dc2ca"));
	lstColors.AddHead(CHZColor::FromString(L"#dad45e"));
	lstColors.AddHead(CHZColor::FromString(L"#140c1c"));
	lstColors.AddHead(CHZColor::FromString(L"#deeed6"));
	*/
	/* Natural
	lstColors.AddHead(CHZColor::FromString(L"#F6DDDB"));
	lstColors.AddHead(CHZColor::FromString(L"#C4D5D5"));
	lstColors.AddHead(CHZColor::FromString(L"#D1D3B4"));
	lstColors.AddHead(CHZColor::FromString(L"#D3CF6C"));
	lstColors.AddHead(CHZColor::FromString(L"#CCA3A1"));
	lstColors.AddHead(CHZColor::FromString(L"#CD9235"));
	lstColors.AddHead(CHZColor::FromString(L"#D17793"));
	lstColors.AddHead(CHZColor::FromString(L"#5CAAA8"));
	lstColors.AddHead(CHZColor::FromString(L"#7D7F2B"));
	lstColors.AddHead(CHZColor::FromString(L"#D0422C"));
	lstColors.AddHead(CHZColor::FromString(L"#456289"));
	lstColors.AddHead(CHZColor::FromString(L"#30712B"));
	lstColors.AddHead(CHZColor::FromString(L"#47254A"));
	lstColors.AddHead(CHZColor::FromString(L"#4E2018"));
	lstColors.AddHead(CHZColor::FromString(L"#051D07"));
	*/
	
	/* Mono
	lstColors.AddHead(CHZColor::FromString(L"#1B1BD6"));
	lstColors.AddHead(CHZColor::FromString(L"#FFFFFF"));
	*/

	/* TI-89 */
	/*
	lstColors.AddHead(CHZColor::FromString(L"#1F272E"));
	lstColors.AddHead(CHZColor::FromString(L"#4E5F5B"));
	lstColors.AddHead(CHZColor::FromString(L"#808E89"));
	lstColors.AddHead(CHZColor::FromString(L"#AFBEB1"));
	*/

	/*
	arrColors.SetSize(2);
	arrColors.SetAt(0, CHZColor::FromString(L"#000000"));
	arrColors.SetAt(1, CHZColor::FromString(L"#FFFFFF"));
	*/

	/*
	arrColors.Add(CHZColor::FromString(L"#000000"));
	arrColors.Add(CHZColor::FromString(L"#0000FF"));
	arrColors.Add(CHZColor::FromString(L"#FF0000"));
	arrColors.Add(CHZColor::FromString(L"#FF00FF"));
	arrColors.Add(CHZColor::FromString(L"#00FF00"));
	arrColors.Add(CHZColor::FromString(L"#00FFFF"));
	arrColors.Add(CHZColor::FromString(L"#FF0000"));
	arrColors.Add(CHZColor::FromString(L"#FFFFFF"));
	*/

	/*
	arrColors.Add(CHZColor::FromString(L"#9BBC0F"));
	arrColors.Add(CHZColor::FromString(L"#8BAC0F"));
	arrColors.Add(CHZColor::FromString(L"#306230"));
	arrColors.Add(CHZColor::FromString(L"#0F380F"));
	*/

	/*
	arrColors.Add(CHZColor::FromString(L"#1B1BD6"));
	arrColors.Add(CHZColor::FromString(L"#373BDB"));
	arrColors.Add(CHZColor::FromString(L"#989AE0"));
	//arrColors.Add(CHZColor::FromString(L"#1B1BD6"));
	arrColors.Add(CHZColor::FromString(L"#FFFFFF"));
	*/

	/*
	srand(time(0));
	int nColors = (rand() % (32 - 2 + 1)) + 2;

	int r, g, b;
	
	for (int c = 0; c < nColors; c++)
	{
		r = (rand() % (255 - 0 + 1)) + 0;
		g = (rand() % (255 - 0 + 1)) + 0;
		b = (rand() % (255 - 0 + 1)) + 0;

		CHZColor col = RGB(r, g, b);
		lstColors.AddHead(col);
	}
	*/

	for (int nX = 0; nX < (sizeSrc.cx); nX++)
	{
		for (int nY = 0; nY < (sizeSrc.cy); nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			CHZColor crNew;
			
			if (lstColors.GetSize() != 2)
			{
				int nIndex = 0;
				float fMaxDelta = 20000000.0f;
				POSITION pos = lstColors.GetHeadPosition();
				CHZColor crIterator;
				while (pos != NULL)
				{
					crIterator = lstColors.GetNext(pos);

					float fDelta = 0.0;

					int rmean = (pRGBSrc->btRed + crIterator.GetRed()) / 2;
					int r = pRGBSrc->btRed - crIterator.GetRed();
					int g = pRGBSrc->btGreen - crIterator.GetGreen();
					int b = pRGBSrc->btBlue - crIterator.GetBlue();
					fDelta = sqrt((float)(((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
					if (fDelta < fMaxDelta) { crNew = crIterator; fMaxDelta = fDelta; }

				}
			}
			else
			{
				CHZColor crPixel = CHZColor(RGB(pRGBSrc->btRed, pRGBSrc->btGreen, pRGBSrc->btBlue));
				if (crPixel.GetLuminance() > 0.25f) crNew = lstColors.GetHead(); else crNew = lstColors.GetTail();
			}
			

			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			pRGBDest->btRed = GetRValue(crNew);
			pRGBDest->btGreen = GetGValue(crNew);
			pRGBDest->btBlue = GetBValue(crNew);
			pRGBDest->btAlpha = 255;// pRGBSrc->btAlpha;


			/*
			float ErrorR = float(pRGBSrc->btRed - crNew.GetRed()) / 16.0f;
			float ErrorG = float(pRGBSrc->btGreen - crNew.GetGreen()) / 16.0f;
			float ErrorB = float(pRGBSrc->btBlue - crNew.GetBlue()) / 16.0f;

			RGBX* pRGBSrc1 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 0)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 1))];
			RGBX* pRGBSrc2 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + max(0,              (nX - 1))];
			RGBX* pRGBSrc3 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + max(0,              (nX + 0))];
			RGBX* pRGBSrc4 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 1))];

			pRGBSrc1->btRed = max(0, min(255,	pRGBSrc1->btRed + ErrorR * 7.0f));
			pRGBSrc1->btGreen = max(0, min(255,	pRGBSrc1->btGreen + ErrorG * 7.0f));
			pRGBSrc1->btBlue = max(0, min(255,	pRGBSrc1->btBlue + ErrorB * 7.0f));

			pRGBSrc2->btRed = max(0, min(255,	pRGBSrc2->btRed + ErrorR * 3.0f));
			pRGBSrc2->btGreen = max(0, min(255,	pRGBSrc2->btGreen + ErrorG * 3.0f));
			pRGBSrc2->btBlue = max(0, min(255,	pRGBSrc2->btBlue + ErrorB * 3.0f));

			pRGBSrc3->btRed = max(0, min(255,	pRGBSrc3->btRed + ErrorR * 5.0f));
			pRGBSrc3->btGreen = max(0, min(255,	pRGBSrc3->btGreen + ErrorG * 5.0f));
			pRGBSrc3->btBlue = max(0, min(255,	pRGBSrc3->btBlue + ErrorB * 5.0f));

			pRGBSrc4->btRed = max(0, min(255,	pRGBSrc4->btRed + ErrorR * 1.0f));
			pRGBSrc4->btGreen = max(0, min(255,	pRGBSrc4->btGreen + ErrorG * 1.0f));
			pRGBSrc4->btBlue = max(0, min(255,	pRGBSrc4->btBlue + ErrorB * 1.0f));
			*/
			

			float ErrorR = float(pRGBSrc->btRed - crNew.GetRed()) / 42.0f;
			float ErrorG = float(pRGBSrc->btGreen - crNew.GetGreen()) / 42.0f;
			float ErrorB = float(pRGBSrc->btBlue - crNew.GetBlue()) / 42.0f;

			RGBX* pRGBSrc1 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 0)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 1))];
			RGBX* pRGBSrc2 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 0)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 2))];
			RGBX* pRGBSrc3 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + max(0,              (nX - 2))];
			RGBX* pRGBSrc4 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + max(0,              (nX - 1))];
			RGBX* pRGBSrc5 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 0))];
			RGBX* pRGBSrc6 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 1))];
			RGBX* pRGBSrc7 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 1)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 2))];
			RGBX* pRGBSrc8 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 2)) * sizeSrc.cx + max(0,              (nX - 2))];
			RGBX* pRGBSrc9 = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 2)) * sizeSrc.cx + max(0,              (nX - 1))];
			RGBX* pRGBSrcA = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 2)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 0))];
			RGBX* pRGBSrcB = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 2)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 1))];
			RGBX* pRGBSrcC = &pSrcPixels[min(sizeSrc.cy - 1, (nY + 2)) * sizeSrc.cx + min(sizeSrc.cx - 1, (nX + 2))];

			pRGBSrc1->btRed = max(0, min(255,	pRGBSrc1->btRed + ErrorR * 7.0f));
			pRGBSrc1->btGreen = max(0, min(255,	pRGBSrc1->btGreen + ErrorG * 7.0f));
			pRGBSrc1->btBlue = max(0, min(255,	pRGBSrc1->btBlue + ErrorB * 7.0f));

			pRGBSrc2->btRed = max(0, min(255,	pRGBSrc2->btRed + ErrorR * 5.0f));
			pRGBSrc2->btGreen = max(0, min(255,	pRGBSrc2->btGreen + ErrorG * 5.0f));
			pRGBSrc2->btBlue = max(0, min(255,	pRGBSrc2->btBlue + ErrorB * 5.0f));

			pRGBSrc3->btRed = max(0, min(255,	pRGBSrc3->btRed + ErrorR * 2.0f));
			pRGBSrc3->btGreen = max(0, min(255,	pRGBSrc3->btGreen + ErrorG * 2.0f));
			pRGBSrc3->btBlue = max(0, min(255,	pRGBSrc3->btBlue + ErrorB * 2.0f));

			pRGBSrc4->btRed = max(0, min(255,	pRGBSrc4->btRed + ErrorR * 4.0f));
			pRGBSrc4->btGreen = max(0, min(255,	pRGBSrc4->btGreen + ErrorG * 4.0f));
			pRGBSrc4->btBlue = max(0, min(255,	pRGBSrc4->btBlue + ErrorB * 4.0f));

			pRGBSrc5->btRed = max(0, min(255,	pRGBSrc5->btRed + ErrorR * 8.0f));
			pRGBSrc5->btGreen = max(0, min(255,	pRGBSrc5->btGreen + ErrorG * 8.0f));
			pRGBSrc5->btBlue = max(0, min(255,	pRGBSrc5->btBlue + ErrorB * 8.0f));

			pRGBSrc6->btRed = max(0, min(255,	pRGBSrc6->btRed + ErrorR * 4.0f));
			pRGBSrc6->btGreen = max(0, min(255,	pRGBSrc6->btGreen + ErrorG * 4.0f));
			pRGBSrc6->btBlue = max(0, min(255,	pRGBSrc6->btBlue + ErrorB * 4.0f));

			pRGBSrc7->btRed = max(0, min(255,	pRGBSrc7->btRed + ErrorR * 2.0f));
			pRGBSrc7->btGreen = max(0, min(255,	pRGBSrc7->btGreen + ErrorG * 2.0f));
			pRGBSrc7->btBlue = max(0, min(255,	pRGBSrc7->btBlue + ErrorB * 2.0f));

			pRGBSrc8->btRed = max(0, min(255,	pRGBSrc8->btRed + ErrorR * 1.0f));
			pRGBSrc8->btGreen = max(0, min(255,	pRGBSrc8->btGreen + ErrorG * 1.0f));
			pRGBSrc8->btBlue = max(0, min(255,	pRGBSrc8->btBlue + ErrorB * 1.0f));

			pRGBSrc9->btRed = max(0, min(255,	pRGBSrc9->btRed + ErrorR * 2.0f));
			pRGBSrc9->btGreen = max(0, min(255,	pRGBSrc9->btGreen + ErrorG * 2.0f));
			pRGBSrc9->btBlue = max(0, min(255,	pRGBSrc9->btBlue + ErrorB * 2.0f));

			pRGBSrcA->btRed = max(0, min(255,	pRGBSrcA->btRed + ErrorR * 4.0f));
			pRGBSrcA->btGreen = max(0, min(255,	pRGBSrcA->btGreen + ErrorG * 4.0f));
			pRGBSrcA->btBlue = max(0, min(255,	pRGBSrcA->btBlue + ErrorB * 4.0f));

			pRGBSrcB->btRed = max(0, min(255,	pRGBSrcB->btRed + ErrorR * 2.0f));
			pRGBSrcB->btGreen = max(0, min(255,	pRGBSrcB->btGreen + ErrorG * 2.0f));
			pRGBSrcB->btBlue = max(0, min(255,	pRGBSrcB->btBlue + ErrorB * 2.0f));

			pRGBSrcC->btRed = max(0, min(255,	pRGBSrcC->btRed + ErrorR * 1.0f));
			pRGBSrcC->btGreen = max(0, min(255,	pRGBSrcC->btGreen + ErrorG * 1.0f));
			pRGBSrcC->btBlue = max(0, min(255,	pRGBSrcC->btBlue + ErrorB * 1.0f));
			
		}
	}

	return TRUE;
}

///////

CHZBitmapFader::CHZBitmapFader(int nAmount)
{
	m_strFunctionName = "Fader";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapFader::~CHZBitmapFader()
{
}

BOOL CHZBitmapFader::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	float multiplier = (float)(10 - m_nAmount - 1) / 10;
	if (multiplier<0) return TRUE;

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			
			if (pRGBSrc->btAlpha>0)
			{
				RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

				pRGBDest->btRed = pRGBSrc->btRed * multiplier;
				pRGBDest->btGreen = pRGBSrc->btGreen * multiplier;
				pRGBDest->btBlue = pRGBSrc->btBlue * multiplier;
				pRGBDest->btAlpha = pRGBSrc->btAlpha * multiplier;
			}
		}
	}

	return TRUE;
}

///////

CHZBitmapConvoluter::CHZBitmapConvoluter(int nAmount)
{
	m_strFunctionName = "Convoluter";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);

}

CHZBitmapConvoluter::~CHZBitmapConvoluter()
{
}


BOOL CHZBitmapConvoluter::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{

	SetMask();

	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);

		for (int nX = 0; nX < sizeSrc.cx; nX++)
		{
			for (int nY = 0; nY < sizeSrc.cy; nY++)
			{
				int nRed = 0, nGreen = 0, nBlue = 0, nAlpha = 0, nSubCount = 0, nDivisor = 0;

				for (int nSubX = nX - 1; nSubX <= nX + 1; nSubX++)
				{
					for (int nSubY = nY - 1; nSubY <= nY + 1; nSubY++)
					{
						if (nSubX >= 0 && nSubX < sizeSrc.cx && nSubY >= 0 && nSubY < sizeSrc.cy)
						{
							RGBX* pRGBSub = &pSrcPixels[nSubY * sizeSrc.cx + nSubX];

							if (pRGBSub->btAlpha>0)
							{
								nRed += pRGBSub->btRed * m_cMask[nSubCount];
								nGreen += pRGBSub->btGreen * m_cMask[nSubCount];	
								nBlue += pRGBSub->btBlue * m_cMask[nSubCount];
								nAlpha += pRGBSub->btAlpha * m_cMask[nSubCount];
							}

							nDivisor += m_cMask[nSubCount];
						}

						nSubCount++;
					}
				}

				RGBX* pRGBDest = &pDestPixels[nY * sizeDest.cx + nX];
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeDest.cx + nX];

				if (nDivisor==0) nDivisor = 255;
				
				pRGBDest->btAlpha = max(0,min(255, nAlpha / nDivisor));// * alpha;
				pRGBDest->btRed = min(pRGBDest->btAlpha, nRed / nDivisor);// * alpha;
				pRGBDest->btGreen = min(pRGBDest->btAlpha, nGreen / nDivisor);// * alpha;
				pRGBDest->btBlue = min(pRGBDest->btAlpha, nBlue / nDivisor);// * alpha;
				

			}
		}
	}


	return TRUE;
}

///////

CHZBitmapBlurrer::CHZBitmapBlurrer(int nAmount) : CHZBitmapConvoluter(nAmount)
{
	m_strFunctionName = "Blurrer";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapBlurrer::~CHZBitmapBlurrer()
{
}

void CHZBitmapBlurrer::SetMask()
{
	char cEmbossMask[9] =		{-2, -1, 0,		-1, 1, 1,		0, 1, 2 };
	char cEmbossMoreMask[9] =	{-2, -2, 0,		-2, 0, 2, 		0, 2, 2 };
	char cSharpen3x3Mask[9] =	{0, -1, 0, 		-1, 5, -1, 		0, -1, 0 };
	char cSharpenFactorMask[9] ={0, -2, 0, 		-2, 11, -2,		0, -2, 0 };
	char cSharpenMask[9] =		{-1, -1, -1, 	-1, 9, -1, 		-1, -1, -1 };
	char cSharpenIntenseMask[9] ={1, 1, 1, 		1, -7, 1, 		1, 1, 1 };
	char cBlurMask[9] =			{1, 1, 1, 		1, 1, 1, 		1, 1, 1 };
	char cGuassianBlurMask[9] = {1, 2, 1, 		2, 3, 2, 		1, 2, 1 };
	char cEdgeEnhanceMask[9] =	{0, 0, 0,		-2, 1, 0, 		0, 0, 0 };
	char cEdgeDetectMask[9] =	{0, 1, 0, 		1, -5, 1, 		0, 1, 0 };
	char cChiselMask[9] =		{-1, -1, -1, 	-1, 0, 0, 		-1, 0, 1 };
	char cEdgeDetect2Mask[9] =	{-1, -1, -1, 	-1, 8, -1, 		-1, -1, -1 };
	char cAntiAliasMask[9] =	{1, 1, 1, 		1, 0, 1, 		1, 1, 1 };

	memcpy(m_cMask, cEdgeEnhanceMask, 9); //USE THIS ONE
	//memcpy(m_cMask, cAntiAliasMask, 9);
}

BOOL CHZBitmapBlurrer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	CHZBitmapConvoluter::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	
	ASSERT (sizeSrc == sizeDest);

	return TRUE;
}

///////

CHZBitmapSharpener::CHZBitmapSharpener(int nAmount)
{
	m_strFunctionName = "Sharpener";
	m_nAmount = max(0, nAmount);
	m_nAmount = min(m_nAmount, 10);
}

CHZBitmapSharpener::~CHZBitmapSharpener()
{
}

BOOL CHZBitmapSharpener::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_nAmount == 0)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		ASSERT (sizeSrc == sizeDest);

		double dMinMaxRatio = (double)1 / (1 + (10 - m_nAmount) * 5);

		double dMaxFactor = 1 / (4 * (1 + dMinMaxRatio));
		double dMinFactor = dMaxFactor * dMinMaxRatio;

		double dMask[9] = { -dMinFactor, -dMaxFactor, -dMinFactor, 
							-dMaxFactor,  2, -dMaxFactor,
							-dMinFactor, -dMaxFactor, -dMinFactor };

		for (int nX = 0; nX < sizeSrc.cx; nX++)
		{
			for (int nY = 0; nY < sizeSrc.cy; nY++)
			{
				if (nX > 0 && nX < sizeSrc.cx - 1 && nY > 0 && nY < sizeSrc.cy - 1)
				{
					double dRed = 0, dGreen = 0, dBlue = 0, dAlpha = 0, dDivisor = 0;
					int nSubCount = 0;
					
					for (int nSubX = nX - 1; nSubX <= nX + 1; nSubX++)
					{
						for (int nSubY = nY - 1; nSubY <= nY + 1; nSubY++)
						{
							RGBX* pRGBSub = &pSrcPixels[nSubY * sizeSrc.cx + nSubX];
							
							dRed += pRGBSub->btRed * dMask[nSubCount];
							dGreen += pRGBSub->btGreen * dMask[nSubCount];
							dBlue += pRGBSub->btBlue * dMask[nSubCount];
							dAlpha += pRGBSub->btAlpha * dMask[nSubCount];
							
							nSubCount++;
						}
					}

					RGBX* pRGBDest = &pDestPixels[nY * sizeDest.cx + nX];
					
					dRed = max(0,min(255, dRed));
					dGreen = max(0,min(255, dGreen));
					dBlue = max(0,min(255, dBlue));
					dAlpha = max(0,min(255, dAlpha));

					pRGBDest->btRed = (int)dRed;
					pRGBDest->btGreen = (int)dGreen;
					pRGBDest->btBlue = (int)dBlue;
					//pRGBDest->btAlpha = (int)dAlpha;
					pRGBDest->btAlpha = pSrcPixels[nY * sizeSrc.cx + nX].btAlpha;
				}
				else
				{
					pDestPixels[nY * sizeDest.cx + nX] = pSrcPixels[nY * sizeSrc.cx + nX];
				}
			}
		}
	}
	
	return TRUE;
}

////////

CHZBitmapStretcher::CHZBitmapStretcher(int nWidth, int nHeight)
{
	m_strFunctionName = "Stretcher";
	m_nWidth = nWidth;
	m_nHeight = nHeight;
}

CHZBitmapStretcher::~CHZBitmapStretcher()
{
}

CSize CHZBitmapStretcher::CalcDestSize(CSize sizeSrc)
{
	return CSize(m_nWidth, m_nHeight);
}

inline RGBX average( RGBX a, RGBX b )
{
	RGBX ret;
	ret.btRed = (a.btRed+(unsigned int)b.btRed)>>1;
	ret.btGreen = (a.btGreen+(unsigned int)b.btGreen)>>1;
	ret.btBlue = (a.btBlue+(unsigned int)b.btBlue)>>1;
	ret.btAlpha = (a.btAlpha+(unsigned int)b.btAlpha)>>1;
	return ret;
}


void CHZBitmapStretcher::ScaleLineAvg(RGBX *dest, RGBX *src, int srcWidth, int destWidth)
{
  int NumPixels = destWidth;
  int IntPart = srcWidth / destWidth;
  int FractPart = srcWidth % destWidth;
  int Mid = destWidth / 2;
  int E = 0;
  int skip;
  RGBX p;

  skip = (destWidth < srcWidth) ? 0 : destWidth / (2*srcWidth) + 1;
  NumPixels -= skip;

  while (NumPixels-- > 0) {
    p = *src;
    if (E >= Mid)
      p = average(p, *(src+1));
    *dest++ = p;
    src += IntPart;
    E += FractPart;
    if (E >= destWidth) {
      E -= destWidth;
      src++;
    } /* if */
  } /* while */
  while (skip-- > 0)
    *dest++ = *src;
}


BOOL CHZBitmapStretcher::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
		DWORD dwTick = GetTickCount();

	RGBX* Target = pDestPixels;
	RGBX* Source = pSrcPixels;
	int SrcWidth = sizeSrc.cx;
	int SrcHeight = sizeSrc.cy;
	int TgtWidth = sizeDest.cx;
	int TgtHeight = sizeDest.cy;

	/*  Smooth 2D scaling */


  int NumPixels = TgtHeight;
  int IntPart = (SrcHeight / TgtHeight) * SrcWidth;
  int FractPart = SrcHeight % TgtHeight;
  int Mid = TgtHeight / 2;
  int E = 0;
  int skip;
  RGBX *ScanLine, *ScanLineAhead;
  RGBX *PrevSource = NULL;
  RGBX *PrevSourceAhead = NULL;

  skip = (TgtHeight < SrcHeight) ? 0 : TgtHeight / (2*SrcHeight) + 1;
  NumPixels -= skip;

  ScanLine = (RGBX *)malloc(TgtWidth*sizeof(RGBX));
  ScanLineAhead = (RGBX *)malloc(TgtWidth*sizeof(RGBX));

  while (NumPixels-- > 0) {
    if (Source != PrevSource) {
      if (Source == PrevSourceAhead) {
        /* the next scan line has already been scaled and stored in
         * ScanLineAhead; swap the buffers that ScanLine and ScanLineAhead
         * point to
         */
        RGBX *tmp = ScanLine;
        ScanLine = ScanLineAhead;
        ScanLineAhead = tmp;
      } else {
        ScaleLineAvg(ScanLine, Source, SrcWidth, TgtWidth);
      } /* if */
      PrevSource = Source;
    } /* if */
    if (E >= Mid && PrevSourceAhead != Source+SrcWidth) {
      int x;
      ScaleLineAvg(ScanLineAhead, Source+SrcWidth, SrcWidth, TgtWidth);
      for (x = 0; x < TgtWidth; x++)
        ScanLine[x] = average(ScanLine[x], ScanLineAhead[x]);
      PrevSourceAhead = Source + SrcWidth;
    } /* if */
    memcpy(Target, ScanLine, TgtWidth*sizeof(RGBX));
    Target += TgtWidth;
    Source += IntPart;
    E += FractPart;
    if (E >= TgtHeight) {
      E -= TgtHeight;
      Source += SrcWidth;
    } /* if */
  } /* while */

  if (skip > 0 && Source != PrevSource)
    ScaleLineAvg(ScanLine, Source, SrcWidth, TgtWidth);
  while (skip-- > 0) {
    memcpy(Target, ScanLine, TgtWidth*sizeof(RGBX));
    Target += TgtWidth;
  } /* while */

  free(ScanLine);
  free(ScanLineAhead);

	/*

	uint32_t* src = (uint32_t*)pSrcPixels;
	uint32_t* dst = (uint32_t*)pDestPixels;
	int w = sizeSrc.cx;
	int h = sizeSrc.cy;
	int ow = sizeDest.cx;
	int oh = sizeDest.cy;

	float wr = w/(float)ow;
    float hr = h/(float)oh;
    int x,y;
    float x2, y2;
    int16_t fx, fy, fx1, fy1;
    int px, py;
    __m128i ab, cd, abcd;
    __m128i a12345678;
    __m128i a5678;
    __m128i a1234;
    __m128i a15263748;
    __m128i fx2, fy2;
    uint32_t *p;
    static int16_t *fxfx1 = NULL;
    static int16_t *fyfy1 = NULL;
    int16_t *fxfx1p;
    int16_t *fyfy1p;
    unsigned int wStepFixed16b, hStepFixed16b, wCoef, hCoef, offsetX, offsetY;
 
    if (!fxfx1) {
        fxfx1 = (int16_t*) calloc(8*ow, sizeof(int16_t));
        fxfx1p = fxfx1;
        for (x=0; x<ow; x++) {
            x2 = x*wr;
            px = (int) x2;
            fx = (x2 - px)*256;
            fx1 = 256 - fx;
            fxfx1p[0] = fx1;
            fxfx1p[1] = fx;
            fxfx1p[2] = fx1;
            fxfx1p[3] = fx;
            fxfx1p[4] = fx1;
            fxfx1p[5] = fx;
            fxfx1p[6] = fx1;
            fxfx1p[7] = fx;
            fxfx1p += 8;
        }
 
        fyfy1 = (int16_t*) calloc(8*oh, sizeof(int16_t));
        fyfy1p = fyfy1;
        for (y=0; y<oh; y++) {
            y2 = y*hr;
            py = (int) y2;
            fy = (y2 - py) *256;
            fy1 = 256 - fy;
            fyfy1p[0] = fy1;
            fyfy1p[1] = fy;
            fyfy1p[2] = fy1;
            fyfy1p[3] = fy;
            fyfy1p[4] = fy1;
            fyfy1p[5] = fy;
            fyfy1p[6] = fy1;
            fyfy1p[7] = fy;
            fyfy1p += 8;
        }
    }
 
    wStepFixed16b = ((w - 1) << 16) / (ow - 1);
    hStepFixed16b = ((h - 1) << 16) / (oh - 1);
    hCoef = 0;
 
    fyfy1p = fyfy1;
    for (y=0; y<oh; y++) {
        offsetY = (hCoef >> 16);
        wCoef = 0;
 
        fxfx1p = fxfx1;
        fy2 = _mm_loadu_si128((const __m128i*) fyfy1p);
        fyfy1p+=8;
 
        for (x=0; x<ow; x++) {
            offsetX = (wCoef >> 16);
 
            p = src + offsetY * w + offsetX;
 
            _mm_prefetch((const char*)fxfx1p+8, _MM_HINT_NTA);
            fx2 = _mm_loadu_si128((const __m128i*) fxfx1p);
 
            ab = _mm_loadl_epi64((const __m128i*) p);
            a12345678 = _mm_unpacklo_epi8(ab, _mm_setzero_si128());
            a5678 = _mm_unpacklo_epi64(a12345678, _mm_setzero_si128());
            a1234 = _mm_unpackhi_epi64(a12345678, _mm_setzero_si128());
            a15263748 = _mm_unpacklo_epi16(a5678, a1234);
            ab = _mm_madd_epi16(a15263748, fx2);
            ab = _mm_srai_epi32(ab, 8);
            ab = _mm_packs_epi32(ab, _mm_setzero_si128());

            cd = _mm_loadl_epi64((const __m128i*) (p+w));
            a12345678 = _mm_unpacklo_epi8(cd, _mm_setzero_si128());
            a5678 = _mm_unpacklo_epi64(a12345678, _mm_setzero_si128());
            a1234 = _mm_unpackhi_epi64(a12345678, _mm_setzero_si128());
            a15263748 = _mm_unpacklo_epi16(a5678, a1234);
            cd = _mm_madd_epi16(a15263748, fx2);
            cd = _mm_srai_epi32(cd, 8);
            cd = _mm_packs_epi32(cd, _mm_setzero_si128());
 
            abcd = _mm_unpacklo_epi16(ab, cd);
            abcd = _mm_madd_epi16(abcd, fy2);
            abcd = _mm_srai_epi32(abcd, 8);
            abcd = _mm_packs_epi32(abcd, _mm_setzero_si128());
            abcd = _mm_packus_epi16(abcd, _mm_setzero_si128());
 
            *dst = _mm_cvtsi128_si32(abcd);
            fxfx1p += 8;
            dst++;
            wCoef += wStepFixed16b;
        }
        hCoef += hStepFixed16b;
    }

	if (fxfx1!=NULL) free(fxfx1);
	if (fyfy1!=NULL) free(fyfy1);

	*/

	return TRUE;
}

////////

CHZBitmapPadder::CHZBitmapPadder(int nWidth, int nHeight, int nAlignment)
{
	m_strFunctionName = "Padder";
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nAlignment = nAlignment;
}


CHZBitmapPadder::CHZBitmapPadder(int nWidth, int nHeight, float fXPos, float fYPos)
{
	m_strFunctionName = "Padder";
	
	m_nWidth = nWidth;
	m_nHeight = nHeight;

	m_fXPos = fXPos;
	m_fYPos = fYPos;

	m_nAlignment = -1;

}


CHZBitmapPadder::~CHZBitmapPadder()
{
}

CSize CHZBitmapPadder::CalcDestSize(CSize sizeSrc)
{
	return CSize(m_nWidth, m_nHeight);
}

BOOL CHZBitmapPadder::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	int x, y;
	
	if (m_nAlignment==-1)
	{
		x = -(float(sizeSrc.cx) * m_fXPos);
		y = -(float(sizeSrc.cy) * m_fYPos);
		//x = m_fXPos;
		//y = m_fYPos;
	}
	else
	{
		int left = 0; // LEFT ALIGNMENT
		int center = (m_nWidth - sizeSrc.cx) / 2; // CENTER ALIGNMENT
		int right = m_nWidth - sizeSrc.cx; // RIGHT ALIGNMENT

		//center -= center % 2;

		int nNormalHeight = (abs(m_nHeight-sizeSrc.cy)==1)?sizeSrc.cy:m_nHeight;
		int top = 0; // TOP ALIGNMENT
		int vcenter = (nNormalHeight - sizeSrc.cy) / 2; // VCENTER ALIGNMENT
		int bottom = nNormalHeight - sizeSrc.cy; // BOTTOM ALIGNMENT
		
		//vcenter -= vcenter % 2;
		//bottom += bottom % 2;

		switch (m_nAlignment)
		{
			case 1: { x = left;		y = bottom;		break; }
			case 2: { x = center;	y = bottom;		break; }
			case 3: { x = right;	y = bottom;		break; }
			case 4: { x = left;		y = vcenter;	break; }
			case 5: { x = center;	y = vcenter;	break; }
			case 6: { x = right;	y = vcenter;	break; }
			case 7: { x = left;		y = top;		break; }
			case 8: { x = center;	y = top;		break; }
			case 9: { x = right;	y = top;		break; }
			default: { x = center;	y = bottom;		break; }
		}
	}
	
	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			if (nX >= x && nY >= y && nX < (sizeSrc.cx+x) && nY < (sizeSrc.cy+y))
			{
				RGBX* pRGBSrc = &pSrcPixels[(nY - y) * sizeSrc.cx + (nX - x)];

				if (pRGBSrc->btAlpha>0)
				{
					RGBX* pRGBDest = &pDestPixels[nY * sizeDest.cx + nX];
				
					pRGBDest->btRed = pRGBSrc->btRed;	
					pRGBDest->btGreen = pRGBSrc->btGreen;
					pRGBDest->btBlue = pRGBSrc->btBlue;
					pRGBDest->btAlpha = pRGBSrc->btAlpha;
				}
			}		
		}
	}


	return TRUE;
}

////////

////////

CHZBitmapCropper::CHZBitmapCropper(int x1, int y1, int x2, int y2)
{
	m_strFunctionName = "Cropper";
	m_nX1 = x1;
	m_nY1 = y1;
	m_nX2 = x2;
	m_nY2 = y2;
	m_nWidth = x2 - x1;
	m_nHeight = y2 - y1;
}

CHZBitmapCropper::~CHZBitmapCropper()
{
}

CSize CHZBitmapCropper::CalcDestSize(CSize sizeSrc)
{
	return CSize(m_nWidth, m_nHeight);
}

BOOL CHZBitmapCropper::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			if (nX >= m_nX1 && nY >= m_nY1 && nX < m_nX2 && nY < m_nY2)
			{
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];

				RGBX* pRGBDest = &pDestPixels[(nY - m_nY1) * sizeDest.cx + (nX - m_nX1)];

				pRGBDest->btRed = pRGBSrc->btRed;
				pRGBDest->btGreen = pRGBSrc->btGreen;
				pRGBDest->btBlue = pRGBSrc->btBlue;
				pRGBDest->btAlpha = pRGBSrc->btAlpha;

			}
		}
	}


	return TRUE;
}

/////////

CHZBitmapResampler::CHZBitmapResampler(int nWidth, int nHeight, FILTER nFilter)
{
	m_strFunctionName = "Resampler";
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFilter = nFilter;

	//Expensive algorithms:
	//CUBICCONVOLUTION x2
	//LANCZOS3 x2
	//LANCZOS8 x4

	switch (m_nFilter)
	{
		case BELL: m_fRadius = 1.5f; break;
		case BOX: m_fRadius = 0.5f; break;
		case CATMULLROM: m_fRadius = 2.0f; break;
		case COSINE: m_fRadius = 1.0f; break;
		case CUBICCONVOLUTION: m_fRadius = 3.0f; break;
		case CUBICSPLINE: m_fRadius = 2.0f; break;
		case HERMITE: m_fRadius = 1.0f; break;
		case LANCZOS3: m_fRadius = 3.0f; break;
		case LANCZOS8: m_fRadius = 8.0f; break;
		case MITCHELL: m_fRadius = 2.0f; break;
		case QUADRATIC: m_fRadius = 1.5f; break;
		case QUADRATICBSPLINE: m_fRadius = 1.5f; break;
		case TRIANGLE: m_fRadius = 1.0f; break;
	}
	/*
	switch (m_nFilter)
	{
		case BELL:				m_strFunctionName += L" BELL"; break;
		case BOX:				m_strFunctionName += L" BOX"; break;
		case CATMULLROM:		m_strFunctionName += L" CATMULLROM"; break;
		case COSINE:			m_strFunctionName += L" COSINE"; break;
		case CUBICCONVOLUTION:	m_strFunctionName += L" CUBICCONVOLUTION"; break;
		case CUBICSPLINE:		m_strFunctionName += L" CUBICSPLINE"; break;
		case HERMITE:			m_strFunctionName += L" HERMITE"; break;
		case LANCZOS3:			m_strFunctionName += L" LANCZOS3"; break;
		case LANCZOS8:			m_strFunctionName += L" LANCZOS8"; break;
		case MITCHELL:			m_strFunctionName += L" MITCHELL"; break;
		case QUADRATIC:			m_strFunctionName += L" QUADRATIC"; break;
		case QUADRATICBSPLINE:	m_strFunctionName += L" QUADRATICBSPLINE"; break;
		case TRIANGLE:			m_strFunctionName += L" TRIANGLE"; break;
	}
	*/
}

CHZBitmapResampler::~CHZBitmapResampler()
{
}

CSize CHZBitmapResampler::CalcDestSize(CSize sizeSrc)
{
	ASSERT(m_nWidth>0 && m_nHeight>0);
	return CSize(m_nWidth, m_nHeight);
}

BOOL CHZBitmapResampler::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{

	if (sizeSrc.cx == m_nWidth && sizeSrc.cy == m_nHeight)
	{
		CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); 
		return true;
	}

	BOOL bRes = TRUE;

	BOOL fSuccess = FALSE;
	
	int i, j, n, c;
	float xScale, yScale;

	int* ib; 
	int* ob;

	// Temporary values
	int val; 
	int col;
	float* h_weight; // Weight contribution    [ow][MAX_CONTRIBS]
	int* h_pixel; // Pixel that contributes [ow][MAX_CONTRIBS]
	int* h_count; // How many contribution for the pixel [ow]
	float* h_wsum;   // Sum of weights [ow]							 
	float* v_weight; // Weight contribution    [oh][MAX_CONTRIBS]
	int* v_pixel;  // Pixel that contributes [oh][MAX_CONTRIBS]
	int* v_count;	 // How many contribution for the pixel [oh]
	float* v_wsum;   // Sum of weights [oh]
	
	int* tb;        // Temporary (intermediate buffer)

	float intensity[4];	// RGBA component intensities
	
	float center;				// Center of current sampling 
	float weight;				// Current wight
	int left;						// Left of current sampling
	int right;						// Right of current sampling

	float* p_weight;		// Temporary pointer
	int* p_pixel;     // Temporary pointer

	int MAX_CONTRIBS;    // Almost-const: max number of contribution for current sampling
	float SCALED_RADIUS;	// Almost-const: scaled radius for downsampling operations
	float FILTER_FACTOR; // Almost-const: filter factor for downsampling operations

	BYTE* ibuf = (BYTE*)pSrcPixels;
	int iw = sizeSrc.cx;
	int ih = sizeSrc.cy;
	BYTE* obuf = (BYTE*)pDestPixels;
	int ow = sizeDest.cx;
	int oh = sizeDest.cy;

	// Preliminary (redundant ? ) check 
	if ( iw < 1 || ih < 1 || ibuf == NULL || ow <1 || oh<1 || obuf == NULL)
	{
		return FALSE;
	}
	
	// Aliasing buffers
	ib = (int*)ibuf;
	ob = (int*)obuf;
	
	if ( ow == iw && oh == ih)
	{ // Same size, no resampling 
		CopyMemory(ob, ib, iw * ih * sizeof(COLORREF));
		return TRUE;
	}

	xScale = ((float)ow / iw);
	yScale = ((float)oh / ih);

	h_weight = NULL; 
	h_pixel  = NULL; 
	h_count  = NULL; 
	h_wsum   = NULL; 

	v_weight = NULL; 
	v_pixel  = NULL; 
	v_count  = NULL; 
	v_wsum   = NULL; 

	tb = NULL;
	tb = new int[ow*ih];
	
	if ( xScale > 1.0)
	{
		// Horizontal upsampling
		FILTER_FACTOR = 1.0f;
		SCALED_RADIUS = m_fRadius;
	}
	else
	{ 
		// Horizontal downsampling 
		FILTER_FACTOR = xScale;
		SCALED_RADIUS = m_fRadius / xScale;
	}

	// The maximum number of contributions for a target pixel
	MAX_CONTRIBS  = (int) (2 * SCALED_RADIUS  + 1);

	// Pre-allocating all of the needed memory
	h_weight = new float[ow * MAX_CONTRIBS];
	h_pixel = new int[ow * MAX_CONTRIBS];
	h_count = new int[ow];
	h_wsum = new float[ow];

	// Pre-calculate weights contribution for a row
	for (i = 0; i < ow; i++)
	{
		p_weight    = h_weight + i * MAX_CONTRIBS;
		p_pixel     = h_pixel  + i * MAX_CONTRIBS;

		h_count[i] = 0;
		h_wsum[i] =  0.0f;
		
		center = ((float)i)/xScale;
		left = (int)((center + .5) - SCALED_RADIUS);
		right = (int)(left + 2 * SCALED_RADIUS);

		for (j = left; j<= right; j++)
		{
			if ( j < 0 || j >= iw) continue;
		
			switch (m_nFilter)
			{
				case BELL: weight = Bell((center - j) * FILTER_FACTOR); break;
				case BOX: weight = Box((center - j) * FILTER_FACTOR); break;
				case CATMULLROM: weight = CatmullRom((center - j) * FILTER_FACTOR); break;
				case COSINE: weight = Cosine((center - j) * FILTER_FACTOR); break;
				case CUBICCONVOLUTION:weight = CubicConvolution((center - j) * FILTER_FACTOR); break;
				case CUBICSPLINE: weight = CubicSpline((center - j) * FILTER_FACTOR); break;
				case HERMITE: weight = Hermite((center - j) * FILTER_FACTOR); break;
				case LANCZOS3: weight = Lanczos3((center - j) * FILTER_FACTOR); break;
				case LANCZOS8: weight = Lanczos8((center - j) * FILTER_FACTOR); break;
				case MITCHELL: weight = Mitchell((center - j) * FILTER_FACTOR); break;
				case QUADRATIC: weight = Quadratic((center - j) * FILTER_FACTOR); break;
				case QUADRATICBSPLINE:weight = QuadraticBSpline((center - j) * FILTER_FACTOR); break;
				case TRIANGLE: weight = Triangle((center - j) * FILTER_FACTOR); break;
			}
			
			if (weight == 0.0f) continue;

			n = h_count[i]; // Since h_count[i] is our current index
			p_pixel[n] = j;
			p_weight[n] = weight;
			h_wsum[i] += weight;
			h_count[i]++; // Increment contribution count
		}
	}

	int alpha = 0;
	// Filter horizontally from input to temporary buffer
	for (n = 0; n < ih; n++)
	{
		// Here 'n' runs on the vertical coordinate
		for ( i = 0; i < ow; i++)
		{
			// i runs on the horizontal coordinate
			p_weight = h_weight + i * MAX_CONTRIBS;
			p_pixel  = h_pixel  + i * MAX_CONTRIBS;

			for (c=0; c<4; c++)
			{
				intensity[c] = 0.0f;
			}
			for (j=0; j < h_count[i]; j++)
			{
				weight = p_weight[j];	
				val = ib[p_pixel[j] + n * iw]; // Using val as temporary storage
				// Acting on color components
				for (c=0; c<4; c++)
				{
					intensity[c] += (val & 0xFF) * weight; // PROFILER CHANGED
					val = val >> 8;
				}				
			}
			// val is already 0
			for (c= 0 ; c < 4; c++)
			{
				val = val << 8;
				col = (int)(intensity[4 - c - 1] / h_wsum[i]);
				if (c==0 /*alpha*/) col = alpha = min(255,max(0,col));
				else col = min(alpha,max(0,col));
				val |= col; 
			}
			tb[i+n*ow] = val; // Temporary buffer ow x ih
		}
	}

	// Going to vertical stuff
	if ( yScale > 1.0)
	{
		FILTER_FACTOR = 1.0f;
		SCALED_RADIUS = m_fRadius;
	}
	else
	{
		FILTER_FACTOR = yScale;
		SCALED_RADIUS = m_fRadius / yScale;
	}
	MAX_CONTRIBS  = (int) (2 * SCALED_RADIUS  + 1);

	// Pre-calculate filter contributions for a column
	v_weight = new float[oh * MAX_CONTRIBS];
	v_pixel = new int[oh * MAX_CONTRIBS];
	v_count = new int[oh];
	v_wsum = new float[oh];

	for (i = 0; i < oh; i++)
	{
		p_weight = v_weight + i * MAX_CONTRIBS;
		p_pixel  = v_pixel  + i * MAX_CONTRIBS;
		
		v_count[i] = 0;
		v_wsum[i] = 0.0f;

		center = ((float) i) / yScale;
		left = (int) (center+.5 - SCALED_RADIUS);
		right = (int)( left + 2 * SCALED_RADIUS);

		for (j = left; j <= right; j++)
		{
			if (j < 0 || j >= ih) continue;

			switch (m_nFilter)
			{
				case BELL: weight = Bell((center - j) * FILTER_FACTOR); break;
				case BOX: weight = Box((center - j) * FILTER_FACTOR); break;
				case CATMULLROM: weight = CatmullRom((center - j) * FILTER_FACTOR); break;
				case COSINE: weight = Cosine((center - j) * FILTER_FACTOR); break;
				case CUBICCONVOLUTION:weight = CubicConvolution((center - j) * FILTER_FACTOR); break;
				case CUBICSPLINE: weight = CubicSpline((center - j) * FILTER_FACTOR); break;
				case HERMITE: weight = Hermite((center - j) * FILTER_FACTOR); break;
				case LANCZOS3: weight = Lanczos3((center - j) * FILTER_FACTOR); break;
				case LANCZOS8: weight = Lanczos8((center - j) * FILTER_FACTOR); break;
				case MITCHELL: weight = Mitchell((center - j) * FILTER_FACTOR); break;
				case QUADRATIC: weight = Quadratic((center - j) * FILTER_FACTOR); break;
				case QUADRATICBSPLINE:weight = QuadraticBSpline((center - j) * FILTER_FACTOR); break;
				case TRIANGLE: weight = Triangle((center - j) * FILTER_FACTOR); break;
			}
			
			if ( weight == 0.0f) continue;
			n = v_count[i]; // Our current index
			p_pixel[n] = j;
			p_weight[n] = weight;
			v_wsum[i]+= weight;
			v_count[i]++; // Increment the contribution count 
		}
	}

	// Filter vertically from work to output
	for (n = 0; n < ow; n++)
	{
		 for (i = 0; i < oh; i++)
		 {
			 p_weight = v_weight + i * MAX_CONTRIBS;
			 p_pixel     = v_pixel  + i * MAX_CONTRIBS;

			 for (c=0; c<4; c++)
			 {
				 intensity[c] = 0.0f;
			 }
				
			 for (j = 0; j < v_count[i]; j++)
			 {
				 weight = p_weight[j];
 				 val = tb[ n + ow * p_pixel[j]]; // Using val as temporary storage
				 // Acting on color components
				 for (c=0; c<4; c++)
				 {
					 intensity[c] += (val & 0xFF) * weight; // PROFILER CHANGED
					 val = val >> 8;
				 }				
			 }
			// val is already 0
			for (c=0; c<4; c++)
			{
				val = val << 8;
				col = (int)(intensity[4 - c - 1] / v_wsum[i]);
				if (c==0) col = alpha = min(255,max(0,col));
				else col = min(alpha,max(0,col));
				val |= col;
			}
			ob[n+i*ow] = val;
		}
	}

	fSuccess = TRUE;

	delete[] tb;
	delete[] h_weight;
	delete[] h_pixel;
	delete[] h_count;
	delete[] h_wsum;
	delete[] v_weight;
	delete[] v_pixel;
	delete[] v_count;
	delete[] v_wsum;

	return TRUE;
}


// Lanczos8 filter, default radius 8
float CHZBitmapResampler::Lanczos8(float x)
{
	const float R = 8.0;
	if (x	< 0.0 ) x = - x;

	if ( x == 0.0) return 1;

	if ( x < R)
	{
		x *= PI;
		return R * sin(x) * sin(x / R) / (x*x);
	}
	return 0.0;
}


// Lanczos3 filter, default radius 3
float CHZBitmapResampler::Lanczos3(float x)
{
	const float R = 3.0;
	if (x	< 0.0 ) x = - x;

	if ( x == 0.0) return 1;

	if ( x < R)
	{
		x *= PI;
		return R * sin(x) * sin(x / R) / (x*x);
	}
	return 0.0;
}

// Hermite filter, default radius 1
float CHZBitmapResampler::Hermite(float x) 
{
	if (x < 0.0) x = - x;
	if (x < 1.0) return ((2.0 * x - 3) * x * x + 1.0 );
	return 0.0;	
}

// Box filter, default radius 0.5
float CHZBitmapResampler::Box(float x)
{
	if (x < 0.0) x = - x;
	if (x <= 0.5) return 1.0;
	return 0.0;	
}


// Trangle filter, default radius 1
float CHZBitmapResampler::Triangle(float x)
{
	if (x < 0.0) x = - x;
	if (x < 1.0) return (1.0 - x);
	return 0.0;	 
}

// Bell filter, default radius 1.5
float CHZBitmapResampler::Bell(float x)
{
	if (x < 0.0) x = - x;
	if (x < 0.5) return (0.75 - x*x);
	if (x < 1.5) return (0.5 * pow(x - 1.5, 2.0));
	return 0.0;		
}

// CubicSpline filter, default radius 2
float CHZBitmapResampler::CubicSpline(float x)
{
	float x2;

	if (x < 0.0) x = - x;
	if (x < 1.0 ) 
	{
		x2 = x*x;
		return (0.5 * x2 * x - x2 + 2.0 / 3.0);
	}
	if (x < 2.0) 
	{
		x = 2.0 - x;
		return (pow((double)x, 3.0)/6.0);
	}
	return 0.0;	 
}


// Mitchell filter, default radius 2.0
float CHZBitmapResampler::Mitchell(float x)
{
	const float C = 1.0/3.0;
	float x2;

	if (x < 0.0) x = - x;
	x2 = x*x;
	if (x < 1.0) 
	{
		x = (((12.0 - 9.0 * C - 6.0 * C) * (x * x2)) + ((-18.0 + 12.0 * C + 6.0 * C) * x2) + (6.0 - 2.0 * C));
		return ( x / 6.0);
	}
	if (x < 2.0)
	{
		x = (((-C - 6.0 * C) * (x * x2)) + ((6.0 * C + 30.0 * C) * x2) + ((-12.0 * C - 48.0 * C) * x) + (8.0 * C + 24.0 * C));
		return (x / 6.0);
	}
	return 0.0;
}

// Cosine filter, default radius 1
float CHZBitmapResampler::Cosine(float x) 
{
	if ((x >= -1.0) && (x <= 1.0)) return ((cos(x * PI) + 1.0)/2.0);
	
	return 0.0;	
}

// CatmullRom filter, default radius 2
float CHZBitmapResampler::CatmullRom(float x) 
{
	const float C = 0.5;	
	float x2;
	if (x < 0.0) x = 0.0f - x;
	x2= x * x;

	if (x <= 1.0) return (1.5 * x2 * x - 2.5 * x2 + 1.0);
	if (x <= 2.0) return (- 0.5 * x2 * x + 2.5 * x2 - 4.0 * x + 2.0);
	return 0.0;	
}

// Quadratic filter, default radius 1.5
float CHZBitmapResampler::Quadratic(float x) 
{
	if (x < 0.0) x = - x;
	if (x <= 0.5) return (- 2.0 * x * x + 1);
	if (x <= 1.5) return (x * x - 2.5* x + 1.5);
	return 0.0;
}

// QuadraticBSpline filter, default radius 1.5
float CHZBitmapResampler::QuadraticBSpline(float x) 
{			
	if (x < 0.0) x = - x;
	if (x <= 0.5) return (- x * x + 0.75);
	if (x <= 1.5) return (0.5 * x * x - 1.5 * x + 1.125);
	return 0.0;
}

// CubicConvolution filter, default radius 3
float CHZBitmapResampler::CubicConvolution(float x) 
{
	float x2;
	if (x < 0.0) x = - x;
	x2 = x * x;
	if (x <= 1.0) return ((4.0 / 3.0)* x2 * x - (7.0 / 3.0) * x2 + 1.0);
	if (x <= 2.0) return (- (7.0 / 12.0) * x2 * x + 3 * x2 - (59.0 / 12.0) * x + 2.5);
	if (x <= 3.0) return ( (1.0/12.0) * x2 * x - (2.0 / 3.0) * x2 + 1.75 * x - 1.5);
	return 0.0;		
}

////////

CHZBitmapResizer::CHZBitmapResizer(double dFactor) : m_dFactor(dFactor)
{
	m_strFunctionName = "Resizer";
	ASSERT (m_dFactor > 0);

	if (m_dFactor > 1)
		m_bWeightingEnabled = TRUE;
}

CHZBitmapResizer::~CHZBitmapResizer()
{
}

CSize CHZBitmapResizer::CalcDestSize(CSize sizeSrc)
{
	return CSize((int)(sizeSrc.cx * m_dFactor), (int)(sizeSrc.cy * m_dFactor));
}

BOOL CHZBitmapResizer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	BOOL bRes = TRUE;

	if (m_dFactor <= 0) return FALSE;

	if (m_dFactor == 1) bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else if (m_dFactor > 1) bRes = Enlarge(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else bRes = Shrink(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	
	return TRUE;
}

BOOL CHZBitmapResizer::Enlarge(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (m_dFactor > 1);

	if (m_dFactor <= 1) return FALSE;

	double dFactor = 1 / m_dFactor;
	double dXSrc = 0;

	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		double dYSrc = 0;

		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			pDestPixels[nY * sizeDest.cx + nX] = CalcWeightedColor(pSrcPixels, sizeSrc, dXSrc, dYSrc);
			dYSrc += dFactor; // next dest pixel in source coords
		}

		dXSrc += dFactor; // next dest pixel in source coords
	}

	return TRUE;
}

BOOL CHZBitmapResizer::Shrink(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (m_dFactor < 1 && m_dFactor > 0);

	if (m_dFactor >= 1 || m_dFactor <= 0) return FALSE;

	double dFactor = 1 / m_dFactor;
	double dXEnd = -dFactor / 2;
	int nXStart, nXEnd = -1;

	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		int nYStart, nYEnd = -1;
		double dYEnd = -dFactor / 2;

		nXStart = nXEnd + 1;
		dXEnd += dFactor;
		nXEnd = min(sizeSrc.cx - 1, (int)dXEnd + 1);

		if (nXStart > nXEnd)
			continue;

		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			nYStart = nYEnd + 1;
			dYEnd += dFactor;
			nYEnd = min(sizeSrc.cy - 1, (int)dYEnd + 1);

			if (nYStart > nYEnd)
				continue;

			int nCount = 0, nRed = 0, nGreen = 0, nBlue = 0, nAlpha = 0;

			// average the pixels over the range
			for (int nXSub = nXStart; nXSub <= nXEnd; nXSub++)
			{
				for (int nYSub = nYStart; nYSub <= nYEnd; nYSub++)
				{
					RGBX* pRGBSrc = &pSrcPixels[nYSub * sizeSrc.cx + nXSub];

					nRed += pRGBSrc->btRed;
					nGreen += pRGBSrc->btGreen;
					nBlue += pRGBSrc->btBlue;
					nAlpha += pRGBSrc->btAlpha * pRGBSrc->btAlpha;
					nCount++;
				}
			}

			RGBX* pRGBDest = &pDestPixels[nY * sizeDest.cx + nX];

			pRGBDest->btRed = nRed / nCount;
			pRGBDest->btGreen = nGreen / nCount;
			pRGBDest->btBlue = nBlue / nCount;
			pRGBDest->btAlpha = sqrt((double)nAlpha / nCount);
		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapOutliner::CHZBitmapOutliner(COLORREF crColor, int nRadius)
{
	m_strFunctionName = "Outliner";
	m_crColor = crColor;
	m_nRadius = nRadius;
}

CHZBitmapOutliner::~CHZBitmapOutliner()
{
}


BOOL CHZBitmapOutliner::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);
	
	//CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default

	int alphaT, alphaL, alphaB, alphaR;
	int inc = 1;
	/*
	
	int sx = sizeSrc.cx,sy = sizeSrc.cy,ex = 0,ey = 0;

	for (int nX = inc; nX < sizeSrc.cx-inc; nX++)
	{
		for (int nY = inc; nY < sizeSrc.cy-inc; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			if (pRGBSrc->btAlpha>0)
			{
				sx = min(sx, nX); sy = min(sy, nY); ex = max(ex, nX); ey = max(ey, nY);
				alphaT = pSrcPixels[(nY-inc) * sizeSrc.cx + nX].btAlpha;
				alphaL = pSrcPixels[nY * sizeSrc.cx + (nX-inc)].btAlpha;
				alphaB = pSrcPixels[(nY+inc) * sizeSrc.cx + nX].btAlpha;
				alphaR = pSrcPixels[nY * sizeSrc.cx + (nX+inc)].btAlpha;
				//if (alphaT == 255 && alphaL == 255 && alphaB == 255 && alphaR == 255) 
				if (alphaT + alphaL + alphaB + alphaR == 1020) 
					pRGBDest->btAlpha = 0;
				else
				{
					pRGBDest->btAlpha = pRGBSrc->btAlpha;
				}
			}
		}
	}

	CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	*/

	//CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default

	float x1, y1, x2, y2;
	float t, a, b, cx, cy, x, y, f;
	int ix, iy, i1, i2, alphaOLD, alphaNEW;
	bool exch;

//	DWORD dwTicks = 0;

//	DWORD dwTicks1 = 0;
//	DWORD dwTicks2 = 0;
//	DWORD dwTicks3 = 0;

	for (int nX = m_nRadius+1; nX < sizeSrc.cx-m_nRadius; nX++)
	{
		for (int nY = m_nRadius+1; nY < sizeSrc.cy-m_nRadius; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			
			if (pRGBSrc->btAlpha>0)
			{
				RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
				//pRGBSrc->btAlpha = max(0,(pRGBSrc->btAlpha + 128) >> 2);

				alphaT = pSrcPixels[(nY-inc) * sizeSrc.cx + nX].btAlpha;
				alphaL = pSrcPixels[nY * sizeSrc.cx + (nX-inc)].btAlpha;
				alphaB = pSrcPixels[(nY+inc) * sizeSrc.cx + nX].btAlpha;
				alphaR = pSrcPixels[nY * sizeSrc.cx + (nX+inc)].btAlpha;
			
				//if (alphaT + alphaL + alphaB + alphaR < 800) // used to be 1000
				if (alphaT + alphaL + alphaB + alphaR + pRGBSrc->btAlpha < 1200)
				{

					if (false)//m_nRadius<3) // RECOMMEND 5
					{
				
						alphaNEW = pRGBSrc->btAlpha;//max(0,(pRGBSrc->btAlpha + 0) >> 2);
						int x = m_nRadius;
						int y = 0;
						int xChange = 1 - (m_nRadius << 1);
						int yChange = 0;
						int radiusError = 0;
				
						int x0 = nX;
						int y0 = nY;

						while (x >= y)
						{
							int y1 = y0 + x;
							int y2 = y0 + y;
							int y3 = y0 - x;
							int y4 = y0 - y;

							for (int i = min(sizeSrc.cx,x0 + x); i>=max(0,x0 - x); i--)
							{
								if (y2<sizeSrc.cy) { pRGBDest = &pDestPixels[(y2) * sizeSrc.cx + i]; pRGBDest->btAlpha = min(255, pRGBDest->btAlpha + INT_ALPHA[max(0,alphaNEW)][255-pRGBDest->btAlpha]); }
								if (y4>0) { pRGBDest = &pDestPixels[(y4) * sizeSrc.cx + i]; pRGBDest->btAlpha = min(255, pRGBDest->btAlpha + INT_ALPHA[max(0,alphaNEW)][255-pRGBDest->btAlpha]); }
							}
							for (int i = min(sizeSrc.cx,x0 + y); i>=max(0,x0 - y); i--)
							{
								if (y1<sizeSrc.cy) { pRGBDest = &pDestPixels[(y1) * sizeSrc.cx + i]; pRGBDest->btAlpha = min(255, pRGBDest->btAlpha + INT_ALPHA[max(0,alphaNEW)][255-pRGBDest->btAlpha]); }
								if (y3>0) { pRGBDest = &pDestPixels[(y3) * sizeSrc.cx + i]; pRGBDest->btAlpha = min(255, pRGBDest->btAlpha + INT_ALPHA[max(0,alphaNEW)][255-pRGBDest->btAlpha]); }
							}

							y++;
							radiusError += yChange;
							yChange += 2;
							if (((radiusError << 1) + xChange) > 0)
							{
								x--;
								radiusError += xChange;
								xChange += 2;
							}
						}
					}
					else
					{
						x1 = nX - m_nRadius;
						y1 = nY - m_nRadius;
						x2 = nX + m_nRadius;
						y2 = nY + m_nRadius;

						if( x2<x1 )
						{
							t = x1;
							x1 = x2;
							x2 = t;
						}
						if( y2<y1 )
						{
							t = y1;
							y1 = y2;
							y2 = t;
						}
						if( x2-x1<y2-y1 )
						{
							exch = false;
							ASSERT(false);
							
						}
						else
						{
							exch = true;
							t = x1;
							x1 = y1;
							y1 = t;
							t = x2;
							x2 = y2;
							y2 = t;
						}

						a = (x2-x1)/2;
						b = (y2-y1)/2;
						cx = (x1+x2)/2;
						cy = (y1+y2)/2;
						t = a*a/sqrtz(a*a+b*b);
						i1 = floorz(cx-t);
						i2 = ceilz(cx+t);

						for (ix = i1; ix <= i2; ix++)
						{
							if (1-sqr((ix-cx)/a)<0)
							{
								continue;
							}
							y = b*sqrtz(1-sqr((ix-cx)/a));
							iy = ceilz(cy+y);

							if ((exch?ix:iy) >= sizeDest.cy || (exch?iy:ix) >= sizeDest.cx)	continue;
							
							f = iy-cy-y;
							alphaNEW = int((1.0-f)*pRGBSrc->btAlpha);
							if (!exch)
							{
								alphaOLD = pDestPixels[iy * sizeDest.cx + ix].btAlpha;
								pDestPixels[iy * sizeDest.cx + ix].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
							else
							{
								alphaOLD = pDestPixels[ix * sizeDest.cx + iy].btAlpha;
								pDestPixels[ix * sizeDest.cx + iy].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}

							iy = floorz(cy-y);
							if ((exch?ix:iy) >= sizeDest.cy || (exch?iy:ix) >= sizeDest.cx)	continue;

							f = cy-y-iy;
							alphaNEW = int((1.0-f)*pRGBSrc->btAlpha);
							if (!exch)
							{
								alphaOLD = pDestPixels[iy * sizeDest.cx + ix].btAlpha;
								pDestPixels[iy * sizeDest.cx + ix].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
							else
							{
								alphaOLD = pDestPixels[ix * sizeDest.cx + iy].btAlpha;
								pDestPixels[ix * sizeDest.cx + iy].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
						}

						

						t = b*b/sqrtz(a*a+b*b);
						i1 = ceilz(cy-t);
						i2 = floorz(cy+t);

						for (iy = i1; iy <= i2; iy++)
						{
							

							if (1-sqr((iy-cy)/b)<0)
							{
								continue;
							}
							x = a*sqrtz(1-sqr((iy-cy)/b));
							ix = floorz(cx-x);

							if ((exch?ix:iy) >= sizeDest.cy || (exch?iy:ix) >= sizeDest.cx)	continue;

							f = cx-x-ix;
							alphaNEW = int((1.0-f)*pRGBSrc->btAlpha);

							if (!exch)
							{
								alphaOLD = pDestPixels[iy * sizeDest.cx + ix].btAlpha;
								pDestPixels[iy * sizeDest.cx + ix].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
							else
							{
								alphaOLD = pDestPixels[ix * sizeDest.cx + iy].btAlpha;
								pDestPixels[ix * sizeDest.cx + iy].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
							
							ix = ceilz(cx+x);

							if ((exch?ix:iy) >= sizeDest.cy || (exch?iy:ix) >= sizeDest.cx)	continue;

							f = ix-cx-x;
							alphaNEW = int((1.0-f)*pRGBSrc->btAlpha);
							if (!exch)
							{
								alphaOLD = pDestPixels[iy * sizeDest.cx + ix].btAlpha;
								pDestPixels[iy * sizeDest.cx + ix].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
							else
							{
								alphaOLD = pDestPixels[ix * sizeDest.cx + iy].btAlpha;
								pDestPixels[ix * sizeDest.cx + iy].btAlpha = avg(alphaOLD,alphaNEW);//min(255, alphaOLD + INT_ALPHA[alphaNEW][255-alphaOLD]);
							}
						}

						i1 = ceilz(cx-a);
						i2 = floorz(cx+a);
					
						for(ix = i1; ix <= i2; ix++)
						{
							if( 1-sqr((ix-cx)/a)<0 )
							{
								continue;
							}

							y = b*sqrtz(1-sqr((ix-cx)/a));
							if(!exch)
							{
								//memset(&pDestPixels[ceilz((cy-y)+1) * sizeDest.cx + ix].btAlpha, 255, max(0,ceilz(((y*2)-2)*4)));

								/*
								for(int i = ceilz(cy-y); i <= floorz(cy+y); i++)
								{
									alphaOLD = pDestPixels[i * sizeDest.cx + ix].btAlpha;
									pDestPixels[i * sizeDest.cx + ix].btAlpha = avg(alphaOLD,pRGBSrc->btAlpha);//min(255, alpha + INT_ALPHA[255][255-alpha]);

									//pDestPixels[i * sizeDest.cx + ix].btAlpha = 255;//min(255, alpha + INT_ALPHA[255][255-alpha]);

								}
								*/
							}
							else
							{
								//dwTicks = GetTickCount();

								if (true)
								{
									alphaNEW = pDestPixels[ix * sizeDest.cx + ceilz(cy-y)].btAlpha;
									alphaOLD = pDestPixels[ix * sizeDest.cx + floorz(cy+y)].btAlpha;
									memset(&pDestPixels[ix * sizeDest.cx + ceilz(cy-y)].btAlpha, 255, max(0,floorz(((y*2)-1)*4)));
									pDestPixels[ix * sizeDest.cx + ceilz(cy-y)].btAlpha = bnd(alphaNEW, pRGBSrc->btAlpha);
									pDestPixels[ix * sizeDest.cx + floorz(cy+y)].btAlpha = bnd(alphaOLD, pRGBSrc->btAlpha);
								}
								else
								{
									for(int i = ceilz(cy-y); i <= floorz(cy+y); i++)
									{
										// USING BND COZ ITS FASTER, but should this be AVG? This loop is expensive
										//alphaOLD = pSrcPixels[ix * sizeDest.cx + i].btAlpha;
										//if (alphaOLD==255) break;

										alphaOLD = pDestPixels[ix * sizeDest.cx + i].btAlpha;
										pDestPixels[ix * sizeDest.cx + i].btAlpha = bnd(alphaOLD,pRGBSrc->btAlpha);//min(255, alpha + INT_ALPHA[255][255-alpha]);

									}
								}
								
								
								//dwTicks3 += GetTickCount()-dwTicks;
								
							}
						}
					}
				}
			}

			

		}
	}

	//TRACE("\t\t\tProcess 1= %.3f seconds\n", (float)(dwTicks1)/1000.0f);
	//TRACE("\t\t\tProcess 2= %.3f seconds\n", (float)(dwTicks2)/1000.0f);
	//TRACE("\t\t\tProcess 3= %.3f seconds\n", (float)(dwTicks3)/1000.0f);

/**** BRESENHAM

	for (int nX = sx; nX <= ex; nX++)
	{
		for (int nY = sy; nY <= ey; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			bool bDraw = pRGBSrc->btAlpha>0;

			pRGBSrc->btAlpha = max(0,(pRGBSrc->btAlpha + 0) >> 2);
			//int newAlpha = (pRGBSrc->btAlpha + pRGBSrc->btAlpha + pRGBSrc->btAlpha + pRGBDest->btAlpha) >> 2;
			int newAlpha = pRGBSrc->btAlpha;
			
			//bDraw = bDraw && (pDestPixels[nY * sizeSrc.cx + nX].btAlpha != 255);

			if ((pRGBSrc->btAlpha>0) && bDraw)
			{

				// Circle
				int x = m_nRadius -1;
				int y = 0;
				int radiusError = 1-x;

				int x0 = nX;
				int y0 = nY;
 
				while(x>=y)
				{
					int x1 = x0 + x; bool bx1 = (x1<sizeSrc.cx);
					int x2 = x0 + y; bool bx2 = (x2<sizeSrc.cx);
					int x3 = x0 - x; bool bx3 = (x3>0);
					int x4 = x0 - y; bool bx4 = (x4>0);
					int y1 = y0 + x; bool by1 = (y1<sizeSrc.cy);
					int y2 = y0 + y; bool by2 = (y2<sizeSrc.cy);
					int y3 = y0 - x; bool by3 = (y3>0);
					int y4 = y0 - y; bool by4 = (y4>0);
					
					if (bx1 && by2) { RGBX* pRGBDest = &pDestPixels[y2 * sizeSrc.cx + x1]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx2 && by1) { RGBX* pRGBDest = &pDestPixels[y1 * sizeSrc.cx + x2]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx3 && by2) { RGBX* pRGBDest = &pDestPixels[y2 * sizeSrc.cx + x3]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx4 && by1) { RGBX* pRGBDest = &pDestPixels[y1 * sizeSrc.cx + x4]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx3 && by4) { RGBX* pRGBDest = &pDestPixels[y4 * sizeSrc.cx + x3]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx4 && by3) { RGBX* pRGBDest = &pDestPixels[y3 * sizeSrc.cx + x4]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx1 && by4) { RGBX* pRGBDest = &pDestPixels[y4 * sizeSrc.cx + x1]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }
					if (bx2 && by3) { RGBX* pRGBDest = &pDestPixels[y3 * sizeSrc.cx + x2]; pRGBDest->btAlpha = max(pRGBDest->btAlpha, (pRGBDest->btAlpha+pRGBSrc->btAlpha*3) >> 2); }

					y++;
					
					if (radiusError<0) radiusError += 2 * y + 1;
					else
					{
						x--;
						radiusError += 2 * (y - x + 1);
					}
				}
				
				
			}
		}
	}
*/
	int red = GetRValue(m_crColor);
	int green = GetGValue(m_crColor);
	int blue = GetBValue(m_crColor);

	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			
			if (pRGBDest->btAlpha==255)
			{
				pRGBDest->btRed = red;
				pRGBDest->btGreen = green;
				pRGBDest->btBlue = blue;
				pRGBDest->btAlpha = 255;
			}
			else if (pRGBDest->btAlpha>0)
			{	
				pRGBDest->btRed = INT_ALPHA[red][pRGBDest->btAlpha];
				pRGBDest->btGreen = INT_ALPHA[green][pRGBDest->btAlpha];
				pRGBDest->btBlue = INT_ALPHA[blue][pRGBDest->btAlpha];
			} 
		}
	}
	
	return TRUE;
}



//////////////////////////////////////////////////////////////////////

CHZBitmapShifter::CHZBitmapShifter(int nAngle, int nRadius)
{
	m_strFunctionName = "Shifter";
	m_nAngle = nAngle;
	m_nRadius= nRadius;
}

CHZBitmapShifter::~CHZBitmapShifter()
{
}

BOOL CHZBitmapShifter::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	int dx, dy;

	if (m_nAngle==90) { dx=m_nRadius; dy=0; }
	else if (m_nAngle==180) { dx=0; dy=-m_nRadius;}
	else if (m_nAngle==270) { dx=-m_nRadius; dy=0; }
	else if (m_nAngle==0) { dx=0; dy=m_nRadius; }
	else
	{
		dx = m_nRadius * sin(m_nAngle*PI/180);
		dy = m_nRadius * cos(m_nAngle*PI/180);
	}

	/*
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			int xpix = nX - dx;
			int ypix = nY + dy;

			if (xpix >= 0 && ypix >= 0 && xpix < sizeSrc.cx && ypix < sizeSrc.cy)
			{
				RGBX* pRGBSrc = &pSrcPixels[ypix * sizeSrc.cx + xpix];
				RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

				pRGBDest->btRed = pRGBSrc->btRed;
				pRGBDest->btGreen = pRGBSrc->btGreen;
				pRGBDest->btBlue = pRGBSrc->btBlue;
				pRGBDest->btAlpha = pRGBSrc->btAlpha;
			}
		}
	}
	*/

	for (int xpix = max(0, -dx); xpix < min(sizeSrc.cx, sizeSrc.cx - dx); xpix++)
	{
		for (int ypix = max(0, dy); ypix < min(sizeSrc.cy, sizeSrc.cy+dy); ypix++)
		{
			RGBX* pRGBSrc = &pSrcPixels[ypix * sizeSrc.cx + xpix];
			RGBX* pRGBDest = &pDestPixels[(ypix - dy) * sizeSrc.cx + (xpix + dx)];

			pRGBDest->btRed = pRGBSrc->btRed;
			pRGBDest->btGreen = pRGBSrc->btGreen;
			pRGBDest->btBlue = pRGBSrc->btBlue;
			pRGBDest->btAlpha = pRGBSrc->btAlpha;
		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapSweeper::CHZBitmapSweeper(COLORREF crColor, int nAngle, int nRadius)
{
	m_strFunctionName = "Sweeper";
	m_crColor = crColor;
	m_nAngle = nAngle;
	m_nRadius = nRadius + 1;
}

CHZBitmapSweeper::~CHZBitmapSweeper()
{
}

BOOL CHZBitmapSweeper::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default

	int red = GetRValue(m_crColor);
	int green = GetGValue(m_crColor);
	int blue = GetBValue(m_crColor);

	int SIN_OFF = 0;//1 * sin(m_nAngle*PI/180);
	int COS_OFF = 0;//1 * cos(m_nAngle*PI/180);
	int SIN = m_nRadius * sin(m_nAngle*PI/180);
	int COS = m_nRadius * cos(m_nAngle*PI/180);
	
	int xStart = (SIN<=0)?0:sizeSrc.cx-1;
	int xEnd = (SIN>0)?0:sizeSrc.cx-1;
	int yStart = (COS>=0)?0:sizeSrc.cy-1;
	int yEnd = (COS<0)?0:sizeSrc.cy-1;
	/*
	int xStart = (SIN>=0)?0:sizeSrc.cx-1;
	int xEnd = (SIN<0)?0:sizeSrc.cx-1;
	int yStart = (COS<=0)?0:sizeSrc.cy-1;
	int yEnd = (COS>0)?0:sizeSrc.cy-1;
	*/

	for (int nX = xStart; nX != xEnd; nX += ((xStart>xEnd)?-1:1))
	{
		for (int nY = yStart; nY != yEnd; nY += ((yStart>yEnd)?-1:1))
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			int inc = 1;

			if ((nY + inc) >= sizeSrc.cy || (nX + inc) >= sizeSrc.cx) continue;

			short baseColor;// = pRGBSrc->btAlpha;
			short numLevels = 256;
			short intensityBits = 8;
			unsigned short intensityShift, errorAdj, errorAcc;
			unsigned short errorAcctemp, weighting, weightingComplementMask;
			short deltaX, deltaY, temp, xDir;

			int alphaT, alphaL, alphaB, alphaR;
			

			//if (nY==20 && nX==20) pRGBSrc->btAlpha=255;


			if (pRGBSrc->btAlpha>0)
			//if ((nX==200 && (nY==200 || nY==201 || nY==202 || nY==203)) || (nX==210 && nY==200))
			{
				

				alphaT = pSrcPixels[(nY-inc) * sizeSrc.cx + nX].btAlpha;
				alphaL = pSrcPixels[nY * sizeSrc.cx + (nX-inc)].btAlpha;
				alphaB = pSrcPixels[(nY+inc) * sizeSrc.cx + nX].btAlpha;
				alphaR = pSrcPixels[nY * sizeSrc.cx + (nX+inc)].btAlpha;

				if (alphaT + alphaL + alphaB + alphaR + pRGBSrc->btAlpha < 1200)
				{
					int x1 = max(0, min(sizeDest.cx-1, nX + SIN));
					int y1 = max(0, min(sizeDest.cy-1, nY - COS));
					int x0 = max(0, min(sizeDest.cx-1, nX + SIN_OFF));
					int y0 = max(0, min(sizeDest.cy-1, nY - COS_OFF));

					baseColor = 255;//pRGBSrc->btAlpha;

					if (y0 > y1) 
					{
						temp = y0; y0 = y1; y1 = temp;
						temp = x0; x0 = x1; x1 = temp;
					}

					baseColor = pRGBSrc->btAlpha;
					pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor);
					//baseColor = 255;

					if ((deltaX = x1 - x0) >= 0) 
					{
						xDir = 1;
					} 
					else 
					{
						xDir = -1;
						deltaX = -deltaX;
					}

					if ((deltaY = y1 - y0) == 0) 
					{
						while (deltaX-- != 0)
						{
							x0 += xDir;
							pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor);
						}
						continue;
					}

					if (deltaX == 0)
					{
						while (--deltaY != 0)
						{
							y0++;
							pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor);
						} 
						continue;
					}

					if (deltaX == deltaY) 
					{
						while (--deltaY != 0)
						{
							x0 += xDir;
							y0++;
							pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor);
						} 
						continue;
					}

					errorAcc = 0;
					intensityShift = 16 - intensityBits;
					weightingComplementMask = numLevels - 1;
					if (deltaY > deltaX) 
					{
						errorAdj = ((unsigned long) deltaX << 16) / (unsigned long) deltaY;
						while (--deltaY) 
						{
							errorAcctemp = errorAcc;
							errorAcc += errorAdj;
							if (errorAcc <= errorAcctemp) 
							{
								x0 += xDir;
							}
							y0++;
							weighting = errorAcc >> intensityShift;
							pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor - weighting);
							pDestPixels[y0 * sizeSrc.cx + x0 + xDir].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0 + xDir].btAlpha, baseColor - (weighting ^ weightingComplementMask));
						}
						pDestPixels[y1 * sizeSrc.cx + x1].btAlpha = bnd(pDestPixels[y1 * sizeSrc.cx + x1].btAlpha, baseColor);
						continue;
					}

					errorAdj = ((unsigned long) deltaY << 16) / (unsigned long) deltaX;
					while (--deltaX) 
					{
						errorAcctemp = errorAcc;
						errorAcc += errorAdj;
						if (errorAcc <= errorAcctemp) 
						{
							y0++;
						}
						x0 += xDir;
						weighting = errorAcc >> intensityShift;
						pDestPixels[y0 * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[y0 * sizeSrc.cx + x0].btAlpha, baseColor - weighting);
						pDestPixels[(y0 + 1) * sizeSrc.cx + x0].btAlpha = bnd(pDestPixels[(y0+1) * sizeSrc.cx + x0].btAlpha, baseColor - (weighting ^ weightingComplementMask));
					}

					baseColor = pRGBSrc->btAlpha;
					pDestPixels[y1 * sizeSrc.cx + x1].btAlpha = avg(pDestPixels[y1 * sizeSrc.cx + x1].btAlpha, baseColor);
				}
				else
				{
					// Bresenham
					int x1 = nX + SIN;
					int y1 = nY - COS;
					int x0 = nX + 2 * sin(m_nAngle*PI/180);
					int y0 = nY - 2 * cos(m_nAngle*PI/180);

					int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
					int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
					int err = (dx>dy ? dx : -dy)/2, e2;
 
					while (true)
					{
						if (x0 > 0 && y0 > 0 && x0 < sizeSrc.cx-1 && y0 < sizeSrc.cy-1)
						{
							RGBX* pRGBDest = &pDestPixels[y0 * sizeSrc.cx + x0];
							pRGBDest->btAlpha = 255;//bnd(pRGBDest->btAlpha, pRGBSrc->btAlpha);
							//pRGBDest = &pDestPixels[(y0 - COS_OFF + 1) * sizeSrc.cx + (x0 + SIN_OFF + 1)];
							//pRGBDest->btAlpha = bnd(pRGBDest->btAlpha, pRGBSrc->btAlpha);
							
							//pRGBDest->btAlpha = max(pRGBDest->btAlpha, pRGBSrc->btAlpha);
							//pRGBDest->btRed = INT_ALPHA[red][pRGBDest->btAlpha];
							//pRGBDest->btGreen = INT_ALPHA[green][pRGBDest->btAlpha];
							//pRGBDest->btBlue = INT_ALPHA[blue][pRGBDest->btAlpha];

							pRGBDest = &pDestPixels[y0 * sizeSrc.cx + x0 + 1];
							pRGBDest->btAlpha = 255;
							
						}

						if (x0==x1 && y0==y1) break;
						e2 = err;
						if (e2 >-dx) { err -= dy; x0 += sx; }
						if (e2 < dy) { err += dx; y0 += sy; }

					}
				}
			}
		}
	}

	//int red = GetRValue(m_crColor);
	//int green = GetGValue(m_crColor);
	//int blue = GetBValue(m_crColor);

	for (int nX = 0; nX < sizeDest.cx; nX++)
	{
		for (int nY = 0; nY < sizeDest.cy; nY++)
		{
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			
			if (pRGBDest->btAlpha>254)
			{
				pRGBDest->btAlpha = 255;
				pRGBDest->btRed = red;
				pRGBDest->btGreen = green;
				pRGBDest->btBlue = blue;
			}
			else if (pRGBDest->btAlpha>0)
			{
				pRGBDest->btRed = INT_ALPHA[red][pRGBDest->btAlpha];
				pRGBDest->btGreen = INT_ALPHA[green][pRGBDest->btAlpha];
				pRGBDest->btBlue = INT_ALPHA[blue][pRGBDest->btAlpha];
			}
		}
	}
	
	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapNegator::CHZBitmapNegator()
{
	m_strFunctionName = "Negator";
}

CHZBitmapNegator::~CHZBitmapNegator()
{
}

BOOL CHZBitmapNegator::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);
	
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			float fdiv = ALPHA[(int)pRGBSrc->btAlpha];
			pRGBDest->btRed = (255 - pRGBSrc->btRed) * fdiv;
			pRGBDest->btGreen = (255 - pRGBSrc->btGreen) * fdiv;
			pRGBDest->btBlue = (255 - pRGBSrc->btBlue) * fdiv;
			pRGBDest->btAlpha = pRGBSrc->btAlpha;
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

CHZBitmapColorizer::CHZBitmapColorizer(COLORREF crColor)
{
	m_strFunctionName = "Colorizer";
	m_crColor = crColor;
}

CHZBitmapColorizer::~CHZBitmapColorizer()
{
}

BOOL CHZBitmapColorizer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);
	
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			double dRed = 0, dGreen = 0, dBlue = 0, dAlpha = 0;

			/*
			float fdiv = ALPHA[(int)pRGBSrc->btAlpha];
			dRed = sqrt((double)pRGBSrc->Gray().btRed * GetRValue(m_crColor)) * fdiv;
			dGreen = sqrt((double)pRGBSrc->Gray().btGreen * GetGValue(m_crColor)) * fdiv;
			dBlue = sqrt((double)pRGBSrc->Gray().btBlue * GetBValue(m_crColor)) * fdiv;
			*/
			
			dRed = (((double)pRGBSrc->btRed + GetRValue(m_crColor))/2) ; 
			dGreen = (((double)pRGBSrc->btGreen + GetGValue(m_crColor))/2) ;
			dBlue = (((double)pRGBSrc->btBlue + GetBValue(m_crColor))/2);
			

			dAlpha = 255;// pRGBSrc->btAlpha;

			dRed = max(0,min(255, dRed));
			dGreen = max(0,min(255, dGreen));
			dBlue = max(0,min(255, dBlue));
			dAlpha = max(0,min(255, dAlpha));

			pRGBDest->btRed = dRed;
			pRGBDest->btGreen = dGreen;
			pRGBDest->btBlue = dBlue;
			pRGBDest->btAlpha = dAlpha;

		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

CHZBitmapMasker::CHZBitmapMasker(COLORREF crMask, bool bAntiAlias)
{
	m_strFunctionName = "Masker";
	m_crMask = crMask;
	m_bAntiAlias = bAntiAlias;
}

CHZBitmapMasker::~CHZBitmapMasker()
{
}

BOOL CHZBitmapMasker::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	int red = GetRValue(m_crMask);
	int green = GetGValue(m_crMask);
	int blue = GetBValue(m_crMask);

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			int alpha = pRGBSrc->btAlpha;//0.2126 * pRGBSrc->btRed + 0.7152 * pRGBSrc->btGreen + 0.0722 * pRGBSrc->btBlue;

			if (!m_bAntiAlias && alpha>0)
			{
				if (pRGBSrc->Equals(RGBX(red,green,blue,255)))
				{
					pRGBDest->btRed = 0;
					pRGBDest->btGreen = 0;
					pRGBDest->btBlue = 0;
					pRGBDest->btAlpha = 0;
				}
				else
				{
					pRGBDest->btRed = pRGBSrc->btRed;
					pRGBDest->btGreen = pRGBSrc->btGreen;
					pRGBDest->btBlue = pRGBSrc->btBlue;
					pRGBDest->btAlpha = 255;
				}

				/*
				pRGBDest->btRed = min(255,pRGBSrc->btRed * BETA[pRGBSrc->btAlpha]);
				pRGBDest->btGreen = min(255,pRGBSrc->btGreen * BETA[pRGBSrc->btAlpha]);
				pRGBDest->btBlue = min(255,pRGBSrc->btBlue * BETA[pRGBSrc->btAlpha]);
				pRGBDest->btAlpha = 255;
				*/
			}
			else if (m_bAntiAlias && alpha>0)
			{
				pRGBDest->btRed = INT_ALPHA[red][alpha];
				pRGBDest->btGreen = INT_ALPHA[green][alpha];
				pRGBDest->btBlue = INT_ALPHA[blue][alpha];
				pRGBDest->btAlpha = alpha;
			}
		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////


CHZBitmapBoxer::CHZBitmapBoxer(COLORREF crColor, CRect rcRect, int nAmount, int nBorder)
{
	m_strFunctionName = "Boxer";
	m_crColor = crColor;
	m_rcRect = rcRect;
	m_nAmount = max(0, min(10, nAmount));
	m_nBorder = max(0, min(10, nBorder));
}

CHZBitmapBoxer::~CHZBitmapBoxer()
{
}

BOOL CHZBitmapBoxer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	float multiplier = float(m_nAmount) / 10.0f;
	if (multiplier<0) return TRUE;

	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	
	m_rcRect.NormalizeRect();
	m_rcRect.left = max(0, m_rcRect.left);
	m_rcRect.right = min(sizeSrc.cx-1, m_rcRect.right);
	m_rcRect.top = max(0, m_rcRect.top);
	m_rcRect.bottom = min(sizeSrc.cy-1, m_rcRect.bottom);

	int alpha = 255 * multiplier;
	int red = min(alpha, GetRValue(m_crColor) * multiplier);
	int green = min(alpha, GetGValue(m_crColor) * multiplier);
	int blue = min(alpha, GetBValue(m_crColor) * multiplier);
	RGBX rgbFill(red, green, blue, alpha);

	for (int nX = m_rcRect.left; nX < m_rcRect.right+1; nX++)
	{
		for (int nY = m_rcRect.top; nY < m_rcRect.bottom+1; nY++)
		{
			if (m_nBorder==0 || (m_nBorder>0 &&
				!((nX-m_rcRect.left)>m_nBorder && (m_rcRect.right-nX)>m_nBorder && (nY-m_rcRect.top)>m_nBorder && (m_rcRect.bottom-nY)>m_nBorder)))
			{
				memcpy(&pDestPixels[nY * sizeSrc.cx + nX], &rgbFill, sizeof(RGBX));

				/*
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
				RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
				pRGBDest->btRed = red;
				pRGBDest->btGreen = green;
				pRGBDest->btBlue = blue;
				pRGBDest->btAlpha = avg(alpha, 255 -  pRGBSrc->btAlpha);
				*/
			}
		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapShader::CHZBitmapShader(int nAmount)
{
	m_strFunctionName = "Shader";
	m_nAmount = nAmount;
}

CHZBitmapShader::~CHZBitmapShader()
{
}

BOOL CHZBitmapShader::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];


		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapFlipper::CHZBitmapFlipper(BOOL bHorz, BOOL bVert) : m_bHorz(bHorz), m_bVert(bVert)
{
	m_strFunctionName = "Flipper";
}

CHZBitmapFlipper::~CHZBitmapFlipper()
{
}

BOOL CHZBitmapFlipper::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		int nDestX = m_bHorz ? sizeDest.cx - nX - 1 : nX;

		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];

			int nDestY = m_bVert ? sizeDest.cy - nY - 1 : nY;
			RGBX* pRGBDest = &pDestPixels[nDestY * sizeDest.cx + nDestX];

			*pRGBDest = *pRGBSrc;
		}
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////

CHZBitmapClamper::CHZBitmapClamper(bool bMultiply, bool bFlatten, bool bSwap)
{
	m_strFunctionName = "Clamper";
	m_bMultiply = bMultiply;
	m_bFlatten = bFlatten;
}

CHZBitmapClamper::~CHZBitmapClamper()
{
}


BOOL CHZBitmapClamper::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	ASSERT (sizeSrc == sizeDest);

	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			/*
			pRGBDest->btRed = pRGBSrc->btRed;
			pRGBDest->btGreen = pRGBSrc->btGreen;
			pRGBDest->btBlue = pRGBSrc->btBlue;
			pRGBDest->btAlpha = pRGBSrc->btAlpha;
			*/

			if (pRGBSrc->btAlpha>0 || m_bFlatten)
			{
				if (m_bMultiply)
				{
					pRGBDest->btAlpha = m_bFlatten?255:pRGBSrc->btAlpha;
					pRGBDest->btRed = min(pRGBDest->btAlpha, INT_ALPHA[m_bSwap ? pRGBSrc->btRed : pRGBSrc->btBlue][pRGBDest->btAlpha]);
					pRGBDest->btGreen = min(pRGBDest->btAlpha, INT_ALPHA[pRGBSrc->btGreen][pRGBDest->btAlpha]);
					pRGBDest->btBlue = min(pRGBDest->btAlpha, INT_ALPHA[m_bSwap ? pRGBSrc->btBlue : pRGBSrc->btRed][pRGBDest->btAlpha]);
				}
				else
				{
					pRGBDest->btAlpha = m_bFlatten?255:pRGBSrc->btAlpha;
					pRGBDest->btRed = min(255, (m_bSwap?pRGBSrc->btRed:pRGBSrc->btBlue) * BETA[pRGBDest->btAlpha]);
					pRGBDest->btGreen = min(255, pRGBSrc->btGreen * BETA[pRGBDest->btAlpha]);
					pRGBDest->btBlue = min(255, (m_bSwap?pRGBSrc->btBlue:pRGBSrc->btRed) * BETA[pRGBDest->btAlpha]);
				}
			}
			
		}
	}

	return TRUE;


	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];

			/*
			pRGBDest->btRed = pRGBSrc->btRed;
			pRGBDest->btGreen = pRGBSrc->btGreen;
			pRGBDest->btBlue = pRGBSrc->btBlue;
			pRGBDest->btAlpha = pRGBSrc->btAlpha;
			*/

			if (pRGBSrc->btAlpha>0)
			{

				int R = pRGBSrc->btRed;
				int G = pRGBSrc->btGreen;
				int B = pRGBSrc->btBlue;

				if (R+G+B>0)
				{


					
					int Y = 0.213 * R + 0.715 * G + 0.072 * B;
					int U = -0.115 * R - 0.385 * G + 0.5 * B;
					int V = 0.5 * R - 0.454 * G - 0.046 * B;

					Y = max(16, min(235, Y));
//					U = max(16, min(240, U));
//					V = max(16, min(240, V));

					R = Y + (1.575 * V);
					G = Y - (0.468 * V) - (0.187 * U);
					B = Y + (1.856 * U);

					R = max(0, min(255, R));
					G = max(0, min(255, G));
					B = max(0, min(255, B));

					/*
					R = max(16, min(235, R));
					G = max(16, min(235, G));
					B = max(16, min(235, B));
					*/



					/*

					if (R > 255) R = 255;
					if (G > 255) G = 255;
					if (B > 255) B = 255;
				
					if (R < 0) R = 0;
					if (G < 0) G = 0;
					if (B < 0) B = 0;

				

					R = R * 220 / 256;
					G = G * 220 / 256;
					B = B * 220 / 256;


	*/


					/*
					pRGBDest->btRed = R;
					pRGBDest->btGreen = G;
					pRGBDest->btBlue = B;
					*/
				
					float alpha = ALPHA[pRGBSrc->btAlpha];
					pRGBDest->btRed = R * alpha;
					pRGBDest->btGreen = G * alpha;
					pRGBDest->btBlue = B * alpha;

				}
				
				pRGBDest->btAlpha = pRGBSrc->btAlpha;

			}
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

CHZColorReplacer::CHZColorReplacer(COLORREF crFrom, COLORREF crTo) : m_crFrom(crFrom), m_crTo(crTo)
{
}

CHZColorReplacer::~CHZColorReplacer()
{
}

BOOL CHZColorReplacer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest)
{
	DWORD dwTick = GetTickCount();
	BOOL bRes = TRUE;

	if (m_crFrom == m_crTo)
		bRes = CHZBitmapProcessor::ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	else
	{
		RGBX rgbFrom(m_crFrom,255), rgbTo(m_crTo,255);

		for (int nX = 0; nX < sizeSrc.cx; nX++)
		{
			for (int nY = 0; nY < sizeSrc.cy; nY++)
			{
				RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
				RGBX* pRGBDest = &pDestPixels[nY * sizeDest.cx + nX];

				if (pRGBSrc->Equals(rgbFrom))
				{
					rgbTo.btAlpha = pRGBSrc->btAlpha;
					*pRGBDest = rgbTo;
				}
				else
					*pRGBDest = *pRGBSrc;
			} 
		}
	}

	return bRes;
}

//////////////////////////////////////////////////////////////////////

