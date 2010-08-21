#pragma once

#include <string>
#include <map>
#include <vector>
#include <Windows.h>
#include <GdiPlus.h>
#include "blurarea.h"
#include "utility.h"
#include "shlwapi.h"

class BlurArea;

// Typedefs
typedef std::map<std::string, BlurArea*, stringicmp> BlurMap;
typedef struct MonitorInfo {
	int Top;
	int Left;
	int ResX;
	int ResY;
} MonitorInfo;

// Constants
const char g_rcsRevision[]		= "0.2";
const char g_szAppName[]		= "LSBlur";
const char g_szMsgHandler[]		= "LSBlurManager";
const char g_szBlurHandler[]	= "Blur";
const char g_szAuthor[]			= "Alurcard2";
const DWORD g_dwStyle			= WS_VISIBLE | WS_POPUP;
const DWORD g_dwExStyle			= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;

const int g_lsMessages[] = {LM_GETREVID, LM_REFRESH, 0};

// Externs
extern "C" {
    __declspec( dllexport ) int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath);
    __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);
}

// Function Declarations
void ReadConfig();
bool CreateMessageHandlers(HINSTANCE hInst);
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI BlurHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddBlur(RECT rRect);
void Experimenting();
bool ParseBlurLine(LPCSTR szLine, CBitmapEx* bmpWallpaper);
CBitmapEx* GetWallpaper();
void BangBlur(HWND hWnd, LPCSTR pszArgs);
void BangRemoveBlur(HWND hWnd, LPCSTR pszArgs);
void UpdateMonitorInfo();
BOOL CALLBACK SetMonitorVars(HMONITOR hMonitor, HDC, LPRECT, LPARAM);

// Variables
HWND g_hwndMessageHandler = NULL;
HWND g_hwndDesktop = NULL;
HINSTANCE g_hInstance = NULL;
bool g_bStoreWallpaper;
BlurMap g_BlurMap;
CBitmapEx* g_pWallpaper = NULL;
std::vector<MonitorInfo> g_Monitors;
ULONG_PTR gdiToken;