//#include "stdafx.h"
#include "SubtitleWindow.h"

SubtitleWindow::SubtitleWindow()
{
}

SubtitleWindow::SubtitleWindow
(HINSTANCE hInstance, HWND parent, LPCWSTR Cursor, int Color, HFONT font)
{
	this->RegisterMyClass(hInstance, Cursor, Color);
	this->main = CreateWindowEx(WS_EX_CONTROLPARENT, this->WindowClassName.c_str(), nullptr,
								WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0,
								0, 0,parent,nullptr,hInstance,nullptr);
	//TODO(fran): where should I put this?
	SetWindowLongPtr(this->main, GWLP_USERDATA, (LONG_PTR)this);

}


SubtitleWindow::~SubtitleWindow()
{
}

LRESULT CALLBACK SubtitleWindow::SubProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	SubtitleWindow* pThis = (SubtitleWindow*)GetWindowLongPtr(window, GWLP_USERDATA);
	//This is how you get the particular object you are currently in, using a static function
	
	switch (message) {

	}

	return 0;
}

void SubtitleWindow::RegisterMyClass(HINSTANCE hInstance, LPCWSTR Cursor, int Color)
//TODO(fran): check what happens if the class gets registered multiple times with the same name
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &SubtitleWindow::SubProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, Cursor);

	this->SetValidWindowClassName(hInstance);

	wcex.lpszClassName = this->WindowClassName.c_str();

	RegisterClassExW(&wcex);
}

void SubtitleWindow::SetValidWindowClassName(HINSTANCE hInstance)
{
	auto random_string = []()->std::wstring {
		std::wstring str(L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

		std::random_device rd;
		std::mt19937 generator(rd());

		std::shuffle(str.begin(), str.end(), generator);

		return str.substr(0, 32);
	};
	WNDCLASSEXW wcx;//TODO(fran): how do you free this?
	do {
		this->WindowClassName = random_string();
	} while (GetClassInfoExW(hInstance, this->WindowClassName.c_str(), &wcx));
}

