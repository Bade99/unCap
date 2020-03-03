//#include "stdafx.h"
#include "SubtitleWindow.h"
#include "text.h"

SubtitleWindow::SubtitleWindow()
{
}

SubtitleWindow::SubtitleWindow
(HINSTANCE hInstance, HWND parent, std::wstring WindowClassName, HFONT font)
{
	this->main = CreateWindowEx(WS_EX_CONTROLPARENT, WindowClassName.c_str(), nullptr,
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

void SetOptionsComboBox(HWND & hCombo, bool isNew) {//@is there a simpler way to modify the combo box??
	int previous_index = 0;
	if (!isNew) {
		previous_index = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
		for (int number_of_options = 4; number_of_options > 0; number_of_options--)
			//@Remember to change number_of_options if I add more!!
			SendMessageW(hCombo, CB_DELETESTRING, (WPARAM)0, 0);
	}
	SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)brackets_T[global_locale]);
	SendMessageW(hCombo, CB_ADDSTRING, 1, (LPARAM)parenthesis_T[global_locale]);
	SendMessageW(hCombo, CB_ADDSTRING, 2, (LPARAM)braces_T[global_locale]);
	SendMessageW(hCombo, CB_ADDSTRING, 3, (LPARAM)other_T[global_locale]);
	SendMessageW(hCombo, CB_SETCURSEL, previous_index, 0);
}

void EnableOtherChar(bool op) {
	EnableWindow(hInitialText, op);
	EnableWindow(hInitialChar, op);
	EnableWindow(hFinalText, op);
	EnableWindow(hFinalChar, op);
}

void AddControls(HWND hWnd) {

	//hReadFile = CreateWindowExW(0, PROGRESS_CLASS, (LPWSTR)NULL, WS_CHILD
	//	, 10, y_place, 664, 20, hWnd, (HMENU)NULL, NULL, NULL);

	hRemoveCommentWith = CreateWindowW(L"Static", remove_comment_with_T[global_locale], WS_VISIBLE | WS_CHILD
		, 25, y_place + 34, 155, 20, hWnd, NULL, NULL, NULL);


	hOptions = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP
		, 195, y_place + 31/*30*/, 130, 90/*20*/, hWnd, (HMENU)COMBO_BOX, NULL, NULL);
	SetOptionsComboBox(hOptions, TRUE);
	//WCHAR explain_combobox[] = L"Also separates the lines";
	//CreateToolTip(COMBO_BOX, hWnd, explain_combobox);

	hInitialText = CreateWindowW(L"Static", initial_char_T[global_locale], WS_VISIBLE | WS_CHILD | WS_DISABLED
		, 78, y_place + 65, 105, 20, hWnd, NULL, NULL, NULL);

	hInitialChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_place + 64, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hInitialChar, EM_LIMITTEXT, 1, 0);

	hFinalText = CreateWindowW(L"Static", final_char_T[global_locale], WS_VISIBLE | WS_CHILD | WS_DISABLED
		, 78, y_place + 95, 105, 20, hWnd, NULL, NULL, NULL);

	hFinalChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_place + 94, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hFinalChar, EM_LIMITTEXT, 1, 0);

	hRemove = CreateWindowW(L"Button", remove_T[global_locale], WS_VISIBLE | WS_CHILD | WS_TABSTOP
		, 256, y_place + 74, 70, 30, hWnd, (HMENU)REMOVE, NULL, NULL);

	//hRemoveProgress = CreateWindowExW(0,PROGRESS_CLASS,(LPWSTR)NULL, WS_CHILD
	//	, 256, y_place + 74, 70, 30,hWnd, (HMENU)NULL,NULL,NULL);

	//@@@Multiple tabs support?
	hSubs = CreateWindowExW(WS_EX_ACCEPTFILES | WS_EX_TOPMOST, L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER// | WS_DISABLED
		| ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL	//@this last two should appear only when needed
		, 10, y_place + 134, 664 - 50, 588, hWnd, (HMENU)SUBS_WND, NULL, NULL);

	SetWindowSubclass(hSubs, EditCatchDrop, 0, 0); //adds extra handler for catching dropped files

	SendMessageW(hSubs, EM_SETLIMITTEXT, (WPARAM)MAX_TEXT_LENGTH, NULL); //cantidad de caracteres que el usuario puede escribir(asi le dejo editar)
}

void AcceptedFile(wstring filename) {
	//INFO(fran): take what I need from here but do not use acceptedfile

	isAcceptedFile = true;

	//save file dir+name+ext
	accepted_file = filename;

	//save file dir
	accepted_file_dir = filename.substr(0, filename.find_last_of(L"\\") + 1);

	//save file name+ext
	accepted_file_name_with_ext = filename.substr(filename.find_last_of(L"\\") + 1);

	//save file ext
	accepted_file_ext = filename.substr(filename.find_last_of(L".") + 1);

	//show file contents (new thread)
	wstring text;
	PGLP parameters = (PGLP)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLP));
	parameters->text = &text;
	HANDLE hReadTextThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadTextThread, parameters, 0, NULL);



	//show filename in top bar
	SetWindowTextW(hFile, accepted_file.c_str());

	//wait for thread to finish
	WaitForSingleObject(hReadTextThread, INFINITE);
	//delete thread and parameters
	CloseHandle(hReadTextThread);
	HeapFree(GetProcessHeap(), 0, parameters);
	parameters = NULL;

	ProperLineSeparation(text);
	SetWindowTextW(hSubs, text.c_str());

	//enable buttons and text editor
	EnableWindow(hRemove, TRUE);
	//EnableWindow(hSave, TRUE);
	EnableWindow(hSubs, TRUE);
}

void DoInitialPainting() {
	initial_paint = false;
	//@@@check EnumChildWindows so this is simplified in a loop
	SendMessageW(hFile, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemoveCommentWith, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hOptions, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hInitialText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hInitialChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hFinalText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hFinalChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemove, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	//SendMessageW(hSave, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hSubs, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemove, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
}

void SetLocaleW(int locale) {
	global_locale = locale;
	//Menu text
	MENUITEMINFOW menu_setter;
	menu_setter.cbSize = sizeof(MENUITEMINFOW);
	menu_setter.fMask = MIIM_STRING;
	menu_setter.dwTypeData = _wcsdup(file_T[global_locale]);
	SetMenuItemInfoW(hMenu, (UINT_PTR)hFileMenu, FALSE, &menu_setter);
	menu_setter.dwTypeData = _wcsdup(open_T[global_locale]);
	SetMenuItemInfoW(hFileMenu, OPEN_FILE, FALSE, &menu_setter);
	menu_setter.dwTypeData = _wcsdup(save_T[global_locale]);
	SetMenuItemInfoW(hFileMenu, SAVE_FILE, FALSE, &menu_setter);
	menu_setter.dwTypeData = _wcsdup(save_as_T[global_locale]);
	SetMenuItemInfoW(hFileMenu, SAVE_AS_FILE, FALSE, &menu_setter);
	menu_setter.dwTypeData = _wcsdup(backup_T[global_locale]);
	SetMenuItemInfoW(hFileMenu, BACKUP_FILE, FALSE, &menu_setter);
	menu_setter.dwTypeData = _wcsdup(language_T[global_locale]);
	SetMenuItemInfoW(hFileMenu, (UINT_PTR)hFileMenuLang, FALSE, &menu_setter);

	DrawMenuBar(hWnd);

	//Control text
	SetWindowTextW(hRemoveCommentWith, remove_comment_with_T[global_locale]);
	SetOptionsComboBox(hOptions, FALSE);
	SetWindowTextW(hInitialText, initial_char_T[global_locale]);
	SetWindowTextW(hFinalText, final_char_T[global_locale]);
	SetWindowTextW(hRemove, remove_T[global_locale]);

	UpdateWindow(hWnd);

	//@@Alguna forma de alinear los controles por la derecha?? (asi todo queda alineado)

	//notification messages get updated on their own
}

//TODO(fran): do I need this?
void ResizeWindows() {
	GetWindowRect(hWnd, &rect);
	wnd.sizex = rect.right - rect.left;
	wnd.sizey = rect.bottom - rect.top;
	MoveWindow(hFile, 10, y_place, wnd.sizex - 36, 20, TRUE);
	//@No se si actualizar las demas
	MoveWindow(hSubs, 10, y_place + 134, wnd.sizex - 36, wnd.sizey - 212, TRUE);
}

void DoPainting() {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	if (initial_paint) DoInitialPainting();
	EndPaint(hWnd, &ps);
}

/*TODO(fran): ayuda para mi procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:

		AddMenus(hWnd);
		AddControls(hWnd);
		if (Backup_Is_Checked)CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);
		else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);

		break;

	case WM_COMMAND:
	{
		// Analizar los mensajes de mis controles:
		switch (LOWORD(wParam))
		{

		case OPEN_FILE:
		case ID_OPEN:

			ChooseFile(L"srt");

			break;

		case BACKUP_FILE:

			if (GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED)
				CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);
			else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);

			break;

		case COMBO_BOX:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				switch (SendMessageW(hOptions, CB_GETCURSEL, 0, 0)) {
				case 0:case 1:case 2:
					EnableOtherChar(false);//disable buttons
					break;
				case 3:
					EnableOtherChar(true);//enable buttons
					break;
				}
			break;

		case REMOVE:
			if (IsAcceptedFile()) {
				switch (SendMessageW(hOptions, CB_GETCURSEL, 0, 0)) {
				case 0:
					CommentRemoval(L'[', L']');
					break;
				case 1:
					CommentRemoval(L'(', L')');
					break;
				case 2:
					CommentRemoval(L'{', L'}');
					break;
				case 3:
					CustomCommentRemoval();
					break;
				}
			}
			break;

		case SAVE_FILE:
		case ID_SAVE:
			if (IsAcceptedFile()) {
				if (GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED) DoBackup();
				DoSave(accepted_file);
			}
			break;
		case SAVE_AS_FILE:
		case ID_SAVE_AS:
			if (IsAcceptedFile()) {
				DoSaveAs(); //also manages DoBackup internally
			}
			break;

		case ENGLISH:
			SetLocaleW(ENGLISH);
			break;
		case SPANISH:
			SetLocaleW(SPANISH);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_SIZE:
		ResizeWindows();
		break;
	case WM_PAINT:
	{
		DoPainting();
	}
	break;
	case WM_DESTROY:
		CreateInfoFile();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
*/