#pragma once

//Pointless warnings
#pragma warning(disable : 4505) //unreferenced local function has been removed
#pragma warning(disable : 4189) //local variable is initialized but not referenced

//C++17 deprecation
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 

#include "windows_msg_mapper.h"

#define WIN32_LEAN_AND_MEAN 
#define STRICT_TYPED_ITEMIDS
#include "targetver.h"

#include "resource.h"
#include <Windows.h>
#include <Windowsx.h> //GET_X_LPARAM GET_Y_LPARAM
#include "Open_Save_FileHandler.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <propvarutil.h>
#include <Propsys.h>
#include <propkey.h>
#include <Commdlg.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include "LANGUAGE_MANAGER.h"
#include <Shlobj.h>//SHGetKnownFolderPath
#include "utf8.h" //Thanks to http://utfcpp.sourceforge.net/
#include "text_encoding_detect.h" //Thanks to https://github.com/AutoItConsulting/text-encoding-detect
#include "fmt.h"
#include "unCap_Helpers.h"
#include "unCap_Global.h"
#include "unCap_math.h"
#include "unCap_uncapnc.h"
#include "unCap_scrollbar.h"
#include "unCap_button.h"
#include"unCap_Renderer.h"
#include "unCap_Core.h"
#include "unCap_combobox.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "unCap_uncapcl.h"

//----------------------LINKER----------------------:
// Linker->Input->Additional Dependencies (right way to link the .lib)
#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"propsys.lib") //open save file handler
#pragma comment(lib,"shlwapi.lib") //open save file handler
#pragma comment(lib,"UxTheme.lib") // setwindowtheme

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") 
//#pragma comment(lib,"User32.lib")

//----------------------TODOs----------------------:
//unCap_Logger.h
//Paint Initial char and Final char wnds to look like the rest, add white border so they are more easily noticeable
//Parametric sizes for everything, and DPI awareness for controls
//Move error messages from a separate dialog window into the space on the right of the Remove controls
//Option to retain original file encoding on save, otherwise utf8/user defined default
//Additional File Format support?: webvtt,microdvd,subviewer,ttml,sami,mpsub,ttxt

//BUGS
//When the tab control is empty and you load the first file it doesnt seem to update the comment marker
//On different colors the menus' images dont render correctly
//Not every img is using uncap_colors.Img
//The name of the application (unCap) does not show until you add a tab
//Deserialization of unCap_colors seems to fail sometimes and have to go to defaults
//Maximizing breaks everything

//----------------------GLOBALS----------------------:
i32 n_tabs = 0;//Needed for serialization

UNCAP_COLORS unCap_colors{0};
UNCAP_FONTS unCap_fonts{0};

//Timing info for testing
f64 GetPCFrequency() {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return f64(li.QuadPart) / 1000.0; //milliseconds
}
inline i64 StartCounter() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}
inline f64 EndCounter(i64 CounterStart, f64 PCFreq = GetPCFrequency())
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

//The dc is passed to EnumFontFamiliesEx, you can just pass the desktop dc for example //TODO(fran): can we guarantee which dc we use doesnt matter? in that case dont ask the user for a dc and do it myself
BOOL hasFontFace(HDC dc, const TCHAR* facename) {
	int res = EnumFontFamiliesEx(dc/*You have to put some dc,not NULL*/, NULL
		, [](const LOGFONT *lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lparam)->int {
			if (!StrCmpI((TCHAR*)lparam, lpelfe->lfFaceName)) {//Non case-sensitive comparison
				return 0;
			}
			return 1;
		}
	, (LPARAM)facename, NULL);
	return !res;
}

#if 0 //TODO(fran): this method without std::vector is faster for the best case but slower for the worst case
/// <summary>
/// Decides which font FaceName is appropiate for the current system
/// </summary>
str GetFontFaceName() {
	//Font guidelines: https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-fonts
	//Stock fonts: https://docs.microsoft.com/en-us/windows/win32/gdi/using-a-stock-font-to-draw-text

	//TODO(fran): can we take the best codepage from each font and create our own? (look at font linking & font fallback)

	//We looked at 2195 fonts, this is what's left
	//Options:
	//Segoe UI (good txt, jp ok) (10 codepages) (supported on most versions of windows)
	//-Arial Unicode MS (good text; good jp) (33 codepages) (doesnt come with windows)
	//-Microsoft YaHei or UI (look the same,good txt,good jp) (6 codepages) (supported on most versions of windows)
	//-Microsoft JhengHei or UI (look the same,good txt,ok jp) (3 codepages) (supported on most versions of windows)

	i64 cnt = StartCounter(); defer{ printf("ELAPSED: %f\n",EndCounter(cnt)); };

	TCHAR res[ARRAYSIZE(((LOGFONT*)0)->lfFaceName)]{0};
	const TCHAR* requested_fontname[] = { TEXT("Segoe UI"), TEXT("Arial Unicode MS"), TEXT("Microsoft YaHei"), TEXT("Microsoft YaHei UI")
										, TEXT("Microsoft JhengHei"), TEXT("Microsoft JhengHei UI")};
	HDC dc = GetDC(GetDesktopWindow()); defer{ ReleaseDC(GetDesktopWindow(),dc); };
	for (const TCHAR* req : requested_fontname) {
		if (hasFontFace(dc, req)) {
			StrCpy(res, req);
			break;
		}
	}

	return res;
}
#else
str GetFontFaceName() {
	//Font guidelines: https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-fonts
	//Stock fonts: https://docs.microsoft.com/en-us/windows/win32/gdi/using-a-stock-font-to-draw-text

	//TODO(fran): can we take the best codepage from each font and create our own? (look at font linking & font fallback)

	//We looked at 2195 fonts, this is what's left
	//Options:
	//Segoe UI (good txt, jp ok) (10 codepages) (supported on most versions of windows)
	//-Arial Unicode MS (good text; good jp) (33 codepages) (doesnt come with windows)
	//-Microsoft YaHei or UI (look the same,good txt,good jp) (6 codepages) (supported on most versions of windows)
	//-Microsoft JhengHei or UI (look the same,good txt,ok jp) (3 codepages) (supported on most versions of windows)

	i64 cnt = StartCounter(); defer{ printf("ELAPSED: %f ms\n",EndCounter(cnt)); };

	HDC dc = GetDC(GetDesktopWindow()); defer{ ReleaseDC(GetDesktopWindow(),dc); }; //You can use any hdc, but not NULL
	std::vector<str> fontnames;
	EnumFontFamiliesEx(dc, NULL
		, [](const LOGFONT *lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)->int {((std::vector<str>*)lParam)->push_back(lpelfe->lfFaceName); return TRUE; }
	, (LPARAM)&fontnames, NULL);

	const TCHAR* requested_fontname[] = { TEXT("Segoe UI"), TEXT("Arial Unicode MS"), TEXT("Microsoft YaHei"), TEXT("Microsoft YaHei UI")
										, TEXT("Microsoft JhengHei"), TEXT("Microsoft JhengHei UI") };

	for (int i = 0; i < ARRAYSIZE(requested_fontname); i++)
		if (std::any_of(fontnames.begin(), fontnames.end(), [f = requested_fontname[i]](str s) {return !s.compare(f); })) return requested_fontname[i];

	return TEXT("");
}
#endif

v2 GetDPI(HWND hwnd)//https://docs.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
{
	HDC hdc = GetDC(hwnd);
	v2 dpi;
	dpi.x = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	dpi.y = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(hwnd, hdc);
	return dpi;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	urender::init(); defer{ urender::uninit(); };

	//Initialization of common controls
	INITCOMMONCONTROLSEX icc{ sizeof(icc) };
	icc.dwICC = ICC_STANDARD_CLASSES;

	BOOL comm_res = InitCommonControlsEx(&icc);
	Assert(comm_res);

#ifdef _DEBUG
	AllocConsole();
	FILE* ___s; defer{ fclose(___s); };
	freopen_s(&___s, "CONIN$", "r", stdin);
	freopen_s(&___s, "CONOUT$", "w", stdout);
	freopen_s(&___s, "CONOUT$", "w", stderr);
#endif


	LOGFONT lf{0};
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfHeight = -15;//TODO(fran): parametric
	//INFO: by default if I dont set faceName it uses "Modern", looks good but it lacks some charsets
	StrCpyN(lf.lfFaceName, GetFontFaceName().c_str(), ARRAYSIZE(lf.lfFaceName));
	
	unCap_fonts.General = CreateFontIndirect(&lf);
	if (!unCap_fonts.General) MessageBoxW(NULL, RCS(LANG_FONT_ERROR), RCS(LANG_ERROR), MB_OK);

	lf.lfHeight = (LONG)((float)GetSystemMetrics(SM_CYMENU)*.85f);

	unCap_fonts.Menu = CreateFontIndirectW(&lf);
	if (!unCap_fonts.Menu) MessageBoxW(NULL, RCS(LANG_FONT_ERROR), RCS(LANG_ERROR), MB_OK);

	const str to_deserialize = load_file_serialized();

	LANGUAGE_MANAGER& lang_mgr = LANGUAGE_MANAGER::Instance(); lang_mgr.SetHInstance(hInstance);
	unCapClSettings uncap_cl;

	_BeginDeserialize();
	_deserialize_struct(lang_mgr, to_deserialize);
	_deserialize_struct(uncap_cl, to_deserialize);
	_deserialize_struct(unCap_colors, to_deserialize);
	default_colors_if_not_set(&unCap_colors);
	defer{ for (HBRUSH& b : unCap_colors.brushes) if (b) { DeleteBrush(b); b = NULL; } };

	init_wndclass_unCap_uncap_cl(hInstance);

	init_wndclass_unCap_uncap_nc(hInstance);

	init_wndclass_unCap_scrollbar(hInstance);

	init_wndclass_unCap_button(hInstance);

	RECT uncapnc_rc = UNCAPNC_calc_nonclient_rc_from_client(uncap_cl.rc,TRUE);

	unCapNcLpParam nclpparam;
	nclpparam.client_class_name = unCap_wndclass_uncap_cl;
	nclpparam.client_lp_param = &uncap_cl;

	//TODO(fran): I think the problem I get that someone else is drawing my nc area is because I ask for OVERLAPPED_WINDOW when I create the window, so it takes care of doing that, which I dont want, REMOVE IT
	HWND uncapnc = CreateWindowEx(WS_EX_CONTROLPARENT/*|WS_EX_ACCEPTFILES*/, unCap_wndclass_uncap_nc, NULL, WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		uncapnc_rc.left, uncapnc_rc.top, RECTWIDTH(uncapnc_rc), RECTHEIGHT(uncapnc_rc), nullptr, nullptr, hInstance, &nclpparam);

	if (!uncapnc) return FALSE;

	ShowWindow(uncapnc, nCmdShow);
	UpdateWindow(uncapnc);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SRTFIXWIN32));

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		//if (!IsDialogMessage(hwnd, &msg))
		//{
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}

	}

	str serialized;
	_BeginSerialize();
	serialized += _serialize_struct(lang_mgr);
	serialized += _serialize_struct(uncap_cl);
	serialized += _serialize_struct(unCap_colors);

	save_to_file_serialized(serialized);

	return (int)msg.wParam;
}