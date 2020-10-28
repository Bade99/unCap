#pragma once

// La inclusión de SDKDDKVer.h define la plataforma Windows más alta disponible.

// Si desea compilar la aplicación para una plataforma Windows anterior, incluya WinSDKVer.h y
// establezca la macro _WIN32_WINNT en la plataforma que desea admitir antes de incluir SDKDDKVer.h.

//TODO(fran): decide minimum supported version, by default it is win7 sp1
//https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=vs-2019

#include <SDKDDKVer.h>
