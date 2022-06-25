// TranslucentFlyoutsGUI.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "Resource.h"
#include "TranslucentFlyoutsGUI.h"
#include "..\TranslucentFlyouts\tflapi.h"
#define WM_SHELLICON WM_APP
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

HINSTANCE g_hInst;
const UINT& WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

void OnInitDpiScailing()
{
	static const auto& pfnSetProcessDpiAwarenessContext = (BOOL(WINAPI*)(int*))GetProcAddress(GetModuleHandle(TEXT("User32")), "SetProcessDpiAwarenessContext");
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
	AppendMenu(hMenu, MF_STRING, 1, TEXT("设置(&S)"));
	AppendMenu(hMenu, MF_STRING, 0, TEXT("退出(&X)"));
	SetForegroundWindow(hWnd);
	TrackPopupMenuEx(hMenu, TPM_NONOTIFY, Pt.x, Pt.y, hWnd, nullptr);
	PostMessage(hWnd, WM_NULL, 0, 0);
	DestroyMenu(hMenu);
}

BOOL ShowBalloonTip(HWND hWnd, LPCTSTR szMsg, LPCTSTR szTitle, UINT uTimeout, DWORD dwInfoFlags)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1, NIF_INFO | NIF_MESSAGE, WM_SHELLICON};
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
		TCHAR szErrorString[MAX_PATH];
		FormatMessage(
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR)&szErrorString,
		    MAX_PATH,
		    NULL
		);

		MessageBeep(MB_ICONSTOP);
		ShowBalloonTip(hWnd, szErrorString, TEXT("出现了一个错误"), 3000, NIIF_ERROR | NIIF_NOSOUND);
	}
}

void ShellCreateIcon(HWND hWnd)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_SHELLICON, LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1)), {TEXT("TranslucentFlyouts 用户界面")}};
	Shell_NotifyIcon(NIM_ADD, &data);
}

void ShellDestroyIcon(HWND hWnd)
{
	NOTIFYICONDATA data = {sizeof(data), hWnd, IDI_ICON1};
	Shell_NotifyIcon(NIM_DELETE, &data);
}

void OnInitDialogItem(const HWND& hWnd)
{
	const HWND& hCombobox1 = GetDlgItem(hWnd, IDC_COMBO1);
	const HWND& hCombobox2 = GetDlgItem(hWnd, IDC_COMBO2);
	const HWND& hCombobox3 = GetDlgItem(hWnd, IDC_COMBO3);
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
			ComboBox_SelectString(hCombobox3, -1, TEXT("完全不透明（适用于Win10）"));
			break;
		}
		case 1:
		{
			ComboBox_SelectString(hCombobox3, -1, TEXT("跟随透明度（适用于Win11）"));
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

void OnInitDialog(const HWND& hWnd)
{
	const HWND& hCombobox1 = GetDlgItem(hWnd, IDC_COMBO1);
	const HWND& hCombobox2 = GetDlgItem(hWnd, IDC_COMBO2);
	const HWND& hCombobox3 = GetDlgItem(hWnd, IDC_COMBO3);
	//
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	wp.flags = WPF_RESTORETOMAXIMIZED;
	wp.showCmd = SW_HIDE;
	//SetWindowPlacement(hWnd, &wp);
	//
	AssociateTooltip(hWnd, IDC_COMBO1);
	AssociateTooltip(hWnd, IDC_COMBO2);
	AssociateTooltip(hWnd, IDC_SLIDER1);
	AssociateTooltip(hWnd, IDC_COMBO3);
	AssociateTooltip(hWnd, IDC_BUTTON1);
	//
	SendDlgItemMessage(hWnd, IDC_SLIDER1, TBM_SETRANGE, 0, MAKELPARAM(0, 255));
	//
	HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)hIcon);
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
	ComboBox_AddString(hCombobox3, TEXT("跟随透明度（适用于Win11）"));
	ComboBox_AddString(hCombobox3, TEXT("完全不透明（适用于Win10）"));
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

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			ShellCreateIcon(hWnd);
			if (!RegisterHook())
			{
				ShowBalloonTip(hWnd);
			}
			OnInitDialog(hWnd);
			//
			break;
		}
		case WM_DESTROY:
		{
			if (!UnregisterHook())
			{
				ShowBalloonTip(hWnd);
			}
			ShellDestroyIcon(hWnd);
			break;
		}
		case WM_CLOSE:
		{
			ShowBalloonTip(hWnd, _T("如果要退出，请右键系统托盘图标"), _T("已最小化至系统托盘"), 3000, NIIF_INFO);
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		case WM_SHELLICON:
		{
			if (wParam == IDI_ICON1)
			{
				if (lParam == WM_LBUTTONUP)
				{
					ShowWindow(hWnd, SW_SHOW);
				}
				if (lParam == WM_RBUTTONUP)
				{
					ShowMenu(hWnd);
				}
			}
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
					ShowBalloonTip(hWnd);
				}
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
					pInfo->lpszText = (LPTSTR)TEXT("此参数决定弹出菜单鼠标悬停项的主题位图混合方式\r\n这是调用AlphaBlend混合时的不透明度选择方案\r\n建议在Windows 10普通主题下选择不透明增加对比度\r\nWindows 11圆角主题具有缺陷，在较高透明度下视觉效果有瑕疵，建议使用跟随透明度设置");
				}
				if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hWnd, IDC_BUTTON1))
				{
					pInfo->lpszText = (LPTSTR)TEXT("注意此选项会删除默认的配置信息\n但是不影响用户界面选项的设置\n即不会影响自启动");
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				case 0:
				{
					EndDialog(hWnd, TRUE);
					break;
				}
				case 1:
				{
					ShowWindow(hWnd, SW_SHOW);
					break;
				}
				case IDC_BUTTON2:
				{
					InvalidateRect(nullptr, nullptr, true);
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
							ShowBalloonTip(hWnd);
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
							ShowBalloonTip(hWnd);
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
						if (!_tcsicmp(szBuffer, TEXT("完全不透明（适用于Win10）")))
						{
							dwColorizeOption = 0;
						}
						if (!_tcsicmp(szBuffer, TEXT("跟随透明度（适用于Win11）")))
						{
							dwColorizeOption = 1;
						}
						if (!SetFlyoutColorizeOption(dwColorizeOption))
						{
							ShowBalloonTip(hWnd);
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
						ShowBalloonTip(hWnd);
					}
					else
					{
						ShowBalloonTip(hWnd, _T("已删除旧的配置信息"), _T("设置已更新"), 3000, NIIF_INFO);
					}
					OnInitDialogItem(hWnd);
					break;
				}
				case IDC_CHECK4:
				{
					HKEY hKey = nullptr;
					if (IsDlgButtonChecked(hWnd, IDC_CHECK4))
					{
						TCHAR pszModuleFileName[MAX_PATH];
						GetModuleFileName(NULL, pszModuleFileName, MAX_PATH);
						if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
						{
							if (RegSetValueEx(hKey, TEXT("TFGUI"), 0, REG_SZ, (LPBYTE)pszModuleFileName, sizeof(pszModuleFileName)) != ERROR_SUCCESS)
							{
								ShowBalloonTip(hWnd);
							}
							RegCloseKey(hKey);
						}
						else
						{
							ShowBalloonTip(hWnd);
						}
					}
					else
					{
						if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
						{
							if (RegDeleteValue(hKey, TEXT("TFGUI")) != ERROR_SUCCESS)
							{
								ShowBalloonTip(hWnd);
							}
							RegCloseKey(hKey);
						}
						else
						{
							ShowBalloonTip(hWnd);
						}
					}
				}
				default:
					break;
			}
			break;
		}
		default:
			if (Message == WM_TASKBARCREATED)
			{
				ShellCreateIcon(hWnd);
				return TRUE;
			}
			return FALSE;
	}
	return TRUE;
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
		return (int)DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
	}
	else
	{
		SetLastError(ERROR_ALIAS_EXISTS);
		ShowBalloonTip(FindWindow(_T("#32770"), _T("TranslucentFlyouts 选项")), _T("如果你需要访问用户界面，请单击系统托盘图标"), _T("实例已存在"), 3000, NIIF_INFO);
		return -1;
	}
}