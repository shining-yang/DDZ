//
// File: GameRoomWnd.cpp
//
//  游戏大厅窗口
//
#include "stdafx.h"
#include "GameSeatWnd.h"
#include "GameTableWnd.h"
#include "GameRoomWnd.h"



GameRoomWnd::GameRoomWnd(void)
{
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;
    m_bShow = TRUE;
    m_nWholeDrawHeight = 0;
    m_nVScrollPos = 0;
}

GameRoomWnd::~GameRoomWnd(void)
{

}

ATOM GameRoomWnd::GameRoomWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpszClassName  = GAME_ROOM_WND_CLASS_NAME;
    wcex.lpfnWndProc    = GameRoomWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;//(HBRUSH)(COLOR_WINDOW + 1);

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GameRoomWnd::GameRoomWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    GameRoomWnd* lpWnd = (GameRoomWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_VSCROLL:
            lpWnd->OnVScroll(wParam, lParam);
            break;

        case WM_SIZE:
            lpWnd->OnSize(wParam, lParam);
            break;

        case WM_USER_CLICK_SEAT:
            SendMessage(lpWnd->m_hWndParent, WM_USER_CLICK_SEAT, wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND GameRoomWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;

    m_hWnd = CreateWindowEx(0,
        GAME_ROOM_WND_CLASS_NAME, 
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    m_nWholeDrawHeight = cy;

    for (int i = 0; i < GAME_TABLE_NUM; i++) {
        m_TableWnd[i].Create(0, 0, 0, 0, (UINT)i, m_hWnd, hInstance);
    }

    return m_hWnd;
}

void GameRoomWnd::OnPaint(HDC hdc)
{
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);

    RECT rcWholeDraw;
    CopyRect(&rcWholeDraw, &rcClient);

    if (rcWholeDraw.bottom < m_nWholeDrawHeight) {
        rcWholeDraw.bottom = m_nWholeDrawHeight;
    }

    // 创建要绘制的矩形区域
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP membmp = CreateCompatibleBitmap(hdc, rcWholeDraw.right, rcWholeDraw.bottom);
    HBITMAP oldmembmp = (HBITMAP)SelectObject(memdc, membmp);

    // 刷底色
    HBRUSH brush = CreateSolidBrush(GAME_ROOM_WND_DEF_BG_COLR);
    FillRect(memdc, &rcWholeDraw, brush);

    // 设置绘制时默认使用的GDI环境。要求子窗口（椅子窗口）在MEMDC上绘制
    HBRUSH oldbrush = (HBRUSH)SelectObject(memdc, brush);
    int mode = SetBkMode(memdc, TRANSPARENT);
    COLORREF txtclr = SetTextColor(memdc, RGB(255,255,255));
    HFONT hfont = (HFONT)SelectObject(memdc, GetStockObject(DEFAULT_GUI_FONT));

    for (int i = 0; i < GAME_TABLE_NUM; i++) {
        m_TableWnd[i].PaintTableWnd(memdc);
    }

    SelectObject(memdc, hfont);
    SetTextColor(memdc, txtclr);
    SetBkMode(memdc, mode);

    // 最后，将MEMDC刷向屏幕
    BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, memdc, 0, 0, SRCCOPY);

    SelectObject(memdc, oldbrush);
    SelectObject(memdc, oldmembmp);

    DeleteObject(brush);
    DeleteObject(membmp);
    DeleteDC(memdc);
}

void GameRoomWnd::OnSize(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);

    if ((cx <= 0) || (cy <= 0)) { return; }

    if (cx < CX_TABLE_BMP) { return; } // avoid dividing by 0 later

    // 尺寸发生变化后，重新计算整个画布的高度
    int nColumn = cx / CX_TABLE_BMP;
    int nTableHeight = CY_TABLE_BMP + TABLE_NUMBER_RECT_HT;
    m_nWholeDrawHeight = ((GAME_TABLE_NUM + (nColumn - 1)) / nColumn) * nTableHeight;

    // 设置子窗口尺寸
    int nRemain = cx % CX_TABLE_BMP;
    int nOffset = nRemain / nColumn;

    for (int i = 0; i < GAME_TABLE_NUM; i++) {
        int nCol = i % nColumn;
        int nRow = i / nColumn;

        m_TableWnd[i].SetWindowRect(
            nCol * (nOffset + CX_TABLE_BMP),
            nRow * nTableHeight,
            CX_TABLE_BMP,
            CY_TABLE_BMP + TABLE_NUMBER_RECT_HT);
    }

    // 设置滚动条范围
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    si.nPos = m_nVScrollPos;
    si.nMin = 0;
    si.nMax = m_nWholeDrawHeight;
    si.nPage = cy;
    SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

    if (m_nVScrollPos > 0) {
        int nVScroll = max(0, min(m_nVScrollPos, (int)(si.nMax - si.nPage - 1)));
        ScrollWindow(m_hWnd, 0, -nVScroll, NULL, NULL);
        UpdateWindow(m_hWnd);
    }
}

void GameRoomWnd::OnVScroll(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    int code = LOWORD(wParam);
    //int pos = HIWORD(wParam);

    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(m_hWnd, SB_VERT, &si);

    int yPos = si.nPos;

    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int cy = rect.bottom;
    int nScrollPerLine = (CY_TABLE_BMP + TABLE_NUMBER_RECT_HT); // 每行所占的像素
    int nScrollPerPage = (cy / nScrollPerLine) * nScrollPerLine; // 取整

    switch (code) {
        case SB_LINEDOWN:
            si.nPos += nScrollPerLine;
            break;
        case SB_LINEUP:
            si.nPos -= nScrollPerLine;
            break;
        case SB_PAGEDOWN:
            si.nPos += nScrollPerPage;
            break;
        case SB_PAGEUP:
            si.nPos -= nScrollPerPage;
            break;
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;
        case SB_TOP:
            si.nPos = si.nMin;
            break;
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;
        case SB_THUMBPOSITION:
        case SB_ENDSCROLL:
        default:
            return;
    }

    si.nPos = max(0, si.nPos);
    si.nPos = min(si.nPos, si.nMax);

    si.fMask = SIF_POS;
    SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

    si.fMask = SIF_POS;
    GetScrollInfo(m_hWnd, SB_VERT, &si);
    if (yPos != si.nPos) {
        m_nVScrollPos = si.nPos;
        ScrollWindow(m_hWnd, 0, (yPos - si.nPos), NULL, NULL);
        UpdateWindow (m_hWnd);
    }
}

void GameRoomWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void GameRoomWnd::Show(BOOL bShow /*= TRUE*/)
{
    if (m_bShow != bShow) {
        m_bShow = bShow;
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
    }
}

BOOL GameRoomWnd::IsVisible(void)
{
    return m_bShow;
}

//====================
// Game room data.
//====================

void GameRoomWnd::UpdateWnd(void)
{
    if (m_bShow == TRUE) {
        //UpdateWindow(m_hWnd);
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void GameRoomWnd::ResetGameRoom(void)
{
    for (int i = 0; i < GAME_TABLE_NUM; i++) {
        m_TableWnd[i].ResetGameTable();
    }
}

int GameRoomWnd::GetGamerId(int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    return m_TableWnd[table].GetGamerId(seat);
}

int* GameRoomWnd::GetLookonIdsArrayPtr(int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    return m_TableWnd[table].GetLookonIdsArrayPtr(seat);
}

void GameRoomWnd::GamerTakeSeat(int nUserId, int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].GamerTakeSeat(nUserId, seat);
}

void GameRoomWnd::GamerLeaveSeat(int nUserId, int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].GamerLeaveSeat(nUserId, seat);
}

void GameRoomWnd::GamerGetReady(int table, int seat, BOOL bReady /*= TRUE */)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].GamerGetReady(seat, bReady);
}

void GameRoomWnd::TableGameStarted(int table, BOOL bStart /*= TRUE*/)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].GameStarted(bStart);
}

void GameRoomWnd::LookonTakeSeat(int nUserId, int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].LookonTakeSeat(nUserId, seat);
}

void GameRoomWnd::LookonLeaveSeat(int nUserId, int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].LookonLeaveSeat(nUserId, seat);
}

void GameRoomWnd::SetAllowLookon(int table, int seat, BOOL bAllow)
{
    assert(!IS_INVALID_TABLE(table));
    m_TableWnd[table].SetAllowLookon(seat, bAllow);
}

BOOL GameRoomWnd::IsGamerReady(int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    return m_TableWnd[table].IsGamerReady(seat);
}