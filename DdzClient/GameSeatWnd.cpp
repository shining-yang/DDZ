//
// File: GameSeatWnd.cpp
//
//  游戏椅子窗口（用于大厅窗口中显示）
//
#include "stdafx.h"
#include "GameSeatWnd.h"

GameSeatWnd::GameSeatWnd(void)
{
    m_bMouseEnter = FALSE;
    m_bLButtonDown = FALSE;
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;
    m_nSeatId = INVALID_SEAT;

    m_bAllowLookon = TRUE;

    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        m_nLookonIds[i] = INVALID_USER_ID;
    }
}

GameSeatWnd::~GameSeatWnd(void)
{

}

ATOM GameSeatWnd::GameSeatWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpszClassName  = GAME_SEAT_WND_CLASS_NAME;
    wcex.lpfnWndProc    = GameSeatWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_HAND);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;//(HBRUSH)(COLOR_WINDOW + 1);

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GameSeatWnd::GameSeatWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    GameSeatWnd* lpWnd = (GameSeatWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc =BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_MOUSEMOVE:
            lpWnd->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MOUSELEAVE:
            lpWnd->OnMouseLeave(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            lpWnd->OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONDOWN:
            lpWnd->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_RBUTTONUP:
            lpWnd->OnRButtonUp(LOWORD(lParam), HIWORD(lParam));
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND GameSeatWnd::Create(int x, int y, int cx, int cy, UINT nID, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_nSeatId = nID;

    m_hWnd = CreateWindowEx(0,
        GAME_SEAT_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);

    return m_hWnd;
}

void GameSeatWnd::OnPaint(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    if ((m_bMouseEnter == TRUE) && (m_bLButtonDown == TRUE)) {
        Draw3DRect(hdc, &rect, RGB(0,0,0), RGB(255,255,255));
    } else if (m_bMouseEnter == TRUE) {
        Draw3DRect(hdc, &rect, RGB(255,255,255), RGB(0,0,0));
    } else {
        COLORREF color = GAME_SEAT_WND_DEF_BG_COLR;
        HBRUSH hbrush = CreateSolidBrush(color);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, hbrush);
        FrameRect(hdc, &rect, hbrush);
        SelectObject(hdc, oldbrush);
        DeleteObject(hbrush);
    }
}

void GameSeatWnd::OnMouseMove(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (m_bAllowLookon == FALSE) { return; }

    if (m_bMouseEnter == FALSE) {
        m_bMouseEnter = TRUE;

        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        tme.dwFlags = TME_LEAVE;
        TrackMouseEvent(&tme);

        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void GameSeatWnd::OnMouseLeave(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (m_bAllowLookon == FALSE) { return; }

    m_bMouseEnter = FALSE;

    InvalidateRect(m_hWnd, NULL, FALSE);
}

void GameSeatWnd::OnLButtonDown(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (m_bAllowLookon == FALSE) { return; }

    if (m_bLButtonDown == FALSE) {
        m_bLButtonDown = TRUE;
        SetCapture(m_hWnd);
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void GameSeatWnd::OnLButtonUp(int x, int y)
{
    if (m_bAllowLookon == FALSE) { return; }

    if (m_bLButtonDown == TRUE) {
        m_bLButtonDown = FALSE;
        ReleaseCapture();

        RECT rect;
        GetClientRect(m_hWnd, &rect);

        POINT pt;
        pt.x = x;
        pt.y = y;

        if (PtInRect(&rect, pt)) {
            SendMessage(m_hWndParent, WM_COMMAND, (WPARAM)m_nSeatId, 0);
        }

        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void GameSeatWnd::OnRButtonUp(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    // ToDo:
    //  Notify PARENT window to track a pop-up menu or something
}

void GameSeatWnd::Draw3DRect(HDC hdc, RECT* lpRect, COLORREF clrLT, COLORREF clrRB)
{
    HPEN oldpen;

    HPEN hpenLT = CreatePen(PS_SOLID, 1, clrLT);
    HPEN hpenRB = CreatePen(PS_SOLID, 1, clrRB);

    oldpen = (HPEN)SelectObject(hdc, hpenLT);
    MoveToEx(hdc, lpRect->left, lpRect->top, NULL);
    LineTo(hdc, lpRect->right, lpRect->top);
    MoveToEx(hdc, lpRect->left, lpRect->top, NULL);
    LineTo(hdc, lpRect->left, lpRect->bottom);
    SelectObject(hdc, oldpen);

    oldpen = (HPEN)SelectObject(hdc, hpenRB);
    MoveToEx(hdc, lpRect->right - 1, lpRect->top, NULL);
    LineTo(hdc, lpRect->right - 1, lpRect->bottom);
    MoveToEx(hdc, lpRect->left, lpRect->bottom - 1, NULL);
    LineTo(hdc, lpRect->right, lpRect->bottom - 1);
    SelectObject(hdc, oldpen);

    DeleteObject(hpenLT);
    DeleteObject(hpenRB);
}

void GameSeatWnd::SetAllowLookon(BOOL bAllow /*= TRUE*/)
{
    m_bAllowLookon = bAllow;
}

BOOL GameSeatWnd::GetSeatAllowLookon(void)
{
    return m_bAllowLookon;
}

void GameSeatWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void GameSeatWnd::ResetGameSeat(void)
{
    m_bAllowLookon = TRUE; // default is watch-allowable

    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        m_nLookonIds[i] = INVALID_USER_ID;
    }
}

int* GameSeatWnd::GetLookonIdArrayPtr(void)
{
    return m_nLookonIds;
}

void GameSeatWnd::LookonTakeSeat(int id)
{
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (m_nLookonIds[i] == INVALID_USER_ID) {
            m_nLookonIds[i] = id;
            break;
        }
    }
}

void GameSeatWnd::LookonLeaveSeat(int id)
{
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (m_nLookonIds[i] == id) {
            m_nLookonIds[i] = INVALID_USER_ID;
            break;
        }
    }
}
