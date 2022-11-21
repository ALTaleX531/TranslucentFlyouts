// TranslucentFlyoutsGUI.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "TranslucentFlyoutsGUI.h"
#include "..\TranslucentFlyouts\tflapi.h"
#include <string>
#define WM_TASKBARICON WM_APP + 1
#ifdef _WIN64
	#pragma comment(lib, "..\\x64\\Release\\TranslucentFlyoutsLib.lib")
	//#pragma comment(lib, "..\\Libraries\\x64\\libkcrt.lib")
	//#pragma comment(lib, "..\\Libraries\\x64\\ntdll.lib")
#else
	#pragma comment(lib, "..\\Release\\TranslucentFlyoutsLib.lib")
	//#pragma comment(lib, "..\\Libraries\\x86\\libkcrt.lib")
	//#pragma comment(lib, "..\\Libraries\\x86\\ntdll.lib")
#endif
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma data_seg("shared")
HWND g_mainWindow = nullptr;
HWND g_settingsWindow = nullptr;
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

HINSTANCE g_hInst = nullptr;
bool g_showSettingsDialog = false;
const UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

TCHAR pszAcrylic[MAX_PATH + 1] = TEXT("亚克力模糊");
TCHAR pszAero[MAX_PATH + 1] = TEXT("模糊");
TCHAR pszTransparent[MAX_PATH + 1] = TEXT("透明");
TCHAR pszReserved[MAX_PATH + 1] = TEXT("不支持");
TCHAR pszNone[MAX_PATH + 1] = TEXT("无");
TCHAR pszShadow[MAX_PATH + 1] = TEXT("额外的阴影");
TCHAR pszOpaque[MAX_PATH + 1] = TEXT("不透明");
TCHAR pszFollow[MAX_PATH + 1] = TEXT("跟随不透明度");
TCHAR pszAuto[MAX_PATH + 1] = TEXT("自动计算");

void OnInitString()
{
	if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	{
		LoadString(g_hInst, IDS_AB, pszAcrylic, MAX_PATH);
		LoadString(g_hInst, IDS_AERO, pszAero, MAX_PATH);
		LoadString(g_hInst, IDS_TRANS, pszTransparent, MAX_PATH);
		LoadString(g_hInst, IDS_RESERVED, pszReserved, MAX_PATH);
		LoadString(g_hInst, IDS_NONE, pszNone, MAX_PATH);
		LoadString(g_hInst, IDS_SHD, pszShadow, MAX_PATH);
		LoadString(g_hInst, IDS_OPAC, pszOpaque, MAX_PATH);
		LoadString(g_hInst, IDS_FO, pszFollow, MAX_PATH);
		LoadString(g_hInst, IDS_AC, pszAuto, MAX_PATH);
	}
}


void OnInitDpiScailing()
{
	static const auto pfnSetProcessDpiAwarenessContext = (BOOL(WINAPI*)(int*))GetProcAddress(GetModuleHandle(TEXT("User32")), "SetProcessDpiAwarenessContext");
	if (pfnSetProcessDpiAwarenessContext)
	{
		pfnSetProcessDpiAwarenessContext((int*) - 4);
	}
	else
	{
		SetProcessDPIAware();
	}
}

HWND AssociateTooltip(HWND hwnd, int nDlgItemId)
{
	HWND hwndTool = GetDlgItem(hwnd, nDlgItemId);
	HWND hwndTip = CreateWindowEx(
	                   NULL, TOOLTIPS_CLASS, NULL,
	                   WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_USEVISUALSTYLE,
	                   CW_USEDEFAULT, CW_USEDEFAULT,
	                   CW_USEDEFAULT, CW_USEDEFAULT,
	                   hwnd, NULL,
	                   g_hInst, NULL
	               );

	if (!hwndTool || !hwndTip)
	{
		return (HWND)NULL;
	}
	TOOLINFO toolInfo = {0};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hwnd;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = LPSTR_TEXTCALLBACK;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	return hwndTip;
}

void ShowMenu(HWND hWnd)
{
	TCHAR pszSettings[MAX_PATH + 1] = TEXT("设置(&S)");
	TCHAR pszExit[MAX_PATH + 1] = TEXT("退出(&X)");
	if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	{
		LoadString(g_hInst, IDS_MENUSETTINGS, pszSettings, MAX_PATH);
		LoadString(g_hInst, IDS_MENUEXIT, pszExit, MAX_PATH);
	}
	HMENU hMenu = CreatePopupMenu();
	POINT Pt = {};
	GetCursorPos(&Pt);
	AppendMenu(hMenu, MF_STRING, 3, pszSettings);
	AppendMenu(hMenu, MF_STRING, 2, pszExit);
	SetForegroundWindow(hWnd);
	switch (TrackPopupMenuEx(hMenu, TPM_NONOTIFY | TPM_RETURNCMD, Pt.x, Pt.y, hWnd, nullptr))
	{
		case 2:
		{
			DestroyWindow(hWnd);
			break;
		}
		case 3:
		{
			if (IsWindow(g_settingsWindow))
			{
				FlashWindow(g_settingsWindow, TRUE);
				ShowWindow(g_settingsWindow, SW_SHOWNORMAL);
				SetActiveWindow(g_settingsWindow);
			}
			else
			{
				DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, DialogProc);
			}
			break;
		}
		default:
			break;
	}
	DestroyMenu(hMenu);
}

BOOL ShowBalloonTip(HWND hWnd, LPCTSTR szMsg, LPCTSTR szTitle, UINT uTimeout, DWORD dwInfoFlags)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1, NIF_INFO | NIF_MESSAGE, WM_TASKBARICON};
	data.uTimeout = uTimeout;
	data.dwInfoFlags = dwInfoFlags;
	_tcscpy_s(data.szInfo, szMsg ? szMsg : _T(""));
	_tcscpy_s(data.szInfoTitle, szTitle ? szTitle : _T(""));
	return Shell_NotifyIcon(NIM_MODIFY, &data);
}

void ShowBalloonTip(HWND hWnd, DWORD dwLastError = GetLastError())
{
	if (dwLastError != NO_ERROR)
	{
		TCHAR pszErrorString[MAX_PATH + 1], pszErrorCaptionText[MAX_PATH + 1] = TEXT("出现了一个错误");
		FormatMessage(
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR)&pszErrorString,
		    MAX_PATH,
		    NULL
		);

		MessageBeep(MB_ICONSTOP);
		if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
		{
			LoadString(g_hInst, IDS_ERROR, pszErrorCaptionText, MAX_PATH);
		}
		ShowBalloonTip(g_mainWindow, pszErrorString, pszErrorCaptionText, 3000, NIIF_ERROR | NIIF_NOSOUND);
	}
}

void TaskbarCreateIcon(HWND hWnd)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_TASKBARICON, LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1)), {TEXT("TranslucentFlyouts 用户界面")}};
	Shell_NotifyIcon(NIM_ADD, &data);
}

void TaskbarDestroyIcon(HWND hWnd)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1};
	Shell_NotifyIcon(NIM_DELETE, &data);
}

void OnInitDialogItem(HWND hWnd)
{
	HWND hCombobox1 = GetDlgItem(hWnd, IDC_COMBO1);
	HWND hCombobox2 = GetDlgItem(hWnd, IDC_COMBO2);
	HWND hCombobox3 = GetDlgItem(hWnd, IDC_COMBO3);
	//
	SendDlgItemMessage(hWnd, IDC_SLIDER1, TBM_SETPOS, TRUE, GetCurrentFlyoutOpacity());
	//
	switch (GetCurrentFlyoutEffect())
	{
		case 0:
		{
			ComboBox_SelectString(hCombobox1, -1, pszNone);
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox1, -1, pszReserved);
			break;
		}
		case 2:
		{
			ComboBox_SelectString(hCombobox1, -1, pszTransparent);
			break;
		}
		case 3:
		{
			ComboBox_SelectString(hCombobox1, -1, pszAero);
			break;
		}
		case 4:
		{
			ComboBox_SelectString(hCombobox1, -1, pszAcrylic);
			break;
		}
		default:
			break;
	}
	//
	switch (GetCurrentFlyoutBorder())
	{
		case 0:
		{
			ComboBox_SelectString(hCombobox2, -1, pszNone);
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox2, -1, pszShadow);
			break;
		}
		default:
			break;
	}
	//
	switch (GetCurrentFlyoutColorizeOption())
	{
		case 0:
		{
			ComboBox_SelectString(hCombobox3, -1, pszOpaque);
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox3, -1, pszFollow);
			break;
		}
		case 2:
		{
			ComboBox_SelectString(hCombobox3, -1, pszAuto);
			break;
		}
		default:
			break;
	}
	DWORD dwPolicy = GetCurrentFlyoutPolicy();
	if (dwPolicy & PopupMenu)
	{
		CheckDlgButton(hWnd, IDC_CHECK1, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(hWnd, IDC_CHECK1, BST_UNCHECKED);
	}
	if (dwPolicy & Tooltip)
	{
		CheckDlgButton(hWnd, IDC_CHECK2, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(hWnd, IDC_CHECK2, BST_UNCHECKED);
	}
	if (dwPolicy & ViewControl)
	{
		CheckDlgButton(hWnd, IDC_CHECK3, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(hWnd, IDC_CHECK3, BST_UNCHECKED);
	}
}

void OnInitDialog(HWND hWnd)
{
	g_settingsWindow = hWnd;
	OnInitString();
	//
	HWND hCombobox1 = GetDlgItem(hWnd, IDC_COMBO1);
	HWND hCombobox2 = GetDlgItem(hWnd, IDC_COMBO2);
	HWND hCombobox3 = GetDlgItem(hWnd, IDC_COMBO3);
	//
	AssociateTooltip(hWnd, IDC_COMBO1);
	AssociateTooltip(hWnd, IDC_COMBO2);
	AssociateTooltip(hWnd, IDC_SLIDER1);
	AssociateTooltip(hWnd, IDC_COMBO3);
	AssociateTooltip(hWnd, IDC_BUTTON1);
	AssociateTooltip(hWnd, IDC_CHECK4);
	//
	SendDlgItemMessage(hWnd, IDC_SLIDER1, TBM_SETRANGE, 0, MAKELPARAM(0, 255));
	//
	HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)hIcon);
	DestroyIcon(hIcon);
	//
	ComboBox_AddString(hCombobox1, pszAcrylic);
	ComboBox_AddString(hCombobox1, pszAero);
	ComboBox_AddString(hCombobox1, pszTransparent);
	ComboBox_AddString(hCombobox1, pszNone);
	ComboBox_AddString(hCombobox1, pszReserved);
	//
	ComboBox_AddString(hCombobox2, pszShadow);
	ComboBox_AddString(hCombobox2, pszNone);
	//
	ComboBox_AddString(hCombobox3, pszAuto);
	ComboBox_AddString(hCombobox3, pszOpaque);
	ComboBox_AddString(hCombobox3, pszFollow);
	//
	TCHAR pszStartupName[MAX_PATH + 1];
	DWORD dwSize = sizeof(pszStartupName);
	if (RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), TEXT("TFGUI"), RRF_RT_REG_SZ, nullptr, pszStartupName, &dwSize) == ERROR_SUCCESS)
	{
		CheckDlgButton(hWnd, IDC_CHECK4, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(hWnd, IDC_CHECK4, BST_UNCHECKED);
	}
	//
	OnInitDialogItem(hWnd);
}

BOOL IsUserAgreeLicense()
{
	DWORD dwValue = 0;
	DWORD dwSize = sizeof(dwValue);
	return RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("UserAgreeLicense"), RRF_RT_REG_DWORD, nullptr, &dwValue, &dwSize) == ERROR_SUCCESS and dwValue == 1;
}

VOID UserAgreeLicense()
{
	HKEY hKey = nullptr;
	DWORD dwUserAgreeLicense = 1;
	LRESULT lResult = NO_ERROR;
	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
	if (lResult != NO_ERROR)
	{
		return;
	}
	lResult = RegSetValueEx(hKey, TEXT("UserAgreeLicense"), 0, REG_DWORD, (LPBYTE)&dwUserAgreeLicense, sizeof(DWORD));
	if (lResult != NO_ERROR)
	{
		return;
	}
	RegCloseKey(hKey);
	return;
}

INT_PTR CALLBACK DialogProc3(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)hIcon);
			DestroyIcon(hIcon);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK pNMLink = (PNMLINK)lParam;
					LITEM item = pNMLink->item;

					ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
					break;
				}
			}
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hWnd, FALSE);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					EndDialog(hWnd, TRUE);
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hWnd, FALSE);
					break;
				}
			}
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK DialogProc2(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			TCHAR pszVersionInfo[MAX_PATH + 1] = {};
			TCHAR pszLibVersion[MAX_PATH + 1];
			HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)hIcon);
			DestroyIcon(hIcon);
			GetVersionString(pszLibVersion, MAX_PATH);
			_stprintf_s(pszVersionInfo, TEXT("TranslucentFlyoutsLib v%s\nTranslucentFlyoutsGUI v1.0.3"), pszLibVersion);
			SetWindowText(GetDlgItem(hWnd, IDC_STATIC4), pszVersionInfo);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK pNMLink = (PNMLINK)lParam;
					LITEM item = pNMLink->item;

					ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
					break;
				}
			}
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hWnd, TRUE);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					EndDialog(hWnd, TRUE);
					break;
				}
			}
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			OnInitDialog(hWnd);
			break;
		}
		case WM_CLOSE:
		{
			TCHAR pszNoticeText[MAX_PATH + 1] = _T("如果要退出，请右键系统托盘图标");
			TCHAR pszNotice[MAX_PATH + 1] = _T("已最小化至系统托盘");
			if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
			{
				LoadString(g_hInst, IDS_EXIT, pszNotice, MAX_PATH);
				LoadString(g_hInst, IDS_EXITTEXT, pszNoticeText, MAX_PATH);
			}
			ShowBalloonTip(g_mainWindow, pszNoticeText, pszNotice, 3000, NIIF_INFO);
			EndDialog(hWnd, TRUE);
			break;
		}
		case WM_DESTROY:
		{
			g_settingsWindow = nullptr;
			break;
		}
		case WM_HSCROLL:
		{
			if (
			    LOWORD(wParam) >= TB_LINEUP and
			    LOWORD(wParam) <= TB_ENDTRACK
			)
			{
				DWORD dwOpacity = GetCurrentFlyoutOpacity();
				dwOpacity = (DWORD)SendDlgItemMessage(hWnd, IDC_SLIDER1, TBM_GETPOS, 0, 0);
				if (!SetFlyoutOpacity(dwOpacity))
				{
					ShowBalloonTip(g_mainWindow);
				}
				FlushSettingsCache();
			}
			break;
		}
		case WM_NOTIFY:
		{
			TCHAR pszEffectTip[512] = TEXT("此参数决定对弹出窗口应用的特效\n如使用亚克力模糊作为背景\n我们推荐使用<亚克力模糊>");
			TCHAR pszOpacityTip[512] = TEXT("当前不透明度：");
			TCHAR pszOpacity2Tip[512] = TEXT("此参数决定弹出窗口的不透明度\n此值越高，越不透明(Opaque)，窗口背后的内容可见度越低，反之亦然\n只有渲染完全不透明的主题位图会受此影响，正常情况下不应也不需要设置为255或0");
			TCHAR pszBorderTip[512] = TEXT("此参数决定弹出窗口是否具有一个额外的边框或是阴影\n当窗口被设置特效时，边框也会被同步添加到窗口\n当使用高透明度设置时，这对于增强视觉对比度十分有用\n");
			TCHAR pszColorizeOptionTip[512] = TEXT("此参数决定弹出菜单鼠标悬停项的主题位图AlphaBlend混合方式，即不透明度选择方案\n通常情况下选择<不透明>增加对比度，但Windows 11 22H2之前默认主题黑暗模式下具有缺陷\n其在较高透明度下菜单项会又一层黑边，所以推荐使用<跟随透明度>设置\n但是如果你什么都不知道，就选择<自动计算>");
			TCHAR pszResetTip[512] = TEXT("注意此选项会删除默认的配置信息，清除你对此应用的授权\n但是不影响用户界面选项的设置，即不会影响自启动\n如果你想停用此应用且不残留信息，点击它我会自动帮你清理掉");
			TCHAR pszAutoRunTip[512] = TEXT("该选项会让你开机时运行此程序\n但如果你需要以管理员权限自启动，请在计划任务处添加自启动任务\n而不是在此处设置自启动，该处设置与计划任务保持独立");
			if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
			{
				LoadString(g_hInst, IDS_EFFECT, pszEffectTip, 512);
				LoadString(g_hInst, IDS_OPACITY1, pszOpacityTip, 512);
				LoadString(g_hInst, IDS_OPACITY2, pszOpacity2Tip, 512);
				LoadString(g_hInst, IDS_BORDER, pszBorderTip, 512);
				LoadString(g_hInst, IDS_COPTION, pszColorizeOptionTip, 512);
				LoadString(g_hInst, IDS_RESET, pszResetTip, 512);
				LoadString(g_hInst, IDS_AUTORUN, pszAutoRunTip, 512);
			}
			if (((LPNMHDR)lParam)->code == TTN_GETDISPINFO)
			{
				LPNMTTDISPINFO pInfo = (LPNMTTDISPINFO)lParam;
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 500);
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(300, 0));
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(30000, 0));
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO1))
				{
					pInfo->lpszText = (LPTSTR)pszEffectTip;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO2))
				{
					pInfo->lpszText = (LPTSTR)pszBorderTip;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_SLIDER1))
				{
					TCHAR lpText[1024] = {};
					_stprintf_s(lpText, TEXT("%s%d/255\n"), pszOpacityTip, GetCurrentFlyoutOpacity());
					_stprintf_s(lpText, TEXT("%s%s"), lpText, pszOpacity2Tip);
					pInfo->lpszText = (LPTSTR)lpText;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO3))
				{
					pInfo->lpszText = (LPTSTR)pszColorizeOptionTip;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_BUTTON1))
				{
					pInfo->lpszText = (LPTSTR)pszResetTip;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_CHECK4))
				{
					pInfo->lpszText = (LPTSTR)pszAutoRunTip;
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				{
					EndDialog(hWnd, TRUE);
					DestroyWindow(g_mainWindow);
					break;
				}
				case IDC_BUTTON2:
				{
					InvalidateRect(nullptr, nullptr, true);
					break;
				}
				case IDC_COMBO1:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						DWORD dwEffect = GetCurrentFlyoutEffect();
						TCHAR szBuffer[MAX_PATH + 1];
						ComboBox_GetText(GetDlgItem(hWnd, IDC_COMBO1), szBuffer, MAX_PATH);
						if (!_tcsicmp(szBuffer, pszNone))
						{
							dwEffect = 0;
						}
						if (!_tcsicmp(szBuffer, pszReserved))
						{
							dwEffect = 1;
						}
						if (!_tcsicmp(szBuffer, pszTransparent))
						{
							dwEffect = 2;
						}
						if (!_tcsicmp(szBuffer, pszAero))
						{
							dwEffect = 3;
						}
						if (!_tcsicmp(szBuffer, pszAcrylic))
						{
							dwEffect = 4;
						}
						if (!SetFlyoutEffect(dwEffect))
						{
							ShowBalloonTip(g_mainWindow);
						}
					}
					break;
				}
				case IDC_COMBO2:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						DWORD dwBorder = GetCurrentFlyoutBorder();
						TCHAR szBuffer[MAX_PATH + 1];
						ComboBox_GetText(GetDlgItem(hWnd, IDC_COMBO2), szBuffer, MAX_PATH);
						if (!_tcsicmp(szBuffer, pszNone))
						{
							dwBorder = 0;
						}
						if (!_tcsicmp(szBuffer, pszShadow))
						{
							dwBorder = 1;
						}
						if (!SetFlyoutBorder(dwBorder))
						{
							ShowBalloonTip(g_mainWindow);
						}
					}
					break;
				}
				case IDC_COMBO3:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						DWORD dwColorizeOption = GetCurrentFlyoutColorizeOption();
						TCHAR szBuffer[MAX_PATH + 1];
						ComboBox_GetText(GetDlgItem(hWnd, IDC_COMBO3), szBuffer, MAX_PATH);
						if (!_tcsicmp(szBuffer, pszOpaque))
						{
							dwColorizeOption = 0;
						}
						if (!_tcsicmp(szBuffer, pszFollow))
						{
							TCHAR pszText[MAX_PATH + 1] = _T("注意，自1.0.3开始<跟随透明度>已被弃用。其依旧可以正常工作，但用户没有理由也不需要继续使用该值");
							TCHAR pszCaption[MAX_PATH + 1] = _T("被弃用的值");
							if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
							{
								LoadString(g_hInst, IDS_DEPRECATED, pszCaption, MAX_PATH);
								LoadString(g_hInst, IDS_DEPRECATEDTEXT, pszText, MAX_PATH);
							}
							ShowBalloonTip(g_mainWindow, pszText, pszCaption, 3000, NIIF_INFO);
							dwColorizeOption = 1;
						}
						if (!_tcsicmp(szBuffer, pszAuto))
						{
							dwColorizeOption = 2;
						}
						if (!SetFlyoutColorizeOption(dwColorizeOption))
						{
							ShowBalloonTip(g_mainWindow);
						}
					}
					break;
				}
				case IDC_CHECK1:
				{
					DWORD dwPolicy = GetCurrentFlyoutPolicy();
					dwPolicy = IsDlgButtonChecked(hWnd, IDC_CHECK1) ? dwPolicy | PopupMenu : dwPolicy & ~PopupMenu;
					SetFlyoutPolicy(dwPolicy);
					break;
				}
				case IDC_CHECK2:
				{
					DWORD dwPolicy = GetCurrentFlyoutPolicy();
					dwPolicy = IsDlgButtonChecked(hWnd, IDC_CHECK2) ? dwPolicy | Tooltip : dwPolicy & ~Tooltip;
					SetFlyoutPolicy(dwPolicy);
					break;
				}
				case IDC_CHECK3:
				{
					DWORD dwPolicy = GetCurrentFlyoutPolicy();
					dwPolicy = IsDlgButtonChecked(hWnd, IDC_CHECK3) ? dwPolicy | ViewControl : dwPolicy & ~ViewControl;
					SetFlyoutPolicy(dwPolicy);
					break;
				}
				case IDC_BUTTON1:
				{
					if (!ClearFlyoutConfig())
					{
						ShowBalloonTip(g_mainWindow);
					}
					else
					{
						TCHAR pszUpdateText[MAX_PATH + 1] = _T("已删除旧的配置信息\n下一次启动此应用需再次授权");
						TCHAR pszUpdate[MAX_PATH + 1] = _T("设置已更新");
						if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
						{
							LoadString(g_hInst, IDS_UPDATE, pszUpdate, MAX_PATH);
							LoadString(g_hInst, IDS_UPDATETEXT, pszUpdateText, MAX_PATH);
						}
						ShowBalloonTip(g_mainWindow, pszUpdateText, pszUpdate, 3000, NIIF_INFO);
					}
					OnInitDialogItem(hWnd);
					break;
				}
				case IDC_BUTTON3:
				{
					DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DialogProc2);
					break;
				}
				case IDC_CHECK4:
				{
					HKEY hKey = nullptr;
					if (IsDlgButtonChecked(hWnd, IDC_CHECK4))
					{
						TCHAR pszModuleFileName[MAX_PATH + 1], pszCommandLine[MAX_PATH + 1];
						GetModuleFileName(NULL, pszModuleFileName, MAX_PATH);
						std::wstring path = L"\"";
						path += pszModuleFileName;
						path += L"\" /onboot";
						/*_stprintf_s(pszCommandLine, TEXT("\"%s\""), pszModuleFileName);
						_stprintf_s(pszCommandLine, TEXT("%s /onboot"), pszCommandLine);*/
						if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
						{
							if (RegSetValueExW(hKey, TEXT("TFGUI"), 0, REG_SZ, (LPBYTE)path.c_str(), path.length() * sizeof(wchar_t)) != ERROR_SUCCESS)
							{
								ShowBalloonTip(g_mainWindow);
							}
							RegCloseKey(hKey);
						}
						else
						{
							ShowBalloonTip(g_mainWindow);
						}
					}
					else
					{
						if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
						{
							if (RegDeleteValue(hKey, TEXT("TFGUI")) != ERROR_SUCCESS)
							{
								ShowBalloonTip(g_mainWindow);
							}
							RegCloseKey(hKey);
						}
						else
						{
							ShowBalloonTip(g_mainWindow);
						}
					}
				}
				default:
					break;
			}
			FlushSettingsCache();
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_CREATE:
		{
			TaskbarCreateIcon(hwnd);
			if (!RegisterHook())
			{
				ShowBalloonTip(hwnd);
			}
			if (g_showSettingsDialog)
			{
				PostMessage(hwnd, WM_TASKBARICON, IDI_ICON1, WM_LBUTTONUP);
			}
			break;
		}
		case WM_DESTROY:
		{
			if (!UnregisterHook())
			{
				ShowBalloonTip(hwnd);
			}
			TaskbarDestroyIcon(hwnd);
			PostQuitMessage(0);
			break;
		}
		case WM_TASKBARICON:
		{
			if (wParam == IDI_ICON1)
			{
				if (lParam == WM_LBUTTONUP)
				{
					if (IsWindow(g_settingsWindow))
					{
						FlashWindow(g_settingsWindow, TRUE);
						ShowWindow(g_settingsWindow, SW_SHOWNORMAL);
						SetActiveWindow(g_settingsWindow);
					}
					else
					{
						DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, DialogProc);
					}
				}
				if (lParam == WM_RBUTTONUP)
				{
					ShowMenu(hwnd);
				}
			}
			break;
		}
		default:
			if (Message == WM_TASKBARCREATED)
			{
				TaskbarCreateIcon(hwnd);
				break;
			}
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

int APIENTRY _tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	g_hInst = hInstance;
	OnInitDpiScailing();

	if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	{
		SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}
	if (!IsUserAgreeLicense())
	{
		if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG3), nullptr, DialogProc3, 0))
		{
			return -1;
		}
		else
		{
			UserAgreeLicense();
		}
	}
	if (!IsHookInstalled())
	{
		g_showSettingsDialog = _tcsstr(lpCmdLine, TEXT("/onboot")) == nullptr;

		WNDCLASS wc = {};
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hInstance = hInstance;
		wc.lpszClassName = L"TranslucentFlyoutsHostWindow";
		wc.lpfnWndProc = WndProc;
		if (!RegisterClass(&wc))
		{
			return -1;
		}
		g_mainWindow = CreateWindowEx(
		                   WS_EX_PALETTEWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
		                   wc.lpszClassName,
		                   nullptr,
		                   WS_POPUP,
		                   0, 0, 0, 0,
		                   nullptr, nullptr, wc.hInstance, nullptr
		               );
		if (!g_mainWindow)
		{
			return -1;
		}
		MSG msg = {};
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		SetLastError(ERROR_FILE_EXISTS);
		TCHAR pszCaption[MAX_PATH + 1], pszText[MAX_PATH + 1];
		LoadString(hInstance, IDS_INSTEXIST, pszCaption, MAX_PATH);
		LoadString(hInstance, IDS_INSTEXISTTEXT, pszText, MAX_PATH);
		ShowBalloonTip(g_mainWindow, pszText, pszCaption, 3000, NIIF_INFO);
		return -1;
	}
	return 0;
}