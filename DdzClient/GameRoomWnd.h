//
// File: GameRoomWnd.h
//
#pragma once

#define GAME_ROOM_WND_CLASS_NAME        _T("_GAME_ROOME_WND_")

#define GAME_ROOM_WND_DEF_BG_COLR       RGB(81,113,158)

class GameRoomWnd
{
    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

    BOOL        m_bShow;
    int         m_nVScrollPos;
    int         m_nWholeDrawHeight; // 窗口整个画布高度，客户区高度可能小于画布高度

    GameTableWnd     m_TableWnd[GAME_TABLE_NUM];

public:
    GameRoomWnd(void);
    ~GameRoomWnd(void);
    static ATOM GameRoomWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK GameRoomWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnPaint(HDC hdc);
    void OnSize(WPARAM wParam, LPARAM lParam);
    void OnVScroll(WPARAM wParam, LPARAM lParam);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetWindowRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);

    int  GetGamerId(int table, int seat);
    int* GetLookonIdsArrayPtr(int table, int seat);

    void GamerTakeSeat(int nUserId, int table, int seat);
    void GamerLeaveSeat(int nUserId, int table, int seat);
    void LookonTakeSeat(int nUserId, int table, int seat);
    void LookonLeaveSeat(int nUserId, int table, int seat);

    BOOL IsGamerReady(int table, int seat);
    void GamerGetReady(int table, int seat, BOOL bReady = TRUE);
    void TableGameStarted(int table, BOOL bStart = TRUE);
    void SetAllowLookon(int table, int seat, BOOL bAllow);

    void UpdateWnd(void);
    void ResetGameRoom(void);
};