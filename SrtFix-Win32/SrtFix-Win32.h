#pragma once

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


//Objective: we will have the controls that apply effects over the text and separate from that we will have the text controls themselves which will store
//			any data neccesary for the "effects" controls to work

//	effects controls

//	container
//		texts -> info for effects controls (struct)