//
// NetProc.cpp
//
//  处理网络消息并接受客户端的连接
//
// Modification:
//
//  2009-06-05  Change the calling point of DeallocID()
//      The ID is used to identify a client or a gamer. It's critical to handle 
//      ID resource correctly for the server since it is used to synchronize the
//      data structure of server with client's and do many other operations.
//      Here, there's an issue, say, one client connects to the server and been 
//      allocated an ID with 0, and then he leaves. So, the server will reclaim
//      the ID and tell the OnTimeBroadcast thread that someone's left with 
//      EVT_LEAVE flag. Since the broadcast thread will be scheduled each second. 
//      While, before that happens, another client logons the server, and with 
//      ID 0 assigned. So this client might receive a message of [ID:0 EVT_LEAVE] 
//      from OnTimeBroadcast sometime and will receive a message [ID:0 EVT_ENTER] 
//      about himself at the next second. This may not be harmful for the client,
//      but i don't think it's something good. 
//      From this point, i decide to make a change that there will only one place
//      to call DeallocID(), that's within OnTimeBroadcast thread when it processes
//      a node with EVT_LEAVE flagged.
//
// 2009-06-12 Added bConnLost member to CONNECTION_INFO, and closed socket when DeallocID
//      For instance, three gamers were in game, and two of them disconnected 
//      unexpectedly. Since the game had started, the server would output cards
//      for the disconnected gamer. Before the game was over, server should not 
//      remove the info of these disconnected players, or label them unusable. 
//      Instead, it should keep them valid until the game was over. Otherwise, 
//      other new login clients would see that one guy was just gaming with 
//      nobody in the game room.
//
// 2010-05-22 Added IP address selection dialog on application startup
//
//
#include "stdafx.h"
#include <Wspiapi.h> // BACKWARD-COMPATIBILITY: for `addrinfo' be used on win2k
#include "resource.h"
#include "Crc.h"
#include "NetProc.h"
#include "GameProc.h"

extern DWORD g_nGameThreadIds[];
extern DWORD g_nMainThreadId;

// 服务器的套接字
SOCKET g_ServerSocket = INVALID_SOCKET;
volatile BOOL g_bServerWillShutdown = FALSE;

DWORD   g_dwServerIP = 0;   // 服务器使用的IP，网络序
USHORT  g_nServerPort = 0;  // 服务器使用的端口，网络序

// 当前连接数量
volatile int g_nCurConnectionNum = 0;

// ID资源
CONNECTION_ID_RES g_ConnectionIdRes = { 0 };
CRITICAL_SECTION g_csConnectionNum = { 0 };

// 游戏桌信息
GAME_TABLE_INFO g_GameTableInfo[GAME_TABLE_NUM] = { 0 };

// 客户端连接（玩家）相关信息
CONNECTION_INFO g_ConnectionInfo[MAX_CONNECTION_NUM] = { 0 };

// 记录玩家进入或离开房间，然后定时将这些信息广播发送给所有在线玩家
// 链表以队列方式工作。在链表结尾进行添加操作，从链表头开始读取和删除操作
USER_STATE_CHANGE_LIST g_UserStateChangeList = { 0 };
CRITICAL_SECTION g_csUserStateChangeList = { 0 };

HANDLE g_hConnectionThread[MAX_CONNECTION_NUM] = { 0 };
HANDLE g_hListenThread = NULL;
HANDLE g_hOnTimeBroadcastThread = NULL;
HANDLE g_mutexOnTimeBroadcast = NULL;

// 客户端主动断开连接而非服务器主动断开连接时，关闭连接线程句柄
void CloseConnectionThreadHandle(HANDLE hThread)
{
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
}

// 停止网络服务
void StopNetService(void)
{
    g_bServerWillShutdown = TRUE;

    shutdown(g_ServerSocket, SD_BOTH);
    closesocket(g_ServerSocket);

    // 退出等待连接线程
    WaitForSingleObject(g_hListenThread, INFINITE);
    CloseHandle(g_hListenThread);

    // 释放Mutex，退出定时广播线程
    ReleaseMutex(g_mutexOnTimeBroadcast);
    WaitForSingleObject(g_mutexOnTimeBroadcast, INFINITE);
    CloseHandle(g_mutexOnTimeBroadcast);
    CloseHandle(g_hOnTimeBroadcastThread);

    // 关闭SOCKET，退出连接线程
    for (int id = 0; id < MAX_CONNECTION_NUM; id++) {
        if (M_CONNECT_SOCK(id) != INVALID_SOCKET) {
            shutdown(M_CONNECT_SOCK(id), SD_BOTH);
            closesocket(M_CONNECT_SOCK(id));
        }

        if (g_hConnectionThread[id] != NULL) {
            WaitForSingleObject(g_hConnectionThread[id], INFINITE);
            CloseHandle(g_hConnectionThread[id]);
        }
    }

    // 清除Winsock系统资源
    WSACleanup();

    // 释放资源
    ReleaseNetServiceRes();

    // 保存服务器本次运行所使用的端口号
    WriteServerPortToIni();
}

// 初始化WinSock，并创建服务器连接等待线程
BOOL StartNetService(void)
{
    DWORD nThreadID = 0;
    if (InitNetServiceRes() == FALSE) {
        return FALSE;
    }

    if (InitWinSocket() == FALSE) {
        ReleaseNetServiceRes();
        return FALSE;
    }

    // 创建等待连接线程
    g_hListenThread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)WaitForConnectionThreadProc, (LPVOID)NULL, 0, &nThreadID);

    // 创建定时广播线程
    g_hOnTimeBroadcastThread = CreateThread(NULL, 0, 
        (LPTHREAD_START_ROUTINE)OnTimeBroadcastThreadProc, (LPVOID)NULL, 0, &nThreadID);

    return TRUE;
}

// 释放网络服务所占用的相关资源
static void ReleaseNetServiceRes(void)
{
    EnterCriticalSection(&g_csUserStateChangeList);
    if (g_UserStateChangeList.lpFirstNode != NULL) {
        USER_STATE_CHANGE* pNode = NULL;
        USER_STATE_CHANGE* pTemp = NULL;

        // 清除链表
        pNode = g_UserStateChangeList.lpFirstNode;
        while (pNode != NULL) {
            pTemp = pNode;
            pNode = pNode->next;
            LocalFree(pTemp);
        }
        g_UserStateChangeList.lpFirstNode = NULL;
        g_UserStateChangeList.lpLastNode = NULL;
    }
    LeaveCriticalSection(&g_csUserStateChangeList);

    DeleteCriticalSection(&g_csConnectionNum);
    DeleteCriticalSection(&g_csUserStateChangeList);
}

// 初始化网络服务相关数据结构
static BOOL InitNetServiceRes(void)
{
    g_bServerWillShutdown = FALSE;
    g_nCurConnectionNum = 0;

    // 所有数据结构复位为0
    ZeroMemory(&g_ConnectionIdRes, sizeof(CONNECTION_ID_RES));
    ZeroMemory(&g_UserStateChangeList, sizeof(USER_STATE_CHANGE_LIST));
    ZeroMemory(&g_ConnectionInfo, sizeof(CONNECTION_INFO) * MAX_CONNECTION_NUM);
    ZeroMemory(&g_GameTableInfo, sizeof(GAME_TABLE_INFO) * GAME_TABLE_NUM);

    // 初始化连接信息默认值
    for (int id = 0; id < MAX_CONNECTION_NUM; id++) {
        M_CONNECT_SOCK(id) = INVALID_SOCKET;
        M_CONNECT_TABLE(id) = INVALID_TABLE;
        M_CONNECT_SEAT(id) = INVALID_SEAT;
        M_CONNECT_STATE(id) = STATE_IDLE;
        M_CONNECT_ALLOW_LOOKON(id) = TRUE;
    }

    // 初始化游戏桌信息默认值
    for (int table = 0; table < GAME_TABLE_NUM; table++) {
        for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
            M_TABLE_PLAYER_ID(table, seat) = INVALID_USER_ID;

            for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
                M_TABLE_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
            }
        }
    }

    // 创建为被占用的Mutex，仅为定时广播线程提供定时器的机制
    g_mutexOnTimeBroadcast = CreateMutex(NULL, TRUE, NULL);
    if (g_mutexOnTimeBroadcast == NULL) {
        return FALSE;
    }

    InitializeCriticalSection(&g_csConnectionNum);
    InitializeCriticalSection(&g_csUserStateChangeList);

    return TRUE;
}

// ON Init IP Selection Dialog
INT_PTR InitIpSelDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    HWND hIpList = NULL;

    // 给对话框标题栏添加图标
    HICON hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DDZSERVER));
    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hicon);

    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
    hIpList = GetDlgItem(hDlg, IDC_LIST_ADAPTER);
    ListView_SetExtendedListViewStyle(hIpList, LVS_EX_FULLROWSELECT);

    // insert column for list-view ctrl 
    struct _lv_colum_data {
        LPTSTR  label;
        int     width;
        int     format;
    } lvcolumn[] = {
        { _T("IP地址"), 110, LVCFMT_LEFT },
        { _T("子网掩码"), 110, LVCFMT_LEFT },
        { _T("描述"), 160, LVCFMT_LEFT },
    };

    for (int i = 0; i < (sizeof(lvcolumn) / sizeof(lvcolumn[0])); i++) {
        LVCOLUMN lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.pszText = lvcolumn[i].label;
        lvc.cchTextMax = _tcslen(lvcolumn[i].label);
        lvc.cx = lvcolumn[i].width;
        lvc.fmt = lvcolumn[i].format;
        ListView_InsertColumn(hIpList, i, &lvc);
    }

    // init data for list-view ctrl
    IP_ADAPTER_INFO* pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;

    // query the size needed
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW) {
        return (INT_PTR)EndDialog(hDlg, IDCANCEL);
    }

    //assert(ulOutBufLen > 0);
    pAdapterInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, ulOutBufLen); 
    if (pAdapterInfo == NULL) {
        return (INT_PTR)EndDialog(hDlg, IDCANCEL);
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != NO_ERROR) {
        GlobalFree(pAdapterInfo);
        return (INT_PTR)EndDialog(hDlg, IDCANCEL);
    }

    IP_ADAPTER_INFO* pAdapter = NULL;
    IP_ADDR_STRING* pIpAddrStr = NULL;
    int nItemIndex = 0;

    // use ANSI type string functions for easy operation
    for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) { // iterate all adapters
        for (pIpAddrStr = &pAdapter->IpAddressList; pIpAddrStr; pIpAddrStr = pIpAddrStr->Next) { //iterate all ip within the adapter
            // get network-bit-order IP
            DWORD dwIP = inet_addr(pIpAddrStr->IpAddress.String);
            if (dwIP == 0) {
                continue;
            }

            // insert item
            LVITEMA lvi = { 0 };
            lvi.iItem = nItemIndex;
            SendMessageA(hIpList, LVM_INSERTITEMA, 0, (LPARAM)&lvi);

            // set item text
            lvi.mask = LVIF_TEXT;

            lvi.iSubItem = 0;
            lvi.pszText = pIpAddrStr->IpAddress.String;
            SendMessageA(hIpList, LVM_SETITEMTEXTA, nItemIndex, (LPARAM)&lvi);

            lvi.iSubItem = 1;
            lvi.pszText = pIpAddrStr->IpMask.String;
            SendMessageA(hIpList, LVM_SETITEMTEXTA, nItemIndex, (LPARAM)&lvi);

            lvi.iSubItem = 2;
            lvi.pszText = pAdapter->Description;
            SendMessageA(hIpList, LVM_SETITEMTEXTA, nItemIndex, (LPARAM)&lvi);

            nItemIndex++;
        }
    }

    GlobalFree(pAdapterInfo);
    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
    return (INT_PTR)FALSE;
}

INT_PTR FinishIpSel(HWND hDlg, int iItem)
{
    // use ANSI type functions, so we don't need to convert ip string.
    IP_ADDRESS_STRING ipstr = { 0 };
    LVITEMA lvi = { 0 };
    lvi.iItem = iItem;
    lvi.iSubItem = 0; // ip addr column
    lvi.pszText = &ipstr.String[0];
    lvi.cchTextMax = sizeof(ipstr.String) / sizeof(ipstr.String[0]);
    SendMessageA(GetDlgItem(hDlg, IDC_LIST_ADAPTER), LVM_GETITEMTEXTA,
        (WPARAM)iItem, (LPARAM)&lvi);

    DWORD dwIpSelected = inet_addr(ipstr.String);
    if (dwIpSelected > 0) {
        DWORD* lpRetIp = (DWORD*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
        if (lpRetIp) {
            *lpRetIp = dwIpSelected;
            return EndDialog(hDlg, IDOK);
        }
    }

    return (INT_PTR)FALSE;
}

INT_PTR IpListNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    LPNMHDR pNotifyHdr = (LPNMHDR)lParam;
    if (pNotifyHdr->idFrom != IDC_LIST_ADAPTER) {
        return (INT_PTR)FALSE;
    }

    LPNMITEMACTIVATE pNMItemSel = (LPNMITEMACTIVATE)lParam;
    switch (pNotifyHdr->code) {
    case NM_CLICK:
        if (pNMItemSel->iItem >= 0) {
            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        }
        break;

    case NM_DBLCLK:
        if (pNMItemSel->iItem >= 0) {
            return FinishIpSel(hDlg, pNMItemSel->iItem);
        }
        break;
    }

    return (INT_PTR)FALSE;
}

// 服务器IP地址选择对话框的消息处理程序。
INT_PTR CALLBACK IpSelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hIpList = NULL;
    int nItemCount = 0;
    int nItemSel = -1;
    DWORD dwState = 0;

    switch (message) {
    case WM_INITDIALOG:
        return InitIpSelDialog(hDlg, wParam, lParam);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            return (INT_PTR)EndDialog(hDlg, IDCANCEL);
        }

        if (LOWORD(wParam) == IDOK) {
            hIpList = GetDlgItem(hDlg, IDC_LIST_ADAPTER);
            nItemCount = SendMessage(hIpList, LVM_GETITEMCOUNT, 0, 0);
            for (int i = 0; i < nItemCount; i++) {
                dwState = SendMessage(hIpList, LVM_GETITEMSTATE, i, LVIS_SELECTED);
                if (dwState & LVIS_SELECTED) {
                    nItemSel = i;
                    break;
                }
            }

            if (nItemSel >= 0) {
                return FinishIpSel(hDlg, nItemSel);
            }
        }
        break;

    case WM_NOTIFY:
        return IpListNotify(hDlg, wParam, lParam);

    case WM_DESTROY:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
        break;
    }

    return (INT_PTR)FALSE;
}

// 设置服务器使用的IP
BOOL InitServerIP(void)
{
    DWORD dwIpArray[8] = { 0 };
    int nIpCount = 0;

    GetHostIP(dwIpArray, sizeof(dwIpArray) / sizeof(dwIpArray[0]), &nIpCount);
    if (nIpCount <= 0) {
        return FALSE;
    }

    // 使用主机上第一个IP
    g_dwServerIP = dwIpArray[0];

    // Shining added ip addresses selection dialog, 2010-05
    if (nIpCount > 1) {
        extern HINSTANCE g_hAppInstance;
        DWORD dwIpSelected = 0;
        int ret = DialogBoxParam(g_hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_IP_SEL),
            NULL, IpSelProc, (LPARAM)&dwIpSelected);
        if (ret != IDOK) {
            return FALSE;
        }

        g_dwServerIP = dwIpSelected;
    }

    return TRUE;
}

// 设置服务器使用的端口
BOOL InitServerPort(void)
{
    TCHAR szIniFile[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, szIniFile, MAX_PATH);
    PathRemoveFileSpec(szIniFile);
    _tcscat_s(szIniFile, MAX_PATH, _T("\\"));
    _tcscat_s(szIniFile, MAX_PATH, SERVER_CONF_FILE);

    UINT port = GetPrivateProfileInt(SVR_INI_SEC_SERVER, SVR_INI_KEY_PORT, DDZSERVER_PORT, szIniFile);

    g_nServerPort = htons((USHORT)port);
    return TRUE;
}

// 保存服务器本次使用的端口号
void WriteServerPortToIni(void)
{
    TCHAR szIniFile[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, szIniFile, MAX_PATH);
    PathRemoveFileSpec(szIniFile);
    _tcscat_s(szIniFile, MAX_PATH, _T("\\"));
    _tcscat_s(szIniFile, MAX_PATH, SERVER_CONF_FILE);

    TCHAR szPort[16] = { 0 };
    USHORT port = ntohs(g_nServerPort);

    _stprintf_s(szPort, sizeof(szPort) / sizeof(szPort[0]), _T("%d"), port);

    WritePrivateProfileString(SVR_INI_SEC_SERVER, SVR_INI_KEY_PORT, szPort, szIniFile);
}

// 初始化Winsocket
static BOOL InitWinSocket(void)
{
    WORD wVersion;
    WSADATA wsaData;

    wVersion = MAKEWORD(2, 2);
    if (WSAStartup(wVersion, &wsaData) != NO_ERROR) {
        return FALSE;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        WSACleanup();
        return FALSE; 
    }

    g_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_ServerSocket == INVALID_SOCKET) {
        WSACleanup();
        return FALSE; 
    }

    if (!InitServerIP() || !InitServerPort()) {
        WSACleanup();
        return FALSE;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = g_dwServerIP;
    service.sin_port = g_nServerPort;

    if (bind(g_ServerSocket, (SOCKADDR*)&service, sizeof(SOCKADDR)) != NO_ERROR) {
        closesocket(g_ServerSocket);
        WSACleanup();
        return FALSE;
    }

    if (listen(g_ServerSocket, 1) != NO_ERROR) {
        closesocket(g_ServerSocket);
        WSACleanup();
        return FALSE;
    }

    return TRUE;
}

// 获取主机IP地址（32-bit整数，网络序），返回参数count 表示主机有几个IP
void GetHostIP(DWORD ipArray[], int num, int* count)
{
    int ret = SOCKET_ERROR;
    char nodename[64] = { 0 };
    char servname[] = "2048";
    struct addrinfo aiHints = { 0 };
    struct addrinfo* ai = NULL;
    struct addrinfo* aiLists = NULL;
    struct sockaddr_in* skaddr = NULL;

    // 初始化返回的IP数量
    int n = 0;
    *count = 0;

    aiHints.ai_family = AF_INET;
    aiHints.ai_socktype = SOCK_STREAM;
    aiHints.ai_protocol = IPPROTO_TCP;

    ret = gethostname(nodename, sizeof(nodename) / sizeof(nodename[0]));
    if (ret != NO_ERROR) {
        return;
    }

    ret = getaddrinfo(nodename, servname, &aiHints, &aiLists);
    if (ret != NO_ERROR) {
        return;
    }

    for (ai = aiLists; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET) {
            continue;
        }

        skaddr = (struct sockaddr_in*)ai->ai_addr;
        if (skaddr != NULL) {
            ipArray[n++] = skaddr->sin_addr.s_addr;
            if (n >= num) {
                break;
            }
        }
    }

    if (aiLists != NULL) {
        freeaddrinfo(aiLists);
        aiLists = NULL;
    }

    *count = n;
}

//
// 为每个连接分配一个的唯一ID，必须确保连接数量没超过最大允许值
//
// 服务器侦听线程接受客户端连接时，会调用该函数为客户端分配ID
// 访问共享资源g_ConnectionIdRes，调用时需要加锁保护
//
// 调用者需判断返回值是否为INVALID_USER_ID，若是，则表示分配ID失败
//
int AllocID(void)
{
    static int ELEM_BITS = sizeof(g_ConnectionIdRes.idResource[0]) * 8;

    for (int id = 0; id < MAX_CONNECTION_NUM; id++) {
        if ((g_ConnectionIdRes.idResource[id / ELEM_BITS] & (1 << (id % ELEM_BITS))) == 0) {
            g_ConnectionIdRes.idResource[id / ELEM_BITS] |= (1 << (id % ELEM_BITS));
            return id;
        }
    }

    WriteLog(LOG_ERROR, _T("ID资源耗尽 [Check your program!]"));
    return INVALID_USER_ID;
}

//
// 回收ID资源
// 
// 连接线程退出时，或游戏工作线程结束游戏时，会调用该函数以回收ID，表示客户端断开连接
// 访问共享资源g_ConnectionIdRes，调用者负责加锁保护
//
void DeallocID(int id)
{
    static int ELEM_BITS = sizeof(g_ConnectionIdRes.idResource[0]) * 8;

    if ((id >= 0) && (id < MAX_CONNECTION_NUM)) {
        g_ConnectionIdRes.idResource[id / ELEM_BITS] &= ~(1 << (id % ELEM_BITS));
    } else {
        WriteLog(LOG_ERROR, _T("无法回收非法ID: %d [Check your program!]"), id);
    }
}

// 等待连接线程的入口函数
// 该线程的工作是等待连接，若到来一个连接，就创建一个连接线程
DWORD WaitForConnectionThreadProc(LPVOID lpData)
{
    UNREFERENCED_PARAMETER(lpData);

    int id;
    DWORD nThreadID;
    SOCKET sock = INVALID_SOCKET;

    int len = 0;
    SOCKADDR_IN addr = { 0 };

    for (;;) {
        sock = INVALID_SOCKET;
        while (sock == INVALID_SOCKET) {
            if (g_bServerWillShutdown == TRUE) { return 0; }

            len = sizeof(SOCKADDR_IN);
            ZeroMemory(&addr, len);

            // 等待客户端来连接
            sock = accept(g_ServerSocket, (SOCKADDR*)&addr, &len);
        }

        EnterCriticalSection(&g_csConnectionNum);

        // 限制客户连接数量
        if (g_nCurConnectionNum >= MAX_CONNECTION_NUM) {
            shutdown(sock, SD_BOTH);
            closesocket(sock);
            WriteLog(LOG_WARN, _T("客户连接数达到限制值: %d，拒绝远程连接: [%d.%d.%d.%d: %d]"),
                g_csConnectionNum, (BYTE)addr.sin_addr.s_net, (BYTE)addr.sin_addr.s_host,
                (BYTE)addr.sin_addr.s_imp, (BYTE)addr.sin_addr.s_impno, ntohs(addr.sin_port));

            LeaveCriticalSection(&g_csConnectionNum);
            continue;
        }

        id = AllocID();
        if (IS_INVALID_USER_ID(id)) {
            shutdown(sock, SD_BOTH);
            closesocket(sock);

            LeaveCriticalSection(&g_csConnectionNum);
            continue;
        }

        g_nCurConnectionNum++;
        LeaveCriticalSection(&g_csConnectionNum);

        ZeroMemory(&g_ConnectionInfo[id], sizeof(CONNECTION_INFO));
        M_CONNECT_SOCK(id) = sock;
        M_CONNECT_STATE(id) = STATE_IDLE;
        M_CONNECT_TABLE(id) = INVALID_TABLE;
        M_CONNECT_SEAT(id) = INVALID_SEAT;
        M_CONNECT_ALLOW_LOOKON(id) = TRUE;
        GetLocalTime(&M_CONNECT_TIME(id));

        WriteLog(LOG_INFO, _T("接受客户端 [%d.%d.%d.%d: %d] 连接，分配ID: %d"), 
            (BYTE)addr.sin_addr.s_net, (BYTE)addr.sin_addr.s_host,
            (BYTE)addr.sin_addr.s_imp, (BYTE)addr.sin_addr.s_impno,
            ntohs(addr.sin_port), id);

        // 为该连接创建一个连接线程
        g_hConnectionThread[id] = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)ConnectionThreadProc, (LPVOID)id, 0, &nThreadID);
    }
}

// 连接线程的入口函数
// 该线程的工作是与客户端通信（接收消息，处理消息，反馈消息）
DWORD ConnectionThreadProc(LPVOID lpData)
{
    char* buf = NULL;
    int nRecv = 0;

    int id = (int)lpData;
    int len = sizeof(SOCKADDR_IN);
    SOCKADDR_IN addr = { 0 };
    SOCKET sock = INVALID_SOCKET;

    sock = M_CONNECT_SOCK(id);
    getpeername(sock, (SOCKADDR*)&addr, &len);

    buf = (char*)LocalAlloc(LMEM_FIXED, CONNECTION_RECV_BUF_SZ);
    if (buf == NULL) {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        M_CONNECT_SOCK(id) = INVALID_SOCKET;

        //
        // 直接回收ID，并不需要定时广播线程去回收
        // 因为本线程将返回，不会接收客户端发送来的 REQ_ROOM_INFO，即不会通知
        // 定时广播线程有玩家 EVT_ENTER。
        //
        EnterCriticalSection(&g_csConnectionNum);
        WriteLog(LOG_INFO, _T("回收ID: %d 并关闭套接字。[分配接收数据的缓冲区失败，服务器断开与客户端的连接]"), id);
        DeallocID(id);
        g_nCurConnectionNum--;
        LeaveCriticalSection(&g_csConnectionNum);

        // 通知主线程关闭该连接线程句柄
        PostThreadMessage(g_nMainThreadId, TM_CLIENT_DISCONNECT, (WPARAM)g_hConnectionThread[id], 0);
        g_hConnectionThread[id] = NULL;

        return 0;
    }

    for (;;) {
        nRecv = recv(sock, buf, CONNECTION_RECV_BUF_SZ, 0);

        if (nRecv > 0) {
#if 0
#define DBG_RCV_SZ      64
            TCHAR strData[DBG_RCV_SZ];
            int len = 0;

            for (int i = 0; i < nRecv; i++) {
                if (len >= DBG_RCV_SZ - 8) { break; }
                len += _stprintf_s(strData + len, sizeof(strData) / sizeof(strData[0]) - len,
                    _T("0x%02X "), (BYTE)buf[i]);
            }

            WriteLog(LOG_DEBUG, _T("收到数据: [%4d] %s"), nRecv, strData);
#endif
            // 处理接收到的消息
            OnMessage(id, buf, nRecv);
        }
        
        else if (nRecv == 0) {
            WriteLog(LOG_INFO, _T("远程主机:[%d.%d.%d.%d: %d] 主动断开连接"),
                (BYTE)addr.sin_addr.s_net, (BYTE)addr.sin_addr.s_host,
                (BYTE)addr.sin_addr.s_imp, (BYTE)addr.sin_addr.s_impno, ntohs(addr.sin_port));
            break;
        }
        
        else {
            WriteLog(LOG_WARN, _T("接收数据出错: [%d]。或远程主机 ID: [%d] 可能意外断线"),
                WSAGetLastError(), id);
            break;
        }
    }

    LocalFree(buf);

    if (g_bServerWillShutdown == TRUE) {
        // 服务器退出，因而关闭连接。主线程将负责关闭Socket并关闭该线程句柄
        return 0;
    }

    // 标识该客户端断开连接
    M_CONNECT_LOST(id) = TRUE;

    //
    // 连接断开后，通知游戏工作线程，做有关清理工作，即擦除TABLE_INFO结构中相应
    // 座位的玩家ID。如果玩家正在进行游戏，则需等到游戏结束后，才能擦除；否则，
    // 别的玩家就会进入该座位
    //
    if (M_CONNECT_STATE(id) != STATE_IDLE) {
        int table = M_CONNECT_TABLE(id);
        int seat = M_CONNECT_SEAT(id);
        PostThreadMessage(g_nGameThreadIds[table], TM_CONNECTION_LOST, (WPARAM)id, (LPARAM)seat);
    } else {
        WriteLog(LOG_INFO, _T("ID: %d 将被回收。[空闲玩家断开连接]"), id);
        AddUserStateChangeEvent(id, EVT_LEAVE, (PLAYER_STATE)0, FALSE);
    }

    //
    // 该线程即将退出，其线程句柄需要被关闭。
    // 如果是客户端断开连接，则寄送消息通知主线程关闭该连接线程句柄，否则，
    // 表示服务器主动退出程序，服务器程序退出时会关闭该连接线程句柄。
    //
    if (g_bServerWillShutdown != TRUE) {
        PostThreadMessage(g_nMainThreadId, TM_CLIENT_DISCONNECT, (WPARAM)g_hConnectionThread[id], 0);
        g_hConnectionThread[id] = NULL;
    }

    return 0;
}

// 定时广播线程的入口函数
// 该线程的工作是定时将进入或离开房间，及状态变化的玩家信息发送给所有在线客户端
DWORD OnTimeBroadcastThreadProc(LPVOID lpData)
{
    UNREFERENCED_PARAMETER(lpData);

#define MAX_SEND_DATA_LEN   4096
    char* data = (char*)LocalAlloc(LMEM_FIXED, MAX_SEND_DATA_LEN);
    if (data == NULL) {
        WriteLog(LOG_ERROR, _T("定时广播线程分配发送数据的内存失败，线程将退出"));
        return 0;
    }

    while (WaitForSingleObject(g_mutexOnTimeBroadcast, BROADCAST_TIME_INTERVAL) != WAIT_OBJECT_0) {
        EnterCriticalSection(&g_csUserStateChangeList);

        OnTimeBuildMessageAndBroadcast(data, MAX_SEND_DATA_LEN);

        LeaveCriticalSection(&g_csUserStateChangeList);
    }

    ReleaseMutex(g_mutexOnTimeBroadcast);

    LocalFree(data);
    return 0;
}

// 定时构造玩家状态变化的消息，并广播出去
//
// Param:
//      mem  - memory for fill in the message data
//      len  - length of the memory
//
static void OnTimeBuildMessageAndBroadcast(char* mem, int len)
{
    int id = INVALID_USER_ID;
    BYTE* data = (BYTE*)mem;

    USER_STATE_CHANGE* pNode = NULL;
    USER_STATE_CHANGE* pTemp = NULL;

    // 记录本次广播中，包含的节点数量
    int nodecount = 0;

    // 记录哪些玩家退出，然后回收其ID
    static int s_LeftUserIds[MAX_CONNECTION_NUM];
    int nLeftUserCount = 0;

    // 第0字节为消息码，第1，2字节为消息体长度，第3～6字节为服务器当前状态，
    // 第7、8字节为发送的节点数量
    int offset = 9;

    pNode = g_UserStateChangeList.lpFirstNode;
    while (pNode != NULL) {
        pTemp = pNode;
        id = pTemp->id;

        // 填充ID: 16-bit 
        data[offset + 0] = (BYTE)(id >> 8);
        data[offset + 1] = (BYTE)(id >> 0);
        offset += 2;

        if (pTemp->evt == EVT_CHGSTATE) {
            // 状态类型
            data[offset] = (BYTE)((int)EVT_CHGSTATE);
            offset++;

            // 玩家状态
            data[offset] = (BYTE)pTemp->newState;
            offset++;

            // 如果玩家处于空闲状态，则用一个字节的最高位表示他是否允许旁观
            // 如果玩家不是处于空闲状态，则用一个字节的最高位表示他是否允许旁观，
            // 低4位表示所在椅子号，再加一字节表示他所在的桌子号
            data[offset] = 0;
            if (pTemp->newState == STATE_IDLE) {
                data[offset] = M_CONNECT_ALLOW_LOOKON(id) ? 0x80 : 0x00;
                offset++;
            } else {
                data[offset] = M_CONNECT_ALLOW_LOOKON(id) ? 0x80 : 0x00;
                data[offset] |= (BYTE)(M_CONNECT_SEAT(id) & 0x0F);
                offset++;

                data[offset] = (BYTE)(M_CONNECT_TABLE(id));
                offset++;
            }

            if (pTemp->bContainStatistics == FALSE) { // 是否需要携带玩家统计信息
                data[offset] = 0;
                offset++;
            } else {
                data[offset] = 0x80;
                offset++;

                // 玩家的积分等统计信息
                data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 24);
                data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 16);
                data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 8);
                data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 0);
                offset += 4;

                data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 24);
                data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 16);
                data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 8);
                data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 0);
                offset += 4;

                data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 24);
                data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 16);
                data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 8);
                data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 0);
                offset += 4;

                data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 24);
                data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 16);
                data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 8);
                data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 0);
                offset += 4;
            }
        } else if (pTemp->evt == EVT_ENTER) {
            // 状态类型
            data[offset] = (BYTE)((int)EVT_ENTER);
            offset++;

            // 名字长度，1字节，先保存其下标，等会再填入值
            int name_len_offset = offset;
            offset++;

            // 填充名字
            _tcscpy_s((TCHAR*)(data + offset), MAX_USER_NAME_LEN, M_CONNECT_PLAYER_NAME(id));
            int name_len = _tcslen(M_CONNECT_PLAYER_NAME(id)) * sizeof(TCHAR);
            offset += name_len;

            // 填充名字长度（字节数)
            data[name_len_offset] = (BYTE)name_len;

            // 第7-bit表示性别，6~4-bit为状态，3~0-bit为头像索引
            data[offset] = 0;
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_GENDER(id)) << 7);
            data[offset] |= (BYTE)((int)(STATE_IDLE) << 4);
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_IMG_IDX(id) & 0x0F));
            offset++;
        } else if (pTemp->evt == EVT_LEAVE) {
            // 状态类型
            data[offset] = (BYTE)((int)EVT_LEAVE);
            offset++;

            s_LeftUserIds[nLeftUserCount++] = id;
        }

        // 读完一个结点，就删除一个结点
        pNode = pNode->next;
        g_UserStateChangeList.lpFirstNode = pNode;
        LocalFree(pTemp);

        nodecount++;

        if (offset > len - (MAX_USER_NAME_LEN + 8)) {
            break;
        }
    }

    if (nodecount > 0) { // 有待发的数据
        // 填充消息码
        data[0] = NMSG_PLAYER_CHANGE_STATE;

        // 填充消息体长度，除去作为消息码的首字节及消息体长度字段两字节
        data[1] = (BYTE)((offset - 3) >> 8);
        data[2] = (BYTE)((offset - 3) >> 0);

        // 填充服务器当前负载状况
        //EnterCriticalSection(&g_csConnectionNum);
        data[3] = (BYTE)(g_nCurConnectionNum >> 8);
        data[4] = (BYTE)(g_nCurConnectionNum >> 0);
        //LeaveCriticalSection(&g_csConnectionNum);

        int max_conn_num = MAX_CONNECTION_NUM;
        data[5] = (BYTE)(max_conn_num >> 8);
        data[6] = (BYTE)(max_conn_num >> 0);

        data[7] = (BYTE)(nodecount >> 8);
        data[8] = (BYTE)(nodecount >> 0);

        // 追加CRC字段
        WORD crc = CRC16(data, offset);
        data[offset] = (BYTE)(crc >> 8);
        data[offset + 1] = (BYTE)(crc >> 0);
        offset += 2;

        // 广播消息到所有在线客户端
        for (int id = 0; id < MAX_CONNECTION_NUM; id++) {
            SOCKET sock = M_CONNECT_SOCK(id);
            if (sock != INVALID_SOCKET) {
                if (M_CONNECT_LOST(id) == FALSE) {
                    int num = send(sock, (char*)data, offset, 0);
                    if (num <= 0) {
                        WriteLog(LOG_WARN, _T("定时广播消息时，发送到客户端[ID: %d]失败。[该客户端可能已掉线]"), id);
                    }
                }
            }
        }

        //WriteLog(LOG_INFO, _T("本次定时广播消息时，共发送 %d 个消息"), nodecount);

        // 该链表全部访问完
        if (g_UserStateChangeList.lpFirstNode == NULL) {
            g_UserStateChangeList.lpLastNode = NULL;
        } else {
            WriteLog(LOG_WARN, _T("定时线程未发送完所有待发数据，余下的数据将累积到下次发送"));
        }
    } else {
        //WriteLog(LOG_DEBUG, _T("本次定时广播消息时，没有需要发送的数据"));
    }

    // 回收离线玩家的ID
    if (nLeftUserCount > 0) {
        EnterCriticalSection(&g_csConnectionNum);

        for (int i = 0; i < nLeftUserCount; i++) {
            DeallocID(s_LeftUserIds[i]);
            g_nCurConnectionNum--;

            closesocket(M_CONNECT_SOCK(s_LeftUserIds[i]));
            M_CONNECT_SOCK(s_LeftUserIds[i]) = INVALID_SOCKET;
            M_CONNECT_PALYER_AVAILABLE(s_LeftUserIds[i]) = FALSE;

            WriteLog(LOG_INFO, _T("回收ID: %d"), s_LeftUserIds[i]);
        }

        LeaveCriticalSection(&g_csConnectionNum);
    }
}

//
// 分析接收到的消息，并分发给各消息处理函数
//
// 允许客户端同时发送多个消息到服务器，只要每个消息结构合法即可
// 这也是考虑到服务器可能忙不过来，当忙完再去接收数据时，可能已经累积多个消息待处理
//
static void OnMessage(int id, char* buf, int len)
{
    WORD myCrc, yourCrc;

    int nMsgCode = 0;       // 消息码
    int nMsgBodyLen = 0;    // 消息体长度
    int nParsedLen = 0;     // 分析过数据的长度

    BYTE* data = (BYTE*)buf;

    char* msg = NULL;
    int   msglen = 0;
    int   msgcount = 0;

    while (nParsedLen < len) {
        if (nParsedLen + 2 >= len) {
            WriteLog(LOG_ERROR, _T("[1]解析消息时，遇到结构不合法的消息，结束本次解析。已解析 %d 个合法消息"), msgcount);
            return;
        }

        nMsgBodyLen = (data[nParsedLen + 1] << 8) | data[nParsedLen + 2];

        // 1个字节消息码，2个字节消息体长度，nMsgBodyLen个字节消息体，2个字节CRC
        if (nParsedLen + 1 + 2 + nMsgBodyLen + 2 > len) {
            WriteLog(LOG_ERROR, _T("[2]解析消息时，遇到结构不合法的消息，结束本次解析。已解析 %d 个合法消息"), msgcount);
            return;
        }

        myCrc = CRC16((data + nParsedLen), 1 + 2 + nMsgBodyLen);
        yourCrc = (data[nParsedLen + 1 + 2 + nMsgBodyLen] << 8) | data[nParsedLen + 1 + 2 + nMsgBodyLen + 1];
        if (myCrc != yourCrc) {
            WriteLog(LOG_ERROR, _T("接收的数据CRC字段不正确，结束本次解析。已解析 %d 个合法消息"), msgcount);
            return;
        }

        nMsgCode = data[nParsedLen];
        msg = (char*)(data + nParsedLen);
        msglen = 1 + 2 + nMsgBodyLen;

        // 1个字节消息码，2个字节消息体长度，nMsgBodyLen个字节消息体，2个字节CRC
        nParsedLen += (1 + 2 + nMsgBodyLen + 2);
        msgcount++;

        // 分发消息时，将整则消息及不包括CRC字段的长度传递给相应处理函数
        switch (nMsgCode) {
        case NMSG_REQ_ROOM_INFO:
            OnMsgReqRoomInfo(id, msg, msglen);
            break;

        case NMSG_REQ_GAMER_TAKE_SEAT:
            OnMsgReqGamerTakeSeat(id, msg, msglen);
            break;

        case NMSG_REQ_GAMER_LEAVE_SEAT:
            OnMsgReqGamerLeaveSeat(id, msg, msglen);
            break;

        case NMSG_REQ_LOOKON_TAKE_SEAT:
            OnMsgReqLookonTakeSeat(id, msg, msglen);
            break;

        case NMSG_REQ_LOOKON_LEAVE_SEAT:
            OnMsgReqLookonLeaveSeat(id, msg, msglen);
            break;

        case NMSG_REQ_GAMER_READY:
            OnMsgReqGamerReady(id, msg, msglen);
            break;

        // 其它消息为游戏消息，直接转交给游戏工作线程
        default:
            PostMsgDataToGameThread(id, msg, msglen);
            break;
        }
    }
}

// 发送大厅数据给客户端
static void SendRoomInfo(int nAllocID)
{
    SOCKET sock = M_CONNECT_SOCK(nAllocID);

    int offset = 0;
    BYTE* data = NULL;

    // 分配一块足够大的内存
    int nBufLen = (sizeof(CONNECTION_INFO) - sizeof(SYSTEMTIME)) * MAX_CONNECTION_NUM;

    data = (BYTE*)LocalAlloc(LMEM_FIXED, nBufLen);
    if (data == NULL) {
        WriteLog(LOG_ERROR, _T("发送大厅数据给客户端失败，分配内存失败"));
        return;
    }

    //
    // 填充消息体
    //
    // 第0字节为消息码，第1、2字节为消息体长度，第3、4字节为服务器为该客户端分配的ID，
    // 第5、6字节为当前在线玩家数量。从第7字节开始填充玩家信息
    //
    offset = 7;

    int nUserCount = 0;

    // 本消息中将不包含请求者自身的信息
    for (int id = 0; id < MAX_CONNECTION_NUM; id++) {
        if (M_CONNECT_PALYER_AVAILABLE(id) == TRUE) {
            // 填充ID: 16-bit 
            data[offset] = (BYTE)(id >> 8);
            data[offset + 1] = (BYTE)(id >> 0);
            offset += 2;

            // 名字长度（字节数），占用1字节。先保存其下标，等会再填入值
            int name_len_offset = offset;
            offset++;

            // 填充名字
            _tcscpy_s((TCHAR*)(data + offset), MAX_USER_NAME_LEN, M_CONNECT_PLAYER_NAME(id));
            int name_len = _tcslen(M_CONNECT_PLAYER_NAME(id)) * sizeof(TCHAR);
            offset += name_len;

            // 填充名字长度
            data[name_len_offset] = (BYTE)name_len;

            // 第7-bit表示性别，6~4-bit为状态，3~0-bit为头像索引
            data[offset] = 0;
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_GENDER(id)) << 7);
            data[offset] |= (BYTE)((int)(M_CONNECT_STATE(id) & 0x07) << 4);
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_IMG_IDX(id) & 0x0F));
            offset++;

            // 如果玩家处于空闲状态，则用一个字节的最高位表示他是否允许旁观
            // 如果玩家不是处于空闲状态，则用一个字节的最高位表示他是否允许旁观，
            // 低4位表示所在椅子号，再加一字节表示他所在的桌子号
            data[offset] = 0;
            if (M_CONNECT_STATE(id) == STATE_IDLE) {
                data[offset] = M_CONNECT_ALLOW_LOOKON(id) ? 0x80 : 0x00;
                offset++;
            } else {
                data[offset] = M_CONNECT_ALLOW_LOOKON(id) ? 0x80 : 0x00;
                data[offset] |= (BYTE)(M_CONNECT_SEAT(id) & 0x0F);
                offset++;

                data[offset] = (BYTE)(M_CONNECT_TABLE(id));
                offset++;
            }

            // 玩家的积分等统计信息
            data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 24);
            data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 16);
            data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 8);
            data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_SCORE(id) >> 0);
            offset += 4;

            data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 24);
            data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 16);
            data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 8);
            data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_TOTALGAMES(id) >> 0);
            offset += 4;

            data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 24);
            data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 16);
            data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 8);
            data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_WINGAMES(id) >> 0);
            offset += 4;

            data[offset + 0] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 24);
            data[offset + 1] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 16);
            data[offset + 2] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 8);
            data[offset + 3] = (BYTE)(M_CONNECT_PLAYER_RUNAWAY(id) >> 0);
            offset += 4;

            // 完成一个玩家信息的收集
            nUserCount++;
        }
    }

    // 消息码
    data[0] = NMSG_ACK_ROOM_INFO;

    // 填充消息体长度，除去作为消息码的首字节及消息体长度字段两字节
    data[1] = (BYTE)((offset - 3) >> 8);
    data[2] = (BYTE)((offset - 3) >> 0);

    // 服务器分配给该客户端的ID
    data[3] = (BYTE)(nAllocID >> 8);
    data[4] = (BYTE)(nAllocID >> 0);

    // 玩家数量
    data[5] = (BYTE)(nUserCount >> 8);
    data[6] = (BYTE)(nUserCount >> 0);

    // 追加CRC字段
    WORD crc = CRC16(data, offset);
    data[offset + 0] = (BYTE)(crc >> 8);
    data[offset + 1] = (BYTE)(crc >> 0);
    offset += 2;

    int num = send(sock, (char*)data, offset, 0);
    if (num < 0) {
        WriteLog(LOG_ERROR, _T("发送大厅信息失败"));
    }

    LocalFree(data);
}

// 将新进入房间的玩家信息存入链表
// 函数动态申请了内存，由定时广播信息的线程释放
void AddUserStateChangeEvent(int id, STATE_CHANGE_EVENT evt, PLAYER_STATE newState, BOOL bContainStat)
{
    USER_STATE_CHANGE* pNode;

    if (g_bServerWillShutdown == TRUE) {
        return;
    }

    pNode = (USER_STATE_CHANGE*)LocalAlloc(LMEM_FIXED, sizeof(USER_STATE_CHANGE));
    if (pNode == NULL) {
        WriteLog(LOG_ERROR, _T("添加新客户信息到链表时，分配内存失败"));
        return;
    }

    EnterCriticalSection(&g_csUserStateChangeList);

    pNode->id = id;
    pNode->evt = evt;
    pNode->newState = newState;
    pNode->bContainStatistics = bContainStat;
    pNode->next = NULL;

    if (g_UserStateChangeList.lpFirstNode == NULL) {
        g_UserStateChangeList.lpFirstNode = pNode;
        g_UserStateChangeList.lpLastNode = pNode;
    } else {
        g_UserStateChangeList.lpLastNode->next = pNode;
        g_UserStateChangeList.lpLastNode = pNode;
    }

    LeaveCriticalSection(&g_csUserStateChangeList);
}

// 客户端玩家进入大厅，请求大厅数据
static void OnMsgReqRoomInfo(int id, char* msg, int len)
{
    //
    // 先给客户端发送大厅数据，接着保存客户端的基本信息
    //
    SendRoomInfo(id);

    // 保存进入大厅的该客户端玩家基本信息
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    // 跳过消息码字段
    offset++;

    // 消息体长度
    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_REQ_ROOM_INFO 消息结构不正确"));
        return;
    }
    offset += 2;

    // 名字长度
    int name_len = data[offset];
    offset++;

    // 名字
    int max_valid_len = (MAX_USER_NAME_LEN - 1) * sizeof(TCHAR);
    int valid_len = name_len < max_valid_len ?  name_len : max_valid_len;

    BYTE* pName = (BYTE*)M_CONNECT_PLAYER_NAME(id);
    for (int i = 0; i < valid_len; i++) {
        *(pName + i) = data[offset + i];
    }

    int valid_char_num = valid_len / sizeof(TCHAR);
    M_CONNECT_PLAYER_NAME(id)[valid_char_num] = _T('\0');
    offset += name_len;

    // 性别，状态，头像索引
    M_CONNECT_PLAYER_GENDER(id) = (PLAYER_GENDER)(data[offset] >> 7);
    M_CONNECT_STATE(id) = STATE_IDLE; // 刚进入大厅，设IDLE状态
    M_CONNECT_PLAYER_IMG_IDX(id) = data[offset] & 0x0F;
    offset++;

    // 标识该玩家的信息已经上传到服务器
    M_CONNECT_PALYER_AVAILABLE(id) = TRUE;

    // 追加用户状态变化链表结点
    AddUserStateChangeEvent(id, EVT_ENTER, (PLAYER_STATE)0, FALSE);

    WriteLog(LOG_INFO, _T("收到用户 [ID: %d] 基本信息: %s, %s, %d"), id,
        M_CONNECT_PLAYER_NAME(id),
        M_CONNECT_PLAYER_GENDER(id) == MALE ? _T("MALE"): _T("FEMALE") ,
        M_CONNECT_PLAYER_IMG_IDX(id));
}

// 在游戏桌内广播所有成员的完整信息
// 收集游戏桌内所有成员的信息，并广播到每个成员
//
// 最初设计时，考虑到这样一个问题：例如两个玩家连接服务器后，分别选择坐入1号桌的
// 1号椅子和2号椅子，假设这两个REQ_GAMER_TAKE_SEAT消息到达服务器时，在1秒钟的
// 定时广播之前，后连接的玩家没有先连接的玩家基本信息。为了避免这个问题，
// 当时设计时，特地在回复玩家入座时，即服务器发送ACK_GAMER_TAKE_SEAT时，将
// 该游戏桌内所有成员的完整信息都发送到客户端。这样势必与定时广播线程的工作有所
// 重复，而且增加服务器的网络负载。现在想来，其实没必要这样。但为了避免该可能存在
// 的问题，客户端最好在收到ACK_GAMER_TAKE_SEAT、ACK_LOOKON_TAKE_SEAT时，等1秒才
// 进入游戏界面。
//
// 修改这个函数：不发送游戏桌内玩家所有信息，仅在游戏内广播入座的玩家ID及其椅子号
// 2009-06-07
//
static void ExchangeInfoInTable(int myid, int table, int seat, BYTE msgcode)
{
    int id;
    int offset = 0;

    int nBufLen = (MAX_USER_NAME_LEN + 10) * (GAME_SEAT_NUM_PER_TABLE) * (MAX_LOOKON_NUM_PER_SEAT + 1);
    BYTE* data = (BYTE*)LocalAlloc(LMEM_FIXED, nBufLen);
    if (data == NULL) {
        WriteLog(LOG_ERROR, _T("游戏桌内广播消息时，为发送数据而分配内存失败"));
        return;
    }

    // 消息码
    data[0] = (BYTE)msgcode;

    // 消息体长度
    data[1] = 0;
    data[2] = 0;

    // 消息体
    data[3] = (BYTE)TRUE; // 玩家或旁观者可以进入游戏桌
    offset = 4;

#if 0
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        id = M_TABLE_PLAYER_ID(table, seat);

        // 该座位已经有一个玩家
        if (id != INVALID_USER_ID) {
            // ID
            data[offset] = (BYTE)(id >> 8);
            data[offset + 1] = (BYTE)(id >> 0);
            offset += 2;

            // 名字长度，1字节，先保存其下标，等会再填入值
            int name_len_offset = offset;
            offset++;

            // 填充名字
            _tcscpy_s((TCHAR*)(data + offset), MAX_USER_NAME_LEN, M_CONNECT_PLAYER_NAME(id));
            int name_len = _tcslen(M_CONNECT_PLAYER_NAME(id)) * sizeof(TCHAR);
            offset += name_len;

            // 填充名字长度
            data[name_len_offset] = (BYTE)name_len;

            // 第7-bit表示性别，6~4-bit为状态，3~0-bit为头像索引
            data[offset] = 0;
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_GENDER(id)) << 7);
            data[offset] |= (BYTE)((int)(M_CONNECT_STATE(id) & 0x07) << 4);
            data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_IMG_IDX(id) & 0x0F));
            offset++;

            data[offset] = M_CONNECT_ALLOW_LOOKON(id) ? 0x80 : 0x00;
            data[offset] |= (BYTE)(M_CONNECT_SEAT(id) & 0x0F);
            offset++;

            data[offset] = (BYTE)(M_CONNECT_TABLE(id));
            offset++;

            // 该座位的旁观者信息
            for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
                id = M_TABLE_LOOKON_ID(table, seat)[i];

                if (id != INVALID_USER_ID) {
                    // ID
                    data[offset] = (BYTE)(id >> 8);
                    data[offset + 1] = (BYTE)(id >> 0);
                    offset += 2;

                    // 名字长度，1字节，先保存其下标，等会再填入值
                    int name_len_offset = offset;
                    offset++;

                    // 填充名字
                    _tcscpy_s((TCHAR*)(data + offset), MAX_USER_NAME_LEN, M_CONNECT_PLAYER_NAME(id));
                    int name_len = _tcslen(M_CONNECT_PLAYER_NAME(id)) * sizeof(TCHAR);
                    offset += name_len;

                    // 填充名字长度
                    data[name_len_offset] = (BYTE)name_len;

                    // 第7-bit表示性别，6~4-bit为状态，3~0-bit为头像索引
                    data[offset] = 0;
                    data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_GENDER(id)) << 7);
                    data[offset] |= (BYTE)((int)(M_CONNECT_STATE(id)& 0x07) << 4);
                    data[offset] |= (BYTE)((int)(M_CONNECT_PLAYER_IMG_IDX(id)& 0x0F));
                    offset++;

                    data[offset] = M_CONNECT_ALLOW_LOOKON(id)? 0x80 : 0x00;
                    data[offset] |= (BYTE)(M_CONNECT_SEAT(id) & 0x0F);
                    offset++;

                    data[offset] = (BYTE)(M_CONNECT_TABLE(id));
                    offset++;
                }
            }
        }
    }
#else
    data[offset] = (BYTE)table;
    offset++;

    data[offset] = (BYTE)seat;
    offset++;

    data[offset + 0] = (BYTE)(myid >> 8);
    data[offset + 1] = (BYTE)(myid >> 0);
    offset += 2;
#endif

    // 填充消息体长度，除去作为消息码的首字节及消息体长度字段两字节
    data[1] = (BYTE)((offset - 3) >> 8);
    data[2] = (BYTE)((offset - 3) >> 0);

    // 追加CRC字段
    WORD crc = CRC16(data, offset);
    data[offset] = (BYTE)(crc >> 8);
    data[offset + 1] = (BYTE)(crc >> 0);
    offset += 2;

    // 收集完数据后，在游戏桌中广播
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        id = M_TABLE_PLAYER_ID(table, seat);

        if (id != INVALID_USER_ID) {
            SOCKET sock = M_CONNECT_SOCK(id);
            if (sock != INVALID_SOCKET) {
                if (M_CONNECT_LOST(id) == FALSE) {
                    int num = send(sock, (char*)data, offset, 0);
                    if (num <= 0) {
                        WriteLog(LOG_ERROR, _T("游戏桌内广播消息时，发送消息到客户端[ID: %d]失败"), id);
                    }
                }
            }

            // 该椅子的旁观者（若椅子上没有玩家，则该椅子没有旁观者）
            for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
                id = M_TABLE_LOOKON_ID(table, seat)[i];

                if (id != INVALID_USER_ID) {
                    SOCKET sock = M_CONNECT_SOCK(id);
                    if (sock != INVALID_SOCKET) {
                        if (M_CONNECT_LOST(id) == FALSE) {
                            int num = send(sock, (char*)data, offset, 0);
                            if (num <= 0) {
                                WriteLog(LOG_ERROR, _T("游戏桌内广播消息时，发送消息到客户端[ID: %d]失败"), id);
                            }
                        }
                    }
                }
            }
        }
    }

    LocalFree(data);
}

// 向游戏桌内广播某个玩家的状态信息
// （玩家离开游戏桌，旁观者离开游戏桌，玩家准备就绪)
static void BroadcastInfoInTable(int id, int table, int seat, BYTE msgcode)
{
    int len = 0;

    BYTE data[16] = { 0 };
    WORD crc = 0;

    //
    // 2009-06-08 修改这三个消息的格式，见文档。需要重新封闭消息体，特修改代码如下。
    // （玩家或旁观者离开，服务器会通知游戏桌内其它成员。玩家准备就绪，服务器会通知
    // 游戏桌内所有成员）。
    //
    switch (msgcode) {
        case NMSG_ACK_GAMER_LEAVE_SEAT:
        case NMSG_ACK_GAMER_READY:
            data[0] = msgcode;
            data[1] = 0;
            data[2] = 2; // 消息体长度为2字节

            data[3] = (BYTE)table;
            data[4] = (BYTE)seat;

            crc = CRC16(data, 5);
            data[5] = (BYTE)(crc >> 8);
            data[6] = (BYTE)(crc >> 0);

            len = 7; // 要发送的数据长度
            break;

        case NMSG_ACK_LOOKON_LEAVE_SEAT:
            data[0] = msgcode;
            data[1] = 0;
            data[2] = 4; // 消息体长度为4字节

            data[3] = (BYTE)table;
            data[4] = (BYTE)seat;
            data[5] = (BYTE)(id >> 8);
            data[6] = (BYTE)(id >> 0);

            crc = CRC16(data, 7);
            data[7] = (BYTE)(crc >> 8);
            data[8] = (BYTE)(crc >> 0);

            len = 9; // 要发送的数据长度
            break;

        default:
            // No other type of message supported.
            return;
    }

    // 在游戏桌中广播其他成员
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int gamerid = M_TABLE_PLAYER_ID(table, i);

        if (gamerid != INVALID_USER_ID) {
            SOCKET sock = M_CONNECT_SOCK(gamerid);
            if (sock != INVALID_SOCKET) {
                if (M_CONNECT_LOST(id) == FALSE) {
                    int num = send(sock, (char*)data, len, 0);
                    if (num <= 0) {
                        WriteLog(LOG_ERROR, _T("游戏桌内广播消息时，发送消息到客户端[ID: %d]失败"), gamerid);
                    }
                }
            }
        }

        // 该玩家的旁观者
        //（若玩家离开，则椅子上没有玩家，但椅子旁有旁观者；若玩家进入，则先清空椅子上所有玩家ID）
        for (int j = 0; j < MAX_LOOKON_NUM_PER_SEAT; j++) {
            int lookonid = M_TABLE_LOOKON_ID(table, i)[j];

            if (lookonid != INVALID_USER_ID) {
                SOCKET sock = M_CONNECT_SOCK(lookonid);
                if (sock != INVALID_SOCKET) {
                    if (M_CONNECT_LOST(id) == FALSE) {
                        int num = send(sock, (char*)data, len, 0);
                        if (num <= 0) {
                            WriteLog(LOG_ERROR, _T("游戏桌内广播消息时，发送消息到客户端[ID: %d]失败"), lookonid);
                        }
                    }
                }
            }
        }
    }
}

// 响应消息，客户玩家请求坐入位子
static void OnMsgReqGamerTakeSeat(int id, char* msg, int len)
{
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    offset++; // 跳过消息码

    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_REQ_GAMER_TAKE_SEAT 消息结构不正确"));
        return;
    }
    offset += 2;

    int table = data[offset];
    IF_INVALID_TABLE_RET(table);
    offset++;

    int seat = data[offset];
    IF_INVALID_SEAT_RET(seat);
    offset++;

    BOOL bSeatAvailable = FALSE;
    if (IS_INVALID_USER_ID(M_TABLE_PLAYER_ID(table, seat))) { // 空位子
        bSeatAvailable = TRUE;
    } else {
        bSeatAvailable = FALSE;
    }

    if (bSeatAvailable == FALSE) { // 不能入座
        BYTE msgRet[8];
        msgRet[0] = NMSG_ACK_GAMER_TAKE_SEAT;
        msgRet[1] = 0;
        msgRet[2] = 3; // 消息体长度为3
        msgRet[3] = (BYTE)bSeatAvailable;
        msgRet[4] = (BYTE)table;
        msgRet[5] = (BYTE)seat;

        // 追加CRC字段
        WORD crc = CRC16(msgRet, 6);
        msgRet[6] = (BYTE)(crc >> 8);
        msgRet[7] = (BYTE)(crc >> 0);

        SOCKET sock = M_CONNECT_SOCK(id);
        int nSend = send(sock, (char*)msgRet, 8, 0);
        if (nSend <= 0) {
            WriteLog(LOG_ERROR, _T("NMSG_ACK_GAMER_TAKE_SEAT 消息发送失败"));
            return;
        }
    } else { // 可以入座，则更新数据结构；同时通知对应的游戏桌工作线程
        M_CONNECT_STATE(id)= STATE_SIT;
        M_CONNECT_TABLE(id)= table;
        M_CONNECT_SEAT(id)= seat;
        M_TABLE_PLAYER_ID(table, seat) = id;

        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            M_TABLE_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
        }

        // 游戏桌内广播
        ExchangeInfoInTable(id, table, seat, NMSG_ACK_GAMER_TAKE_SEAT);

        // 状态改变
        AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_SIT, FALSE);

        // 本线程负责广播消息到游戏桌其它成员，然后通知游戏工作线程
        PostThreadMessage(g_nGameThreadIds[table], TM_GAMER_ENTER, (WPARAM)id, (LPARAM)seat);
    }

    if (bSeatAvailable == TRUE) {
        WriteLog(LOG_INFO, _T("接受: 玩家 [ID: %d] 坐入第 %d 桌的 %d 号位子"), id, table, seat);
    } else {
        WriteLog(LOG_INFO, _T("拒绝: 玩家 [ID: %d] 坐入第 %d 桌的 %d 号位子。玩家 [ID: %d] 已经先坐入该位子"),
            id, table, seat, M_TABLE_PLAYER_ID(table, seat));
    }
}

// 响应消息，客户玩家离开位子
static void OnMsgReqGamerLeaveSeat(int id, char* msg, int len)
{
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    offset++; // 跳过消息码

    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_GAMER_LEAVE_SEAT 消息结构不正确"));
        return;
    }
    offset += 2;

    int table = data[offset];
    IF_INVALID_TABLE_RET(table);
    offset++;

    int seat = data[offset];
    IF_INVALID_SEAT_RET(seat);
    offset++;

    // 更新数据结构
    M_CONNECT_STATE(id)= STATE_IDLE;
    M_TABLE_PLAYER_ID(table, seat) = INVALID_USER_ID;

    // 玩家离开游戏桌后，通知游戏桌其他成员
    BroadcastInfoInTable(id, table, seat, NMSG_ACK_GAMER_LEAVE_SEAT);

    // 状态改变
    AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_IDLE, FALSE);

    // 本线程负责广播消息到游戏桌其它成员，然后通知游戏工作线程
    PostThreadMessage(g_nGameThreadIds[table], TM_GAMER_LEAVE, (WPARAM)id, (LPARAM)seat);

    WriteLog(LOG_INFO, _T("玩家 [ID: %d] 离开第 %d 桌的 %d 号位子"), id, table, seat);
}

// 响应消息，玩家请求旁观
static void OnMsgReqLookonTakeSeat(int id, char* msg, int len)
{
    //
    // 客户端软件先进行判断，如果游戏玩家设置不允许旁观选项，则客户端软件不发送
    // 请求旁观的消息到服务器，直接在客户端提示不允许旁观
    //
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    offset++; // 跳过消息码

    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_REQ_LOOKON_TAKE_SEAT 消息结构不正确"));
        return;
    }
    offset += 2;

    int table = data[offset];
    IF_INVALID_TABLE_RET(table);
    offset++;

    int seat = data[offset];
    IF_INVALID_SEAT_RET(seat);
    offset++;

    // 想要旁观哪个玩家
    int nPlayerID = M_TABLE_PLAYER_ID(table, seat);

    // 不能旁观的原因
    int nNoLookonReason = 0;
    BOOL bCanLookon = FALSE;
    if (IS_INVALID_USER_ID(nPlayerID)) { // 空位子
        // 客户端的数据可能没及时与服务器同步，所以，可能会向服务器请求旁观一个刚离开的玩家
        bCanLookon = FALSE;
        nNoLookonReason = 0;
    } else if (M_CONNECT_ALLOW_LOOKON(nPlayerID)== FALSE) {
        bCanLookon = FALSE;
        nNoLookonReason = 1;
    } else {
        bCanLookon = TRUE;
    }

    int nLookonIndex = -1;

    // 判断一下旁观人数是否已满
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (IS_INVALID_USER_ID(M_TABLE_LOOKON_ID(table, seat)[i])) {
            nLookonIndex = i;
            break;
        }
    }

    if (nLookonIndex == -1) { // 旁观位子已满
        bCanLookon = FALSE;
        nNoLookonReason = 2;
    }

    if (bCanLookon == FALSE) { // 不能旁观
        BYTE msgRet[8];
        msgRet[0] = NMSG_ACK_LOOKON_TAKE_SEAT;
        msgRet[1] = 0;
        msgRet[2] = 3; // 消息体长度为3
        msgRet[3] = (BYTE)bCanLookon;
        msgRet[4] = (BYTE)table;
        msgRet[5] = (BYTE)seat;

        // 追加CRC字段
        WORD crc = CRC16(msgRet, 6);
        msgRet[6] = (BYTE)(crc >> 8);
        msgRet[7] = (BYTE)(crc >> 0);

        SOCKET sock = M_CONNECT_SOCK(id);
        int nSend = send(sock, (char*)msgRet, 8, 0);
        if (nSend <= 0) {
            WriteLog(LOG_ERROR, _T("NMSG_ACK_LOOKON_TAKE_SEAT 消息发送失败"));
            return;
        }
    } else { // 更新数据结构；同时通知对应的游戏桌工作线程
        M_CONNECT_STATE(id) = STATE_LOOKON;
        M_CONNECT_TABLE(id)= table;
        M_CONNECT_SEAT(id)= seat;
        M_TABLE_LOOKON_ID(table, seat)[nLookonIndex] = id;

        // 游戏桌内广播
        ExchangeInfoInTable(id, table, seat, NMSG_ACK_LOOKON_TAKE_SEAT);

        // 状态改变
        AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_LOOKON, FALSE);

        // 本线程负责广播消息到游戏桌其它成员，然后通知游戏工作线程
        PostThreadMessage(g_nGameThreadIds[table], TM_LOOKON_ENTER, (WPARAM)id, (LPARAM)seat);
    }

    if (bCanLookon == TRUE) {
        WriteLog(LOG_INFO, _T("接受: 玩家 [ID: %d] 在第 %d 桌的 %d 号位子旁观玩家 [ID: %d]"), id, table, seat, nPlayerID);
    } else if (nNoLookonReason == 0) {
        WriteLog(LOG_INFO, _T("拒绝: 玩家 [ID: %d] 在第 %d 桌的 %d 号位子旁观玩家 [ID: %d]。因为该玩家已经离开!"),
            id, table, seat, nPlayerID);
    } else if (nNoLookonReason == 1) {
        WriteLog(LOG_INFO, _T("拒绝: 玩家 [ID: %d] 在第 %d 桌的 %d 号位子旁观玩家 [ID: %d]。因为该玩家设置为不允许旁观!"),
            id, table, seat, nPlayerID);
    } else if (nNoLookonReason == 2) {
        WriteLog(LOG_INFO, _T("拒绝: 玩家 [ID: %d] 在第 %d 桌的 %d 号位子旁观玩家 [ID: %d]。因为旁观该玩家的人数已经达到最大值!"),
            id, table, seat, nPlayerID);
    }
}

// 响应消息，旁观者离席
static void OnMsgReqLookonLeaveSeat(int id, char* msg, int len)
{
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    offset++; // 跳过消息码

    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_REQ_LOOKON_LEAVE_SEAT 消息结构不正确"));
        return;
    }
    offset += 2;

    int table = data[offset];
    offset++;

    int seat = data[offset];
    offset++;

    // 更新数据结构
    M_CONNECT_STATE(id) = STATE_IDLE;

    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (M_TABLE_LOOKON_ID(table, seat)[i] == id) {
            M_TABLE_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
            break;
        }
    }

    // 旁观者离开游戏桌后，通知游戏桌其他成员
    BroadcastInfoInTable(id, table, seat, NMSG_ACK_LOOKON_LEAVE_SEAT);

    // 状态改变
    AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_IDLE, FALSE);

    WriteLog(LOG_INFO, _T("旁观玩家 [ID: %d] 离开第 %d 桌的 %d 号位子"), id, table, seat);

    // 本线程负责广播消息到游戏桌其它成员，然后通知游戏工作线程
    PostThreadMessage(g_nGameThreadIds[table], TM_LOOKON_LEAVE, (WPARAM)id, (LPARAM)seat);
}

// 响应消息，玩家准备就绪，请求服务器发牌
static void OnMsgReqGamerReady(int id, char* msg, int len)
{
    int offset = 0;
    BYTE* data = (BYTE*)msg;

    if (M_CONNECT_STATE(id) != STATE_SIT) {
        WriteLog(LOG_ERROR, _T("玩家不能准备游戏，其状态不正确。客户端发送不正确的消息到服务器"));
        return;
    }

    offset++; // 跳过消息码

    int nMsgBodyLen = (data[offset] << 8) | data[offset + 1];
    if (nMsgBodyLen != len - 3) { // 除去1字节消息码和2字节的消息体长度
        WriteLog(LOG_ERROR, _T("NMSG_REQ_GAMER_READY 消息结构不正确"));
        return;
    }
    offset += 2;

    int table = data[offset];
    IF_INVALID_TABLE_RET(table);
    offset++;

    int seat = data[offset];
    IF_INVALID_SEAT_RET(seat);
    offset++;

    // 更新数据结构
    M_CONNECT_STATE(id)= STATE_READY;

    // 玩家准备就绪后，通知游戏桌其他成员
    BroadcastInfoInTable(id, table, seat, NMSG_ACK_GAMER_READY);

    // 状态改变
    AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_READY, FALSE);

    WriteLog(LOG_INFO, _T("玩家 [ID: %d] 在第 %d 桌的 %d 号位子准备就绪"), id, table, seat);

    // 本线程负责广播消息到游戏桌其它成员，然后通知游戏工作线程
    PostThreadMessage(g_nGameThreadIds[table], TM_GAMER_READY, (WPARAM)id, (LPARAM)seat);
}

//将接收到的消息转发给游戏工作线程。游戏工作线程处理完消息后，需要负责释放内存
static void PostMsgDataToGameThread(int id, char* buf, int len)
{
    int table = M_CONNECT_TABLE(id);

    LPVOID lpMem = HeapAlloc(GetProcessHeap(), 0, len);
    if (lpMem == NULL) {
        WriteLog(LOG_ERROR, _T("从进程的堆中分配用于保存接收数据的内存失败: %d"), len);
        return;
    }

    CopyMemory(lpMem, buf, len);

    WPARAM wp = MAKEWPARAM(len, id);
    LPARAM lp = (LPARAM)lpMem;

    PostThreadMessage(g_nGameThreadIds[table], TM_RECEIVE_DATA, wp, lp);
}

// 发送数据到客户端
int SendData(int id, char* buf, int len)
{
    int num = -1;
    SOCKET sock = INVALID_SOCKET;

    if (!IS_INVALID_USER_ID(id)) {
        sock = M_CONNECT_SOCK(id);
        if (sock != INVALID_SOCKET) {
            if (M_CONNECT_LOST(id) == FALSE) {
                num = send(sock, buf, len, 0);
            }
        }
    }

    return num;
}

