//
// File: GameMainPage.h
//
//  游戏页面。该页面包括游戏窗口、游戏桌玩家列表、LOG信息窗口
//
#pragma once

#define GAME_MAIN_PAGE_CLASS_NAME           _T("_GAME_MAIN_PAGE_")

#define GMP_MIN_WIDTH                       900
#define GMP_MIN_HEIGHT                      540

#define GMP_USERLIST_WIDTH                  240
#define GMP_USERLIST_HEIGHT                 200

#define GMP_LOGWND_WIDTH                    GMP_USERLIST_WIDTH

#define GMP_CHATMSG_COMBO_WITH              GMP_USERLIST_WIDTH
#define GMP_CHATMSG_COMBO_HEIGHT            20

#define GMP_LIST_IMAGE_COLUMN_WIDTH         32
#define GMP_LIST_NAME_COLUMN_WIDTH          104
#define GMP_LIST_LEVEL_COLUMN_WIDTH         48
#define GMP_LIST_SCORE_COLUMN_WIDTH         56

#define GMP_LISTVIEW_CTRL_ID                1
#define GMP_RICHEDIT_CTRL_ID                2
#define GMP_COMBO_CHATMSG_ID                3

#define GMP_USERLIST_BACK_CLR               RGB(64,96,192)
#define GMP_LOGWND_BACK_CLR                 RGB(64,96,192)
#define GMP_USERLIST_TEXT_CLR               RGB(255,255,255)

#define GMP_DEF_LOGTEXT_CLR                 RGB(255,255,255)
#define GMP_LOGCLR_INOUT                    RGB(192,168,0)


typedef enum
{
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} LOG_TYPE;

class GameMainPage
{
    BOOL        m_bShow;

    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

    HWND        m_hUserList;
    HWND        m_hLogWnd;
    HWND        m_hChatMsgCombo;

    BOOL        m_bLogWndVisible;

    HWND        m_hComboEdit; // The EidtCtrl within Combobox
    WNDPROC     m_lpComboEditWndProc; // Original WndProc of Combobox

public:
    GameMainWnd m_GameMainWnd;

public:
    GameMainPage(void);
    ~GameMainPage(void);

    static ATOM GameMainPageRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK GameMainPageProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK MyComboEditProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void InitListCtrl(void);
    void InitComboCtrl(void);
    void InitRichEditCtrl(void);
    void CreateChildWindows(void);
    void OnSize(WPARAM wParam, LPARAM lParam);

    void OnNotify(WPARAM wParam, LPARAM lParam);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnChatMsgKeyDown(WPARAM wParam, LPARAM lParam);

    BOOL IsBlankString(LPCTSTR lpszText);

    void OnListviewNotify(WPARAM wParam, LPARAM lParam);
    void OnRicheditNotify(WPARAM wParam, LPARAM lParam);
    void OnRicheditNotifyRButtonDown(WPARAM wParam, LPARAM lParam);
    void OnRicheditNotifyKeyUp(WPARAM wParam, LPARAM lParam);
    void OnRicheditCopy(void);
    void OnComboNotify(HWND hCombo, UINT nEvent);

    void OnGameOver(WPARAM wParam, LPARAM lParam);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetWindowRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);

    void ListCtrlAddUser(int nUserId);
    void ListCtrlRemoveUser(int nUserId);
    void ListCtrlClear(void);

    void WriteLog(LPCTSTR lpszMsg);
    void WriteLog(LOG_TYPE type, LPCTSTR lpszMsg);
    void WriteLog(COLORREF clrText, LPCTSTR lpszMsg);
    void WriteLog(CHARFORMAT* charFmt, LPCTSTR lpszMsg);
    void ClearLog(void);

    void Init(BOOL bIsLookon);
    void SetCurentUserSeat(int seat);

    void GamerTakeSeat(int id, int seat);
    void GamerLeaveSeat(int id, int seat);
    void LookonTakeSeat(int id, int seat);
    void LookonLeaveSeat(int id, int seat);
    void GamerReady(int id, int seat);
};