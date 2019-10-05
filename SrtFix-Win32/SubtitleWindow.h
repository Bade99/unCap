#pragma once
#include "SubtitleFile.h"
#include <Windows.h>
#include <string>
#include <random>

///This is the window that will show the subtitle and recieve input and will be an element of a tab
class SubtitleWindow
{
private: 
	SubtitleFile file;
	HWND main;

	std::wstring WindowClassName;
public:
	SubtitleWindow();
	SubtitleWindow(HINSTANCE hInstance, HWND parent, LPCWSTR Cursor,int Color, HFONT font);
	~SubtitleWindow();
private:
	//How to "pass" this to the static function
	//https://devblogs.microsoft.com/oldnewthing/20140127-00/?p=1963
	//https://stackoverflow.com/questions/3725425/class-method-as-winapi-callback
	//https://stackoverflow.com/questions/400257/how-can-i-pass-a-class-member-function-as-a-callback
	//http://p-nand-q.com/programming/cplusplus/using_member_functions_with_c_function_pointers.html
	//https://stackoverflow.com/questions/14292803/can-i-have-main-window-procedure-as-a-lambda-in-winmain
	//https://stackoverflow.com/questions/23083/whats-an-alternative-to-gwl-userdata-for-storing-an-object-pointer
	static LRESULT CALLBACK SubProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	void SetValidWindowClassName(HINSTANCE hInstance);
	void RegisterMyClass(HINSTANCE hInstance, LPCWSTR Cursor, int Color);
};

