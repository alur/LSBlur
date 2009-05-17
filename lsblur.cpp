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
	g_bStoreWallpaper = GetRCBool("BlurStoreWallpaper", false) ? true : false;

	if (!CreateMessageHandlers(hDllInstance))
		return 1;

	// Load *Blur lines
	ReadConfig();

	// Register for !Blur bangs
	AddBangCommand("!Blur", BangBlur);

	return 0; // Initialized succesfully
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// BangBlur
//
void BangBlur(HWND, LPCSTR pszArgs)
{
	CBitmapEx* bmpWallpaper = GetWallpaper();
	ParseBlurLine(pszArgs, bmpWallpaper);
	delete bmpWallpaper;
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

	// Delete all blur areas
	for (BlurMap::iterator iter = g_BlurMap.begin(); iter != g_BlurMap.end(); ++iter)
	{
		delete iter->second;
	}

	// Erase the BlurMap
	g_BlurMap.clear();

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
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
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

	g_hwndBlurHandler = CreateWindowEx(WS_EX_TOOLWINDOW, g_szBlurHandler, 0, WS_POPUP, 0, 0, 0, 0, 0, 0, hInst, 0);

	if (!g_hwndMessageHandler)
		return false; // Failed to create blur handler window

	// Register with litestep to recive LM_ messages
	SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);

	return true;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// GetWallpaper - Returns a reconstruction of the current wallpaper.
//
CBitmapEx* GetWallpaper()
{
	// Get the path to the wallpaper
	char szWallpaperPath[MAX_LINE_LENGTH] = { 0 };
	DWORD dwSize = sizeof(szWallpaperPath), dwType = REG_SZ;
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "Wallpaper", &dwType, &szWallpaperPath, &dwSize);

	// Get whether or not to tile the wallpaper
	char szTemp[32];
	dwSize = sizeof(szTemp);
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "TileWallpaper", &dwType, &szTemp, &dwSize);
	bool bTileWallpaper = atoi(szTemp) ? true : false;

	// Get whether or not to stretch the wallpaper
	SHGetValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "WallpaperStyle", &dwType, &szTemp, &dwSize);
	bool bStretchWallpaper = (atoi(szTemp) == 2) ? true : false;

	// Load the wallpaper
	CBitmapEx* bmpWallpaper = new CBitmapEx();
	bmpWallpaper->Load(szWallpaperPath);

	// TODO::Support center & tiling
	if (bStretchWallpaper) // Stretch
	{
		bmpWallpaper->Scale2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	}
	else if (bTileWallpaper) // Tile
	{
	
	}
	else // Center
	{
		
	}

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

	char szLine[MAX_LINE_LENGTH];
	CBitmapEx* bmpWallpaper = GetWallpaper();

	while (LCReadNextConfig(f, "*Blur", szLine, sizeof(szLine)))
	{
		ParseBlurLine(szLine, bmpWallpaper);
	}

	delete bmpWallpaper;

	LCClose(f);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// ParseBlurLine
//
// Returns true on success
//
bool ParseBlurLine(const char* szLine, CBitmapEx* bmpWallpaper)
{
	LPCSTR pszNext = szLine;
	char szToken[MAX_LINE_LENGTH], szName[MAX_BANGCOMMAND];
	int X, Y, Width, Height, Itterations;
	bool bReturn = false;

	if (szLine != NULL)
	{
		try
		{
			// *Blur/!Blur Name X Y Width Height Repetitions

			// Get the name
			GetToken(pszNext, szToken, &pszNext, false);
			memcpy(szName, szToken, sizeof(szName));

			// Look for the name in the BlurMap
			BlurMap::iterator iter = ::g_BlurMap.find(szName);
			if (iter != g_BlurMap.end())
			{
				return false; // The BlurArea already exists
			}

			// Get the X position
			GetToken(pszNext, szToken, &pszNext, false);
			X = atoi(szToken);

			// Get the Y positions
			GetToken(pszNext, szToken, &pszNext, false);
			Y = atoi(szToken);

			// Get the width
			GetToken(pszNext, szToken, &pszNext, false);
			Width = atoi(szToken);

			// Get the height
			GetToken(pszNext, szToken, &pszNext, false);
			Height = atoi(szToken);

			// Get the number of iterations
			GetToken(pszNext, szToken, &pszNext, false);
			Itterations = atoi(szToken);

			// Create the blurarea and insert it into the BlurMap
			BlurArea* blur = new BlurArea(X, Y, Width, Height, bmpWallpaper, szName, Itterations);
			g_BlurMap.insert(BlurMap::value_type(szName, blur));

			bReturn = true; // Everything went well
		}
		catch (...)
		{
			// TODO::Show error message
			throw;
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
			break;
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
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//	Blur Handler
//
LRESULT WINAPI BlurHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BlurArea* blur = (BlurArea*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!blur) // Not sure why this would happen, but no need to bother with it.
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
			blur->Draw(hDC);
			EndPaint(hWnd, &ps);
			return 0;
		}
		/*case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *c = (WINDOWPOS*)lParam;
			c->hwnd = hWnd;
			c->hwndInsertAfter = HWND_BOTTOM;
			c->x = blur->rPosition.left;
			c->y = blur->rPosition.top;
			c->cx = blur->rPosition.right;
			c->cy = blur->rPosition.bottom;
			c->flags = c->flags & ~SWP_NOZORDER;
			c->flags |= SWP_NOACTIVATE | SWP_NOSENDCHANGING;

			return 0;
		}*/
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}