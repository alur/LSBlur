#include "lsblur.h"

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// DllMain
//
// DLL entry point
//
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID /* lpReserved */)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls((HINSTANCE)hModule);
	}
	return TRUE;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// initModuleEx
//
// Module starting point
//
int initModuleEx(HWND /* hwndParent */, HINSTANCE hDllInstance, LPCSTR /* szPath */)
{
	g_hInstance = hDllInstance;

	// Store wallpaper in memory instead of generating a new copy whenever a !Blur line is executed
	g_bStoreWallpaper = (GetRCBoolDef("BlurStoreWallpaper", FALSE) != FALSE);

	// Get the parent window 
	g_hwndDesktop = FindWindow("DesktopBackgroundClass", NULL);
	if (!g_hwndDesktop)
	{
		return 1;
	}

	// Create window classes and the main window
	if (!CreateMessageHandlers(hDllInstance))
	{
		return 1;
	}

	// Store the wallpaper
	if (g_bStoreWallpaper)
		g_pWallpaper = GetWallpaper();

	// Load *Blur lines
	ReadConfig();

	// Register for !Blur bangs
	AddBangCommand("!Blur", BangBlur);
	AddBangCommand("!RemoveBlur", BangRemoveBlur);

	return 0; // Initialized succesfully
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// BangBlur
//
void BangBlur(HWND, LPCSTR pszArgs)
{
	CBitmapEx* bmpWallpaper = g_bStoreWallpaper ? g_pWallpaper : GetWallpaper();
	ParseBlurLine(pszArgs, bmpWallpaper);
	if (!g_pWallpaper)
		delete bmpWallpaper;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// BangRemoveBlur
//
void BangRemoveBlur(HWND, LPCSTR pszArgs)
{
	BlurMap::iterator iter = g_BlurMap.find(pszArgs);
	if (iter != g_BlurMap.end())
	{
		delete iter->second;
		g_BlurMap.erase(iter);
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// quitModule
//
// Called on module unload
//
void quitModule(HINSTANCE hDllInstance)
{
	// Unregister the !Blur bang
	RemoveBangCommand("!Blur");
	RemoveBangCommand("!RemoveBlur");

	// Delete all blur areas
	for (BlurMap::iterator iter = g_BlurMap.begin(); iter != g_BlurMap.end(); ++iter)
	{
		delete iter->second;
	}

	// Erase the BlurMap
	g_BlurMap.clear();

	if (g_hwndMessageHandler)
	{
		// Unregister for LiteStep messages
		SendMessage(GetLitestepWnd(), LM_UNREGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);

		// Destroy message handling windows
		DestroyWindow(g_hwndMessageHandler);

		g_hwndMessageHandler = NULL;
	}

	// Clear the wallpaper if necesary
	if (g_pWallpaper)
		delete g_pWallpaper;

	// Unregister window classes
	UnregisterClass(g_szBlurHandler, hDllInstance);
	UnregisterClass(g_szMsgHandler, hDllInstance);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// CreateMessageHandlers
//
// Registers window classes and creates the main message handler window
//
bool CreateMessageHandlers(HINSTANCE hInst)
{
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_NOCLOSE;
	wc.lpfnWndProc = MessageHandlerProc;
	wc.hInstance = hInst;
	wc.lpszClassName = g_szMsgHandler;
	wc.hIconSm = 0;

	if (!RegisterClassEx(&wc))
		return false; // Failed to register message handler window class

	wc.lpfnWndProc = BlurHandlerProc;
	wc.lpszClassName = g_szBlurHandler;

	if (!RegisterClassEx(&wc))
		return false; // Failed to register window class

	g_hwndMessageHandler = CreateWindowEx(WS_EX_TOOLWINDOW, g_szMsgHandler, 0, WS_POPUP, 0, 0, 0, 0, 0, 0, hInst, 0);

	if (!g_hwndMessageHandler)
		return false; // Failed to create message handler window

	// Register with LiteStep to receive LM_ messages
	SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);

	return true;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// GetWallpaper - Returns a reconstruction of the current wallpaper.
//
CBitmapEx* GetWallpaper()
{
	// create a DC for the screen and create
	// a compatible memory DC to screen DC
	// the return value of CreateDC() is the handle to a DC for the specified device
	HDC hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	HDC hMemDC = CreateCompatibleDC(hScrDC);

	// get screen resolution for bitmap's width and height
	int nWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int nHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);

	// create a bitmap compatible with the screen DC
	HBITMAP hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

	// select new bitmap into memory DC as a drawing surface
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	// create the clipping region in the memory DC
	HRGN hRegion = CreateRectRgn(x, y, nWidth + x, nHeight + y);
	SelectClipRgn(hScrDC, hRegion);

	// paint the desktop pattern to the memory DC
	PaintDesktop(hScrDC);

	// blitting desktop pattern to the memory DC
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, x, y, SRCCOPY);

	// update all the windows
	InvalidateRect(NULL, NULL, TRUE);

	// select old bitmap back into memory DC (restore settings) and get handle to
	// bitmap of the wallpaper
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

	// clean up before exit the function
	DeleteDC(hScrDC);
	DeleteDC(hMemDC);

	// Load the HBITMAP into the CBitmapEx class :)
	CBitmapEx* bmpWallpaper = new CBitmapEx();
	bmpWallpaper->Load(hBitmap);

	return bmpWallpaper;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// ReadConfig
//
void ReadConfig()
{
	LPVOID f = LCOpen(NULL);
	if (!f)
	{
		throw std::runtime_error("Error: Cannot open step.rc for reading");
	}

	char szLine[MAX_LINE_LENGTH], szBlur[MAX_BANGCOMMAND];
	LPCSTR pszLine;

	CBitmapEx* bmpWallpaper = g_bStoreWallpaper ? g_pWallpaper : GetWallpaper();

	while (LCReadNextConfig(f, "*Blur", szLine, sizeof(szLine)))
	{
		GetToken(szLine, szBlur, &pszLine, false); // Drop the *Blur
		ParseBlurLine(pszLine, bmpWallpaper);
	}
	
	if (!g_bStoreWallpaper)
		delete bmpWallpaper;

	LCClose(f);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// ParseBlurLine
//
// Returns true on success
//
bool ParseBlurLine(LPCSTR szLine, CBitmapEx* bmpWallpaper)
{
	char szName[MAX_LINE_LENGTH], szX[MAX_LINE_LENGTH], szY[MAX_LINE_LENGTH];
	char szHeight[MAX_LINE_LENGTH], szWidth[MAX_LINE_LENGTH], szIterations[MAX_LINE_LENGTH];
	char* szTokens[6] = {szName, szX, szY, szWidth, szHeight, szIterations};

	int X, Y, Width, Height, Itterations;
	bool bReturn = false;

	// Necesary to check if the iterations were omitted.
	szIterations[0] = 0;

	if (szLine != NULL)
	{
		// *Blur/!Blur Name X Y Width Height Repetitions
		if (LCTokenize(szLine, szTokens, 6, NULL) > 4)
		{
			X = atoi(szX);
			Y = atoi(szY);
			Width = atoi(szWidth);
			Height = atoi(szHeight);
			Itterations = atoi(szIterations);
			
			// Look for the name in the BlurMap
			BlurMap::iterator iter = g_BlurMap.find(szName);
			if (iter != g_BlurMap.end())
			{
				// The BlurArea already exists, so just update it
				iter->second->Move(X, Y);
				iter->second->Resize(Width, Height);
				iter->second->SetItterations(Itterations);
				iter->second->UpdateBackground(bmpWallpaper);
			}
			else
			{
				// Create the blurarea and insert it into the BlurMap
				BlurArea* blur = new BlurArea(X, Y, Width, Height, bmpWallpaper, szName, Itterations);
				g_BlurMap.insert(BlurMap::value_type(szName, blur));
			}
			
			bReturn = true; // Everything went well
		}
	}

	return bReturn;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//	Message Handler
//
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case LM_REFRESH:
		{
			ReadConfig();
			return 0;
		}
		case LM_GETREVID:
		{
			size_t uLength;
			StringCchPrintf((char*)lParam, 64, "%s: %s", g_szAppName, g_rcsRevision);
			if (SUCCEEDED(StringCchLength((char*)lParam, 64, &uLength)))
				return uLength;
			lParam = NULL;
			return 0;
		}
		case WM_SETTINGCHANGE:
		{
			if (wParam == SPI_SETDESKWALLPAPER)
			{
				CBitmapEx* bmpWallpaper = g_bStoreWallpaper ? g_pWallpaper : GetWallpaper();
				for (BlurMap::iterator iter = g_BlurMap.begin(); iter != g_BlurMap.end(); ++iter)
				{
					iter->second->UpdateBackground(bmpWallpaper);
				}
				if (!g_bStoreWallpaper)
					delete bmpWallpaper;
				return 0;
			}
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//	Blur Handler
//
LRESULT WINAPI BlurHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BlurArea* pBlur = (BlurArea*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!pBlur) // Not sure why this would happen, but no need to bother with it.
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			// TODO::Forward these
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
			pBlur->Draw(hDC);
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *c = (WINDOWPOS*)lParam;
			c->hwnd = hWnd;
			c->hwndInsertAfter = HWND_BOTTOM;
			//c->flags = c->flags & ~SWP_NOZORDER;
			//c->flags |= SWP_NOACTIVATE | SWP_NOSENDCHANGING;
			c->flags |= SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOMOVE;
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}