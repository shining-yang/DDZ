//
// File: UserListWnd.h
//
#pragma once

#define USER_LIST_WND_CLASS_NAME            _T("_USER_LIST_WND_")

#define ULW_LISTVIEW_OFFSET                 10
#define ULW_ROUND_FRAME_OFFSET              4
#define ULW_ROUND_FRAME_WIDTH               4
#define ULW_ROUND_FRAME_ANGEL               16
#define ULW_WINDOW_BG_CLR                   RGB(70,180,70)
#define ULW_ROUND_FRAME_CLR                 RGB(32,64,192)
#define ULW_LISTVIEW_BG_CLR                 ULW_WINDOW_BG_CLR
#define ULW_LISTVIEW_TEXT_CLR               RGB(255,255,255)

// List-view column width
#define ULW_STATUS_COLUMN_WIDTH             32
#define ULW_IMAGE_COLUMN_WIDTH              32
#define ULW_NAME_COLUMN_WIDTH               112
#define ULW_LEVEL_COLUMN_WIDTH              48
#define ULW_SCORE_COLUMN_WIDTH              56

class UserListWnd
{
    HINSTANCE   m_hInstance;
    HWND        m_hWndParent;
    HWND        m_hWnd;
    BOOL        m_bShow;

    HWND        m_hListCtrl;

public:
    UserListWnd(void);
    ~UserListWnd(void);

public:
    static ATOM UserListWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK UserListWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnSize(WPARAM wParam, LPARAM lParam);
    void OnNotify(WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc);

    void InitListView(void);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);
    void SetWindowRect(int x, int y, int cx, int cy);

    int  InsertUserItem(int nUserId);
    BOOL RemoveUserItem(int nUserId);
    BOOL RemoveAllUserItems(void);

    void UpdateWnd(void);
};