#pragma once

#include "lsapi.h"
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include <string>
#include <map>
#include "utility.h"
#include <aggressiveoptimize.h>
#include "blurarea.h"

class BlurArea;
typedef std::map<std::string, BlurArea*, stringicmp> BlurMap;

// Constants
const char g_rcsRevision[]		= "0.1";
const char g_szAppName[]		= "LSBlur";
const char g_szMsgHandler[]		= "LSBlurManager";
const char g_szBlurHandler[]	= "Blur";
const char g_szAuthor[]			= "Alurcard2";
const DWORD g_dwStyle			= WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
const DWORD g_dwExStyle			= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;

const int g_lsMessages[] = {LM_GETREVID, LM_REFRESH, 0};

// Externs
extern "C" {
    __declspec( dllexport ) int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath);
    __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);
}

// Functions
void ReadConfig();
bool CreateMessageHandlers(HINSTANCE hInst);
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI BlurHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddBlur(RECT rRect);
void Experimenting();
BlurArea* ParseBlurLine(const char* szLine, CBitmapEx* bmpWallpaper);
CBitmapEx* GetWallpaper();

// Variables
HWND g_hwndMessageHandler;
HWND g_hwndBlurHandler;
HINSTANCE g_hInstance;
bool g_bStoreWallpaper;
BlurMap g_BlurMap;