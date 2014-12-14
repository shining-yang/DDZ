//
// File: ServerListWnd.h
//
#pragma once

#define SERVER_LIST_WND_CLASS_NAME          _T("_SERVER_LIST_WND_")

#define ID_SERVER_LIST_TREE                 1
#define ID_SERVER_BUTTON_ADD                2
#define ID_SERVER_BUTTON_EDIT               3
#define ID_SERVER_BUTTON_REMOVE             4

#define SLW_CTRL_OFFSET                     10
#define SLW_BUTTON_HEIGHT                   28
#define SLW_BUTTON_V_INTERVAL               4
#define SLW_ROUND_FRAME_OFFSET              4
#define SLW_ROUND_FRAME_WIDTH               4
#define SLW_ROUND_FRAME_ANGEL               16

#define SLW_WINDOW_BG_CLR                   RGB(70,180,70)
#define SLW_TREE_BG_CLR                     SLW_WINDOW_BG_CLR
#define SLW_ROUND_FRAME_CLR                 RGB(32,64,192)

#define MAX_SERVER_NAME_LEN                 32
#define _LOCAL_CHAR_ARRAY_SZ                MAX_SERVER_NAME_LEN

#define MAX_SERVER_NUM_SUPPORTED            32
#define INVALID_SERVER_INDEX                -1

#define SERVER_SECTION_NAME                 _T("SERVER")
#define SERVER_SECTION_KEY_NAME             _T("NAME")
#define SERVER_SECTION_KEY_IP               _T("IP")
#define SERVER_SECTION_KEY_PORT             _T("PORT")

// 标识删除、修改、添加一个SECTION到INI文件
typedef enum {
    SF_REMOVE = 0,
    SF_MODIFY,
    SF_APPEND
} SF_FLAG;

// 服务器基本信息
typedef struct SERVER_INFO_ITEM_t {
    TCHAR       name[MAX_SERVER_NAME_LEN];
    DWORD       ip;
    USHORT      port;
} SERVER_INFO_ITEM, *LPSERVER_INFO_ITEM;

// 记录当前可用的服务器信息
typedef struct SERVER_INFO_t {
    BOOL                valid;
    SERVER_INFO_ITEM    svi;
} SERVER_INFO, *LPSERVER_INFO;

// 服务器列表面板窗口
class ServerListWnd
{
    HINSTANCE   m_hInstance;
    HWND        m_hWndParent;
    HWND        m_hWnd;
    BOOL        m_bShow;

    HWND        m_hServerListTree;
    HWND        m_hButtonAdd;
    HWND        m_hButtonEdit;
    HWND        m_hButtonRemove;

    SERVER_INFO m_ServerInfo[MAX_SERVER_NUM_SUPPORTED];
    TCHAR       m_szConfigFile[MAX_PATH];
public:
    ServerListWnd(void);
    ~ServerListWnd(void);

public:
    static ATOM ServerListWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK ServerListWndProc(HWND, UINT, WPARAM, LPARAM);

    static INT_PTR CALLBACK AddServerInfoProc(HWND, UINT, WPARAM, LPARAM);
    static INT_PTR CALLBACK EditServerInfoProc(HWND, UINT, WPARAM, LPARAM);

private:
    static BOOL OnAddServerOK(HWND hDlg);
    static BOOL OnEditServerOK(HWND hDlg);

private:
    void CreateChildWindows(void);
    void InsertServerNodeToTree(LPTSTR szLabel, int nImage, int nSelImage, HTREEITEM htiParent, LPARAM lParam);
    
    BOOL InitServerInfoTree(void);
    BOOL LoadServerInfoFile(LPCTSTR lpszFileName);

    int  FindAvailableIndex(void);
    int  CreateIndexFromSectionName(LPCTSTR szSecName);

    void RemoveServerInfo(int index);
    void SaveServerInfo(int index, LPCTSTR name, DWORD ip, USHORT port);
    void SaveServerInfoToConfigFile(int index, SF_FLAG flag);

protected:
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnNotify(WPARAM wParam, LPARAM lParam);
    void OnSize(WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc);

    void OnDoubleClickServerTree(HWND hWnd);
    void OnServerTreeSelChange(HWND hWnd);
    void OnRClickServerTree(HWND hWnd);

    void OnButtonAdd(void);
    void OnButtonEdit(void);
    void OnButtonRemove(void);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetServerInfoImageList(HIMAGELIST himl);
    void SetServerInfoConfigFile(LPCTSTR lpszFileName);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);
    void SetWindowRect(int x, int y, int width, int height);
};