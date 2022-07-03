// TranslucentFlyoutsGUI.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "TranslucentFlyoutsGUI.h"
#include "..\TranslucentFlyouts\tflapi.h"
#define WM_TASKBARICON WM_APP + 1
#ifdef _WIN64
	#pragma comment(lib, "..\\x64\\Release\\TranslucentFlyoutsLib.lib")
	#pragma comment(lib, "..\\Libraries\\x64\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x64\\ntdll.lib")
#else
	#pragma comment(lib, "..\\Release\\TranslucentFlyoutsLib.lib")
	#pragma comment(lib, "..\\Libraries\\x86\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x86\\ntdll.lib")
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
	HMENU hMenu = CreatePopupMenu();
	POINT Pt = {};
	GetCursorPos(&Pt);
	AppendMenu(hMenu, MF_STRING, 3, TEXT("设置(&S)"));
	AppendMenu(hMenu, MF_STRING, 2, TEXT("退出(&X)"));
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
		TCHAR pszErrorString[MAX_PATH];
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
		ShowBalloonTip(g_mainWindow, pszErrorString, TEXT("出现了一个错误"), 3000, NIIF_ERROR | NIIF_NOSOUND);
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
			ComboBox_SelectString(hCombobox1, -1, TEXT("无"));
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox1, -1, TEXT("不支持的选项"));
			break;
		}
		case 2:
		{
			ComboBox_SelectString(hCombobox1, -1, TEXT("Transparent"));
			break;
		}
		case 3:
		{
			ComboBox_SelectString(hCombobox1, -1, TEXT("Aero"));
			break;
		}
		case 4:
		{
			ComboBox_SelectString(hCombobox1, -1, TEXT("Acrylic Blur"));
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
			ComboBox_SelectString(hCombobox2, -1, TEXT("无"));
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox2, -1, TEXT("额外的阴影"));
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
			ComboBox_SelectString(hCombobox3, -1, TEXT("不透明"));
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox3, -1, TEXT("跟随不透明度"));
			break;
		}
		case 2:
		{
			ComboBox_SelectString(hCombobox3, -1, TEXT("自动计算"));
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
	ComboBox_AddString(hCombobox1, TEXT("无"));
	ComboBox_AddString(hCombobox1, TEXT("不支持的选项"));
	ComboBox_AddString(hCombobox1, TEXT("Transparent"));
	ComboBox_AddString(hCombobox1, TEXT("Aero"));
	ComboBox_AddString(hCombobox1, TEXT("Acrylic Blur"));
	//
	ComboBox_AddString(hCombobox2, TEXT("无"));
	ComboBox_AddString(hCombobox2, TEXT("额外的阴影"));
	//
	ComboBox_AddString(hCombobox3, TEXT("跟随不透明度"));
	ComboBox_AddString(hCombobox3, TEXT("不透明"));
	ComboBox_AddString(hCombobox3, TEXT("自动计算"));
	//
	TCHAR pszStartupName[MAX_PATH];
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
			TCHAR pszVersionInfo[MAX_PATH] = {};
			TCHAR pszLibVersion[MAX_PATH];
			HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)hIcon);
			DestroyIcon(hIcon);
			GetVersionString(pszLibVersion, MAX_PATH);
			_stprintf_s(pszVersionInfo, TEXT("TranslucentFlyoutsLib v%s\nTranslucentFlyoutsGUI v1.0.1"), pszLibVersion);
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
			ShowBalloonTip(g_mainWindow, _T("如果要退出，请右键系统托盘图标"), _T("已最小化至系统托盘"), 3000, NIIF_INFO);
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
			if (((LPNMHDR)lParam)->code == TTN_GETDISPINFO)
			{
				LPNMTTDISPINFO pInfo = (LPNMTTDISPINFO)lParam;
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 500);
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(300, 0));
				SendMessage(pInfo->hdr.hwndFrom, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(30000, 0));
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO1))
				{
					pInfo->lpszText = (LPTSTR)TEXT("此参数决定对弹出窗口应用的特效\r\n如使用Acrylic Blur背景为亚克力模糊\r\n当窗口第一次渲染时设置特效");
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO2))
				{
					pInfo->lpszText = (LPTSTR)TEXT("此参数决定弹出窗口是否具有一个额外的边框或者说阴影\r\n当窗口被设置特效时，边框也会被同步添加到窗口\r\n当使用高透明度设置时，这对于增强视觉对比度十分有用\r\n");
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_SLIDER1))
				{
					TCHAR lpText[MAX_PATH] = {};
					_stprintf_s(lpText, TEXT("当前不透明度：%d/255\r\n"), GetCurrentFlyoutOpacity());
					_stprintf_s(lpText, TEXT("%s%s"), lpText, TEXT("此参数决定弹出窗口的不透明度\r\n此值越高，越不透明，窗口背后的内容可见度越低，反之亦然\r\n只有渲染完全不透明的主题位图会受此影响，正常情况下不应也不需要设置为255或0"));
					pInfo->lpszText = (LPTSTR)lpText;
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_COMBO3))
				{
					pInfo->lpszText = (LPTSTR)TEXT("此参数决定弹出菜单鼠标悬停项的主题位图AlphaBlend混合方式，即不透明度选择方案\r\n建议在Windows 10普通主题下选择<不透明>增加对比度\r\nWindows 11默认主题黑暗模式下具有缺陷，在较高透明度下视觉效果有瑕疵，建议使用<跟随透明度>设置\r\n但是如果你什么都不知道，就选择<自动计算>");
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_BUTTON1))
				{
					pInfo->lpszText = (LPTSTR)TEXT("注意此选项会删除默认的配置信息，清除你对此应用的授权\r\n但是不影响用户界面选项的设置，即不会影响自启动\r\n如果你想停用此应用且不残留信息，点击它我会自动帮你清理掉");
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_CHECK4))
				{
					pInfo->lpszText = (LPTSTR)TEXT("该选项会让你开机时运行此程序\n但如果你需要以管理员权限自启动，请在计划任务处添加自启动任务\n而不是在此处设置自启动，该处设置与计划任务保持独立");
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
						if (!_tcsicmp(szBuffer, TEXT("无")))
						{
							dwEffect = 0;
						}
						if (!_tcsicmp(szBuffer, TEXT("不支持的选项")))
						{
							dwEffect = 1;
						}
						if (!_tcsicmp(szBuffer, TEXT("Transparent")))
						{
							dwEffect = 2;
						}
						if (!_tcsicmp(szBuffer, TEXT("Aero")))
						{
							dwEffect = 3;
						}
						if (!_tcsicmp(szBuffer, TEXT("Acrylic Blur")))
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
						if (!_tcsicmp(szBuffer, TEXT("无")))
						{
							dwBorder = 0;
						}
						if (!_tcsicmp(szBuffer, TEXT("额外的阴影")))
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
						if (!_tcsicmp(szBuffer, TEXT("不透明")))
						{
							dwColorizeOption = 0;
						}
						if (!_tcsicmp(szBuffer, TEXT("跟随不透明度")))
						{
							dwColorizeOption = 1;
						}
						if (!_tcsicmp(szBuffer, TEXT("自动计算")))
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
						ShowBalloonTip(g_mainWindow, _T("已删除旧的配置信息\n下一次启动此应用需再次授权"), _T("设置已更新"), 3000, NIIF_INFO);
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
						TCHAR pszModuleFileName[MAX_PATH], pszCommandLine[MAX_PATH];
						GetModuleFileName(NULL, pszModuleFileName, MAX_PATH);
						_stprintf_s(pszCommandLine, TEXT("\"%s\""), pszModuleFileName);
						_stprintf_s(pszCommandLine, TEXT("%s /onboot"), pszCommandLine);
						if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
						{
							if (RegSetValueEx(hKey, TEXT("TFGUI"), 0, REG_SZ, (LPBYTE)pszCommandLine, sizeof(pszCommandLine)) != ERROR_SUCCESS)
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
					DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, DialogProc);
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

	if (!IsHookInstalled())
	{
		g_showSettingsDialog = _tcsstr(lpCmdLine, TEXT("/onboot")) == nullptr;
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
		//
		WNDCLASS wc = {};
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hInstance = hInstance;
		wc.lpszClassName = L"TaskbarIconOwner";
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
		ShowBalloonTip(g_mainWindow, _T("如果你需要访问用户界面，请单击系统托盘图标"), _T("实例已存在"), 3000, NIIF_INFO);
		return -1;
	}
}