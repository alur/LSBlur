#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include "BitmapEx.h"
#include "lsapi.h"
#include "shlwapi.h"

class CBitmapEx;

extern HINSTANCE g_hInstance;
extern const DWORD g_dwExStyle;
extern const DWORD g_dwStyle;
extern const char g_szBlurHandler[];

class BlurArea
{
public:
	// Constructor/Destructor
	BlurArea(UINT X, UINT Y, UINT Width, UINT Height, CBitmapEx* bmpWallpaper, const char *szName, UINT Itterations);
	virtual ~BlurArea();

	// Functions
	void UpdateBackground(CBitmapEx* bmpWallpaper);
	void Draw(HDC hdc);

	// Variables
	char m_szName[MAX_PATH];
	CBitmapEx* m_BitMapHandler;
	HWND m_Window; // The main window
	UINT m_X, m_Y, m_Width, m_Height; // Positions
	UINT m_Itterations; // Number of times to apply the blur
};