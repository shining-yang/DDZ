// DdzServer.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "PokerAlgor.h"
#include "NetProc.h"
#include "GameProc.h"
#include "DdzServer.h"


volatile int g_nCurPokerAlgorithm = 0;

// 斗地主游戏洗牌算法列表
POKER_ALOGRITHM g_PokerAlgorithm[] = {
    { PA_Randomize1,        _T("Randomize1") },
    { PA_Randomize10,       _T("Randomize10") },
    { PA_RandRemainder,     _T("随机错位") },
    { PA_Test1,             _T("Test1") },
    { PA_Test2,             _T("Test2") },
    { PA_Test3,             _T("Test3") }
};

// 服务器主线程ID
DWORD g_nMainThreadId = 0;

HWND g_hMainWnd = NULL;
HWND g_hLogWnd = NULL;
HWND g_hPokerSelStatic = NULL;
HWND g_hPokerSelCombo = NULL;
HWND g_hClearLogButton = NULL;
HWND g_hLogInfoButton = NULL;
HWND g_hLogWarnButton = NULL;
HWND g_hLogErrorButton = NULL;
HWND g_hLogDebugButton = NULL;


HINSTANCE g_hAppInstance;                       // 当前实例
TCHAR g_szAppTitle[MAX_LOADSTRING];             // 标题栏文本
TCHAR g_szAppWindowClass[MAX_LOADSTRING];       // 主窗口类名

extern DWORD    g_dwServerIP;
extern USHORT   g_nServerPort;


// 主函数
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // 将主线程ID保存起来，以便其它线程可向主线程发送消息
    g_nMainThreadId = GetCurrentThreadId();

	// 执行应用程序初始化:
    if (!InitInstance(hInstance, nCmdShow)) {
		return -1;
	}

    // 初始化并开启服务器日志功能
    if (!StartServerLog()) {
        DestroyWindow(g_hMainWnd);
        return -1;
    }

    // 开启网络侦听
    if (!StartNetService()) {
        StopServerLog();
        DestroyWindow(g_hMainWnd);
        return -1;
    }

    // 创建游戏工作线程
    if (!BeginGameThreads()) {
        StopNetService();
        StopServerLog();
        DestroyWindow(g_hMainWnd);
        return -1;
    }

    // Show main window when all tasks succeed. 2010-05
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    // 完成所有必要的初始工作，服务器已经开始运行
    WriteLog(LOG_INFO, _T("服务器侦听网络连接于 [%d.%d.%d.%d: %d]。服务器开始运行..."),
        (ntohl(g_dwServerIP) >> 24) & 0xFF,
        (ntohl(g_dwServerIP) >> 16) & 0xFF,
        (ntohl(g_dwServerIP) >> 8) & 0xFF,
        (ntohl(g_dwServerIP) >> 0) & 0xFF,
        (ntohs(g_nServerPort)));

	// 主消息循环
    int ret;
    MSG msg;
    HACCEL hAccelTable;
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DDZSERVER));

	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) { continue; }

        if (msg.message == TM_CLIENT_DISCONNECT) {
            CloseConnectionThreadHandle((HANDLE)msg.wParam);
            continue;
        }

		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    // 退出游戏工作线程
    EndGameThreads();

    // 开闭网络连接，并等待其它服务线程退出
    StopNetService();

    // 关闭服务器日志功能
    StopServerLog();

	return (int)msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DDZSERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DDZSERVER);
	wcex.lpszClassName	= g_szAppWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    UNREFERENCED_PARAMETER(nCmdShow);

    HWND hWnd;

    // 将实例句柄存储在全局变量中
    g_hAppInstance = hInstance;

    // 初始化全局字符串
    LoadString(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DDZSERVER, g_szAppWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 创建主窗口
    hWnd = CreateWindow(g_szAppWindowClass, g_szAppTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL) {
        return FALSE;
    }

    // 保存主窗口句柄
    g_hMainWnd = hWnd;

    // 创建子窗口控件
    CreateChildWindows();

    // Show main window until all tasks succeed. 2010-05
    //ShowWindow(hWnd, nCmdShow);
    //UpdateWindow(hWnd);
    return TRUE;
}

//
//  函数: MainWndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int nID, nEvent;
    PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
	case WM_COMMAND:
        nID = LOWORD(wParam);
        nEvent = HIWORD(wParam);

        if ((HWND)lParam == g_hPokerSelCombo) {
            DoPokerSelCombo((HWND)lParam, nEvent);
            break;
        }

		// 分析菜单选择:
		switch (nID) {
		case IDM_ABOUT:
			DialogBox(g_hAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

        case IDC_BTN_LOGINFO:
        case IDC_BTN_LOGWARN:
        case IDC_BTN_LOGERROR:
        case IDC_BTN_LOGDEBUG:
            OnLogOptions((HWND)lParam, nID, nEvent);
            break;

        case IDC_BTN_CLEARLOG:
            ClearLog();
            break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            EnumChildWindows(hWnd, EnumChildWndProc, lParam);
        } else {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
        PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 创建子窗口
BOOL CreateChildWindows(void)
{
    // 初始化系统通用控件
    INITCOMMONCONTROLSEX ccex;
    ccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ccex.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES;
    InitCommonControlsEx(&ccex);

    // RichEdit
    HINSTANCE hRichED = LoadLibrary(_T("riched20.dll"));
    if (hRichED == NULL) {
        return FALSE;
    }
    g_hLogWnd = CreateWindow(RICHEDIT_CLASS, NULL,
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_DISABLENOSCROLL | ES_SUNKEN | ES_MULTILINE | ES_READONLY,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_RICHED_LOGWND, g_hAppInstance, NULL);
    if (g_hLogWnd == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hPokerSelStatic = CreateWindow(_T("STATIC"), _T("洗牌算法: "),
        WS_CHILD | SS_RIGHT,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_STATIC_POKERSEL, g_hAppInstance, NULL);
    if (g_hPokerSelStatic == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hLogWarnButton = CreateWindow(_T("BUTTON"), _T("WARNING"),
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_BTN_LOGWARN, g_hAppInstance, NULL);
    if (g_hLogWarnButton == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hLogErrorButton = CreateWindow(_T("BUTTON"), _T("ERROR"),
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_BTN_LOGERROR, g_hAppInstance, NULL);
    if (g_hLogErrorButton == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hLogDebugButton = CreateWindow(_T("BUTTON"), _T("DEBUG"),
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_BTN_LOGDEBUG, g_hAppInstance, NULL);
    if (g_hLogDebugButton == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hLogInfoButton = CreateWindow(_T("BUTTON"), _T("INFO"),
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_BTN_LOGINFO, g_hAppInstance, NULL);
    if (g_hLogInfoButton == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    g_hClearLogButton = CreateWindow(_T("BUTTON"), _T("清除日志"),
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_BTN_CLEARLOG, g_hAppInstance, NULL);
    if (g_hClearLogButton == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    //
    // ComboBoxEx 可以支持图标，这样就省去自绘列表图标。
    // 但 ComboBoxEx 不支持 CBS_DISABLENOSCROLL 风格，即始终显示垂直滚动条，而且，
    // 选择其中的列表项时，其高亮宽度只是文本宽度而不是整个列表矩形的宽度。
    // 此外，它还存在另一个问题，见 WM_SIZE 的消息处理
    //
//#define _USE_COMBO_EX_CTRL
#ifdef _USE_COMBO_EX_CTRL
    g_hPokerSelCombo = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
        WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_COMBO_POKERSEL, g_hAppInstance, NULL);
#else
    g_hPokerSelCombo = CreateWindow(WC_COMBOBOX, NULL,
        WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL,
        0, 0, 0, 0,
        g_hMainWnd, (HMENU)IDC_COMBO_POKERSEL, g_hAppInstance, NULL);
#endif
    if (g_hPokerSelCombo == NULL) {
        goto CREATE_CHILD_WND_FAIL;
    }

    for (int i = 0; i < sizeof(g_PokerAlgorithm) / sizeof(g_PokerAlgorithm[0]); i++) {
#ifdef _USE_COMBO_EX_CTRL
        COMBOBOXEXITEM cbei;
        cbei.mask       = CBEIF_TEXT;
        cbei.iItem      = i;
        cbei.pszText    = g_PokerAlgorithm[i].name;
        cbei.cchTextMax = (int)_tcslen(g_PokerAlgorithm[i].name);
        SendMessage(g_hPokerSelCombo, CBEM_INSERTITEM, 0, (LPARAM)&cbei);
#else
        SendMessage(g_hPokerSelCombo, CB_ADDSTRING, i, (LPARAM)g_PokerAlgorithm[i].name);
#endif
    }

    // 默认使用第0个算法
    SendMessage(g_hPokerSelCombo, CB_SETCURSEL, 0, 0);

    // 默认开启所有日志选项，参见变量s_logCurLevel
    SendMessage(g_hLogWarnButton, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(g_hLogErrorButton, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(g_hLogDebugButton, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(g_hLogInfoButton, BM_SETCHECK, BST_CHECKED, 0);

    // 设置字体，弃用系统默认字体
    SendMessage(g_hLogWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), (LPARAM)FALSE);

    SendMessage(g_hPokerSelStatic, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hPokerSelCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hLogWarnButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hLogErrorButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hLogDebugButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hLogInfoButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(g_hClearLogButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    return TRUE;

CREATE_CHILD_WND_FAIL:
#define DESTROY_CHILD_WND(_wnd) if (_wnd != NULL) { DestroyWindow(_wnd); _wnd = NULL; }
    DESTROY_CHILD_WND(g_hLogWnd);
    DESTROY_CHILD_WND(g_hPokerSelStatic);
    DESTROY_CHILD_WND(g_hLogWarnButton);
    DESTROY_CHILD_WND(g_hLogErrorButton);
    DESTROY_CHILD_WND(g_hLogDebugButton);
    DESTROY_CHILD_WND(g_hLogInfoButton);
    DESTROY_CHILD_WND(g_hClearLogButton);
    DESTROY_CHILD_WND(g_hPokerSelCombo);
    return FALSE;
}

// 处理COMMOBOX事件
void DoPokerSelCombo(HWND hWnd, int notify)
{
    int i = 0;
    int len = 0;
    TCHAR szText[64] = { 0 };

    switch (notify) {
    case CBN_SELCHANGE:
        i = (int)SendMessage(hWnd, CB_GETCURSEL, 0, 0);
        len = (int)SendMessage(hWnd, CB_GETLBTEXT, i, (LPARAM)szText);
        if (len != CB_ERR) {
            if (g_nCurPokerAlgorithm != i) {
                g_nCurPokerAlgorithm = i;
                WriteLog(LOG_INFO, _T("服务器洗牌算法已经更改为 [ %s ]，将在下次发牌时生效"), szText);
            }
        }
        break;
    }
}

// 调整子窗口尺寸
BOOL CALLBACK EnumChildWndProc(HWND hWnd, LPARAM lParam)
{
    int id = GetWindowLong(hWnd, GWL_ID);
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    int x = 0;
    int y = 0;
    int cx = 0;
    int cy = 0;

#define MY_STARTX               0
#define MY_STARTY               4
#define MY_CHECKBOX_WIDTH       100
#define MY_BTN_HEIGHT           24
#define MY_BTN_WIDTH            100

    if ((width < MY_BTN_WIDTH) || (height < MY_BTN_HEIGHT)) {
        return FALSE;
    }

    if (id == IDC_STATIC_POKERSEL) {
        x = MY_STARTX;
        y = MY_STARTY + 4;
        cx = 70;
        cy = MY_BTN_HEIGHT - 4;

#ifdef _USE_COMBO_EX_CTRL
    } else if (hWnd == (HWND)SendMessage(g_hPokerSelCombo, CBEM_GETCOMBOCONTROL, 0, 0)) {
        //
        // 这是COMBOBOXEX控件窗口中的子窗口COMBOBOX
        //
        // 如果使用控件ComboBoxEx，则该控件会返回两个WM_SIZE消息到父窗口，
        // 一个是ComboBoxEx返回的，一个是ComboBox返回的。为什么要返回两个呢？
        //
        // 当MoveWindow或SetWindowPos使ComboBoxEx控件改变位置或尺寸时，需要将
        // ComboBoxEx和ComboBox控件都移入新的矩形，此外，ComboBox的矩形坐标是
        // 相对于其父窗口ComboBoxEx的
        //
        //（如果使用ComboBox控件，则不会有此问题）
        //
        x = 0;
        y = 0;
        cx = 120;
        cy = 160;
#endif

    } else if (id == IDC_COMBO_POKERSEL) {
        x = 70;
        y = MY_STARTY;
        cx = 120;
        cy = 160;
    } else if (id == IDC_BTN_LOGWARN) {
        x = 240;
        y = MY_STARTY;
        cx = MY_CHECKBOX_WIDTH;
        cy = MY_BTN_HEIGHT;
    } else if (id == IDC_BTN_LOGERROR) {
        x = 340;
        y = MY_STARTY;
        cx = MY_CHECKBOX_WIDTH;
        cy = MY_BTN_HEIGHT;
    } else if (id == IDC_BTN_LOGDEBUG) {
        x = 440;
        y = MY_STARTY;
        cx = MY_CHECKBOX_WIDTH;
        cy = MY_BTN_HEIGHT;
    } else if (id == IDC_BTN_LOGINFO) {
        x = 540;
        y = MY_STARTY;
        cx = MY_CHECKBOX_WIDTH;
        cy = MY_BTN_HEIGHT;
    } else if (id == IDC_BTN_CLEARLOG) {
        x = width - (MY_BTN_WIDTH + 10);
        y = MY_STARTY;
        cx = MY_BTN_WIDTH;
        cy = MY_BTN_HEIGHT;
    } else if (id == IDC_RICHED_LOGWND) {
        x = MY_STARTX;
        y = MY_STARTY + MY_BTN_HEIGHT + MY_STARTY;
        cx = width;
        cy = height - (MY_STARTY + MY_BTN_HEIGHT + MY_STARTY);
    } else {
        return TRUE;
    }

    MoveWindow(hWnd, x, y, cx, cy, TRUE);
    ShowWindow(hWnd, SW_SHOW);
    return TRUE;
}

// 处理按钮通知
void OnLogOptions(HWND hWnd, int id, int nEvent)
{
    if (hWnd == NULL) { return; }
    if (nEvent != BN_CLICKED) { return; }

    BOOL bChecked = FALSE;
    bChecked = SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;

    switch (id) {
        case IDC_BTN_LOGINFO:
            LogEnableInfo(bChecked);
            break;
        case IDC_BTN_LOGWARN:
            LogEnableWarn(bChecked);
            break;
        case IDC_BTN_LOGERROR:
            LogEnableError(bChecked);
            break;
        case IDC_BTN_LOGDEBUG:
            LogEnableDebug(bChecked);
            break;
    }
}

