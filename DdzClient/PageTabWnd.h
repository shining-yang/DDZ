//
// File: PageTabWnd.h
//
#pragma once

#define PAGE_TAB_WND_CLASS_NAME             _T("_PAGE_TAB_WND_")

#define PTW_BACK_CLR                        RGB(64,192,96)

#define PTW_RET_BUTTON_WIDTH                60
#define PTW_RET_BUTTON_HEIGHT               32

#define PTW_RET_BUTTON_ID                   1
#define PTW_TAB_CTRL_ID                     2


class PageTabWnd
{
    HWND        m_hTabCtrl;

    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

public:
    MyButton    m_btnReturn;

public:
    PageTabWnd(void);
    ~PageTabWnd(void);

public:
    static ATOM PageTabWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK PageTabWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnPaint(HDC hdc);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnNotify(WPARAM wParam, LPARAM lParam);
    void OnBtnReturn(void);
    void OnPageSelChanging(void);
    void OnPageSelChange(void);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetWindwRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);

    BOOL InsertTabPage(int index, int nImage, LPCTSTR lpszLabel, LPARAM lParam);
    BOOL RemoveTabPage(int index);

    HIMAGELIST SetTabImageList(HIMAGELIST himlTab);
    int  GetCurTabSel(void);
    void SetCurTabSel(int index);

    void InsertHomeTabPage(void);
    void InsertRoomTabPage(void);
    void InsertGameTabPage(void);
    void RemoveHomeTabPage(void);
    void RemoveRoomTabPage(void);
    void RemoveGameTabPage(void);
};