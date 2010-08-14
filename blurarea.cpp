#include "blurarea.h"

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// BlurArea - Constructor for BlurArea class.
//
// bmpWallpaper should be a pointer to a CBitmapEx class which contains a
// reconstruction of the wallpaper.
//
BlurArea::BlurArea(UINT X, UINT Y, UINT Width, UINT Height, CBitmapEx* bmpWallpaper, const char *szName, UINT Itterations)
{
	// Store some values
	m_X = X;
	m_Y = Y;
	m_Width = Width;
	m_Height = Height;
	m_Itterations = Itterations;
	StringCchCopy(m_szName, MAX_PATH, szName);

	// Initalize the bitmap handler
	m_BitMapHandler = new CBitmapEx();
	UpdateBackground(bmpWallpaper);

	// Create Window
	m_Window = CreateWindowEx(g_dwExStyle, g_szBlurHandler, szName, g_dwStyle, X, Y, Width, Height, g_hwndDesktop, NULL, g_hInstance, 0);
	SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG)this);

	SetWindowPos(m_Window, HWND_BOTTOM, 0,0,0,0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// ~BlurArea - Destructor
//
BlurArea::~BlurArea()
{
	DestroyWindow(m_Window);
	delete m_BitMapHandler;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// Move - Moves the BlurArea
//
void BlurArea::Move(UINT X, UINT Y, CBitmapEx* bmpWallpaper)
{
	m_X = X;
	m_Y = Y;
	if (bmpWallpaper)
	{
		UpdateBackground(bmpWallpaper);
	}
	SetWindowPos(m_Window, HWND_BOTTOM, X, Y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// Resize - Resizes the BlurArea
//
void BlurArea::Resize(UINT Width, UINT Height, CBitmapEx* bmpWallpaper)
{
	m_Width = Width;
	m_Height = Height;
	if (bmpWallpaper)
	{
		UpdateBackground(bmpWallpaper);
	}
	SetWindowPos(m_Window, HWND_BOTTOM, 0, 0, Width, Height, SWP_NOACTIVATE | SWP_NOMOVE);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// SetItterations - Changes the number of itterations to do
//
void BlurArea::SetItterations(UINT Itterations, CBitmapEx* bmpWallpaper)
{
	m_Itterations = Itterations;
	if (bmpWallpaper)
	{
		UpdateBackground(bmpWallpaper);
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// UpdateBackground - Updates the image that is drawn
//
void BlurArea::UpdateBackground(CBitmapEx* bmpWallpaper)
{
	// Copy over the wallpaper replica
	m_BitMapHandler->Create(bmpWallpaper);
	
	// Get the correct part of the wallpaper
	m_BitMapHandler->Crop(m_X - GetSystemMetrics(SM_XVIRTUALSCREEN), m_Y - GetSystemMetrics(SM_YVIRTUALSCREEN), m_Width, m_Height);
	
	// Apply blur
	for (UINT i = 0; i < m_Itterations; i++)
		m_BitMapHandler->GaussianBlur();
	
	// Make sure that the window gets updated
	if (m_Window)
	{
		RECT InvalidRect = {0, 0, m_Width, m_Height};
		InvalidateRect(m_Window, &InvalidRect, false);
		UpdateWindow(m_Window);
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// Draw - Draws the blured area to DC
//
void BlurArea::Draw(HDC hDC)
{
	m_BitMapHandler->Draw(hDC);
}