// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <VersionHelpers.h>
// Windows Runtime
#include "WindowsRuntime.h"
// Theme
#include <Uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <dwmapi.h>
// C and C++ Runtime
#include <intrin.h>
#include <stdio.h>
#include <tchar.h>
#include <list>
#include <vector>
#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <map>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

#endif //PCH_H
