#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winternl.h>

#include <sddl.h>
#include <aclapi.h>
#include <Psapi.h>
#include <processsnapshot.h>
#include <Imagehlp.h>
#include <TlHelp32.h>
#include <delayimp.h>

#include <appmodel.h>
#include <oleacc.h>
#include <taskschd.h>
#include <comutil.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <UIAnimation.h>

#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "version.lib")

#pragma comment(lib, "delayimp.lib")
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Imagehlp.lib")
#pragma comment(lib, "Psapi.lib")

#pragma comment(lib, "Oleacc.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "PathCch.lib")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")