#pragma once
#include <Windows.h>
#include <windowsx.h>
#include "windows_msg_mapper.h"
#include "windows_undoc.h"

constexpr TCHAR unCap_wndclass_edit_oneline[] = TEXT("unCap_wndclass_edit_oneline");

static LRESULT CALLBACK EditOnelineProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

struct char_sel { u32 x, y; };//xth character of the yth row, goes left to right and top to bottom
struct EditOnelineProcState {
	HWND wnd;
	struct editonelinebrushes {
		HBRUSH txt, bk, border;//NOTE: for now we use the border color for the caret
		HBRUSH txt_dis, bk_dis, border_dis; //disabled
	}brushes;
	u32 char_max_sz;//doesnt include terminating null, also we wont have terminating null
	HFONT font;
	char_sel char_cur_sel;//this is in "character" coordinates, first char will be 0, second 1 and so on
	struct caretnfo {
		HBITMAP bmp;
		SIZE dim;
		POINT pos;//client coords, it's also top down so this is the _top_ of the caret
	}caret;

	str char_text;//much simpler to work with and debug
	std::vector<int> char_dims;//NOTE: specifies, for each character, its width
	//TODO(fran): it's probably better to use signed numbers in case the text overflows ths size of the control
	u32 char_pad_x;//NOTE: specifies x offset from which characters start being placed on the screen, relative to the client area. For a left aligned control this will be offset from the left, for right aligned it'll be offset from the right, and for center alignment it'll be the left most position from where chars will be drawn
};

void init_wndclass_unCap_edit_oneline(HINSTANCE instance) {
	WNDCLASSEXW edit_oneline;

	edit_oneline.cbSize = sizeof(edit_oneline);
	edit_oneline.style = CS_HREDRAW | CS_VREDRAW;
	edit_oneline.lpfnWndProc = EditOnelineProc;
	edit_oneline.cbClsExtra = 0;
	edit_oneline.cbWndExtra = sizeof(EditOnelineProcState*);
	edit_oneline.hInstance = instance;
	edit_oneline.hIcon = NULL;
	edit_oneline.hCursor = LoadCursor(nullptr, IDC_IBEAM);
	edit_oneline.hbrBackground = NULL;
	edit_oneline.lpszMenuName = NULL;
	edit_oneline.lpszClassName = unCap_wndclass_edit_oneline;
	edit_oneline.hIconSm = NULL;

	ATOM class_atom = RegisterClassExW(&edit_oneline);
	Assert(class_atom);
}

EditOnelineProcState* EDITONELINE_get_state(HWND wnd) {
	EditOnelineProcState* state = (EditOnelineProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	return state;
}

void EDITONELINE_set_state(HWND wnd, EditOnelineProcState* state) {//NOTE: only used on creation
	SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
}

//NOTE: the caller takes care of deleting the brushes, we dont do it
void EDITONELINE_set_brushes(HWND editoneline, BOOL repaint, HBRUSH txt, HBRUSH bk, HBRUSH border, HBRUSH txt_disabled, HBRUSH bk_disabled, HBRUSH border_disabled) {
	EditOnelineProcState* state = EDITONELINE_get_state(editoneline);
	if (txt)state->brushes.txt = txt;
	if (bk)state->brushes.bk = bk;
	if (border)state->brushes.border = border;
	if (txt_disabled)state->brushes.txt_dis = txt_disabled;
	if (bk_disabled)state->brushes.bk_dis = bk_disabled;
	if (border_disabled)state->brushes.txt_dis = border_disabled;
	if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
}

SIZE EDITONELINE_calc_caret_dim(EditOnelineProcState* state) {
	SIZE res;
	HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
	HFONT oldfont = SelectFont(dc, state->font); defer{ SelectFont(dc, oldfont); };
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	int avg_height = tm.tmHeight;
	res.cx = 1;
	res.cy = avg_height;
	return res;
}

int EDITONELINE_calc_line_height_px(EditOnelineProcState* state) {
	int res;
	HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
	HFONT oldfont = SelectFont(dc, state->font); defer{ SelectFont(dc, oldfont); };
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	int avg_height = tm.tmHeight;
	res = avg_height;
	return res;
}

SIZE EDITONELINE_calc_char_dim(EditOnelineProcState* state, cstr c) {
	Assert(c != _t('\t'));
	SIZE res;
	HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
	HFONT oldfont = SelectFont(dc, state->font); defer{ SelectFont(dc, oldfont); };
	//UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };
	//SetTextAlign(dc, TA_LEFT);
	GetTextExtentPoint32(dc, &c, 1, &res);
	return res;
}

SIZE EDITONELINE_calc_text_dim(EditOnelineProcState* state) {
	SIZE res;
	HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
	HFONT oldfont = SelectFont(dc, state->font); defer{ SelectFont(dc, oldfont); };
	//UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };
	//SetTextAlign(dc, TA_LEFT);
	GetTextExtentPoint32(dc, state->char_text.c_str(), (int)state->char_text.length(), &res);
	return res;
}

POINT EDITONELINE_calc_caret_p(EditOnelineProcState* state) {
	Assert(state->char_cur_sel.x-1 < state->char_dims.size());
	POINT res = { state->char_pad_x, 0 };
	for (int i=0; i < state->char_cur_sel.x; i++) {
		res.x+= state->char_dims[i];
		//TODO(fran): probably wrong
	}
	return res;
}

//BUGs:
//-caret stops blinking after a few seconds of being shown

LRESULT CALLBACK EditOnelineProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	printf(msgToString(msg)); printf("\n");
	EditOnelineProcState* state= EDITONELINE_get_state(hwnd);
	switch (msg) {
		//TODO(fran): on a WM_STYLECHANGING we should check if the alignment has changed and recalc/redraw every char, NOTE: I dont think windows' controls bother with this since it's not too common of a use case
	case WM_NCCREATE: 
	{ //1st msg received
		CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;

		Assert(!(creation_nfo->style & ES_MULTILINE));
		Assert(!(creation_nfo->style & ES_RIGHT));//TODO(fran)

		EditOnelineProcState* st = (EditOnelineProcState*)calloc(1, sizeof(EditOnelineProcState));
		Assert(st);
		EDITONELINE_set_state(hwnd, st);
		st->wnd = hwnd;
		st->char_max_sz = 32767;//default established by windows
		//NOTE: ES_LEFT==0, that was their way of defaulting to left
		if (creation_nfo->style & ES_CENTER) {
			//NOTE: ES_CENTER needs the pad to be recalculated all the time
			
			st->char_pad_x = abs(creation_nfo->cx / 2);//HACK
		}
		else {//we are either left or right
			st->char_pad_x = 1;
		}
		st->char_text = str();//REMEMBER: this c++ guys dont like being calloc-ed, they need their constructor, or, in this case, someone else's, otherwise they are badly initialized
		st->char_dims = std::vector<int>();
		return TRUE; //continue creation
	} break;
	case WM_NCCALCSIZE: { //2nd msg received https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-nccalcsize
		if (wparam) {
			//Indicate part of current client area that is valid
			NCCALCSIZE_PARAMS* calcsz = (NCCALCSIZE_PARAMS*)lparam;
			return 0; //causes the client area to resize to the size of the window, including the window frame
		}
		else {
			RECT* client_rc = (RECT*)lparam;
			//TODO(fran): make client_rc cover the full window area
			return 0;
		}
	} break;
	case WM_CREATE://3rd msg received
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SIZE: {//4th, strange, I though this was sent only if you didnt handle windowposchanging (or a similar one)
		//NOTE: neat, here you resize your render target, if I had one or cared to resize windows' https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOVE: //5th. Sent on startup after WM_SIZE, although possibly sent by DefWindowProc after I let it process WM_SIZE, not sure
	{
		//This msg is received _after_ the window was moved
		//Here you can obtain x and y of your window's client area
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SHOWWINDOW: //6th. On startup I received this cause of WS_VISIBLE flag
	{
		//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCPAINT://7th
	{
		//Paint non client area, we shouldnt have any
		HDC hdc = GetDCEx(hwnd, (HRGN)wparam, DCX_WINDOW | DCX_USESTYLE);
		ReleaseDC(hwnd, hdc);
		return 0; //we process this message, for now
	} break;
	case WM_ERASEBKGND://8th
	{
		//You receive this msg if you didnt specify hbrBackground  when you registered the class, now it's up to you to draw the background
		HDC dc = (HDC)wparam;
		//TODO(fran): look at https://docs.microsoft.com/en-us/windows/win32/gdi/drawing-a-custom-window-background and SetMapModek, allows for transforms

		return 0; //If you return 0 then on WM_PAINT fErase will be true, aka paint the background there
	} break;
	case EM_LIMITTEXT://Set text limit in _characters_ ; does NOT include the terminating null
		//wparam = unsigned int with max char count ; lparam=0
	{
		//TODO(fran): docs says this shouldnt affect text already inside the control nor text set via WM_SETTEXT, not sure I agree with that
		state->char_max_sz = (u32)wparam;
		return 0;
	} break;
	case WM_SETFONT:
	{
		//TODO(fran): should I make a copy of the font? it seems like windows just tells the user to delete the font only after they deleted the control so probably I dont need a copy
		HFONT font = (HFONT)wparam;
		state->font = font;
		BOOL redraw = LOWORD(lparam);
		if(redraw) RedrawWindow(state->wnd, NULL, NULL, RDW_INVALIDATE);
		return 0;
	} break;
	case WM_DESTROY:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
	{
		//NOTE: the brushes are deleted by whoever created them
		free(state);
		return 0;
	}break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RECT rc; GetClientRect(state->wnd, &rc);
		//ps.rcPaint
		HDC dc = BeginPaint(state->wnd, &ps);
		int border_thickness = 1;
		HBRUSH bk_br, txt_br, border_br;
		if (IsWindowEnabled(state->wnd)) {
			bk_br = state->brushes.bk;
			txt_br = state->brushes.txt;
			border_br = state->brushes.border;
		}
		else {
			bk_br = state->brushes.bk_dis;
			txt_br = state->brushes.txt_dis;
			border_br = state->brushes.border_dis;
		}
		FillRectBorder(dc, rc, border_thickness, border_br, BORDERLEFT | BORDERTOP | BORDERRIGHT | BORDERBOTTOM);

		{
			//Clip the drawing region to avoid overriding the border
			HRGN restoreRegion = CreateRectRgn(0, 0, 0, 0); if (GetClipRgn(dc, restoreRegion) != 1) { DeleteObject(restoreRegion); restoreRegion = NULL; } defer{ SelectClipRgn(dc, restoreRegion); if (restoreRegion != NULL) DeleteObject(restoreRegion); };
			{
				RECT left = rectNpxL(rc, border_thickness);
				RECT top = rectNpxT(rc, border_thickness);
				RECT right = rectNpxR(rc, border_thickness);
				RECT bottom = rectNpxB(rc, border_thickness);
				ExcludeClipRect(dc, left.left, left.top, left.right, left.bottom);
				ExcludeClipRect(dc, top.left, top.top, top.right, top.bottom);
				ExcludeClipRect(dc, right.left, right.top, right.right, right.bottom);
				ExcludeClipRect(dc, bottom.left, bottom.top, bottom.right, bottom.bottom);
				//TODO(fran): this aint to clever, it'd be easier to set the clipping region to the deflated rect
			}
			//Draw

			RECT bk_rc = rc; InflateRect(&bk_rc, -border_thickness, -border_thickness);
			FillRect(dc, &bk_rc, bk_br);//TODO(fran): we need to clip this where the text was already drawn, this will be the last thing we paint

			{
				HFONT oldfont = SelectFont(dc, state->font); defer{ SelectFont(dc, oldfont); };
				UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };

				COLORREF oldtxtcol = SetTextColor(dc, ColorFromBrush(txt_br)); defer{ SetTextColor(dc, oldtxtcol); };
				COLORREF oldbkcol = SetBkColor(dc, ColorFromBrush(bk_br)); defer{ SetBkColor(dc, oldbkcol); };

				TEXTMETRIC tm;
				GetTextMetrics(dc, &tm);
				// Calculate vertical position for the string so that it will be vertically centered
				// We are single line so we want vertical alignment always
				int yPos = (rc.bottom + rc.top - tm.tmHeight) / 2;
				int xPos;
				LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				//ES_LEFT ES_CENTER ES_RIGHT
				//TODO(fran): store char positions in the vector
				if (style & ES_CENTER) {
					SetTextAlign(dc, TA_CENTER);
					xPos = (rc.right - rc.left) / 2;
				}
				else if (style & ES_RIGHT) {
					SetTextAlign(dc, TA_RIGHT);
					xPos = rc.right - state->char_pad_x;
				}
				else /*ES_LEFT*/ {//NOTE: ES_LEFT==0, that was their way of defaulting to left
					SetTextAlign(dc, TA_LEFT);
					xPos = rc.left + state->char_pad_x;
				}

				TextOut(dc, xPos, yPos, state->char_text.c_str(), (int)state->char_text.length());
			}
		}
		EndPaint(hwnd, &ps);
		return 0;
	} break;
	case WM_ENABLE:
	{//Here we get the new enabled state
		BOOL now_enabled = (BOOL)wparam;
		InvalidateRect(state->wnd, NULL, TRUE);
		//TODO(fran): Hide caret
		return 0;
	} break;
	case WM_CANCELMODE:
	{
		//Stop mouse capture, and similar input related things
		//Sent, for example, when the window gets disabled
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCHITTEST://When the mouse goes over us this is 1st msg received
	{
		//Received when the mouse goes over the window, on mouse press or release, and on WindowFromPoint

		// Get the point coordinates for the hit test.
		POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner

		// Get the window rectangle.
		RECT rw; GetWindowRect(state->wnd, &rw);

		LRESULT hittest = HTNOWHERE;

		// Determine if the point is inside the window
		if (test_pt_rc(mouse, rw))hittest = HTCLIENT;

		return hittest;
	} break;
	case WM_SETCURSOR://When the mouse goes over us this is 2nd msg received
	{
		//DefWindowProc passes this to its parent to see if it wants to change the cursor settings, we'll make a decision, setting the mouse cursor, and halting proccessing so it stays like that
		//Sent after getting the result of WM_NCHITTEST, mouse is inside our window and mouse input is not being captured

		/* https://docs.microsoft.com/en-us/windows/win32/learnwin32/setting-the-cursor-image
			if we pass WM_SETCURSOR to DefWindowProc, the function uses the following algorithm to set the cursor image:
			1. If the window has a parent, forward the WM_SETCURSOR message to the parent to handle.
			2. Otherwise, if the window has a class cursor, set the cursor to the class cursor.
			3. If there is no class cursor, set the cursor to the arrow cursor.
		*/
		//NOTE: I think this is good enough for now
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEMOVE /*WM_MOUSEFIRST*/://When the mouse goes over us this is 3rd msg received
	{
		//wparam = test for virtual keys pressed
		POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEACTIVATE://When the user clicks on us this is 1st msg received
	{
		HWND parent = (HWND)wparam;
		WORD hittest = LOWORD(lparam);
		WORD mouse_msg = HIWORD(lparam);
		SetFocus(state->wnd);//set keyboard focus to us
		return MA_ACTIVATE; //Activate our window and post the mouse msg
	} break;
	case WM_LBUTTONDOWN://When the user clicks on us this is 2nd msg received (possibly, maybe wm_setcursor again)
	{
		//wparam = test for virtual keys pressed
		POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area
		int caret_x, caret_y= EDITONELINE_calc_line_height_px(state)*0;
		if (state->char_text.empty()) {
			state->char_cur_sel = { 0,0 };
			caret_x = state->char_pad_x;//TODO(fran): how calcs the char_pad?
		}
		else {
			//TODO
			int mouse_x = mouse.x;
			int dim_x = state->char_pad_x;
			int char_cur_sel_x;
			if ((mouse_x - dim_x) <= 0) {
				caret_x = dim_x;
				char_cur_sel_x = 0;
			}
			else {
				int i = 0;
				for (; i < (int)state->char_dims.size(); i++) {
					dim_x += state->char_dims[i];
					if ((mouse_x - dim_x) < 0) {
						//if (i + 1 < (int)state->char_dims.size()) {//if there were more chars to go
							dim_x -= state->char_dims[i];
						//}
						break;
					}//TODO(fran): there's some bug here, selection change isnt as tight as it should, caret placement is correct but this overextends the size of the char
				}
				caret_x = dim_x;
				char_cur_sel_x = i;
			}
			state->char_cur_sel.x = char_cur_sel_x;
		}
		SetCaretPos(caret_x, caret_y);
		InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): dont invalidate everything
		return 0;
	} break;
	case WM_LBUTTONUP:
	{
		//TODO(fran):Stop mouse tracking?
	} break;
	case WM_IME_SETCONTEXT://Sent after SetFocus to us
	{//TODO(fran): lots to discover here
		BOOL is_active = (BOOL)wparam;
		//lparam = display options for IME window
		u32 disp_flags = (u32)lparam;
		//NOTE: if we draw the composition window we have to modify some of the lparam values before calling defwindowproc or ImmIsUIMessage

		//If we have created an IME window, call ImmIsUIMessage. Otherwise pass this message to DefWindowProc
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}break;
	case WM_SETFOCUS://SetFocus -> WM_IME_SETCONTEXT -> WM_SETFOCUS
	{
		//We gained keyboard focus
		SIZE caret_dim = EDITONELINE_calc_caret_dim(state);
		if (caret_dim.cx != state->caret.dim.cx || caret_dim.cy != state->caret.dim.cy) {
			if (state->caret.bmp) {
				DeleteBitmap(state->caret.bmp);
				state->caret.bmp = 0;
			}
			//TODO(fran): we should probably check the bpp of our control's img
			int pitch = caret_dim.cx * 4/*bytes per pixel*/;//NOTE: windows expects word aligned bmps, 32bits are always word aligned
			int sz = caret_dim.cy*pitch;
			u32* mem = (u32*)malloc(sz); defer{ free(mem); };
			COLORREF color = ColorFromBrush(state->brushes.txt_dis);
			int px_count = sz / 4;
			for (int i = 0; i < px_count; i++) *mem = color;
			state->caret.bmp = CreateBitmap(caret_dim.cx, caret_dim.cy, 1, 32, (void*)mem);
			state->caret.dim = caret_dim;
		}
		BOOL caret_res = CreateCaret(state->wnd, state->caret.bmp, 0, 0);
		Assert(caret_res);
		BOOL showcaret_res = ShowCaret(state->wnd); //TODO(fran): the docs seem to imply you display the caret here but I dont wanna draw outside wm_paint
		Assert(showcaret_res);
		return 0;
	} break;
	case WM_KILLFOCUS:
	{
		//We lost keyboard focus
		//TODO(fran): docs say we should destroy the caret now
		DestroyCaret();
		//Also says to not display/activate a window here cause we can lock the thread
		return 0;
	} break;
	case WM_IME_NOTIFY:
	{
		//Notifies about changes to the IME window
		//TODO(fran): process this msgs once we manage the ime window
		u32 command = (u32)wparam;
		//lparam = command specific data
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_KEYDOWN://When the user presses a non-system key this is the 1st msg we receive
	{
		//TODO(fran): here we process things like VK_HOME VK_NEXT VK_LEFT VK_RIGHT VK_DELETE
		//NOTE: there are some keys that dont get sent to WM_CHAR, we gotta handle them here, also there are others that get sent to both, TODO(fran): it'd be nice to have all the key things in only one place
		char vk = (char)wparam;
		switch (vk) {
		case VK_HOME://Home
		{

		} break;

		}
		InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): dont invalidate everything
		return 0;
	}break;
	case WM_CHAR://When the user presses a non-system key this is the 2nd msg we receive
	{//NOTE: a WM_KEYDOWN msg was translated by TranslateMessage() into WM_CHAR
		TCHAR c = (TCHAR)wparam;
		//lparam = flags
		switch (c) { //https://docs.microsoft.com/en-us/windows/win32/menurc/using-carets
		case VK_BACK://Backspace
		{
			
		}break;
		case VK_TAB://Tab
		{

		}break;
		case VK_RETURN://Carriage Return aka "Enter"
		{

		}break;
		case VK_ESCAPE://Escape
		{

		}break;
		case 0x0A://Linefeed, wtf is that
		{

		}break;
		default:
		{
			//We have some normal character
			//TODO(fran): what happens with surrogate pairs? I dont even know what they are -> READ
			if (state->char_text.length() < state->char_max_sz) {
				state->char_text += c;
				state->char_cur_sel.x++;//TODO: this is wrong
				state->char_dims.push_back(EDITONELINE_calc_char_dim(state,c).cx);//TODO(fran): also wrong, we'll need to insert in the middle if the cur_sel is not a the end

				LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				if (style & ES_CENTER) {
					//Recalc pad_x
					RECT rc; GetClientRect(state->wnd, &rc);
					state->char_pad_x = (RECTWIDTH(rc) - EDITONELINE_calc_text_dim(state).cx) / 2;

					state->caret.pos = EDITONELINE_calc_caret_p(state);
					SetCaretPos(state->caret.pos.x, state->caret.pos.y);

				}

				wprintf(L"%s\n", state->char_text.c_str());
			}

		}break;
		}
		InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): dont invalidate everything
		return 0;
	} break;
	case WM_KEYUP:
	{
		//TODO(fran): smth to do here?
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	default:
	{
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
	} break;
	}
	return 0;
}
//TODO(fran): the rest of the controls should also SetFocus to themselves