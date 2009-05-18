#pragma once

#define WIN32_LEAN_AND_MEAN
#include "lsapi.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include "BitmapEx.h"

class CBitmapEx;

extern HINSTANCE g_hInstance;
extern const DWORD g_dwExStyle;
extern const DWORD g_dwStyle;
extern const char g_szBlurHandler[];
extern HWND g_hwndMessageHandler;

class BlurArea
{
public:
	// Constructor/Destructor
	BlurArea(UINT X, UINT Y, UINT Width, UINT Height, CBitmapEx* bmpWallpaper, const char *szName, UINT Itterations);
	virtual ~BlurArea();

	// Functions
	void UpdateBackground(CBitmapEx* bmpWallpaper);
	void Draw(HDC hdc);
	void Move(UINT X, UINT Y, CBitmapEx* bmpWallpaper = NULL);
	void Resize(UINT Width, UINT Height, CBitmapEx* bmpWallpaper = NULL);
	void SetItterations(UINT Itterations, CBitmapEx* bmpWallpaper = NULL);

	// Variables
	char m_szName[MAX_PATH];
	CBitmapEx* m_BitMapHandler;
	HWND m_Window; // The main window
	UINT m_X, m_Y, m_Width, m_Height; // Positions
	UINT m_Itterations; // Number of times to apply the blur
};