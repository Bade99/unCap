#pragma once
#include <Windows.h>
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "unCap_Core.h"
#include "LANGUAGE_MANAGER.h"
#include "fmt/format.h"
#include "resource.h"
#include "unCap_combobox.h"
#include "unCap_uncapnc.h"
#include <filesystem>
#include <fstream>
#include <propvarutil.h>
#include <propkey.h>
#include "Open_Save_FileHandler.h"
#include "unCap_tab.h"
#include "unCap_notify.h"
#include "unCap_edit.h"
#include <Commdlg.h> //OPENFILENAME
#include <Shellapi.h> //HDROP

#define OPEN_FILE 11
#define COMBO_BOX 12
#define SAVE_AS_FILE 13
#define REMOVE 14
#define SAVE_FILE 15
#define BACKUP_FILE 16
#define SUBS_WND 17
#define TABCONTROL 18
#define TIMEDMESSAGES 19
#define INITIALFINALCHAR 20
#define SEPARATOR1 21
#define SEPARATOR2 22

#define MAX_TEXT_LENGTH 288000 //Edit controls have their text limit set by default at 32767, we need to change that

constexpr COMDLG_FILTERSPEC c_rgSaveTypes[] = //TODO(fran): this should go in resource file, but it isnt really necessary
{
{ L"Srt (*.srt)",       L"*.srt" },
{ L"Advanced SSA (*.ass)",       L"*.ass" },
{ L"Sub Station Alpha (*.ssa)",       L"*.ssa" },
{ L"Txt (*.txt)",       L"*.txt" },
{ L"All Documents (*.*)",         L"*.*" }
};

//NOTE: extension format should be ".txt"
//NOTE: the function that needs this (SetFileTypeIndex) uses 1 based indexing, I know, insane, therefore we return a 1 based index too
int getExtensionIdx(int types_cnt, const COMDLG_FILTERSPEC* save_types, std::wstring extension) {
	//compare against valid extensions
	for (int i = 0; i < types_cnt; i++) {
		if (!extension.compare(save_types[i].pszSpec + 1)) {//TODO(fran): case insensitive comparison
			return i+1;
		}
	}
	//if not found look for "All documents" which should be *
	std::wstring cmp{ L".*" };
	for (int i = 0; i < types_cnt; i++) {
		if (!cmp.compare(save_types[i].pszSpec + 1)) {//TODO(fran): case insensitive comparison
			return i+1;
		}
	}
	//if not found return 1
	return 1;
}

constexpr int y_pad = 10;

constexpr TCHAR appName[] = _t("unCap"); //Program name, to be displayed on the title of windows

BOOL SetCurrentTabTitle(HWND tab, std::wstring title) {
	int index = (int)SendMessage(tab, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM item;
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = (LPWSTR)title.c_str();
		int ret = (int)SendMessage(tab, TCM_SETITEM, index, (LPARAM)&item);
		return ret;
	}
	else return index;
}

TAB_INFO GetTabExtraInfo(HWND tab, int index) {
	if (index != -1) {
		CUSTOM_TCITEM item;
		item.tab_info.mask = TCIF_PARAM;
		BOOL ret = (BOOL)SendMessage(tab, TCM_GETITEM, index, (LPARAM)&item);
		if (ret) {
			return item.extra_info;
		}
	}
	TAB_INFO invalid = { 0 };
	return invalid;
}

//Returns the current tab's text info or a TAB_INFO struct with everything set to 0
TAB_INFO GetCurrentTabExtraInfo(HWND tab) {
	int index = (int)SendMessage(tab, TCM_GETCURSEL, 0, 0);

	return GetTabExtraInfo(tab, index);
}

BOOL SetTabExtraInfo(HWND tab, int index, const TAB_INFO& text_data) {
	CUSTOM_TCITEM item;
	item.tab_info.mask = TCIF_PARAM;
	item.extra_info = text_data;

	int ret = (int)SendMessage(tab, TCM_SETITEM, index, (LPARAM)&item);
	return ret;
}

//Adds a new item to the tab control, inside of which will be an edit control
//·Populate text_data with the values you need except for any HWND
//Returns position of the new Tab Item or -1 if failed
int AddTab(HWND TabControl, int position, LPWSTR TabName, TAB_INFO text_data) { //INFO: TabName only needs to be valid during the execution of this function

	RECT tab_rec, text_rec;
	GetClientRect(TabControl, &tab_rec);
	text_rec.left = TabOffset.leftOffset;
	text_rec.top = TabOffset.topOffset;
	text_rec.right = tab_rec.right - TabOffset.rightOffset;
	text_rec.bottom = tab_rec.bottom - TabOffset.bottomOffset;

	HWND TextControl = CreateWindowExW(NULL, L"Edit", NULL, WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_CLIPCHILDREN //| WS_VSCROLL | WS_HSCROLL //|WS_VISIBLE
		, text_rec.left, text_rec.top, text_rec.right - text_rec.left, text_rec.bottom - text_rec.top
		, TabControl
		, NULL, NULL, NULL);

	SendMessageW(TextControl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

	SendMessageW(TextControl, EM_SETLIMITTEXT, (WPARAM)MAX_TEXT_LENGTH, NULL); //Change default text limit of 32767 to a bigger value

	SetWindowSubclass(TextControl, EditProc, 0, (DWORD_PTR)calloc(1, sizeof(EditProcState)));

	HWND VScrollControl = CreateWindowExW(NULL, unCap_wndclass_scrollbar, NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, TextControl, NULL, NULL, NULL);
	SendMessage(VScrollControl, U_SB_SET_PLACEMENT, (WPARAM)ScrollBarPlacement::right, 0);

	SendMessageW(TextControl, EM_SETVSCROLL, (WPARAM)VScrollControl, 0);

	//TAB_INFO* text_info = (TAB_INFO*)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE| HEAP_ZERO_MEMORY,sizeof(TAB_INFO));
	//Assert(text_info);
	//text_info->hText = TextControl;

	CUSTOM_TCITEM newItem;
	newItem.tab_info.mask = TCIF_TEXT | TCIF_PARAM;
	newItem.tab_info.pszText = TabName;
	//newItem.tab_info.cchTextMax = (wcslen(TabName)+1)*sizeof(*TabName); I think this is wrong and should be in chars/wchars not bytes
	newItem.tab_info.iImage = -1;
	newItem.extra_info = text_data;
	newItem.extra_info.hText = TextControl;

	return (int)SendMessage(TabControl, TCM_INSERTITEM, position, (LPARAM)&newItem);
}

std::wstring GetTabTitle(HWND tab, int index) {
	if (index != -1) {
		CUSTOM_TCITEM item;
		WCHAR title[200];
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = title;
		item.tab_info.cchTextMax = ARRAYSIZE(title);
		BOOL ret = (BOOL)SendMessage(tab, TCM_GETITEM, index, (LPARAM)&item);
		if (ret) {
			return title;
		}
	}
	return L"";
}

struct unCapClSettings {

#define foreach_unCapClSettings_member(op) \
		op(bool, is_backup_enabled, true) \
		op(str, last_dir) \
		op(RECT, rc,200,200,700,800 ) \

	//TODO(fran): last_dir wont work for ansi cause the function that uses it is only unicode, problem is I need serialization and that is encoding dependent, +1 for binary serializaiton

	foreach_unCapClSettings_member(_generate_member);

	_generate_default_struct_serialize(foreach_unCapClSettings_member);

	_generate_default_struct_deserialize(foreach_unCapClSettings_member);

};

_add_struct_to_serialization_namespace(unCapClSettings)

struct unCapClProcState {
	HWND wnd;
	HWND nc_parent;

	struct {//menu related
		HMENU menu;
		HMENU menu_file;
		HMENU menu_lang;
	};

	struct unCapClControls {
		HWND tab, combo_commentmarker, edit_commentbegin, edit_commentend, static_commentbegin, static_commentend, static_removecomment, button_removecomment, static_notify;
	}controls;

	unCapClSettings* settings;

	bool is_commentmarker_other;//TODO(fran): remove once I paint my own static controls
};

TCHAR unCap_wndclass_uncap_cl[] = TEXT("unCap_wndclass_uncap_cl"); //Client uncap

void DoBackup(const TCHAR* filepath) { //TODO(fran): I feel backup is pretty pointless, more confusing than anything else, REMOVE once I implement undo

	std::wstring new_file = filepath;
	size_t found = new_file.find_last_of(L"\\") + 1;
	Assert(found != std::wstring::npos);
	new_file.insert(found, L"SDH_");//TODO(fran): add SDH to resource file?

	if (!CopyFileW(filepath, new_file.c_str(), FALSE)) MessageBoxW(NULL, L"The filename is probably too large", L"TODO(fran): Fix the program!", MB_ICONERROR);
}

//TODO(fran): DoSave should receive the string, not have to call the HWND
//TODO(fran): NEVER should you write straight into the file already existing, write to a secondary and then rename
std::wstring DoSave(HWND textControl, std::wstring save_file, TXT_ENCODING encoding) { //got to pass the encoding to the function too, and do proper conversion
	printf("Encoding Written: %d\n", encoding);
	//https://docs.microsoft.com/es-es/windows/desktop/api/commdlg/ns-commdlg-tagofna
	//http://www.winprog.org/tutorial/app_two.html

	//TODO(fran):	WideCharToMultiByte  :: Maps a UTF-16 (wide character) string to a new character string. 
	//				The new character string is not necessarily from a multibyte character set.
	std::wstring res;
	std::wofstream new_file(save_file, std::ios::binary);
	if (new_file.is_open()) {
		switch (encoding) {
		case TXT_ENCODING::UTF8: new_file.imbue(std::locale(new_file.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>)); break;//TODO(fran): I dont think I should generate the header, if I dont then Im free to treat it as both ansi and utf8 and remove the need for ansi
		case TXT_ENCODING::UTF16: new_file.imbue(std::locale(new_file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::generate_header>)); break;
		//TODO(fran): what to do with ansi?
		default:Assert(0);
		}
		int text_length = GetWindowTextLengthW(textControl)+1;
		std::wstring text(text_length, L'\0');
		GetWindowTextW(textControl, &text[0], text_length);
		//NOTE: the text control forcefully appends a null terminator, we gotta get rid of it
		if (!text.empty() && text.back() == L'\0') text.erase(text.end() - 1);
		new_file << text;
		new_file.close();

		res = save_file;

	}
	else MessageBoxW(NULL, RCS(LANG_SAVE_ERROR), RCS(LANG_ERROR), MB_ICONEXCLAMATION);
	return res;
}

//si hay /n antes del corchete que lo borre tmb (y /r tmb)
void UNCAPCL_comment_removal(unCapClProcState* state, HWND hText, FILE_FORMAT format, WCHAR start, WCHAR end) {//TODO(fran): Should we give the option to detect for more than one char?
	int text_length = GetWindowTextLengthW(hText) + 1;
	std::wstring text(text_length, L'\0');
	GetWindowTextW(hText, &text[0], text_length);
	size_t comment_count = 0;
	switch (format) {
	case FILE_FORMAT::SRT:
		comment_count = CommentRemovalSRT(text, start, end); break;
	case FILE_FORMAT::SSA:
		comment_count = CommentRemovalSSA(text, start, end); break;
		//TODO(fran): should add default:?
	}

	if (comment_count) {
		SetWindowTextW(hText, text.c_str()); //TODO(fran): add message for number of comments removed
		std::wstring comment_count_str = fmt::format(RS(LANG_CONTROL_TIMEDMESSAGES), comment_count);
		SetWindowText(state->controls.static_notify, comment_count_str.c_str());
	}
	else {
		SendMessage(state->controls.static_notify, WM_SETTEXT, 0, (LPARAM)RCS(LANG_COMMENTREMOVAL_NOTHINGTOREMOVE));
	}
}

struct GetSaveAsStrResult { std::wstring savefile; TXT_ENCODING encoding; };
GetSaveAsStrResult GetSaveAsStr(std::wstring default_folder,std::wstring default_ext, TXT_ENCODING default_encoding) //https://msdn.microsoft.com/en-us/library/windows/desktop/bb776913(v=vs.85).aspx
{
	GetSaveAsStrResult res;
	// CoCreate the File Open Dialog object.
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);//single threaded, @multi?
	IFileSaveDialog *pfsd = NULL;
	hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfsd));

	if (SUCCEEDED(hr))
	{

		////Add items to the Places Bar
		////-
		//IKnownFolderManager *pkfm = NULL;

		//hr = CoCreateInstance(CLSID_KnownFolderManager, NULL,
		//	CLSCTX_INPROC_SERVER,
		//	IID_PPV_ARGS(&pkfm));

		//// Offer some known folders.
		////https://docs.microsoft.com/en-us/windows/desktop/shell/knownfolderid
		//IKnownFolder *pKnownFolder = NULL;
		//hr = pkfm->GetFolder(FOLDERID_PublicMusic, &pKnownFolder);

		//// File Dialog APIs need an IShellItem that represents the location.
		//IShellItem *psi = NULL;
		//hr = pKnownFolder->GetShellItem(0, IID_PPV_ARGS(&psi));

		//// Add the place to the bottom of default list in Common File Dialog.
		//hr = pfsd->AddPlace(psi, FDAP_BOTTOM);
		////-


		// Create an event handling object, and hook it up to the dialog.(for extra controls)
		IFileDialogEvents   *pfde = NULL;
		DWORD               dwCookie = 0;
		hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));

		if (SUCCEEDED(hr))
		{
			// Hook up the event handler.
			hr = pfsd->Advise(pfde, &dwCookie);

			// Set up a Customization.
			//-
			IFileDialogCustomize *pfdc = NULL;
			hr = pfsd->QueryInterface(IID_PPV_ARGS(&pfdc));

			// Create a Visual Group.
			hr = pfdc->StartVisualGroup(CONTROL_GROUP, L"Encoding");

			// Add a radio-button list.
			hr = pfdc->AddRadioButtonList(RADIOBTNLIST_ENCODINGS);

			// Set the state of the added radio-button list.
			hr = pfdc->SetControlState(RADIOBTNLIST_ENCODINGS,
				CDCS_VISIBLE | CDCS_ENABLED);

			// Add individual buttons to the radio-button list.
			hr = pfdc->AddControlItem(RADIOBTNLIST_ENCODINGS,
				RADIOBTNUTF8,
				L"UTF-8");

			hr = pfdc->AddControlItem(RADIOBTNLIST_ENCODINGS,
				RADIOBTNUTF16,
				L"UTF-16");

			hr = pfdc->AddControlItem(RADIOBTNLIST_ENCODINGS,
				RADIOBTNANSI,
				L"ANSI");

			// Set the default selection
			{
				DWORD encoding_selected_item;
				switch (default_encoding) {
				case TXT_ENCODING::UTF8: encoding_selected_item = RADIOBTNUTF8; break;
				case TXT_ENCODING::UTF16: encoding_selected_item = RADIOBTNUTF16; break;
				case TXT_ENCODING::ANSI: encoding_selected_item = RADIOBTNANSI; break;
				default: Assert(0);
				}
				hr = pfdc->SetSelectedControlItem(RADIOBTNLIST_ENCODINGS, encoding_selected_item);
			}

			// End the visual group.
			pfdc->EndVisualGroup();

			//-- Set up another customization(only for open dialog)

			//The user's choice can be verified after the dialog returns from the Show 
			//method as you would for a ComboBox, or it can verified as part of the 
			//handling by IFileDialogEvents::OnFileOk.

			//hr = pfdc->EnableOpenDropDown(OPENCHOICES);

			//hr = pfdc->AddControlItem(OPENCHOICES, OPEN, L"&Open");

			//hr = pfdc->AddControlItem(OPENCHOICES,
			//	OPEN_AS_READONLY,
			//	L"Open as &read-only");

			//--

			pfdc->Release();

			//-


			// Set the options on the dialog.
			//-
			//DWORD dwFlags;
			//// Before setting, always get the options first in order not to override existing options.
			//hr = pfsd->GetOptions(&dwFlags);
			//// In this case, get shell items only for file system items.
			//hr = pfsd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
			//multiselect(only for open dialog): hr = pfd->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
			//-


			// Set the file types to display.
			hr = pfsd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);

			// Set default file index @@create function to translate file type to INDEX_...
			hr = pfsd->SetFileTypeIndex(getExtensionIdx(ARRAYSIZE(c_rgSaveTypes),c_rgSaveTypes,default_ext));

			// Set def file type @@ again needs function
			hr = pfsd->SetDefaultExtension(default_ext.c_str());//L"doc;docx"
			
			if (default_folder != L"") {
				IShellItem *defFolder;//@@shouldn't this be IShellFolder?
				//does accepted_file_dir need to be converted to UTF-16 ?
				hr = SHCreateItemFromParsingName(default_folder.c_str(), NULL, IID_IShellItem, (void**)&defFolder);
				// Set def folder
				hr = pfsd->SetDefaultFolder(defFolder);

				defFolder->Release();
			}


			//State Persistence(make last visited folder change depending on which dialog is executing)
			//pfsd->SetClientGuid(REFGUID guid);


			IPropertyStore *pps = NULL;
			// The InMemory Property Store is a Property Store that is
			// kept in the memory instead of persisted in a file stream.
			hr = PSCreateMemoryPropertyStore(IID_PPV_ARGS(&pps));
			PROPVARIANT propvarValue = {};
			//initializes the properties and gives them a name
			hr = InitPropVariantFromString(L"Props", &propvarValue);
			// Set the value to the property store of the item.
			hr = pps->SetValue(PKEY_Keywords, propvarValue);
			// Commit does the actual writing back to the in memory store.
			hr = pps->Commit();

			// Hand these properties to the File Dialog.
			hr = pfsd->SetCollectedProperties(NULL, TRUE);

			// Set iPropertyStore properties
			hr = pfsd->SetProperties(pps);

			PropVariantClear(&propvarValue);
			pps->Release();

			if (FAILED(hr))
			{
				// Unadvise here in case we encounter failures before we get a chance to show the dialog.
				pfsd->Unadvise(dwCookie);
			}
			pfde->Release(); //releases the extra controls manager
		}
		if (SUCCEEDED(hr))
		{
			// Now show the dialog.
			hr = pfsd->Show(NULL);

			if (SUCCEEDED(hr))//@here goes the code to handle the file received
			{
				//https://docs.microsoft.com/en-us/windows/desktop/api/shobjidl_core/nn-shobjidl_core-ifiledialog
				IShellItem *file_handler; SFGAOF
				hr = pfsd->GetResult(&file_handler);
				LPWSTR filename_holder = NULL;
				hr = file_handler->GetDisplayName(SIGDN_FILESYSPATH, &filename_holder); //@check other options for the flag
				res.savefile = filename_holder;

				CoTaskMemFree(filename_holder);
				file_handler->Release();

				//Check the chosen encoding
				IFileDialogCustomize *custom = NULL;
				hr = pfsd->QueryInterface(IID_PPV_ARGS(&custom));
				DWORD selected_encoding;
				custom->GetSelectedControlItem(RADIOBTNLIST_ENCODINGS,&selected_encoding);
				switch (selected_encoding) {
				case (DWORD)RADIOBTNUTF8: res.encoding = TXT_ENCODING::UTF8; break;
				case (DWORD)RADIOBTNUTF16:res.encoding = TXT_ENCODING::UTF16; break;
				case (DWORD)RADIOBTNANSI: res.encoding = TXT_ENCODING::ANSI; break;
				default:Assert(0);
				}
			}
			// Unhook the event handler.
			pfsd->Unadvise(dwCookie);
		}

		//Places Bar releases
		//psi->Release();
		//pKnownFolder->Release();
		//pkfm->Release();
		//


		pfsd->Release();
		CoUninitialize();
	}
	return res;
}

void UNCAPCL_ResizeWindows(unCapClProcState* state) {
	RECT rect;
	GetClientRect(state->wnd, &rect);
	//MoveWindow(hFile, 10, y_pad, RECTWIDTH(rect) - 36, 20, TRUE);
	MoveWindow(state->controls.static_notify, 256 + 70 + 10, y_pad + 44, RECTWIDTH(rect) - (256 + 70 + 4) - 36, 30, TRUE);
	//@No se si actualizar las demas
	int txtcont_top = y_pad + 104;
	MoveWindow(state->controls.tab, 10, txtcont_top, RECTWIDTH(rect) - 20, RECTHEIGHT(rect) - txtcont_top - 10, TRUE);
	SendMessage(state->controls.tab, TCM_RESIZETABS, 0, 0);
}

void UNCAPCL_custom_comment_removal(unCapClProcState* state, HWND hText, FILE_FORMAT format) {
	WCHAR temp[2] = { 0 };
	GetWindowTextW(state->controls.edit_commentbegin, temp, 2); //tambien la funcion devuelve 0 si no agarra nada, podemos agregar ese check
	WCHAR start = temp[0];//vale '\0' si no tiene nada
	GetWindowTextW(state->controls.edit_commentend, temp, 2);
	WCHAR end = temp[0];

	if (start != L'\0' && end != L'\0') UNCAPCL_comment_removal(state, hText, format, start, end);
	else SendMessage(state->controls.static_notify, WM_SETTEXT, 0, (LPARAM)RCS(LANG_COMMENTREMOVAL_ADDCHARS));
}

void SetText_file_app(HWND wnd, const TCHAR* new_filename, const TCHAR* new_appname) {
	if (new_filename == NULL || *new_filename == NULL) {
		SetWindowTextW(wnd, new_appname);
	}
	else {
		std::wstring title_window = new_filename;
		title_window += L" - ";
		title_window += new_appname;
		SetWindowTextW(wnd, title_window.c_str());
	}
}


int UNCAPCL_enable_tab(unCapClProcState* state, const TAB_INFO& text_data) {
	ShowWindow(text_data.hText, SW_SHOW);

	SetText_file_app(state->nc_parent, text_data.filePath, appName);

	SendMessage(state->controls.combo_commentmarker, CB_SETCURSEL, text_data.commentType, 0);

	WCHAR temp[2] = { 0 };
	temp[1] = L'\0';

	temp[0] = text_data.initialChar;
	SetWindowText(state->controls.edit_commentbegin, temp);

	temp[0] = text_data.finalChar;
	SetWindowText(state->controls.edit_commentend, temp);

	return 1;//TODO(fran): proper return
}

void UNCAPCL_add_controls(unCapClProcState* state, HINSTANCE hInstance) {
	//hFile = CreateWindowW(L"Static", L"", /*WS_VISIBLE |*/ WS_CHILD | WS_BORDER//| SS_WHITERECT
	//		| ES_AUTOHSCROLL | SS_CENTERIMAGE
	//		, 10, y_pad, 664, 20, hwnd, NULL, NULL, NULL);
	//ChangeWindowMessageFilterEx(hFile, WM_DROPFILES, MSGFLT_ADD,NULL); y mas que habia

	//TODO(fran): add more interesting progress bar
	//hReadFile = CreateWindowExW(0, PROGRESS_CLASS, (LPWSTR)NULL, WS_CHILD
	//	, 10, y_pad, 664, 20, hwnd, (HMENU)NULL, NULL, NULL);

	state->controls.static_removecomment = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE
		, 25, y_pad + 3, 155, 20, state->wnd, NULL, NULL, NULL);
	AWT(state->controls.static_removecomment, LANG_CONTROL_REMOVECOMMENTWITH);

	state->controls.combo_commentmarker = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP
		, 195, y_pad, 130, 90/*20*/, state->wnd, (HMENU)COMBO_BOX, hInstance, NULL);
	ACT(state->controls.combo_commentmarker, COMMENT_TYPE::brackets, LANG_CONTROL_CHAROPTIONS_BRACKETS);
	ACT(state->controls.combo_commentmarker, COMMENT_TYPE::parenthesis, LANG_CONTROL_CHAROPTIONS_PARENTHESIS);
	ACT(state->controls.combo_commentmarker, COMMENT_TYPE::braces, LANG_CONTROL_CHAROPTIONS_BRACES);
	ACT(state->controls.combo_commentmarker, COMMENT_TYPE::other, LANG_CONTROL_CHAROPTIONS_OTHER);
	SendMessage(state->controls.combo_commentmarker, CB_SETCURSEL, 0, 0);
	SetWindowSubclass(state->controls.combo_commentmarker, ComboProc, 0, 0);
	HBITMAP dropdown = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(UNCAP_BMP_DROPDOWN));
	SendMessage(state->controls.combo_commentmarker, CB_SETDROPDOWNIMG, (WPARAM)dropdown, 0);

	//WCHAR explain_combobox[] = L"Also separates the lines";
	//CreateToolTip(COMBO_BOX, hwnd, explain_combobox);

	state->controls.static_commentbegin = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE //| WS_DISABLED 
		, 78, y_pad + 35, 105, 20, state->wnd, (HMENU)INITIALFINALCHAR, NULL, NULL);
	AWT(state->controls.static_commentbegin, LANG_CONTROL_INITIALCHAR);

	state->controls.edit_commentbegin = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_pad + 34, 20, 21, state->wnd, NULL, NULL, NULL);
	SendMessageW(state->controls.edit_commentbegin, EM_LIMITTEXT, 1, 0);

	state->controls.static_commentend = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE //| WS_DISABLED
		, 78, y_pad + 65, 105, 20, state->wnd, (HMENU)INITIALFINALCHAR, NULL, NULL);
	AWT(state->controls.static_commentend, LANG_CONTROL_FINALCHAR);

	state->controls.edit_commentend = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_pad + 64, 20, 21, state->wnd, NULL, NULL, NULL);
	SendMessageW(state->controls.edit_commentend, EM_LIMITTEXT, 1, 0);

	state->controls.button_removecomment = CreateWindowW(unCap_wndclass_button, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP
		, 256, y_pad + 44, 70, 30, state->wnd, (HMENU)REMOVE, NULL, NULL);
	AWT(state->controls.button_removecomment, LANG_CONTROL_REMOVE);
	UNCAPBTN_set_brushes(state->controls.button_removecomment, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

	state->controls.static_notify = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE
		, 256 + 70 + 10, y_pad + 44, 245, 30, state->wnd, (HMENU)TIMEDMESSAGES, NULL, NULL); //INFO: width will be as large as needed to show the needed string
	SetWindowSubclass(state->controls.static_notify, NotifyProc, 0, 0);
	SendMessage(state->controls.static_notify, UNCAPNOTIFY_SETTEXTDURATION, 5000, 0);

	//hRemoveProgress = CreateWindowExW(0,PROGRESS_CLASS,(LPWSTR)NULL, WS_CHILD
	//	, 256, y_pad + 74, 70, 30,state->wnd, (HMENU)NULL,NULL,NULL);

	//TODO(fran): init common controls for tab control
	//INFO:https://docs.microsoft.com/en-us/windows/win32/controls/tab-controls
	state->controls.tab = CreateWindowExW(WS_EX_ACCEPTFILES, WC_TABCONTROL, NULL, WS_CHILD | WS_VISIBLE | TCS_FORCELABELLEFT | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH
		, 10, y_pad + 104, 664 - 50, 618, state->wnd, (HMENU)TABCONTROL, NULL, NULL);
	TabCtrl_SetItemExtra(state->controls.tab, sizeof(TAB_INFO));
	int tabWidth = 100, tabHeight = 20;
	SendMessage(state->controls.tab, TCM_SETITEMSIZE, 0, MAKELONG(tabWidth, tabHeight));
	SetWindowSubclass(state->controls.tab, TabProc, 0, 0);

	//SendMessage(hFile, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE); //TODO(fran): for loop with union struct
	SendMessage(state->controls.static_removecomment, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.combo_commentmarker, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.static_commentbegin, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.edit_commentbegin, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.static_commentend, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.edit_commentend, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.button_removecomment, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.tab, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	SendMessage(state->controls.static_notify, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
}


void AcceptedFile(unCapClProcState* state, std::wstring filename) {
	//TODO(fran): multithreading for multiple files?

	//save file dir
	state->settings->last_dir = filename.substr(0, filename.find_last_of(L"\\") + 1);

	//save file name+ext
	std::wstring accepted_file_name_with_ext = filename.substr(filename.find_last_of(L"\\") + 1);


	TAB_INFO text_data;
	wcsncpy_s(text_data.filePath, filename.c_str(), sizeof(text_data.filePath) / sizeof(text_data.filePath[0]));

	int pos = TAB_get_next_pos(state->controls.tab);//NOTE: careful with multithreading here

	ReadTextResult text_res = ReadText(filename);

	text_data.fileEncoding = text_res.encoding;

	text_data.commentType = GetCommentType(text_res.text);

	text_data.fileFormat = GetFileFormat(filename);

	ProperLineSeparation(text_res.text);

	int res = AddTab(state->controls.tab, pos, (LPWSTR)accepted_file_name_with_ext.c_str(), text_data);
	Assert(res != -1);

	TAB_INFO new_text_data = GetTabExtraInfo(state->controls.tab, res);

	SetWindowTextW(new_text_data.hText, text_res.text.c_str());

	//enable buttons and text editor
	EnableWindow(state->controls.button_removecomment, TRUE); //TODO(fran): this could also be a saved parameter

	TAB_change_selected(state->controls.tab, res);
}

inline BOOL isMultiFile(LPWSTR file, WORD offsetToFirstFile) {
	return file[max(offsetToFirstFile - 1, 0)] == L'\0';//TODO(fran): is this max useful? if [0] is not valid I think it will fail anyway
}

void UNCAPCL_choose_file(unCapClProcState* state, std::wstring ext) {
	//TODO(fran): support for folder selection?
	//TODO(fran): check for mem-leaks, the moment I select the open-file menu 10MB of ram are assigned for some reason
	WCHAR name[MAX_PATH]; //TODO(fran): I dont think this is big enough
	name[0] = L'\0';
	OPENFILENAMEW new_file;
	ZeroMemory(&new_file, sizeof(OPENFILENAMEW));

	new_file.lStructSize = sizeof(OPENFILENAMEW);
	new_file.hwndOwner = NULL;
	new_file.lpstrFile = name; //used for the default file selected
	new_file.nMaxFile = MAX_PATH; //TODO(fran): this will not be enough with multi-file (Pd. dont forget lpstrFile and name[])
	std::wstring filter = RS(LANG_CHOOSEFILE_FILTER_ALL_1) + L'\0' + RS(LANG_CHOOSEFILE_FILTER_ALL_2) + L'\0'
		+ RS(LANG_CHOOSEFILE_FILTER_SRT_1) + L'\0' + RS(LANG_CHOOSEFILE_FILTER_SRT_2) + L'\0' + L'\0';

	new_file.lpstrFilter = filter.c_str();

	new_file.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST; //TODO(fran): check other useful flags

	new_file.lpstrDefExt = ext.c_str();
	if (state->settings->last_dir != L"\0") new_file.lpstrInitialDir = state->settings->last_dir.c_str();
	else new_file.lpstrInitialDir = NULL; //@este if genera bug si initialDir tiene data y abris el buscador pero no agarras nada

	if (GetOpenFileNameW(&new_file)) { //si el usuario elige un archivo

		//TODO(fran): I know that for multi-file the string is terminated with two NULLs, from my tests when it is single file this also happens, can I guarantee
		//this always happens? Meanwhile I'll use IsMultiFile
		BOOL multifile = isMultiFile(new_file.lpstrFile, new_file.nFileOffset);

		//std::vector<std::wstring> files;

		if (multifile) {
			for (int i = new_file.nFileOffset - 1;; i++) {
				if (new_file.lpstrFile[i] == L'\0') {
					if (new_file.lpstrFile[i + 1] == L'\0') break;
					else {
						std::wstring onefile = new_file.lpstrFile;//Will only take data until the first \0 it finds
						onefile += L'\\';
						onefile += &new_file.lpstrFile[i + 1];
						//files.push_back(onefile);

						if (std::filesystem::exists(onefile)) AcceptedFile(state, onefile);
						else  MessageBoxW(NULL, RCS(LANG_CHOOSEFILE_ERROR_NONEXISTENT), RCS(LANG_ERROR), MB_ICONERROR);

						i += (int)wcslen(&new_file.lpstrFile[i + 1]);//TODO(fran): check this is actually faster
					}
				}
			}
		}
		else {
			//files.push_back(new_file.lpstrFile);
			if (std::filesystem::exists(new_file.lpstrFile)) AcceptedFile(state, new_file.lpstrFile);
			else  MessageBoxW(NULL, RCS(LANG_CHOOSEFILE_ERROR_NONEXISTENT), RCS(LANG_ERROR), MB_ICONERROR);
		}

	}
}

void UNCAPCL_save_settings(unCapClProcState* state) {
	//std::wstring info_save_file = GetInfoPath();

	RECT rc; GetWindowRect(state->wnd, &rc);// MapWindowPoints(state->wnd, HWND_DESKTOP, (LPPOINT)&rc, 2); //TODO(fran): can I simply use GetWindowRect?

	state->settings->rc = rc;
	//NOTE: the rest of settings are already being updated, and this one should too
}

void UNCAPCL_add_menus(unCapClProcState* state) {
	//NOTE: each menu gets its parent HMENU stored in the itemData part of the struct
	LANGUAGE_MANAGER::Instance().AddMenuDrawingHwnd(state->wnd);

	//INFO: the top 32 bits of an HMENU are random each execution, in a way, they can actually get set to FFFFFFFF or to 00000000, so if you're gonna check two of those you better make sure you cut the top part in BOTH

	HMENU hMenu = CreateMenu(); state->menu = hMenu;
	HMENU hFileMenu = CreateMenu(); state->menu_file = hFileMenu;
	HMENU hFileMenuLang = CreateMenu(); state->menu_lang = hFileMenuLang;
	AppendMenuW(hMenu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)hFileMenu, (LPCWSTR)hMenu);
	AMT(hMenu, (UINT_PTR)hFileMenu, LANG_MENU_FILE);

	AppendMenuW(hFileMenu, MF_STRING | MF_OWNERDRAW, OPEN_FILE, (LPCWSTR)hFileMenu); //NOTE: when MF_OWNERDRAW is used the 4th param is itemData
	AMT(hFileMenu, OPEN_FILE, LANG_MENU_OPEN);

	AppendMenuW(hFileMenu, MF_SEPARATOR | MF_OWNERDRAW, SEPARATOR1, (LPCWSTR)hFileMenu);

	AppendMenuW(hFileMenu, MF_STRING | MF_OWNERDRAW, SAVE_FILE, (LPCWSTR)hFileMenu);
	AMT(hFileMenu, SAVE_FILE, LANG_MENU_SAVE);

	AppendMenuW(hFileMenu, MF_STRING | MF_OWNERDRAW, SAVE_AS_FILE, (LPCWSTR)hFileMenu);
	AMT(hFileMenu, SAVE_AS_FILE, LANG_MENU_SAVEAS);

	AppendMenuW(hFileMenu, MF_STRING | MF_OWNERDRAW, BACKUP_FILE, (LPCWSTR)hFileMenu);
	AMT(hFileMenu, BACKUP_FILE, LANG_MENU_BACKUP);

	AppendMenuW(hFileMenu, MF_SEPARATOR | MF_OWNERDRAW, SEPARATOR2, (LPCWSTR)hFileMenu);

	AppendMenuW(hFileMenu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)hFileMenuLang, (LPCWSTR)hFileMenu);
	AMT(hFileMenu, (UINT_PTR)hFileMenuLang, LANG_MENU_LANGUAGE);
	//TODO(fran): SetMenuItemInfo only accepts UINT, not the UINT_PTR of MF_POPUP, plz dont tell me I have to redo all of it a different way (LANGUAGE_MANAGER just does it normally not caring for the extra 32 bits)

	HBITMAP bTick = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(UNCAP_BMP_CHECKMARK)); //TODO(fran): preload the bitmaps or destroy them when we destroy the menu

#define _language_appendtomenu(member,value_expr) \
		AppendMenuW(hFileMenuLang, MF_STRING | MF_OWNERDRAW, LANGUAGE::member, (LPCWSTR)hFileMenuLang); \
		SetMenuItemString(hFileMenuLang, LANGUAGE::member, FALSE, _t(#member)); \
		SetMenuItemBitmaps(hFileMenuLang, LANGUAGE::member, MF_BYCOMMAND, NULL, bTick); \

	_foreach_language(_language_appendtomenu)
#undef _language_appendtomenu
		CheckMenuItem(hFileMenuLang, LANGUAGE_MANAGER::Instance().GetCurrentLanguage(), MF_BYCOMMAND | MF_CHECKED);

	HBITMAP bCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(UNCAP_BMP_CLOSE));
	HBITMAP bEarth = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(UNCAP_BMP_EARTH));

	SetMenuItemBitmaps(hFileMenu, BACKUP_FILE, MF_BYCOMMAND, bCross, bTick);//1ºunchecked,2ºchecked
	if (state->settings->is_backup_enabled) CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);

	SetMenuItemBitmaps(hFileMenu, (UINT)(UINT_PTR)hFileMenuLang, MF_BYCOMMAND, bEarth, bEarth);
	//https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getmenustate
	//https://docs.microsoft.com/es-es/windows/desktop/menurc/using-menus#using-custom-check-mark-bitmaps
	//https://www.daniweb.com/programming/software-development/threads/490612/win32-can-i-change-the-hbitmap-structure-for-be-transparent

	UNCAPNC_set_menu(state->nc_parent, state->menu);
}

unCapClProcState* UNCAPCL_get_state(HWND uncapcl) {
	unCapClProcState* state = (unCapClProcState*)GetWindowLongPtr(uncapcl, 0);
	return state;
}

//Returns the current position of that tab or -1 if failed
int UNCAPCL_SaveAndDisableCurrentTab(unCapClProcState* state) {
	int index = (int)SendMessage(state->controls.tab, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM prev_item;
		prev_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
		BOOL res = (BOOL)SendMessage(state->controls.tab, TCM_GETITEM, index, (LPARAM)&prev_item);//TODO(fran): replace with gettabextrainfo
		if (res) {
			ShowWindow(prev_item.extra_info.hText, SW_HIDE);

			prev_item.extra_info.commentType = (COMMENT_TYPE)SendMessage(state->controls.combo_commentmarker, CB_GETCURSEL, 0, 0);

			WCHAR temp[2] = { 0 };
			WCHAR initialChar;
			WCHAR finalChar;
			GetWindowTextW(state->controls.edit_commentbegin, temp, 2); //tambien la funcion devuelve 0 si no agarra nada, podemos agregar ese check
			initialChar = temp[0];//vale '\0' si no tiene nada
			GetWindowTextW(state->controls.edit_commentend, temp, 2);
			finalChar = temp[0];

			prev_item.extra_info.initialChar = initialChar;

			prev_item.extra_info.finalChar = finalChar;

			//Save back to the control
			prev_item.tab_info.mask = TCIF_PARAM; //just in case it changed for some reason
			SendMessage(state->controls.tab, TCM_SETITEM, index, (LPARAM)&prev_item);

		}
	}
	return index;
}

void UNCAPCL_enable_other_char(unCapClProcState* state, bool op) {
	state->is_commentmarker_other = op;
	InvalidateRect(state->controls.static_commentbegin, NULL, TRUE);//TODO(fran): I need to paint my own static wnds, these are terrible, they dont handle well being disabled and enabled
	InvalidateRect(state->controls.static_commentend, NULL, TRUE);
	EnableWindow(state->controls.edit_commentbegin, op);
	EnableWindow(state->controls.edit_commentend, op);
}

//Returns complete path of all the files found, if a folder is found files are searched inside it
std::vector<std::wstring> GetFiles(LPCWSTR s)//, int dir_lenght)
{
	std::vector<std::wstring> files; //capaz deberia ser = L"";
	for (auto& p : std::experimental::filesystem::recursive_directory_iterator(s)) {
		//if (p.status().type() == std::experimental::filesystem::file_type::directory) files.push_back(p.path().wstring().substr(dir_lenght, string::npos));
		//else
		//if (!p.path().extension().compare(".srt")) files.push_back(p.path().wstring().substr(dir_lenght, string::npos));
		//auto ext = p.path().extension();
		//if (ext.empty() || !ext.compare(_t(".srt")) || !ext.compare(_t(".ass")) || !ext.compare(_t(".ssa")) || !ext.compare(_t(".txt"))) files.push_back(p.path().wstring());

		//TODO(fran): do one pass storing all the extesnsions, then show a window to the user with multiple selection, like those circles you can press, and let the user choose what types they want
		files.push_back(p.path().wstring());
	}
	return files;
}

std::vector<std::wstring> CatchDrop(WPARAM wParam) { //TODO(fran): check for valid file extesion
	HDROP hDrop = (HDROP)wParam;
	WCHAR lpszFile[MAX_PATH] = { 0 };
	UINT file_count = 0;

	file_count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, NULL);

	std::vector<std::wstring>files;

	for (UINT i = 0; i < file_count; i++) {
		UINT requiredChars = DragQueryFileW(hDrop, i, NULL, NULL);//INFO: does NOT include terminating NULL char
		Assert(requiredChars < (sizeof(lpszFile) / sizeof(lpszFile[0])));

		if (DragQueryFileW(hDrop, i, lpszFile, sizeof(lpszFile) / sizeof(lpszFile[0]))) {//retrieve the string

			if (std::filesystem::is_directory(std::filesystem::path(lpszFile))) {//user dropped a folder
				files = GetFiles(lpszFile);
			}
			else {
				files.push_back(lpszFile);
			}
		}
		else MessageBoxW(NULL, RCS(LANG_DRAGDROP_ERROR_FILE), RCS(LANG_ERROR), MB_ICONERROR);
	}

	DragFinish(hDrop); //free mem
	return files;
}

LRESULT CALLBACK UncapClProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	unCapClProcState * state = UNCAPCL_get_state(hwnd);
	switch (msg) {
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		switch (wparam) {//wParam specifies the identifier of the control that needs painting
		case TABCONTROL: //TODO(fran): this whole thing shouldnt happen here, if I make a good tab control the user should be able to set up everything like they want and let me handle the stupidly long drawing routines
		{
			if (item->itemAction & (ODA_DRAWENTIRE | ODA_FOCUS | ODA_SELECT)) {//Determines what I have to do with the control
				//ODA_DRAWENTIRE:Draw everything
				//ODA_FOCUS:Control lost or gained keyboard focus, check itemState
				//ODA_SELECT:Selection status changed, check itemState
				//Fill background //TODO(fran): the original background is still visible for some reason
				FillRect(item->hDC, &item->rcItem, unCap_colors.ControlBk);
				//IMPORTANT INFO: this is no region set so I can actually draw to the entire control, useful for going over what the control draws

				UINT index = item->itemID;

				bool is_cursel = index == (UINT)TabCtrl_GetCurSel(item->hwndItem);

				//Draw text
				std::wstring text = GetTabTitle(state->controls.tab, index);

				TEXTMETRIC tm;
				GetTextMetrics(item->hDC, &tm);
				// Calculate vertical position for the item string so that it will be vertically centered

				int yPos = (item->rcItem.bottom + item->rcItem.top - tm.tmHeight) / 2;

				int xPos = item->rcItem.left + (int)((float)RECTWIDTH(item->rcItem)*.1f);


				//SIZE text_size;
				//GetTextExtentPoint32(item->hDC, text.c_str(), text.length(), &text_size);

				//INFO TODO: probably simpler is to just mark a region and then unmark it, not sure how it will work for animation though
				//also I'm not sure how this method handles other unicode chars, ie japanese
				RECT close_button_rc = TAB_calc_close_button(item->rcItem);
				int close_button_pad_right = distance(item->rcItem.right, close_button_rc.right);
				int max_count;
				SIZE fulltext_size;
				RECT modified_rc = item->rcItem;
				modified_rc.left = xPos;
				modified_rc.right -= (close_button_pad_right + RECTWIDTH(close_button_rc));
				BOOL res = GetTextExtentExPoint(item->hDC, text.c_str(), (int)text.length()
					, RECTWIDTH(modified_rc)/*TODO(fran):This should be DPI aware*/, &max_count, NULL, &fulltext_size);
				Assert(res);
				int len = max_count;


				COLORREF old_bk_color = GetBkColor(item->hDC);
				COLORREF old_txt_color = GetTextColor(item->hDC);

				SetBkColor(item->hDC, ColorFromBrush(unCap_colors.ControlBk));
				SetTextColor(item->hDC, ColorFromBrush(is_cursel ? unCap_colors.ControlTxt : unCap_colors.ControlTxt_Inactive));
				TextOut(item->hDC, xPos, yPos, text.c_str(), len);

				//Reset hdc as it was before our painting
				SetBkColor(item->hDC, old_bk_color);
				SetTextColor(item->hDC, old_txt_color);

				//Draw close button 

				//POINT button_topleft;
				//button_topleft.x = item->rcItem.right - TabCloseButtonInfo.rightPadding - TabCloseButtonInfo.icon.cx;
				//button_topleft.y = item->rcItem.top + (RECTHEIGHT(item->rcItem) - TabCloseButtonInfo.icon.cy) / 2;

				//RECT r;
				//r.left = button_topleft.x;
				//r.right = r.left + TabCloseButtonInfo.icon.cx;
				//r.top = button_topleft.y; 
				//r.bottom = r.top + TabCloseButtonInfo.icon.cy;
				//FillRect(item->hDC, &r, unCap_colors.ControlTxt);

				//HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
				//LONG icon_id = GetWindowLongPtr(hwnd, GWL_USERDATA);
				//TODO(fran): we could set the icon value on control creation, use GWL_USERDATA
				//HICON close_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(CROSS_ICON), IMAGE_ICON, TabCloseButtonInfo.icon.cx, TabCloseButtonInfo.icon.cy, LR_SHARED);

				//if (close_icon) {
				//	DrawIconEx(item->hDC, button_topleft.x, button_topleft.y,
				//		close_icon, TabCloseButtonInfo.icon.cx, TabCloseButtonInfo.icon.cy, 0, NULL, DI_NORMAL);//INFO: changing that NULL gives the option of "flicker free" icon drawing, if I need
				//	DestroyIcon(close_icon);
				//}

				static HBITMAP HACK_close = NULL;
				if (!HACK_close) HACK_close = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(UNCAP_BMP_CLOSE));
				HBITMAP bmp = HACK_close;
				BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
				if (bitmap.bmBitsPixel == 1) {
					//TODO(fran): there's no reason why I cant go in halves to, add extra if for bigger or smaller than bmp
					int max_sz = roundNdown(bitmap.bmWidth, min(RECTWIDTH(close_button_rc), RECTHEIGHT(close_button_rc))); //HACK: instead use png + gdi+ + color matrices
					if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

					int bmp_height = max_sz;
					int bmp_width = bmp_height;
					int bmp_align_width = item->rcItem.right - close_button_pad_right - bmp_width;
					int bmp_align_height = item->rcItem.top + (RECTHEIGHT(item->rcItem) - bmp_height) / 2;
					urender::draw_mask(item->hDC, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, is_cursel ? unCap_colors.Img : unCap_colors.Img_Inactive);//TODO(fran): parametric color
				}
				//TODO?(fran): add a plus sign for the last tab and when pressed launch choosefile? or no plus sign at all since we dont really need it

			}

			return TRUE;
		}
		default: return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	} break;
	case WM_CTLCOLORLISTBOX:
	{
		HDC comboboxDC = (HDC)wparam;
		SetBkColor(comboboxDC, ColorFromBrush(unCap_colors.ControlBk));
		SetTextColor(comboboxDC, ColorFromBrush(unCap_colors.ControlTxt));

		return (INT_PTR)unCap_colors.ControlBk;
	}
	case WM_CTLCOLOREDIT://TODO(fran): paint my own edit box so I can add a 1px border, OR paint it here, I do get the dc, and apply a clip region so no one overrides it
	{//NOTE: there doesnt seem to be a way to modify the border that I added with WS_BORDER, it probably is handled outside of the control
		/*HWND ctl = (HWND)lparam;
		HDC dc = (HDC)wparam;
		if (ctl == hInitialChar || ctl == hFinalChar)
		{
			SetBkColor(dc, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		}
		else return DefWindowProc(hwnd, msg, wparam, lparam);*/
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_CTLCOLORSTATIC:
	{
		//TODO(fran): check we are changing the right controls
		HWND static_wnd = (HWND)lparam;
		switch (GetDlgCtrlID(static_wnd)) {
		case TIMEDMESSAGES:
		{
			SetBkColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlMsg));
			return (LRESULT)unCap_colors.ControlBk;
		}
		case INITIALFINALCHAR:
		{
			SetBkColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlBk));
			COLORREF txt = NULL;
			printf("enabled: %d\n", IsWindowEnabled(static_wnd));
			if (state->is_commentmarker_other) {
				txt = ColorFromBrush(unCap_colors.ControlTxt);
			}
			else txt = ColorFromBrush(unCap_colors.InitialFinalCharDisabled);
			SetTextColor((HDC)wparam, txt);
			return (LRESULT)unCap_colors.ControlBk;
		}
		default:
		{
			//TODO(fran): there's something wrong with Initial & Final character controls when they are disabled, documentation says windows always uses same color
			//text when window is disabled but it looks to me that it is using both this and its own color
			SetBkColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		}
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	case WM_CREATE:
	{
		CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

		UNCAPCL_add_controls(state, (HINSTANCE)GetWindowLongPtr(state->wnd, GWLP_HINSTANCE));
		UNCAPCL_add_menus(state);

		SetText_file_app(state->nc_parent,nullptr, appName);

		return 0;
	} break;
	case WM_SIZE:
	{
		UNCAPCL_ResizeWindows(state);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCDESTROY:
	{
		UNCAPCL_save_settings(state);
		if (state) {
			free(state);
			state = nullptr;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCCREATE:
	{
		CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
		unCapClProcState* st = (unCapClProcState*)calloc(1, sizeof(unCapClProcState));
		Assert(st);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)st);
		st->wnd = hwnd;
		st->nc_parent = create_nfo->hwndParent;
		st->settings = (unCapClSettings*)create_nfo->lpCreateParams;
		//UNCAPCL_load_settings(st);

		return TRUE;
	} break;
	case WM_NOTIFY:
	{
		NMHDR* msg_info = (NMHDR*)lparam;
		switch (msg_info->code) {
			//Handling tab changes, since there is no specific msg to manage this from inside the tab control, THANKS WINDOWS
		case TCN_SELCHANGING: //TODO(fran): now we are outside the tab control we should use the other parameters to check we are actually changing the control we want
		{
			if (msg_info->hwndFrom == state->controls.tab) {
				UNCAPCL_SaveAndDisableCurrentTab(state);
				return FALSE; //FALSE=Allow selection to change. TRUE= prevent it from changing
			}
			else return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case TCN_SELCHANGE:
		{
			if (msg_info->hwndFrom == state->controls.tab) {
				int index = (int)SendMessage(msg_info->hwndFrom, TCM_GETCURSEL, 0, 0);
				printf("CURSEL: %d\n", index);
				if (index != -1) {
					CUSTOM_TCITEM new_item;
					new_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
					BOOL ret = (BOOL)SendMessage(msg_info->hwndFrom, TCM_GETITEM, index, (LPARAM)&new_item);
					if (ret) {
						UNCAPCL_enable_tab(state, new_item.extra_info);
					}
				}
				else {
					TAB_INFO blank = { 0 };
					UNCAPCL_enable_tab(state, blank);
				}
				return 0;//Return value isnt used
			}
			else return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	} break;
	case WM_COMMAND:
	{
		printf("HIWORD(wparam)=%d\n", HIWORD(wparam));
		switch (HIWORD(wparam)) {//NOTE: old controls use this style of messaging the parent instead of WM_NOTIFY
		case CBN_SELCHANGE://Combobox selection has changed
			WORD ctl_identifier = LOWORD(wparam);
			HWND ctl_wnd = (HWND)lparam;
			if (ctl_wnd == state->controls.combo_commentmarker) {
				int cursel = ComboBox_GetCurSel(ctl_wnd);
				UNCAPCL_enable_other_char(state, cursel == COMMENT_TYPE::other);
				return 0;
			}
		}

		//TODO(fran): determine a standard to avoid msgs entering into the wrong switch statement, eg 0 on the HI or LOWORD of wparam means dont enter in that switch
		//            Or replace the WM_COMMAND sendmsg in the controls for WM_NOTIFY

		// Msgs from my controls
		switch (LOWORD(wparam))
		{
		case OPEN_FILE:
		case ID_OPEN:
		{
			UNCAPCL_choose_file(state, L"srt");//TODO(fran): not only srt
			return 0;
		}
		case BACKUP_FILE: //Do backup menu item was clicked
		{
			if (state->settings->is_backup_enabled) {
				CheckMenuItem(state->menu_file, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED); //turn it off
			}
			else {
				CheckMenuItem(state->menu_file, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);//turn it on
			}
			state->settings->is_backup_enabled = !state->settings->is_backup_enabled;
			//printf("BACKUP=%s\n", state->settings->is_backup_enabled ? "true" : "false");
			return 0;
		}
		case COMBO_BOX:
			if (HIWORD(wparam) == CBN_SELCHANGE)
				UNCAPCL_enable_other_char(state, SendMessageW(state->controls.combo_commentmarker, CB_GETCURSEL, 0, 0) == COMMENT_TYPE::other);
			return 0;

		case REMOVE:
		{
			FILE_FORMAT format = GetCurrentTabExtraInfo(state->controls.tab).fileFormat;
			switch (SendMessageW(state->controls.combo_commentmarker, CB_GETCURSEL, 0, 0)) {
			case COMMENT_TYPE::brackets:
				UNCAPCL_comment_removal(state, GetCurrentTabExtraInfo(state->controls.tab).hText, format, L'[', L']');
				break;
			case COMMENT_TYPE::parenthesis:
				UNCAPCL_comment_removal(state, GetCurrentTabExtraInfo(state->controls.tab).hText, format, L'(', L')');
				break;
			case COMMENT_TYPE::braces:
				UNCAPCL_comment_removal(state, GetCurrentTabExtraInfo(state->controls.tab).hText, format, L'{', L'}');
				break;
			case COMMENT_TYPE::other:
				UNCAPCL_custom_comment_removal(state, GetCurrentTabExtraInfo(state->controls.tab).hText, format);
				break;
			}
			return 0;
		}
		case SAVE_FILE:
		case ID_SAVE:
		{
			TAB_INFO tabnfo = GetCurrentTabExtraInfo(state->controls.tab);
			if (tabnfo.hText && tabnfo.filePath) {
				if (state->settings->is_backup_enabled) DoBackup(tabnfo.filePath);

				std::wstring filename_saved = DoSave(tabnfo.hText, tabnfo.filePath, tabnfo.fileEncoding);

				if (filename_saved[0]) SetWindowText(state->controls.static_notify, RCS(LANG_SAVE_DONE)); //Notify success
			}
			return 0;
		}
		case SAVE_AS_FILE:
		case ID_SAVE_AS:
		{
			//TODO(fran): DoSaveAs allows for saving of file when no tab is created and sets all the controls but doesnt create an empty tab,
			//should we create the tab or not allow to save?
			TAB_INFO tabnfo = GetCurrentTabExtraInfo(state->controls.tab);
			if (tabnfo.hText) {
				TXT_ENCODING og_encoding = tabnfo.fileEncoding;
				std::wstring og_extension = (std::filesystem::path(tabnfo.filePath)).extension();//TODO(fran): should I use tabnfo.fileFormat?
				GetSaveAsStrResult saveas_res = GetSaveAsStr(state->settings->last_dir, og_extension, og_encoding);
				if (saveas_res.savefile != L"") {
					//NOTE: No backup since the idea is the user saves to a new file obviously
					std::wstring filename_saved = DoSave(tabnfo.hText, saveas_res.savefile, saveas_res.encoding);
					if (filename_saved[0]) {

						SetWindowText(state->controls.static_notify, RCS(LANG_SAVE_DONE)); //Notify success
						//SetWindowTextW(hFile, filename_saved.c_str()); //Update filename viewer
						SetText_file_app(state->nc_parent, filename_saved.c_str(), appName); //Update nc title
						SetCurrentTabTitle(state->controls.tab, filename_saved.substr(filename_saved.find_last_of(L"\\") + 1)); //Update tab name

						int tabindex = (int)SendMessage(state->controls.tab, TCM_GETCURSEL, 0, 0);
						TAB_INFO tabnfo = GetCurrentTabExtraInfo(state->controls.tab);
						tabnfo.fileEncoding = saveas_res.encoding;
						wcsncpy_s(tabnfo.filePath, filename_saved.c_str(), ARRAYSIZE(tabnfo.filePath));//Update tab item's filepath
						SetTabExtraInfo(state->controls.tab, tabindex, tabnfo);
					}
				}
			}
			return 0;
		}
#define _generate_enum_cases(member,value_expr) case LANGUAGE::member:
		_foreach_language(_generate_enum_cases)
			//case LANGUAGE_MANAGER::LANGUAGE::ENGLISH://INFO: this relates to the menu item, TODO(fran): a way to check a range of numbers, from first lang to last
			//case LANGUAGE_MANAGER::LANGUAGE::SPANISH:
		{
			//NOTE: maybe I can force menu item remeasuring by SetMenu, basically sending it the current menu
			//MenuChangeLang((LANGUAGE)LOWORD(wparam));
			LANGUAGE_MANAGER::Instance().ChangeLanguage((LANGUAGE)LOWORD(wparam));

#if 1 //One way of updating the menus and getting them recalculated, destroy everything and create it again, problems: gotta change internal code to fix some state which doesnt take into account the realtime values, gotta take language_manager related things out of the function to avoid repetition, or add checks in language_mgr to ignore repeated objects
			HMENU old_menu = GetMenu(state->nc_parent);
			//SetMenu(state->nc_parent, NULL);
			UNCAPCL_add_menus(state); //TODO(fran): get this working, we are gonna need to implement removal into lang_mgr, and other things
			DestroyMenu(old_menu); //NOTE: I could also use RemoveMenu which doesnt destroy it's submenus and reattach them to a new "main" menu and correct parenting (itemData param)
#else //the undocumented way, aka the way it should have always been
			//WIP
#endif
			//TODO(fran): continue with this undocumented search, it doesnt look very good, I could only get the hwnd when opening the languages sub menu
			//https://www.google.com/search?q=FindWindow(NULL,+%22%2332768%22)%3B&rlz=1C1GIWA_enAR589AR589&sxsrf=ALeKk03pPU3gTyENAggyeDZgCHuNKyustA:1600226751225&ei=v4VhX6WFDZCz5OUPwJ2y6A8&start=10&sa=N&ved=2ahUKEwjl4MOY3ezrAhWQGbkGHcCODP0Q8NMDegQIDhBA&biw=1645&bih=1646
			//https://www.codeproject.com/Articles/2080/Transparent-Menu
			//https://microsoft.public.win32.programmer.ui.narkive.com/jQJBmxzp/open-submenu-programmatically#post5
			//https://comp.os.ms-windows.programmer.win32.narkive.com/RhPGGM4C/forcing-menu-item-to-refresh
			//case WM_INITMENU:
			//case WM_INITMENUPOPUP:
			//{
			//	HWND hMenuWnd = FindWindow(TEXT("#32768"), NULL);
			//	if (IsWindow(hMenuWnd)) {
			//		PostMessage(hMenuWnd, 0x1e2, 0, 0); //doesnt seem to work
			//		PostMessage(hMenuWnd, 0x1e5, 0, 0); //works
			//	}
			//} break;
			return 0;
		}
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCCALCSIZE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_PARENTNOTIFY: //TODO(fran): I dont know if I can get anything from here
	{
		WORD submsg = LOWORD(wparam);
		if (submsg == (WORD)TCN_DELETEITEM && state->controls.tab == (HWND)lparam) {
			int idx = HIWORD(wparam);
			TAB_INFO info = GetTabExtraInfo((HWND)lparam, idx);
			DestroyWindow(info.hText);
			return 0;
		}
		else return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DROPFILES://NOTE: tab control sends me this, TODO(fran): maybe I should handle/receive it in a different way
	{
		std::vector<std::wstring>files = CatchDrop(wparam);

		for (std::wstring const& file : files) {
			AcceptedFile(state, file); //process the new file
		}
		return 0;
	} break;
	case WM_NOTIFYFORMAT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_QUERYUISTATE: //Neat, I dont know who asks for this but it's interesting, you can tell whether you need to draw keyboards accels
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOVE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SHOWWINDOW:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGING:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGED:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCPAINT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ERASEBKGND:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): or return 1, idk
	} break;
	case WM_PAINT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCHITTEST:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SETCURSOR:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEMOVE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEACTIVATE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_LBUTTONDOWN:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_LBUTTONUP:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_RBUTTONDOWN:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_RBUTTONUP:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_CONTEXTMENU://Interesting
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DESTROY:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DELETEITEM://sent to owner of combo/list box (for menus also) when it is destroyed or items are being removed. when you do SetMenu you also get this msg if you had a menu previously attached
		//TODO(fran): we could try to use this to manage state destruction
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_GETTEXT://we should return NULL
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	default:
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
	}
	return 0;
}

ATOM init_wndclass_unCap_uncapcl(HINSTANCE inst) {
	WNDCLASSEXW wcex{ sizeof(wcex) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = UncapClProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(unCapClProcState*);
	wcex.hInstance = inst;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = unCap_colors.ControlBk;
	wcex.lpszMenuName = 0;// MAKEINTRESOURCEW(IDC_SRTFIXWIN32);//TODO(fran): remove?
	wcex.lpszClassName = unCap_wndclass_uncap_cl;
	wcex.hIconSm = 0;

	ATOM class_atom = RegisterClassExW(&wcex);
	Assert(class_atom);
	return class_atom;
}