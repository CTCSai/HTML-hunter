/*
# Copyright (C) 2017 Chris Pro
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma warning(disable : 4996)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <wininet.h>
#include <exdisp.h> //IWebBrowser2
#include <mshtml.h> //IHTMLDocument2
#include "xgetopt.h"
#include <Shlwapi.h>
#pragma comment (lib, "wininet.lib")
#pragma comment (lib, "Shlwapi.lib")

#define BUFSIZE 4096

int g_count = 0;

typedef enum FNTYPE {
	FN_BITMAP,
	FN_CACHE,
	FN_DOM
} FNTYPE;

LPWSTR GetFnFromUrl(BSTR burl, FNTYPE fn)
{
	URL_COMPONENTSW url;
	WCHAR szHostname[MAX_PATH];
	WCHAR szPath[MAX_PATH];

	LPWSTR szFile = new WCHAR[MAX_PATH * 2];

	memset(&url, 0, sizeof(url));
	memset(szHostname, 0, sizeof(szHostname));
	memset(szPath, 0, sizeof(szPath));

	url.dwStructSize = sizeof(url);
	url.dwHostNameLength = MAX_PATH;
	url.lpszHostName = szHostname;
	url.dwUrlPathLength = MAX_PATH;
	url.lpszUrlPath = szPath;

	InternetCrackUrlW((LPWSTR)burl, 0, 0, &url);

	if (wcschr(szPath, L'?') != NULL) {
		*(wcschr(szPath, L'?')) = L'\x00';
	}
	if (szPath[0] = L'/') {
		szPath[0] = '\x00';
	}

	switch (fn)
	{
	case FN_BITMAP:
		wsprintf(szFile, L"%ws_%ws.bmp", szHostname, PathFindFileNameW(szPath));
		break;
	case FN_CACHE:
		wsprintf(szFile, L"%ws_%ws_cache.txt", szHostname, PathFindFileNameW(szPath));
		break;
	case FN_DOM:
		wsprintf(szFile, L"%ws_%ws_dom.txt", szHostname, PathFindFileNameW(szPath));
		break;
	default:
		break;
	};

	return szFile;
}


int CaptureAnImage(HWND hWnd, BSTR bloc)
{

done:

	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcScreen = GetDC(NULL);
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC)
	{
		//MessageBox(hWnd, L"StretchBlt has failed",L"Failed", MB_OK);
		goto done;
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	//This is the best stretch mode
	SetStretchBltMode(hdcWindow, HALFTONE);

	//The source DC is the entire screen and the destination DC is the current window (HWND)
	if (!StretchBlt(hdcWindow,
		0, 0,
		rcClient.right, rcClient.bottom,
		hdcScreen,
		0, 0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SRCCOPY))
	{
		//MessageBox(hWnd, L"StretchBlt has failed",L"Failed", MB_OK);
		goto done;
	}

	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmScreen)
	{
		//MessageBox(hWnd, L"CreateCompatibleBitmap Failed",L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0,
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		hdcWindow,
		0, 0,
		SRCCOPY))
	{
		//MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
		goto done;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char *lpbitmap = (char *)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(hdcWindow, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	LPWSTR szFile = GetFnFromUrl(bloc, FN_BITMAP);
	printf("  Saving BMP to %ws\n", szFile);

	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFileW(szFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	delete[] szFile;

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	//Close the handle for the file that was created
	CloseHandle(hFile);

	//Clean up

	DeleteObject(hbmScreen);
	ReleaseDC(hWnd, hdcMemDC);
	ReleaseDC(NULL, hdcScreen);
	ReleaseDC(hWnd, hdcWindow);
	return 0;
}

LPWSTR CheckCacheEntry(LPTSTR szURL)
{
	DWORD cbEntryInfo;
	DWORD MAX_ENTRY_SIZE = 65536;
	LPINTERNET_CACHE_ENTRY_INFO lpCacheEntry;

	cbEntryInfo = MAX_ENTRY_SIZE;
	lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[cbEntryInfo];
	lpCacheEntry->dwStructSize = cbEntryInfo;

	while (true) {
		if (!GetUrlCacheEntryInfo(szURL, lpCacheEntry, &cbEntryInfo)) {
			delete[] lpCacheEntry;
			switch (GetLastError())
			{
			case ERROR_INSUFFICIENT_BUFFER:
				lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[cbEntryInfo];
				lpCacheEntry->dwStructSize = cbEntryInfo;
				continue;
			case ERROR_FILE_NOT_FOUND:
			default:
				return NULL;
			};
		}
		break;
	}

	LPWSTR _wscdup = wcsdup(lpCacheEntry->lpszLocalFileName);
	delete[] lpCacheEntry;
	return _wscdup;
}

void DumpToFile(LPWSTR w_str, BSTR bloc)
{
	char * a_str = NULL;
	DWORD len;
	HANDLE hFile;

	LPWSTR szFile = GetFnFromUrl(bloc, FN_DOM);

	len = WideCharToMultiByte(CP_ACP, 0, w_str, -1, NULL, 0, NULL, NULL);
	a_str = new char[len];
	if (a_str) {
		len = WideCharToMultiByte(CP_ACP, 0, w_str, -1, a_str, len, NULL, NULL);
		hFile = CreateFileW(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			WriteFile(hFile, a_str, len, &len, NULL);
			printf("[INFO] Dumped %d bytes of page content to %ws\n", len, szFile);
			CloseHandle(hFile);
		}
		delete[] a_str;
	}
}

void DoWork(IHTMLDocument2 *document, BSTR bloc)
{

	IHTMLElement *body;

	HRESULT      hr;
	BSTR         as_string;
	LPWSTR       szCached;
	BSTR      as_value;
	LPWSTR szFile = GetFnFromUrl(bloc, FN_CACHE);

	//Get the DOM body 
	hr = document->get_body(&body);
	if (hr != S_OK || !body)
		return;

	//Get the Cookies!
	hr = document->get_cookie(&as_value);
	if (hr != S_OK || !body)
		return;


	//Get the HTML as a string 
	hr = body->get_innerHTML(&as_string);
	if (hr != S_OK || !as_string) {
		body->Release();
		return;
	}

	// we can't perform a straight comparison with the DOM and cache file
	// because IE normalizes the HTML (i.e. it capitalizes tags, removes/inserts
	// line breaks, and re-orders tag names/values in various ways) 
	DumpToFile(as_string, bloc);
	SysFreeString(as_string);

	// try to find the cached file. in some cases, like if the server requests
	// no caching, then it won't be available. no big deal, we'll still dump
	// the DOM which contains the real necessary data
	szCached = CheckCacheEntry((LPWSTR)bloc);
	if (szCached) {
		printf("[INFO] Cache file: %ws\n", szCached);
		printf("[INFO] Copied to: %ws\n", szFile);
		CopyFileW(szCached, szFile, FALSE);
		delete[] szCached;
	}
	else {
		printf("[ERROR] File not in cache.\n");
	}

	return;
}

void Navigate(IWebBrowser2 *browser, BSTR burl, bool save_screen)
{
	VARIANT      vEmpty;
	HRESULT      hr;
	BSTR		 bloc;
	VARIANT_BOOL fBusy;
	IDispatch    *html;
	IHTMLDocument2 *document;
	SHANDLE_PTR		 hWnd; //SHANDLE_PTR instead of long 
	int          nIndex;


	VariantInit(&vEmpty);
	hr = browser->Navigate(burl, &vEmpty, &vEmpty, &vEmpty, &vEmpty);

	if (!SUCCEEDED(hr)) {
		return;
	}

	browser->put_Visible(VARIANT_TRUE);
	fBusy = VARIANT_TRUE;

	// wait for the page to download completely
	while (fBusy) {
		Sleep(1000);
		browser->get_Busy(&fBusy);
	}

	// we may have been redirected to a new URL
	hr = browser->get_LocationURL(&bloc);
	if (hr != S_OK || !bloc) {
		return;
	}

	// wait a moment for any trojans to do their injection 
	printf("[INFO] Redirect URL: %ws\n", bloc);
	printf("[INFO] Navigate completed. Waiting 3 seconds.\n");
	Sleep(3000);
	
	browser->get_HWND(&hWnd);
	if (save_screen) {
		CaptureAnImage((HWND)hWnd, bloc);
	}

	hr = browser->get_Document((IDispatch**)&html);

	if (hr == S_OK && html) {
		hr = html->QueryInterface(IID_IHTMLDocument2, (void**)&document);
		if (hr == S_OK && document) {
			DoWork(document, bloc);
			document->Release();
		}
		html->Release();
	}

	SysFreeString(bloc);
}

void NewBrowser(WCHAR * lpwsURL, bool save_screen)
{
	CLSID        clsid;
	HRESULT      hr;
	IUnknown     *ppv = NULL;
	IWebBrowser2 *browser = NULL;
	BSTR         bstrURL = NULL;

	_tprintf(_T("[INFO] Requested URL: %s\n"), lpwsURL);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	hr = CLSIDFromProgID((LPCOLESTR)L"InternetExplorer.Application", &clsid);

	if (hr != S_OK) {
		printf("[ERROR] CLSIDFromProgID failed: 0x%x!\n", hr);
		return;
	}

	hr = CoCreateInstance((const IID&)clsid, NULL,
		CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
		IID_IUnknown, (void **)&ppv);

	if (hr != S_OK) {
		printf("[ERROR] CoCreateInstance failed: 0x%x!\n", hr);
		CoUninitialize();
		return;
	}

	hr = ppv->QueryInterface(IID_IWebBrowser2, (void **)&browser);

	if (hr == S_OK && browser) {
		bstrURL = SysAllocString((OLECHAR*)lpwsURL);
		if (bstrURL) {
			Navigate(browser, bstrURL, save_screen);
			SysFreeString(bstrURL);
		}
		browser->Stop();
		browser->Quit();
		browser->Release();
	}

	ppv->Release();
	CoUninitialize();
	return;
}

void help(LPTSTR szName)
{
	_tprintf(_T("\nUsage: %s [OPTIONS]\n")
		_T("OPTIONS:\n")
		_T("  -h           Show help message and exit\n")
		_T("  -f <FILE>    Submit the text file with URLs to check\n")
		_T("  -s           Save ice-screen shots (default>no)\n")
		_T("\n"),
		szName);
}

int _tmain(int argc, TCHAR *argv[])
{
	bool save_screen = false;
	const TCHAR *szFile = _T("");
	FILE * f = NULL;


	CHAR  url[BUFSIZE];
	WCHAR wurl[BUFSIZE];
	int c;
	

	printf("\n-------------------------------------\n");
	printf(" Cybertrigo CLI-toolkit \n");
	printf(" HTML hunter v1.0.0            \n");
	printf(" Open Source Software Donation version: 2017 \n");
	printf("---------------------------------------\n");
/*


while ((c = getopt(argc, argv, _T("hf:s"))) != EOF)
{
switch (c)
{
case _T('s'):
save_screen = true;
break;
case _T('f'):
szFile = optarg;
break;
default:
help(argv[0]);
return -1;
};
}

*/
	

	_tfopen_s(&f, szFile, _T("r"));
	if (f == NULL) {
		help(argv[0]);
		printf("[ERROR] You must supply a file with URLs!\n");
		return -1;
	}

	memset(url, 0, BUFSIZE);

	while (fgets(url, BUFSIZE, f) != NULL)
	{
		if (strchr(url, '\n') != NULL)
			*(strchr(url, '\n')) = '\x00';
		if (strchr(url, '\r') != NULL)
			*(strchr(url, '\r')) = '\x00';
		if (strchr(url, '\r') != NULL)
			*(strchr(url, '\r')) = '\x000';
		if (strchr(url, '\r') != NULL)
			*(strchr(url, '\r')) = '\x0000';

		memset(wurl, 0, BUFSIZE * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, url, -1, wurl, BUFSIZE);
		NewBrowser(wurl, save_screen);
		puts("");
	}

	fclose(f);
	return 0;
}