#pragma once
#include <Windows.h>
#include <CommCtrl.h>

#define UNCAPNOTIFY_SETTEXTDURATION (WM_USER+52) //Sets duration of text in a control before it's hidden. wParam=(UINT)duration in milliseconds ; lParam = NOT USED

/// <summary>
/// Shows and hides text on a control that receives WM_SETTEXT messages
/// <para>You can set the duration of text on the control by sending</para>
/// <para>UNCAP_SETTEXTDURATION message with wParam=(UINT)duration in milliseconds</para>
/// <para>When the desired time has elapsed the control is hidden</para>
/// <para>If WM_SETTEXT is sent with an empty string then the control is hidden</para>
/// <para>If UNCAP_SETTEXTDURATION is sent with wParam=0 then the text will not be hidden</para>
/// </summary>
/// <param name="uIdSubclass">Not used</param>
/// <param name="dwRefData">Not used</param>
LRESULT CALLBACK NotifyProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//INFO: for this procedure and its hwnds we are going to try the SetProp method for storing info in the hwnd
	TCHAR text_duration[] = TEXT("TextDuration_unCap");
	switch (Msg)
	{
	case UNCAPNOTIFY_SETTEXTDURATION:
	{
		SetProp(hwnd, text_duration, (HANDLE)wParam);
		return TRUE;
	}
	case WM_TIMER: {
		KillTimer(hwnd, 1);//Stop timer, otherwise it keeps sending messages
		ShowWindow(hwnd, SW_HIDE);
		break;
	}
	case WM_SETTEXT:
	{
		WCHAR* text = (WCHAR*)lParam;

		if (text[0] != L'\0') { //We have new text to process

			//Calculate minimum size of the control for the new text. TODO(fran): should we do this or just say that the control is as long as it can be?

			//...

			//Start Timer for text removal: this messages will be shown for a little period of time before the control is hidden again
			UINT msgDuration = (UINT)(UINT_PTR)GetProp(hwnd, text_duration);	//Retrieve the user defined duration of text on screen
																	//If this value was not set then the timer is not used and the text remains on screen
			if (msgDuration)
				SetTimer(hwnd, 1, msgDuration, NULL); //By always setting the second param to 1 we are going to override any existing timer

			ShowWindow(hwnd, SW_SHOW); //Show the control with its new text
		}
		else { //We want to hide the control and clear whatever text is in it
			ShowWindow(hwnd, SW_HIDE);
		}

		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_DESTROY:
	{
		//Cleanup
		RemoveProp(hwnd, text_duration);
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	default: return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	return 0;
}