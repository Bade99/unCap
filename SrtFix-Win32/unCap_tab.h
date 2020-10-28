#pragma once
#include <Windows.h>
#include <windowsx.h>
#include "unCap_Global.h"
#include "unCap_Helpers.h"
#include <Commctrl.h>
#include "unCap_Core.h"

//NOTE: this control is only for TCS_FIXEDWIDTH tabs

//Setting offsets for what will define the "client" area of a tab control
struct TABCLIENT { //Construct the "client" space of the tab control from this offsets, aka the space where you put your controls, in my case text editor
	short leftOffset, topOffset, rightOffset, bottomOffset;
};
constexpr TABCLIENT TabOffset{ 3,24,3,3 };//TODO(fran): parametric, and we need a TAB_GetClientRc instead of this crap

#define TCM_RESIZETABS (WM_USER+50) //Sent to a tab control for it to tell its tabs' controls to resize. wParam = lParam = 0
#define TCM_RESIZE (WM_USER+51) //wParam= pointer to SIZE of the tab control ; lParam = 0

#define TCN_DELETEITEM (TCN_FIRST - 8U) //Notifies the parent through WM_PARENTNOTIFY before an item is deleted; LOWORD(wParam)=TCN_DELETEITEM ; HIWORD(wParam)=(int) index of the item ; lParam=HWND of the tab control

struct TAB_INFO {
	//TODO(fran): will we store the data for each control or store the controls themselves and hide them?
	HWND hText = NULL;
	WCHAR filePath[MAX_PATH] = { 0 };//TODO(fran): nowadays this isn't really the max path lenght
	COMMENT_TYPE commentType = COMMENT_TYPE::brackets;
	WCHAR initialChar = L'\0';
	WCHAR finalChar = L'\0';
	FILE_FORMAT fileFormat = FILE_FORMAT::SRT;
	TXT_ENCODING fileEncoding = TXT_ENCODING::UTF8;
};

struct CUSTOM_TCITEM {
	TC_ITEMHEADER tab_info;
	TAB_INFO extra_info;
};

RECT TAB_calc_close_button(RECT tab) {
	RECT res = tab;
	int h = RECTHEIGHT(tab);
	int w = RECTWIDTH(tab);
	LONG sz = min((LONG)(h*.6f), w);
	res.right -= ((h - sz) / 2);
	res.left = res.right - sz;
	res.top += ((h - sz) / 2);
	res.bottom = res.top + sz;
	return res;
}

int TAB_get_next_pos(HWND tab) {
	int count = (int)SendMessage(tab, TCM_GETITEMCOUNT, 0, 0);
	return count + 1; //TODO(fran): check if this is correct, or there is a better way
}

int TAB_change_selected(HWND tab, int index) {
	return (int)SendMessage(tab, TCM_SETCURSEL, index, 0);
}

//Determines if a close button was pressed in any of the tab control's tabs and returns its zero based index
//p must be in client space of the tab control
//Returns -1 if no button was pressed
int TestCloseButton(HWND tabControl, POINT p) {
	//1.Determine which tab was hit and retrieve its rect
	//From what I understand the tab change happens before we get here, so for simplicity's sake we will only test against the currently selected tab.
	//If this didnt work we'd probably have to use the TCM_HITTEST message
	RECT item_rc;
	int item_index = (int)SendMessage(tabControl, TCM_GETCURSEL, 0, 0);
	if (item_index != -1) {
		BOOL ret = (BOOL)SendMessage(tabControl, TCM_GETITEMRECT, SendMessage(tabControl, TCM_GETCURSEL, 0, 0), (LPARAM)&item_rc); //returns rc in "viewport" coords, aka in respect to tab control, so it's good

		if (ret) {
			item_rc = TAB_calc_close_button(item_rc);
			//2.Adjust rect to close button position
			//item_rc.left += RECTWIDTH(item_rc) - closeButtonInfo.icon.cx - closeButtonInfo.rightPadding;
			//item_rc.right -= closeButtonInfo.rightPadding;
			//LONG yChange = (RECTHEIGHT(item_rc) - closeButtonInfo.icon.cy) / 2;
			//item_rc.top += yChange;
			//item_rc.bottom -= yChange;

			//3.Determine if the close button was hit
			BOOL res = test_pt_rc(p, item_rc);

			//4.Return index or -1
			if (res) return item_index;
		}

	}
	return -1;
}

//TODO(fran): get this guy out of here once the HACKs are removed
LRESULT CALLBACK TabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//INFO: if I ever feel like managing WM_PAINT: https://social.msdn.microsoft.com/Forums/expression/en-US/d25d08fb-acf1-489e-8e6c-55ac0f3d470d/tab-controls-in-win32-api?forum=windowsuidevelopment
	//printf(msgToString(msg)); printf("\n");

	//TODO: state

	switch (msg) {
#if 1
	case WM_PAINT://TODO(fran): there's a mysterious pair of scroll buttons that appear when we open too many tabs, being rendered god knows where
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hwnd, &ps);
		RECT tabrc; GetClientRect(hwnd, &tabrc);
		HBRUSH borderbr = unCap_colors.Img;
		int border_thickness = 1;

		int itemcnt = TabCtrl_GetItemCount(hwnd);
		int cursel = TabCtrl_GetCurSel(hwnd);

		RECT itemsrc = tabrc;
		int og_top = itemsrc.top;
		HFONT tabfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
		HFONT oldfont = SelectFont(dc, tabfont); defer{ SelectFont(dc, oldfont); };
		for (int i = 0; i < itemcnt; i++)
		{
			RECT itemrc; TabCtrl_GetItemRect(hwnd, i, &itemrc);
			u8 borderflags = BORDERLEFT | BORDERTOP | BORDERRIGHT;
			if (cursel != i) {
				borderflags |= BORDERBOTTOM;
				itemsrc.top = og_top + max(1, (LONG)((float)RECTHEIGHT(itemrc)*.1f));
			}
			else {
				itemsrc.top = og_top;
			}
			itemsrc.right = itemrc.right;
			itemsrc.bottom = itemrc.bottom;//TODO(fran): find out correct height (tcm_adjustrect?)
			FillRectBorder(dc, itemsrc, border_thickness, borderbr, borderflags);

			int tabcontrol_identifier = GetDlgCtrlID(hwnd);

			DRAWITEMSTRUCT drawitem;
			drawitem.CtlType = ODT_TAB;
			drawitem.CtlID = tabcontrol_identifier;
			drawitem.itemID = i;
			drawitem.itemAction = ODA_DRAWENTIRE | ODA_SELECT;
			drawitem.itemState = ODS_DEFAULT;//TODO(fran): ODS_HOTLIGHT , ODS_SELECTED if cursel==i
			drawitem.hwndItem = hwnd;
			drawitem.hDC = dc;
			RECT drawitemrc = itemsrc;
			InflateRect(&drawitemrc, -1, -1);
			drawitem.rcItem = drawitemrc;
			/*TODO(fran):
			By default, the itemData member of DRAWITEMSTRUCT contains the value of the lParam member of the TCITEM structure.
			However, if you change the amount of application-defined data per tab, itemData contains the address of the data instead.
			You can change the amount of application-defined data per tab by using the TCM_SETITEMEXTRA message.
			*/
			//TCITEM itemnfo; itemnfo.mask = TCIF_PARAM;
			//TabCtrl_GetItem(hwnd, i, &itemnfo); //NOTE: beware, I have a custom tcitem so this writes mem it shouldnt, we gotta check if it is custom or not somehow
			//drawitem.itemData = itemnfo.lParam;
			drawitem.itemData = 0;

			//TODO(fran): I really feel like I should leave clipping to the user, it's very annoying being on the other side (I need to do clipping on the other side for it to look good when I shrink the tab size)
			//Clip the drawing region
			HRGN restoreRegion = CreateRectRgn(0, 0, 0, 0); if (GetClipRgn(dc, restoreRegion) != 1) { DeleteObject(restoreRegion); restoreRegion = NULL; }
			IntersectClipRect(dc, drawitem.rcItem.left, drawitem.rcItem.top, drawitem.rcItem.right, drawitem.rcItem.bottom);

			SendMessage(GetParent(hwnd), WM_DRAWITEM, (WPARAM)tabcontrol_identifier, (LPARAM)&drawitem);

			//Restore old region
			SelectClipRgn(dc, restoreRegion); if (restoreRegion != NULL) DeleteObject(restoreRegion);

			itemsrc.left = itemsrc.right;
		}
		itemsrc.right = tabrc.right;
		u8 topflags = itemcnt ? BORDERBOTTOM : BORDERTOP;
		FillRectBorder(dc, itemsrc, border_thickness, borderbr, topflags);
		RECT rightrc = tabrc; rightrc.top = itemcnt ? itemsrc.bottom : itemsrc.top; FillRectBorder(dc, rightrc, border_thickness, borderbr, BORDERRIGHT);
		RECT botleftrc = tabrc; botleftrc.top = itemcnt ? itemsrc.bottom : botleftrc.top;
		FillRectBorder(dc, botleftrc, border_thickness, borderbr, BORDERLEFT | BORDERBOTTOM);

		EndPaint(hwnd, &ps);
	} break;
#endif
	case WM_NCPAINT://It seems like nothing is done on ncpaint
	{
		return 0;
	} break;
	case TCM_RESIZETABS://TODO(fran): this shouldnt exist here, this goes in uncapcl
	{
		int item_count = (int)SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
		if (item_count != 0) {
			for (int i = 0; i < item_count; i++) {
				CUSTOM_TCITEM item; //TODO(fran): tab control should NOT know about extra item info
				item.tab_info.mask = TCIF_PARAM;
				BOOL ret = (BOOL)SendMessage(hwnd, TCM_GETITEM, i, (LPARAM)&item);
				if (ret) {
					RECT rc;
					GetClientRect(hwnd, &rc);
					SIZE control_size;
					control_size.cx = RECTWIDTH(rc);
					control_size.cy = RECTHEIGHT(rc);
					//SendMessage(item.extra_info.hText, TCM_RESIZE, (WPARAM)&control_size, 0);

					MoveWindow(hwnd, TabOffset.leftOffset, TabOffset.topOffset, control_size.cx - TabOffset.rightOffset - TabOffset.leftOffset, control_size.cy - TabOffset.bottomOffset - TabOffset.topOffset, TRUE);

				}
			}
		}
		break;
	}
	case WM_DROPFILES:
	{
		LRESULT res = SendMessage(GetParent(hwnd), WM_DROPFILES, wParam, lParam);//TODO(fran): should I send this through WM_PARENTNOTIFY or something like that?
		
		return res;
	}
	case WM_ERASEBKGND:
	{
		HDC  hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hwnd, &rc);

		FillRect(hdc, &rc, unCap_colors.ControlBk);
		return 1;
	}
	case WM_CTLCOLOREDIT:
	{
		//if ((HWND)lParam == GetDlgItem(hwnd, EDIT_ID)) //TODO(fran): check we are changing the control we want
		if (TRUE)
		{
			SetBkColor((HDC)wParam, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor((HDC)wParam, ColorFromBrush(unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		}
		else return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	case WM_LBUTTONUP:
	{
		POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		int index = TestCloseButton(hwnd, p);
		if (index != -1) {
			//Delete corresponding tab
			SendMessage(hwnd, TCM_DELETEITEM, index, 0); //After this no other messages are sent and the tab control's index gets set to -1
			//Now we have to send a TCM_SETCURSEL, but in the special case that no other tabs are left we should clean the controls
			int item_count = (int)SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);

			if (item_count == 0) { //Notify when no tabs are left
				int tabcontrol_identifier = GetDlgCtrlID(hwnd);
				NMHDR nmhdr;
				nmhdr.hwndFrom = hwnd;
				nmhdr.idFrom = tabcontrol_identifier;
				nmhdr.code = TCN_SELCHANGE;
				SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)tabcontrol_identifier, (LPARAM)&nmhdr);
			}
			else if (index == item_count) SendMessage(hwnd, TCM_SETCURSEL, index - 1, 0);	//if the deleted tab was the rightmost one change to the one on the left
			else SendMessage(hwnd, TCM_SETCURSEL, index, 0); //otherwise change to the tab that now holds the index that this one did (will select the one on its right)
			return 0;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	case TCM_DELETEITEM:
	{
		//Since right now our controls (text editor) are not children of the particular tab they dont get deleted when that one does, so gotta do it manually
		int index = (int)wParam;

		//TAB_INFO info = GetTabExtraInfo(hwnd/*HACK: only the owner should know about the extra info, this has to be done in the owner*/, index); //TODO(fran): do cleanup on the parent
		//Any hwnd should be destroyed
		//DestroyWindow(info.hText);

		SendMessage(GetParent(hwnd), WM_PARENTNOTIFY, MAKELONG(TCN_DELETEITEM, index), (LPARAM)hwnd );

		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	//case WM_PARENTNOTIFY:
	//{
	//	switch (LOWORD(wParam)) {
	//	case WM_CREATE:
	//	{//Each time a new tab gets created this message is received and the lParam is the new HWND
	//		TESTTAB = (HWND)lParam;
	//		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	//	}
	//	default: return DefSubclassProc(hwnd, Msg, wParam, lParam);
	//	}
	//	return DefSubclassProc(hwnd, Msg, wParam, lParam);
	//}
	case TCM_SETCURSEL: //When sending SETCURSEL, TCN_SELCHANGING and TCN_SELCHANGE are no sent, therefore we have to do everything in one message here
		//NOTE: the defproc for this msg does NOT notify the parent with TCN_SELCHANGING and TCN_SELCHANGE, so yet again, we have to do it
	{
		int tabcontrol_identifier = GetDlgCtrlID(hwnd);
		NMHDR nmhdr;
		nmhdr.hwndFrom = hwnd;
		nmhdr.idFrom = tabcontrol_identifier;
		nmhdr.code = TCN_SELCHANGING;
		SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)tabcontrol_identifier, (LPARAM)&nmhdr);
		LRESULT res = DefSubclassProc(hwnd, msg, wParam, lParam);
		nmhdr.code = TCN_SELCHANGE;
		SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)tabcontrol_identifier, (LPARAM)&nmhdr);
		return res;
	}
	default:
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}