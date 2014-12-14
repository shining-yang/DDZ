//
// File: PageTabWnd.cpp
//
//  Contain the tab ctrl for page selection.
//
#include "stdafx.h"
#include "MyButton.h"
#include "PageTabWnd.h"


PageTabWnd::PageTabWnd(void)
{
    m_hWnd = NULL;
}

PageTabWnd::~PageTabWnd(void)
{

}

ATOM PageTabWnd::PageTabWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = PAGE_TAB_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = PageTabWndProc;
    wcex.style			    = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra		    = 0;
    wcex.cbWndExtra		    = 0;
    wcex.hInstance		    = hInstance;
    wcex.hIcon			    = NULL;
    wcex.hCursor		    = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	    = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName	    = NULL;
    wcex.hIconSm		    = NULL;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK PageTabWnd::PageTabWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    PageTabWnd* lpWnd = (PageTabWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_COMMAND:
            lpWnd->OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            lpWnd->OnNotify(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND PageTabWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWnd = CreateWindowEx(0,
        PAGE_TAB_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE | TCS_FOCUSNEVER | TCS_FIXEDWIDTH,
        x, y, cx, cy,
        hWndParent, NULL, hInstance, this);
    assert(m_hWnd != NULL);

    m_hTabCtrl = CreateWindowEx(0,
        WC_TABCONTROL,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        0, 0, cx - PTW_RET_BUTTON_WIDTH, cy,
        m_hWnd,
        (HMENU)PTW_TAB_CTRL_ID,
        hInstance, NULL);
    assert(m_hTabCtrl != NULL);

    SendMessage(m_hTabCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);

    m_btnReturn.Create(WS_CHILD | WS_VISIBLE,
        cx - PTW_RET_BUTTON_WIDTH,
        0,
        PTW_RET_BUTTON_WIDTH,
        cy,
        m_hWnd,
        PTW_RET_BUTTON_ID,
        hInstance);

    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    return m_hWnd;
}

void PageTabWnd::SetWindwRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);

    MoveWindow(m_hTabCtrl, 0, 0, cx - PTW_RET_BUTTON_WIDTH, cy, TRUE);

    m_btnReturn.SetWindowRect(cx - PTW_RET_BUTTON_WIDTH,
        (cy - PTW_RET_BUTTON_HEIGHT) / 2,
        PTW_RET_BUTTON_WIDTH,
        PTW_RET_BUTTON_HEIGHT);
}

void PageTabWnd::Show(BOOL bShow /*= TRUE*/)
{
    ShowWindow(m_hWnd, bShow ? TRUE : FALSE);
}

BOOL PageTabWnd::InsertTabPage(int index, int nImage, LPCTSTR lpszLabel, LPARAM lParam)
{
    TCITEM tci = { 0 };
    tci.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
    tci.pszText = (LPTSTR)lpszLabel;
    tci.cchTextMax = _tcslen(lpszLabel);
    tci.iImage = nImage;
    tci.lParam = lParam;

    return (index == TabCtrl_InsertItem(m_hTabCtrl, index, &tci)) ? TRUE : FALSE;
}

BOOL PageTabWnd::RemoveTabPage(int index)
{
    return TabCtrl_DeleteItem(m_hTabCtrl, index);
}

HIMAGELIST PageTabWnd::SetTabImageList(HIMAGELIST himlTab)
{
    return TabCtrl_SetImageList(m_hTabCtrl, himlTab);
}

int PageTabWnd::GetCurTabSel(void)
{
    return TabCtrl_GetCurSel(m_hTabCtrl);
}

void PageTabWnd::SetCurTabSel(int index)
{
    TabCtrl_SetCurSel(m_hTabCtrl, index);
}

void PageTabWnd::OnPaint(HDC hdc)
{
    UNREFERENCED_PARAMETER(hdc);

    //RECT rect;
    //GetClientRect(m_hWnd, &rect);

    //HBRUSH hbrush = CreateSolidBrush(PTW_BACK_CLR);
    //if (hbrush != NULL) {
    //    FillRect(hdc, &rect, hbrush);
    //    DeleteObject(hbrush);
    //}
}

void PageTabWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    UINT nCtrlId = LOWORD(wParam);

    switch (nCtrlId) {
        case PTW_RET_BUTTON_ID:
            OnBtnReturn();
            break;
    }
}

void PageTabWnd::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* nmhdr = (NMHDR*)lParam;

    switch (nmhdr->idFrom) {
    case PTW_TAB_CTRL_ID:
        switch (nmhdr->code) {
        case TCN_SELCHANGING:
            OnPageSelChanging();
            break;

        case TCN_SELCHANGE:
            OnPageSelChange();
            break;
        }
    }
}

void PageTabWnd::OnBtnReturn(void)
{
    int index = TabCtrl_GetCurSel(m_hTabCtrl);
    if (index > 0) { // 0 means the home page. never close it.
        TCITEM tci = { 0 };
        tci.mask = TCIF_PARAM;

        if (TabCtrl_GetItem(m_hTabCtrl, index, &tci) != FALSE) {
            SendMessage(m_hWndParent, WM_PAGE_RETURN, 0, tci.lParam);
        }
    }
}

void PageTabWnd::OnPageSelChanging(void)
{
    int index = TabCtrl_GetCurSel(m_hTabCtrl);
    if (index > -1) {
        TCITEM tci = { 0 };
        tci.mask = TCIF_PARAM;

        if (TabCtrl_GetItem(m_hTabCtrl, index, &tci) != FALSE) {
            SendMessage(m_hWndParent, WM_PAGE_SEL_CHANGING, 0, tci.lParam);
        }
    }
}

void PageTabWnd::OnPageSelChange(void)
{
    int index = TabCtrl_GetCurSel(m_hTabCtrl);
    if (index > -1) {
        TCITEM tci = { 0 };
        tci.mask = TCIF_PARAM;

        if (TabCtrl_GetItem(m_hTabCtrl, index, &tci) != FALSE) {
            SendMessage(m_hWndParent, WM_PAGE_SEL_CHANGE, 0, tci.lParam);
        }
    }
}

void PageTabWnd::InsertHomeTabPage(void)
{
    InsertTabPage(HOME_PAGE_TAB_INDEX, HOME_PAGE_TAB_IMAGE_INDEX, HOME_PAGE_TAB_TEXT, HOME_PAGE_TAB_PARAM);
}

void PageTabWnd::InsertRoomTabPage(void)
{
    InsertTabPage(ROOM_PAGE_TAB_INDEX, ROOM_PAGE_TAB_IMAGE_INDEX, ROOM_PAGE_TAB_TEXT, ROOM_PAGE_TAB_PARAM);
}

void PageTabWnd::InsertGameTabPage(void)
{
    InsertTabPage(GAME_PAGE_TAB_INDEX, GAME_PAGE_TAB_IMAGE_INDEX, GAME_PAGE_TAB_TEXT, GAME_PAGE_TAB_PARAM);
}

void PageTabWnd::RemoveHomeTabPage(void)
{
    RemoveTabPage(HOME_PAGE_TAB_INDEX);
}

void PageTabWnd::RemoveRoomTabPage(void)
{
    RemoveTabPage(ROOM_PAGE_TAB_INDEX);
}

void PageTabWnd::RemoveGameTabPage(void)
{
    RemoveTabPage(GAME_PAGE_TAB_INDEX);
}