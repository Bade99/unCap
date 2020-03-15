﻿#include "SrtFix-Win32.h"

//----------------------LINKER----------------------:
// Linker->Input->Additional Dependencies (right way to link the .lib)
#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"propsys.lib") //open save file handler
#pragma comment(lib,"shlwapi.lib") //open save file handler
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"") 
//#pragma comment(lib,"User32.lib")

//----------------------NAMESPACES----------------------:
using namespace std;
namespace fs = std::experimental::filesystem;


//----------------------TODOs----------------------:
//Draw my own Scrollbars
//Draw my own menu
//Draw my own non client area
//Parametric sizes for everything, and DPI awareness for controls that dont move
//Move error messages from a separate dialog window into the space on the right of the Remove controls
//Option to retain original file encoding on save, otherwise utf8/user defined default
//Fonts with japanese support, basically full unicode charset support


//----------------------DEFINES----------------------:
#ifdef _DEBUG
	#define RELEASE 0 //As I didnt find how to know when the compiler is on Release or Debug I created my own manual one
#else
	#define RELEASE 1
#endif

#define Assert(assertion) if(!(assertion))*(int*)NULL=0

//INFO: Check resource.h for already taken defines
#define OPEN_FILE 11
#define COMBO_BOX 12
#define SAVE_AS_FILE 13
#define REMOVE 14
#define SAVE_FILE 15
#define BACKUP_FILE 16
#define SUBS_WND 17
#define TABCONTROL 18

#define TCM_RESIZETABS (WM_USER+50) //Sent to a tab control for it to tell its tabs' controls to resize. wParam = lParam = 0
#define TCM_RESIZE (WM_USER+51) //wParam= pointer to SIZE of the tab control ; lParam = 0


#define MAX_PATH_LENGTH MAX_PATH
#define MAX_TEXT_LENGTH 288000 //Edit controls have their text limit set by default at 32767, we need to change that

#define RECTWIDTH(r) (r.right >= r.left ? r.right - r.left : r.left - r.right )

#define RECTHEIGHT(r) (r.bottom >= r.top ? r.bottom - r.top : r.top - r.bottom )

//----------------------USER DEFINED VARIABLES----------------------:
enum ENCODING {
	ANSI=1,
	UTF8,
	UTF16, //Big endian or Little endian
	//ASCII is easily encoded in utf8 without problems
};

enum FILE_FORMAT {
	SRT=1, //This can also be used for any format that doesnt use in its sintax any of the characters detected as "begin comment" eg [ { (
	SSA //Used to signal any version of Substation Alpha: v1,v2,v3,v4,v4+ (Advanced Substation Alpha)
};

//The different types of characters that are detected as comments
enum COMMENT_TYPE {
	brackets = 0, parenthesis, braces, other
};

struct TEXT_INFO {
	//TODO(fran): will we store the data for each control or store the controls themselves and hide them?
	HWND hText = NULL;
	WCHAR filePath[MAX_PATH] = { 0 };//TODO(fran): nowadays this isn't really the max path lenght
	COMMENT_TYPE commentType = COMMENT_TYPE::brackets;
	WCHAR initialChar = L'\0';
	WCHAR finalChar = L'\0';
	FILE_FORMAT fileFormat = FILE_FORMAT::SRT;
};

struct CUSTOM_TCITEM {
	TC_ITEMHEADER tab_info;
	TEXT_INFO extra_info;
};

struct _APPCOLORS {
	HBRUSH ControlBk;
	HBRUSH ControlBkPush;
	HBRUSH ControlBkMouseOver;
	HBRUSH ControlTxt;
};

struct TABCLIENT { //Construct the "client" space of the tab control from this offsets, aka the space where you put your controls, in my case text editor
	short leftOffset, topOffset, rightOffset, bottomOffset;
};

struct CLOSEBUTTON {
	int rightPadding;//offset from the right side of each tab's rectangle
	SIZE icon;//Size of the icon (will be placed centered in respect to each tab's rectangle)
};

struct vec2d {
	float x, y;
};

//----------------------GLOBALS----------------------:
HMENU hMenu;//main menu bar(only used to append the rest)
HMENU hFileMenu;
HMENU hFileMenuLang;

HWND hFile;//window for file directory
HWND hOptions;
HWND hInitialText;
HWND hInitialChar;
HWND hFinalText;
HWND hFinalChar;
HWND hRemove;
//HWND hSubs;//window for file contents
HWND hRemoveCommentWith;
//HWND hRemoveProgress;
HWND hReadFile;
HWND TextContainer;//The tab control where the text editors are "contained"

HFONT hGeneralFont;//Main font manager

HMENU hPopupMenu;

bool Backup_Is_Checked = false; //info file check, do backup or not

LANGUAGE_MANAGER::LANGUAGE startup_locale = LANGUAGE_MANAGER::LANGUAGE::ENGLISH; //defaults to english

#define RC_LEFT 0
#define RC_TOP 1
#define RC_RIGHT 2
#define RC_BOTTOM 3
#define DIR 4
#define BACKUP 5
#define LOCALE 6
const wchar_t *info_parameters[] = {
	L"[rc_left]=",
	L"[rc_top]=",
	L"[rc_right]=",
	L"[rc_bottom]=",
	L"[dir]=",
	L"[backup]=",
	L"[locale]="
};
#define NRO_INFO_PARAMETERS size(info_parameters)

wstring last_accepted_file_dir = L""; //Path of the last valid file directory

RECT rcMainWnd = {75,75,600,850}; //Main window size and position //TODO(fran) this only needs to exist during startup

int y_place = 10;

const COMDLG_FILTERSPEC c_rgSaveTypes[] = //TODO(fran): am I using this? if so this should go in resource file
{
{ L"Srt (*.srt)",       L"*.srt" },
{ L"Advanced SSA (*.ass)",       L"*.ass" },
{ L"Sub Station Alpha (*.ssa)",       L"*.ssa" },
{ L"Txt (*.txt)",       L"*.txt" },
{ L"All Documents (*.*)",         L"*.*" }
};

_APPCOLORS AppColors;

TABCLIENT TabOffset;

CLOSEBUTTON TabCloseButtonInfo; //Information for the placement of the close button of each tab in a tab control

//----------------------HELPER FUNCTIONS----------------------:
inline COLORREF ColorFromBrush(HBRUSH br) {
	LOGBRUSH lb;
	GetObject(br, sizeof(lb), &lb);
	return lb.lbColor;
}

inline bool FileOrDirExists(const wstring& file) {
	return fs::exists(file);//true if exists
}

//Retrieves full path to the info for program startup
wstring GetInfoPath() {
	//Load startup info
	PWSTR folder_path;
	HRESULT folder_res = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &folder_path);//TODO(fran): this is only supported on windows vista and above
	//INFO: no retorna la ultima barra, la tenés que agregar vos, tambien te tenés que encargar de liberar el string
	Assert(SUCCEEDED(folder_res));
	wstring known_folder = folder_path;
	CoTaskMemFree(folder_path);
	wstring info_folder = L"unCap";
	wstring info_file = L"info";
	wstring info_extension = L"txt";//just so the user doesnt even have to open the file manually on windows

	wstring info_file_path = known_folder + L"\\" + info_folder + L"\\" + info_file + L"." + info_extension;
	return info_file_path;
}

wstring GetInfoDirectory() {
	//TODO(fran): quick and dirty, make this nice, possibly put each part of the path on a struct
	PWSTR folder_path;
	HRESULT folder_res = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &folder_path);//TODO(fran): this is only supported on windows vista and above
	//INFO: no retorna la ultima barra, la tenés que agregar vos, tambien te tenés que encargar de liberar el string
	Assert(SUCCEEDED(folder_res));
	wstring known_folder = folder_path;
	CoTaskMemFree(folder_path);
	wstring info_folder = L"unCap";

	wstring info_file_dir = known_folder + L"\\" + info_folder;
	return info_file_dir;
}

//Returns the nth tab's text info or a TEXT_INFO struct with everything set to 0
TEXT_INFO GetTabExtraInfo(int index) {
	if (index != -1) {
		CUSTOM_TCITEM item;
		item.tab_info.mask = TCIF_PARAM;
		BOOL ret = SendMessage(TextContainer, TCM_GETITEM, index, (LPARAM)&item);
		if (ret) {
			return item.extra_info;
		}
		//TODO(fran): error handling
	}
	TEXT_INFO invalid = { 0 };
	return invalid;
}

//Returns the current tab's text info or a TEXT_INFO struct with everything set to 0
TEXT_INFO GetCurrentTabExtraInfo() {
	int index = SendMessage(TextContainer, TCM_GETCURSEL, 0, 0);

	return GetTabExtraInfo(index);
}

wstring GetTabTitle(int index){
	if (index != -1) {
		CUSTOM_TCITEM item;
		WCHAR title[200];
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = title;
		item.tab_info.cchTextMax = ARRAYSIZE(title);
		BOOL ret = SendMessage(TextContainer, TCM_GETITEM, index, (LPARAM)&item);
		if (ret) {
			return title;
		}
	}
	return L"";
}

BOOL SetTabExtraInfo(int index,const TEXT_INFO& text_data) {
	CUSTOM_TCITEM item;
	item.tab_info.mask = TCIF_PARAM;
	item.extra_info = text_data;

	int ret = SendMessage(TextContainer, TCM_SETITEM, index, (LPARAM)&item);
	return ret;
}

BOOL SetCurrentTabTitle(wstring title) {
	int index = SendMessage(TextContainer, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM item;
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = (LPWSTR)title.c_str();
		int ret = SendMessage(TextContainer, TCM_SETITEM, index, (LPARAM)&item);
		return ret;
	}
	else return index;
}


int GetNextTabPos() {
	int count = SendMessage(TextContainer, TCM_GETITEMCOUNT, 0, 0);
	return count + 1; //TODO(fran): check if this is correct, or there is a better way
}

//Returns the index of the previously selected tab if successful, or -1 otherwise.
int ChangeTabSelected(int index) {
	return SendMessage(TextContainer, TCM_SETCURSEL, index, 0);
}

//Returns TRUE if successful, or FALSE otherwise.
int DeleteTab(HWND TabControl, int position) {
	return SendMessage(TabControl, TCM_DELETEITEM, position, 0);
}

BOOL TestCollisionPointRect(POINT p, const RECT& rc) {
	//TODO(fran): does this work with negative values?
	if (p.x < rc.left || p.x > rc.right) return FALSE;
	if (p.y < rc.top || p.y > rc.bottom) return FALSE;
	return TRUE;
}

//----------------------FUNCTION PROTOTYPES----------------------:
LRESULT CALLBACK	ComboProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK	ButtonProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK	EditCatchDrop(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void				AddMenus(HWND);
void				ChooseFile(wstring);
void				CommentRemoval(HWND, FILE_FORMAT, WCHAR, WCHAR);
void				CustomCommentRemoval(HWND, FILE_FORMAT);
void				EnableOtherChar(bool);
void				DoBackup();
void				DoSave(HWND,wstring);
HRESULT				DoSaveAs(HWND);
void				CatchDrop(WPARAM);
void				CreateInfoFile(HWND);
void				CheckInfoFile();
void				CreateFonts();
void				AcceptedFile(wstring);
ENCODING			GetTextEncoding(wstring);
void				SetOptionsComboBox(HWND&, bool);
void				ResizeWindows(HWND);




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR lpCmdLine,_In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	/*Initialization of common controls
	INITCOMMONCONTROLSEX icc;

	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES;

	InitCommonControlsEx(&icc);
	*/

	if (!RELEASE) {
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}

	WCHAR szWindowClass[40] = L"unCapClass";	// nombre de clase de la ventana principal
	WCHAR szTitle[40] = L"unCap";				// Texto de la barra de título

    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadStringW(hInstance, IDC_SRTFIXWIN32, szWindowClass, MAX_LOADSTRING);

	CheckInfoFile();

	LANGUAGE_MANAGER::Instance().SetHInstance(hInstance);
	LANGUAGE_MANAGER::Instance().ChangeLanguage(startup_locale);

	AppColors.ControlBk = CreateSolidBrush(RGB(40,41,35));
	AppColors.ControlBkPush = CreateSolidBrush(RGB(0, 110, 200));
	AppColors.ControlBkMouseOver = CreateSolidBrush(RGB(0, 120, 215));
	AppColors.ControlTxt = CreateSolidBrush(RGB(248, 248, 242));

	//Setting offsets for what will define the "client" area of a tab control
	TabOffset.leftOffset = 3;
	TabOffset.rightOffset = TabOffset.leftOffset;
	TabOffset.bottomOffset = TabOffset.leftOffset;
	TabOffset.topOffset = 24;

	CreateFonts();

	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SRTFIXWIN32));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = AppColors.ControlBk;//(HBRUSH)(COLOR_BTNFACE +1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SRTFIXWIN32);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	ATOM class_atom = RegisterClassExW(&wcex);
	if (!class_atom) return FALSE;
	//RegisterSubtitleWindowClass(hInstance,SubtitleWindowClassName, IDC_ARROW, COLOR_BTNFACE + 1);

    // Realizar la inicialización de la aplicación:

	HWND hWnd = CreateWindowEx(WS_EX_CONTROLPARENT/*|WS_EX_ACCEPTFILES*/, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		rcMainWnd.left, rcMainWnd.top, RECTWIDTH(rcMainWnd), RECTHEIGHT(rcMainWnd), nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SRTFIXWIN32));

    MSG msg;

    // Bucle principal de mensajes:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
		

		//if (!IsDialogMessage(hWnd, &msg))
		//{
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}

    }

	//Cleanup
	if (AppColors.ControlBk)DeleteObject(AppColors.ControlBk);//TODO(fran): destructor?
	if (AppColors.ControlTxt)DeleteObject(AppColors.ControlTxt);

    return (int) msg.wParam;
}

void CreateFonts()
{
	LOGFONTW lf;
	HFONT   hFont;

	memset(&lf, 0, sizeof(lf));
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfHeight = -15;
	//TODO(fran): see lf.pszFaceName and EnumFontFamiliesW
	hFont = CreateFontIndirectW(&lf);

	if (hGeneralFont != NULL)
	{
		DeleteObject(hGeneralFont);
		hGeneralFont = NULL;
	}

	hGeneralFont = hFont;
	if (hGeneralFont == NULL)
	{
		MessageBoxW(NULL, RCS(LANG_FONT_ERROR), RCS(LANG_ERROR), MB_OK);
	}
}

// Removing comments for Substation Alpha format (.ssa) and Advanced Substation Alpha format (.ass)
//https://wiki.multimedia.cx/index.php/SubStation_Alpha
//https://matroska.org/technical/specs/subtitles/ssa.html
//INFO: ssa has v1, v2, v3 and v4, v4+ is ass but all of them are very similar in what concerns us
//I only care about lines that start with Dialogue:
//This lines are always one liners, so we could use getline or variants
//This lines can contain extra commands in the format {\command} therefore in case we are removing braces "{" we must check whether the next char is \ and ignore in that case
//The same rule should apply when detecting the type of comments in CommentTypeFound
//If we find one of this extra commands inside of a comment we remove it, eg [Hello {\br} goodbye] -> Remove everything
//TODO(fran): check that a line with no text is still valid, eg if the comment was the only text on that Dialogue line after we remove it there will be nothing


/// <summary>
/// Performs comment removal for .srt type formats
/// </summary>
/// <param name="text"></param>
/// <param name="start"></param>
/// <param name="end"></param>
/// <returns>The number of comments removed, 0 indicates no comments were found</returns>
size_t CommentRemovalSRT(wstring& text, WCHAR start, WCHAR end) {
	size_t comment_count = 0;
	size_t start_pos = 0, end_pos = 0;

	while (true) {
		start_pos = text.find(start, start_pos);
		end_pos = text.find(end, start_pos + 1);//+1, dont add it here 'cause breaks the if when end_pos=-1(string::npos)
		if (start_pos == wstring::npos || end_pos == wstring::npos) {
			//SendMessage(hRemoveProgress, PBM_SETPOS, text_length, 0);
			break;
		}
		text.erase(text.begin() + start_pos, text.begin() + end_pos + 1); //TODO(fran): handle exceptions
		comment_count++;
		//SendMessage(hRemoveProgress, PBM_SETPOS, start_pos, 0);
	}
	return comment_count;
}

/// <summary>
/// Performs comment removal for all .ssa versions including .ass (v1,v2,v3,v4,v4+)
/// </summary>
/// <param name="text"></param>
/// <param name="start"></param>
/// <param name="end"></param>
/// <returns>The number of comments removed, 0 indicates no comments were found</returns>
size_t CommentRemovalSSA(wstring& text, WCHAR start, WCHAR end) {
	size_t comment_count = 0;
	size_t start_pos = 0, end_pos = 0;
	size_t line_pos=0;
	WCHAR text_marker[] = L"Dialogue:";
	size_t text_marker_size = ARRAYSIZE(text_marker);

	//Substation Alpha has a special code for identifying text that will be shown to the user: 
	//The line has to start with "Dialogue:"
	//Also this lines can contain additional commands in the form {\command} 
	//therefore we've got to be careful when "start" is "{" and also check the next char for "\"
	//in which case it's not actually a comment and we should just ignore it and continue

	while (true) {
		//Find the "start" character
		start_pos = text.find(start, start_pos);
		if (start_pos == wstring::npos) break;

		//Check that the character is in a line that start with "Dialogue:"
		line_pos = text.find_last_of(L'\n', start_pos);
		if (line_pos == wstring::npos) line_pos = 0; //We are on the first line
		else line_pos++; //We need to start checking from the next char after the end of line, eg "\nDialogue:" we want the "D"
		if (text.compare(line_pos, text_marker_size - 1, text_marker)) {//if true the line where we found a possible comment does not start with the desired string
			start_pos++; //Since we made no changes to the string at start_pos we have to move start_pos one char forward so we dont get stuck 
			continue; 
		}

		//Now lets check in case "start" is "{" whether the next char is "\"
		if (start == L'{') {
			if ((start_pos + 1) >= text.length()) break; //Check a next character exists after our current position
			if (text.at(start_pos + 1) == L'\\') { //Indeed it was a \ so we where inside a special command, just ignore it and continue searching
				start_pos++; //Since we made no changes to the string at start_pos we have to move start_pos one char forward so we dont get stuck 
				continue; 
			}
		}
		end_pos = text.find(end, start_pos + 1); //Search for the end of the comment
		if (end_pos == wstring::npos) break; //There's no "comment end" so lets just exit the loop, the file is probably incorrectly written

		text.erase(text.begin() + start_pos, text.begin() + end_pos + 1); //Erase the entire comment, including the "comment end" char, that's what the +1 is for
		comment_count++; //Update comment count
	}
	return comment_count;
}

//si hay /n antes del corchete que lo borre tmb (y /r tmb)
void CommentRemoval(HWND hText, FILE_FORMAT format, WCHAR start,WCHAR end) {//TODO(fran): Should we give the option to detect for more than one char?
	int text_length = GetWindowTextLengthW(hText) + 1;
	wstring text(text_length, L'\0');
	GetWindowTextW(hText, &text[0], text_length);
	size_t comment_count=0;
	switch (format) {
	case FILE_FORMAT::SRT:
		comment_count = CommentRemovalSRT(text, start, end); break;
	case FILE_FORMAT::SSA:
		comment_count = CommentRemovalSSA(text, start, end); break;
	//TODO(fran): should add default:?
	}

	if (comment_count) SetWindowTextW(hText, text.c_str()); //TODO(fran): add message for number of comments removed
	else MessageBoxW(NULL, RCS(LANG_COMMENTREMOVAL_NOTHINGTOREMOVE),L"", MB_ICONEXCLAMATION);
	//ShowWindow(hRemoveProgress, SW_HIDE);
}

void CustomCommentRemoval(HWND hText, FILE_FORMAT format) {
	WCHAR temp[2] = {0};
	WCHAR start;
	WCHAR end;
	GetWindowTextW(hInitialChar, temp, 2); //tambien la funcion devuelve 0 si no agarra nada, podemos agregar ese check
	start = temp[0];//vale '\0' si no tiene nada
	GetWindowTextW(hFinalChar, temp, 2);
	end = temp[0];

	if (start != L'\0' && end != L'\0') CommentRemoval(hText,format, start, end);
	else MessageBoxW(NULL, RCS(LANG_COMMENTREMOVAL_ADDCHARS), L"", MB_ICONEXCLAMATION);
}

///Every line separation(\r or \n or \r\n) will be transformed into \r\n
void ProperLineSeparation(wstring &text) {
	unsigned int pos = 0;

	while (pos <= text.length()) {//length doesnt include \0
		if (text[pos] == L'\r') {
			if (text[pos + 1] != L'\n') {
				text.insert(pos + 1, L"\n");//este nunca se pasa del rango
			}
			pos += 2;
		}
		else if (text[pos] == L'\n') {
			//if(text[pos-1] == L'\r')//ya se que no tiene \r xq sino hubiera entrado por el otro
			text.insert(pos, L"\r");
			pos += 2;
		}
		else {
			pos++;
		}
	}
}

void DoBackup() { //TODO(fran): option to change backup folder

	int text_length = GetWindowTextLengthW(hFile) + 1;//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea

	wstring orig_file(text_length, L'\0');
	GetWindowTextW(hFile, &orig_file[0], text_length);

	if (orig_file[0] != L'\0') {//Check in case there is invalid data on the controls
		wstring new_file = orig_file;
		size_t found = new_file.find_last_of(L"\\")+1;
		Assert(found != wstring::npos);
		new_file.insert(found, L"SDH_");//TODO(fran): add SDH to resource file?

		if (!CopyFileW(orig_file.c_str(), new_file.c_str(), FALSE)) MessageBoxW(NULL, L"The filename is probably too large", L"TODO(fran): Fix the program!", MB_ICONERROR);;
	}
}

void DoSave(HWND textControl, wstring save_file) { //got to pass the encoding to the function too, and do proper conversion

	//https://docs.microsoft.com/es-es/windows/desktop/api/commdlg/ns-commdlg-tagofna
	//http://www.winprog.org/tutorial/app_two.html

	//SEARCH(fran):	WideCharToMultiByte  :: Maps a UTF-16 (wide character) string to a new character string. 
	//				The new character string is not necessarily from a multibyte character set.

	//AnimateWindow
	//TODO(fran): add ProgressBar hReadFile
	wofstream new_file(save_file, ios::binary);
	if (new_file.is_open()) {
		new_file.imbue(locale(new_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, generate_header>));//TODO(fran): multiple options for save encoding
		int text_length = GetWindowTextLengthW(textControl) + 1;
		wstring text(text_length, L'\0');
		GetWindowTextW(textControl, &text[0], text_length);
		new_file << text;
		new_file.close();
		//MessageBoxW(NULL, RCS(LANG_SAVE_DONE), L"", MB_ICONINFORMATION);//TODO(fran): this is super annoying, find other way, maybe something turns green for example
		
		SetWindowTextW(hFile, save_file.c_str()); //Update text editor
		
		SetCurrentTabTitle(save_file.substr(save_file.find_last_of(L"\\") + 1)); //Update tab name

		//TODO(fran): this is a bit cheating since we are not actually showing the new file but the old one, so if there was some error or mismatch
		//when saving the user wouldnt know
	}
	else MessageBoxW(NULL, RCS(LANG_SAVE_ERROR), RCS(LANG_ERROR), MB_ICONEXCLAMATION);
} 

HRESULT DoSaveAs(HWND textControl) //https://msdn.microsoft.com/en-us/library/windows/desktop/bb776913(v=vs.85).aspx
{
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
			hr = pfdc->AddRadioButtonList(CONTROL_RADIOBUTTONLIST);

			// Set the state of the added radio-button list.
			hr = pfdc->SetControlState(CONTROL_RADIOBUTTONLIST,
				CDCS_VISIBLE | CDCS_ENABLED);

			// Add individual buttons to the radio-button list.
			hr = pfdc->AddControlItem(CONTROL_RADIOBUTTONLIST,
				CONTROL_RADIOBUTTON1,
				L"UTF-8");

			hr = pfdc->AddControlItem(CONTROL_RADIOBUTTONLIST,
				CONTROL_RADIOBUTTON2,
				L"UTF-16");

			// Set the default selection to option 1.
			hr = pfdc->SetSelectedControlItem(CONTROL_RADIOBUTTONLIST,
				CONTROL_RADIOBUTTON1);

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
			hr = pfsd->SetFileTypeIndex(INDEX_SRT);
			
			// Set def file type @@ again needs function
			hr = pfsd->SetDefaultExtension(L"srt");//L"doc;docx"

			if (last_accepted_file_dir != L"") {
				IShellItem *defFolder;//@@shouldn't this be IShellFolder?
				//does accepted_file_dir need to be converted to UTF-16 ?
				hr = SHCreateItemFromParsingName(last_accepted_file_dir.c_str(), NULL, IID_IShellItem, (void**)&defFolder);
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
				IShellItem *file_handler;
				hr = pfsd->GetResult(&file_handler);
				LPWSTR filename_holder=NULL;
				hr = file_handler->GetDisplayName(SIGDN_FILESYSPATH, &filename_holder); //@check other options for the flag
				DoSave(textControl, filename_holder);
				
				CoTaskMemFree(filename_holder);
				file_handler->Release();
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
	return hr;
}

void AddMenus(HWND hWnd) {

	LANGUAGE_MANAGER::Instance().AddMenuDrawingHwnd(hWnd);

	hMenu = CreateMenu();
	hFileMenu = CreateMenu();
	hFileMenuLang = CreateMenu();
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"");
	AMT(hMenu, (UINT_PTR)hFileMenu, LANG_MENU_FILE);

	AppendMenuW(hFileMenu, MF_STRING, OPEN_FILE, L"");
	AMT(hFileMenu, OPEN_FILE, LANG_MENU_OPEN);

	AppendMenuW(hFileMenu, MF_SEPARATOR, NULL, NULL);

	AppendMenuW(hFileMenu, MF_STRING, SAVE_FILE, L"");
	AMT(hFileMenu, SAVE_FILE, LANG_MENU_SAVE);

	AppendMenuW(hFileMenu, MF_STRING, SAVE_AS_FILE, L"");
	AMT(hFileMenu, SAVE_AS_FILE, LANG_MENU_SAVEAS);

	AppendMenuW(hFileMenu, MF_STRING, BACKUP_FILE, L"");
	AMT(hFileMenu, BACKUP_FILE, LANG_MENU_BACKUP);

	AppendMenuW(hFileMenu, MF_SEPARATOR, NULL, NULL);

	AppendMenuW(hFileMenu, MF_POPUP, (UINT_PTR)hFileMenuLang, L"");
	AMT(hFileMenu, (UINT_PTR)hFileMenuLang, LANG_MENU_LANGUAGE);
	
	AppendMenuW(hFileMenuLang, MF_STRING, LANGUAGE_MANAGER::LANGUAGE::ENGLISH , L"English");
	AppendMenuW(hFileMenuLang, MF_STRING, LANGUAGE_MANAGER::LANGUAGE::SPANISH, L"Español");

	HBITMAP bTick = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(TICK));
	HBITMAP bCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(CROSS));
	HBITMAP bEarth = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(EARTH));

	//TODO(fran): fix old bitmap code that uses wrong sizes and no transparency

	SetMenuItemBitmaps(hFileMenu, BACKUP_FILE, MF_BYCOMMAND,bCross,bTick);//1ºunchecked,2ºchecked
	CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);

	SetMenuItemBitmaps(hFileMenu, (UINT_PTR)hFileMenuLang, MF_BYCOMMAND, bEarth, bEarth);
	//https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getmenustate
	//https://docs.microsoft.com/es-es/windows/desktop/menurc/using-menus#using-custom-check-mark-bitmaps
	//https://www.daniweb.com/programming/software-development/threads/490612/win32-can-i-change-the-hbitmap-structure-for-be-transparent

	//TODO(fran): some automatic way to add this to all langs?
	SetMenuItemBitmaps(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::ENGLISH, MF_BYCOMMAND, NULL, bTick);
	SetMenuItemBitmaps(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::SPANISH, MF_BYCOMMAND, NULL, bTick);

	CheckMenuItem(hFileMenuLang, startup_locale, MF_BYCOMMAND | MF_CHECKED);

	SetMenu(hWnd, hMenu);
}

//Method for managing edit control special features (ie dropped files,...)
//there seems to be other ways like using SetWindowsHookExW or SetWindowLongPtrW 
LRESULT CALLBACK EditCatchDrop(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_DROPFILES:
		CatchDrop(wParam);
		return TRUE;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
//
//	if (Msg == WM_CTLCOLORSCROLLBAR) {
//		return (LRESULT)AppColors.ControlBk;
//	}
//	return DefSubclassProc(hWnd, Msg, wParam, lParam);
	switch (Msg) {
	case TCM_RESIZE:
	{
		SIZE* control_size = (SIZE*)wParam;

		MoveWindow(hWnd, TabOffset.leftOffset, TabOffset.topOffset
			, control_size->cx - TabOffset.rightOffset - TabOffset.leftOffset, control_size->cy - TabOffset.bottomOffset - TabOffset.topOffset, TRUE);
		//x & y remain fixed and only width & height change

		return TRUE;
	}
	default:return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

//Adds a new item to the tab control, inside of which will be an edit control
//·Populate text_data with the values you need except for any HWND
//Returns position of the new Tab Item or -1 if failed
int AddTab(HWND TabControl, int position, LPWSTR TabName, TEXT_INFO text_data) { //INFO: TabName only needs to be valid during the execution of this function

	RECT tab_rec,text_rec;
	GetClientRect(TabControl,&tab_rec);
	text_rec.left = TabOffset.leftOffset;
	text_rec.top = TabOffset.topOffset;
	text_rec.right = tab_rec.right - TabOffset.rightOffset;
	text_rec.bottom = tab_rec.bottom - TabOffset.bottomOffset;
	
	HWND TextControl = CreateWindowExW(NULL, L"Edit", NULL, WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL //|WS_VISIBLE
		, text_rec.left, text_rec.top, text_rec.right - text_rec.left, text_rec.bottom - text_rec.top
		,TabControl //IMPORTANT INFO TODO(fran): this is not right, the parent should be the individual tab not the whole control, not sure though if that control will show our edit control
		, NULL, NULL, NULL); 

	SendMessageW(TextControl, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);

	SendMessageW(TextControl, EM_SETLIMITTEXT, (WPARAM)MAX_TEXT_LENGTH, NULL); //Change default text limit of 32767 to a bigger value

	SetWindowSubclass(TextControl,EditProc,0,0);

	//TEXT_INFO* text_info = (TEXT_INFO*)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE| HEAP_ZERO_MEMORY,sizeof(TEXT_INFO));
	//Assert(text_info);
	//text_info->hText = TextControl;

	CUSTOM_TCITEM newItem;
	newItem.tab_info.mask = TCIF_TEXT | TCIF_PARAM;
	newItem.tab_info.pszText = TabName;
	//newItem.tab_info.cchTextMax = (wcslen(TabName)+1)*sizeof(*TabName); I think this is wrong and should be in chars/wchars not bytes
	newItem.tab_info.iImage = -1;
	newItem.extra_info = text_data;
	newItem.extra_info.hText = TextControl;

	return SendMessage(TabControl, TCM_INSERTITEM, position, (LPARAM)&newItem);
}

//Returns the current position of that tab or -1 if failed
int SaveAndDisableCurrentTab(HWND tabControl) {
	int index = SendMessage(tabControl, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM prev_item;
		prev_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
		//WCHAR buf[20] = { 0 };
		//prev_item.tab_info.pszText = buf;
		//prev_item.tab_info.cchTextMax = 20;
		BOOL res = SendMessage(tabControl, TCM_GETITEM, index, (LPARAM)&prev_item);
		if (res) {
			ShowWindow(prev_item.extra_info.hText, SW_HIDE);

			int text_length = GetWindowTextLengthW(hFile) + 1;//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea

			wstring text(text_length, L'\0');
			GetWindowTextW(hFile, &text[0], text_length);

			if (text[0]!=L'\0') {//Check in case there is invalid data on the controls
				//TODO(fran): make sure this check doesnt break anything in some specific case

				wcsncpy(prev_item.extra_info.filePath, text.c_str(), sizeof(prev_item.extra_info.filePath) / sizeof(prev_item.extra_info.filePath[0]));

				prev_item.extra_info.commentType = (COMMENT_TYPE)SendMessage(hOptions, CB_GETCURSEL, 0, 0);

				WCHAR temp[2] = {0};
				WCHAR initialChar;
				WCHAR finalChar;
				GetWindowTextW(hInitialChar, temp, 2); //tambien la funcion devuelve 0 si no agarra nada, podemos agregar ese check
				initialChar = temp[0];//vale '\0' si no tiene nada
				GetWindowTextW(hFinalChar, temp, 2);
				finalChar = temp[0];

				prev_item.extra_info.initialChar = initialChar;

				prev_item.extra_info.finalChar = finalChar;

				//Save back to the control
				prev_item.tab_info.mask = TCIF_PARAM; //just in case it changed for some reason
				SendMessage(tabControl, TCM_SETITEM, index, (LPARAM)&prev_item);
			}

		}
	}
	return index;
}

int EnableTab(const TEXT_INFO& text_data) {
	ShowWindow(text_data.hText, SW_SHOW);

	SetWindowTextW(hFile, text_data.filePath);//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea
	
	SendMessage(hOptions, CB_SETCURSEL, text_data.commentType, 0);

	WCHAR temp[2] = { 0 };
	temp[1] = L'\0';

	temp[0] = text_data.initialChar;
	SetWindowText(hInitialChar, temp);

	temp[0] = text_data.finalChar;
	SetWindowText(hFinalChar, temp);

	return 1;//TODO(fran): proper return
}

//Determines if a close button was pressed in any of the tab control's tabs and returns its zero based index
//p must be in client space of the tab control
//Returns -1 if no button was pressed
int TestCloseButton(HWND tabControl, CLOSEBUTTON closeButtonInfo, POINT p ) {
	//1.Determine which tab was hit and retrieve its rect
	//From what I understand the tab change happens before we get here, so for simplicity's sake we will only test against the currently selected tab.
	//If this didnt work we'd probably have to use the TCM_HITTEST message
	RECT item_rc;
	int item_index = SendMessage(tabControl, TCM_GETCURSEL, 0, 0);
	if (item_index != -1) {
		BOOL ret = SendMessage(tabControl, TCM_GETITEMRECT, SendMessage(tabControl, TCM_GETCURSEL, 0, 0), (LPARAM)&item_rc); //returns rc in "viewport" coords, aka in respect to tab control, so it's good
		
		if (ret) {
			//2.Adjust rect to close button position
			item_rc.left += RECTWIDTH(item_rc) - closeButtonInfo.icon.cx - closeButtonInfo.rightPadding;
			item_rc.right -= closeButtonInfo.rightPadding;
			LONG yChange = (RECTHEIGHT(item_rc) - closeButtonInfo.icon.cy) / 2;
			item_rc.top += yChange;
			item_rc.bottom -= yChange;

			//3.Determine if the close button was hit
			BOOL res = TestCollisionPointRect(p, item_rc);
			
			//4.Return index or -1
			if (res) return item_index;
		}

	}
	return -1;
}

void CleanTabRelatedControls() {//TODO(fran): probably should ask for a blank TEXT_INFO to some sort of constructor
	TEXT_INFO blank = { 0 };
	EnableTab(blank);
}

LRESULT CALLBACK TabProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	//INFO: if I ever feel like managing WM_PAINT: https://social.msdn.microsoft.com/Forums/expression/en-US/d25d08fb-acf1-489e-8e6c-55ac0f3d470d/tab-controls-in-win32-api?forum=windowsuidevelopment
	switch (Msg) {
	case TCM_RESIZETABS:
	{
		int item_count = SendMessage(hWnd, TCM_GETITEMCOUNT, 0, 0);
		if (item_count != 0) {
			for (int i = 0; i < item_count; i++) {
				CUSTOM_TCITEM item;
				item.tab_info.mask = TCIF_PARAM;
				BOOL ret = SendMessage(hWnd, TCM_GETITEM, i, (LPARAM)&item);
				if (ret) {
					RECT rc;
					GetClientRect(hWnd, &rc);
					SIZE control_size;
					control_size.cx = RECTWIDTH(rc);
					control_size.cy = RECTHEIGHT(rc);
					SendMessage(item.extra_info.hText, TCM_RESIZE, (WPARAM)&control_size, 0);
				}
			}
		}
		break;
	}
	case WM_DROPFILES:
	{
		CatchDrop(wParam);
		return TRUE;
	}
	case WM_ERASEBKGND:
	{
		HDC  hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hWnd, &rc);

		FillRect(hdc, &rc, AppColors.ControlBk);
		return 1;
	}
	case WM_CTLCOLOREDIT:
	{
		//if ((HWND)lParam == GetDlgItem(hWnd, EDIT_ID)) //TODO(fran): check we are changing the control we want
		if(TRUE)
		{
			SetBkColor((HDC)wParam, ColorFromBrush(AppColors.ControlBk));
			SetTextColor((HDC)wParam, ColorFromBrush(AppColors.ControlTxt));
			return (LRESULT)AppColors.ControlBk;
		}
		else return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_LBUTTONUP:
	{
		POINT p;
		p.x = GET_X_LPARAM(lParam);
		p.y = GET_Y_LPARAM(lParam);
		int index = TestCloseButton(hWnd, TabCloseButtonInfo, p);
		if (index != -1) {
			//Delete corresponding tab
			SendMessage(hWnd, TCM_DELETEITEM, index, 0); //After this no other messages are sent and the tab control's index gets set to -1
			//Now we have to send a TCM_SETCURSEL, but in the special case that no other tabs are left we should clean the controls
			int item_count = SendMessage(hWnd, TCM_GETITEMCOUNT, 0, 0);
			if (item_count == 0) CleanTabRelatedControls();									//if there's no tabs left clean the controls
			else if (index == item_count) SendMessage(hWnd, TCM_SETCURSEL, index - 1, 0);	//if the deleted tab was the rightmost one change to the one on the left
			else SendMessage(hWnd, TCM_SETCURSEL, index, 0);								//otherwise change to the tab that now holds the index that this one did(will select the one on its right)

		} 
		break;
	}
	case TCM_DELETEITEM:
	{
		//Since right now our controls (text editor) are not children of the particular tab they dont get deleted when that one does, so gotta do it manually
		int index = wParam;

		TEXT_INFO info = GetTabExtraInfo(index);
		//Any hwnd should be destroyed
		DestroyWindow(info.hText);

		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	//case WM_PARENTNOTIFY:
	//{
	//	switch (LOWORD(wParam)) {
	//	case WM_CREATE:
	//	{//Each time a new tab gets created this message is received and the lParam is the new HWND
	//		TESTTAB = (HWND)lParam;
	//		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	//	}
	//	default: return DefSubclassProc(hWnd, Msg, wParam, lParam);
	//	}
	//	return DefSubclassProc(hWnd, Msg, wParam, lParam);
	//}
	case TCM_SETCURSEL: //When sending SETCURSEL, TCN_SELCHANGING and TCN_SELCHANGE are no sent, therefore we have to do everything in one message here
	{
		SaveAndDisableCurrentTab(hWnd);

		CUSTOM_TCITEM new_item;
		new_item.tab_info.mask = TCIF_PARAM;
		BOOL ret = SendMessage(hWnd, TCM_GETITEM, wParam, (LPARAM)&new_item);
		if (ret) {
			EnableTab(new_item.extra_info);
		}

		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	default:
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}



void AddControls(HWND hWnd, HINSTANCE hInstance) {
	hFile = CreateWindowW(L"Static", L"", WS_VISIBLE | WS_CHILD | WS_BORDER//| SS_WHITERECT
			| ES_AUTOHSCROLL
			, 10, y_place, 664, 20, hWnd, NULL, NULL, NULL);
	//ChangeWindowMessageFilterEx(hFile, WM_DROPFILES, MSGFLT_ADD,NULL); y mas que habia
	
	hReadFile = CreateWindowExW(0, PROGRESS_CLASS, (LPWSTR)NULL, WS_CHILD
		, 10, y_place, 664, 20, hWnd, (HMENU)NULL, NULL, NULL);

	hRemoveCommentWith = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD
	, 25, y_place+34, 155, 20, hWnd, NULL, NULL, NULL);
	AWT(hRemoveCommentWith, LANG_CONTROL_REMOVECOMMENTWITH);


	hOptions = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD| CBS_DROPDOWNLIST|WS_TABSTOP
		, 195, y_place + 31/*30*/, 130, 90/*20*/, hWnd, (HMENU)COMBO_BOX, hInstance, NULL);
	//SetOptionsComboBox(hOptions, TRUE);
	ACT(hOptions, COMMENT_TYPE::brackets, LANG_CONTROL_CHAROPTIONS_BRACKETS);
	ACT(hOptions, COMMENT_TYPE::parenthesis, LANG_CONTROL_CHAROPTIONS_PARENTHESIS);
	ACT(hOptions, COMMENT_TYPE::braces, LANG_CONTROL_CHAROPTIONS_BRACES);
	ACT(hOptions, COMMENT_TYPE::other, LANG_CONTROL_CHAROPTIONS_OTHER);
	SendMessageW(hOptions, CB_SETCURSEL, 0, 0);
	SetWindowSubclass(hOptions, ComboProc, 0, 0);
	//WCHAR explain_combobox[] = L"Also separates the lines";
	//CreateToolTip(COMBO_BOX, hWnd, explain_combobox);

	hInitialText = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD |WS_DISABLED
		, 78, y_place + 65, 105, 20, hWnd, NULL, NULL, NULL);
	AWT(hInitialText, LANG_CONTROL_INITIALCHAR);

	hInitialChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER |ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_place + 64, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hInitialChar, EM_LIMITTEXT, 1, 0);

	hFinalText = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | WS_DISABLED
		, 78, y_place + 95, 105, 20, hWnd, NULL, NULL, NULL);
	AWT(hFinalText, LANG_CONTROL_FINALCHAR);

	hFinalChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER| WS_TABSTOP| WS_DISABLED
		, 195, y_place + 94, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hFinalChar, EM_LIMITTEXT, 1, 0);

	hRemove = CreateWindowW(L"Button", NULL, WS_VISIBLE | WS_CHILD| WS_TABSTOP
		, 256, y_place + 74, 70, 30, hWnd, (HMENU)REMOVE, NULL, NULL);
	AWT(hRemove, LANG_CONTROL_REMOVE);
	SetWindowSubclass(hRemove, ButtonProc, 0, 0);

	//hRemoveProgress = CreateWindowExW(0,PROGRESS_CLASS,(LPWSTR)NULL, WS_CHILD
	//	, 256, y_place + 74, 70, 30,hWnd, (HMENU)NULL,NULL,NULL);

	//TODO(fran): init common controls for tab control
	//INFO:https://docs.microsoft.com/en-us/windows/win32/controls/tab-controls
	TextContainer = CreateWindowExW(WS_EX_ACCEPTFILES, WC_TABCONTROL, NULL, WS_CHILD | WS_VISIBLE | TCS_FORCELABELLEFT | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH
							, 10, y_place + 134, 664 - 50, 588, hWnd, (HMENU)TABCONTROL, NULL, NULL);
	TabCtrl_SetItemExtra(TextContainer, sizeof(TEXT_INFO));
	int tabWidth = 100,tabHeight=20;
	SendMessage(TextContainer, TCM_SETITEMSIZE, 0, MAKELONG(tabWidth, tabHeight));
	SetWindowSubclass(TextContainer, TabProc,0,0);

	TabCloseButtonInfo.icon.cx = tabHeight*.6f;
	TabCloseButtonInfo.icon.cy = TabCloseButtonInfo.icon.cx;
	TabCloseButtonInfo.rightPadding = (tabHeight-TabCloseButtonInfo.icon.cx)/2.f;

	SendMessageW(hFile, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemoveCommentWith, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hOptions, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hInitialText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hInitialChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hFinalText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hFinalChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemove, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessageW(hRemove, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(TextContainer, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
}

void EnableOtherChar(bool op) {
	EnableWindow(hInitialText, op);
	EnableWindow(hInitialChar, op);
	EnableWindow(hFinalText, op);
	EnableWindow(hFinalChar, op);	
}

inline BOOL isMultiFile(LPWSTR file, WORD offsetToFirstFile) { //TODO(fran): lambda function
	return file[max(offsetToFirstFile - 1,0)] == L'\0';//TODO(fran): is this max useful? if [0] is not valid I think it will fail anyway
}

void ChooseFile(wstring ext) {
	//TODO(fran): support for folder selection?
	//TODO(fran): check for mem-leaks, the moment I select the open-file menu 10MB of ram are assigned for some reason
	WCHAR name[MAX_PATH_LENGTH]; //TODO(fran): I dont think this is big enough
	name[0] = L'\0';
	OPENFILENAMEW new_file;
	ZeroMemory(&new_file, sizeof(OPENFILENAMEW));

	new_file.lStructSize = sizeof(OPENFILENAMEW);
	new_file.hwndOwner = NULL;
	new_file.lpstrFile = name; //used for the default file selected
	new_file.nMaxFile = MAX_PATH_LENGTH; //TODO(fran): this will not be enough with multi-file (Pd. dont forget lpstrFile and name[])
	wstring filter = RS(LANG_CHOOSEFILE_FILTER_ALL_1) + L'\0' + RS(LANG_CHOOSEFILE_FILTER_ALL_2) + L'\0' 
		+ RS(LANG_CHOOSEFILE_FILTER_SRT_1) + L'\0' + RS(LANG_CHOOSEFILE_FILTER_SRT_2) + L'\0' + L'\0';

	new_file.lpstrFilter = filter.c_str();
	
	new_file.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST; //TODO(fran): check other useful flags
	
	new_file.lpstrDefExt = ext.c_str();
	if (last_accepted_file_dir != L"\0") new_file.lpstrInitialDir = last_accepted_file_dir.c_str();
	else new_file.lpstrInitialDir = NULL; //@este if genera bug si initialDir tiene data y abris el buscador pero no agarras nada

	if (GetOpenFileNameW(&new_file) != 0) { //si el usuario elige un archivo

		//TODO(fran): I know that for multi-file the string is terminated with two NULLs, from my tests when it is single file this also happens, can I guarantee
		//this always happens? Meanwhile I'll use IsMultiFile
		BOOL multifile = isMultiFile(new_file.lpstrFile, new_file.nFileOffset);

		std::vector<std::wstring> files;

		if (multifile) {
			for (int i = new_file.nFileOffset - 1;;i++) {
				if (new_file.lpstrFile[i]==L'\0') {
					if (new_file.lpstrFile[i + 1] == L'\0') break;
					else {
						wstring onefile = new_file.lpstrFile;//Will only take data until the first \0 it finds
						onefile += L'\\';
						onefile += &new_file.lpstrFile[i+1];
						files.push_back(onefile);
						i += wcslen(&new_file.lpstrFile[i + 1]);//TODO(fran): check this is actually faster
					}
				}
			}
		}
		else files.push_back(new_file.lpstrFile);
		
		for (std::wstring const& onefile : files) {
			if (FileOrDirExists(onefile)) AcceptedFile(onefile);
			else  MessageBoxW(NULL, RCS(LANG_CHOOSEFILE_ERROR_NONEXISTENT), RCS(LANG_ERROR), MB_ICONERROR);
		}
	}
}

//Returns complete path of all the files found, if a folder is found files are searched inside it
std::vector<wstring> GetFiles(LPCWSTR s)//, int dir_lenght)
{
	std::vector<wstring> files; //capaz deberia ser = L"";
	for (auto& p : std::experimental::filesystem::recursive_directory_iterator(s)) {
		//if (p.status().type() == std::experimental::filesystem::file_type::directory) files.push_back(p.path().wstring().substr(dir_lenght, string::npos));
		//else
		//if (!p.path().extension().compare(".srt")) files.push_back(p.path().wstring().substr(dir_lenght, string::npos));
		if (!p.path().extension().compare(".srt")) files.push_back(p.path().wstring());
	}
	return files;
}

void CatchDrop(WPARAM wParam) { //TODO(fran): check for valid file extesion
	HDROP hDrop = (HDROP)wParam;
	WCHAR lpszFile[MAX_PATH_LENGTH] = { 0 };
	lpszFile[0] = '\0';
	UINT file_count = 0;

	file_count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, NULL);

	for (int i = 0; i < file_count; i++) {
		UINT requiredChars = DragQueryFileW(hDrop, i, NULL, NULL);//INFO: does NOT include terminating NULL char
		Assert( requiredChars < (sizeof(lpszFile) / sizeof(lpszFile[0])) );

		if (DragQueryFileW(hDrop, i, lpszFile, sizeof(lpszFile) / sizeof(lpszFile[0]))) {//retrieve the string

			if (fs::is_directory(fs::path(lpszFile))) {//user dropped a folder
				std::vector<wstring>files = GetFiles(lpszFile);
				//GetFiles2(lpszFile);

				for (wstring const& file : files) {
					AcceptedFile(file); //process the new file
				}

				//MessageBoxW(NULL, RCS(LANG_DRAGDROP_ERROR_FOLDER), L"", MB_ICONERROR); 
			} else AcceptedFile(lpszFile); //process the new file

		}
		else MessageBoxW(NULL, RCS(LANG_DRAGDROP_ERROR_FILE), RCS(LANG_ERROR), MB_ICONERROR);
	}

	DragFinish(hDrop); //free mem
}

ENCODING GetTextEncoding(wstring filename) { //analize for ascii,utf8,utf16
	
	HANDLE hFile;
	DWORD  dwBytesRead = 0;
	BYTE   header[4] = { 0 };

	hFile = CreateFile(filename.c_str(),// file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	Assert(hFile != INVALID_HANDLE_VALUE);

	BOOL read_res = ReadFile(hFile, header, 4, &dwBytesRead, NULL);
	Assert(read_res);

	CloseHandle(hFile);

	if (header[0] == 0xEF && header[1] == 0xBB && header[2] == 0xBF)	return ENCODING::UTF8; //File has UTF8 BOM
	else if (header[0] == (char)0xFF && header[1] == (char)0xFE)		return ENCODING::UTF16; //File has UTF16LE BOM, INFO: this could actually be UTF32LE but I've never seen a subtitle file enconded in utf32 so no real need to do the extra checks
	else if (header[0] == (char)0xFE && header[1] == (char)0xFF)		return ENCODING::UTF16; //File has UTF16BE BOM
	else {//Do manual check

		//Check for UTF8

		ifstream ifs(filename); //TODO(fran): can ifstream open wstring (UTF16 encoded) filenames?
		Assert(ifs);

		istreambuf_iterator<char> it(ifs.rdbuf());
		istreambuf_iterator<char> eos;

		bool isUTF8 = utf8::is_valid(it, eos); //there's also https://unicodebook.readthedocs.io/guess_encoding.html

		ifs.close();

		if (isUTF8) return ENCODING::UTF8; //File is valid UTF8

		//Check for UTF16

		hFile = CreateFile(filename.c_str(),GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); //Open file for reading
		Assert(hFile != INVALID_HANDLE_VALUE);

		BYTE fixedBuffer[50000] = {0}; //We dont need the entire file, just enough data for the functions to be as accurate as possible. Pd. this is probably too much already
		read_res = ReadFile(hFile, fixedBuffer, 50000, &dwBytesRead, NULL);
		Assert(read_res);

		CloseHandle(hFile);

		AutoIt::Common::TextEncodingDetect textDetect;
		bool isUTF16 = textDetect.isUTF16(fixedBuffer,dwBytesRead);

		if (isUTF16) return ENCODING::UTF16;

		//Give up and return ANSI
		return ENCODING::ANSI;
	}
}

COMMENT_TYPE CommentTypeFound(wstring &text) {
	if (text.find_first_of(L'{') != wstring::npos) {
	//	MessageBoxW(hWnd, RCS(LANG_COMMENT_FOUND) + L' ' + L'{', L"", MB_OK);
		return COMMENT_TYPE::braces;
	}
	else if (text.find_first_of(L'[') != wstring::npos) {
	//	MessageBoxW(hWnd, RCS(LANG_COMMENT_FOUND) + L' ' + L'[', L"", MB_OK);
		return COMMENT_TYPE::brackets;
	}
	else if (text.find_first_of(L'(') != wstring::npos) {
	//	wstring msg = RS(LANG_COMMENT_FOUND) + L" " + L"(";
	//	MessageBoxW(hWnd, msg.c_str(), L"", MB_OK);
		return COMMENT_TYPE::parenthesis;
	}
	else {
	//	MessageBoxW(hWnd, RCS(LANG_COMMENT_NOTFOUND), L"", MB_ICONERROR);
		return COMMENT_TYPE::other;
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="filepath">path to the file you want read</param>
/// <param name="text">place where the read file's text will be stored</param>
/// <returns>TRUE if successful, FALSE otherwise</returns>
BOOL ReadText(wstring filepath, wstring& text) {

	unsigned char encoding = GetTextEncoding(filepath);

	wifstream file(filepath, ios::binary);

	if (file.is_open()) {

		//set encoding for getline
		if (encoding == UTF8)
			file.imbue(locale(std::locale::empty(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
			//int a;
			//maybe the only thing needed when utf8 is detectedd is to remove the BOM if it is there
		else if (encoding == UTF16)
			file.imbue(locale(std::locale::empty(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));
		//if encoding is ANSI we do nothing

		wstringstream buffer;
		buffer << file.rdbuf();
		text = buffer.str();

		file.close();
		return TRUE;
	}
	else return FALSE;
}

/// <summary>
/// Detects the file's format through different techniques
/// <para>If the filename has an extesion then that will be trusted as correct</para>
/// </summary>
/// <param name="filename">Full path of the file</param>
/// <returns>The presumed file format</returns>
FILE_FORMAT GetFileFormat(wstring& const filename) {
	size_t extesion_pos = 0;
	extesion_pos = filename.find_last_of(L'.', wstring::npos); //Look for file extesion
	if (extesion_pos != wstring::npos && (extesion_pos + 1) < filename.length()) {
		//We found and extension and checked that there's at least one char after the ".", lets see if we support it
		//TODO(fran): create some sort of a map FILE_FORMAT-wstring eg FILE_FORMAT::SRT - L"srt" and a way to enumerate every FILE_FORMAT

		//TODO(fran): make every character in the filename that we're going to compare lowercase
		WCHAR srt[] = L"srt";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(srt) - 1, srt) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(srt) - 1)) 
			return FILE_FORMAT::SRT;
		//TODO(fran): check the -1 is correct
		WCHAR ssa[] = L"ssa";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(ssa) - 1, ssa) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(ssa) - 1)) 
			return FILE_FORMAT::SSA;

		WCHAR ass[] = L"ass";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(ass) - 1, ass) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(ass) - 1))
			return FILE_FORMAT::SSA;
	}
	else {
		//File has no extension, do manual checking. Note that we are either checking the extesion or using manual checks but not both,
		//the idea behind it is that if the file has an extension and we couldn't find it between our supported list then we dont support it,
		//since, for now, we trust that what the extension says is true

		//TODO(fran): manual checking, aka reading the file

	}
	
	//If everything else fails fallback to srt //TODO(fran): we could also reject the file, adding an INVALID into FILE_FORMAT
	return FILE_FORMAT::SRT;
}

//TODO(fran): we could try to offload the entire procedure of AcceptedFile to a new thread so in case we receive multiple files we process them in parallel
void AcceptedFile(wstring filename) {

	//save file dir
	last_accepted_file_dir = filename.substr(0, filename.find_last_of(L"\\")+1);

	//save file name+ext
	wstring accepted_file_name_with_ext = filename.substr(filename.find_last_of(L"\\")+1);

#if 0
	//show file contents (new thread)
	wstring text;
	PGLP parameters = (PGLP)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLP));
	parameters->text = &text;
	wcsncpy(parameters->file, filename.c_str(), sizeof(parameters->file) / sizeof(parameters->file[0]));
	HANDLE hReadTextThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadTextThread, parameters, 0, NULL);
	
	//show filename in top bar
	//SetWindowTextW(hFile, accepted_file.c_str());
	
	//wait for thread to finish
	WaitForSingleObject(hReadTextThread, INFINITE);
	//delete thread and parameters
	CloseHandle(hReadTextThread);
	HeapFree(GetProcessHeap(), 0, parameters);
	parameters = NULL;
#endif
	wstring text;
	ReadText(filename, text);

	TEXT_INFO text_data;
	wcsncpy(text_data.filePath, filename.c_str(), sizeof(text_data.filePath) / sizeof(text_data.filePath[0]));

	int pos = GetNextTabPos();//TODO(fran): careful with multithreading here

	text_data.commentType = CommentTypeFound(text);

	text_data.fileFormat = GetFileFormat(filename);

	int res = AddTab(TextContainer, pos, (LPWSTR)accepted_file_name_with_ext.c_str(), text_data);

	if (res != -1) {
		//TODO(fran): should we check that pos == res? I dont think so

		ProperLineSeparation(text);

		TEXT_INFO new_text_data = GetTabExtraInfo(res);

		SetWindowTextW(new_text_data.hText, text.c_str());

		//enable buttons and text editor
		EnableWindow(hRemove, TRUE); //TODO(fran): this could also be a saved parameter

		ChangeTabSelected(res);
	}
	else Assert(0);

}

void CreateInfoFile(HWND mainWindow){
	wstring info_save_file = GetInfoPath();

	RECT rect;
	GetWindowRect(mainWindow, &rect);

	BOOL dir_ret = CreateDirectoryW((LPWSTR)GetInfoDirectory().c_str(), NULL);

	if (!dir_ret) {
		Assert(GetLastError() != ERROR_PATH_NOT_FOUND);
	}

	_wremove(info_save_file.c_str()); //@I dont know if it's necessary to check if the file exists
	wofstream create_info_file(info_save_file);
	if (create_info_file.is_open()) {
		create_info_file.imbue(locale(create_info_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, generate_header>));
		create_info_file << info_parameters[RC_LEFT] << rect.left << L"\n";			//posx
		create_info_file << info_parameters[RC_TOP] << rect.top << L"\n";			//posy
		create_info_file << info_parameters[RC_RIGHT] << rect.right << L"\n";			//sizex
		create_info_file << info_parameters[RC_BOTTOM] << rect.bottom << L"\n";			//sizey
		create_info_file << info_parameters[DIR] << last_accepted_file_dir << L"\n";	//directory of the last valid file directory
		create_info_file << info_parameters[BACKUP] << (bool)(GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED) << L"\n"; //SDH
		create_info_file << info_parameters[LOCALE] << LANGUAGE_MANAGER::Instance().GetCurrentLanguage() << L"\n"; //global locale
		//@los otros caracteres??(quiza deberiamos guardarlos, puede que moleste)
		create_info_file.close();
		//SetFileAttributes(info_save_file.c_str(), FILE_ATTRIBUTE_HIDDEN);
	}
	//@@it still happens that some times the positions and sizes are saved wrong, investigate!!
}

void removeInitialWhiteSpace(wstring &text){
	while(iswspace(text[0]))text.erase(0, 1);
}

void AddInfoParameters(wstring parameters[]) {//@the debugger only shows the first parameter,why??
	//puts the new data into the global variables

	if (parameters[RC_LEFT] != L"") {
		int rc_left = stoi(parameters[RC_LEFT]);
		if (0 <= rc_left && rc_left <= GetSystemMetrics(SM_CXVIRTUALSCREEN))
			rcMainWnd.left = rc_left;
	}

	if (parameters[RC_TOP] != L"") {
		int rc_top = stoi(parameters[RC_TOP]);
		if (0 <= rc_top && rc_top <= GetSystemMetrics(SM_CYVIRTUALSCREEN))
			rcMainWnd.top = rc_top;
	}

	if (parameters[RC_RIGHT] != L"") {
		int rc_right = stoi(parameters[RC_RIGHT]);
		if (0 <= rc_right)
			rcMainWnd.right = rc_right;
	}

	if (parameters[RC_BOTTOM] != L"") {
		int rc_bottom = stoi(parameters[RC_BOTTOM]);
		if (0 <= rc_bottom)
			rcMainWnd.bottom = rc_bottom;
	}

	if (parameters[DIR] != L"") {
		last_accepted_file_dir = parameters[DIR];
	}

	if (parameters[BACKUP] != L"") {
		Backup_Is_Checked = stoi(parameters[BACKUP]);
	}

	if (parameters[LOCALE] != L"") {
		startup_locale = (LANGUAGE_MANAGER::LANGUAGE)stoi(parameters[LOCALE]);

	}

}

void GetInfoParameters(wstring info) {
	
	wstring parameters[NRO_INFO_PARAMETERS];

	wstring temp_info=L"";

	size_t initial_pos = 0;
	size_t final_pos = 0;

	//@remember that stoi throws exceptions!

	for (int param = 0; param < NRO_INFO_PARAMETERS; param++) {

		initial_pos = info.find(info_parameters[param], 0);
		final_pos = info.find(L'\n', initial_pos);

		if (initial_pos != string::npos) {

			initial_pos += wcslen(info_parameters[param]);//after the =

			if(final_pos!=string::npos) 
				temp_info = wstring(info.begin() + initial_pos, info.begin() + final_pos);
			else 
				temp_info = wstring(info.begin() + initial_pos, info.end());//@check this
			removeInitialWhiteSpace(temp_info);
				parameters[param] = temp_info;//@check that this works
		}

	}

	AddInfoParameters(parameters);

}

void CheckInfoFile() {	//changes default values if file exists
	//@Check that the values are valid, eg for pos: 0 < x < [monitor height]
	//@@Check that the values are there, if not I'm filling the vars with nonsense
	wstring dir = GetInfoPath();
	wifstream info_file(dir);
	if (info_file.is_open()) {
		info_file.imbue(locale(info_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
		
		wstringstream buffer;
		buffer << info_file.rdbuf();
		//wstring info = buffer.str();

		info_file.close();
		
		GetInfoParameters(buffer.str());
		
	}
}

void ResizeWindows(HWND mainWindow) {
	RECT rect;
	GetWindowRect(mainWindow, &rect);
	MoveWindow(hFile, 10, y_place, RECTWIDTH(rect) - 36, 20, TRUE);
	//@No se si actualizar las demas
	MoveWindow(TextContainer, 10, y_place + 134, RECTWIDTH(rect) - 36, RECTHEIGHT(rect) - 212, TRUE);
	SendMessage(TextContainer, TCM_RESIZETABS, 0, 0);
}

LRESULT CALLBACK ButtonProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	static BOOL MouseOver = false;
	static HWND CurrentMouseOverButton;

	switch (Msg) {
	case WM_MOUSEHOVER:
	{
		//force button to repaint and specify hover or not
		if (MouseOver && CurrentMouseOverButton == hWnd) break;
		MouseOver = true;
		CurrentMouseOverButton = hWnd;
		InvalidateRect(hWnd, 0, 1);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_MOUSELEAVE:
	{
		MouseOver = false;
		CurrentMouseOverButton = NULL;
		InvalidateRect(hWnd, 0, 1);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		//TODO(fran): We are tracking the mouse every single time it moves, kind of suspect solution
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = hWnd;

		TrackMouseEvent(&tme);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_PAINT://TODO(fran): I think we should handle erasebkgnd cause there some weird problems happen, we are patching them by calling update every time we change the control from the outside
	{
		PAINTSTRUCT ps; //TODO(fran): we arent using the rectangle from the ps, I think we should for performance
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rc; GetClientRect(hWnd, &rc);
		//int controlID = GetDlgCtrlID(hWnd);

		WORD ButtonState = SendMessageW(hWnd, BM_GETSTATE, 0, 0);

		if (ButtonState & BST_PUSHED) {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBkPush));
			FillRect(hdc, &rc, AppColors.ControlBkPush);
		}
		else if (MouseOver && CurrentMouseOverButton == hWnd) {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBkMouseOver));
			FillRect(hdc, &rc, AppColors.ControlBkMouseOver);
		}
		else {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBk));
			FillRect(hdc, &rc, AppColors.ControlBk);
		}

		int borderSize = max(1, RECTHEIGHT(rc)*.06f);

		HPEN pen = CreatePen(PS_SOLID, borderSize, ColorFromBrush(AppColors.ControlTxt)); //para el borde

		HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));//para lo de adentro
		HPEN oldpen = (HPEN)SelectObject(hdc, pen);

		//Border
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

		SelectObject(hdc, oldbrush);
		SelectObject(hdc, oldpen);
		DeleteObject(pen);

		DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
		if (style & BS_ICON) {//Here will go buttons that only have an icon
			//For this app we dont need buttons with icons
		}
		else { //Here will go buttons that only have text
			HFONT font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
			if (font) {//if font == NULL then it is using system font(default I assume)
				(HFONT)SelectObject(hdc, (HGDIOBJ)(HFONT)font);
			}
			SetTextColor(hdc, ColorFromBrush(AppColors.ControlTxt));
			WCHAR Text[40];
			int len = SendMessage(hWnd, WM_GETTEXT,
				ARRAYSIZE(Text), (LPARAM)Text);

			TEXTMETRIC tm;
			GetTextMetrics(hdc, &tm);
			// Calculate vertical position for the item string so that it will be vertically centered
			int yPos = (rc.bottom + rc.top - tm.tmHeight) / 2;

			SetTextAlign(hdc, TA_CENTER);
			//int xPos = ((rc.right - rc.left) - tm.tmAveCharWidth*len)/2; //not good enough
			int xPos = (rc.right - rc.left) / 2;
			TextOut(hdc, xPos, yPos, Text, len);
		}
		EndPaint(hWnd, &ps);
		//test

		return 0;
	}
	default:
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ComboProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	static BOOL MouseOverCombo = FALSE;
	static HWND CurrentMouseOverCombo;

	switch (Msg)
	{
	//case WM_ERASEBKGND:
	//{
	//	HDC  hdc = (HDC)wParam;
	//	RECT rc;
	//	GetClientRect(hWnd, &rc);

	//	FillRect(hdc, &rc, AppColors.ControlBk);
	//	return 1;
	//}
	case CB_SETCURSEL:
	{
		EnableOtherChar(wParam == COMMENT_TYPE::other);
		InvalidateRect(hWnd, NULL, TRUE); //Simple hack ->//TODO(fran): parts of the combo dont update and stay white when the tab tells it to change
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_MOUSEHOVER:
	{
		//force button to repaint and specify hover or not
		if (MouseOverCombo && CurrentMouseOverCombo == hWnd) break;
		MouseOverCombo = true;
		CurrentMouseOverCombo = hWnd;
		InvalidateRect(hWnd, 0, 1);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_MOUSELEAVE:
	{
		MouseOverCombo = false;
		CurrentMouseOverCombo = NULL;
		InvalidateRect(hWnd, 0, 1);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		//TODO(fran): We are tracking the mouse every single time it moves, kind of suspect solution
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = hWnd;

		TrackMouseEvent(&tme);
		return DefSubclassProc(hWnd, Msg, wParam, lParam);
	}
	//case CBN_DROPDOWN://lets us now that the list is about to be show, therefore the user clicked us
	case WM_PAINT:
	{
		DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
		if (!(style & CBS_DROPDOWNLIST))
			break;

		RECT rc;
		GetClientRect(hWnd, &rc);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		BOOL ButtonState = SendMessageW(hWnd, CB_GETDROPPEDSTATE, 0, 0);
		if (ButtonState) {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBkPush));
			FillRect(hdc, &rc, AppColors.ControlBkPush);
		}
		else if (MouseOverCombo && CurrentMouseOverCombo == hWnd) {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBkMouseOver));
			FillRect(hdc, &rc, AppColors.ControlBkMouseOver);
		}
		else {
			SetBkColor(hdc, ColorFromBrush(AppColors.ControlBk));
			FillRect(hdc, &rc, AppColors.ControlBk);
		}

		RECT client_rec;
		GetClientRect(hWnd, &client_rec);

		HPEN pen = CreatePen(PS_SOLID, max(1, (RECTHEIGHT(client_rec))*.01f), ColorFromBrush(AppColors.ControlTxt)); //para el borde

		HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));//para lo de adentro
		HPEN oldpen = (HPEN)SelectObject(hdc, pen);

		SelectObject(hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
		//SetBkColor(hdc, bkcolor);
		SetTextColor(hdc, ColorFromBrush(AppColors.ControlTxt));

		//Border
		Rectangle(hdc, 0, 0, rc.right, rc.bottom);

		/*
		if (GetFocus() == hWnd)
		{
			//INFO: with this we know when the control has been pressed
			RECT temp = rc;
			InflateRect(&temp, -2, -2);
			DrawFocusRect(hdc, &temp);
		}
		*/
		int DISTANCE_TO_SIDE = 5;

		int index = SendMessage(hWnd, CB_GETCURSEL, 0, 0);
		if (index >= 0)
		{
			int buflen = SendMessage(hWnd, CB_GETLBTEXTLEN, index, 0);
			TCHAR *buf = new TCHAR[(buflen + 1)];
			SendMessage(hWnd, CB_GETLBTEXT, index, (LPARAM)buf);
			rc.left += DISTANCE_TO_SIDE;
			DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			delete[]buf;
		}
		RECT r;
		SIZE icon_size;
		GetClientRect(hWnd, &r);
		icon_size.cy = (r.bottom - r.top)*.6f;
		icon_size.cx = icon_size.cy;

		HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
		//LONG icon_id = GetWindowLongPtr(hWnd, GWL_USERDATA);
		//TODO(fran): we could set the icon value on control creation, use GWL_USERDATA
		HICON combo_dropdown_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(COMBO_ICON), IMAGE_ICON, icon_size.cx, icon_size.cy, LR_SHARED);

		if (combo_dropdown_icon) {
			DrawIconEx(hdc, r.right - r.left - icon_size.cx - DISTANCE_TO_SIDE, ((r.bottom - r.top) - icon_size.cy) / 2,
				combo_dropdown_icon, icon_size.cx, icon_size.cy, 0, NULL, DI_NORMAL);//INFO: changing that NULL gives the option of "flicker free" icon drawing, if I need
			DestroyIcon(combo_dropdown_icon);
		}

		SelectObject(hdc, oldpen);
		SelectObject(hdc, oldbrush);
		//DeleteObject(brush);
		DeleteObject(pen);

		EndPaint(hWnd, &ps);
		return 0;
	}

	//case WM_NCDESTROY:
	//{
	//	RemoveWindowSubclass(hWnd, this->ComboProc, uIdSubclass);
	//	break;
	//}

	}

	return DefSubclassProc(hWnd, Msg, wParam, lParam);
}

vec2d GetDPI(HWND hwnd)//https://docs.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
{
	HDC hdc = GetDC(hwnd);
	vec2d dpi;
	dpi.x = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	dpi.y = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(hwnd, hdc);
	return dpi;
}

void MenuChangeLang(LANGUAGE_MANAGER::LANGUAGE lang, LANGUAGE_MANAGER::LANGUAGE oldLang) {
	LCID ret = LANGUAGE_MANAGER::Instance().ChangeLanguage(lang);

	if (lang == oldLang)return;

	//Check this one
	CheckMenuItem(hFileMenu, lang, MF_BYCOMMAND | MF_CHECKED);

	//Uncheck the old lang
	CheckMenuItem(hFileMenu, oldLang, MF_BYCOMMAND | MF_UNCHECKED);
}

//TODO(fran): DPI awareness
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		AddMenus(hWnd);
		AddControls(hWnd, (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE));
		if (Backup_Is_Checked)CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);
		else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);

		break;
	}
	case WM_CTLCOLORLISTBOX:
	{
		HDC comboboxDC = (HDC)wParam;
		SetBkColor(comboboxDC, ColorFromBrush(AppColors.ControlBk));
		SetTextColor(comboboxDC, ColorFromBrush(AppColors.ControlTxt));

		return (INT_PTR)AppColors.ControlBk;
	}
	case WM_CTLCOLORSTATIC:
	{
		if (TRUE) //TODO(fran): check we are changing the right controls
		{
			//TODO(fran): there's something wrong with Initial & Final character controls when they are disabled, documentation says windows always uses same color
			//text when window is disabled but it looks to me that it is using both this and its own color
			SetBkColor((HDC)wParam, ColorFromBrush(AppColors.ControlBk));
			SetTextColor((HDC)wParam, ColorFromBrush(AppColors.ControlTxt));
			return (LRESULT)AppColors.ControlBk;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_DRAWITEM:
	{
		switch (wParam) {//wParam specifies the identifier of the control that needs painting
		case TABCONTROL:
		{
			DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lParam;

			switch (item->itemAction) {//Determines what I have to do with the control
			case ODA_DRAWENTIRE://Draw everything
			case ODA_FOCUS://Control lost or gained focus, check itemState
			case ODA_SELECT://Selection status changed, check itemState
				//Fill background //TODO(fran): the original background is still visible for some reason
				FillRect(item->hDC, &item->rcItem, AppColors.ControlBk);
				//IMPORTANT INFO: this is no region set so I can actually draw to the entire control, useful for going over what the control draws
				
				UINT index = item->itemID;

				//Draw text
				wstring text = GetTabTitle(index);

				TEXTMETRIC tm;
				GetTextMetrics(item->hDC, &tm);
				// Calculate vertical position for the item string so that it will be vertically centered
				
				int yPos = (item->rcItem.bottom + item->rcItem.top - tm.tmHeight) / 2;

				int xPos = item->rcItem.left + RECTWIDTH(item->rcItem)*.1f;


				//SIZE text_size;
				//GetTextExtentPoint32(item->hDC, text.c_str(), text.length(), &text_size);

				//INFO TODO: probably simpler is to just mark a region and then unmark it, not sure how it will work for animation though
				//also I'm not sure how this method handles other unicode chars, ie japanese
				int max_count;
				SIZE fulltext_size;
				RECT modified_rc = item->rcItem;
				modified_rc.left = xPos;
				modified_rc.right -= (TabCloseButtonInfo.rightPadding + TabCloseButtonInfo.icon.cx);
				BOOL res = GetTextExtentExPoint(item->hDC, text.c_str(), text.length()
					, RECTWIDTH(modified_rc)/*TODO(fran):This should be DPI aware*/, &max_count, NULL, &fulltext_size);
				Assert(res);
				int len = max_count;


				COLORREF old_bk_color = GetBkColor(item->hDC);
				COLORREF old_txt_color = GetTextColor(item->hDC);

				SetBkColor(item->hDC, ColorFromBrush(AppColors.ControlBk));
				SetTextColor(item->hDC, ColorFromBrush(AppColors.ControlTxt));
				TextOut(item->hDC, xPos, yPos, text.c_str(), len);

				//Reset hdc as it was before our painting
				SetBkColor(item->hDC, old_bk_color);
				SetTextColor(item->hDC, old_txt_color);

				//Draw close button 

				POINT button_topleft;
				button_topleft.x = item->rcItem.right - TabCloseButtonInfo.rightPadding - TabCloseButtonInfo.icon.cx;
				button_topleft.y = item->rcItem.top + (RECTHEIGHT(item->rcItem) - TabCloseButtonInfo.icon.cy) / 2;

				//RECT r;
				//r.left = button_topleft.x;
				//r.right = r.left + TabCloseButtonInfo.icon.cx;
				//r.top = button_topleft.y; 
				//r.bottom = r.top + TabCloseButtonInfo.icon.cy;
				//FillRect(item->hDC, &r, AppColors.ControlTxt);
				
				HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
				//LONG icon_id = GetWindowLongPtr(hWnd, GWL_USERDATA);
				//TODO(fran): we could set the icon value on control creation, use GWL_USERDATA
				HICON close_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(CROSS_ICON), IMAGE_ICON, TabCloseButtonInfo.icon.cx, TabCloseButtonInfo.icon.cy, LR_SHARED);

				if (close_icon) {
					DrawIconEx(item->hDC, button_topleft.x, button_topleft.y,
						close_icon, TabCloseButtonInfo.icon.cx, TabCloseButtonInfo.icon.cy, 0, NULL, DI_NORMAL);//INFO: changing that NULL gives the option of "flicker free" icon drawing, if I need
					DestroyIcon(close_icon);
				}

				//TODO?(fran): add a plus sign for the last tab and when pressed launch choosefile? or no plus sign at all since we dont really need it

				break;
			}

			return TRUE;
		}
		default: return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
	case WM_COMMAND:
	{
		// Analizar los mensajes de mis controles:
		switch (LOWORD(wParam))
		{

		case OPEN_FILE:
		case ID_OPEN:
		{
			ChooseFile(L"srt");

			break;
		}
		case BACKUP_FILE:
		{
			if (GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED)
				CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);
			else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);

			break;
		}
		case COMBO_BOX:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				EnableOtherChar(SendMessageW(hOptions, CB_GETCURSEL, 0, 0) == COMMENT_TYPE::other);
				//switch (SendMessageW(hOptions, CB_GETCURSEL, 0, 0)) {
				//case 0:case 1:case 2:
				//	EnableOtherChar(false);//disable buttons
				//	break;
				//case 3:
				//	EnableOtherChar(true);//enable buttons
				//	break;
				//}
			break;

		case REMOVE:
		{
			FILE_FORMAT format = GetCurrentTabExtraInfo().fileFormat;
			switch (SendMessageW(hOptions, CB_GETCURSEL, 0, 0)) {
			case COMMENT_TYPE::brackets:
				CommentRemoval(GetCurrentTabExtraInfo().hText, format, L'[', L']');
				break;
			case COMMENT_TYPE::parenthesis:
				CommentRemoval(GetCurrentTabExtraInfo().hText, format, L'(', L')');
				break;
			case COMMENT_TYPE::braces:
				CommentRemoval(GetCurrentTabExtraInfo().hText, format, L'{', L'}');
				break;
			case COMMENT_TYPE::other:
				CustomCommentRemoval(GetCurrentTabExtraInfo().hText, format);
				break;
			}
			break;
		}
		case SAVE_FILE:
		case ID_SAVE:
		{
			//if (IsAcceptedFile()) {
			if (GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED) DoBackup();

			int text_length = GetWindowTextLengthW(hFile) + 1;//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea

			wstring text(text_length, L'\0');
			GetWindowTextW(hFile, &text[0], text_length);

			TEXT_INFO text_data = GetCurrentTabExtraInfo();

			if (text[0] != L'\0')
				DoSave(text_data.hText, text);
			//TODO(fran): error handling

			//}
			break;
		}
		case SAVE_AS_FILE:
		case ID_SAVE_AS:
			//if (IsAcceptedFile()) {

			//TODO(fran): DoSaveAs allows for saving of file when no tab is created and sets all the controls but doesnt create an empty tab,
			//should we create the tab or not allow to save?
			DoSaveAs(GetCurrentTabExtraInfo().hText); //No DoBackup() for Save As since the idea is the user saves to a new file obviously

			//}
			break;
		case LANGUAGE_MANAGER::LANGUAGE::ENGLISH://INFO: this relates to the menu item, TODO(fran): a way to check a range of numbers, from first lang to last
		{

			MenuChangeLang((LANGUAGE_MANAGER::LANGUAGE)LOWORD(wParam),LANGUAGE_MANAGER::Instance().GetCurrentLanguage());

		}
			break;
		case LANGUAGE_MANAGER::LANGUAGE::SPANISH:
		{
			MenuChangeLang((LANGUAGE_MANAGER::LANGUAGE)LOWORD(wParam), LANGUAGE_MANAGER::Instance().GetCurrentLanguage());
		}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_SIZE:
		ResizeWindows(hWnd);
		break;
	case WM_NOTIFY: 
	{
		NMHDR* msg_info = (NMHDR*)lParam;
		switch (msg_info->code) {
		//Handling tab changes, since there is no specific message to manage this from inside the tab control, THANKS WINDOWS
		//+
		case TCN_SELCHANGING: //TODO(fran): now we are outside the tab control we should use the other parameters to check we are actually changing 
								//the control we want
		{
			SaveAndDisableCurrentTab(msg_info->hwndFrom);
			break;
		}
		case TCN_SELCHANGE:
		{
			int index = SendMessage(msg_info->hwndFrom, TCM_GETCURSEL, 0, 0);
			if (index != -1) {
				CUSTOM_TCITEM new_item;
				new_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
				//WCHAR buf[20] = { 0 };
				//prev_item.tab_info.pszText = buf;
				//prev_item.tab_info.cchTextMax = 20;
				BOOL ret = SendMessage(msg_info->hwndFrom, TCM_GETITEM, index, (LPARAM)&new_item);
				if (ret) {
					EnableTab(new_item.extra_info);
				}
			}
			break;
		}
		//+
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	//case WM_SYSCOMMAND:
	//	if (wParam == SC_MINIMIZE)
	//	{
	//		//MinimizeWndToTray(hWnd);

	//		// Return 0 to tell DefDlgProc that we handled the message, and set
	//		// DWL_MSGRESULT to 0 to say that we handled WM_SYSCOMMAND
	//		//SetWindowLong(hWnd, DWL_MSGRESULT, 0);

	//		RECT rcFrom, rcTo;
	//		GetWindowRect(hWnd, &rcFrom);
	//		GetTrayWndRect(&rcTo);
	//		MyAnimate(hWnd, { rcFrom.left,rcFrom.top }, { rcTo.left,rcTo.top }, 200);

	//		return 0;
	//	}
	//	return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_DESTROY:
		CreateInfoFile(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


//wstring GetExecFolder() {
//	WCHAR ownPth[MAX_PATH_LENGTH];
//	HMODULE hModule = GetModuleHandle(NULL);
//	if (hModule != NULL) GetModuleFileName(hModule, ownPth, (sizeof(ownPth)));//INFO: that sizeof is probably wrong
//	else { 
//		MessageBoxW(hWnd,L"" /*exe_path_failed_T[global_locale]*/, NULL, MB_ICONERROR); 
//		return L"C:\\Temp\\";
//	}
//	wstring dir = ownPth;
//	dir.erase(dir.rfind(L"\\") + 1, string::npos);
//	return dir;
//}

/*
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
	SetWindowTextW(hFinalText,final_char_T[global_locale]);
	SetWindowTextW(hRemove,remove_T[global_locale]);

	UpdateWindow(hWnd);

	//@@Alguna forma de alinear los controles por la derecha?? (asi todo queda alineado)

	//notification messages get updated on their own
}
*/

//typedef struct GetLineParam {
//	wstring *text;
//	WCHAR file[MAX_PATH] = { 0 };
//} GLP, *PGLP;
//DWORD WINAPI ReadTextThread(LPVOID lpParam) {//receives filename and wstring to save to
//
//	PGLP parameters = (PGLP)lpParam; 
//
//	unsigned char encoding = GetTextEncoding(parameters->file);
//
//	wifstream file(parameters->file, ios::binary);
//
//	if (file.is_open()) {
//
//		//set encoding for getline
//		if (encoding == UTF8)
//			file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
//		else if (encoding == UTF16)
//			file.imbue(locale(file.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));
//		//if encoding is ASCII we do nothing
//
//		wstringstream buffer;
//		buffer << file.rdbuf();
//		//wstring for_testing = buffer.str();
//		(*((PGLP)lpParam)->text) = buffer.str();
//
//		file.close();
//	}
//	return 0;
//}

// //For GetTextEncoding
//	else {//do manual checking
//		wifstream file(filename, ios::in | ios::binary);
//		wstringstream buffer;
//		buffer << file.rdbuf(); 
//		file.close();
//		buffer.seekg(0, ios::end);
//		int length = buffer.tellg();//long enough length
//		buffer.clear();
//		buffer.seekg(0, ios::beg);
//		//https://unicodebook.readthedocs.io/guess_encoding.html
//		//NOTE: creo q esto testea UTF16 asi que probablemente haya errores, y toma ascii como utf
//		int test;
//		test = IS_TEXT_UNICODE_ILLEGAL_CHARS;
//		if (IsTextUnicode(buffer.str().c_str(), length, &test)) return ASCII;
//		test = IS_TEXT_UNICODE_STATISTICS;
//		if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
//		test = IS_TEXT_UNICODE_REVERSE_STATISTICS;
//		if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
//		test = IS_TEXT_UNICODE_ASCII16;
//		if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
//		return ASCII; 
//	}
//}

//std::vector<wstring> GetFiles2(LPCWSTR dir) {//dir should not contain \\ in the end
//	wstring dirfilter = dir;
//	dirfilter += L"\\*";
//
//	std::vector<wstring> files;
//
//	WIN32_FIND_DATA data;
//	HANDLE hFind = FindFirstFile(dirfilter.c_str(), &data);
//	//IMPORTANT INFO: this is not recursive, doesnt go inside folders
//	do
//	{
//		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//		{
//			//directory
//		}
//		else
//		{
//			files.push_back(data.cFileName);//TODO(fran): compare extension
//		}
//	} while (FindNextFile(hFind, &data) != 0);
//
//	return files;
//}

/*
void SetOptionsComboBox(HWND & hCombo, bool isNew) {//@is there a simpler way to modify the combo box??
	int previous_index=0;
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
*/

//#define NO_SEPARATION 0
//#define ENTER 1
//#define CARRIAGE_RETURN 2
//#define CR_ENTER 3

//CommentRemoval old
//wifstream file(accepted_file);
//wstring line;
//wstring completed_text;
//int enter_found = 0;
//if (file.is_open()) {
//	while (!file.eof()) { //o .good()
//		getline(file, line,start);
//		
//		enter_found = line.find(L'\n', enter_found);
//		while (enter_found != -1) {
//			line.insert(enter_found, L"\r");
//			enter_found = line.find(L'\n', enter_found + 2);
//		}
//		enter_found = 0;

//		completed_text += line;

//		//int lenght = GetWindowTextLengthW(hSubs); //To append text instead of setting it all at once, though it doesnt do it fast enough, maybe needs to send a != type of msg
//		//SendMessageW(hSubs, EM_SETSEL, (WPARAM)lenght, (LPARAM)lenght);
//		//SendMessageW(hSubs, EM_REPLACESEL, 0, (LPARAM)line.c_str());

//		line = L"";//por las dudas
//		file.ignore(200, end);
//	}

//SetWindowTextW(hSubs, completed_text.c_str());
//file.close();
//}

//void DoSaveAs() { old method
//WCHAR name[MAX_PATH_LENGTH];
//name[0] = L'\0';

//OPENFILENAMEW save_file;
//ZeroMemory(&save_file, sizeof(OPENFILENAMEW));
//save_file.lStructSize = sizeof(OPENFILENAMEW);
//save_file.hwndOwner = NULL;
//save_file.lpstrFile = name;
//save_file.nMaxFile = MAX_PATH_LENGTH;
//save_file.lpstrFilter = L"All files(*.*)\0*.*\0";
//save_file.lpstrFileTitle = &accepted_file_name_with_ext[0];
//save_file.nMaxFileTitle = accepted_file_name_with_ext.length();
//save_file.lpstrInitialDir = &accepted_file_dir[0];
//save_file.lpstrDefExt = &accepted_file_ext[0];

//if (GetSaveFileNameW(&save_file)) {
//	if ( (GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED) &&
//		accepted_file.compare(save_file.lpstrFile) == 0 ) DoBackup();
//	DoSave(save_file.lpstrFile);
//}

//OFN_ENABLETEMPLATE as a flag for file encoding?
//OFN_NOREADONLYRETURN tambien?
//OFN_NOVALIDATE
//OFN_OVERWRITEPROMPT
//OFN_PATHMUSTEXIST
//https://stackoverflow.com/questions/9193944/how-to-customize-open-save-file-dialogs

//DefaultSaveAs(); (new method)
//
//https://msdn.microsoft.com/en-us/library/Bb776913(v=VS.85).aspx
//}

//int GetLineSeparation(wifstream &file) {
//	wstringstream buffer;
//	buffer << file.rdbuf();
//	file.clear();
//	file.seekg(0, ios::beg);
//	return GetLineSeparation(buffer.str());
//}

//int GetLineSeparation(wstring text) { //Check the format that the file uses to encode line changes, \n , \r or \r\n
//	if (text.find(L"\r\n", 0) != string::npos) return CR_ENTER;
//	else if (text.find(L"\r", 0) != string::npos) return CARRIAGE_RETURN;
//	else if (text.find(L"\n", 0) != string::npos) return ENTER;
//	else return NO_SEPARATION;
//}

//int LineCount(wifstream &file) {
//	int text_length = count(istreambuf_iterator<wchar_t>(file),istreambuf_iterator<wchar_t>(), L'\n');
//	file.clear();
//	file.seekg(0, ios::beg);
//	return text_length;
//}

//void LineSeparation(wstring &text) { //@@Check what happens in different encodings 
//
//	int separator_found; //store position where to insert new separator
//
//	switch (GetLineSeparation(text)) {
//	case CARRIAGE_RETURN: // \r -> \r\n
//		separator_found = text.find(L'\r', 0);
//		while (separator_found != string::npos) {
//			text.insert(separator_found + 1, L"\n");
//			separator_found = text.find(L'\r', separator_found + 2);
//		}
//		break;
//	case ENTER: // \n -> \r\n
//		separator_found = text.find(L'\n', 0);
//		while (separator_found != string::npos) {
//			text.insert(separator_found, L"\r");
//			separator_found = text.find(L'\n', separator_found + 2);
//		}
//		break;
//	case CR_ENTER: // already \r\n
//	case NO_SEPARATION: //no line separators found
//		return;
//	}
//
//	//SetWindowTextW(hSubs, text.c_str()/*+L'\0'*/);
//}

//void LineSeparation() { //@@Check what happens in different encodings 
//	
//	int separator_found; //store position where to insert new separator
//
//	int text_length = GetWindowTextLengthW(hSubs) + 1;
//	wstring text(text_length,L'\0');
//	//GetWindowTextW(hSubs, const_cast<WCHAR*>(text.c_str()), text_length - 1);
//	GetWindowTextW(hSubs, &text[0], text_length);
//	
//	switch (GetLineSeparation(text)) {
//	case CARRIAGE_RETURN: // \r -> \r\n
//		separator_found = text.find(L'\r', 0);
//		while (separator_found != string::npos) {
//			text.insert(separator_found + 1, L"\n");
//			separator_found = text.find(L'\r', separator_found + 2);
//		}
//		break;
//	case ENTER: // \n -> \r\n
//		separator_found = text.find(L'\n', 0);
//		while (separator_found != string::npos) {
//			text.insert(separator_found, L"\r");
//			separator_found = text.find(L'\n', separator_found + 2);
//		}
//		break;
//	case CR_ENTER: // already \r\n
//	case NO_SEPARATION: //no line separators found
//		return;
//	}
//
//	SetWindowTextW(hSubs, text.c_str()/*+L'\0'*/);
//}
