#pragma once
#include <Windows.h>

#ifdef _DEBUG
#define UNCAP_ASSERTIONS
#endif

#ifdef UNCAP_ASSERTIONS
#define Assert(assertion) if(!(assertion))*(int*)NULL=0
#else 
#define Assert(assertion) 
#endif


//Thanks to https://handmade.network/forums/t/1273-post_your_c_c++_macro_tricks/3
template <typename F> struct Defer { Defer(F f) : f(f) {} ~Defer() { f(); } F f; };
template <typename F> Defer<F> makeDefer(F f) { return Defer<F>(f); };
#define __defer( line ) defer_ ## line
#define _defer( line ) __defer( line )
struct defer_dummy { };
template<typename F> Defer<F> operator+(defer_dummy, F&& f) { return makeDefer<F>(std::forward<F>(f)); }
//usage: defer{block of code;}; //the last defer in a scope gets executed first (LIFO)
#define defer auto _defer( __LINE__ ) = defer_dummy( ) + [ & ]( )

//RECT related

#define RECTWIDTH(r) (r.right >= r.left ? r.right - r.left : r.left - r.right )

#define RECTHEIGHT(r) (r.bottom >= r.top ? r.bottom - r.top : r.top - r.bottom )

static RECT rectWH(LONG left, LONG top, LONG width, LONG height) {
	RECT r;
	r.left = left;
	r.top = top;
	r.right = r.left + width;
	r.bottom = r.top + height;
	return r;
}

static RECT rectNpxL(RECT r, int n) {
	RECT res{ r.left,r.top,r.left + n,r.bottom };
	return res;
}
static RECT rectNpxT(RECT r, int n) {
	RECT res{ r.left,r.top,r.right,r.top + n };
	return res;
}
static RECT rectNpxR(RECT r, int n) {
	RECT res{ r.right - n,r.top,r.right,r.bottom };
	return res;
}
static RECT rectNpxB(RECT r, int n) {
	RECT res{ r.left ,r.bottom - n,r.right,r.bottom };
	return res;
}

static RECT rect1pxL(RECT r) {
	RECT res = rectNpxL(r, 1);
	return res;
}
static RECT rect1pxT(RECT r) {
	RECT res = rectNpxT(r, 1);
	return res;
}
static RECT rect1pxR(RECT r) {
	RECT res = rectNpxR(r, 1);
	return res;
}
static RECT rect1pxB(RECT r) {
	RECT res = rectNpxB(r, 1);
	return res;
}

#define BORDERLEFT 0x01
#define BORDERTOP 0x02
#define BORDERRIGHT 0x04
#define BORDERBOTTOM 0x08
//NOTE: borders dont get centered, if you choose 5 it'll go 5 into the rc. TODO(fran): centered borders
void FillRectBorder(HDC dc, RECT r, int thickness, HBRUSH br, int borders) {
	RECT borderrc;
	if (borders & BORDERLEFT) { borderrc = rectNpxL(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERTOP) { borderrc = rectNpxT(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERRIGHT) { borderrc = rectNpxR(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERBOTTOM) { borderrc = rectNpxB(r, thickness); FillRect(dc, &borderrc, br); }
}

static bool test_pt_rc(POINT p, RECT r) {
	bool res = false;
	if (p.y >= r.top &&//top
		p.y < r.bottom &&//bottom
		p.x >= r.left &&//left
		p.x < r.right)//right
	{
		res = true;
	}
	return res;
}

static bool rcs_overlap(RECT r1, RECT r2) {
	bool res = (r1.left < r2.right && r1.right > r2.left && r1.top < r2.bottom && r1.bottom > r2.top);
	return res;
}

static bool sameRc(RECT r1, RECT r2) {
	bool res = r1.bottom == r2.bottom && r1.left == r2.left && r1.right == r2.right && r1.top == r2.top;
	return res;
}

//HBRUSH related

static COLORREF ColorFromBrush(HBRUSH br) {
	LOGBRUSH lb;
	GetObject(br, sizeof(lb), &lb);
	return lb.lbColor;
}

//ICON related

struct MYICON_INFO //This is so stupid I find it hard to believe
{
	int     nWidth;
	int     nHeight;
	int     nBitsPerPixel;
};

static MYICON_INFO MyGetIconInfo(HICON hIcon)
{
	MYICON_INFO myinfo; ZeroMemory(&myinfo, sizeof(myinfo));

	ICONINFO info; ZeroMemory(&info, sizeof(info));

	BOOL bRes = FALSE; bRes = GetIconInfo(hIcon, &info);
	if (!bRes) return myinfo;

	BITMAP bmp; ZeroMemory(&bmp, sizeof(bmp));

	if (info.hbmColor)
	{
		const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			myinfo.nWidth = bmp.bmWidth;
			myinfo.nHeight = bmp.bmHeight;
			myinfo.nBitsPerPixel = bmp.bmBitsPixel;
		}
	}
	else if (info.hbmMask)
	{
		// Icon has no color plane, image data stored in mask
		const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			myinfo.nWidth = bmp.bmWidth;
			myinfo.nHeight = bmp.bmHeight / 2;
			myinfo.nBitsPerPixel = 1;
		}
	}

	if (info.hbmColor) DeleteObject(info.hbmColor);
	if (info.hbmMask) DeleteObject(info.hbmMask);

	return myinfo;
}

//HMENU related

BOOL SetMenuItemData(HMENU hmenu, UINT item, BOOL fByPositon, ULONG_PTR data) {
	MENUITEMINFO i;
	i.cbSize = sizeof(i);
	i.fMask = MIIM_DATA;
	i.dwItemData = data;
	return SetMenuItemInfo(hmenu, item, fByPositon, &i);
}

BOOL SetMenuItemString(HMENU hmenu, UINT item, BOOL fByPositon, const TCHAR* str) {
	MENUITEMINFOW menu_setter;
	menu_setter.cbSize = sizeof(menu_setter);
	menu_setter.fMask = MIIM_STRING;
	menu_setter.dwTypeData = const_cast<TCHAR*>(str);
	BOOL res = SetMenuItemInfoW(hmenu, item, fByPositon, &menu_setter);
	return res;
}