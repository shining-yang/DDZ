// DdzClient.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "NetProcess.h"

#include "Logon.h"
#include "MyButton.h"

#include "UserInfoWnd.h"
#include "ServerListWnd.h"

#include "GameSeatWnd.h"
#include "GameTableWnd.h"
#include "GameRoomWnd.h"

#include "HomePageWnd.h"
#include "PageTabWnd.h"
#include "UserListWnd.h"

#include "CurPokerWnd.h"
#include "OutCardWnd.h"
#include "UnderCardWnd.h"
#include "GamerVisualWnd.h"
#include "GameMainWnd.h"
#include "GameMainPage.h"

#include "DdzClient.h"


// 全局变量:
HINSTANCE   g_hAppInstance;                             // 当前实例
TCHAR       g_szAppTitle[MAX_LOADSTRING];               // 标题栏文本
TCHAR       g_szAppWindowClass[MAX_LOADSTRING];         // 主窗口类名

LPCTSTR     g_lpszSingleInstEntryName = _T("DdzClient_Single_Instance");

DWORD       g_nMainThreadId = 0;
HWND        g_hMainWnd = NULL;

BOOL        g_bShutdownConnection = FALSE;
BOOL        g_bProgramWillExit = FALSE;
SOCKET      g_ConSocket = INVALID_SOCKET;
DWORD       g_nConThreadId = 0;
HANDLE      g_hConThread = NULL;


// 当前用户基本信息
USER_LOGON_INFO     g_UserLogonInfo = { _T("Shining"), 2, TRUE };

// 用户界面（窗口）
PageTabWnd          g_PageTabWnd;

UserInfoWnd         g_UserInfoWnd;
ServerListWnd       g_ServerListWnd;
HomePageWnd         g_HomePageWnd;

GameRoomWnd         g_GameRoomWnd;
UserListWnd         g_UserListWnd;

GameMainPage        g_GameMainPage;



// 主程序入口
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化全局字符串
    LoadString(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DDZCLIENT, g_szAppWindowClass, MAX_LOADSTRING);

    g_nMainThreadId = GetCurrentThreadId();
    g_hAppInstance  = hInstance;

    // 作为单一实例运行
#ifdef RUN_AS_SINGLE_INSTANCE
    if (!IsFirstInstance()) {
        return 0;
    }
#endif

    // 注册所有使用的窗口类
    RegisterWindowClass(hInstance);

    // 初始化保存玩家信息的数据结构
    ResetPlayerDataTable();

    // 初始化WinSock
    if (!InitWinSocket()) {
        MessageBox(NULL, _T("您系统的WinSock版本低于2.2，程序将无法运行。\n请安装高级版本的WinSock。"),
            _T("WinSock版本太低"), MB_OK | MB_ICONERROR);
        return -1;
    }

    // 加载资源
    if (!LoadAppResource()) {
        MessageBox(NULL, _T("加载程序运行所需要的资源失败，请确保资源DLL没有损坏。"),
            _T("加载资源出错"), MB_OK | MB_ICONERROR);
        CloseWinSock();
        return -1;
    }


	// 创建窗口
    if (!InitInstance(hInstance, nCmdShow)) {
        FreeAppResource();
        CloseWinSock();
		return -1;
	}

    int ret;
    MSG msg;
    HACCEL hAccelTable;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DDZCLIENT));

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) { continue; }

        if (ProcessNetMessage(&msg)) { continue; }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ExitInstance();

	return (int)msg.wParam;
}

// 退出程序时作清理工作
void ExitInstance(void)
{
    if (g_ConSocket != INVALID_SOCKET) {
        g_bShutdownConnection = TRUE;
        CloseConnection(g_ConSocket);
        g_ConSocket = INVALID_SOCKET;

        if (g_hConThread != NULL) {
            WaitForSingleObject(g_hConThread, INFINITE);
            CloseHandle(g_hConThread);
            g_hConThread = NULL;
        }
    }

    CloseWinSock();
    FreeAppResource();
}

// 注册窗口类
ATOM MainWndRegister(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
    wcex.lpszClassName	= g_szAppWindowClass;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DDZCLIENT));
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcex.hbrBackground	= NULL;//(HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_DDZCLIENT);

	return RegisterClassEx(&wcex);
}

// 注册程序中将使用到的窗口类
BOOL RegisterWindowClass(HINSTANCE hInstance)
{
    // 初始化常用的控件窗口类
    INITCOMMONCONTROLSEX ccex;
    ccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ccex.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES;
    InitCommonControlsEx(&ccex);

    // 主窗口类
    MainWndRegister(hInstance);

    // 注册程序用到的窗口类
    UserInfoWnd::UserInfoWndRegister(hInstance);
    ServerListWnd::ServerListWndRegister(hInstance);
    HomePageWnd::HomePageWndRegister(hInstance);
    PageTabWnd::PageTabWndRegister(hInstance);

    GameSeatWnd::GameSeatWndRegister(hInstance);
    GameTableWnd::GameTableWndRegister(hInstance);
    GameRoomWnd::GameRoomWndRegister(hInstance);

    UserListWnd::UserListWndRegister(hInstance);

    MyButton::MyButtonRegister(hInstance);
    CurPokerWnd::CurPokerWndRegister(hInstance);
    OutCardWnd::OutCardWndRegister(hInstance);
    UnderCardWnd::UnderCardWndRegister(hInstance);
    GamerVisualWnd::GamerVisualWndRegister(hInstance);
    GameMainWnd::GameMainWndRegister(hInstance);
    GameMainPage::GameMainPageRegister(hInstance);

    return TRUE;
}

// 作为单一程序实例运行
BOOL IsFirstInstance(void)
{
    HANDLE hSem = CreateSemaphore(NULL, 1, 1, g_lpszSingleInstEntryName);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hSem);

        HWND hWndPrevious = GetWindow(GetDesktopWindow(), GW_CHILD);

        while (IsWindow(hWndPrevious)) {
            if (GetProp(hWndPrevious, g_lpszSingleInstEntryName) != NULL) {
                if (IsIconic(hWndPrevious)) {
                    ShowWindow(hWndPrevious, SW_RESTORE);
                }

                SetForegroundWindow(hWndPrevious);
                return FALSE;
            }

            hWndPrevious = GetWindow(hWndPrevious, GW_HWNDNEXT);
        }

        MessageBox(NULL, _T("应用程序实例已存在，但找不到其主窗口"), _T("错误"), MB_OK);
        return FALSE;
    }

    return TRUE;
}

// 初始化应用程序实例并创建和显示主窗口
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    UNREFERENCED_PARAMETER(nCmdShow);

    HWND hWnd = CreateWindow(g_szAppWindowClass, g_szAppTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) {
        return FALSE;
    }

    g_hMainWnd = hWnd;
    SetProp(hWnd, g_lpszSingleInstEntryName, (HANDLE)1);

#if 1
    // 显示登陆对话框，并获取用户基本信息
    if (!UserLogon(hInstance, NULL, (LPARAM)&g_UserLogonInfo)) {
        DestroyWindow(hWnd);
        return FALSE;
    }
#endif

    // 创建用户信息面板
    g_UserInfoWnd.Create(0, 0, 0, 0, hWnd, hInstance);
    g_UserInfoWnd.SetUserInfo(g_UserLogonInfo.szName, g_UserLogonInfo.nImage);

    // 创建服务器列表面板
    g_ServerListWnd.Create(0, 0, 0, 0, hWnd, hInstance);
    g_ServerListWnd.SetServerInfoConfigFile(SERVER_LIST_INI_FILE);
    g_ServerListWnd.SetServerInfoImageList(g_himlServerTree);

    // 创建PageTableCtrl
    g_PageTabWnd.Create(0, 0, 0, 0, hWnd, hInstance);
    g_PageTabWnd.SetTabImageList(g_himlTabCtrl);
    g_PageTabWnd.InsertHomeTabPage();

    HMODULE hModule = LoadLibrary(RES_POKER_DLL_NAME);
    if (hModule != NULL) {
        g_PageTabWnd.m_btnReturn.SetBitmap(hModule,
            MAKEINTRESOURCE(IDB_BTN_TEMPLATE), RGB(255,255,255));
        FreeLibrary(hModule);
    }

    g_PageTabWnd.m_btnReturn.SetText(_T("返回"), RGB(255,255,255));

    // 创建首页窗口
    g_HomePageWnd.Create(0, 0, 400, 300, hWnd, hInstance);

#ifdef USE_LOCAL_FILE_AS_HOMEPAGE
    TCHAR homepage[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, homepage, MAX_PATH);
    PathRemoveFileSpec(homepage);
    _tcscat_s(homepage, MAX_PATH, _T("\\"));
    _tcscat_s(homepage, MAX_PATH, DEFAULT_HOME_PAGE);
    g_HomePageWnd.SetPageURL(homepage);
#else
    g_HomePageWnd.SetPageURL(DEFAULT_HOME_PAGE);
#endif

    // 创建大厅窗口
    g_GameRoomWnd.Create(0, 0, 400, 300, hWnd, hInstance);
    g_GameRoomWnd.Show(FALSE);

    // 创建用户列表面板
    g_UserListWnd.Create(0, 0, 0, 0, hWnd, hInstance);
    g_UserListWnd.Show(FALSE);

    // 创建游戏主页面窗口
    g_GameMainPage.Create(0, 0, 0, 0, hWnd, hInstance);
    g_GameMainPage.Show(FALSE);

    //ShowWindow(hWnd, nCmdShow);
    ShowWindow(hWnd, SW_MAXIMIZE);
    UpdateWindow(hWnd);

    return TRUE;
}

// 处理主窗口的消息。
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	UINT wmId;
    UINT wmEvent;

    HDC hdc;
    PAINTSTRUCT ps;

    LPMINMAXINFO lpMinMaxInfo;

	switch (nMsg) {
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

        switch (wmId) {
		    case IDM_ABOUT:
                DialogBox(g_hAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			    break;
		    case IDM_EXIT:
			    DestroyWindow(hWnd);
			    break;
		    default:
			    return DefWindowProc(hWnd, nMsg, wParam, lParam);
		}
		break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        break;

    case WM_SIZE:
        OnSize(wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        lpMinMaxInfo = (LPMINMAXINFO)lParam;
        lpMinMaxInfo->ptMinTrackSize.x = MAIN_WND_MIN_WIDTH;
        lpMinMaxInfo->ptMinTrackSize.y = MAIN_WND_MIN_HEIGHT;
        break;

    case WM_SHOW_HOME_PAGE:
        if (g_PageTabWnd.GetCurTabSel() != HOME_PAGE_TAB_INDEX) {
            g_PageTabWnd.SetCurTabSel(HOME_PAGE_TAB_INDEX);
            OnPageSelChange(hWnd, HOME_PAGE_TAB_PARAM);
        }
        break;

    case WM_CONNECT_SERVER:
        OnConnectServer(wParam, lParam);
        break;

    case WM_PAGE_RETURN:
        OnPageReturn(hWnd, lParam);
        break;

    case WM_PAGE_SEL_CHANGING:
        OnPageSelChanging(hWnd, lParam);
        break;

    case WM_PAGE_SEL_CHANGE:
        OnPageSelChange(hWnd, lParam);
        break;

    case WM_USER_CLICK_SEAT:
        OnUserClickSeat(wParam, lParam);
        break;

	case WM_DESTROY:
        RemoveProp(hWnd, g_lpszSingleInstEntryName);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, nMsg, wParam, lParam);
	}

	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (nMsg) {
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

// 绘制窗口
void OnPaint(HDC hdc)
{
    RECT rect;
    GetClientRect(g_hMainWnd, &rect);

    // 主窗口仅绘制一个边框
    HPEN hpen = CreatePen(PS_SOLID | PS_INSIDEFRAME, MAIN_WND_MARGIN, MAIN_WND_MARGIN_CLR);
    HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
    HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, 0, 0, rect.right, rect.bottom);

    SelectObject(hdc, oldbrush);
    SelectObject(hdc, oldpen);
    DeleteObject(hpen);
}

// 窗口尺寸改变
void OnSize(WPARAM wParam, LPARAM lParam)
{
    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);

    if ((cx <= 0) || (cy <= 0)) { return; }

    // 给主窗口留一点空白区域，这样显得不太拥挤，美观些
    cx -= (2 * MAIN_WND_MARGIN);
    cy -= (2 * MAIN_WND_MARGIN);

    switch (wParam) {
    case SIZE_MAXIMIZED:
    case SIZE_RESTORED:
        if (g_UserInfoWnd.IsVisible() && g_ServerListWnd.IsVisible()) {
            g_UserInfoWnd.SetWindowRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN,
                CX_USER_INFO_WND, CY_USER_INFO_WND);

            g_ServerListWnd.SetWindowRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN + CY_USER_INFO_WND,
                CX_SERVER_LIST_WND, cy - CY_USER_INFO_WND);

            g_PageTabWnd.SetWindwRect(MAIN_WND_MARGIN + CX_USER_INFO_WND, MAIN_WND_MARGIN,
                cx - CX_USER_INFO_WND, CY_PAGE_TAB_CTRL);

            g_HomePageWnd.SetWindowRect(MAIN_WND_MARGIN + CX_USER_INFO_WND, MAIN_WND_MARGIN + CY_PAGE_TAB_CTRL,
                cx - CX_USER_INFO_WND, cy - CY_PAGE_TAB_CTRL);
        } else if (g_GameRoomWnd.IsVisible()) {
            g_PageTabWnd.SetWindwRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN,
                cx - CX_USER_LIST_WND, CY_PAGE_TAB_CTRL);

            g_GameRoomWnd.SetWindowRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN + CY_PAGE_TAB_CTRL,
                cx - CX_USER_LIST_WND, cy - CY_PAGE_TAB_CTRL);

            g_UserListWnd.SetWindowRect(MAIN_WND_MARGIN + cx - CX_USER_LIST_WND, MAIN_WND_MARGIN,
                CX_USER_LIST_WND, cy);
        } else {// if (g_GameMainPage.IsVisible()) {
            g_PageTabWnd.SetWindwRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN,
                cx, CY_PAGE_TAB_CTRL);

            g_GameMainPage.SetWindowRect(MAIN_WND_MARGIN, MAIN_WND_MARGIN + CY_PAGE_TAB_CTRL,
                cx, cy - CY_PAGE_TAB_CTRL);
        }
        break;
    }
}

// 断开连接，并复位相关数据
void DisconnectServer(void)
{
    if (g_ConSocket == INVALID_SOCKET) {
        return;
    }

    g_bShutdownConnection = TRUE;
    CloseConnection(g_ConSocket);
    g_ConSocket = INVALID_SOCKET;

    WaitForSingleObject(g_hConThread, INFINITE);
    CloseHandle(g_hConThread);
    g_hConThread = NULL;
    g_bShutdownConnection = FALSE;

    g_UserListWnd.RemoveAllUserItems();
    g_GameRoomWnd.ResetGameRoom();
    g_GameMainPage.Init(FALSE);

    ResetPlayerDataTable();
}

// 连接服务器
void OnConnectServer(WPARAM wParam, LPARAM lParam)
{
    DWORD ip = (DWORD)wParam;
    USHORT port = LOWORD(lParam);

    PLAYER_STATE state = GetLocalUserState();
    if (state != STATE_IDLE) {
        g_GameMainPage.WriteLog(LOG_WARN, _T("请先离开游戏桌，返回到大厅，再连接别的服务器。"));
        return;
    }

    if (g_ConSocket != INVALID_SOCKET) { // IDLE，并且已经与某个服务器有连接，则删除大厅Tab页面
        g_PageTabWnd.RemoveTabPage(ROOM_PAGE_TAB_INDEX);
        g_PageTabWnd.SetCurTabSel(HOME_PAGE_TAB_INDEX);
        OnPageSelChange(g_hMainWnd, (LPARAM)HOME_PAGE_TAB_PARAM);
    }

    // 如果已经连接到一个服务器，则先断开连接
    DisconnectServer();

    g_ConSocket = EstablishConnection(ip, port);
    if (g_ConSocket == INVALID_SOCKET) {
        MessageBox(g_hMainWnd, _T("连接服务器失败。\n请确保服务器参数设置正确并已经运行可用。"),
            _T("错误"), MB_OK | MB_ICONERROR);
        return;
    }

    g_hConThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ConnectionReceiveDataProc,
        (LPVOID)g_ConSocket, CREATE_SUSPENDED, &g_nConThreadId);
    if (g_hConThread == NULL) {
        MessageBox(g_hMainWnd, _T("连接服务器成功，但创建接收数据的线程失败。\n已经关闭网络连接，请重试几次。"),
            _T("错误"), MB_OK | MB_ICONERROR);
        CloseConnection(g_ConSocket);
        g_ConSocket = INVALID_SOCKET;
        return;
    }

    //
    // 建立连接之后，先发送自己的信息到服务器，然后开始收、发数据
    //
    SendReqRoomInfo();
    ResumeThread(g_hConThread);

    //
    // 为避免进入大厅后，快速点击入座，则其他玩家可能没有自己的信息，这里等待一
    // 会。以便服务器的1秒定时广播能将自己登陆的信息告知其他玩家。
    //
    Sleep(500);

    g_PageTabWnd.InsertRoomTabPage();
    g_PageTabWnd.SetCurTabSel(ROOM_PAGE_TAB_INDEX);
    OnPageSelChange(g_hMainWnd, ROOM_PAGE_TAB_PARAM);
}

// Tab页面返回
void OnPageReturn(HWND hWnd, LPARAM lParam)
{
    PLAYER_STATE state = GetLocalUserState();
    int nUserId = GetLocalUserId();
    int table = GetLocalUserTableId();
    int seat = GetLocalUserSeatId();

    if (lParam == HOME_PAGE_TAB_PARAM) {
        // No returning at this page.
    }

    if (lParam == ROOM_PAGE_TAB_PARAM) {
        if (state != STATE_IDLE) {
            TRACE(_T("您正在游戏桌，不能直接从大厅返回。请先离开游戏桌。\n"));
            g_GameMainPage.WriteLog(LOG_WARN, _T("您正在游戏桌，不能直接从大厅返回。请先离开游戏桌。"));
            return;
        }

        g_PageTabWnd.RemoveTabPage(ROOM_PAGE_TAB_INDEX);
        g_PageTabWnd.SetCurTabSel(HOME_PAGE_TAB_INDEX);
        OnPageSelChange(hWnd, (LPARAM)HOME_PAGE_TAB_PARAM);

        DisconnectServer();
    }

    if (lParam == GAME_PAGE_TAB_PARAM) {
        if (state == STATE_GAMING) {
            // ToDo: 请求中途退出游戏对话框
            TRACE(_T("您正在游戏，不能返回。\n"));
            g_GameMainPage.WriteLog(LOG_WARN, _T("您正在游戏，不能返回。"));
            return;
        }

        if (!IS_INVALID_TABLE(table) && !IS_INVALID_SEAT(seat)) {
            if (state == STATE_LOOKON) {
                SendReqLookonLeaveSeat(table, seat);
                g_GameRoomWnd.LookonLeaveSeat(nUserId, table, seat);
            } else {
                SendReqGamerLeaveSeat(table, seat);
                g_GameRoomWnd.GamerLeaveSeat(nUserId, table, seat);
            }

            // 将自己从本地大厅信息中删除，不必等待服务器1秒定时广播的通知我离开座位，这样要快些
            g_PlayerData[nUserId].playerInfo.table = INVALID_TABLE;
            g_PlayerData[nUserId].playerInfo.seat = INVALID_SEAT;
            g_PlayerData[nUserId].playerInfo.state = STATE_IDLE;
        }

        g_PageTabWnd.RemoveTabPage(GAME_PAGE_TAB_INDEX);
        g_PageTabWnd.SetCurTabSel(ROOM_PAGE_TAB_INDEX);
        OnPageSelChange(hWnd, (LPARAM)ROOM_PAGE_TAB_PARAM);
    }
}

// Tab页面准备发生改变
void OnPageSelChanging(HWND hWnd, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lParam);
}

// Tab页面已经发生改变
void OnPageSelChange(HWND hWnd, LPARAM lParam)
{
    if (lParam == HOME_PAGE_TAB_PARAM) { // 首页
        g_UserInfoWnd.Show();
        g_ServerListWnd.Show();
        g_HomePageWnd.Show();
        g_GameRoomWnd.Show(FALSE);
        g_UserListWnd.Show(FALSE);
        g_GameMainPage.Show(FALSE);
    } else if (lParam == ROOM_PAGE_TAB_PARAM) { // 大厅
        g_HomePageWnd.Show(FALSE);
        g_UserInfoWnd.Show(FALSE);
        g_ServerListWnd.Show(FALSE);
        g_GameRoomWnd.Show();
        g_UserListWnd.Show();
        g_GameMainPage.Show(FALSE);
    } else if (lParam == GAME_PAGE_TAB_PARAM) { // 游戏页面
        g_UserInfoWnd.Show(FALSE);
        g_ServerListWnd.Show(FALSE);
        g_HomePageWnd.Show(FALSE);
        g_GameRoomWnd.Show(FALSE);
        g_UserListWnd.Show(FALSE);
        g_GameMainPage.Show();
    }

    RECT rect;
    GetClientRect(hWnd, &rect);
    OnSize((WPARAM)SIZE_RESTORED, MAKELPARAM(rect.right, rect.bottom));
}

void OnUserClickSeat(int table, int seat)
{
    int nUserId = GetLocalUserId();
    PLAYER_STATE prev_state = GetLocalUserState();
    int prev_table = GetLocalUserTableId();
    int prev_seat = GetLocalUserSeatId();

    if ((table == prev_table) && (seat == prev_seat)) {
        return;
    }

    // 该椅子是否已经有人坐下
    BOOL bAvailable = FALSE;

    int nGamerId = GetGamerId(table, seat);
    if (nGamerId == INVALID_USER_ID) {
        bAvailable = TRUE;
    }

    switch (prev_state) {
        case STATE_SIT:
        case STATE_READY:
            if (!IS_INVALID_TABLE(prev_table)) {
                if (!IS_INVALID_SEAT(prev_seat)) { // 发送玩家离开，并自行更新本地数据
                    SendReqGamerLeaveSeat(prev_table, prev_seat);
                    g_GameRoomWnd.GamerLeaveSeat(nUserId, prev_table, prev_seat);
                }
            }

            g_PageTabWnd.RemoveGameTabPage();
            break;

        case STATE_LOOKON:
            if (!IS_INVALID_TABLE(prev_table)) {
                if (!IS_INVALID_SEAT(prev_seat)) { // 发送旁观者离开，并自行更新本地数据
                    SendReqLookonLeaveSeat(prev_table, prev_seat);
                    g_GameRoomWnd.LookonLeaveSeat(nUserId, prev_table, prev_seat);
                }
            }

            g_PageTabWnd.RemoveGameTabPage();
            break;

        case STATE_GAMING:
            //MessageBox(g_hMainWnd, _T("您正在游戏中，不能离开座位。"), _T("提示"), MB_OK);
            g_GameMainPage.WriteLog(LOG_WARN, _T("您正在游戏中，不能直接进入其它座位。"));
            return; // no more execution
    }

    if (bAvailable == TRUE) {
        SendReqGamerTakeSeat(table, seat);
    } else {
        SendReqLookonTakeSeat(table, seat);
    }
}

// 闪烁非激活窗口
void FlashInactiveWnd(HWND hWnd, UINT uCount)
{
    WINDOWINFO wi = { 0 };
    GetWindowInfo(hWnd, &wi);

    if (wi.dwWindowStatus != WS_ACTIVECAPTION) {
        FLASHWINFO fwi = { 0 };
        fwi.cbSize      = sizeof(FLASHWINFO);
        fwi.hwnd        = hWnd;
        fwi.dwFlags     = FLASHW_TRAY | FLASHW_TIMERNOFG;
        fwi.uCount      = uCount;
        fwi.dwTimeout   = 0;

        FlashWindowEx(&fwi);
    }
}

// 闪烁主窗口 4 次
void FlashInactiveWnd(void)
{
    FlashInactiveWnd(g_hMainWnd, 4);
}

// 处理主线程消息。主要用于处理连接线程给主线程寄送的消息
// 若处理消息则返回TRUE，否则返回FALSE
BOOL ProcessNetMessage(LPMSG lpMsg)
{
    UINT nMsg = lpMsg->message;

#define ON_TMSG_BREAK(TMSG_ID) \
    case TMSG_ID: { On_ ## TMSG_ID (lpMsg->wParam, lpMsg->lParam); break; }

    switch (nMsg) {
        ON_TMSG_BREAK(  TM_CONNECTION_LOST);
        ON_TMSG_BREAK(  TM_RECV_SVR_ROOM_INFO);
        ON_TMSG_BREAK(  TM_RECV_SVR_STATUS);
        ON_TMSG_BREAK(  TM_RECV_PLAYER_STATE_CHANGE);
        ON_TMSG_BREAK(  TM_GAMER_TAKE_SEAT);
        ON_TMSG_BREAK(  TM_LOOKON_TAKE_SEAT);
        ON_TMSG_BREAK(  TM_GAMER_LEAVE_SEAT);
        ON_TMSG_BREAK(  TM_LOOKON_LEAVE_SEAT);
        ON_TMSG_BREAK(  TM_GAMER_READY);
        ON_TMSG_BREAK(  TM_DISTRIBUTE_POKER);
        ON_TMSG_BREAK(  TM_SVR_REQ_VOTE_LORD);
        ON_TMSG_BREAK(  TM_SVR_NOTIFY_VOTE_LORD);
        ON_TMSG_BREAK(  TM_VOTE_LORD_FINISH);
        ON_TMSG_BREAK(  TM_SVR_REQ_OUTPUT_CARD);
        ON_TMSG_BREAK(  TM_SVR_NOTIFY_OUTPUT_CARD);
        ON_TMSG_BREAK(  TM_SVR_NOTIFY_GAME_OVER);
        ON_TMSG_BREAK(  TM_GAMER_CHATTING);
        ON_TMSG_BREAK(  TM_GAMER_DELEGATED);
        ON_TMSG_BREAK(  TM_GAMER_DISCONNECTED);

        default:
            return FALSE;
    }

    return TRUE;
}

void On_TM_CONNECTION_LOST(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int nReason = (int)wParam;

    if (nReason == -1) {
        MessageBox(g_hMainWnd, _T("连接线程分配接收数据缓冲区失败，无法与服务器建立连接。"),
            _T("建立连接失败"), MB_OK | MB_ICONERROR);
    } else {
        MessageBox(g_hMainWnd, _T("服务器可能关闭，或者是您的网络连接已经不可用。\n请确认并重新连接服务器。"),
            _T("断开连接"), MB_OK | MB_ICONERROR);
    }

    if (g_hConThread != NULL) {
        WaitForSingleObject(g_hConThread, INFINITE);
        CloseHandle(g_hConThread);
        g_hConThread = NULL;
    }

    g_PageTabWnd.RemoveGameTabPage();
    g_PageTabWnd.RemoveRoomTabPage();
    g_PageTabWnd.SetCurTabSel(HOME_PAGE_TAB_INDEX);
    OnPageSelChange(g_hMainWnd, HOME_PAGE_TAB_PARAM);
    
    // 复位必要数据结构
    DisconnectServer();
}

void On_TM_RECV_SVR_ROOM_INFO(WPARAM wParam, LPARAM lParam)
{
    int nClientID = LOWORD(wParam);
    int nPlayerCount = HIWORD(wParam);
    LPPLAYER_INFO lpPlayerInfo = (LPPLAYER_INFO)lParam;

    if (nPlayerCount == 0) { // It's the first user connected to the server, no other users yet.
        assert(lpPlayerInfo == NULL);
        g_nLocalPlayerId = nClientID;
        return;
    }

    assert(lpPlayerInfo != NULL);
    assert((nPlayerCount >= 0) && (nPlayerCount <= MAX_CONNECTION_NUM));
    TRACE2(_T("ACK_ROOM_INFO. Assigned ID: %d, Get %d users\n"), nClientID, nPlayerCount);

    for (int i = 0; i < nPlayerCount; i++) {
        int id = lpPlayerInfo[i].id;

        g_PlayerData[id].valid = TRUE;
        g_PlayerData[id].playerInfo.gender = lpPlayerInfo[i].gender;
        g_PlayerData[id].playerInfo.imageIdx = lpPlayerInfo[i].imageIdx;
        _tcscpy_s(g_PlayerData[id].playerInfo.name, MAX_USER_NAME_LEN, lpPlayerInfo[i].name);
        g_PlayerData[id].playerInfo.bAllowLookon = lpPlayerInfo[i].bAllowLookon;
        g_PlayerData[id].playerInfo.state = lpPlayerInfo[i].state;
        g_PlayerData[id].playerInfo.table = lpPlayerInfo[i].table;
        g_PlayerData[id].playerInfo.seat = lpPlayerInfo[i].seat;

        g_PlayerData[id].playerInfo.nScore = lpPlayerInfo[i].nScore;
        g_PlayerData[id].playerInfo.nTotalGames = lpPlayerInfo[i].nTotalGames;
        g_PlayerData[id].playerInfo.nWinGames = lpPlayerInfo[i].nWinGames;
        g_PlayerData[id].playerInfo.nRunawayTimes = lpPlayerInfo[i].nRunawayTimes;

        // Set UserListWnd Data
        g_UserListWnd.InsertUserItem(id);

        // Set GameRoomWnd Data
        switch (lpPlayerInfo[i].state) {
            case STATE_SIT:
                g_GameRoomWnd.GamerTakeSeat(id, lpPlayerInfo[i].table, lpPlayerInfo[i].seat);
                break;

            case STATE_READY:
                g_GameRoomWnd.GamerTakeSeat(id, lpPlayerInfo[i].table, lpPlayerInfo[i].seat);
                g_GameRoomWnd.GamerGetReady(lpPlayerInfo[i].table, lpPlayerInfo[i].seat, TRUE);
                break;

            case STATE_GAMING:
                g_GameRoomWnd.GamerTakeSeat(id, lpPlayerInfo[i].table, lpPlayerInfo[i].seat);
                g_GameRoomWnd.TableGameStarted(lpPlayerInfo[i].table, TRUE);
                break;

            case STATE_LOOKON:
                g_GameRoomWnd.LookonTakeSeat(id, lpPlayerInfo[i].table, lpPlayerInfo[i].seat);
                break;
        }
    }

    // save the ID which the server assigned for me
    g_nLocalPlayerId = nClientID;

    LocalFree(lpPlayerInfo);

    g_GameRoomWnd.UpdateWnd();
    g_UserListWnd.UpdateWnd();
}

void On_TM_RECV_SVR_STATUS(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    TRACE2(_T("SERVER status: (%d/%d)\n"), wParam, lParam);
    // ToDo: update the server-list tree view
}

void On_TM_RECV_PLAYER_STATE_CHANGE(WPARAM wParam, LPARAM lParam)
{
    int count = (int)wParam;
    LPPLAYER_CHG_DATA pChangeData = (LPPLAYER_CHG_DATA)lParam;

    assert(count > 0);
    assert(pChangeData != NULL);

    PLAYER_STATE    prevState;
    int             prevTable;
    int             prevSeat;

    // 客户端需先获取服务器数据（大厅信息），才处理状态变化消息。
    // 若没获取服务器大厅信息之前（没分配一个ID），收到定时广播的消息，则不处理。
    if (GetLocalUserId() == INVALID_USER_ID) {
        LocalFree(pChangeData);
        return;
    }

    for (int i = 0; i < count; i++) {
        STATE_CHANGE_EVENT evtChange = pChangeData[i].event;
        int id = pChangeData[i].playerInfo.id;

        switch (evtChange) {
            case EVT_ENTER:
                TRACE2(_T("EVT_ENTER.[ID: %d, Name: %s]\n"), id, pChangeData[i].playerInfo.name);

                g_PlayerData[id].valid = TRUE;
                g_PlayerData[id].playerInfo.gender = pChangeData[i].playerInfo.gender;
                g_PlayerData[id].playerInfo.imageIdx = pChangeData[i].playerInfo.imageIdx;
                _tcscpy_s(g_PlayerData[id].playerInfo.name, MAX_USER_NAME_LEN, pChangeData[i].playerInfo.name);

                assert(pChangeData[i].playerInfo.state == STATE_IDLE);

                // other fields may use the fault values
                g_PlayerData[id].playerInfo.bAllowLookon = TRUE;
                g_PlayerData[id].playerInfo.state = STATE_IDLE;
                g_PlayerData[id].playerInfo.table = INVALID_TABLE;
                g_PlayerData[id].playerInfo.seat = INVALID_SEAT;

                // add this user to ListCtrl
                g_UserListWnd.InsertUserItem(id);
                break;

            case EVT_LEAVE:
                TRACE2(_T("EVT_LEAVE. [ID: %d, Name: %s]\n"), id, g_PlayerData[id].playerInfo.name);

                prevTable   = g_PlayerData[id].playerInfo.table;
                prevSeat    = g_PlayerData[id].playerInfo.seat;
                prevState   = g_PlayerData[id].playerInfo.state;

                if ((prevTable != INVALID_TABLE) && (prevSeat != INVALID_SEAT)) {
                    if (prevState == STATE_LOOKON) { // look-on leave seat
                        g_GameRoomWnd.LookonLeaveSeat(id, prevTable, prevSeat);
                    } else {
                        g_GameRoomWnd.GamerLeaveSeat(id, prevTable, prevSeat);
                    }
                }

                // reset the entry
                ZeroMemory(&g_PlayerData[id], sizeof(PLAYER_DATA));

                // Fields may set to the default values, or just leave them alone
                g_PlayerData[id].playerInfo.bAllowLookon = TRUE;
                g_PlayerData[id].playerInfo.state = STATE_IDLE;
                g_PlayerData[id].playerInfo.table = INVALID_TABLE;
                g_PlayerData[id].playerInfo.seat = INVALID_SEAT;

                g_UserListWnd.RemoveUserItem(id);
                break;

            case EVT_CHGSTATE:
                if (g_PlayerData[id].playerInfo.bAllowLookon != pChangeData[i].playerInfo.bAllowLookon) {
                    TRACE3(_T("EVT_CHGSTATE. [ID: %d] - [%s] set AllowLookOn [%s]\n"),
                        id, g_PlayerData[id].playerInfo.name, 
                        pChangeData[i].playerInfo.bAllowLookon ? _T("TRUE") : _T("FALSE"));
                }

                g_PlayerData[id].playerInfo.bAllowLookon = pChangeData[i].playerInfo.bAllowLookon;

                if (pChangeData[i].bContainStatistics == TRUE) {
                    g_PlayerData[id].playerInfo.nScore          = pChangeData[i].playerInfo.nScore;
                    g_PlayerData[id].playerInfo.nTotalGames     = pChangeData[i].playerInfo.nTotalGames;
                    g_PlayerData[id].playerInfo.nWinGames       = pChangeData[i].playerInfo.nWinGames;
                    g_PlayerData[id].playerInfo.nRunawayTimes   = pChangeData[i].playerInfo.nRunawayTimes;
                }

                switch (pChangeData[i].playerInfo.state) {
                    case STATE_IDLE:
                        TRACE2(_T("Player [ID: %d] - [%s] ==> IDLE\n"), id, g_PlayerData[id].playerInfo.name);

                        prevTable   = g_PlayerData[id].playerInfo.table;
                        prevSeat    = g_PlayerData[id].playerInfo.seat;
                        prevState   = g_PlayerData[id].playerInfo.state;

                        if ((prevTable != INVALID_TABLE) && (prevSeat != INVALID_SEAT)) {
                            if (prevState == STATE_LOOKON) { // look-on leave seat
                                g_GameRoomWnd.LookonLeaveSeat(id, prevTable, prevSeat);
                            } else {
                                g_GameRoomWnd.GamerLeaveSeat(id, prevTable, prevSeat);
                            }
                        }

                        g_PlayerData[id].playerInfo.table = INVALID_TABLE;
                        g_PlayerData[id].playerInfo.seat = INVALID_SEAT;

                        g_PlayerData[id].playerInfo.state = STATE_IDLE;
                        break;

                    case STATE_SIT:
                        TRACE2(_T("Player [ID: %d] - [%s] ==> SIT\n"), id, g_PlayerData[id].playerInfo.name);

                        g_PlayerData[id].playerInfo.table = pChangeData[i].playerInfo.table;
                        g_PlayerData[id].playerInfo.seat = pChangeData[i].playerInfo.seat;

                        prevState = g_PlayerData[id].playerInfo.state;
                        if (prevState == STATE_IDLE) { // gamer take seat
                            g_GameRoomWnd.GamerTakeSeat(id, g_PlayerData[id].playerInfo.table, g_PlayerData[id].playerInfo.seat);
                            g_GameRoomWnd.SetAllowLookon(g_PlayerData[id].playerInfo.table, g_PlayerData[id].playerInfo.seat, g_PlayerData[id].playerInfo.bAllowLookon);
                        } else if (prevState == STATE_GAMING) { // game over
                            g_GameRoomWnd.GamerGetReady(g_PlayerData[id].playerInfo.table, g_PlayerData[id].playerInfo.seat, FALSE);

                            if (id == GetLocalUserId()) {
                                g_GameMainPage.m_GameMainWnd.GamerCanStart();
                            }
                        }

                        g_PlayerData[id].playerInfo.state = STATE_SIT;
                        break;

                    case STATE_READY:
                        TRACE2(_T("Player [ID: %d] - [%s] ==> READY\n"), id, g_PlayerData[id].playerInfo.name);

                        g_PlayerData[id].playerInfo.table = pChangeData[i].playerInfo.table;
                        g_PlayerData[id].playerInfo.seat = pChangeData[i].playerInfo.seat;

                        prevState = g_PlayerData[id].playerInfo.state;
                        if (prevState == STATE_SIT) {
                            g_GameRoomWnd.GamerGetReady(g_PlayerData[id].playerInfo.table, g_PlayerData[id].playerInfo.seat, TRUE);
                        }

                        g_PlayerData[id].playerInfo.state = STATE_READY;
                        break;

                    case STATE_GAMING:
                        TRACE2(_T("Player [ID: %d] - [%s] ==> GAMING\n"), id, g_PlayerData[id].playerInfo.name);

                        g_PlayerData[id].playerInfo.table = pChangeData[i].playerInfo.table;
                        g_PlayerData[id].playerInfo.seat = pChangeData[i].playerInfo.seat;

                        prevState = g_PlayerData[id].playerInfo.state;
                        if (prevState == STATE_READY) {
                            g_GameRoomWnd.TableGameStarted(g_PlayerData[id].playerInfo.table, TRUE);
                        }

                        g_PlayerData[id].playerInfo.state = STATE_GAMING;
                        break;

                    case STATE_LOOKON:
                        TRACE2(_T("Player [ID: %d] - [%s] ==> LOOKON\n"), id, g_PlayerData[id].playerInfo.name);

                        g_PlayerData[id].playerInfo.table = pChangeData[i].playerInfo.table;
                        g_PlayerData[id].playerInfo.seat = pChangeData[i].playerInfo.seat;
                        g_PlayerData[id].playerInfo.state = STATE_LOOKON;

                        g_GameRoomWnd.LookonTakeSeat(id, g_PlayerData[id].playerInfo.table,
                            g_PlayerData[id].playerInfo.seat);
                        break;
                }
                break;
        }
    }

    LocalFree(pChangeData);

    g_GameRoomWnd.UpdateWnd();
    g_UserListWnd.UpdateWnd();
}

void On_TM_GAMER_TAKE_SEAT(WPARAM wParam, LPARAM lParam)
{
    int table = LOWORD(wParam);
    int seat = HIWORD(wParam);
    int id = (int)lParam;

    int myid = GetLocalUserId();

    if (id != myid) { // 别人进入该游戏桌
        g_GameMainPage.GamerTakeSeat(id, seat);
        return;
    }

    // 初始游戏窗口相关数据
    g_GameMainPage.Init(FALSE);
    g_GameMainPage.SetCurentUserSeat(seat);

    // 把自己给加上，因为在服务器1秒定时广播之前，本地数据表中，游戏桌中可能还
    // 没有自己的信息。服务器是这样处理的：它收到GamerTakeSeat或LookonTakeSeat后，
    // 确认可以坐下所选座位，然后通知客户端坐入游戏桌，最后，通知定时广播线程，
    // 某个玩家状态更改为SIT
    g_GameMainPage.m_GameMainWnd.GamerTakeSeat(myid, seat);

    // 设置玩家形象GIF图片
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int gamerid = GetGamerId(table, i);
        if (gamerid != INVALID_USER_ID) {
            if (gamerid != myid) { // added myself already
                g_GameMainPage.m_GameMainWnd.GamerTakeSeat(gamerid, i);

                if (g_GameRoomWnd.IsGamerReady(table, i) == TRUE) {
                    g_GameMainPage.m_GameMainWnd.GamerReady(gamerid, i);
                }
            }
        }
    }

    // 把自己给加上
    g_GameMainPage.ListCtrlAddUser(myid);

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int nUserId = GetGamerId(table, i);
        if (nUserId != INVALID_USER_ID) {
            if (nUserId != myid) { // added myself already
                g_GameMainPage.ListCtrlAddUser(nUserId);

                int* nLookonIds = GetLookonIdsArrayPtr(table, i);
                for (int j = 0; j < MAX_LOOKON_NUM_PER_SEAT; j++) {
                    if (nLookonIds[j] != INVALID_USER_ID) {
                        g_GameMainPage.ListCtrlAddUser(nLookonIds[j]);
                    }
                }
            }
        }
    }

    g_PageTabWnd.InsertGameTabPage();
    g_PageTabWnd.SetCurTabSel(GAME_PAGE_TAB_INDEX);
    OnPageSelChange(g_hMainWnd, GAME_PAGE_TAB_PARAM);
}

void On_TM_LOOKON_TAKE_SEAT(WPARAM wParam, LPARAM lParam)
{
    int table = LOWORD(wParam);
    int seat = HIWORD(wParam);
    int id = (int)lParam;

    int myid = GetLocalUserId();

    if (id != myid) { // 别人进入该游戏桌
        g_GameMainPage.LookonTakeSeat(id, seat);
        return;
    }

    // 服务器已经确认该旁观者可以进入游戏桌，这里将其信息添加到本地信息结构中，
    // 因为后面的操作需要使用这些信息来判断自己是否是旁观者，而服务器此时可能还
    // 没有将这些有效的信息广播到客户端。本地自行更新数据。
    g_PlayerData[myid].playerInfo.table = table;
    g_PlayerData[myid].playerInfo.seat = seat;
    g_PlayerData[myid].playerInfo.state = STATE_LOOKON;

    // 初始游戏窗口相关数据
    g_GameMainPage.Init(TRUE);
    g_GameMainPage.SetCurentUserSeat(seat);

    // 设置玩家形象GIF图片
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int gamerid = GetGamerId(table, i);
        if (gamerid != INVALID_USER_ID) {
            g_GameMainPage.m_GameMainWnd.GamerTakeSeat(gamerid, i);

            if (g_GameRoomWnd.IsGamerReady(table, i) == TRUE) {
                g_GameMainPage.m_GameMainWnd.GamerReady(gamerid, i);
            }
        }
    }

    // 把自己给加入ListCtrl
    g_GameMainPage.ListCtrlAddUser(myid);

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int nUserId = GetGamerId(table, i);
        if (nUserId != INVALID_USER_ID) {
            g_GameMainPage.ListCtrlAddUser(nUserId);

            int* nLookonIds = GetLookonIdsArrayPtr(table, i);
            for (int j = 0; j < MAX_LOOKON_NUM_PER_SEAT; j++) {
                if (nLookonIds[j] != INVALID_USER_ID) {
                    if (nLookonIds[j] != myid) { // added myself already
                        g_GameMainPage.ListCtrlAddUser(nLookonIds[j]);
                    }
                }
            }
        }
    }

    g_PageTabWnd.InsertGameTabPage();
    g_PageTabWnd.SetCurTabSel(GAME_PAGE_TAB_INDEX);
    OnPageSelChange(g_hMainWnd, GAME_PAGE_TAB_PARAM);
}

void On_TM_GAMER_LEAVE_SEAT(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int table = LOWORD(wParam);
    int seat = HIWORD(wParam);

    int nUserId = GetGamerId(table, seat);

    g_GameMainPage.GamerLeaveSeat(nUserId, seat);
}

void On_TM_LOOKON_LEAVE_SEAT(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int table = LOWORD(wParam);
    int seat = HIWORD(wParam);
    int nUserId = (int)lParam;

    UNREFERENCED_PARAMETER(table);

    g_GameMainPage.LookonLeaveSeat(nUserId, seat);
}

void On_TM_GAMER_READY(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int table = LOWORD(wParam);
    int seat = HIWORD(wParam);
    int nUserId = GetGamerId(table, seat);

    g_GameMainPage.GamerReady(nUserId, seat);
}

void On_TM_DISTRIBUTE_POKER(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    DISTRIBUTE_POKER* lpDistrPoker = (DISTRIBUTE_POKER*)lParam;
    assert(lpDistrPoker != NULL);

    g_GameMainPage.m_GameMainWnd.DistributePokerCards(0, lpDistrPoker->pokerSeat0, lpDistrPoker->numSeat0);
    g_GameMainPage.m_GameMainWnd.DistributePokerCards(1, lpDistrPoker->pokerSeat1, lpDistrPoker->numSeat1);
    g_GameMainPage.m_GameMainWnd.DistributePokerCards(2, lpDistrPoker->pokerSeat2, lpDistrPoker->numSeat2);

    g_GameMainPage.m_GameMainWnd.SetUnderPokerCards(lpDistrPoker->underCards, UNDER_CARDS_NUM);

    LocalFree(lpDistrPoker);
}

void On_TM_SVR_REQ_VOTE_LORD(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int seat = LOWORD(wParam);
    int score = HIWORD(wParam);
    g_GameMainPage.m_GameMainWnd.ReqVoteLord(seat, score);
}

void On_TM_SVR_NOTIFY_VOTE_LORD(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int seat = LOWORD(wParam);
    int score = HIWORD(wParam);
    g_GameMainPage.m_GameMainWnd.GamerVoteLord(seat, score);
}

void On_TM_VOTE_LORD_FINISH(WPARAM wParam, LPARAM lParam)
{
    int seat = LOWORD(wParam);
    int score = HIWORD(wParam);
    BOOL bVote = (BOOL)lParam;

    g_GameMainPage.m_GameMainWnd.VoteLordFinish(bVote, seat, score);
}

void On_TM_SVR_REQ_OUTPUT_CARD(WPARAM wParam, LPARAM lParam)
{
    int seat = LOWORD(wParam);
    int type = HIWORD(wParam);
    int value = LOWORD(lParam);
    int num = HIWORD(lParam);

    POKER_PROPERTY pp;
    pp.num = num;
    pp.type = (POKER_TYPE)type;
    pp.value = value;

    g_GameMainPage.m_GameMainWnd.ReqOutputCard(seat, &pp);
}

void On_TM_SVR_NOTIFY_OUTPUT_CARD(WPARAM wParam, LPARAM lParam)
{
    int seat = LOWORD(wParam);
    int num = HIWORD(wParam);
    OUTPUT_POKER* lpOutputPoker = (OUTPUT_POKER*)lParam;

    if (num <= 0) {
        g_GameMainPage.m_GameMainWnd.OutputCard(seat, num, NULL);
    } else {
        g_GameMainPage.m_GameMainWnd.OutputCard(seat, num, lpOutputPoker->poker);
        LocalFree(lpOutputPoker);
    }
}

void On_TM_SVR_NOTIFY_GAME_OVER(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    GAME_RESULT* lpResult = (GAME_RESULT*)lParam;
    assert(lpResult != NULL);

    g_GameMainPage.m_GameMainWnd.GameOver(lpResult);
    LocalFree(lpResult);
}

void On_TM_GAMER_CHATTING(WPARAM wParam, LPARAM lParam)
{
    int seat = LOWORD(wParam);
    assert(!IS_INVALID_SEAT(seat));

    BOOL bPredefinedMsg = (BOOL)(HIWORD(wParam));

    if (bPredefinedMsg == TRUE) {
        int nMsgIndex = (int)lParam;
        g_GameMainPage.m_GameMainWnd.OutputChatText(seat, nMsgIndex);
    } else {
        TCHAR* lpszChatText = (TCHAR*)lParam;
        if (lpszChatText != NULL) {
            g_GameMainPage.m_GameMainWnd.OutputChatText(seat, lpszChatText);
            LocalFree(lpszChatText);
        }
    }
}

void On_TM_GAMER_DELEGATED(WPARAM wParam, LPARAM lParam)
{
    int seat = (int)wParam;
    BOOL bDelegated = (BOOL)lParam;

    g_GameMainPage.m_GameMainWnd.GamerDelegate(seat, bDelegated);
}

void On_TM_GAMER_DISCONNECTED(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    int seat = (int)wParam;

    g_GameMainPage.m_GameMainWnd.GamerDisconnected(seat);
}

