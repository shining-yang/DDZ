//
// File: GameTableWnd.cpp
//
//  游戏桌窗口
//
#include "stdafx.h"
#include "GameSeatWnd.h"
#include "GameTableWnd.h"


GameTableWnd::GameTableWnd(void)
{
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;
    m_nTableId = INVALID_TABLE;

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        m_nGamerId[i] = INVALID_USER_ID;
        m_bReady[i] = FALSE;
    }

    m_bGameStarted = FALSE;
}

GameTableWnd::~GameTableWnd(void)
{

}

ATOM GameTableWnd::GameTableWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpszClassName  = GAME_TABLE_WND_CLASS_NAME;
    wcex.lpfnWndProc    = GameTableWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;//(HBRUSH)(COLOR_WINDOW + 1);

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GameTableWnd::GameTableWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCT lpcs;
    GameTableWnd* lpWnd = (GameTableWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_COMMAND:
            lpWnd->OnCommand(wParam, lParam);
            break;

        case WM_SIZE:
            lpWnd->OnSize(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND GameTableWnd::Create(int x, int y, int cx, int cy, UINT nID, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_nTableId = nID;

    m_hWnd = CreateWindowEx(0,
        GAME_TABLE_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    m_SeatWnd[0].Create(X_SEAT0, Y_SEAT0, CX_SEAT, CY_SEAT, GAME_SEAT0_ID, m_hWnd, hInstance);
    m_SeatWnd[1].Create(X_SEAT1, Y_SEAT1, CX_SEAT, CY_SEAT, GAME_SEAT1_ID, m_hWnd, hInstance);
    m_SeatWnd[2].Create(X_SEAT2, Y_SEAT2, CX_SEAT, CY_SEAT, GAME_SEAT2_ID, m_hWnd, hInstance);

    return m_hWnd;
}

void GameTableWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    UINT nSeatId = (UINT)wParam;

    switch (nSeatId) {
        case GAME_SEAT0_ID:
        case GAME_SEAT1_ID:
        case GAME_SEAT2_ID:
            SendMessage(m_hWndParent, WM_USER_CLICK_SEAT, (WPARAM)m_nTableId, (LPARAM)nSeatId);
            break;
    }
}

void GameTableWnd::OnSize(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    m_SeatWnd[0].SetWindowRect(X_SEAT0, Y_SEAT0, CX_SEAT, CY_SEAT);
    m_SeatWnd[1].SetWindowRect(X_SEAT1, Y_SEAT1, CX_SEAT, CY_SEAT);
    m_SeatWnd[2].SetWindowRect(X_SEAT2, Y_SEAT2, CX_SEAT, CY_SEAT);
}

void GameTableWnd::PaintTableWnd(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right;
    int cy = rect.bottom;

    POINT ptOrg = {0, 0};
    ClientToScreen(m_hWnd, &ptOrg);
    ScreenToClient(m_hWndParent, &ptOrg);

    // 绘制游戏桌位图
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP oldmembmp;
    if (m_bGameStarted == FALSE) {
        oldmembmp = (HBITMAP)SelectObject(memdc, g_hbmpTable);
    } else {
        oldmembmp = (HBITMAP)SelectObject(memdc, g_hbmpTableStart);
    }

    BitBlt(hdc, ptOrg.x, ptOrg.y, cx, cy, memdc, 0, 0, SRCCOPY);
    SelectObject(memdc, oldmembmp);
    DeleteDC(memdc);

    //
    // 绘制玩家头像，昵称，准备图标
    //
    int nImage;
    LPCTSTR lpszName;
    int nCharNum;
    RECT rcName;

    // seat 0
    if (m_nGamerId[0] != INVALID_USER_ID) {
        // Fill the seat0 rect with default brush
        RECT rcSeat0 = {
            ptOrg.x + X_SEAT0,
            ptOrg.y + Y_SEAT0,
            ptOrg.x + X_SEAT0 + CX_SEAT,
            ptOrg.y + Y_SEAT0 + CY_SEAT
        };

        // use brush of the DC to fill the seat rect
        HBRUSH hbrDC = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        FillRect(hdc, &rcSeat0, hbrDC);
        SelectObject(hdc, hbrDC);

        // then draw user head icon
        nImage = GetUserImageIndex(m_nGamerId[0]);
        ImageList_Draw(g_himlUser32x32, nImage, hdc, ptOrg.x + X_SEAT0, ptOrg.y + Y_SEAT0, ILD_NORMAL);

        // name
        rcName.left     = ptOrg.x + X_SEAT0_NAME;
        rcName.top      = ptOrg.y + Y_SEAT0_NAME;
        rcName.right    = rcName.left + CX_SEAT0_NAME;
        rcName.bottom   = rcName.top + CY_SEAT0_NAME;

        lpszName = GetUserNameStr(m_nGamerId[0]);
        nCharNum = _tcslen(lpszName);
        DrawText(hdc, lpszName, nCharNum, &rcName, DT_WORDBREAK);

        // ready icon
        if ((m_bReady[0] == TRUE) && (m_bGameStarted != TRUE)) {
            DrawIcon(hdc, ptOrg.x + X_SEAT0_READY, ptOrg.y + Y_SEAT0_READY, g_hiconReady);
        }
    }

    // seat 1
    if (m_nGamerId[1] != INVALID_USER_ID) {
        // Fill the seat1 rect with default brush
        RECT rcSeat1 = {
            ptOrg.x + X_SEAT1,
            ptOrg.y + Y_SEAT1,
            ptOrg.x + X_SEAT1 + CX_SEAT,
            ptOrg.y + Y_SEAT1 + CY_SEAT
        };

        // use brush of the DC
        HBRUSH hbrDC = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        FillRect(hdc, &rcSeat1, hbrDC);
        SelectObject(hdc, hbrDC);

        // then draw user head icon
        nImage = GetUserImageIndex(m_nGamerId[1]);
        ImageList_Draw(g_himlUser32x32, nImage, hdc, ptOrg.x + X_SEAT1, ptOrg.y + Y_SEAT1, ILD_NORMAL);

        rcName.left     = ptOrg.x + X_SEAT1_NAME;
        rcName.top      = ptOrg.y + Y_SEAT1_NAME;
        rcName.right    = rcName.left + CX_SEAT1_NAME;
        rcName.bottom   = rcName.top + CY_SEAT1_NAME;

        lpszName = GetUserNameStr(m_nGamerId[1]);
        nCharNum = _tcslen(lpszName);
        DrawText(hdc, lpszName, nCharNum, &rcName, DT_WORDBREAK);

        if ((m_bReady[1] == TRUE) && (m_bGameStarted != TRUE)) {
            DrawIcon(hdc, ptOrg.x + X_SEAT1_READY, ptOrg.y + Y_SEAT1_READY, g_hiconReady);
        }
    }

    // seat 2
    if (m_nGamerId[2] != INVALID_USER_ID) {
        // Fill the seat2 rect with default brush
        RECT rcSeat2 = {
            ptOrg.x + X_SEAT2,
            ptOrg.y + Y_SEAT2,
            ptOrg.x + X_SEAT2 + CX_SEAT,
            ptOrg.y + Y_SEAT2 + CY_SEAT
        };

        // use brush of the DC
        HBRUSH hbrDC = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        FillRect(hdc, &rcSeat2, hbrDC);
        SelectObject(hdc, hbrDC);

        // then draw user head icon
        nImage = GetUserImageIndex(m_nGamerId[2]);
        ImageList_Draw(g_himlUser32x32, nImage, hdc, ptOrg.x + X_SEAT2, ptOrg.y + Y_SEAT2, ILD_NORMAL);

        rcName.left     = ptOrg.x + X_SEAT2_NAME;
        rcName.top      = ptOrg.y + Y_SEAT2_NAME;
        rcName.right    = rcName.left + CX_SEAT2_NAME;
        rcName.bottom   = rcName.top + CY_SEAT2_NAME;

        lpszName = GetUserNameStr(m_nGamerId[2]);
        nCharNum = _tcslen(lpszName);
        DrawText(hdc, lpszName, nCharNum, &rcName, DT_WORDBREAK);

        if ((m_bReady[2] == TRUE) && (m_bGameStarted != TRUE)) {
            DrawIcon(hdc, ptOrg.x + X_SEAT2_READY, ptOrg.y + Y_SEAT2_READY, g_hiconReady);
        }
    }

    // 显示游戏桌号
    RECT rcTableId = {
        ptOrg.x,
        ptOrg.y + CY_TABLE_BMP,
        ptOrg.x + CX_TABLE_BMP,
        ptOrg.y + CY_TABLE_BMP + TABLE_NUMBER_RECT_HT
    };

    TCHAR szTableId[16] = { 0 };
    _stprintf_s(szTableId, sizeof(szTableId) / sizeof(szTableId[0]), _T("- %d -"), m_nTableId + 1);
    nCharNum = _tcslen(szTableId);
    COLORREF clrOldText = SetTextColor(hdc, TABLE_ID_TEXT_COLOR);
    DrawText(hdc, szTableId, nCharNum, &rcTableId, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SetTextColor(hdc, clrOldText);

    // 在当前玩家的游戏桌画一边框
    if (m_nTableId == GetLocalUserTableId()) {
        RECT rcFrame = {
            ptOrg.x, ptOrg.y, ptOrg.x + cx, ptOrg.y + cy
        };

        HBRUSH hbrFrame = CreateSolidBrush(LOCAL_USER_TABLE_FRAME_CLR);
        if (hbrFrame != NULL) {
            FrameRect(hdc, &rcFrame, hbrFrame);
            DeleteObject(hbrFrame);
        }
    }
}

void GameTableWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, FALSE);
}

void GameTableWnd::ResetGameTable(void)
{
    m_bGameStarted = FALSE;

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        m_nGamerId[i] = INVALID_USER_ID;
        m_bReady[i] = FALSE;

        m_SeatWnd[i].ResetGameSeat();
    }
}

int GameTableWnd::GetGamerId(int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    return m_nGamerId[seat];
}

int* GameTableWnd::GetLookonIdsArrayPtr(int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    return m_SeatWnd[seat].GetLookonIdArrayPtr();
}

void GameTableWnd::GamerTakeSeat(int nUserId, int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    m_nGamerId[seat] = nUserId;
    m_bReady[seat] = FALSE;
    m_bGameStarted = FALSE;

    // 玩家进入座位时，复位其椅子相关数据，将旁观者ID清除，
    // 避免先前游戏中，该椅子的玩家退出后，仍有旁观者停留没有退出。
    m_SeatWnd[seat].ResetGameSeat();
}

void GameTableWnd::GamerLeaveSeat(int nUserId, int seat)
{
    assert(!IS_INVALID_SEAT(seat));

    if (m_nGamerId[seat] == nUserId) {
        m_nGamerId[seat] = INVALID_USER_ID;
        m_bReady[seat] = FALSE;
        m_bGameStarted = FALSE;

        // 玩家离开座位后，复位其椅子相关数据，将旁观者ID清除
        m_SeatWnd[seat].ResetGameSeat();
    }
}

void GameTableWnd::LookonTakeSeat(int nUserId, int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    m_SeatWnd[seat].LookonTakeSeat(nUserId);
}

void GameTableWnd::LookonLeaveSeat(int nUserId, int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    m_SeatWnd[seat].LookonLeaveSeat(nUserId);
}

void GameTableWnd::GamerGetReady(int seat, BOOL bReady /*= TRUE */)
{
    assert(!IS_INVALID_SEAT(seat));
    m_bReady[seat] = bReady;

    if (bReady == FALSE) {
        m_bGameStarted = FALSE;
    }
}

BOOL GameTableWnd::IsGamerReady(int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    return m_bReady[seat];
}

void GameTableWnd::GameStarted(BOOL bStart /*= TRUE*/)
{
    m_bGameStarted = bStart;
}

void GameTableWnd::SetAllowLookon(int seat, BOOL bAllow /*= TRUE*/)
{
    assert(!IS_INVALID_SEAT(seat));
    m_SeatWnd[seat].SetAllowLookon(bAllow);
}

