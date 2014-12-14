//
// OutCardWnd.h
//
//  该窗口类用于显示玩家出的牌
//
#pragma once

#define MAX_OUT_CARDS_NUM           20

#define OUT_CARD_WND_CLASS_NAME    _T("_OUT_CARD_WND_")

// 扑克牌在出牌窗口中的位置，始终是垂直居中。本类只提供水平显示扑克。
#define CARD_ALIGN_CENTER           0
#define CARD_ALIGN_LEFT             1
#define CARD_ALIGN_RIGHT            2
#define CARD_ALIGN_MAX              3

// 在出牌窗口显示有关游戏的状态信息
typedef enum {
    GSI_NONE = 0,
    GSI_READY,
    GSI_SCORE0,
    GSI_SCORE1,
    GSI_SCORE2,
    GSI_SCORE3,
    GSI_PASS
} GAME_STAT_INFO;


class OutCardWnd
{
private:
    static const int       m_HSPACE = OCW_POKER_H_SPACE; // 横向排列时，牌与牌的间距
    static const int       m_CARDHEIGHT = POKER_BMP_UNIT_HEIGHT; // 扑克牌的高度
    static const int       m_CARDWIDTH = POKER_BMP_UNIT_WIDTH; // 扑克牌的宽度

private:
    BOOL            m_bShowPrevious;

    // 保存当前出的牌
    int             m_nPrevOutCards[MAX_OUT_CARDS_NUM];
    int             m_nPrevOutCardsNum;

    // 保存上一轮出的牌
    int             m_nCurOutCards[MAX_OUT_CARDS_NUM];
    int             m_nCurOutCardsNum;

    int             m_nOutCardsNum;

    int             m_nAlignment;
    BOOL            m_bShowPoker; // 指示是否显示扑克

    GAME_STAT_INFO  m_StatInfo;

private:
    HINSTANCE       m_hInstance;
    HWND            m_hWndParent;
    HWND            m_hWnd;
    
    // 扑克牌位图句柄（父窗口创建本窗口时传递参数，父窗口销毁前不删除该位图对象）
    HBITMAP         m_hPokerBMP;

    HBITMAP         m_hBkBitmap; // 保存父窗口的背景，用于绘制时避免闪烁。
    BOOL            m_bReloadBkBitmap; // 当尺寸变化时，指示重新获取父窗口背景

protected:
    void OnPaint(HDC hdc);
    void SaveParentBackground(void);
    void SaveParentBackground(HDC parentDC);

public:
    OutCardWnd(void);
    ~OutCardWnd(void);

public:
    static ATOM OutCardWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK OutCardWndProc(HWND, UINT, WPARAM, LPARAM);

public:
    HWND Create(DWORD dwStyle,
        int xCoord,
        int yCoord,
        int nAlign,
        HWND hWndParent,
        HINSTANCE hInstance,
        HBITMAP hPokerBMP);

    BOOL SetWindowCenterPos(int xCenter, int yCenter);
    BOOL ShowPoker(BOOL bShow = TRUE);
    BOOL SetCards(int poker[], int num);
    BOOL Clear(void);
    BOOL SetCardsAlign(int nAlign = 0);
    BOOL ShowPrevRound(BOOL bShowPrev = TRUE);
    BOOL SetGameStatInfo(GAME_STAT_INFO info);

public:
    void ParentWndSizeChanged(void);

#ifdef PARENT_PAINT_CHILD
    void ParentPaintChild(HDC parentDC);
#endif
};