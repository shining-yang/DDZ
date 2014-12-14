//
// File: GameSeatWnd.h
//
#pragma once

#define GAME_SEAT_WND_CLASS_NAME            _T("_GAME_SEAT_WND_")
#define GAME_SEAT_WND_DEF_BG_COLR           RGB(81,113,158)

class GameSeatWnd
{
    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

    BOOL        m_bMouseEnter;
    BOOL        m_bLButtonDown;

    int         m_nSeatId; // CTRL id
    BOOL        m_bAllowLookon;

    int         m_nLookonIds[MAX_LOOKON_NUM_PER_SEAT];

public:
    GameSeatWnd(void);
    ~GameSeatWnd(void);
    static ATOM GameSeatWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK GameSeatWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void Draw3DRect(HDC hdc, RECT* lpRect, COLORREF clrLT, COLORREF clrRB);

    void OnPaint(HDC hdc);
    void OnMouseMove(int x, int y);
    void OnMouseLeave(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnRButtonUp(int x, int y);

public:
    HWND Create(int x, int y, int cx, int cy, UINT nID, HWND hWndParent, HINSTANCE hInstance);
    void SetAllowLookon(BOOL bAllow = TRUE);
    BOOL GetSeatAllowLookon(void);
    void SetWindowRect(int x, int y, int cx, int cy);

    void LookonTakeSeat(int id);
    void LookonLeaveSeat(int id);

    int* GetLookonIdArrayPtr(void);
    void ResetGameSeat(void);
};
