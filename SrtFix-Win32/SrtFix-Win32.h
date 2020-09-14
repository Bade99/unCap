#pragma once

//Pointless warnings
#pragma warning(disable : 4505) //unreferenced local function has been removed
#pragma warning(disable : 4189) //local variable is initialized but not referenced

//Pointless unsafe complaints
#ifdef _DEBUG
#define _CRT_SECURE_NO_WARNINGS
#endif


#define WIN32_LEAN_AND_MEAN 
#define STRICT_TYPED_ITEMIDS
#include "targetver.h"

#include "resource.h"
//#include "text.h"
#include <Windows.h>
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
//#include "SubtitleWindow.h"
#include "LANGUAGE_MANAGER.h"
#include <Shlobj.h>//SHGetKnownFolderPath
#include "utf8.h" //Thanks to http://utfcpp.sourceforge.net/
#include "text_encoding_detect.h" //Thanks to https://github.com/AutoItConsulting/text-encoding-detect
#include <Windowsx.h> //GET_X_LPARAM GET_Y_LPARAM
#include "fmt/format.h" //Thanks to https://github.com/fmtlib/fmt
#include "unCap_helpers.h"
#include "unCap_global.h"
#include "unCap_math.h"
#include "unCap_scrollbar.h"
