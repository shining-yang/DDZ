//
// CurPokerWnd.h
//
//      Current Poker Window
//
#pragma once

#define CUR_POKER_WND_CLASS_NAME    _T("_CUR_POKER_WND_")

#define INVALID_COORD               -1

#define SEL_CARDS_FRAME_WIDTH       4
#define SEL_CARDS_FRAME_CLR         RGB(255,255,0)

class CurPokerWnd
{
private:
    static const int       m_HSPACE = CPW_POKER_H_SPACE; // 横向排列时，牌与牌的间距
    static const int       m_VSPACE = CPW_POKER_V_SPACE; // 纵向排列时，牌与牌的间距
    static const int       m_POPSPACE = CPW_POKER_POPUP_SPACE; // 选中的牌弹出的间距

    static const int       m_CARDHEIGHT = POKER_BMP_UNIT_HEIGHT; // 扑克牌的高度
    static const int       m_CARDWIDTH = POKER_BMP_UNIT_WIDTH; // 扑克牌的宽度

private:
    // 当前牌，元素为0～53的扑克索引
    int             m_nCurPokerCards[PLAYER_MAX_CARDS_NUM];
    int             m_nCurPokerCardsNum;

    // 指示哪些牌被选中，元素取值0～20
    BOOL            m_bPokerCardSelected[PLAYER_MAX_CARDS_NUM];

private:
    HINSTANCE       m_hInstance;
    HWND            m_hWndParent;

private:
    UINT            m_nID;
    HWND            m_hWnd;

    // 扑克牌位图句柄（父窗口创建本窗口时传递参数，父窗口销毁前不删除该位图对象）
    HBITMAP         m_hPokerBMP;

    HBITMAP         m_hBkBitmap; // 保存父窗口的背景，用于绘制时避免闪烁。
    BOOL            m_bReloadBkBitmap; // 当尺寸变化时，指示重新获取父窗口背景

private:
    BOOL            m_bShowPoker;       // 显示扑克牌
    BOOL            m_bHorizontal;      // 扑克牌是否为水平排列

    BOOL            m_bIsLord;          // 该玩家是否为地主

    BOOL            m_bAllowPokerSelect; // 扑克牌是否可以被选择，（旁观者不能选择牌）

    BOOL            m_bLButtonDown;
    POINT           m_ptLButtonDown;

    BOOL            m_bLBtnDownAndMouseMove;
    POINT           m_ptMouseMove;

private:
    void OnPaint(HDC hdc);
    void SaveParentBackground(void);
    void SaveParentBackground(HDC parentDC);

    void PaintHorizontally(HDC hdc);
    void PaintVertically(HDC hdc);

    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y);

    void ClickCard(int x, int y);
    void SelectCardRange(int xStart, int xEnd);

    void GetCardRangeSelectRect(LPRECT lpRect);
    void AdjustWindowRect(void);

public:
    CurPokerWnd(void);
    ~CurPokerWnd(void);

public:
    static ATOM CurPokerWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK CurPokerWndProc(HWND, UINT, WPARAM, LPARAM);

public:
    HWND Create(DWORD dwStyle,
        int xCenter,
        int yCenter,
        HWND hWndParent,
        UINT nID,
        HINSTANCE hInstance,
        HBITMAP hPokerBMP,
        BOOL bShowPoker,
        BOOL bHorizontal);

    BOOL ShowPoker(BOOL bShow = TRUE);
    BOOL SetCards(int poker[], int num);
    BOOL GetCards(int poker[], int* num);
    BOOL GetSelectedCards(int poker[], int num, int* selnum);
    BOOL RemoveSelectedCards(void);
    BOOL RemoveCards(int poker[], int num);
    BOOL AddCards(int poker[], int num);
    BOOL SelectCardsFromIndex(int index[], int num);
    BOOL UnSelectAllCards(void);
    BOOL AllowPokerSelection(BOOL bAllow = TRUE);
    BOOL SetLord(BOOL bLord = TRUE);

    BOOL SetWindowCenterPos(int xCenter, int yCenter);

public:
    void ParentWndSizeChanged(void);

#ifdef PARENT_PAINT_CHILD
    void ParentPaintChild(HDC parentDC);
#endif
};