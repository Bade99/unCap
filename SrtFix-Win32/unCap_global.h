#pragma once
#include <Windows.h>

struct UNCAP_COLORS {
	HBRUSH ControlBk;
	HBRUSH ControlBkPush;
	HBRUSH ControlBkMouseOver;
	HBRUSH ControlTxt;
	HBRUSH ControlMsg;
	COLORREF InitialFinalCharDisabledColor;
	COLORREF InitialFinalCharCurrentColor;
	HBRUSH Scrollbar;
	HBRUSH ScrollbarMouseOver;
	HBRUSH ScrollbarBk;
	HBRUSH Img;
};

extern UNCAP_COLORS unCap_colors;
