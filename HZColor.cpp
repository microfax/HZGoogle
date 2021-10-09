/****************************************************************************\
Datei	: Color.cpp
Projekt: Farbverwaltung
Inhalt : CHZColor Implementierung
Datum	: 10.01.1999
Autor	: Christian Rodemeyer
Hinweis: © 1999 by Christian Rodemeyer
				 Info über HLS Konvertierungsfunktion
				 - Foley and Van Dam: "Fundamentals of Interactive Computer Graphics"	
				 - MSDN: 'HLS Color Spaces'
				 - MSDN: 'Converting Colors Between RGB and HLS' 
\****************************************************************************/

#include "pch.h"
#include "HZColor.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************\
 CHZColor: Implementierung
\****************************************************************************/
const CHZColor::DNamedColor CHZColor::m_namedColor[CHZColor::numNamedColors] =
{
	{aliceblue								, L"aliceblue"},
	{antiquewhite							, L"antiquewhite"},
	{aqua									, L"aqua"},
	{aquamarine								, L"aquamarine"},
	{azure									, L"azure"},
	{beige									, L"beige"},
	{bisque									, L"bisque"},
	{black									, L"black"},
	{blanchedalmond							, L"blanchedalmond"},
	{blue									, L"blue"},
	{blueviolet								, L"blueviolet"},
	{brown									, L"brown"},
	{burlywood								, L"burlywood"},
	{cadetblue								, L"cadetblue"},
	{chartreuse								, L"chartreuse"},
	{chocolate								, L"chocolate"},
	{coral									, L"coral"},
	{cornflower								, L"cornflower"},
	{cornsilk								, L"cornsilk"},
	{crimson								, L"crimson"},
	{cyan									, L"cyan"},
	{darkblue								, L"darkblue"},
	{darkcyan								, L"darkcyan"},
	{darkgoldenrod							, L"darkgoldenrod"},
	{darkgray								, L"darkgray"},
	{darkgreen								, L"darkgreen"},
	{darkkhaki								, L"darkkhaki"},
	{darkmagenta							, L"darkmagenta"},
	{darkolivegreen							, L"darkolivegreen"},
	{darkorange								, L"darkorange"},
	{darkorchid								, L"darkorchid"},
	{darkred								, L"darkred"},
	{darksalmon								, L"darksalmon"},
	{darkseagreen							, L"darkseagreen"},
	{darkslateblue							, L"darkslateblue"},
	{darkslategray							, L"darkslategray"},
	{darkturquoise							, L"darkturquoise"},
	{darkviolet								, L"darkviolet"},
	{deeppink								, L"deeppink"},
	{deepskyblue							, L"deepskyblue"},
	{dimgray								, L"dimgray"},
	{dodgerblue								, L"dodgerblue"},
	{firebrick								, L"firebrick"},
	{floralwhite							, L"floralwhite"},
	{forestgreen							, L"forestgreen"},
	{fuchsia								, L"fuchsia"},
	{gainsboro								, L"gainsboro"},
	{ghostwhite								, L"ghostwhite"},
	{gold									, L"gold"},
	{goldenrod								, L"goldenrod"},
	{gray									, L"gray"},
	{green									, L"green"},
	{greenyellow							, L"greenyellow"},
	{honeydew								, L"honeydew"},
	{hotpink								, L"hotpink"},
	{indianred								, L"indianred"},
	{indigo									, L"indigo"},
	{ivory									, L"ivory"},
	{khaki									, L"khaki"},
	{lavender								, L"lavender"},
	{lavenderblush							, L"lavenderblush"},
	{lawngreen								, L"lawngreen"},
	{lemonchiffon							, L"lemonchiffon"},
	{lightblue								, L"lightblue"},
	{lightcoral								, L"lightcoral"},
	{lightcyan								, L"lightcyan"},
	{lightgoldenrodyellow					, L"lightgoldenrodyellow"},
	{lightgreen								, L"lightgreen"},
	{lightgrey								, L"lightgrey"},
	{lightpink								, L"lightpink"},
	{lightsalmon							, L"lightsalmon"},
	{lightseagreen							, L"lightseagreen"},
	{lightskyblue							, L"lightskyblue"},
	{lightslategray							, L"lightslategray"},
	{lightsteelblue							, L"lightsteelblue"},
	{lightyellow							, L"lightyellow"},
	{lime									, L"lime"},
	{limegreen								, L"limegreen"},
	{linen									, L"linen"},
	{magenta								, L"magenta"},
	{maroon									, L"maroon"},
	{mediumaquamarine						, L"mediumaquamarine"},
	{mediumblue								, L"mediumblue"},
	{mediumorchid							, L"mediumorchid"},
	{mediumpurple							, L"mediumpurple"},
	{mediumseagreen							, L"mediumseagreen"},
	{mediumslateblue						, L"mediumslateblue"},
	{mediumspringgreen						, L"mediumspringgreen"},
	{mediumturquoise						, L"mediumturquoise"},
	{mediumvioletred						, L"mediumvioletred"},
	{midnightblue							, L"midnightblue"},
	{mintcream								, L"mintcream"},
	{mistyrose								, L"mistyrose"},
	{moccasin								, L"moccasin"},
	{navajowhite							, L"navajowhite"},
	{navy									, L"navy"},
	{oldlace								, L"oldlace"},
	{olive									, L"olive"},
	{olivedrab								, L"olivedrab"},
	{orange									, L"orange"},
	{orangered								, L"orangered"},
	{orchid									, L"orchid"},
	{palegoldenrod							, L"palegoldenrod"},
	{palegreen								, L"palegreen"},
	{paleturquoise							, L"paleturquoise"},
	{palevioletred							, L"palevioletred"},
	{papayawhip								, L"papayawhip"},
	{peachpuff								, L"peachpuff"},
	{peru									, L"peru"},
	{pink									, L"pink"},
	{plum									, L"plum"},
	{powderblue								, L"powderblue"},
	{purple									, L"purple"},
	{red									, L"red"},
	{rosybrown								, L"rosybrown"},
	{royalblue								, L"royalblue"},
	{saddlebrown							, L"saddlebrown"},
	{salmon									, L"salmon"},
	{sandybrown								, L"sandybrown"},
	{seagreen								, L"seagreen"},
	{seashell								, L"seashell"},
	{sienna									, L"sienna"},
	{silver									, L"silver"},
	{skyblue								, L"skyblue"},
	{slateblue								, L"slateblue"},
	{slategray								, L"slategray"},
	{snow									, L"snow"},
	{springgreen							, L"springgreen"},
	{steelblue								, L"steelblue"},
	{tan									, L"tan"},
	{teal									, L"teal"},
	{thistle								, L"thistle"},
	{tomato									, L"tomato"},
	{turquoise								, L"turquoise"},
	{violet									, L"violet"},
	{wheat									, L"wheat"},
	{white									, L"white"},
	{whitesmoke								, L"whitesmoke"},
	{yellow									, L"yellow"},
	{yellowgreen							, L"yellowgreen"}
};


CHZColor CHZColor::FromString(LPCTSTR pcColor)
{
	CHZColor t;
	t.SetString(pcColor);
	return t;

}

CHZColor::CHZColor(CHZColor* pColor)
{
	m_color[c_red] = pColor->GetRed();
	m_color[c_green] = pColor->GetGreen();
	m_color[c_blue] = pColor->GetBlue();
	m_fCompare = pColor->m_fCompare;
	m_bIsLAB = pColor->m_bIsLAB;
	if (m_bIsLAB)
	{
		m_arrLab[0] = pColor->m_arrLab[0];
		m_arrLab[1] = pColor->m_arrLab[1];
		m_arrLab[2] = pColor->m_arrLab[2];
	}
	m_bIsRGB = true;
	m_bIsHLS = false;
}



CHZColor::CHZColor(COLORREF cr)
: m_bIsRGB(true), m_bIsHLS(false), m_colorref(cr)
{}

CHZColor::operator COLORREF() const
{
	const_cast<CHZColor*>(this)->ToRGB();
	return m_colorref;
}

CHZColor& CHZColor::operator=(CHZColor col)
{
	m_color[c_red] = col.GetRed();
	m_color[c_green] = col.GetGreen();
	m_color[c_blue] = col.GetBlue();
	m_fCompare = col.m_fCompare;
	m_bIsLAB = col.m_bIsLAB;
	if (m_bIsLAB)
	{
		m_arrLab[0] = col.m_arrLab[0];
		m_arrLab[1] = col.m_arrLab[1];
		m_arrLab[2] = col.m_arrLab[2];
	}
	m_bIsHLS = false;
	return *this;
}

CHZColor& CHZColor::operator = (COLORREF cr IN)
{
	const_cast<CHZColor*>(this)->ToRGB();
	m_color[c_red] = static_cast<unsigned char>(GetRValue(cr));
	m_color[c_green] = static_cast<unsigned char>(GetGValue(cr));
	m_color[c_blue] = static_cast<unsigned char>(GetBValue(cr));
	m_bIsHLS = false;
	return *this;
}

// RGB

void CHZColor::SetRed(int red)
{
	red = min(255, max(0, red));
	ToRGB();
	m_color[c_red] = static_cast<unsigned char>(red);
	m_bIsHLS = false;
}

void CHZColor::SetGreen(int green)
{
	green = min(255, max(0, green));
	ToRGB();
	m_color[c_green] = static_cast<unsigned char>(green);
	m_bIsHLS = false;
}

void CHZColor::SetBlue(int blue)
{
	blue = min(255, max(0, blue));
	ToRGB();
	m_color[c_blue] = static_cast<unsigned char>(blue);
	m_bIsHLS = false;
}

void CHZColor::SetRGB(int red, int blue, int green)
{
	ASSERT(0 <= red && red <= 255);
	ASSERT(0 <= green && green <= 255);
	ASSERT(0 <= blue && blue <= 255);

	m_color[c_red]	 = static_cast<unsigned char>(red);
	m_color[c_green] = static_cast<unsigned char>(green);
	m_color[c_blue]	= static_cast<unsigned char>(blue);
	m_bIsHLS = false;
	m_bIsRGB = true;
}

int CHZColor::GetRed() const
{
	const_cast<CHZColor*>(this)->ToRGB();
	return m_color[c_red];
}

int CHZColor::GetGreen() const
{
	const_cast<CHZColor*>(this)->ToRGB();
	return m_color[c_green];
}

int CHZColor::GetBlue() const
{
	const_cast<CHZColor*>(this)->ToRGB();
	return m_color[c_blue];
}

// HSL

void CHZColor::SetHue(float hue)
{
	ASSERT(hue >= 0.0 && hue <= 360.0);

	ToHLS();
	m_hue = hue;
	m_bIsRGB = false;
}

void CHZColor::SetSaturation(float saturation)
{
	ASSERT(saturation >= 0.0 && saturation <= 1.0); // 0.0 ist undefiniert

	ToHLS();
	m_saturation = saturation;
	m_bIsRGB = false;
}

void CHZColor::SetLuminance(float luminance)
{
	luminance = min(1.0,luminance);
	luminance = max(0.0,luminance);
	//ASSERT(luminance >= 0.0 && luminance <= 1.0);

	ToHLS();
	m_luminance = luminance;
	m_bIsRGB = false;
}

void CHZColor::SetHLS(float hue, float luminance, float saturation)
{
	ASSERT(hue >= 0.0 && hue <= 360.0);
	ASSERT(luminance >= 0.0 && luminance <= 1.0);
	ASSERT(saturation >= 0.0 && saturation <= 1.0); // 0.0 ist undefiniert

	m_hue = hue;
	m_luminance = luminance;
	m_saturation = saturation;
	m_bIsRGB = false;
	m_bIsHLS = true;
}

float CHZColor::GetHue() const
{
	const_cast<CHZColor*>(this)->ToHLS();
	return m_hue;
}

float CHZColor::GetSaturation() const
{
	const_cast<CHZColor*>(this)->ToHLS();
	return m_saturation;
}

float CHZColor::GetLuminance() const
{
	const_cast<CHZColor*>(this)->ToHLS();
	return m_luminance;
}

// Konvertierung

void CHZColor::ToHLS() 
{
	if (!m_bIsHLS)
	{
		// Konvertierung
		unsigned char minval = min(m_color[c_red], min(m_color[c_green], m_color[c_blue]));
		unsigned char maxval = max(m_color[c_red], max(m_color[c_green], m_color[c_blue]));
		float mdiff	= float(maxval) - float(minval);
		float msum	 = float(maxval) + float(minval);
	 
		m_luminance = msum / 510.0f;

		if (maxval == minval) 
		{
			m_saturation = 0.0f;
			m_hue = 0.0f; 
		}	 
		else 
		{ 
			float rnorm = (maxval - m_color[c_red]	) / mdiff;			
			float gnorm = (maxval - m_color[c_green]) / mdiff;
			float bnorm = (maxval - m_color[c_blue] ) / mdiff;	 

			m_saturation = (m_luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

			if (m_color[c_red]	 == maxval) m_hue = 60.0f * (6.0f + bnorm - gnorm);
			if (m_color[c_green] == maxval) m_hue = 60.0f * (2.0f + rnorm - bnorm);
			if (m_color[c_blue]	== maxval) m_hue = 60.0f * (4.0f + gnorm - rnorm);
			if (m_hue > 360.0f) m_hue = m_hue - 360.0f;
		}
		m_bIsHLS = true;
	}
}

void CHZColor::ToRGB() 
{
	if (!m_bIsRGB)
	{
		if (m_saturation == 0.0) // Grauton, einfacher Fall
		{
			m_color[c_red] = m_color[c_green] = m_color[c_blue] = unsigned char(m_luminance * 255.0);
		}
		else
		{
			float rm1, rm2;
				 
			if (m_luminance <= 0.5f) rm2 = m_luminance + m_luminance * m_saturation;	
			else										 rm2 = m_luminance + m_saturation - m_luminance * m_saturation;
			rm1 = 2.0f * m_luminance - rm2;	 
			m_color[c_red]	 = ToRGB1(rm1, rm2, m_hue + 120.0f);	 
			m_color[c_green] = ToRGB1(rm1, rm2, m_hue);
			m_color[c_blue]	= ToRGB1(rm1, rm2, m_hue - 120.0f);
		}
		m_bIsRGB = true;
	}
}

unsigned char CHZColor::ToRGB1(float rm1, float rm2, float rh)
{
	if			(rh > 360.0f) rh -= 360.0f;
	else if (rh <	 0.0f) rh += 360.0f;
 
	if			(rh <	60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;	 
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;			
									 
	return static_cast<unsigned char>(rm1 * 255);
}

// Stringkonvertierung im Format RRGGBB

CString CHZColor::GetString() const 
{
	CString color;
	int red = GetRed();
	int green = GetGreen();
	int blue = GetBlue();
	color.Format(L"%02X%02X%02X", red, green, blue);
	return color;
}

float CHZColor::GetLabLuminance()
{
	float var_R = float(GetRed()) / 255.0;
	float var_G = float(GetGreen()) / 255.0;
	float var_B = float(GetBlue()) / 255.0;


	if (var_R > 0.04045) var_R = pow(((var_R + 0.055) / 1.055), 2.4);
	else                   var_R = var_R / 12.92;
	if (var_G > 0.04045) var_G = pow(((var_G + 0.055) / 1.055), 2.4);
	else                   var_G = var_G / 12.92;
	if (var_B > 0.04045) var_B = pow(((var_B + 0.055) / 1.055), 2.4);
	else                   var_B = var_B / 12.92;

	var_R = var_R * 100.;
	var_G = var_G * 100.;
	var_B = var_B * 100.;

	//Observer. = 2°, Illuminant = D65
	float X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
	float Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
	float Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

	//return Y / 100.0;


	float var_X = X / 95.047;         //ref_X =  95.047   Observer= 2°, Illuminant= D65
	float var_Y = Y / 100.000;          //ref_Y = 100.000
	float var_Z = Z / 108.883;          //ref_Z = 108.883

	if (var_X > 0.008856) var_X = pow(var_X, (1. / 3.));
	else                    var_X = (7.787 * var_X) + (16. / 116.);
	if (var_Y > 0.008856) var_Y = pow(var_Y, (1. / 3.));
	else                    var_Y = (7.787 * var_Y) + (16. / 116.);
	if (var_Z > 0.008856) var_Z = pow(var_Z, (1. / 3.));
	else                    var_Z = (7.787 * var_Z) + (16. / 116.);


	

	return ((116. * var_Y) - 16.) / 100.0;
	//a_s = 500. * (var_X - var_Y);
	//b_s = 200. * (var_Y - var_Z);


}

void CHZColor::SetLabLuminance(float l_s)
{
	l_s = max(0.0f, l_s);
	l_s = min(1.0f, l_s);

	l_s *= 100.0;
	float var_R = float(GetRed()) / 255.0;
	float var_G = float(GetGreen()) / 255.0;
	float var_B = float(GetBlue()) / 255.0;


	if (var_R > 0.04045) var_R = pow(((var_R + 0.055) / 1.055), 2.4);
	else                   var_R = var_R / 12.92;
	if (var_G > 0.04045) var_G = pow(((var_G + 0.055) / 1.055), 2.4);
	else                   var_G = var_G / 12.92;
	if (var_B > 0.04045) var_B = pow(((var_B + 0.055) / 1.055), 2.4);
	else                   var_B = var_B / 12.92;

	var_R = var_R * 100.;
	var_G = var_G * 100.;
	var_B = var_B * 100.;

	//Observer. = 2°, Illuminant = D65
	float X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
	float Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
	float Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;


	float var_X = X / 95.047;         //ref_X =  95.047   Observer= 2°, Illuminant= D65
	float var_Y = Y / 100.000;          //ref_Y = 100.000
	float var_Z = Z / 108.883;          //ref_Z = 108.883

	if (var_X > 0.008856) var_X = pow(var_X, (1. / 3.));
	else                    var_X = (7.787 * var_X) + (16. / 116.);
	if (var_Y > 0.008856) var_Y = pow(var_Y, (1. / 3.));
	else                    var_Y = (7.787 * var_Y) + (16. / 116.);
	if (var_Z > 0.008856) var_Z = pow(var_Z, (1. / 3.));
	else                    var_Z = (7.787 * var_Z) + (16. / 116.);

	//float l_s = (116. * var_Y) - 16.;
	float a_s = 500. * (var_X - var_Y);
	float b_s = 200. * (var_Y - var_Z);

	var_Y = (l_s + 16.) / 116.;
	var_X = a_s / 500. + var_Y;
	var_Z = var_Y - b_s / 200.;

	if (pow(var_Y, 3) > 0.008856) var_Y = pow(var_Y, 3);
	else                      var_Y = (var_Y - 16. / 116.) / 7.787;
	if (pow(var_X, 3) > 0.008856) var_X = pow(var_X, 3);
	else                      var_X = (var_X - 16. / 116.) / 7.787;
	if (pow(var_Z, 3) > 0.008856) var_Z = pow(var_Z, 3);
	else                      var_Z = (var_Z - 16. / 116.) / 7.787;

	X = 95.047 * var_X;    //ref_X =  95.047     Observer= 2°, Illuminant= D65
	Y = 100.000 * var_Y;   //ref_Y = 100.000
	Z = 108.883 * var_Z;    //ref_Z = 108.883

	var_X = X / 100.;       //X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
	var_Y = Y / 100.;       //Y from 0 to 100.000
	var_Z = Z / 100.;      //Z from 0 to 108.883

	var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
	var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
	var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

	if (var_R > 0.0031308) var_R = 1.055 * pow(var_R, (1 / 2.4)) - 0.055;
	else                     var_R = 12.92 * var_R;
	if (var_G > 0.0031308) var_G = 1.055 * pow(var_G, (1 / 2.4)) - 0.055;
	else                     var_G = 12.92 * var_G;
	if (var_B > 0.0031308) var_B = 1.055 * pow(var_B, (1 / 2.4)) - 0.055;
	else                     var_B = 12.92 * var_B;

	SetRed(var_R * 255.);
	SetGreen(var_G * 255.);
	SetBlue(var_B * 255.);

}

float* CHZColor::GetLab()
{
	if (m_bIsLAB) return &m_arrLab[0];
	
	float r, g, b, X, Y, Z, fx, fy, fz, xr, yr, zr;
	float Ls, as, bs;
	float eps = 216.f/24389.f;
	float k = 24389.f/27.f;
	   
	float Xr = 0.964221f;  // reference white D50
	float Yr = 1.0f;
	float Zr = 0.825211f;
	
	// RGB to XYZ
	r = GetRed()/255.f; //R 0..1
	g = GetGreen()/255.f; //G 0..1
	b = GetBlue()/255.f; //B 0..1
	
	// assuming sRGB (D65)
	if (r <= 0.04045) r = r/12;
	else r = (float) pow((r+0.055)/1.055,2.4);
	
	if (g <= 0.04045) g = g/12;
	else g = (float) pow((g+0.055)/1.055,2.4);
	
	if (b <= 0.04045) b = b/12;
	else b = (float) pow((b+0.055)/1.055,2.4);
	
	X =  0.436052025f*r     + 0.385081593f*g + 0.143087414f *b;
	Y =  0.222491598f*r     + 0.71688606f *g + 0.060621486f *b;
	Z =  0.013929122f*r     + 0.097097002f*g + 0.71418547f  *b;
	
	// XYZ to Lab
	xr = X/Xr;
	yr = Y/Yr;
	zr = Z/Zr;
			
	if ( xr > eps ) fx =  (float) pow(xr, 1/3.f);
	else fx = (float) ((k * xr + 16.) / 116.);
	 
	if ( yr > eps ) fy =  (float) pow(yr, 1/3.f);
	else fy = (float) ((k * yr + 16.) / 116.);
	
	if ( xr > eps ) fz =  (float) pow(zr, 1/3.f);
	else fz = (float) ((k * zr + 16.) / 116);
	
	Ls = (116.0 * fy ) - 16;
	as = 500.0 * (fx - fy);
	bs = 200.0 * (fy - fz);

	float C = sqrtf(as * as + bs * bs);
	float H = atan2f(bs, as);

	as = C * cosf(H);
	bs = C * sinf(H);
	
	m_arrLab[0] = Ls; // (2.55 * Ls + .5);
	m_arrLab[1] = as; // (as + .5) + 128;
	m_arrLab[2] = bs; // (bs + .5) + 128;

	//m_arrLab[0] -= 128.0;
	//m_arrLab[1] -= 128.0;
	//m_arrLab[2] -= 128.0;              

	m_arrLab[0] -= 50.0f;
	m_arrLab[0] /= 50.0f;
	m_arrLab[1] /= 128.0f;
	m_arrLab[2] /= 128.0f;

	m_bIsLAB = true;
	return &m_arrLab[0];
}

bool CHZColor::Brighten(float fVal)
{
	
	SetLabLuminance(GetLabLuminance() + (fVal*100.));

	/*
	ToRGB();
	SetRed(float(GetRed()) + 30.0*(0.448 * fVal));
	SetGreen(float(GetRed()) + 30.0 * (0.184 * fVal));
	SetBlue(float(GetRed()) + 30.0 * (0.092 * fVal));
	*/
	return true;
}

bool CHZColor::SetString(LPCTSTR pcColor) 
{
	ASSERT(pcColor);
	int r, g, b;
	CStringA strColor = CStringA(pcColor);

	if (strColor.GetAt(0)=='#')
	{
		strColor.TrimLeft('#');
		if (sscanf((char*)strColor.GetBuffer(), "%2x%2x%2x", &r, &g, &b) != 3) 
		{
			m_color[c_red] = m_color[c_green] = m_color[c_blue] = 0;
			return false;
		}
		else
		{
			m_color[c_red]	 = static_cast<unsigned char>(r);
			m_color[c_green] = static_cast<unsigned char>(g);
			m_color[c_blue]	= static_cast<unsigned char>(b);
			m_bIsRGB = true;
			m_bIsHLS = false;
			return true;
		}
	}
	else if (strColor.Left(3)==L"rgb")
	{
		strColor.Replace("rgb", "");
		strColor.Remove('a');
		strColor.Remove('(');
		strColor.Remove(')');
		strColor.Remove(';');
		strColor.Remove(' ');
		int index = 0;
		char* val  = strtok((char*)strColor.GetBuffer(), ",");
		while (val != NULL)
		{
			m_color[index] = (unsigned char)atoi(val);
			val  = strtok(NULL, ",");
			index++;
			if (index>2) break;
		}
		m_bIsRGB = true;
		m_bIsHLS = false;
		return true;
	}
	else if (strColor.Left(3)==L"hsl")
	{
		strColor.Replace("hsl", "");
		strColor.Remove('a');
		strColor.Remove('(');
		strColor.Remove(')');
		strColor.Remove(';');
		strColor.Remove(' ');
		int index = 0;
		float h, l, s;
		char* val  = strtok((char*)strColor.GetBuffer(), ",");
		while (val != NULL)
		{
			if (index==0) h = max(360.0f, atof(val));
			else if (index==1) s = max(atof(val),100.0f)/100.0f;
			else if (index==2) l = max(atof(val),100.0f)/100.0f;
			else break;
			val  = strtok(NULL, ",");
			index++;
		}
		SetHLS(h, l, s);
		return true;
	}
	else
	{
		int i = numNamedColors; 
		while (i-- && (strColor!=CStringA(m_namedColor[i].name)));
		if (i < 0) 
		{
			m_color[c_red] = m_color[c_green] = m_color[c_blue] = 0;
			return false;
		}
		else
		{
			m_color[c_red]	 = GetRValue(m_namedColor[i].color);
			m_color[c_green] = GetGValue(m_namedColor[i].color);
			m_color[c_blue]	= GetBValue(m_namedColor[i].color);
			m_bIsRGB = true;
			m_bIsHLS = false;
			return true;
		}
	}
}

CString CHZColor::GetName() const
{
	const_cast<CHZColor*>(this)->ToRGB();
	int i = numNamedColors; 
	while (i-- && m_colorref != m_namedColor[i].color);
	if (i < 0) 
	{
		return L"#" + GetString();
	}
	else return m_namedColor[i].name;
}

CString CHZColor::GetNearestName() const
{
	const_cast<CHZColor*>(this)->ToHLS();

	CString strColor = GetName();

	float fMaxDelta = 1000.0f;

	for (int i=0; i<numNamedColors; i++)
	{
		float fDelta = 0.0;
		
		CHZColor test = CHZColor(m_namedColor[i].color);
		test.ToHLS();

		fDelta += fabs(test.GetHue()-GetHue()) * 0.475;
		fDelta += fabs(test.GetSaturation()-GetSaturation()) * 0.2875;
		fDelta += fabs(test.GetLuminance()-GetLuminance()) * 0.2375;

		if (fDelta<fMaxDelta)
		{
			strColor = test.GetName();
			fMaxDelta = fDelta;
		}
	}

	return strColor;
}

CHZColor CHZColor::GetHeatmapColor(float fValue)
{
	/*
	const int NUM_COLORS = 4;
	//static float color[NUM_COLORS][3] = { {0,0,0}, {1,0,0}, {1,1,0}, {1,1,1} };
	
	static float color[NUM_COLORS][3] = { 
		{252.0/255.0,241.0/255.0,228.0/255.0}, 
		{252.0/255.0,198.0/255.0,147.0/255.0}, 
		{218.0/255.0,73.0/255.0,3.0/255.0}, 
		{125.0/255.0,38.0/255.0,3.0/255.0}
		
	};

	*/

	const int NUM_COLORS = 11;
	static float color[NUM_COLORS][3] = { 
		{0.0/255.0,0.0/255.0,0.0/255.0}, 
		{37.0/255.0,7.0/255.0,5.0/255.0}, 
		{73.0/255.0,14.0/255.0,10.0/255.0}, 
		{105.0/255.0,14.0/255.0,9.0/255.0}, 
		{136.0/255.0,11.0/255.0,5.0/255.0}, 
		{176.0/255.0,42.0/255.0,17.0/255.0}, 
		{216.0/255.0,102.0/255.0,39.0/255.0}, 
		{229.0/255.0,160.0/255.0,43.0/255.0}, 
		{228.0/255.0,212.0/255.0,77.0/255.0}, 
		{235.0/255.0,233.0/255.0,192.0/255.0}, 
		{255.0/255.0,255.0/255.0,255.0/255.0}
	};


	int idx1;
	int idx2;
	float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.
	if(fValue <= 0.0f) idx1 = idx2 = 0;
	else if(fValue >= 1.0f) idx1 = idx2 = NUM_COLORS-1;
	else
	{
		fValue = fValue * (NUM_COLORS-1);
		idx1  = floor(fValue);
		idx2  = idx1+1;
		fractBetween = fValue - float(idx1);
	}

	int r = 255.0f * ((color[idx2][0] - color[idx1][0]) * fractBetween + color[idx1][0]);
	int g = 255.0f * ((color[idx2][1] - color[idx1][1]) * fractBetween + color[idx1][1]);
	int b = 255.0f * ((color[idx2][2] - color[idx1][2]) * fractBetween + color[idx1][2]);

	return CHZColor(RGB(r, g, b));
}


int CHZColor::CompareColor(const void* col1, const void* col2)
{
	CHZColor* c1 = *(CHZColor**)col1;
	CHZColor* c2 = *(CHZColor**)col2;

	if (c1->m_fCompare < c2->m_fCompare) return 1;
	if (c1->m_fCompare > c2->m_fCompare) return -1;
	return 0;
}
