#pragma once

#define WIN32_LEAN_AND_MEAN 
#define STRICT_TYPED_ITEMIDS
#include "targetver.h"

#include "resource.h"
#include "text.h"
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