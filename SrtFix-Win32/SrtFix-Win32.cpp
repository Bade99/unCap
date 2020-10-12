#pragma once

//Pointless warnings
#pragma warning(disable : 4505) //unreferenced local function has been removed
#pragma warning(disable : 4189) //local variable is initialized but not referenced

//Pointless unsafe complaints
#ifdef _DEBUG
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "windows_msg_mapper.h"

#define WIN32_LEAN_AND_MEAN 
#define STRICT_TYPED_ITEMIDS
#include "targetver.h"

#include "resource.h"
#include <Windows.h>
#include <Windowsx.h> //GET_X_LPARAM GET_Y_LPARAM
#include "Open_Save_FileHandler.h"
#include <string>
#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include <propvarutil.h>
#include <Propsys.h>
#include <propkey.h>
#include <Commdlg.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include <wchar.h> //_wcsdup
#include "LANGUAGE_MANAGER.h"
#include <Shlobj.h>//SHGetKnownFolderPath
#include "utf8.h" //Thanks to http://utfcpp.sourceforge.net/
#include "text_encoding_detect.h" //Thanks to https://github.com/AutoItConsulting/text-encoding-detect
#include "fmt/format.h" //Thanks to https://github.com/fmtlib/fmt
#include "unCap_helpers.h"
#include "unCap_global.h"
#include "unCap_math.h"
#include "unCap_scrollbar.h"
#include "unCap_button.h"
#include"unCap_Renderer.h"

//----------------------LINKER----------------------:
// Linker->Input->Additional Dependencies (right way to link the .lib)
#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"propsys.lib") //open save file handler
#pragma comment(lib,"shlwapi.lib") //open save file handler
#pragma comment(lib,"UxTheme.lib") // setwindowtheme

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") 
//#pragma comment(lib,"User32.lib")

//----------------------NAMESPACES----------------------:
using namespace std;
namespace fs = std::experimental::filesystem;


//----------------------TODOs----------------------:
//Paint Initial char and Final char wnds to look like the rest, add white border so they are more easily noticeable
//Parametric sizes for everything, and DPI awareness for controls that dont move
//Move error messages from a separate dialog window into the space on the right of the Remove controls
//Option to retain original file encoding on save, otherwise utf8/user defined default
//Additional File Format support?: webvtt,microdvd,subviewer,ttml,sami,mpsub,ttxt

//----------------------DEFINES----------------------:

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

#define TCM_RESIZETABS (WM_USER+50) //Sent to a tab control for it to tell its tabs' controls to resize. wParam = lParam = 0
#define TCM_RESIZE (WM_USER+51) //wParam= pointer to SIZE of the tab control ; lParam = 0

#define UNCAP_SETTEXTDURATION (WM_USER+52) //Sets duration of text in a control before it's hidden. wParam=(UINT)duration in milliseconds ; lParam = NOT USED

#define MAX_PATH_LENGTH MAX_PATH
#define MAX_TEXT_LENGTH 288000 //Edit controls have their text limit set by default at 32767, we need to change that

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

struct TABCLIENT { //Construct the "client" space of the tab control from this offsets, aka the space where you put your controls, in my case text editor
	short leftOffset, topOffset, rightOffset, bottomOffset;
};

struct CLOSEBUTTON {
	int rightPadding;//offset from the right side of each tab's rectangle
	SIZE icon;//Size of the icon (will be placed centered in respect to each tab's rectangle)
};

//----------------------GLOBALS----------------------:
TCHAR unCap_wndclass_uncap_nc[] = TEXT("unCap_wndclass_uncap_nc"); //Non client uncap
TCHAR unCap_wndclass_uncap_cl[] = TEXT("unCap_wndclass_uncap_cl"); //Client uncap

TCHAR appName[] = TEXT("unCap"); //Program name, to be displayed on the title of windows

HMENU hMenu;//main menu bar(only used to append the rest)
HMENU hFileMenu;
HMENU hFileMenuLang;

HWND hMain;//main window where everything else is displayed
HWND hFile;//window for file directory //TODO(fran): remove this guy, and the stupid things that depend on it; for now I'll just hide it
HWND hOptions;
HWND hInitialText;
HWND hInitialChar;
HWND hFinalText;
HWND hFinalChar;
HWND hRemove;
//HWND hSubs;//window for file contents
HWND hRemoveCommentWith;
//HWND hRemoveProgress;
//HWND hReadFile;
HWND TextContainer;//The tab control where the text editors are "contained"
HWND hMessage;//Show timed messages

HFONT hGeneralFont;//Main font
HFONT hMenuFont;//Menus font

HMENU hPopupMenu;

bool Backup_Is_Checked = false; //info file check, do backup or not

LANGUAGE_MANAGER::LANGUAGE startup_locale = LANGUAGE_MANAGER::LANGUAGE::ENGLISH; //defaults to english

UNCAP_COLORS unCap_colors;

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

const COMDLG_FILTERSPEC c_rgSaveTypes[] = //TODO(fran): this should go in resource file, but it isnt really necessary
{
{ L"Srt (*.srt)",       L"*.srt" },
{ L"Advanced SSA (*.ass)",       L"*.ass" },
{ L"Sub Station Alpha (*.ssa)",       L"*.ssa" },
{ L"Txt (*.txt)",       L"*.txt" },
{ L"All Documents (*.*)",         L"*.*" }
};

TABCLIENT TabOffset;

CLOSEBUTTON TabCloseButtonInfo; //Information for the placement of the close button of each tab in a tab control

//----------------------HELPER FUNCTIONS----------------------:



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
		BOOL ret = (BOOL)SendMessage(TextContainer, TCM_GETITEM, index, (LPARAM)&item);
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
	int index = (int)SendMessage(TextContainer, TCM_GETCURSEL, 0, 0);

	return GetTabExtraInfo(index);
}

wstring GetTabTitle(int index){
	if (index != -1) {
		CUSTOM_TCITEM item;
		WCHAR title[200];
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = title;
		item.tab_info.cchTextMax = ARRAYSIZE(title);
		BOOL ret = (BOOL)SendMessage(TextContainer, TCM_GETITEM, index, (LPARAM)&item);
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

	int ret = (int)SendMessage(TextContainer, TCM_SETITEM, index, (LPARAM)&item);
	return ret;
}

BOOL SetCurrentTabTitle(wstring title) {
	int index = (int)SendMessage(TextContainer, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM item;
		item.tab_info.mask = TCIF_TEXT;
		item.tab_info.pszText = (LPWSTR)title.c_str();
		int ret = (int)SendMessage(TextContainer, TCM_SETITEM, index, (LPARAM)&item);
		return ret;
	}
	else return index;
}

int GetNextTabPos() {
	int count = (int)SendMessage(TextContainer, TCM_GETITEMCOUNT, 0, 0);
	return count + 1; //TODO(fran): check if this is correct, or there is a better way
}

//Returns the index of the previously selected tab if successful, or -1 otherwise.
int ChangeTabSelected(int index) {
	return (int)SendMessage(TextContainer, TCM_SETCURSEL, index, 0);
}

//Returns TRUE if successful, or FALSE otherwise.
int DeleteTab(HWND TabControl, int position) {
	return (int)SendMessage(TabControl, TCM_DELETEITEM, position, 0);
}

BOOL TestCollisionPointRect(POINT p, const RECT& rc) {
	//TODO(fran): does this work with negative values?
	if (p.x < rc.left || p.x > rc.right) return FALSE;
	if (p.y < rc.top || p.y > rc.bottom) return FALSE;
	return TRUE;
}

//if new_filename==NULL || *new_filename == NULL then filename related info is cleared
void UpdateMainWindowTitle_Filename(const WCHAR* new_filename) {
	if (new_filename == NULL || *new_filename == NULL) {
		SetWindowTextW(hMain, appName);
	}
	else {
		wstring title_window = wstring(new_filename) + L" - " + appName;
		SetWindowTextW(hMain, title_window.c_str());
	}
}

//----------------------FUNCTION PROTOTYPES----------------------:
LRESULT CALLBACK	ShowMessageProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK	ComboProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK	EditCatchDrop(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK    UncapNcProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	UncapClProc(HWND, UINT, WPARAM, LPARAM);
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

/// <summary>
/// Decides which font FaceName is appropiate for the current system
/// </summary>
wstring GetFontFaceName() {
	//Font guidelines: https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-fonts
	//Stock fonts: https://docs.microsoft.com/en-us/windows/win32/gdi/using-a-stock-font-to-draw-text

	//TODO(fran): can we take the best codepage from each font and create our own? (look at font linking & font fallback)

	//We looked at 2195 fonts, this is what's left
	//Options:
	//Segoe UI (good txt, jp ok) (10 codepages) (supported on most versions of windows)
	//-Arial Unicode MS (good text; good jp) (33 codepages) (doesnt come with windows)
	//-Microsoft YaHei or UI (look the same,good txt,good jp) (6 codepages) (supported on most versions of windows)
	//-Microsoft JhengHei or UI (look the same,good txt,ok jp) (3 codepages) (supported on most versions of windows)

	HDC hdc = GetDC(GetDesktopWindow()); //You can use any hdc, but not NULL
	vector<wstring> fontnames;
	EnumFontFamiliesEx(hdc, NULL
		, [](const LOGFONT *lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)->int {((vector<wstring>*)lParam)->push_back(lpelfe->lfFaceName); return TRUE; }
	, (LPARAM)&fontnames, NULL);
	
	const WCHAR* requested_fontname[] = { TEXT("Segoe UI"), TEXT("Arial Unicode MS"), TEXT("Microsoft YaHei"), TEXT("Microsoft YaHei UI")
										, TEXT("Microsoft JhengHei"), TEXT("Microsoft JhengHei UI")};

	for(int i=0;i< ARRAYSIZE(requested_fontname);i++)
		if(std::any_of(fontnames.begin(), fontnames.end(), [f=requested_fontname[i]](wstring s) {return !s.compare(f); })) return requested_fontname[i];
	
	return L"";
}

void CreateFonts()
{
	LOGFONTW lf;

	memset(&lf, 0, sizeof(lf));
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfHeight = -15;
	
	//INFO: by default if I dont set faceName it uses "Modern", looks good but it lacks some charsets
	wcsncpy_s(lf.lfFaceName, GetFontFaceName().c_str(), ARRAYSIZE(lf.lfFaceName));

	if (hGeneralFont != NULL) {
		DeleteObject(hGeneralFont);
		hGeneralFont = NULL;
	}
	hGeneralFont = CreateFontIndirectW(&lf);
	if (hGeneralFont == NULL)
		MessageBoxW(NULL, RCS(LANG_FONT_ERROR), RCS(LANG_ERROR), MB_OK);

	if (hMenuFont != NULL) {
		DeleteObject(hMenuFont);
		hMenuFont = NULL;
	}
	lf.lfHeight = (LONG)((float)GetSystemMetrics(SM_CYMENU)*.85f);
	hMenuFont = CreateFontIndirectW(&lf);
	if (hMenuFont == NULL)
	{
		MessageBoxW(NULL, RCS(LANG_FONT_ERROR), RCS(LANG_ERROR), MB_OK);
	}
}

/// <summary>
/// Performs comment removal for .srt type formats
/// </summary>
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
/// <returns>The number of comments removed, 0 indicates no comments were found</returns>
size_t CommentRemovalSSA(wstring& text, WCHAR start, WCHAR end) {

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
		if (start == L'{') { //TODO(fran): can this be embedded in the code below?
			if ((start_pos + 1) >= text.length()) break; //Check a next character exists after our current position
			if (text.at(start_pos + 1) == L'\\') { //Indeed it was a \ so we where inside a special command, just ignore it and continue searching
				start_pos++; //Since we made no changes to the string at start_pos we have to move start_pos one char forward so we dont get stuck 
				continue; 
			}
		}

		//TODO(fran): there's a bigger check that we need to perform, some of the commands inside {\command} can have parenthesis()
		// So what we should do is always check if we are inside of a command in which case we ignore and continue
		// How to perform the check: we have to go back from our position and search for the closest "{\" in our same line, but also check that there is
		// no "}" that is closer
		size_t command_begin_pos = text.rfind(L"{\\", start_pos); //We found a command, are we inside it?
		if (command_begin_pos != wstring::npos) {
			if (command_begin_pos >= line_pos) {//Check that the "{\" is on the same line as us
				//INFO: we are going to assume that nesting {} or {\} inside a {\} is invalid and is never used, otherwise add this extra check (probably needs a loop)
				//size_t possible_comment_begin_pos; //Closest "{" going backwards from our current position
				//possible_comment_begin_pos = text.find_last_of(L'{', start_pos);
				size_t possible_comment_end_pos; //Closest "}" going forwards from command begin position
				possible_comment_end_pos = text.find_first_of(L'}', command_begin_pos);
				if (possible_comment_end_pos == wstring::npos) break; //There's a command that has no end, invalid
				//TODO(fran): add check if the comment end is on the same line
				if (possible_comment_end_pos > start_pos) { //TODO(fran): what to do if they are equal??
					start_pos++;
					continue; //We are inside of a command, ignore and continue
				}
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

	if (comment_count) {
		SetWindowTextW(hText, text.c_str()); //TODO(fran): add message for number of comments removed
		wstring comment_count_str = fmt::format(RS(LANG_CONTROL_TIMEDMESSAGES), comment_count);
		SetWindowText(hMessage, comment_count_str.c_str());
	}
	else {
		SendMessage(hMessage, WM_SETTEXT, 0, (LPARAM)RCS(LANG_COMMENTREMOVAL_NOTHINGTOREMOVE));
	}
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
	else SendMessage(hMessage, WM_SETTEXT, 0, (LPARAM)RCS(LANG_COMMENTREMOVAL_ADDCHARS));
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

void DoBackup() { //TODO(fran): option to change backup folder //TODO(fran): I feel backup is pretty pointless at this point, more confusing than anything else, REMOVE

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

		SetWindowText(hMessage, RCS(LANG_SAVE_DONE));
		
		SetWindowTextW(hFile, save_file.c_str()); //Update filename viewer
		UpdateMainWindowTitle_Filename(save_file.c_str());
		
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

BOOL SetMenuItemData(HMENU hmenu, UINT item, BOOL fByPositon, ULONG_PTR data) {
	MENUITEMINFO i;
	i.cbSize = sizeof(i);
	i.fMask = MIIM_DATA;
	i.dwItemData = data;
	return SetMenuItemInfo(hmenu, item, fByPositon, &i);
}

BOOL SetMenuItemString(HMENU hmenu, UINT item, BOOL fByPositon, TCHAR* str) {
	MENUITEMINFOW menu_setter;
	menu_setter.cbSize = sizeof(menu_setter);
	menu_setter.fMask = MIIM_STRING;
	menu_setter.dwTypeData = str;
	BOOL res = SetMenuItemInfoW(hmenu, item, FALSE, &menu_setter);
	return res;
}

HMENU HACK_toplevelmenu = NULL; //TODO(fran)

void AddMenus(HWND hwnd) {
	//NOTE: each menu gets its parent HMENU stored in the itemData part of the struct
	LANGUAGE_MANAGER::Instance().AddMenuDrawingHwnd(hwnd);

	//INFO: the top 32 bits of an HMENU are random each execution, in a way, they can actually get set to FFFFFFFF or to 00000000, so if you're gonna check two of those you better make sure you cut the top part in BOTH

	hMenu = CreateMenu();
	hFileMenu = CreateMenu();
	hFileMenuLang = CreateMenu();
	AppendMenuW(hMenu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)hFileMenu, (LPCWSTR)hMenu);
	HACK_toplevelmenu = hFileMenu;
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
	
	AppendMenuW(hFileMenuLang, MF_STRING | MF_OWNERDRAW, LANGUAGE_MANAGER::LANGUAGE::ENGLISH , (LPCWSTR)hFileMenuLang);
	SetMenuItemString(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::ENGLISH, 0, const_cast<TCHAR*>(TEXT("English")));

	AppendMenuW(hFileMenuLang, MF_STRING | MF_OWNERDRAW, LANGUAGE_MANAGER::LANGUAGE::SPANISH, (LPCWSTR)hFileMenuLang);
	SetMenuItemString(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::SPANISH, 0, const_cast<TCHAR*>(TEXT("Español")));

	HBITMAP bTick = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(TICK)); //TODO(fran): preload the bitmaps or destroy them when we destroy the menu
	HBITMAP bCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(CLOSE));
	HBITMAP bEarth = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(EARTH));

	//TODO(fran): fix old bitmap code that uses wrong sizes and no transparency

	SetMenuItemBitmaps(hFileMenu, BACKUP_FILE, MF_BYCOMMAND,bCross,bTick);//1ºunchecked,2ºchecked
	if(Backup_Is_Checked) CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED);

	SetMenuItemBitmaps(hFileMenu, (UINT)hFileMenuLang, MF_BYCOMMAND, bEarth, bEarth);
	//https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getmenustate
	//https://docs.microsoft.com/es-es/windows/desktop/menurc/using-menus#using-custom-check-mark-bitmaps
	//https://www.daniweb.com/programming/software-development/threads/490612/win32-can-i-change-the-hbitmap-structure-for-be-transparent
	
	//TODO(fran): some automatic way to add this to all langs?
	SetMenuItemBitmaps(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::ENGLISH, MF_BYCOMMAND, NULL, bTick);
	SetMenuItemBitmaps(hFileMenuLang, LANGUAGE_MANAGER::LANGUAGE::SPANISH, MF_BYCOMMAND, NULL, bTick);

	CheckMenuItem(hFileMenuLang, LANGUAGE_MANAGER::Instance().GetCurrentLanguage(), MF_BYCOMMAND | MF_CHECKED);
	
	SetMenu(hwnd, hMenu);
}

//Method for managing edit control special features (ie dropped files,...)
//there seems to be other ways like using SetWindowsHookExW or SetWindowLongPtrW 
LRESULT CALLBACK EditCatchDrop(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	switch (uMsg)
	{
	case WM_DROPFILES:
		CatchDrop(wParam);
		return TRUE;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

#define EM_GET_MAX_VISIBLE_LINES (WM_USER+200) /*Retrieves a count for the max number of lines that can be displayed at once in the current window. return=int*/
#define EM_SETVSCROLL (WM_USER+201) /*Sets the vertical scrollbar that is to be used. wParam=HWND of the scrollbar control*/
//#define EM_SETHSCROLL (WM_USER+202) /*Sets the horizontal scrollbar that is to be used. wParam=HWND of the scrollbar control*/
//#define EM_GET_MAX_VISIBLE_CHARS_PER_LINE (WM_USER+203) /*Retrieves a count for the max number of characters that can be displayed in one line at once in the current window. return=int*/
struct EditProcState {
	bool initialized;
	HWND wnd;
	HWND vscrollbar;// , hscrollbar;
};
//NOTE: EditProc controls require the creation of a EditProcState struct with calloc, and for it to be passed as the 4th param in SetWindowSubclass, the object will now be managed by the procedure and does not need the user to handle its memory
//NOTE: no left-right scrolling, it is line wrap handled
int EDIT_get_max_visible_lines(HWND hwnd) {
	RECT rc;
	GetClientRect(hwnd, &rc);
	int page_height = RECTHEIGHT(rc);

	TEXTMETRIC tm;
	HDC dc = GetDC(hwnd);
	GetTextMetrics(dc, &tm);
	ReleaseDC(hwnd, dc);
	int line_height = tm.tmHeight + tm.tmExternalLeading;

	int visible_lines = safe_ratio0(page_height, line_height);

	return visible_lines;
}
void EDIT_update_scrollbar(EditProcState* state) {
	if (state->vscrollbar) {
		int line_count = EDIT_get_max_visible_lines(state->wnd);
		int range_max = (int)DefSubclassProc(state->wnd, EM_GETLINECOUNT, 0, 0);
		int pos = (int)DefSubclassProc(state->wnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		U_SB_set_stats(state->vscrollbar, range_max, line_count, pos);
	}
}
LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	Assert(dwRefData);
	EditProcState* state = (EditProcState*)dwRefData;
	if (!state->initialized) {
		state->initialized = true;
		state->wnd = hwnd;
	}

	//static long long msg_cnt = 0;
	//printf("%lld:%s\n",msg_cnt++, msgToString(msg));

	switch (msg) {
	case WM_MOUSEWHEEL:
	{
		//TODO(fran): look at more reasonable algorithms, also this one should probably get a little exponential
		short zDelta = (short)(((float)GET_WHEEL_DELTA_WPARAM(wparam)/ (float)WHEEL_DELTA) * 3.f);
		if(zDelta>=0)
			for(int i=0;i< zDelta;i++)
				SendMessage(state->wnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0); //TODO(fran): use ScrollWindowEx ?
		else
			for (int i = 0; i > zDelta; i--)
				SendMessage(state->wnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
		return 0;
	} break;
	case EM_GET_MAX_VISIBLE_LINES: { 
		return EDIT_get_max_visible_lines(hwnd);
	} break;
	/*case EM_GET_MAX_VISIBLE_CHARS_PER_LINE: {
		RECT rc;
		GetClientRect(hwnd, &rc);
		int line_width = RECTWIDTH(rc);

		TEXTMETRIC tm;
		HDC dc = GetDC(hwnd);
		GetTextMetrics(dc, &tm);
		ReleaseDC(hwnd, dc);
		int char_width = tm.tmAveCharWidth; //TODO(fran): better approximation

		int char_count = line_width / char_width;

		return char_count;
	} break;*/
	case EM_SETVSCROLL: {
		state->vscrollbar = (HWND)wparam;
	} break;
	//case EM_SETHSCROLL: {
	//	state->hscrollbar = (HWND)wparam;
	//} break;
	case TCM_RESIZE:
	{
		SIZE* control_size = (SIZE*)wparam;

		MoveWindow(hwnd, TabOffset.leftOffset, TabOffset.topOffset
			, control_size->cx - TabOffset.rightOffset - TabOffset.leftOffset, control_size->cy - TabOffset.bottomOffset - TabOffset.topOffset, TRUE);
		//x & y remain fixed and only width & height change

		//TODO(fran): resize scrollbars
		SendMessage(state->vscrollbar, U_SB_AUTORESIZE, 0, 0);

		EDIT_update_scrollbar(state); //NOTE: actually here you just need to update nPage

		return TRUE;
	} //TODO(fran): now comes the hard part, to tell the scrollbar where we are, and the ranges
	case WM_DESTROY:
	{
		free(state);
	}break;
	//Messages that could trigger the need for updating the scrollbar
	case WM_PAINT:
	case EM_POSFROMCHAR:
	//TODO(fran): add more
	{
		LRESULT res = DefSubclassProc(hwnd, msg, wparam, lparam);
		EDIT_update_scrollbar(state);
		return res;
	} break;
	default:return DefSubclassProc(hwnd, msg, wparam, lparam);
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
	
	HWND TextControl = CreateWindowExW(NULL, L"Edit", NULL, WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_CLIPCHILDREN //| WS_VSCROLL | WS_HSCROLL //|WS_VISIBLE
		, text_rec.left, text_rec.top, text_rec.right - text_rec.left, text_rec.bottom - text_rec.top
		,TabControl //IMPORTANT INFO TODO(fran): this is not right, the parent should be the individual tab not the whole control, not sure though if that control will show our edit control
		, NULL, NULL, NULL); 

	SendMessageW(TextControl, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);

	SendMessageW(TextControl, EM_SETLIMITTEXT, (WPARAM)MAX_TEXT_LENGTH, NULL); //Change default text limit of 32767 to a bigger value

	SetWindowSubclass(TextControl,EditProc,0, (DWORD_PTR)calloc(1,sizeof(EditProcState)));

	HWND VScrollControl = CreateWindowExW(NULL, unCap_wndclass_scrollbar, NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, TextControl, NULL, NULL, NULL);
	SendMessage(VScrollControl, U_SB_SET_PLACEMENT, (WPARAM)ScrollBarPlacement::right, 0);

	SendMessageW(TextControl, EM_SETVSCROLL, (WPARAM)VScrollControl, 0);

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

	return (int)SendMessage(TabControl, TCM_INSERTITEM, position, (LPARAM)&newItem);
}

//Returns the current position of that tab or -1 if failed
int SaveAndDisableCurrentTab(HWND tabControl) {
	int index = (int)SendMessage(tabControl, TCM_GETCURSEL, 0, 0);
	if (index != -1) {
		CUSTOM_TCITEM prev_item;
		prev_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
		//WCHAR buf[20] = { 0 };
		//prev_item.tab_info.pszText = buf;
		//prev_item.tab_info.cchTextMax = 20;
		BOOL res = (BOOL)SendMessage(tabControl, TCM_GETITEM, index, (LPARAM)&prev_item);
		if (res) {
			ShowWindow(prev_item.extra_info.hText, SW_HIDE);

			int text_length = GetWindowTextLengthW(hFile) + 1;//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea

			wstring text(text_length, L'\0');
			GetWindowTextW(hFile, &text[0], text_length);

			if (text[0]!=L'\0') {//Check in case there is invalid data on the controls
				//TODO(fran): make sure this check doesnt break anything in some specific case

				wcsncpy_s(prev_item.extra_info.filePath, text.c_str(), sizeof(prev_item.extra_info.filePath) / sizeof(prev_item.extra_info.filePath[0]));

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
	UpdateMainWindowTitle_Filename(text_data.filePath);

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
	int item_index = (int)SendMessage(tabControl, TCM_GETCURSEL, 0, 0);
	if (item_index != -1) {
		BOOL ret = (BOOL)SendMessage(tabControl, TCM_GETITEMRECT, SendMessage(tabControl, TCM_GETCURSEL, 0, 0), (LPARAM)&item_rc); //returns rc in "viewport" coords, aka in respect to tab control, so it's good
		
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

LRESULT CALLBACK TabProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//INFO: if I ever feel like managing WM_PAINT: https://social.msdn.microsoft.com/Forums/expression/en-US/d25d08fb-acf1-489e-8e6c-55ac0f3d470d/tab-controls-in-win32-api?forum=windowsuidevelopment
	switch (Msg) {
		//TODO(fran): NCPAINT
	case TCM_RESIZETABS:
	{
		int item_count = (int)SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
		if (item_count != 0) {
			for (int i = 0; i < item_count; i++) {
				CUSTOM_TCITEM item;
				item.tab_info.mask = TCIF_PARAM;
				BOOL ret = (BOOL)SendMessage(hwnd, TCM_GETITEM, i, (LPARAM)&item);
				if (ret) {
					RECT rc;
					GetClientRect(hwnd, &rc);
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
		GetClientRect(hwnd, &rc);

		FillRect(hdc, &rc, unCap_colors.ControlBk);
		return 1;
	}
	case WM_CTLCOLOREDIT:
	{
		//if ((HWND)lParam == GetDlgItem(hwnd, EDIT_ID)) //TODO(fran): check we are changing the control we want
		if(TRUE)
		{
			SetBkColor((HDC)wParam, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor((HDC)wParam, ColorFromBrush(unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		}
		else return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_LBUTTONUP:
	{
		POINT p;
		p.x = GET_X_LPARAM(lParam);
		p.y = GET_Y_LPARAM(lParam);
		int index = TestCloseButton(hwnd, TabCloseButtonInfo, p);
		if (index != -1) {
			//Delete corresponding tab
			SendMessage(hwnd, TCM_DELETEITEM, index, 0); //After this no other messages are sent and the tab control's index gets set to -1
			//Now we have to send a TCM_SETCURSEL, but in the special case that no other tabs are left we should clean the controls
			int item_count = (int)SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
			if (item_count == 0) CleanTabRelatedControls();									//if there's no tabs left clean the controls
			else if (index == item_count) SendMessage(hwnd, TCM_SETCURSEL, index - 1, 0);	//if the deleted tab was the rightmost one change to the one on the left
			else SendMessage(hwnd, TCM_SETCURSEL, index, 0);								//otherwise change to the tab that now holds the index that this one did(will select the one on its right)

		} 
		break;
	}
	case TCM_DELETEITEM:
	{
		//Since right now our controls (text editor) are not children of the particular tab they dont get deleted when that one does, so gotta do it manually
		int index = (int)wParam;

		TEXT_INFO info = GetTabExtraInfo(index);
		//Any hwnd should be destroyed
		DestroyWindow(info.hText);

		return DefSubclassProc(hwnd, Msg, wParam, lParam);
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
	{
		SaveAndDisableCurrentTab(hwnd);

		CUSTOM_TCITEM new_item;
		new_item.tab_info.mask = TCIF_PARAM;
		BOOL ret = (BOOL)SendMessage(hwnd, TCM_GETITEM, wParam, (LPARAM)&new_item);
		if (ret) {
			EnableTab(new_item.extra_info);
		}

		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	default:
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	return 0;
}

void AddControls(HWND hwnd, HINSTANCE hInstance) {
	hFile = CreateWindowW(L"Static", L"", /*WS_VISIBLE |*/ WS_CHILD | WS_BORDER//| SS_WHITERECT
			| ES_AUTOHSCROLL | SS_CENTERIMAGE
			, 10, y_place, 664, 20, hwnd, NULL, NULL, NULL);
	//ChangeWindowMessageFilterEx(hFile, WM_DROPFILES, MSGFLT_ADD,NULL); y mas que habia
	
	//TODO(fran): add more interesting progress bar
	//hReadFile = CreateWindowExW(0, PROGRESS_CLASS, (LPWSTR)NULL, WS_CHILD
	//	, 10, y_place, 664, 20, hwnd, (HMENU)NULL, NULL, NULL);

	hRemoveCommentWith = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE
	, 25, y_place+3, 155, 20, hwnd, NULL, NULL, NULL);
	AWT(hRemoveCommentWith, LANG_CONTROL_REMOVECOMMENTWITH);

	hOptions = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD| CBS_DROPDOWNLIST|WS_TABSTOP
		, 195, y_place, 130, 90/*20*/, hwnd, (HMENU)COMBO_BOX, hInstance, NULL);
	//SetOptionsComboBox(hOptions, TRUE);
	ACT(hOptions, COMMENT_TYPE::brackets, LANG_CONTROL_CHAROPTIONS_BRACKETS);
	ACT(hOptions, COMMENT_TYPE::parenthesis, LANG_CONTROL_CHAROPTIONS_PARENTHESIS);
	ACT(hOptions, COMMENT_TYPE::braces, LANG_CONTROL_CHAROPTIONS_BRACES);
	ACT(hOptions, COMMENT_TYPE::other, LANG_CONTROL_CHAROPTIONS_OTHER);
	SendMessageW(hOptions, CB_SETCURSEL, 0, 0);
	SetWindowSubclass(hOptions, ComboProc, 0, 0);
	//WCHAR explain_combobox[] = L"Also separates the lines";
	//CreateToolTip(COMBO_BOX, hwnd, explain_combobox);

	hInitialText = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE //| WS_DISABLED 
		, 78, y_place + 35, 105, 20, hwnd, (HMENU)INITIALFINALCHAR, NULL, NULL);
	AWT(hInitialText, LANG_CONTROL_INITIALCHAR);

	hInitialChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER |ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_place + 34, 20, 21, hwnd, NULL, NULL, NULL);
	SendMessageW(hInitialChar, EM_LIMITTEXT, 1, 0);

	hFinalText = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE //| WS_DISABLED
		, 78, y_place + 65, 105, 20, hwnd, (HMENU)INITIALFINALCHAR, NULL, NULL);
	AWT(hFinalText, LANG_CONTROL_FINALCHAR);

	hFinalChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER| WS_TABSTOP| WS_DISABLED
		, 195, y_place + 64, 20, 21, hwnd, NULL, NULL, NULL);
	SendMessageW(hFinalChar, EM_LIMITTEXT, 1, 0);

	hRemove = CreateWindowW(unCap_wndclass_button, NULL, WS_VISIBLE | WS_CHILD| WS_TABSTOP
		, 256, y_place + 44, 70, 30, hwnd, (HMENU)REMOVE, NULL, NULL);
	AWT(hRemove, LANG_CONTROL_REMOVE);
	UNCAPBTN_set_brushes(hRemove, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

	hMessage = CreateWindowW(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE
		, 256+70+10, y_place + 44, 245, 30,hwnd,(HMENU)TIMEDMESSAGES,NULL,NULL); //INFO: width will be as large as needed to show the needed string
	SetWindowSubclass(hMessage, ShowMessageProc, 0, 0);
	SendMessage(hMessage, UNCAP_SETTEXTDURATION, 5000, 0);

	//hRemoveProgress = CreateWindowExW(0,PROGRESS_CLASS,(LPWSTR)NULL, WS_CHILD
	//	, 256, y_place + 74, 70, 30,hwnd, (HMENU)NULL,NULL,NULL);

	//TODO(fran): init common controls for tab control
	//INFO:https://docs.microsoft.com/en-us/windows/win32/controls/tab-controls
	TextContainer = CreateWindowExW(WS_EX_ACCEPTFILES, WC_TABCONTROL, NULL, WS_CHILD | WS_VISIBLE | TCS_FORCELABELLEFT | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH
							, 10, y_place + 104, 664 - 50, 618, hwnd, (HMENU)TABCONTROL, NULL, NULL);
	TabCtrl_SetItemExtra(TextContainer, sizeof(TEXT_INFO));
	int tabWidth = 100,tabHeight=20;
	SendMessage(TextContainer, TCM_SETITEMSIZE, 0, MAKELONG(tabWidth, tabHeight));
	SetWindowSubclass(TextContainer, TabProc,0,0);

	TabCloseButtonInfo.icon.cx = (LONG)(tabHeight*.6f);
	TabCloseButtonInfo.icon.cy = TabCloseButtonInfo.icon.cx;
	TabCloseButtonInfo.rightPadding = (tabHeight-TabCloseButtonInfo.icon.cx)/2;

	SendMessage(hFile, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hRemoveCommentWith, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hOptions, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hInitialText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hInitialChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hFinalText, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hFinalChar, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hRemove, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(TextContainer, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
	SendMessage(hMessage, WM_SETFONT, (WPARAM)hGeneralFont, TRUE);
}

bool HACK_EnableOtherChar = false;

void EnableOtherChar(bool op) {
	//EnableWindow(hInitialText, op);
	//EnableWindow(hFinalText, op);
	HACK_EnableOtherChar = op;
	InvalidateRect(hInitialText, NULL, TRUE);
	InvalidateRect(hFinalText, NULL, TRUE);
	EnableWindow(hInitialChar, op);
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
						i += (int)wcslen(&new_file.lpstrFile[i + 1]);//TODO(fran): check this is actually faster
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

	for (UINT i = 0; i < file_count; i++) {
		UINT requiredChars = DragQueryFileW(hDrop, i, NULL, NULL);//INFO: does NOT include terminating NULL char
		Assert( requiredChars < (sizeof(lpszFile) / sizeof(lpszFile[0])) );

		if (DragQueryFileW(hDrop, i, lpszFile, sizeof(lpszFile) / sizeof(lpszFile[0]))) {//retrieve the string

			if (fs::is_directory(fs::path(lpszFile))) {//user dropped a folder
				std::vector<wstring>files = GetFiles(lpszFile);
				//GetFiles2(lpszFile);

				for (wstring const& file : files) {
					AcceptedFile(file); //process the new file
				}

			} else AcceptedFile(lpszFile); //process the new file

		}
		else MessageBoxW(NULL, RCS(LANG_DRAGDROP_ERROR_FILE), RCS(LANG_ERROR), MB_ICONERROR);
	}

	DragFinish(hDrop); //free mem
}

ENCODING GetTextEncoding(wstring filename) {
	
	HANDLE file;
	DWORD  dwBytesRead = 0;
	BYTE   header[4] = { 0 };

	file = CreateFile(filename.c_str(),// file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	Assert(file != INVALID_HANDLE_VALUE);

	BOOL read_res = ReadFile(file, header, 4, &dwBytesRead, NULL);
	Assert(read_res);

	CloseHandle(file);

	if (header[0] == 0xEF && header[1] == 0xBB && header[2] == 0xBF)	return ENCODING::UTF8; //File has UTF8 BOM
	else if (header[0] == 0xFF && header[1] == 0xFE)		return ENCODING::UTF16; //File has UTF16LE BOM, INFO: this could actually be UTF32LE but I've never seen a subtitle file enconded in utf32 so no real need to do the extra checks
	else if (header[0] == 0xFE && header[1] == 0xFF)		return ENCODING::UTF16; //File has UTF16BE BOM
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

		file = CreateFile(filename.c_str(),GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); //Open file for reading
		Assert(file != INVALID_HANDLE_VALUE);

		BYTE fixedBuffer[50000] = {0}; //We dont need the entire file, just enough data for the functions to be as accurate as possible. Pd. this is probably too much already
		read_res = ReadFile(file, fixedBuffer, 50000, &dwBytesRead, NULL);
		Assert(read_res);

		CloseHandle(file);

		AutoIt::Common::TextEncodingDetect textDetect;
		bool isUTF16 = textDetect.isUTF16(fixedBuffer,dwBytesRead);

		if (isUTF16) return ENCODING::UTF16;

		//Give up and return ANSI
		return ENCODING::ANSI;
	}
}

COMMENT_TYPE CommentTypeFound(wstring &text) {
	if (text.find_first_of(L'{') != wstring::npos) {
		return COMMENT_TYPE::braces;
	}
	else if (text.find_first_of(L'[') != wstring::npos) {
		return COMMENT_TYPE::brackets;
	}
	else if (text.find_first_of(L'(') != wstring::npos) {
		return COMMENT_TYPE::parenthesis;
	}
	else {
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

	ENCODING encoding = GetTextEncoding(filepath);

	wifstream file(filepath, ios::binary);

	if (file.is_open()) {

		//set encoding for getline
		if (encoding == ENCODING::UTF8)
			file.imbue(locale(std::locale::empty(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
			//int a;
			//maybe the only thing needed when utf8 is detectedd is to remove the BOM if it is there
		else if (encoding == ENCODING::UTF16)
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
FILE_FORMAT GetFileFormat(wstring& filename) {
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
	wcsncpy_s(text_data.filePath, filename.c_str(), sizeof(text_data.filePath) / sizeof(text_data.filePath[0]));

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
		create_info_file << info_parameters[BACKUP] << Backup_Is_Checked << L"\n"; //SDH
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
	
	for (unsigned int param = 0; param < NRO_INFO_PARAMETERS; param++) {

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

void UNCAPCL_ResizeWindows(HWND mainWindow) {
	RECT rect;
	GetClientRect(mainWindow, &rect);
	//MoveWindow(hFile, 10, y_place, RECTWIDTH(rect) - 36, 20, TRUE);
	MoveWindow(hMessage, 256 + 70 + 10, y_place + 44, RECTWIDTH(rect) - (256+70+4) - 36, 30, TRUE);
	//@No se si actualizar las demas
	int txtcont_top = y_place + 104;
	MoveWindow(TextContainer, 10, txtcont_top, RECTWIDTH(rect) - 20, RECTHEIGHT(rect) - txtcont_top - 10, TRUE);
	SendMessage(TextContainer, TCM_RESIZETABS, 0, 0);
}

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
LRESULT CALLBACK ShowMessageProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//INFO: for this procedure and its hwnds we are going to try the SetProp method for storing info in the hwnd
	TCHAR text_duration[] = TEXT("TextDuration_unCap");
	switch (Msg)
	{
	case UNCAP_SETTEXTDURATION:
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
			UINT msgDuration = (UINT)GetProp(hwnd, text_duration);	//Retrieve the user defined duration of text on screen
																	//If this value was not set then the timer is not used and the text remains on screen
			if(msgDuration)
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

LRESULT CALLBACK ComboProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	static BOOL MouseOverCombo = FALSE;
	static HWND CurrentMouseOverCombo;

	switch (Msg)
	{
	case CB_SETCURSEL:
	{
		EnableOtherChar(wParam == COMMENT_TYPE::other);
		InvalidateRect(hwnd, NULL, TRUE); //Simple hack ->//TODO(fran): parts of the combo dont update and stay white when the tab tells it to change
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_MOUSEHOVER:
	{
		//force button to repaint and specify hover or not
		if (MouseOverCombo && CurrentMouseOverCombo == hwnd) break;
		MouseOverCombo = true;
		CurrentMouseOverCombo = hwnd;
		InvalidateRect(hwnd, 0, 1);
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_MOUSELEAVE:
	{
		MouseOverCombo = false;
		CurrentMouseOverCombo = NULL;
		InvalidateRect(hwnd, 0, 1);
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		//TODO(fran): We are tracking the mouse every single time it moves, kind of suspect solution
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = hwnd;

		TrackMouseEvent(&tme);
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	//case CBN_DROPDOWN://lets us now that the list is about to be show, therefore the user clicked us
	case WM_PAINT:
	{
		DWORD style = (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE);
		if (!(style & CBS_DROPDOWNLIST))
			break;

		RECT rc;
		GetClientRect(hwnd, &rc);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		BOOL ButtonState = (BOOL)SendMessageW(hwnd, CB_GETDROPPEDSTATE, 0, 0);
		if (ButtonState) {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBkPush));
			FillRect(hdc, &rc, unCap_colors.ControlBkPush);
		}
		else if (MouseOverCombo && CurrentMouseOverCombo == hwnd) {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBkMouseOver));
			FillRect(hdc, &rc, unCap_colors.ControlBkMouseOver);
		}
		else {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBk));
			FillRect(hdc, &rc, unCap_colors.ControlBk);
		}

		RECT client_rec;
		GetClientRect(hwnd, &client_rec);

		HPEN pen = CreatePen(PS_SOLID, max(1, (int)((RECTHEIGHT(client_rec))*.01f)), ColorFromBrush(unCap_colors.ControlTxt)); //para el borde

		HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));//para lo de adentro
		HPEN oldpen = (HPEN)SelectObject(hdc, pen);

		SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		//SetBkColor(hdc, bkcolor);
		SetTextColor(hdc, ColorFromBrush(unCap_colors.ControlTxt));

		//Border
		Rectangle(hdc, 0, 0, rc.right, rc.bottom);

		/*
		if (GetFocus() == hwnd)
		{
			//INFO: with this we know when the control has been pressed
			RECT temp = rc;
			InflateRect(&temp, -2, -2);
			DrawFocusRect(hdc, &temp);
		}
		*/
		int DISTANCE_TO_SIDE = 5;

		int index = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
		if (index >= 0)
		{
			int buflen = (int)SendMessage(hwnd, CB_GETLBTEXTLEN, index, 0);
			TCHAR *buf = new TCHAR[(buflen + 1)];
			SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
			rc.left += DISTANCE_TO_SIDE;
			DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			delete[]buf;
		}
		RECT r;
		SIZE icon_size;
		GetClientRect(hwnd, &r);
		icon_size.cy = (LONG)((float)(r.bottom - r.top)*.6f);
		icon_size.cx = icon_size.cy;

		HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
		//LONG icon_id = GetWindowLongPtr(hwnd, GWL_USERDATA);
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

		EndPaint(hwnd, &ps);
		return 0;
	}

	//case WM_NCDESTROY:
	//{
	//	RemoveWindowSubclass(hwnd, this->ComboProc, uIdSubclass);
	//	break;
	//}

	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

v2 GetDPI(HWND hwnd)//https://docs.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
{
	HDC hdc = GetDC(hwnd);
	v2 dpi;
	dpi.x = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	dpi.y = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(hwnd, hdc);
	return dpi;
}

void MenuChangeLang(LANGUAGE_MANAGER::LANGUAGE new_lang) {
	LANGUAGE_MANAGER::LANGUAGE old_lang = LANGUAGE_MANAGER::Instance().GetCurrentLanguage();
	LANGUAGE_MANAGER::Instance().ChangeLanguage(new_lang);

	//Uncheck the old lang
	CheckMenuItem(hFileMenu, old_lang, MF_BYCOMMAND | MF_UNCHECKED);

	//Check the new one
	CheckMenuItem(hFileMenu, new_lang, MF_BYCOMMAND | MF_CHECKED);
}

void DrawMenuArrow(HDC destDC, RECT& r)
{
	//Thanks again https://www.codeguru.com/cpp/controls/menu/miscellaneous/article.php/c13017/Owner-Drawing-the-Submenu-Arrow.htm
	//NOTE: I dont know if this will be final, dont really wanna depend on windows, but it's pretty good for now. see https://docs.microsoft.com/en-us/windows/win32/gdi/scaling-an-image maybe some of those stretch modes work better than HALFTONE

	//Create the DCs and Bitmaps we will need
	HDC arrowDC = CreateCompatibleDC(destDC);
	HDC fillDC = CreateCompatibleDC(destDC);
	int arrowW = RECTWIDTH(r);
	int arrowH = RECTHEIGHT(r);
	HBITMAP arrowBitmap = CreateCompatibleBitmap(destDC, arrowW, arrowH);
	HBITMAP oldArrowBitmap = (HBITMAP)SelectObject(arrowDC, arrowBitmap);
	HBITMAP fillBitmap = CreateCompatibleBitmap(destDC, arrowW, arrowH);
	HBITMAP oldFillBitmap = (HBITMAP)SelectObject(fillDC, fillBitmap);

	//Set the offscreen arrow rect
	RECT tmpArrowR = rectWH(0, 0, arrowW, arrowH);

	//Draw the frame control arrow (The OS draws this as a black on white bitmap mask)
	DrawFrameControl(arrowDC, &tmpArrowR, DFC_MENU, DFCS_MENUARROW);

	//Set the arrow color
	HBRUSH arrowBrush = unCap_colors.Img;

	//Fill the fill bitmap with the arrow color
	FillRect(fillDC, &tmpArrowR, arrowBrush);

	//Blit the items in a masking fashion
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, fillDC, 0, 0, SRCINVERT);
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, arrowDC, 0, 0, SRCAND);
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, fillDC, 0, 0, SRCINVERT);

	//Clean up
	SelectObject(fillDC, oldFillBitmap);
	DeleteObject(fillBitmap);
	SelectObject(arrowDC, oldArrowBitmap);
	DeleteObject(arrowBitmap);
	DeleteDC(fillDC);
	DeleteDC(arrowDC);
}

void DrawMenuImg(HDC destDC, RECT& r, HBITMAP mask) {
	int imgW = RECTWIDTH(r);
	int imgH = RECTHEIGHT(r);

	HBRUSH imgBr = unCap_colors.Img;
	HBRUSH oldBr = SelectBrush(destDC, imgBr);

	//TODO(fran): I have no idea why I need to pass the "srcDC", no information from it is needed, and the function explicitly says it should be NULL, but if I do it returns false aka error (some error, cause it also doesnt tell you what it is)
	//NOTE: info on creating your own raster ops https://docs.microsoft.com/en-us/windows/win32/gdi/ternary-raster-operations?redirectedfrom=MSDN
	//Thanks https://stackoverflow.com/questions/778666/raster-operator-to-use-for-maskblt
	BOOL res = MaskBlt(destDC, r.left, r.top, imgW, imgH, destDC, 0, 0, mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY)); 
	SelectBrush(destDC, oldBr);
}

struct unCapClProcState {
	HWND wnd;
	HWND nc_parent;
};

ATOM init_wndclass_unCap_uncap_cl(HINSTANCE inst) {
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


LRESULT CALLBACK UncapClProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	unCapClProcState * state = (unCapClProcState *)GetWindowLongPtr(hwnd, 0);
	switch (msg) {
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		switch (wparam) {//wParam specifies the identifier of the control that needs painting
		case TABCONTROL:
		{
			switch (item->itemAction) {//Determines what I have to do with the control
			case ODA_DRAWENTIRE://Draw everything
			case ODA_FOCUS://Control lost or gained focus, check itemState
			case ODA_SELECT://Selection status changed, check itemState
				//Fill background //TODO(fran): the original background is still visible for some reason
				FillRect(item->hDC, &item->rcItem, unCap_colors.ControlBk);
				//IMPORTANT INFO: this is no region set so I can actually draw to the entire control, useful for going over what the control draws

				UINT index = item->itemID;

				//Draw text
				wstring text = GetTabTitle(index);

				TEXTMETRIC tm;
				GetTextMetrics(item->hDC, &tm);
				// Calculate vertical position for the item string so that it will be vertically centered

				int yPos = (item->rcItem.bottom + item->rcItem.top - tm.tmHeight) / 2;

				int xPos = item->rcItem.left + (int)((float)RECTWIDTH(item->rcItem)*.1f);


				//SIZE text_size;
				//GetTextExtentPoint32(item->hDC, text.c_str(), text.length(), &text_size);

				//INFO TODO: probably simpler is to just mark a region and then unmark it, not sure how it will work for animation though
				//also I'm not sure how this method handles other unicode chars, ie japanese
				int max_count;
				SIZE fulltext_size;
				RECT modified_rc = item->rcItem;
				modified_rc.left = xPos;
				modified_rc.right -= (TabCloseButtonInfo.rightPadding + TabCloseButtonInfo.icon.cx);
				BOOL res = GetTextExtentExPoint(item->hDC, text.c_str(), (int)text.length()
					, RECTWIDTH(modified_rc)/*TODO(fran):This should be DPI aware*/, &max_count, NULL, &fulltext_size);
				Assert(res);
				int len = max_count;


				COLORREF old_bk_color = GetBkColor(item->hDC);
				COLORREF old_txt_color = GetTextColor(item->hDC);

				SetBkColor(item->hDC, ColorFromBrush(unCap_colors.ControlBk));
				SetTextColor(item->hDC, ColorFromBrush(unCap_colors.ControlTxt));
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
				//FillRect(item->hDC, &r, unCap_colors.ControlTxt);

				HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
				//LONG icon_id = GetWindowLongPtr(hwnd, GWL_USERDATA);
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
	case WM_CTLCOLORSTATIC:
	{
		//TODO(fran): check we are changing the right controls
		switch (GetDlgCtrlID((HWND)lparam)) {
		case TIMEDMESSAGES:
		{
			SetBkColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlMsg));
			return (LRESULT)unCap_colors.ControlBk;
		}
		case INITIALFINALCHAR:
		{
			SetBkColor((HDC)wparam, ColorFromBrush(unCap_colors.ControlBk));
			COLORREF txt=NULL;
			if (HACK_EnableOtherChar) {
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
		AddControls(state->wnd, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
		AddMenus(state->nc_parent); //TODO(fran): menu rendering //TODO(fran): nc_parent has to re-send all the menu msgs to me, the client, via the standard WM_COMMAND way it gets them

		return 0;
	} break;
	case WM_SIZE:
	{
		UNCAPCL_ResizeWindows(state->wnd);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCDESTROY:
	{
		CreateInfoFile(hwnd);
		if (state) {
			free(state);
			state = nullptr;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCCREATE:
	{
		CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
		unCapClProcState* state = (unCapClProcState*)calloc(1, sizeof(unCapClProcState));
		Assert(state);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)state);
		state->wnd = hwnd;
		state->nc_parent = create_nfo->hwndParent;
		return TRUE;
	} break;
	case WM_NOTIFY:
	{
		NMHDR* msg_info = (NMHDR*)lparam;
		switch (msg_info->code) {
			//Handling tab changes, since there is no specific msg to manage this from inside the tab control, THANKS WINDOWS
		case TCN_SELCHANGING: //TODO(fran): now we are outside the tab control we should use the other parameters to check we are actually changing the control we want
		{
			SaveAndDisableCurrentTab(msg_info->hwndFrom);
			return FALSE; //FALSE=Allow selection to change. TRUE= prevent it from changing
		}
		case TCN_SELCHANGE:
		{
			int index = (int)SendMessage(msg_info->hwndFrom, TCM_GETCURSEL, 0, 0);
			if (index != -1) {
				CUSTOM_TCITEM new_item;
				new_item.tab_info.mask = TCIF_PARAM; //|TCIF_TEXT;
				//WCHAR buf[20] = { 0 };
				//prev_item.tab_info.pszText = buf;
				//prev_item.tab_info.cchTextMax = 20;
				BOOL ret = (BOOL)SendMessage(msg_info->hwndFrom, TCM_GETITEM, index, (LPARAM)&new_item);
				if (ret) {
					EnableTab(new_item.extra_info);
				}
			}
			return 0;//Return value isnt used
		}
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	} break;
	case WM_COMMAND:
	{
		// Msgs from my controls
		switch (LOWORD(wparam))
		{
		case OPEN_FILE:
		case ID_OPEN:
		{
			ChooseFile(L"srt");
			return 0;
		}
		case BACKUP_FILE: //Do backup menu item was clicked
		{
			if (Backup_Is_Checked) {
				CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_UNCHECKED); //turn it off
			}
			else {
				CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);//turn it on
			}
			Backup_Is_Checked = !Backup_Is_Checked;
			printf("BACKUP=%s\n", Backup_Is_Checked ? "true" : "false");
			return 0;
		}
		case COMBO_BOX:
			if (HIWORD(wparam) == CBN_SELCHANGE)
				EnableOtherChar(SendMessageW(hOptions, CB_GETCURSEL, 0, 0) == COMMENT_TYPE::other);
			return 0;

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
			return 0;
		}
		case SAVE_FILE:
		case ID_SAVE:
		{
			//if (IsAcceptedFile()) {
			if (Backup_Is_Checked) DoBackup();

			int text_length = GetWindowTextLengthW(hFile) + 1;//TODO(fran): this is pretty ugly, maybe having duplicate controls is not so bad of an idea

			wstring text(text_length, L'\0');
			GetWindowTextW(hFile, &text[0], text_length);

			TEXT_INFO text_data = GetCurrentTabExtraInfo();

			if (text[0] != L'\0')
				DoSave(text_data.hText, text);
			//TODO(fran): error handling

			//}
			return 0;
		}
		case SAVE_AS_FILE:
		case ID_SAVE_AS:
		{
			//TODO(fran): DoSaveAs allows for saving of file when no tab is created and sets all the controls but doesnt create an empty tab,
			//should we create the tab or not allow to save?
			DoSaveAs(GetCurrentTabExtraInfo().hText); //No DoBackup() for Save As since the idea is the user saves to a new file obviously
			return 0;
		}
		case LANGUAGE_MANAGER::LANGUAGE::ENGLISH://INFO: this relates to the menu item, TODO(fran): a way to check a range of numbers, from first lang to last
		case LANGUAGE_MANAGER::LANGUAGE::SPANISH:
		{
			//NOTE: maybe I can force menu item remeasuring by SetMenu, basically sending it the current menu
			MenuChangeLang((LANGUAGE_MANAGER::LANGUAGE)LOWORD(wparam));
#if 1 //One way of updating the menus and getting them recalculated, destroy everything and create it again, problems: gotta change internal code to fix some state which doesnt take into account the realtime values, gotta take language_manager related things out of the function to avoid repetition, or add checks in language_mgr to ignore repeated objects
			HMENU old_menu = GetMenu(state->nc_parent);
			SetMenu(state->nc_parent, NULL);
			DestroyMenu(old_menu); //NOTE: I could also use RemoveMenu which doesnt destroy it's submenus and reattach them to a new "main" menu and correct parenting (itemData param)
			AddMenus(state->nc_parent); //TODO(fran): get this working, we are gonna need to implement removal into lang_mgr, and other things
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
		default: return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	} break;
	//case WM_ERASEBKGND:
	//{
	//	return 1;
	//} break;
	default:
//#ifdef _DEBUG //TODO(fran): uncomment
//		Assert(0);
//#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
//#endif
	}
	return 0;
}

struct unCapNcProcState {
	HWND wnd;//TODO(fran): might be good to store client and wnd rect, find the one place where we get this two. Then we wont have the code littered with those two calls
	HWND client;

	HWND btn_min, btn_max, btn_close;

	RECT rc_caption;
	SIZE caption_btn;
	bool active;
	bool has_menu;
};

ATOM init_wndclass_unCap_uncap_nc(HINSTANCE inst) {
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = UncapNcProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(unCapNcProcState*);
	wcex.hInstance = inst;
	wcex.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_SRTFIXWIN32));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = unCap_colors.ControlBk;
	wcex.lpszMenuName = 0;// MAKEINTRESOURCEW(IDC_SRTFIXWIN32);//TODO(fran): remove?
	wcex.lpszClassName = unCap_wndclass_uncap_nc;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	ATOM class_atom = RegisterClassExW(&wcex);
	Assert(class_atom);
	return class_atom;
}

int FillRectAlpha(HDC dc, RECT* r, HBRUSH br, unsigned char alpha) {
	COLORREF color = ColorFromBrush(br);
	color |= (alpha << 24);

	BITMAPINFO nfo;
	nfo.bmiHeader.biSize = sizeof(nfo.bmiHeader);
	nfo.bmiHeader.biWidth = 1;
	nfo.bmiHeader.biHeight = -1;
	nfo.bmiHeader.biPlanes = 1;
	nfo.bmiHeader.biBitCount = 32;
	nfo.bmiHeader.biCompression = BI_RGB;
	nfo.bmiHeader.biSizeImage = 0;
	nfo.bmiHeader.biXPelsPerMeter = 0;
	nfo.bmiHeader.biYPelsPerMeter = 0;
	nfo.bmiHeader.biClrUsed = 0;
	nfo.bmiHeader.biClrImportant = 0;

	int res = StretchDIBits(dc, r->left, r->top, RECTWIDTH((*r)), RECTHEIGHT((*r)), 0, 0, 1, 1, &color, &nfo, DIB_RGB_COLORS, SRCCOPY);
	return res;
}

//INFO: WM_NCACTIVATE also does painting, again making no sense, but we can use it to know when to change drawing from active to inactive

void UNCAPNC_calc_caption(unCapNcProcState* state) {
	GetClientRect(state->wnd, &state->rc_caption);

	RECT testrc{ 0,0,100,100 }; RECT ncrc = testrc;
	AdjustWindowRect(&ncrc, WS_OVERLAPPEDWINDOW, FALSE);//no menu

	state->rc_caption.bottom = distance(testrc.top, ncrc.top);

	state->caption_btn.cy = RECTHEIGHT(state->rc_caption); state->caption_btn.cx = state->caption_btn.cy * 16 / 9;;
}

RECT UNCAPNC_calc_menu_rc(unCapNcProcState* state) {
	RECT rc{0};
	if (state->has_menu) {
		RECT r; GetClientRect(state->wnd, &r);
		rc.left = r.left;
		rc.top = RECTHEIGHT(state->rc_caption);
		rc.right = r.right;
		rc.bottom = rc.top + GetSystemMetrics(SM_CYMENU);
	}
	return rc;
}

RECT UNCAPNC_calc_client_rc(unCapNcProcState* state) {
	RECT r; GetClientRect(state->wnd, &r);
	RECT menu_rc = UNCAPNC_calc_menu_rc(state);
	RECT rc{ r.left, RECTHEIGHT(state->rc_caption) + RECTHEIGHT(menu_rc), r.right, r.bottom };
	return rc;
}

RECT UNCAPNC_calc_btn_min_rc(unCapNcProcState* state) {
	RECT r; GetClientRect(state->wnd, &r);
	RECT rc{ r.right - 3 * state->caption_btn.cx, r.top, r.right - 2 * state->caption_btn.cx, r.top + state->caption_btn.cy };
	return rc;
}

RECT UNCAPNC_calc_btn_max_rc(unCapNcProcState* state) {
	RECT r; GetClientRect(state->wnd, &r);
	RECT rc{ r.right - 2 * state->caption_btn.cx, r.top, r.right - 1 * state->caption_btn.cx, r.top + state->caption_btn.cy };
	return rc;
}

RECT UNCAPNC_calc_btn_close_rc(unCapNcProcState* state) {
	RECT r; GetClientRect(state->wnd, &r);
	RECT rc{ r.right - 1 * state->caption_btn.cx, r.top, r.right - 0 * state->caption_btn.cx, r.top + state->caption_btn.cy };
	return rc;
}

#define WM_UAHDESTROYWINDOW 0x90
#define WM_UAHDRAWMENU 0x91
#define WM_UAHDRAWMENUITEM 0x92
#define WM_UAHINITMENU 0x93
#define WM_UAHMEASUREMENUITEM 0x94
#define WM_UAHNCPAINTMENUPOPUP 0x95
#define WM_UAHUPDATE 0x96

//TODO(fran): UNDO support for comment removal
//TODO(fran): DPI awareness
LRESULT CALLBACK UncapNcProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//TODO(fran): beware of the menu bar, it decided to draw itself for some reason once I moved the window
	//TODO(fran): it'd be nice to have a way to implement good subclassing, eg letting the user assign clip regions where they can draw and we dont, things like that, more communication
#define UNCAPNC_MINIMIZE 100 //sent through WM_COMMAND
#define UNCAPNC_MAXIMIZE 101 //sent through WM_COMMAND
#define UNCAPNC_CLOSE 102 //sent through WM_COMMAND

	//printf(msgToString(msg)); printf("\n");
	unCapNcProcState* state = (unCapNcProcState*)GetWindowLongPtr(hwnd, 0);

	switch (msg)
	{
	case WM_UAHDESTROYWINDOW: //Only called when the window is being destroyed, wparam=lparam=0
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_UAHDRAWMENU: //Non here were ever called
	case WM_UAHDRAWMENUITEM:
	case WM_UAHMEASUREMENUITEM:
	case WM_UAHNCPAINTMENUPOPUP:
	case WM_UAHUPDATE:
	{
		Assert(0);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_UAHINITMENU: //After a SetMenu: lParam=some system generated HMENU with your HMENU stored in the ""unused"" member ; wParam=0
							//NOTE: this system HMENU uses the first byte from the top 32bits, no other HMENU does that
						 //When clicking on a submenu:
	{
		/*static int c = 1;
		printf("%d: WM_UAHINITMENU\n",c++);*/
		//TODO(fran): since this msg gets called multiple times and not only for the main menu it is not reliable, we should add an intermediary call for the user, and we call SetMenu ourselves, same for other menu procedures

		//NOTE: for some reason, after SetMenu, this msg gets sent more than once after different events, but always with the same params. This does not depend on the number of menus/submenus it has, but it seems like SetMenu checks that your HMENU has at least one submenu, otherwise this msg doesnt get sent
		HMENU menu = (HMENU)lparam;

		//{
		//	MENUINFO mi{ sizeof(mi) };mi.fMask = MIM_BACKGROUND; mi.hbrBack = state->active ? unCap_colors.CaptionBk : unCap_colors.CaptionBk_Inactive;
		//	BOOL res = SetMenuInfo(hMenu, &mi); Assert(res);
		//}

		//TODO(fran): why on the first time we create a menu its borders are only 1 pixel but the next times they grow to like 3 ???? looks awful

		if (menu->unused == (int)hMenu) {
			MENUINFO mi { sizeof(mi) };
			mi.fMask = MIM_BACKGROUND;
			//TODO(fran): MIM_MAXHEIGHT and other fMask params that we should set
			
			mi.hbrBack = state->active ? unCap_colors.CaptionBk : unCap_colors.CaptionBk_Inactive;

			SetMenuInfo(hMenu, &mi);
		}
		Assert(wparam == 0);
		state->has_menu = true;


		//Ask to update client area to accommodate for menu
		RECT newcl = UNCAPNC_calc_client_rc(state); //TODO(fran): maybe I should start using this like windows wants me to
		WORD w = RECTWIDTH(newcl), h = RECTHEIGHT(newcl);
		SendMessage(state->wnd, WM_SIZE, SIZE_RESTORED, MAKELONG(w,h));
		
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGED: //TODO(fran): we can use this guy to replace WM_SIZE and WM_MOVE by not calling defwindowproc
	{
		UNCAPNC_calc_caption(state);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(state->wnd, &ps);

		FillRect(dc, &state->rc_caption, state->active ? unCap_colors.CaptionBk : unCap_colors.CaptionBk_Inactive);

		SelectFont(dc, GetStockFont(DEFAULT_GUI_FONT));
		TCHAR title[256]; int sz = GetWindowText(state->wnd, title, ARRAYSIZE(title));
		TEXTMETRIC tm; GetTextMetrics(dc, &tm);


		HICON icon = (HICON)GetClassLongPtr(state->wnd, GCLP_HICONSM);
		int icon_height = (int)((float)tm.tmHeight*1.5f);
		int icon_width = icon_height;
		int icon_align_height = (RECTHEIGHT(state->rc_caption) - icon_height) / 2;
		int icon_align_width = icon_align_height;
		MYICON_INFO iconnfo = MyGetIconInfo(icon);
		urender::draw_icon(dc, icon_align_height, icon_align_width, icon_width, icon_height, icon, 0, 0, iconnfo.nWidth, iconnfo.nHeight);

		int yPos = (state->rc_caption.bottom + state->rc_caption.top - tm.tmHeight) / 2;
		HBRUSH txtbr = state->active ? unCap_colors.ControlTxt : unCap_colors.ControlTxt_Inactive;
		SetTextColor(dc, ColorFromBrush(txtbr)); SetBkMode(dc, TRANSPARENT);


		TextOut(dc, icon_align_width * 2 + icon_width, yPos, title, sz);

		if (state->active) {
			UNCAPBTN_set_brushes(state->btn_close, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, 0, 0);
			UNCAPBTN_set_brushes(state->btn_min, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, 0, 0);
			UNCAPBTN_set_brushes(state->btn_max, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, 0, 0);
		}
		else {
			UNCAPBTN_set_brushes(state->btn_close, TRUE, unCap_colors.CaptionBk_Inactive, unCap_colors.CaptionBk_Inactive, unCap_colors.ControlTxt_Inactive, 0, 0);
			UNCAPBTN_set_brushes(state->btn_min, TRUE, unCap_colors.CaptionBk_Inactive, unCap_colors.CaptionBk_Inactive, unCap_colors.ControlTxt_Inactive, 0, 0);
			UNCAPBTN_set_brushes(state->btn_max, TRUE, unCap_colors.CaptionBk_Inactive, unCap_colors.CaptionBk_Inactive, unCap_colors.ControlTxt_Inactive, 0, 0);
		}
		DrawMenuBar(state->wnd); //TODO(fran): we need to update the menu more often, specifically when the mouse goes over some submenu in the menu bar

		EndPaint(state->wnd, &ps);


		return 0;
	} break;
	case WM_CREATE:
	{

		CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam; 
		SetWindowText(state->wnd, createnfo->lpszName); //TODO(fran): for some weird reason I have to manually call setwindowtext

		RECT btn_min_rc = UNCAPNC_calc_btn_min_rc(state);
		state->btn_min = CreateWindow(unCap_wndclass_button, TEXT(""), WS_CHILD | WS_VISIBLE | BS_BITMAP, btn_min_rc.left, btn_min_rc.top, RECTWIDTH(btn_min_rc), RECTHEIGHT(btn_min_rc), state->wnd, (HMENU)UNCAPNC_MINIMIZE, 0, 0);
		UNCAPBTN_set_brushes(state->btn_min, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		RECT btn_max_rc = UNCAPNC_calc_btn_max_rc(state);
		state->btn_max = CreateWindow(unCap_wndclass_button, TEXT(""), WS_CHILD | WS_VISIBLE | BS_BITMAP, btn_max_rc.left, btn_max_rc.top, RECTWIDTH(btn_max_rc), RECTHEIGHT(btn_max_rc), state->wnd, (HMENU)UNCAPNC_MAXIMIZE, 0, 0);
		UNCAPBTN_set_brushes(state->btn_max, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		RECT btn_close_rc = UNCAPNC_calc_btn_close_rc(state);
		state->btn_close = CreateWindow(unCap_wndclass_button, TEXT(""), WS_CHILD | WS_VISIBLE | BS_BITMAP, btn_close_rc.left, btn_close_rc.top, RECTWIDTH(btn_close_rc), RECTHEIGHT(btn_close_rc), state->wnd,(HMENU)UNCAPNC_CLOSE, 0, 0);
		UNCAPBTN_set_brushes(state->btn_close, TRUE, unCap_colors.CaptionBk, unCap_colors.CaptionBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		HBITMAP bCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(CLOSE));//TODO(fran): let the user set this guys, store them in state
		HBITMAP bMax = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(MAX));
		HBITMAP bMin = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(MIN));
		SendMessage(state->btn_close, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bCross);
		SendMessage(state->btn_max, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bMax);
		SendMessage(state->btn_min, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bMin);

		//TODO(fran): create the icons, one idea is to ask windows to paint them in some dc, and then I can store the HBITMAP and re-use it all I want

		ATOM class_res = init_wndclass_unCap_uncap_cl((HINSTANCE)GetWindowLongPtr(state->wnd, GWLP_HINSTANCE));
		Assert(class_res);
		RECT rc = UNCAPNC_calc_client_rc(state);
		state->client = CreateWindow(unCap_wndclass_uncap_cl, TEXT(""), WS_CHILD | WS_VISIBLE, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), state->wnd, 0, 0, 0);

		return 0;
	} break;
	case WM_NCLBUTTONDBLCLK:
	{
		LRESULT hittest = (LRESULT)wparam;
		if (hittest == HTCAPTION) {
			WINDOWPLACEMENT p{ sizeof(p) }; GetWindowPlacement(state->wnd, &p);

			if (p.showCmd == SW_SHOWMAXIMIZED) ShowWindow(state->wnd, SW_RESTORE);
			else ShowWindow(state->wnd, SW_MAXIMIZE);
		}
		return 0;
	} break;
	case WM_NCHITTEST:
	{
		
		LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
		if (res == HTCLIENT) {
			int top_margin = 8;
			int top_side_margin = 16;
			POINT mouse{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) }; ScreenToClient(hwnd, &mouse);
			RECT rc; GetClientRect(hwnd, &rc);
			RECT rc_top; rc_top.top = rc.top; rc_top.left = rc.left + top_side_margin; rc_top.bottom = rc_top.top + top_margin; rc_top.right = rc.right - top_side_margin;
			RECT rc_topleft; rc_topleft.top = rc.top; rc_topleft.left = rc.left; rc_topleft.bottom = rc_topleft.top + top_margin; rc_topleft.right = rc_topleft.left + top_side_margin;
			RECT rc_topright; rc_topright.top = rc.top; rc_topright.left = rc.right - top_side_margin; rc_topright.bottom = rc_topright.top + top_margin; rc_topright.right = rc.right;

			RECT rc_menu = UNCAPNC_calc_menu_rc(state);

			if		(test_pt_rc(mouse, rc_top)) res = HTTOP;
			else if (test_pt_rc(mouse, rc_topleft)) res = HTTOPLEFT;
			else if (test_pt_rc(mouse, rc_topright)) res = HTTOPRIGHT;
			/*else if (test_pt_rc(mouse, close_btn)) res = HTCLOSE; //TODO(fran): request this buttons for their rc
			else if (test_pt_rc(mouse, max_btn)) res = HTMAXBUTTON;
			else if (test_pt_rc(mouse, min_btn)) res = HTMINBUTTON;*/
			else if (test_pt_rc(mouse, state->rc_caption)) res = HTCAPTION;
			else if (test_pt_rc(mouse, rc_menu)) res = HTSYSMENU;//HTMENU also works, but it seems a little slower on clicking for some reason, IMPORTANT INFO: this is crucial for the menu to detect clicks, otherwise no mouse input goes to it

			//if (res == HTSYSMENU) {
			//	HMENU submenu = GetSubMenu(hMenu, 0); Assert(submenu);
			//	ClientToScreen(state->wnd, &mouse);
			//	TrackPopupMenu(submenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, mouse.x, mouse.y, 0, state->wnd, 0);
			//}
		}


		//printf("%s\n", hittestToString(res));
		return res;
	} break;
	case WM_NCCALCSIZE:
	{
		RECT testrc{ 0,0,100,100 };
		RECT ncrc = testrc;
		AdjustWindowRect(&ncrc, WS_OVERLAPPEDWINDOW, FALSE); //TODO(fran): somehow link this params with UNCAPNC_calc_caption

		UNCAPNC_calc_caption(state);//TODO(fran): find out which is the one and only magical msg where this can be put in

		struct MY_MARGINS
		{
			int cxLeftWidth;      
			int cxRightWidth;     
			int cyTopHeight;      
			int cyBottomHeight;   
		} margins;

		margins.cxLeftWidth = distance(testrc.left, ncrc.left);
		margins.cxRightWidth = distance(testrc.right, ncrc.right);
		margins.cyBottomHeight = distance(testrc.bottom, ncrc.bottom);
		margins.cyTopHeight = 0;

		if ((BOOL)wparam == TRUE) {
			//IMPORTANT: absolutely _everything_ is in screen coords
			NCCALCSIZE_PARAMS* calcsz = (NCCALCSIZE_PARAMS*)lparam; //input and ouput respectively is and has to be in screen coords
			RECT new_wnd_coords_proposed = calcsz->rgrc[0];
			calcsz->rgrc[1];//old wnd coords
			calcsz->rgrc[2];//old client coords

			//new client coords
			calcsz->rgrc[0].left += margins.cxLeftWidth;
			calcsz->rgrc[0].top += margins.cyTopHeight;
			calcsz->rgrc[0].right -= margins.cxRightWidth;
			calcsz->rgrc[0].bottom -= margins.cyBottomHeight;

			//valid dest coords
			calcsz->rgrc[1] = new_wnd_coords_proposed;

			//valid source coords
			calcsz->rgrc[1] = calcsz->rgrc[0];
		}
		else {
			RECT* calcrc = (RECT*)lparam; //input and ouput respectively is and has to be in screen coords
			RECT new_wnd_coords_proposed = *calcrc;

			//new client coords
			calcrc->left += margins.cxLeftWidth;
			calcrc->top += margins.cyTopHeight;
			calcrc->right -= margins.cxRightWidth;
			calcrc->bottom -= margins.cyBottomHeight;
		}
		return 0; //For wparam==TRUE this means we will reuse the old client area's painting and just align it with the top-left corner of the new client area pos
	} break;
	case WM_SIZE:
	{
		//TODO(fran):resize buttons
		RECT rc = UNCAPNC_calc_client_rc(state); MoveWindow(state->client, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), TRUE);

		RECT btn_min_rc = UNCAPNC_calc_btn_min_rc(state); MoveWindow(state->btn_min, btn_min_rc.left, btn_min_rc.top, RECTWIDTH(btn_min_rc), RECTHEIGHT(btn_min_rc), TRUE);//TODO(fran): I dont really need to ask for repaint do I?
		RECT btn_max_rc = UNCAPNC_calc_btn_max_rc(state); MoveWindow(state->btn_max, btn_max_rc.left, btn_max_rc.top, RECTWIDTH(btn_max_rc), RECTHEIGHT(btn_max_rc), TRUE);
		RECT btn_close_rc = UNCAPNC_calc_btn_close_rc(state); MoveWindow(state->btn_close, btn_close_rc.left, btn_close_rc.top, RECTWIDTH(btn_close_rc), RECTHEIGHT(btn_close_rc), TRUE);

		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SETTEXT:
	{
		//This function is insane, it actually does painting on it's own without telling nobody, so we need a way to kill that
		//I think there are a couple approaches that work, I took this one which works since windows 95 (from what the article says)
		//Many thanks for yet another hack http://www.catch22.net/tuts/win32/custom-titlebar
		LONG_PTR  dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		// turn off WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);

		// perform the default action, minus painting
		LRESULT ret = DefWindowProc(hwnd, msg, wparam, lparam);

		// turn on WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle);

		// perform custom painting, aka dont and do what should be done, repaint, it's really not that expensive for our case, we barely call WM_SETTEXT, and it can be optimized out later
		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);

		return ret;
	} break;
	case WM_NCDESTROY:
	{
		if (state) {
			free(state);
			state = nullptr;
		}
	} break;
	case WM_NCCREATE: 
	{
		CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
		//TODO(fran): check for minimize and maximize button requirements
		unCapNcProcState* state = (unCapNcProcState*)calloc(1, sizeof(unCapNcProcState));
		Assert(state);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)state);
		state->wnd = hwnd;
		return TRUE;
	} break;
	case WM_NCACTIVATE:
	{
		//So basically this guy is another WM_NCPAINT, just do exactly the same, but also here we have the option to paint slightly different if we are deactivated, so the user can see the change
		BOOL active = (BOOL)wparam; //Indicates active or inactive state for a title bar or icon that needs to be changed
		state->active = active;
		HRGN opt_upd_rgn = (HRGN)lparam; // handle to optional update region for the nonclient area. If set to -1, do nothing

		if (state->has_menu) {
			MENUINFO mi{ sizeof(mi) };
			mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS; //TODO(fran): MIM_APPLYTOSUBMENUS is garbage, it only goes one level down, just terrible, make my own
			//TODO(fran): MIM_MAXHEIGHT and other fMask params that we should set

			mi.hbrBack = state->active ? unCap_colors.CaptionBk : unCap_colors.CaptionBk_Inactive;

			SetMenuInfo(hMenu, &mi);
		}

		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE); //TODO(fran): we should draw here, so we can show a different color for deactivated
		return TRUE; //NOTE: it says we can return FALSE to "prevent the change"
	} break;
//NOTE: WM_NCPAINT: check https://chromium.googlesource.com/chromium/chromium/+/5db69ae220c803e9c1675219b5cc5766ea3bb698/chrome/views/window.cc they block drawing so windows doesnt draw on top of them, cause the non client area is also painted in other msgs like settext
//also check https://social.msdn.microsoft.com/Forums/windows/en-US/a407591a-4b1e-4adc-ab0b-3c8b3aec3153/the-evil-wmncpaint?forum=windowsuidevelopment I took the implementation from there, but there's also two others I can try

	case WM_MEASUREITEM: //NOTE: this is so stupid, this guy doesnt send hwndItem like WM_DRAWITEM does, so you have no way of knowing which type of menu you get, gotta do it manually
	{
		//wparam has duplicate info from item
		MEASUREITEMSTRUCT* item = (MEASUREITEMSTRUCT*)lparam;
		if (item->CtlType == ODT_MENU && item->itemData) { //menu with parent HMENU
			//item->CtlID is not used
			
			//Determine which type of menu we're to measure
			MENUITEMINFO menu_type;
			menu_type.cbSize = sizeof(menu_type);
			menu_type.fMask = MIIM_FTYPE;
			GetMenuItemInfo((HMENU)item->itemData, item->itemID, FALSE, &menu_type);
			menu_type.fType ^= MFT_OWNERDRAW; //remove ownerdraw since we know all guys should be
			switch (menu_type.fType) {
			case MFT_STRING:
			{
				//TODO(fran): check if it has a submenu, in which case, if it opens it to the side, we should leave a little more space for an arrow bmp (though there seems to be some extra space added already)

				//Determine text space:
				MENUITEMINFO menu_nfo; menu_nfo.cbSize = sizeof(menu_nfo);
				menu_nfo.fMask = MIIM_STRING;
				menu_nfo.dwTypeData = NULL;
				GetMenuItemInfo((HMENU)item->itemData, item->itemID, FALSE, &menu_nfo);
				UINT menu_str_character_cnt = menu_nfo.cch + 1; //includes null terminator
				menu_nfo.cch = menu_str_character_cnt;
				TCHAR* menu_str = (TCHAR*)malloc(menu_str_character_cnt * sizeof(TCHAR));
				menu_nfo.dwTypeData = menu_str;
				GetMenuItemInfo((HMENU)item->itemData, item->itemID, FALSE, &menu_nfo);

				HDC dc = GetDC(hwnd); //Of course they had to ask for a dc, and not give the option to just provide the font, which is the only thing this function needs
				HFONT hfntPrev = (HFONT)SelectObject(dc, hMenuFont);
				int old_mapmode = GetMapMode(dc);
				SetMapMode(dc, MM_TEXT);
				WORD text_width = LOWORD(GetTabbedTextExtent(dc, menu_str, menu_str_character_cnt - 1,0,NULL)); //TODO(fran): make common function for this and the one that does rendering, also look at how tabs work
				WORD space_width = LOWORD(GetTabbedTextExtent(dc, TEXT(" "), 1, 0, NULL)); //a space at the beginning
				SetMapMode(dc, old_mapmode);

				SelectObject(dc, hfntPrev);
				ReleaseDC(hwnd, dc);
				free(menu_str);
				//
				
				if (item->itemID == ((UINT)HACK_toplevelmenu & 0xFFFFFFFF)) { //Check if we are a "top level" menu
					item->itemWidth = text_width + space_width * 2;
				}
				else {
					item->itemWidth = GetSystemMetrics(SM_CXMENUCHECK) + text_width + space_width; /*Extra space for left bitmap*/; //TODO(fran): we'll probably add a 1 space separation between bmp and txt
				
				}
				item->itemHeight = GetSystemMetrics(SM_CYMENU); //Height of menu

				
				return TRUE;
			} break;
			case MF_SEPARATOR:
			{
				item->itemHeight = 3;
				item->itemWidth = 1;
				return TRUE;
			} break;
			default: return DefWindowProc(hwnd, msg, wparam, lparam);
			}
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		switch (wparam) {//wparam specifies the identifier of the control that needs painting
		case 0: //menu
		{ //TODO(fran): handle WM_MEASUREITEM so we can make the menu bigger
			Assert(item->CtlType == ODT_MENU); //TODO(fran): I could use this instead of wparam
			/*NOTES
			- item->CtlID isnt used for menus
			- item->itemID menu item identifier
			- item->itemAction required drawing action //TODO(fran): use this 
			- item->hwndItem handle to the menu that contains the item, aka HMENU
			*/

			//NOTE: do not use itemData here, go straight for item->hwndItem
			
			//Determine which type of menu we're to draw
			MENUITEMINFO menu_type;
			menu_type.cbSize = sizeof(menu_type);
			menu_type.fMask = MIIM_FTYPE | MIIM_SUBMENU;
			GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_type);
			menu_type.fType ^= MFT_OWNERDRAW; //remove ownerdraw since we know all guys should be

			switch (menu_type.fType) {
				//NOTE: MFT_BITMAP, MFT_SEPARATOR, and MFT_STRING cannot be combined with one another, so we know those are separate types
			case MFT_STRING: //Text menu
			{//NOTE: we render the bitmaps inverted, cause I find it easier to edit on external programs, this may change later, not a hard change
				//TODO(fran): we should just do 1 channel bmps, easier to edit, we can precalculate all each time the color changes and that's it
				COLORREF clrPrevText, clrPrevBkgnd;
				HFONT hfntPrev;
				int x, y;

				// Set the appropriate foreground and background colors. 
				HBRUSH txt_br, bk_br;
				if (item->itemState & ODS_SELECTED || item->itemState & ODS_HOTLIGHT /*needed for "top level" menus*/) //TODO(fran): ODS_CHECKED ODS_FOCUS
				{
					txt_br = unCap_colors.ControlTxt;
					bk_br = unCap_colors.ControlBkMouseOver;
				}
				else
				{
					txt_br = unCap_colors.ControlTxt;
					Assert((int)item->hwndItem);
					MENUINFO mnfo{ sizeof(mnfo) }; mnfo.fMask = MIM_BACKGROUND;
					//GetMenuInfo((HMENU)item->hwndItem, &mnfo); //We will not ask our "parent" hmenu since sometimes it decides it doesnt have the hbrush, we'll go straight to the menu bar
					GetMenuInfo(hMenu, &mnfo);

					bk_br = mnfo.hbrBack; //TODO: unfinished trash
				}
				clrPrevText = SetTextColor(item->hDC, ColorFromBrush(txt_br));//TODO(fran): separate menu brushes
				clrPrevBkgnd = SetBkColor(item->hDC, ColorFromBrush(bk_br));
				
				if (item->itemAction & ODA_DRAWENTIRE || item->itemAction & ODA_SELECT) { //Draw background
					FillRect(item->hDC, &item->rcItem, bk_br);
					//printf("PAINTED BK: %#08X\n", ColorFromBrush(bk_br));
				}
				
				// Select the font and draw the text. 
				hfntPrev = (HFONT)SelectObject(item->hDC, hMenuFont);
				
				WORD x_pad = LOWORD(GetTabbedTextExtent(item->hDC, TEXT(" "), 1, 0, NULL)); //an extra 1 space before drawing text (for not top level menus)

				if (item->hwndItem == (HWND)GetMenu(hwnd)) { //If we are a "top level" menu
					
					//we just want to draw the text, nothing more
					//TODO(fran): clean this huge if-else, very bug prone with things being set/initialized in different parts

					//Get text lenght //TODO(fran): use language_mgr method, we dont need to fight with all this garbage
					MENUITEMINFO menu_nfo;
					menu_nfo.cbSize = sizeof(menu_nfo);
					menu_nfo.fMask = MIIM_STRING;
					menu_nfo.dwTypeData = NULL;
					GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_nfo); //TODO(fran): check about that 3rd param, there are 2 different ways of addressing menus
					//Get actual text
					UINT menu_str_character_cnt = menu_nfo.cch + 1; //includes null terminator
					menu_nfo.cch = menu_str_character_cnt;
					TCHAR* menu_str = (TCHAR*)malloc(menu_str_character_cnt * sizeof(TCHAR));
					menu_nfo.dwTypeData = menu_str;
					GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_nfo);

					//Thanks https://stackoverflow.com/questions/3478180/correct-usage-of-getcliprgn
					//WINDOWS THIS MAKES NO SENSE!!!!!!!!!
					HRGN restoreRegion = CreateRectRgn(0, 0, 0, 0); if (GetClipRgn(item->hDC, restoreRegion) != 1) { DeleteObject(restoreRegion); restoreRegion = NULL; }

					// Set new region, do drawing
					IntersectClipRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom);//This is also stupid, did they have something against RECT ???????
					UINT old_align = GetTextAlign(item->hDC);
					SetTextAlign(item->hDC, TA_CENTER); //TODO(fran): VTA_CENTER for kanji and the like
					// Calculate vertical and horizontal position for the string so that it will be centered
					TEXTMETRIC tm; GetTextMetrics(item->hDC, &tm);
					int yPos = (item->rcItem.bottom + item->rcItem.top - tm.tmHeight) / 2;
					int xPos = item->rcItem.left + (item->rcItem.right - item->rcItem.left) / 2;
					TextOut(item->hDC, xPos, yPos, menu_str, menu_str_character_cnt - 1);
					//wprintf(L"%s\n",menu_str);
					free(menu_str);
					SetTextAlign(item->hDC, old_align);
					
					SelectClipRgn(item->hDC, restoreRegion); if (restoreRegion != NULL) DeleteObject(restoreRegion); //Restore old region
				}
				else {

					//Render img on the left //TODO(fran): there seems to be a bug somewhere, the img isnt shown till the user moves the mouse on top of the item, maybe Im not checking some state
					{
						MENUITEMINFO menu_img;
						menu_img.cbSize = sizeof(menu_img);
						menu_img.fMask = MIIM_CHECKMARKS | MIIM_STATE;
						GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_img);
						HBITMAP hbmp = NULL;
						if (menu_img.fState & MFS_CHECKED) { //If it is checked you can be sure you are going to draw some bmp
							if (!menu_img.hbmpChecked) {
								//TODO(fran): assign default checked bmp
							}
							hbmp = menu_img.hbmpChecked;
						}
						else if (menu_img.fState == MFS_UNCHECKED || menu_img.fState == MFS_HILITE) {//Really Windows? you really needed to set the value to 0? //TODO(fran): maybe it's better to just use else, maybe that's windows' logic for doing this
							if (menu_img.hbmpUnchecked) {
								hbmp = menu_img.hbmpUnchecked;
							}
							//If there's no bitmap we dont draw
						}
						if (hbmp) {
							//HDC bmp_dc = CreateCompatibleDC(item->hDC);
							//HGDIOBJ oldBitmap = SelectObject(bmp_dc, hbmp);
							BITMAP bitmap; GetObject(hbmp, sizeof(bitmap), &bitmap);
							//BitBlt(item->hDC, item->rcItem.left, item->rcItem.top, bitmap.bmWidth, bitmap.bmHeight, bmp_dc, 0, 0, NOTSRCCOPY);

							int img_max_x = GetSystemMetrics(SM_CXMENUCHECK);
							int img_max_y = RECTHEIGHT(item->rcItem);
							int img_sz = min(img_max_x, img_max_y);
							//StretchBlt(item->hDC,
							//	item->rcItem.left + (img_max_x+x_pad - img_sz)/2, item->rcItem.top + (img_max_y - img_sz)/2, img_sz, img_sz,
							//	bmp_dc,
							//	0, 0, bitmap.bmWidth, bitmap.bmHeight,
							//	NOTSRCCOPY
							//);
							RECT mask_rc = rectWH(item->rcItem.left + (img_max_x + x_pad - img_sz) / 2, item->rcItem.top + (img_max_y - img_sz) / 2, img_sz,img_sz);
							DrawMenuImg(item->hDC, mask_rc, hbmp); //TODO(fran): stretch the mask bmp

							//TODO(fran): clipping, centering, transparency & render in the text color (take some value as transparent and the rest use just for intensity)
							//SelectObject(bmp_dc, oldBitmap);
							//DeleteDC(bmp_dc);
						}
					}

					// Determine where to draw, leave space for a check mark and the extra 1 space
					x = item->rcItem.left;
					y = item->rcItem.top;
					x += GetSystemMetrics(SM_CXMENUCHECK) + x_pad;

					//Get text lenght //TODO(fran): use language_mgr method, we dont need to fight with all this garbage
					MENUITEMINFO menu_nfo;
					menu_nfo.cbSize = sizeof(menu_nfo);
					menu_nfo.fMask = MIIM_STRING;
					menu_nfo.dwTypeData = NULL;
					GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_nfo); //TODO(fran): check about that 3rd param, there are 2 different ways of addressing menus
					//Get actual text
					UINT menu_str_character_cnt = menu_nfo.cch + 1; //includes null terminator
					menu_nfo.cch = menu_str_character_cnt;
					TCHAR* menu_str = (TCHAR*)malloc(menu_str_character_cnt * sizeof(TCHAR));
					menu_nfo.dwTypeData = menu_str;
					GetMenuItemInfo((HMENU)item->hwndItem, item->itemID, FALSE, &menu_nfo);

					//Thanks https://stackoverflow.com/questions/3478180/correct-usage-of-getcliprgn
					//WINDOWS THIS MAKES NO SENSE!!!!!!!!!
					HRGN restoreRegion = CreateRectRgn(0, 0, 0, 0);
					if (GetClipRgn(item->hDC, restoreRegion) != 1)
					{
						DeleteObject(restoreRegion);
						restoreRegion = NULL;
					}

					// Set new region, do drawing
					IntersectClipRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom);//This is also stupid, did they have something against RECT ???????
					//TODO(fran): tabs start spacing from the initial x coord, which is completely wrong, we're probably gonna need to do a for loop or just convert the string from tabs to spaces
					TabbedTextOut(item->hDC, x, y, menu_str, menu_str_character_cnt - 1, 0, NULL, x);
					//wprintf(L"%s\n", menu_str);
					free(menu_str);
					//TODO(fran): find a better function, this guy doesnt care  about alignment, only TextOut and ExtTextOut do, but, of course, both cant handle tabs //NOTE: the normal rendering seems to have very long tab spacing so maybe it uses TabbedTextOut with 0 and NULL as the tab params

					SelectClipRgn(item->hDC, restoreRegion);
					if (restoreRegion != NULL)
					{
						DeleteObject(restoreRegion);
					}

					if (menu_type.hSubMenu) { //Draw the submenu arrow

						//int img_sz = RECTHEIGHT(item->rcItem);
						//HBITMAP arrow = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(RIGHT_ARROW), IMAGE_BITMAP, img_sz, img_sz, LR_SHARED); //NOTE: because of LR_SHARED we dont need to free the resource ourselves
						//if (arrow) {
							//HDC bmp_dc = CreateCompatibleDC(item->hDC);
							//HGDIOBJ oldBitmap = SelectObject(bmp_dc, arrow);

							//BITMAP bitmap; GetObject(arrow, sizeof(bitmap), &bitmap);
							//TODO(fran): transparency & render in the text color (take some value as transparent and the rest use just for intensity)

							int img_max_x = GetSystemMetrics(SM_CXMENUCHECK);
							int img_max_y = RECTHEIGHT(item->rcItem);
							int img_sz = min(img_max_x, img_max_y); 

							//NOTE: think I found a semi solution, maintain the new value even or odd depending on the sz of the img, odd -> odd, even -> even
							//if ((bitmap.bmHeight % 2) == 0) { if ((img_sz % 2) != 0)img_sz--; } 
							//else { if ((img_sz % 2) == 0)img_sz--; } 

							//TODO(fran): all of these Blt functions are terrible, I really need to move to my own drawing or directx/...
							//StretchBlt(item->hDC,
							//	item->rcItem.right - img_sz, item->rcItem.top + (img_max_y - img_sz) / 2, img_sz, img_sz,
							//	bmp_dc,
							//	0, 0, bitmap.bmWidth, bitmap.bmHeight,
							//	NOTSRCCOPY
							//);

							RECT arrow_rc = rectWH(item->rcItem.right - img_sz, item->rcItem.top + (img_max_y - img_sz) / 2, img_sz, img_sz);
							DrawMenuArrow(item->hDC, arrow_rc);

							//SelectObject(bmp_dc, oldBitmap);
							//DeleteDC(bmp_dc);

							//Prevent windows from drawing what nobody asked it to draw
							//Many thanks to David Sumich https://www.codeguru.com/cpp/controls/menu/miscellaneous/article.php/c13017/Owner-Drawing-the-Submenu-Arrow.htm 
							ExcludeClipRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom);
						//}
					}
				}
				// Restore the original font and colors. 
				SelectObject(item->hDC, hfntPrev);
				SetTextColor(item->hDC, clrPrevText);
				SetBkColor(item->hDC, clrPrevBkgnd);
			} break;
			case MFT_SEPARATOR:
			{
				const int separator_x_padding = 3;
				FillRect(item->hDC, &item->rcItem, unCap_colors.ControlBk);
				RECT separator_rc;
				separator_rc.top = item->rcItem.top + RECTHEIGHT(item->rcItem)/2;
				separator_rc.bottom = separator_rc.top + 1; //TODO(fran): fancier calc and position
				separator_rc.left = item->rcItem.left + separator_x_padding;
				separator_rc.right = item->rcItem.right - separator_x_padding;
				FillRect(item->hDC, &separator_rc, unCap_colors.ControlTxt);
				//TODO(fran): clipping
			}
			default: return DefWindowProc(hwnd, msg, wparam, lparam);
			}
			
			return TRUE;
		}
		default: return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		} break;
		
	case WM_COMMAND:
	{
		// Msgs from my controls
		switch (LOWORD(wparam))
		{
		case UNCAPNC_MINIMIZE:
		{
			ShowWindow(state->wnd, SW_MINIMIZE);
			return 0;
		} break;
		case UNCAPNC_MAXIMIZE:
		{
			WINDOWPLACEMENT p{ sizeof(p) }; GetWindowPlacement(state->wnd, &p);

			if (p.showCmd == SW_SHOWMAXIMIZED) ShowWindow(state->wnd, SW_RESTORE);
			else ShowWindow(state->wnd, SW_MAXIMIZE); //TODO(fran): maximize covers the whole screen, I dont want that, I want to live the navbar visible. For this to be done automatically by windows we need the WS_MAXIMIZEBOX style, which decides to draw a maximize box when pressed, if we can hide that we are all set
			return 0;
		} break;
		case UNCAPNC_CLOSE:
		{
			DestroyWindow(state->wnd);
			return 0;
		} break;
		default: return SendMessage(state->client, msg, wparam, lparam);
		}
	} break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_STYLECHANGING:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_STYLECHANGED:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_PARENTNOTIFY://Different notifs about your childs
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGING:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SHOWWINDOW:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ACTIVATEAPP:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ACTIVATE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_IME_SETCONTEXT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_IME_NOTIFY:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_GETOBJECT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SETFOCUS:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCPAINT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ERASEBKGND:
	{
		return 1; //So we dont erase the menu bar
		//return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOVE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_GETTEXT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_GETICON:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DWMNCRENDERINGCHANGED:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	default:
	case 49558: //TaskbarButtonCreated, where that comes from or what that does I have no clue
	{
		TCHAR arr[256];
		int res = GetClipboardFormatName(49558, arr, 256); //IMPORTANT: a way to find out the name of 0xC000 through 0xFFFF messages, these are: "String messages for use by applications."
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
	}
	return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	urender::init(); defer{ urender::uninit(); };

	//Initialization of common controls
	INITCOMMONCONTROLSEX icc;

	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_STANDARD_CLASSES;

	BOOL comm_res = InitCommonControlsEx(&icc);
	Assert(comm_res);

#ifdef _DEBUG
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

	CheckInfoFile();

	LANGUAGE_MANAGER::Instance().SetHInstance(hInstance);
	LANGUAGE_MANAGER::Instance().ChangeLanguage(startup_locale);

	unCap_colors.ControlBk = CreateSolidBrush(RGB(40, 41, 35)); //TODO(fran): all this should come from the save file, maybe also font
	unCap_colors.ControlBkPush = CreateSolidBrush(RGB(0, 110, 200));
	unCap_colors.ControlBkMouseOver = CreateSolidBrush(RGB(0, 120, 215));
	unCap_colors.ControlTxt = CreateSolidBrush(RGB(248, 248, 242));
	unCap_colors.ControlTxt_Inactive = CreateSolidBrush(RGB(208, 208, 202));
	unCap_colors.ControlMsg = CreateSolidBrush(RGB(248, 230, 0));
	//Thanks to Windows' broken Static Control text color when disabled
	unCap_colors.InitialFinalCharDisabled = CreateSolidBrush(RGB(128, 128, 128));
	unCap_colors.Scrollbar = CreateSolidBrush(RGB(148, 148, 142));
	unCap_colors.ScrollbarMouseOver = CreateSolidBrush(RGB(188, 188, 182));
	unCap_colors.ScrollbarBk = CreateSolidBrush(RGB(50, 51, 45));
	unCap_colors.Img = CreateSolidBrush(RGB(228, 228, 222));
	unCap_colors.CaptionBk = CreateSolidBrush(RGB(20, 21, 15));
	unCap_colors.CaptionBk_Inactive = CreateSolidBrush(RGB(60, 61, 65));
	defer{ for (HBRUSH& b : unCap_colors.brushes) if (b) { DeleteBrush(b); b = NULL; } };

	//Setting offsets for what will define the "client" area of a tab control
	TabOffset.leftOffset = 3;
	TabOffset.rightOffset = TabOffset.leftOffset;
	TabOffset.bottomOffset = TabOffset.leftOffset;
	TabOffset.topOffset = 24;

	CreateFonts();

	init_wndclass_unCap_uncap_nc(hInstance);

	init_wndclass_unCap_scrollbar(hInstance);

	init_wndclass_unCap_button(hInstance);

	//TODO(fran): I think the problem I get that someone else is drawing my nc area is because I ask for OVERLAPPED_WINDOW when I create the window, so it takes care of doing that, which I dont want, REMOVE IT
	hMain = CreateWindowEx(WS_EX_CONTROLPARENT/*|WS_EX_ACCEPTFILES*/, unCap_wndclass_uncap_nc, appName, WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		rcMainWnd.left, rcMainWnd.top, RECTWIDTH(rcMainWnd), RECTHEIGHT(rcMainWnd), nullptr, nullptr, hInstance, nullptr);

	if (!hMain) return FALSE;

	ShowWindow(hMain, nCmdShow);
	UpdateWindow(hMain);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SRTFIXWIN32));

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		//if (!IsDialogMessage(hwnd, &msg))
		//{
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}

	}

	return (int)msg.wParam;
}