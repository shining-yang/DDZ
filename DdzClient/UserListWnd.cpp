//
// File: UserListWnd.cpp
//
//  List all users on the game servr (in the room).
//
#include "stdafx.h"
#include "UserListWnd.h"


UserListWnd::UserListWnd(void)
{
    m_hInstance = NULL;
    m_hWndParent = NULL;
    m_hWnd = NULL;
    m_bShow = TRUE;

    m_hListCtrl = NULL;
}

UserListWnd::~UserListWnd(void)
{

}

ATOM UserListWnd::UserListWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = USER_LIST_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = UserListWndProc;
    wcex.style			    = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra		    = 0;
    wcex.cbWndExtra		    = 0;
    wcex.hInstance		    = hInstance;
    wcex.hIcon			    = NULL;
    wcex.hCursor		    = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	    = NULL;//(HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	    = NULL;
    wcex.hIconSm		    = NULL;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK UserListWnd::UserListWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    UserListWnd* lpWnd = (UserListWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_SIZE:
            lpWnd->OnSize(wParam, lParam);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_COMMAND:
            //lpWnd->OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            lpWnd->OnNotify(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND UserListWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;

    m_hWnd = CreateWindowEx(0,
        USER_LIST_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    m_hListCtrl = CreateWindowEx(0,
        WC_LISTVIEW,
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
        LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS,
        ULW_LISTVIEW_OFFSET,
        ULW_LISTVIEW_OFFSET,
        cx - (2 * ULW_LISTVIEW_OFFSET),
        cy - (2 * ULW_LISTVIEW_OFFSET),
        m_hWnd,
        NULL,
        hInstance,
        NULL);
    assert(m_hListCtrl != NULL);

    InitListView();

    return m_hWnd;
}

void UserListWnd::Show(BOOL bShow /*= TRUE*/)
{
    if (m_bShow != bShow) {
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        m_bShow = bShow;
    }
}

BOOL UserListWnd::IsVisible(void)
{
    return m_bShow;
}

void UserListWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void UserListWnd::OnSize(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);
    if ((cx <= 0) || (cy <= 0)) { return; }

    MoveWindow(m_hListCtrl, ULW_LISTVIEW_OFFSET, ULW_LISTVIEW_OFFSET,
        cx - (2 * ULW_LISTVIEW_OFFSET), cy - (2 * ULW_LISTVIEW_OFFSET), TRUE);
}

void UserListWnd::OnPaint(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    HDC memdc = CreateCompatibleDC(hdc);
    assert(memdc != NULL);

    HBITMAP hbmp = CreateCompatibleBitmap(hdc, cx, cy);
    assert(hbmp != NULL);

    HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, hbmp);

    HBRUSH hbrush = CreateSolidBrush(ULW_LISTVIEW_BG_CLR);
    if (hbrush != NULL) {
        FillRect(memdc, &rect, hbrush);

        HPEN hpen = CreatePen(PS_SOLID, ULW_ROUND_FRAME_WIDTH, ULW_ROUND_FRAME_CLR);
        if (hpen != NULL) {
            HPEN oldpen = (HPEN)SelectObject(memdc, hpen);
            HBRUSH oldbrush = (HBRUSH)SelectObject(memdc, GetStockObject(NULL_BRUSH));

            // 绘制一个圆角边框
            RoundRect(memdc,
                rect.left + ULW_ROUND_FRAME_OFFSET,
                rect.top + ULW_ROUND_FRAME_OFFSET,
                rect.right - ULW_ROUND_FRAME_OFFSET,
                rect.bottom - ULW_ROUND_FRAME_OFFSET,
                ULW_ROUND_FRAME_ANGEL,
                ULW_ROUND_FRAME_ANGEL);

            SelectObject(memdc, oldbrush);
            SelectObject(memdc, oldpen);
            DeleteObject(hpen);
        }
        DeleteObject(hbrush);
    }

    BitBlt(hdc, 0, 0, cx, cy, memdc, 0, 0, SRCCOPY);

    SelectObject(memdc, oldbmp);
    DeleteObject(hbmp);
    DeleteDC(memdc);
}

void UserListWnd::InitListView(void)
{
    SendMessage(m_hListCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);

    ListView_SetBkColor(m_hListCtrl, ULW_LISTVIEW_BG_CLR);
    ListView_SetTextColor(m_hListCtrl, ULW_LISTVIEW_TEXT_CLR);
    ListView_SetTextBkColor(m_hListCtrl, ULW_LISTVIEW_BG_CLR);

    ListView_SetExtendedListViewStyle(m_hListCtrl,
        LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES | LVS_EX_DOUBLEBUFFER);

    ListView_SetImageList(m_hListCtrl, g_himlUser32x32, LVSIL_NORMAL);
    ListView_SetImageList(m_hListCtrl, g_himlUser16x16, LVSIL_SMALL);

    struct _lv_colum_data {
        LPTSTR  label;
        int     width;
        int     format;
    } lvcolumn[] = {
        { _T(""),       ULW_STATUS_COLUMN_WIDTH,    LVCFMT_LEFT },
        { _T(""),       ULW_IMAGE_COLUMN_WIDTH,     LVCFMT_LEFT },
        { _T("昵称"),   ULW_NAME_COLUMN_WIDTH,      LVCFMT_LEFT },
        { _T("级别"),   ULW_LEVEL_COLUMN_WIDTH,     LVCFMT_LEFT },
        { _T("积分"),   ULW_SCORE_COLUMN_WIDTH,     LVCFMT_LEFT }
    };

    for (int i = 0; i < (sizeof(lvcolumn) / sizeof(lvcolumn[0])); i++) {
        LVCOLUMN lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
        lvc.pszText = lvcolumn[i].label;
        lvc.cchTextMax = _tcslen(lvcolumn[i].label);
        lvc.cx = lvcolumn[i].width;
        lvc.fmt = lvcolumn[i].format;

        ListView_InsertColumn(m_hListCtrl, i, &lvc);
    }
}

int UserListWnd::InsertUserItem(int nUserId)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.iImage = I_IMAGECALLBACK;
    lvi.lParam = (LPARAM)nUserId;

    return ListView_InsertItem(m_hListCtrl, &lvi);
}

BOOL UserListWnd::RemoveUserItem(int nUserId)
{
    int nCount = ListView_GetItemCount(m_hListCtrl);

    LVITEM lvi = { 0 };

    for (int i = 0; i < nCount; i++) {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;

        if (ListView_GetItem(m_hListCtrl, &lvi) == TRUE) {
            if (lvi.lParam == nUserId) {
                return ListView_DeleteItem(m_hListCtrl, i);
            }
        }
    }

    return FALSE;
}

BOOL UserListWnd::RemoveAllUserItems(void)
{
    return ListView_DeleteAllItems(m_hListCtrl);
}

void UserListWnd::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* nmhdr = (NMHDR*)lParam;
    NMLVDISPINFO* lvdi = (NMLVDISPINFO*)lParam;

    int nUserId = lvdi->item.lParam;

    switch (nmhdr->code) {
        case LVN_GETDISPINFO:
            switch (lvdi->item.iSubItem) { // 设置列的数据
                case 0:
                    lvdi->item.mask = LVIF_IMAGE;
                    lvdi->item.iImage = GetUserImageIndex(nUserId);
                    break;
                
                case 1:
                    lvdi->item.mask = LVIF_IMAGE;
                    lvdi->item.iImage = GetUserStatusImageIndex(nUserId);
                    break;

                case 2:
                    lvdi->item.mask = LVIF_TEXT;
                    lvdi->item.pszText = (LPTSTR)GetUserNameStr(nUserId);
                    break;

                case 3:
                    lvdi->item.mask = LVIF_TEXT;
                    lvdi->item.pszText = (LPTSTR)GetUserLevelStr(nUserId);
                    break;

                case 4:
                    lvdi->item.mask = LVIF_TEXT;
                    _stprintf_s(lvdi->item.pszText, lvdi->item.cchTextMax, _T("%d"), GetUserScore(nUserId));
                    break;
            }
            break;
    }
}

void UserListWnd::UpdateWnd(void)
{
    if (m_bShow == TRUE) {
        UpdateWindow(m_hWnd);
        //InvalidateRect(m_hWnd, NULL, FALSE);
    }
}
