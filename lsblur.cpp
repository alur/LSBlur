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

	// Start GDI+
	Gdiplus::GdiplusStartupInput gdistartup;
	gdistartup.GdiplusVersion = 1;
	gdistartup.DebugEventCallback = NULL;
	gdistartup.SuppressBackgroundThread = FALSE;
	gdistartup.SuppressExternalCodecs = FALSE;
	Gdiplus::GdiplusStartup(&gdiToken, &gdistartup, NULL);

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

	// Shutdown GDI+
	Gdiplus::GdiplusShutdown(gdiToken);

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
// UpdateMonitorInfo
//
// Updates the information about all monitors
//
void UpdateMonitorInfo()
{
	g_Monitors.clear();
	
	MonitorInfo mInfo;
	mInfo.Left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	mInfo.Top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	mInfo.ResX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	mInfo.ResY = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	g_Monitors.push_back(mInfo);

	EnumDisplayMonitors(NULL, NULL, SetMonitorVars, 0);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// SetMonitorVars
//
// Callback function for UpdateMonitorInfo
//
BOOL CALLBACK SetMonitorVars(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
{
	MonitorInfo mInfo;
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &mi);

	mInfo.Top = mi.rcMonitor.top;
	mInfo.Left = mi.rcMonitor.left;
	mInfo.ResY = mi.rcMonitor.bottom - mi.rcMonitor.top;
	mInfo.ResX = mi.rcMonitor.right - mi.rcMonitor.left;

	if ((mi.dwFlags & MONITORINFOF_PRIMARY) == MONITORINFOF_PRIMARY)
	{
		g_Monitors.insert(g_Monitors.begin()+1, mInfo);
	}
	else
	{
		g_Monitors.push_back(mInfo);
	}

	return TRUE;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// GetWallpaper - Returns a reconstruction of the current wallpaper.
//
CBitmapEx* GetWallpaper()
{
	UpdateMonitorInfo(); // Update our information about the monitors

	// Get the path to the wallpaper
	char szWallpaperPath[MAX_LINE_LENGTH];
	DWORD dwSize = sizeof(szWallpaperPath), dwType = REG_SZ;
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "Wallpaper", &dwType, &szWallpaperPath, &dwSize);

	// Get whether or not to tile the wallpaper
	char szTemp[32];
	dwSize = sizeof(szTemp);
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "TileWallpaper", &dwType, &szTemp, &dwSize);
	bool bTileWallpaper = atoi(szTemp) ? true : false;

	// Get whether or not to stretch the wallpaper
	dwSize = sizeof(szTemp);
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "WallpaperStyle", &dwType, &szTemp, &dwSize);
	int iWallpaperStyle = atoi(szTemp);

	// Create a HBITMAP the size of the virtual screen
	HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
	HDC hdcDesktop = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, g_Monitors.at(0).ResX, g_Monitors.at(0).ResY);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcDesktop, hBitmap);

	// Fill the bitmap with the background color
	HRGN hRegion = CreateRectRgn(0, 0, g_Monitors.at(0).ResX, g_Monitors.at(0).ResY);
    SelectClipRgn(hdcScreen, hRegion);
	FillRgn(hdcDesktop, hRegion, GetSysColorBrush(COLOR_DESKTOP));

	// Use GDI+ to parse the file
	WCHAR wszWallpaperPath[MAX_LINE_LENGTH];
	MultiByteToWideChar (CP_ACP, 0, szWallpaperPath, -1, wszWallpaperPath, MAX_LINE_LENGTH);
	Gdiplus::Bitmap* bm = Gdiplus::Bitmap::FromFile(wszWallpaperPath);

	if (bm)
	{
		// Convert the GDI+ format to a HBITMAP
		HBITMAP hbmWallpaper;
		Gdiplus::Color clr(0xFF,0xFF,0xFF); 
		bm->GetHBITMAP(clr, &hbmWallpaper);

		if(hbmWallpaper)
		{
			BITMAP bm2;
			GetObject(hbmWallpaper, sizeof(BITMAP), &bm2);

			int cxWallpaper = bm2.bmWidth;
			int cyWallpaper = bm2.bmHeight;

			DeleteObject(&bm2);

			HDC hdcWallpaper = CreateCompatibleDC(hdcScreen);
			HBITMAP hbmOldWallpaper = (HBITMAP) SelectObject(hdcWallpaper, hbmWallpaper);
			
			SetStretchBltMode(hdcWallpaper, STRETCH_DELETESCANS);
			
			if (bTileWallpaper) // Tile
			{
				// The x point where we should start tiling the image
				int xInitial = -g_Monitors.at(0).Left + (int)floor((float)g_Monitors.at(0).Left/cxWallpaper)*cxWallpaper;
				// The y point where we should start tiling the image
				int yInitial = -g_Monitors.at(0).Top + (int)floor((float)g_Monitors.at(0).Top/cyWallpaper)*cyWallpaper;
				for (int x = xInitial; x < g_Monitors.at(0).ResX - g_Monitors.at(0).Left; x += cxWallpaper)
					for (int y = yInitial; y < g_Monitors.at(0).ResY - g_Monitors.at(0).Top; y += cyWallpaper)
						BitBlt(hdcDesktop, x, y, cxWallpaper, cyWallpaper, hdcWallpaper, 0, 0, SRCCOPY);
			}
			else // Some type of stretching
			{
				// Work out the dimensions the wallpaper should be stretched to
				int WallpaperResX, WallpaperResY;
				double scaleX, scaleY;
				switch (iWallpaperStyle)
				{
				case 2: // Stretch
					WallpaperResX = g_Monitors.at(1).ResX;
					WallpaperResY = g_Monitors.at(1).ResY;
					break;
				case 6: // Fit
					scaleX = (double)g_Monitors.at(1).ResX/cxWallpaper;
					scaleY = (double)g_Monitors.at(1).ResY/cyWallpaper;
					if (scaleX > scaleY)
					{
						WallpaperResY = g_Monitors.at(1).ResY;
						WallpaperResX = (int)(scaleY*cxWallpaper);
					}
					else
					{
						WallpaperResY = (int)(scaleX*cyWallpaper);
						WallpaperResX = g_Monitors.at(1).ResX;
					}
					break;
				case 10: // Fill
					scaleX = (double)g_Monitors.at(1).ResX/cxWallpaper;	
					scaleY = (double)g_Monitors.at(1).ResY/cyWallpaper;
					if (scaleX < scaleY)
					{
						WallpaperResY = g_Monitors.at(1).ResY;
						WallpaperResX = (int)(scaleY*cxWallpaper);
					}
					else
					{
						WallpaperResY = (int)(scaleX*cyWallpaper);
						WallpaperResX = g_Monitors.at(1).ResX;
					}
					break;
				default: // Center (actually 0), but this way we can fail graciously if the value is invalid
					WallpaperResX = cxWallpaper;
					WallpaperResY = cyWallpaper;
					break;
				}

				// Stretch the wallpaper as necesary
				HDC hdcStretchedWallpaper = CreateCompatibleDC(hdcScreen);
				HBITMAP hbmStretchedWallpaper = CreateCompatibleBitmap(hdcScreen, WallpaperResX, WallpaperResY);
				hbmStretchedWallpaper = (HBITMAP)SelectObject(hdcStretchedWallpaper, hbmStretchedWallpaper);
				StretchBlt(hdcStretchedWallpaper, 0, 0, WallpaperResX, WallpaperResY, hdcWallpaper, 0, 0, cxWallpaper, cyWallpaper, SRCCOPY);

				// Center the stretched wallpaper on all monitors
				for (int i = 1; i < (int)g_Monitors.size(); i++)
				{
					// Work out X coordinates and width
					int xsrc, xdest, width;
					if (g_Monitors.at(i).ResX > WallpaperResX)
					{
						xdest = (g_Monitors.at(i).ResX - WallpaperResX)/2;
						width = WallpaperResX;
						xsrc = 0;
					}
					else
					{
						xdest = 0;
						width = g_Monitors.at(i).ResX;
						xsrc = (WallpaperResX - g_Monitors.at(i).ResX)/2;
					}
					xdest += g_Monitors.at(i).Left - g_Monitors.at(0).Left;

					// Work out Y coordinates and height
					int ysrc, ydest, height;
					if (g_Monitors.at(i).ResY > WallpaperResY)
					{
						ydest = (g_Monitors.at(i).ResY - WallpaperResY)/2;
						height = WallpaperResY;
						ysrc = 0;
					}
					else
					{
						ydest = 0;
						height = g_Monitors.at(i).ResY;
						ysrc = (WallpaperResY - g_Monitors.at(i).ResY)/2;
					}
					ydest += g_Monitors.at(i).Top - g_Monitors.at(0).Top;

					BitBlt(hdcDesktop, xdest, ydest, width, height, hdcStretchedWallpaper, xsrc, ysrc, SRCCOPY);
				}
				DeleteDC(hdcStretchedWallpaper);
			}

			hbmWallpaper = (HBITMAP) SelectObject(hdcWallpaper, hbmOldWallpaper);
			DeleteObject(hbmWallpaper);
			DeleteDC(hdcWallpaper);
		}
		delete bm;
	}

	hBitmap = (HBITMAP)SelectObject(hdcDesktop, hOldBitmap);

	DeleteDC(hdcScreen);
	DeleteDC(hdcDesktop);

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
		case WM_ERASEBKGND:
		{
			pBlur->Draw((HDC)wParam);
			return 1;
		}
		case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *c = (WINDOWPOS*)lParam;
			c->hwnd = hWnd;
			c->hwndInsertAfter = HWND_BOTTOM;
			c->flags |= SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOMOVE;
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}