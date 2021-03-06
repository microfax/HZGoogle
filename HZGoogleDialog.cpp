
// HZGoogleDialog.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "HZGoogle.h"
#include "HZGoogleDialog.h"

#include "afxdialogex.h"
#include <fstream>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHZGoogleDialog dialog



CHZGoogleDialog::CHZGoogleDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HZGOOGLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHZGoogleDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHZGoogleDialog, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CHZGoogleDialog message handlers

BOOL CHZGoogleDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//::CreateThread(NULL, 0, CHZGoogleDialog::DoLoad, this, 0, NULL);

	// TODO: Add extra initialization here

	


	m_objFont.CreateFont(24, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, \
		CLIP_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH | FF_DONTCARE, L"AndaleTeletextW99-Regular43");


	CString strCCS = L"r,ccs=UTF-8";
	FILE* fStream = _wfsopen(L"C:\\Temp\\words.txt", strCCS, _SH_DENYNO);
	if (!fStream) return NULL;
	CStdioFile comfile(fStream);  // open the file from this stream
	CString word;
	comfile.ReadString(word);
	while (word != "")
	{
		m_arrWords.Add(word);
		comfile.ReadString(word);
	}
	comfile.Close();

	m_strWord = m_arrWords[m_nWord];
	::CreateThread(NULL, 0, CHZGoogleDialog::DoLoad, this, 0, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHZGoogleDialog::OnPaint()
{
	CPaintDC dc(this);

	try
	{
		CRect rcClient;
		GetClientRect(rcClient);

		CDC memDC;
		CHZBitmap memBitmap;
		memBitmap.CreateImage(rcClient.Width(), rcClient.Height(), false);
		memDC.CreateCompatibleDC(&dc);
		CBitmap* pOldBitmap = (CBitmap*)memDC.SelectObject(&memBitmap);

		CPen penBorder(PS_DOT, 1, RGB(255, 255, 0));

		memDC.SelectObject(&m_objFont);
		memDC.SetTextColor(RGB(0, 0, 0));
		memDC.SetBkMode(TRANSPARENT);

		memDC.FillSolidRect(rcClient, GetSysColor(COLOR_BTNFACE));


		CSize szText = memDC.GetTextExtent(m_strWord);
		memDC.TextOut((rcClient.Width() / 2) - (szText.cx / 2), 10, m_strWord);


		// Number of columns
		int nBorder = 10;
		int nSquare = 150;
		int nX = nBorder, nY = nBorder + 40;
		int nColumns = rcClient.Width() / (nSquare + nBorder);

		for (int i = 0; i < m_arrThumbs.GetCount(); i++)
		{
			CImage imgThumb;
			m_arrThumbs[i]->GetImage(&imgThumb);
			imgThumb.Draw(memDC.GetSafeHdc(), nX, nY);
			if (i == m_nIndex)
			{
				memDC.InvertRect(CRect(CPoint(nX, nY), CSize(imgThumb.GetWidth(), imgThumb.GetHeight())));
				CRect rcHighlight = m_rcCrop;

				rcHighlight.right += nX;
				rcHighlight.left += nX;
				rcHighlight.bottom += nY;
				rcHighlight.top += nY;

				memDC.InvertRect(rcHighlight);
			}
			nX = nX + nSquare + nBorder;
			if ((nX + nSquare + nBorder) > rcClient.Width())
			{
				nX = nBorder;
				nY += nSquare + nBorder;
			}
		}


		memDC.FillSolidRect(0, rcClient.bottom - nBorder * 2 - m_nSize, rcClient.Width(), nBorder * 2 + m_nSize, RGB(128, 128, 128));

		for (int i = 0; i < m_arrImages.GetCount(); i++)
		{
			CImage imgThumb;
			m_arrImages[i]->GetImage(&imgThumb);
			imgThumb.Draw(memDC.GetSafeHdc(), i * (m_nSize + 10) + 10, rcClient.bottom - nBorder - m_nSize);
		}

		memDC.DrawFrameControl(m_rcButton, DFC_BUTTON, DFCS_BUTTONPUSH);
		memDC.DrawText(L"Next", m_rcButton, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

		dc.BitBlt(0, 0, rcClient.right, rcClient.bottom, &memDC, 0, 0, SRCCOPY);
		memDC.SelectObject(pOldBitmap);
		ReleaseDC(&dc);
		ReleaseDC(&memDC);
	}
	catch (...)
	{
	}

}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CHZGoogleDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


DWORD WINAPI CHZGoogleDialog::DoLoad(LPVOID lpvThreadParam)
{

	CHZGoogleDialog* pDialog = (CHZGoogleDialog*)lpvThreadParam;

	/*
	CFileFind finder;
	CString strFoundFile = L"";
	BOOL bWorking = finder.FindFile(L"C:\\temp\\thumbs\\*");
	if (bWorking)
	{
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			if (finder.IsDots()) continue;
			CFile::Remove(finder.GetFilePath());
		}
	}
	*/


	/*
	CString strCCS = L"r,ccs=UTF-8";
	FILE* fStream = _wfsopen(L"C:\\Temp\\words.txt", strCCS, _SH_DENYNO);
	if (!fStream) return NULL;
	CStdioFile comfile(fStream);  // open the file from this stream
	*/

	pDialog->BeginWaitCursor();

	

	CString strHeaders = _T("User-Agent: User-Agent=Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\nAccept-Language: es-MX,es;q=0.9,en-US;q=0.8,en;q=0.7\r\nConnection: keep-alive\r\nContent-Type: application/x-www-form-urlencoded");
	CInternetSession session;
	CHttpConnection* pServer = NULL;
	CHttpFile* pFile = NULL;

	try
	{


		CString strServerName;
		CString strObject;
		INTERNET_PORT nPort;
		DWORD dwServiceType;


		CString word;
		//comfile.ReadString(word);
		CString redirect = L"";
		CStringA pagea;

		word = pDialog->m_strWord;

		pagea = "";
		CString strURL = L"http://www.google.com/search?source=lnms&tbm=isch&sa=X&q=" + word;
		AfxParseURL(strURL, dwServiceType, strServerName, strObject, nPort);
		pServer = session.GetHttpConnection(strServerName, nPort);
		pFile = pServer->OpenRequest(CHttpConnection::HTTP_VERB_GET,
			strObject, NULL, 1, NULL, NULL, INTERNET_FLAG_EXISTING_CONNECT);
		pFile->AddRequestHeaders(strHeaders);
		pFile->SendRequest(strHeaders);
		DWORD dwRet;
		pFile->QueryInfoStatusCode(dwRet);


		int count;

		const int BUFSIZE = 1024;
		char buffer[BUFSIZE];
		if (pFile)
		{
			while (count = pFile->Read(buffer, BUFSIZE - 1))
			{
				buffer[count] = '\0';
				pagea += buffer;
			}
		}

		int nIndex = pagea.Find(">_setImgSrc(");
		//nIndex = pagea.Find(">_setImgSrc(", nIndex);
		//nIndex = pagea.Find(">_setImgSrc(", nIndex);
		
		int nItem = 1;
		CString strFilename = L"";
		CStringA strExtension = "";
		CStringA strBase64 = "";
		while (nIndex != -1)
		{
			nIndex = pagea.Find("\\/", nIndex) + 2;
			int nNext = pagea.Find(";", nIndex) - nIndex;
			if (nNext > 4) continue;
			strExtension = pagea.Mid(nIndex, nNext);
			strFilename = "C:\\temp\\thumbs\\%i." + strExtension;
			strFilename.Format(strFilename, nItem);
			nIndex = pagea.Find("base64,", nIndex) + 7;
			strBase64 = pagea.Mid(nIndex, pagea.Find("'", nIndex) - nIndex);

			int byteLength = 0;// Base64DecodeGetRequiredLength(sizeof(SYSTEM_INFO));
			CByteArray arrBytes;
			Base64Decode(strBase64, strBase64.GetLength(), arrBytes.GetData(), &byteLength);
			if (byteLength > 0)
			{
				arrBytes.SetSize(byteLength);
				Base64Decode(strBase64, strBase64.GetLength(), arrBytes.GetData(), &byteLength);
				
				//ofstream outfile(strFilename, ios::out | ios::binary);
				//outfile.write((const char*)&arrBytes[0], arrBytes.GetCount());
				//outfile.close();
				
				if (nItem == 1) pDialog->ClearThumbs();
				CHZBitmap* pBitmap = new CHZBitmap();
				//pBitmap->LoadImage(strFilename);
				pBitmap->LoadImage(arrBytes.GetData(), arrBytes.GetCount());
				if (pBitmap->GetHeight() == 0)
				{
					delete pBitmap;
					continue;
				}
				CHZBitmap* pThumb = new CHZBitmap(RGB(0,0,0));
				pThumb->CopyImage(pBitmap);

				int nSquare = 150;

				int nWidth = pBitmap->GetWidth();
				int nHeight = pBitmap->GetHeight();
				if (nWidth > nHeight) pThumb->ResizeImage(nSquare, float(nHeight) * (float(nSquare) / float(nWidth)), 7);
				else pThumb->ResizeImage(float(nWidth) * (float(nSquare) / float(nHeight)), nSquare, 7);

				pDialog->m_arrThumbs.Add(pThumb);
				pDialog->Invalidate();
				pDialog->m_arrPics.Add(pBitmap);

				/*
				CFile file;
				file.Open(strFilename, CFile::modeCreate | CFile::modeWrite, NULL);
				CT2CA strOutputA(strBase64, CP_UTF8);
				file.Write(strOutputA, ::strlen(strOutputA));
				file.Close();
				*/
			}

			nItem++;
			nIndex = pagea.Find(">_setImgSrc(", nIndex);
		}



		/*
		CString page;
		DWORD dwNum = MultiByteToWideChar(ANSI_CHARSET, 0, pagea, -1, NULL, 0);
		MultiByteToWideChar(ANSI_CHARSET, 0, pagea, -1, page.GetBuffer(dwNum + 2), dwNum);
		page.ReleaseBuffer();
		*/

		/*
		int nStart = page.Find(L"<span id=\"ch_lblModel");
		nStart = page.Find(L"<a", nStart);
		nStart = page.Find(L">", nStart) + 1;

		//CString emails = L"";
		//emails = page.Mid(nStart, page.Find(L"<", nStart) - nStart);
		*/

		pFile->Close();
		pServer->Close();




		/*
		CFile file;
		file.Open(L"C:\\Temp\\page.txt", CFile::modeCreate | CFile::modeWrite, NULL);
		file.Write(pagea, ::strlen(pagea));
		file.Close();
		*/

		delete pFile;
		delete pServer;

		//comfile.Close();

	}
	catch (CException* e)
	{
		TCHAR   szCause[255];
		CString strFormatted;

		e->GetErrorMessage(szCause, 255);

		strFormatted = _T("Error: ");
		strFormatted += szCause;

		AfxMessageBox(strFormatted);

		pFile->Close();
		pServer->Close();
		delete pFile;
		delete pServer;

		pDialog->EndWaitCursor();

		return 0;
	}

	pDialog->EndWaitCursor();
}

void CHZGoogleDialog::ClearThumbs()
{
	SetRedraw(FALSE);
	for (int i = 0; i < m_arrThumbs.GetCount(); i++)
	{
		CHZBitmap* pBitmap = m_arrThumbs[i];
		if (pBitmap)
		{
			pBitmap->DeleteObject();
			delete pBitmap;
			pBitmap = NULL;
		}
	}
	m_arrThumbs.RemoveAll();

	for (int i = 0; i < m_arrPics.GetCount(); i++)
	{
		CHZBitmap* pBitmap = m_arrPics[i];
		if (pBitmap)
		{
			pBitmap->DeleteObject();
			delete pBitmap;
			pBitmap = NULL;
		}
	}
	m_arrPics.RemoveAll();

	CString strFilename;
	for (int i = 0; i < m_arrImages.GetCount(); i++)
	{
		strFilename.Format(L"C:\\temp\\thumbs\\" + m_arrWords[m_nWord - 1] + L" %02d.png", i);
		CHZBitmap* pBitmap = m_arrImages[i];
		if (pBitmap)
		{
			pBitmap->SaveImage(strFilename);
			pBitmap->DeleteObject();
			delete pBitmap;
			pBitmap = NULL;
		}
	}
	m_arrImages.RemoveAll();
	SetRedraw(TRUE);
	m_nWord++;
}

void CHZGoogleDialog::OnDestroy()
{
	CDialogEx::OnDestroy();

	ClearThumbs();
}


void CHZGoogleDialog::OnSize(UINT nType, int cx, int cy)
{
	//CDialogEx::OnSize(nType, cx, cy);

	CRect rcClient;
	GetClientRect(&rcClient);

	int nBorder = 10;
	m_rcButton = CRect(CPoint(rcClient.right - 100, rcClient.bottom - nBorder * 2 - m_nSize - 50), CSize(90, 40));

	Invalidate();
}


BOOL CHZGoogleDialog::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
	//return CDialogEx::OnEraseBkgnd(pDC);
}


void CHZGoogleDialog::OnMouseMove(UINT nFlags, CPoint point)
{

	SetFocus();

	CRect rcClient;
	GetClientRect(&rcClient);

	int nBorder = 10;
	int nSquare = 150;
	int nGrid = nBorder + nSquare;
	int nX = nBorder, nY = nBorder + 40;
	int nColumns = rcClient.Width() / nGrid;
	int nIndex = -1;
	int nWidth, nHeight, nEdge;

	if (point.y > 40 || point.x > nBorder)
	{
		CRect rcFocus;
		for (int i = 0; i < m_arrThumbs.GetCount(); i++)
		{
			nWidth = m_arrThumbs[i]->GetWidth();
			nHeight = m_arrThumbs[i]->GetHeight();
			int nEdge = min(nWidth, nHeight);

			int nTop = (i / nColumns) * nGrid + 40 + nBorder;
			int nLeft = (i % nColumns) * nGrid + nBorder;
			rcFocus = CRect(CPoint(nLeft, nTop), CSize(nWidth, nHeight));

			if (rcFocus.PtInRect(point))
			{
				nIndex = i;
				CRect rcCrop = CRect(CPoint(point.x - nEdge / 2, point.y - nEdge / 2), CSize(nEdge, nEdge));
				if (rcCrop.left < nLeft) rcCrop.MoveToX(nLeft);
				if (rcCrop.right > (nLeft + nWidth)) rcCrop.MoveToX(nLeft + nWidth - nEdge);
				if (rcCrop.top < nTop) rcCrop.MoveToY(nTop);
				if (rcCrop.bottom > (nTop + nHeight)) rcCrop.MoveToY(nTop + nHeight - nEdge);
				rcCrop.left -= nLeft;
				rcCrop.right -= nLeft;
				rcCrop.top -= nTop;
				rcCrop.bottom -= nTop;
				m_rcCrop = rcCrop;
				break;
			}
		}
	}
	//if (m_nIndex != nIndex)
	{
		m_nIndex = nIndex;
		Invalidate();
	}

	CDialogEx::OnMouseMove(nFlags, point);
}


void CHZGoogleDialog::OnLButtonDown(UINT nFlags, CPoint point)
{

	CRect rcClient;
	GetClientRect(&rcClient);

	if (m_rcButton.PtInRect(point))
	{
		m_strWord = m_arrWords[m_nWord];
		::CreateThread(NULL, 0, CHZGoogleDialog::DoLoad, this, 0, NULL);
	}
	else if (point.y > (rcClient.Height() - (m_nSize + 20)))
	{
		int nIndex = point.x / (m_nSize + 10);
		if (nIndex < m_arrImages.GetCount())
		{
			CHZBitmap* pBitmap = m_arrImages[nIndex];
			if (pBitmap)
			{
				pBitmap->DeleteObject();
				delete pBitmap;
				pBitmap = NULL;

				for (int i = nIndex; i < (m_arrImages.GetCount() - 1); i++) m_arrImages[i] = m_arrImages[i + 1];
				m_arrImages.SetSize(m_arrImages.GetCount() - 1);
				Invalidate();
			}
		}
	}
	else if (m_nIndex != -1)
	{
		CHZBitmap* pBitmap = new CHZBitmap();
		pBitmap->CopyImage(m_arrPics[m_nIndex]);
		int nEdge = min(pBitmap->GetWidth(), pBitmap->GetHeight());

		pBitmap->Design();
		pBitmap->PadImage(nEdge, nEdge,
			float(m_rcCrop.left) / float(m_arrThumbs[m_nIndex]->GetWidth()),
			float(m_rcCrop.top) / float(m_arrThumbs[m_nIndex]->GetHeight()));
		pBitmap->ResizeImage(m_nSize, m_nSize, 7);
		pBitmap->Render();

		m_arrImages.Add(pBitmap);
		Invalidate();
	}


	CDialogEx::OnLButtonDown(nFlags, point);
}
