#pragma once
#include <Windows.h>

union UNCAP_COLORS {
	struct {
		HBRUSH ControlBk;
		HBRUSH ControlBkPush;
		HBRUSH ControlBkMouseOver;
		HBRUSH ControlTxt;
		HBRUSH ControlTxt_Inactive;
		HBRUSH ControlMsg;
		HBRUSH InitialFinalCharDisabled;
		HBRUSH Scrollbar;
		HBRUSH ScrollbarMouseOver;
		HBRUSH ScrollbarBk;
		HBRUSH Img;
		HBRUSH CaptionBk;
		HBRUSH CaptionBk_Inactive;
	};
	HBRUSH brushes[13];//REMEMBER to update
};

extern UNCAP_COLORS unCap_colors;

union UNCAP_FONTS {
	struct {
		HFONT General;
		HFONT Menu;
	};
	HFONT fonts[2];//REMEMBER to update
};

extern UNCAP_FONTS unCap_fonts;