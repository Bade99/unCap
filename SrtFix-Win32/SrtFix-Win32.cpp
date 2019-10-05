#include "SrtFix-Win32.h"

// Linker->Input->Additional Dependencies (right way to link the .lib)
#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"propsys.lib") //open save file handler
#pragma comment(lib,"shlwapi.lib") //open save file handler
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"") 

using namespace std;
namespace fs = std::experimental::filesystem;

//PROBLEMS:
// - 2001 a space odyssey srt en español tiene encoding ansi y cuando llega a í se caga, por qué????

#ifdef _DEBUG
	#define RELEASE 0 //As I didnt find how to know when the compiler is on Release or Debug I created my own manual one
#else
	#define RELEASE 1
#endif

//@@Revisar resource.h para ver defines que ya esten ocupados!!!
#define OPEN_FILE 11
#define COMBO_BOX 12
#define SAVE_AS_FILE 13
#define REMOVE 14
#define SAVE_FILE 15
#define BACKUP_FILE 16
#define SUBS_WND 17

#define ASCII 1 //NOTE: should really be ANSI //IMPORTANT(fran): decision -> I'd say no ASCII/ANSI support , o podría agregar un boton ANSI
//ASCII is easily encoded in utf8 without problems
#define UTF8 2
#define UTF16 3

#define NO_SEPARATION 0
#define ENTER 1
#define CARRIAGE_RETURN 2
#define CR_ENTER 3

#define MAX_PATH_LENGTH (MAX_PATH*2)
#define MAX_TEXT_LENGTH 288000

#define MAX_LOADSTRING 100

// Variables globales:
HINSTANCE hInst;                                // Instancia actual
WCHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal

HMENU hMenu;//main menu bar(only used to append the rest)
HMENU hFileMenu;
HMENU hFileMenuLang;

HWND hWnd; //main window

HWND hFile;//window for file directory
HWND hOptions;
HWND hInitialText;
HWND hInitialChar;
HWND hFinalText;
HWND hFinalChar;
HWND hRemove;
HWND hSubs;//window for file contents
HWND hRemoveCommentWith;
//HWND hRemoveProgress;
HWND hReadFile;

HFONT hGeneralFont;//Main font manager

HMENU hPopupMenu;

wstring infoPath = L"info_unCap.txt"; //name of hidden info file

bool Backup_Is_Checked = false; //info file check, do backup or not

int global_locale = ENGLISH;//program current locale,@should be enum & defined in text.h

#define POSX 0
#define POSY 1
#define SIZEX 2
#define SIZEY 3
#define DIR 4
#define BACKUP 5
#define LOCALE 6
const wchar_t *info_parameters[] = {
	L"[posx]=",
	L"[posy]=",
	L"[sizex]=",
	L"[sizey]=",
	L"[dir]=",
	L"[backup]=",
	L"[locale]="
};
#define NRO_INFO_PARAMETERS size(info_parameters)

wstring accepted_file=L"";
wstring accepted_file_dir = L""; //path of last valid file dir
wstring accepted_file_name_with_ext = L"";
wstring accepted_file_ext = L"";

bool isAcceptedFile = false;

struct mainWindow { //main window size and position
	int posx = 75;
	int posy = 75;
	int sizex = 650;
	int sizey = 800;
}wnd;

RECT rect;

int y_place = 10;

bool initial_paint = true; //way to handle the initial painting: fonts, ...

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
{ L"Srt (*.srt)",       L"*.srt" },
{ L"Advanced SSA (*.ass)",       L"*.ass" },
{ L"Sub Station Alpha (*.ssa)",       L"*.ssa" },
{ L"Txt (*.txt)",       L"*.txt" },
{ L"All Documents (*.*)",         L"*.*" }
};

// Declaraciones de funciones:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK	EditCatchDrop(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK  About(HWND, UINT, WPARAM, LPARAM);
bool				FileOrDirExists(const wstring&);
void				AddMenus(HWND);
void				AddControls(HWND);
void				ChooseFile(wstring);
void				CommentRemoval(WCHAR, WCHAR);
//int					GetLineSeparation(wstring);
//void				LineSeparation();
//void				LineSeparation(wstring&);
void				EnableOtherChar(bool);
void				DoBackup();
void				DoSave(wstring);
HRESULT				DoSaveAs();
//HWND				CreateToolTip(int, HWND, LPWSTR);
void				CatchDrop(WPARAM);
wstring				GetExecFolder();
void				CreateInfoFile();
void				CheckInfoFile();
void				CreateFonts();
void				AcceptedFile(wstring);
bool				IsAcceptedFile();
void				CustomCommentRemoval();
unsigned char		GetTextEncoding(wstring);
void				DoInitialPainting();
wstring				ReadText(wstring);
//int					LineCount(wifstream&);
void				SetOptionsComboBox(HWND&, bool);
void				SetLocaleW(int);
void				ResizeWindows();
void				DoPainting();


typedef struct GetLineParam {
	wstring *text;
} GLP, *PGLP;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow)
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

	lstrcpyW(szTitle, L"unCap");
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SRTFIXWIN32, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Realizar la inicialización de la aplicación:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

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

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SRTFIXWIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE +1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SRTFIXWIN32);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Almacenar identificador de instancia en una variable global

   CheckInfoFile();

   hWnd = CreateWindowEx(WS_EX_CONTROLPARENT/*|WS_EX_ACCEPTFILES*/,szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   wnd.posx, wnd.posy, wnd.sizex, wnd.sizey, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) return FALSE;

   CreateFonts();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}

void CreateFonts()
{
	LOGFONTW lf;
	HFONT   hFont;

	memset(&lf, 0, sizeof(lf));
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfHeight = -15;
	//@ver: lf.pszFaceName y EnumFontFamiliesW
	hFont = CreateFontIndirectW(&lf);

	if (hGeneralFont != NULL)
	{
		DeleteObject(hGeneralFont);
		hGeneralFont = NULL;
	}

	hGeneralFont = hFont;
	if (hGeneralFont == NULL)
	{
		MessageBox(hWnd, (LPCWSTR)font_failed_T[global_locale], error_T[global_locale], MB_OK);
	}
}

bool IsAcceptedFile() {
	return isAcceptedFile;
}

//si hay /n antes del corchete que lo borre tmb (y /r tmb)

void CommentRemoval(WCHAR start,WCHAR end) {//@@tenemos la opcion de buscar mas de un solo caracter
	int text_length = GetWindowTextLengthW(hSubs) + 1;
	wstring text(text_length, L'\0');
	GetWindowTextW(hSubs, &text[0], text_length);
	size_t start_pos=0, end_pos=0;
	bool found=false;//check if it removed at least 1 item

	//SendMessage(hRemoveProgress, PBM_SETRANGE, 0, text_length);
	//ShowWindow(hRemoveProgress, SW_SHOW);

	while (true) {
		start_pos = text.find(start, start_pos);
		end_pos = text.find(end, start_pos + 1);//+1, dont add it here 'cause breaks the if when end_pos=-1(string::npos)
		if (start_pos == string::npos || end_pos == string::npos) { 
			//SendMessage(hRemoveProgress, PBM_SETPOS, text_length, 0);
			break; 
		}
		text.erase(text.begin() + start_pos, text.begin() + end_pos+1); //@@need to start handling exceptions
		found = true;//@@can we somehow do this only once?
		//SendMessage(hRemoveProgress, PBM_SETPOS, start_pos, 0);
	}
	if (found) SetWindowTextW(hSubs, text.c_str());
	else MessageBoxW(hWnd, nothing_to_remove_T[global_locale],L"", MB_ICONEXCLAMATION);
	//ShowWindow(hRemoveProgress, SW_HIDE);
}

void CustomCommentRemoval() {
	WCHAR temp[2]; //@Revisar que entra en temp, xq debe meter basura o algo
	WCHAR start;
	WCHAR end;
	GetWindowTextW(hInitialChar, temp, 2); //tambien la funcion devuelve 0 si no agarra nada, podemos agregar ese check
	start = temp[0];//vale '\0' si no tiene nada
	GetWindowTextW(hFinalChar, temp, 2);
	end = temp[0];

	if (start != L'\0' && end != L'\0') CommentRemoval(start, end);
	else MessageBoxW(hWnd, add_chars_T[global_locale], L"", MB_ICONEXCLAMATION);
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

void DoBackup() { //@give option to change backup folder
	//size_t found = accepted_file.find_last_of(L"\\");
	//wstring tempdir = accepted_file.substr(0, found+1);	//@@simplify to string.add
	//wstring tempname = accepted_file.substr(found + 1);	//
	wstring new_file=accepted_file_dir+L"SDH_"+ accepted_file_name_with_ext;//
	if (!CopyFileW(accepted_file.c_str(), new_file.c_str(), FALSE)) MessageBoxW(hWnd, L"The file name is probably too large", L"Fix the program!", MB_ICONERROR);; //@@the name is limited to MAX_PATH characters (use CopyFileW?)
}

void DoSave(wstring save_file) { //got to pass the encoding to the function too, and do proper conversion
	int text_length = GetWindowTextLengthW(hSubs) + 1;

	//https://docs.microsoft.com/es-es/windows/desktop/api/commdlg/ns-commdlg-tagofna
	//http://www.winprog.org/tutorial/app_two.html

	//SEARCH(fran):	WideCharToMultiByte  :: Maps a UTF-16 (wide character) string to a new character string. 
	//				The new character string is not necessarily from a multibyte character set.

	//AnimateWindow
	//@@How to save with choosen encoding, (problem in gone with the wind, probably cause has only one char with different encoding)
	//@@more weird things: after saving gone with the wind, if opened up again, everything has one extra separation
	//@@agregar ProgressBar hReadFile
	wofstream new_file(save_file, ios::binary);
	if (new_file.is_open()) {
		new_file.imbue(locale(new_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, generate_header>));
		wstring text(text_length, L'\0');
		GetWindowTextW(hSubs, &text[0], text_length);
		new_file << text;
		new_file.close();
		MessageBoxW(hWnd, done_T[global_locale], L"", MB_ICONINFORMATION);
		SetWindowTextW(hFile, save_file.c_str());
	}
	else MessageBoxW(hWnd, save_failed_T[global_locale], error_T[global_locale], MB_ICONEXCLAMATION);
} 
//@@Ver el formato con que guardo las cosas (check that BOM is created)

HRESULT DoSaveAs() //https://msdn.microsoft.com/en-us/library/windows/desktop/bb776913(v=vs.85).aspx
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

			if (accepted_file_dir != L"") {
				IShellItem *defFolder;//@@shouldn't this be IShellFolder?
				//does accepted_file_dir need to be converted to UTF-16 ?
				hr = SHCreateItemFromParsingName(accepted_file_dir.c_str(), NULL, IID_IShellItem, (void**)&defFolder);
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
				DoSave(filename_holder);
				
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
	hMenu = CreateMenu();
	hFileMenu = CreateMenu();
	hFileMenuLang = CreateMenu();
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, file_T[global_locale]);
	AppendMenuW(hFileMenu, MF_STRING, OPEN_FILE, open_T[global_locale]);
	AppendMenuW(hFileMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(hFileMenu, MF_STRING, SAVE_FILE, save_T[global_locale]);
	AppendMenuW(hFileMenu, MF_STRING, SAVE_AS_FILE, save_as_T[global_locale]);
	AppendMenuW(hFileMenu, MF_STRING, BACKUP_FILE, backup_T[global_locale]);
	AppendMenuW(hFileMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(hFileMenu, MF_POPUP, (UINT_PTR)hFileMenuLang, language_T[global_locale]);
	AppendMenuW(hFileMenuLang, MF_STRING, ENGLISH, L"English");
	AppendMenuW(hFileMenuLang, MF_STRING, SPANISH, L"Español");

	HBITMAP bTick = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(TICK));
	HBITMAP bCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(CROSS));
	HBITMAP bEarth = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(EARTH));

	//@@ver como hacer bitmaps transparentes, y el resize

	SetMenuItemBitmaps(hFileMenu, BACKUP_FILE, MF_BYCOMMAND,bCross,bTick);//1ºunchecked,2ºchecked
	CheckMenuItem(hFileMenu, BACKUP_FILE, MF_BYCOMMAND | MF_CHECKED);
	SetMenuItemBitmaps(hFileMenu, (UINT_PTR)hFileMenuLang, MF_BYCOMMAND, bEarth, bEarth);
	//https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getmenustate
	//https://docs.microsoft.com/es-es/windows/desktop/menurc/using-menus#using-custom-check-mark-bitmaps
	//https://www.daniweb.com/programming/software-development/threads/490612/win32-can-i-change-the-hbitmap-structure-for-be-transparent

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
//@@Capaz puedo usar esto, mandarle un mensaje y que modifique 
// una progress bar al mismo tiempo que el otro thread

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

void AddControls(HWND hWnd) {
	hFile = CreateWindowW(L"Static", L"", WS_VISIBLE | WS_CHILD | WS_BORDER//| SS_WHITERECT
			| ES_AUTOHSCROLL
			, 10, y_place, 664, 20, hWnd, NULL, NULL, NULL);
	//ChangeWindowMessageFilterEx(hFile, WM_DROPFILES, MSGFLT_ADD,NULL); y mas que habia
	
	hReadFile = CreateWindowExW(0, PROGRESS_CLASS, (LPWSTR)NULL, WS_CHILD
		, 10, y_place, 664, 20, hWnd, (HMENU)NULL, NULL, NULL);

	hRemoveCommentWith = CreateWindowW(L"Static", remove_comment_with_T[global_locale], WS_VISIBLE | WS_CHILD
	, 25, y_place+34, 155, 20, hWnd, NULL, NULL, NULL);


	hOptions = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD| CBS_DROPDOWNLIST|WS_TABSTOP
		, 195, y_place + 31/*30*/, 130, 90/*20*/, hWnd, (HMENU)COMBO_BOX, NULL, NULL);
	SetOptionsComboBox(hOptions, TRUE);
	//WCHAR explain_combobox[] = L"Also separates the lines";
	//CreateToolTip(COMBO_BOX, hWnd, explain_combobox);

	hInitialText = CreateWindowW(L"Static", initial_char_T[global_locale], WS_VISIBLE | WS_CHILD |WS_DISABLED
		, 78, y_place + 65, 105, 20, hWnd, NULL, NULL, NULL);

	hInitialChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER |ES_CENTER | WS_TABSTOP | WS_DISABLED
		, 195, y_place + 64, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hInitialChar, EM_LIMITTEXT, 1, 0);

	hFinalText = CreateWindowW(L"Static", final_char_T[global_locale], WS_VISIBLE | WS_CHILD | WS_DISABLED
		, 78, y_place + 95, 105, 20, hWnd, NULL, NULL, NULL);

	hFinalChar = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER| WS_TABSTOP| WS_DISABLED
		, 195, y_place + 94, 20, 21, hWnd, NULL, NULL, NULL);
	SendMessageW(hFinalChar, EM_LIMITTEXT, 1, 0);

	hRemove = CreateWindowW(L"Button", remove_T[global_locale], WS_VISIBLE | WS_CHILD| WS_TABSTOP
		, 256, y_place + 74, 70, 30, hWnd, (HMENU)REMOVE, NULL, NULL);

	//hRemoveProgress = CreateWindowExW(0,PROGRESS_CLASS,(LPWSTR)NULL, WS_CHILD
	//	, 256, y_place + 74, 70, 30,hWnd, (HMENU)NULL,NULL,NULL);

	//@@@Multiple tabs support?
	hSubs = CreateWindowExW(WS_EX_ACCEPTFILES|WS_EX_TOPMOST, L"Edit",NULL, WS_VISIBLE | WS_CHILD | WS_BORDER// | WS_DISABLED
		| ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL	//@this last two should appear only when needed
		, 10, y_place + 134, 664 - 50, 588, hWnd, (HMENU)SUBS_WND,NULL,NULL);

	SetWindowSubclass(hSubs, EditCatchDrop, 0, 0); //adds extra handler for catching dropped files

	SendMessageW(hSubs, EM_SETLIMITTEXT, (WPARAM)MAX_TEXT_LENGTH, NULL); //cantidad de caracteres que el usuario puede escribir(asi le dejo editar)
}

void EnableOtherChar(bool op) {
	EnableWindow(hInitialText, op);
	EnableWindow(hInitialChar, op);
	EnableWindow(hFinalText, op);
	EnableWindow(hFinalChar, op);	
}

void ChooseFile(wstring ext) {
	WCHAR name[MAX_PATH_LENGTH]; //@I dont think this is big enough
	name[0] = L'\0';
	OPENFILENAMEW new_file;
	ZeroMemory(&new_file, sizeof(OPENFILENAMEW));

	new_file.lStructSize = sizeof(OPENFILENAMEW);
	new_file.hwndOwner = NULL;
	new_file.lpstrFile = name; //used for the default file selected
	new_file.nMaxFile = MAX_PATH_LENGTH;
	new_file.lpstrFilter = file_filter_T[global_locale];
	new_file.lpstrDefExt = ext.c_str();
	if (accepted_file_dir != L"\0") new_file.lpstrInitialDir = accepted_file_dir.c_str();
	else new_file.lpstrInitialDir = NULL; //@este if genera bug si initialDir tiene data y abris el buscador pero no agarras nada

	if (GetOpenFileNameW(&new_file) != 0) { //si el usuario elige un archivo
		if (FileOrDirExists(new_file.lpstrFile)) AcceptedFile(new_file.lpstrFile);
		else  MessageBoxW(hWnd, file_not_existent_T[global_locale], error_T[global_locale], MB_ICONERROR);
	}
}

void CatchDrop(WPARAM wParam) {
	HDROP hDrop = (HDROP)wParam;
	WCHAR lpszFile[MAX_PATH_LENGTH] = { 0 };
	UINT uFile = 0;

	uFile = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, NULL);
	if (uFile != 1) {//limit drop to 1 file at a time
		MessageBoxW(hWnd, multi_file_drop_T[global_locale], L"", MB_ICONERROR);
		DragFinish(hDrop);
		return;
	}
	lpszFile[0] = '\0';
	if (DragQueryFileW(hDrop, 0, lpszFile, MAX_PATH_LENGTH)) {
		
		if (fs::is_directory(fs::path(lpszFile))) {//cant recieve entire folders
			MessageBoxW(hWnd, folder_drop_T[global_locale], L"", MB_ICONERROR);
			DragFinish(hDrop);
			return;
		}

		AcceptedFile(lpszFile);
	}
	else MessageBoxW(hWnd, file_drop_failed_T[global_locale], error_T[global_locale], MB_ICONERROR);//is retrieving the best word?
	DragFinish(hDrop);
}

unsigned char GetTextEncoding(wstring filename) { //analize for ascii,utf8,utf16
	
	//wstring confusing_utf8 = L"aϒáϢϴ";
	//wofstream new_file("C:\\Users\\Brenda-Vero-Frank\\Desktop\\hola.txt", ios::binary);
	//if (new_file.is_open()) {
	//	new_file.imbue(locale(new_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, generate_header>));
	//	new_file << confusing_utf8;
	//	new_file.close();
	//}

	char header[4]; //no usar wchar asi leo de a un byte, lo mismo con ifstream en vez de wifstream
	ifstream file(filename, ios::in | ios::binary); //@@Can this read unicode filenames??
	if (!file) {
		return UTF8;//parcheo rapido de momento
	}
	else {
		file.read(header, 3);
		file.close();
		
		if		(header[0] == (char)0XEF && header[1] == (char)0XBB && header[2] == (char)0XBF)	return UTF8;
		else if (header[0] == (char)0XEF && header[1] == (char)0XBB)							return UTF8;
		else if (header[0] == (char)0xFF && header[1] == (char)0xFE)							return UTF16;//little-endian
		else if (header[0] == (char)0xFE && header[1] == (char)0xFF)							return UTF16;//big-ending
		else {//do manual checking
			wifstream file(filename, ios::in | ios::binary);//@@no se si wifstream o ifstream (afecta a stringstream)
			wstringstream buffer;
			buffer << file.rdbuf(); 
			file.close();
			buffer.seekg(0, ios::end);
			int length = buffer.tellg();//long enough length
			buffer.clear();
			buffer.seekg(0, ios::beg);
			//https://unicodebook.readthedocs.io/guess_encoding.html
			//NOTE: creo q esto testea UTF16 asi que probablemente haya errores, y toma ascii como utf
			int test;
			test = IS_TEXT_UNICODE_ILLEGAL_CHARS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return ASCII;
			test = IS_TEXT_UNICODE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
			test = IS_TEXT_UNICODE_REVERSE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
			test = IS_TEXT_UNICODE_ASCII16;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return UTF8;
			return ASCII; 
		}
	}
}

DWORD WINAPI ReadTextThread(LPVOID lpParam) {//receives filename and wstring to save to
	
	PGLP parameters = (PGLP)lpParam;
	
	unsigned char encoding = GetTextEncoding(accepted_file);

	wifstream file(accepted_file, ios::binary);

	if (file.is_open()) {

		//set encoding for getline
		if ( encoding == UTF8)
			file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
		else if (encoding == UTF16)
			file.imbue(locale(file.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));
		//if encoding is ASCII we do nothing

		wstringstream buffer;
		buffer << file.rdbuf();
		//wstring for_testing = buffer.str();
		(*((PGLP)lpParam)->text) = buffer.str();

		file.close();
	}
	return 0;
}

void AcceptedFile(wstring filename) {

	isAcceptedFile = true;

	//save file dir+name+ext
	accepted_file = filename;

	//save file dir
	accepted_file_dir = filename.substr(0, filename.find_last_of(L"\\")+1);

	//save file name+ext
	accepted_file_name_with_ext = filename.substr(filename.find_last_of(L"\\")+1);

	//save file ext
	accepted_file_ext = filename.substr(filename.find_last_of(L".")+1);

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

void CreateInfoFile(){
	wstring info_save_file = GetExecFolder() + infoPath;

	GetWindowRect(hWnd, &rect);
	wnd.sizex = rect.right - rect.left;
	wnd.sizey = rect.bottom - rect.top;

	_wremove(info_save_file.c_str()); //@I dont know if it's necessary to check if the file exists
	wofstream create_info_file(info_save_file);
	if (create_info_file.is_open()) {
		create_info_file.imbue(locale(create_info_file.getloc(), new codecvt_utf8<wchar_t, 0x10ffff, generate_header>));
		create_info_file << info_parameters[POSX] << rect.left << L"\n";			//posx
		create_info_file << info_parameters[POSY] << rect.top << L"\n";			//posy
		create_info_file << info_parameters[SIZEX] << wnd.sizex << L"\n";			//sizex
		create_info_file << info_parameters[SIZEY] << wnd.sizey << L"\n";			//sizey
		create_info_file << info_parameters[DIR] << accepted_file_dir << L"\n";	//directory of the last valid file directory
		create_info_file << info_parameters[BACKUP] << (bool)(GetMenuState(hFileMenu, BACKUP_FILE, MF_BYCOMMAND) & MF_CHECKED) << L"\n"; //SDH
		create_info_file << info_parameters[LOCALE] << global_locale << L"\n"; //global locale
		//@los otros caracteres??(quiza deberiamos guardarlos, puede que moleste)
		create_info_file.close();
		SetFileAttributes(info_save_file.c_str(), FILE_ATTRIBUTE_HIDDEN);
	}
	//@@it still happens that some times the positions and sizes are saved wrong, investigate!!
}

void removeInitialWhiteSpace(wstring &text){
	while(iswspace(text[0]))text.erase(0, 1);
}

void AddInfoParameters(wstring parameters[]) {//@the debugger only shows the first parameter,why??
	//puts the new data into the global variables

	if (parameters[POSX] != L"") {
		int posx = stoi(parameters[POSX]);
		if (0 <= posx && posx <= GetSystemMetrics(SM_CXVIRTUALSCREEN))
			wnd.posx = posx;
	}

	if (parameters[POSY] != L"") {
		int posy = stoi(parameters[POSY]);
		if (0 <= posy && posy <= GetSystemMetrics(SM_CYVIRTUALSCREEN))
			wnd.posy = posy;
	}

	if (parameters[SIZEX] != L"") {
		int sizex = stoi(parameters[SIZEX]);
		if (0 <= sizex)
			wnd.sizex = sizex;
	}

	if (parameters[SIZEY] != L"") {
		int sizey = stoi(parameters[SIZEY]);
		if (0 <= sizey)
			wnd.sizey = sizey;
	}

	if (parameters[DIR] != L"") {
		accepted_file_dir = parameters[DIR];
	}

	if (parameters[BACKUP] != L"") {
		Backup_Is_Checked = stoi(parameters[BACKUP]);
	}

	if (parameters[LOCALE] != L"") {
		global_locale = stoi(parameters[LOCALE]);

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
	wstring dir = GetExecFolder() + infoPath;
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

wstring GetExecFolder() {
	WCHAR ownPth[MAX_PATH_LENGTH];
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL) GetModuleFileName(hModule, ownPth, (sizeof(ownPth)));
	else { 
		MessageBoxW(hWnd, exe_path_failed_T[global_locale], error_T[global_locale], MB_ICONERROR); 
		return L"C:\\Temp\\";
	}
	wstring dir = ownPth;
	dir.erase(dir.rfind(L"\\") + 1, string::npos);
	return dir;
}

bool FileOrDirExists(const wstring& file) {
	return fs::exists(file);//true if exists
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
	SetWindowTextW(hFinalText,final_char_T[global_locale]);
	SetWindowTextW(hRemove,remove_T[global_locale]);
	
	UpdateWindow(hWnd);

	//@@Alguna forma de alinear los controles por la derecha?? (asi todo queda alineado)
	
	//notification messages get updated on their own
}

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

//@@DPI awareness
//@@Dark mode??
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

/* small comments when mouseover
HWND CreateToolTip(int toolID, HWND hDlg, LPWSTR pszText){
if (!toolID || !hDlg || !pszText) return FALSE;
// Get the window of the tool.
HWND hwndTool = GetDlgItem(hDlg, toolID);

// Create the tooltip. g_hInst is the global instance handle.
HWND hwndTip = CreateWindowExW(NULL, TOOLTIPS_CLASS, NULL,
WS_POPUP | TTS_ALWAYSTIP //| TTS_BALLOON
,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
hDlg, NULL,hInst, NULL);

if (!hwndTool || !hwndTip) return (HWND)NULL;

// Associate the tooltip with the tool.
TOOLINFO toolInfo = { 0 };
toolInfo.cbSize = TTTOOLINFOA_V1_SIZE;
toolInfo.hwnd = hDlg;
toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
toolInfo.uId = (UINT_PTR)hwndTool;
toolInfo.lpszText = pszText;

SendMessageW(hwndTip, TTM_ACTIVATE, TRUE, 0);
if (!SendMessageW(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo)) { //Will add the Tool Tip on Control
int err = GetLastError();
MessageBox(NULL, L"Couldn't create the ToolTip control.WTF", L"Error", MB_OK);
}
return hwndTip;
}
*/

//hCheckBox = CreateWindowW(L"Button", /*L"Backup file"*/L"Keep SDH file", WS_VISIBLE| WS_CHILD| BS_AUTOCHECKBOX
//	, 552 - 50, y_place + 27, 125, 30, hWnd, NULL/*(HMENU)BACKUP*/, NULL, NULL);
//SendMessageW(hCheckBox, BM_SETCHECK, BST_CHECKED, 0); //@@Convertirlo en un menu
//WCHAR explain_backup[] = L"Only for the Remove function";
//CreateToolTip(BACKUP, hWnd, explain_backup);

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

//@@creo q no hace falta declarar estos externs
//menu text
//extern const wchar_t *file_T[];
//extern const wchar_t *open_T[];
//extern const wchar_t *save_T[];
//extern const wchar_t *save_as_T[];
//extern const wchar_t *backup_T[];
//extern const wchar_t *language_T[];

//controls text
//extern const wchar_t *remove_comment_with_T[];
//extern const wchar_t *brackets_T[];
//extern const wchar_t *parenthesis_T[];
//extern const wchar_t *braces_T[];
//extern const wchar_t *other_T[];
//extern const wchar_t *initial_char_T[];
//extern const wchar_t *final_char_T[];
//extern const wchar_t *remove_button_T[];

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