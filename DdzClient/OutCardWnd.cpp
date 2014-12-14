//
// OutCardWnd.cpp
//
//  该窗口类用于显示玩家出的牌
//
#include "stdafx.h"
#include "OutCardWnd.h"


// 构造函数
OutCardWnd::OutCardWnd(void)
{
    for (int i = 0; i < MAX_OUT_CARDS_NUM; i++) {
        m_nPrevOutCards[i] = POKER_BACK_INDEX;
        m_nCurOutCards[i] = POKER_BACK_INDEX;
    }

    m_nCurOutCardsNum = 0;
    m_nPrevOutCardsNum = 0;
    m_nOutCardsNum = 0;

    m_hInstance = NULL;
    m_hWndParent = NULL;
    m_hWnd = NULL;

    m_hPokerBMP = NULL;
    m_hBkBitmap = NULL;
    m_bReloadBkBitmap = FALSE;
    m_nAlignment = CARD_ALIGN_CENTER;

    m_bShowPoker = TRUE;
    m_StatInfo = GSI_NONE;
}

// 析构函数
OutCardWnd::~OutCardWnd(void)
{
}

// 窗口类注册函数
ATOM OutCardWnd::OutCardWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = OUT_CARD_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = OutCardWndProc;
    wcex.style			    = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.cbClsExtra		    = 0;
    wcex.cbWndExtra		    = 0;
    wcex.hInstance		    = hInstance;
    wcex.hIcon			    = NULL;
    wcex.hCursor		    = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	    = NULL;
    wcex.lpszMenuName	    = NULL;
    wcex.hIconSm		    = NULL;

    return RegisterClassEx(&wcex);
}

// 窗口处理函数
LRESULT
CALLBACK
OutCardWnd::OutCardWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    OutCardWnd* lpWnd = (OutCardWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            if (lpWnd->m_hBkBitmap != NULL) {
                DeleteObject(lpWnd->m_hBkBitmap);
                lpWnd->m_hBkBitmap = NULL;
            }
            break;

        case WM_SIZE:
            // 本窗口尺寸或位置发生变化，需要重新获取本窗口所在区域的父窗口背景
            lpWnd->m_bReloadBkBitmap = TRUE;
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SendMessage(lpWnd->m_hWndParent, nMsg, wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

//
// 创建窗口
//
// 调用者在创建出牌窗口时，不传递其客户矩形，而是传递一个坐标点和一个对齐标志。
// 如果对齐标志为居中，说明该坐标点为窗口的中心；如果为左对齐，则说明该坐标点
// 在矩形的最左边垂直中间点；如果为右对齐，则表示该坐标点为矩形最右边垂直中间点。
//
// 创建函数根据该坐标点，创建一个能容纳最多扑克牌的矩形窗口。
//
HWND OutCardWnd::Create(DWORD dwStyle, int xCoord, int yCoord, int nAlign,
                        HWND hWndParent, HINSTANCE hInstance, HBITMAP hPokerBMP)
{
    int x, y;
    int nMaxWidth, nMaxHeight;

    // 设置窗口矩形为容纳最多扑克牌的矩形
    nMaxHeight = m_CARDHEIGHT;
    nMaxWidth = (MAX_OUT_CARDS_NUM - 1) * m_HSPACE + m_CARDWIDTH;

    if (nAlign == CARD_ALIGN_LEFT) { // 左对齐
        x = xCoord;
    } else if (nAlign == CARD_ALIGN_RIGHT) { // 右对齐
        x = xCoord - nMaxWidth;
    } else { // 居中对齐
        x = xCoord - nMaxWidth / 2;
    }

    y = yCoord - nMaxHeight / 2;

    m_hWnd = CreateWindowEx(0,
        OUT_CARD_WND_CLASS_NAME,
        _T(""),
        dwStyle,
        x,
        y,
        nMaxWidth,
        nMaxHeight,
        hWndParent,
        NULL,
        hInstance,
        this);

    m_nAlignment = nAlign;
    m_hInstance = hInstance;
    m_hWndParent = hWndParent;
    m_hPokerBMP = hPokerBMP;

    return m_hWnd;
}

// 正常显示或显示背面（不允许旁观时，调用此函数显示扑克牌背面）
BOOL OutCardWnd::ShowPoker(BOOL bShow /*= TRUE*/)
{
    if (m_bShowPoker != bShow) {
        m_bShowPoker = bShow;

        if (m_nCurOutCardsNum > 0) {
#ifdef PARENT_PAINT_CHILD
            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
        }
    }
    return TRUE;
}

// 设置扑克牌在矩形对齐方式（要求：上家左对齐，当前玩家居中对齐，下家右对齐）
BOOL OutCardWnd::SetCardsAlign(int nAlign /*= 0*/)
{
    if ((nAlign >= 0) && (nAlign < CARD_ALIGN_MAX)) {
        if (m_nAlignment != nAlign) {
            m_nAlignment = nAlign;

#ifdef PARENT_PAINT_CHILD
            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
        }
    }
    return TRUE;
}

// 父窗口的尺寸已经改变，设置标志以指示绘制窗口时，需要重新获取父窗口背景
void OutCardWnd::ParentWndSizeChanged(void)
{
    m_bReloadBkBitmap = TRUE;
}

// 显示上一轮出的牌
BOOL OutCardWnd::ShowPrevRound(BOOL bShowPrev /*= TRUE*/)
{
    m_bShowPrevious = bShowPrev;
    
#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 设置扑克牌数据（出牌）
BOOL OutCardWnd::SetCards(int poker[], int num)
{
    // 保存原来的牌作为上一轮
    for (int i = 0; i < m_nOutCardsNum; i++) {
        m_nPrevOutCards[i] = m_nCurOutCards[i];
    }

    m_nPrevOutCardsNum = m_nOutCardsNum;

    if ((num == 0) && (m_nCurOutCardsNum == 0)) {
        // 保存本轮出牌的个数，下次出牌前，赋值给上一轮牌数据
        m_nOutCardsNum = m_nCurOutCardsNum;
        return TRUE;
    }

    // 接收新来的牌
    for (int i = 0; i < num; i++) {
        m_nCurOutCards[i] = poker[i];
    }

    m_nCurOutCardsNum = num;

    // 保存本轮出牌的个数，下次出牌前，赋值给上一轮牌数据
    m_nOutCardsNum = m_nCurOutCardsNum;

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 清空窗口
BOOL OutCardWnd::Clear(void)
{
    // 保存原来的牌作为上一轮
    for (int i = 0; i < m_nCurOutCardsNum; i++) {
        m_nPrevOutCards[i] = m_nCurOutCards[i];
    }

    m_nPrevOutCardsNum = m_nCurOutCardsNum;

    m_StatInfo = GSI_NONE;
    m_nCurOutCardsNum = 0;

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 保存父窗口背景。父窗口尺寸变化时需要重新保存。
void OutCardWnd::SaveParentBackground(void)
{
    // 取父窗口DC
    HDC parentDC = GetDC(m_hWndParent);
    assert(parentDC != NULL);

    SaveParentBackground(parentDC);

    // 释放父窗口DC
    ReleaseDC(m_hWndParent, parentDC);
}

// 保存父窗口背景
void OutCardWnd::SaveParentBackground(HDC parentDC)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;
    assert((cx != 0) && (cy != 0));

    POINT pt = { rect.left, rect.top };
    ClientToScreen(m_hWnd, &pt);
    ScreenToClient(m_hWndParent, &pt);

    // 创建要画的位图
    if (m_hBkBitmap == NULL) { 
        m_hBkBitmap = CreateCompatibleBitmap(parentDC, cx, cy);
        assert(m_hBkBitmap != NULL);
    } else {
        // 曾经创建过该位图，但窗口尺寸已经变化，需要重新创建该位图
        BITMAP bi = { 0 };
        GetObject(m_hBkBitmap, sizeof(BITMAP), &bi);

        if ((bi.bmWidth != cx) && (bi.bmHeight != cy)) {
            DeleteObject(m_hBkBitmap);
            m_hBkBitmap = CreateCompatibleBitmap(parentDC, cx, cy);
            assert(m_hBkBitmap != NULL);
        }
    }

    // 创建与父窗口兼容DC
    HDC memDC = CreateCompatibleDC(parentDC);
    assert(memDC != NULL);

    // 选中要画的位图
    HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, m_hBkBitmap);

    // 将父窗口的背景画到要保存的位图
    BitBlt(memDC, 0, 0, cx, cy, parentDC, pt.x, pt.y, SRCCOPY);

    // 选择旧的位图。新的位图已经创建到 m_hBkBitmap
    SelectObject(memDC, hOldBmp);

    // 删除创建的兼容DC
    DeleteDC(memDC);
}

// 绘制窗口
void OutCardWnd::OnPaint(HDC hdc)
{
#ifdef PARENT_PAINT_CHILD
    UNREFERENCED_PARAMETER(hdc);
    // Parent window will paint me. Do nothing...
#else
    if (m_bReloadBkBitmap == TRUE) {
        SaveParentBackground();
        m_bReloadBkBitmap = FALSE;
    }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int nCardsNum = m_nCurOutCardsNum;
    int* pCardsData = m_nCurOutCards;

    if (m_bShowPrevious == TRUE) {
        nCardsNum = m_nPrevOutCardsNum;
        pCardsData = m_nPrevOutCards;
    }

    int nPokersLen = m_HSPACE * (nCardsNum - 1) + m_CARDWIDTH;

    int nStartX = 0;
    int nStartY = 0;

    nStartY = (cy - m_CARDHEIGHT) / 2;
    if (nStartY < 0) { nStartY = 0; };

    if (m_nAlignment == CARD_ALIGN_CENTER) {
        nStartX = (cx - nPokersLen) / 2;
    } else if (m_nAlignment == CARD_ALIGN_LEFT) {
        nStartX = rect.left;
    } else if (m_nAlignment == CARD_ALIGN_RIGHT) {
        nStartX = rect.right - nPokersLen;
    }

    if (nStartX < 0) { nStartX = 0; }

    // 创建内存缓冲DC及其位图
    HDC memDC = CreateCompatibleDC(hdc);
    assert(memDC != NULL);

    HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, cx, cy);
    assert(hMemBmp != NULL);

    HBITMAP hOldMemBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

    // 先画父窗口背景
    HDC tempDC = CreateCompatibleDC(hdc);
    assert(tempDC != NULL);

    HBITMAP hOldTempBmp = (HBITMAP)SelectObject(tempDC, m_hBkBitmap);

    BitBlt(memDC, 0, 0, cx, cy, tempDC, 0, 0, SRCCOPY);

    // 再画扑克牌
    HDC pokerDC = CreateCompatibleDC(hdc);
    assert(pokerDC != NULL);

    HBITMAP hOldPokerBmp = (HBITMAP)SelectObject(pokerDC, m_hPokerBMP);

    int nCardBmpIndex;
    for (int i = 0; i < nCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(pCardsData[i]);
        } else {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
        }

        BitBlt(memDC,
            nStartX + i * m_HSPACE,
            nStartY,
            m_CARDWIDTH,
            m_CARDHEIGHT,
            pokerDC,
            nCardBmpIndex % 13 * m_CARDWIDTH,
            nCardBmpIndex / 13 * m_CARDHEIGHT,
            SRCCOPY);
    }

    if (m_bShowPrevious != TRUE) {
        if (m_StatInfo != GSI_NONE) {
            int nImageIndex = (int)(m_StatInfo - GSI_READY);
            int xImageStart = 0;
            int yImageStart = (cy - CY_GAME_INFO_IMAGE) / 2;

            if (m_nAlignment == CARD_ALIGN_CENTER) {
                xImageStart = (cx - CX_GAME_INFO_IMAGE) / 2;
            } else if (m_nAlignment == CARD_ALIGN_LEFT) {
                xImageStart = 0;
            } else if (m_nAlignment == CARD_ALIGN_RIGHT) {
                xImageStart = cx - CX_GAME_INFO_IMAGE;
            }

            ImageList_Draw(g_himlGameInfo, nImageIndex, memDC, xImageStart, yImageStart, ILD_NORMAL);
        }
    }

    BitBlt(hdc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);

    // 还原及释放资源
    SelectObject(tempDC, hOldTempBmp);
    SelectObject(pokerDC, hOldPokerBmp);
    SelectObject(memDC, hOldMemBmp);

    DeleteObject(hMemBmp);

    DeleteDC(tempDC);
    DeleteDC(pokerDC);
    DeleteDC(memDC);
#endif
}

// 父窗口通过设置本窗口的客户矩形中心点的位置来移动本窗口。坐标为父窗口坐标系
BOOL OutCardWnd::SetWindowCenterPos(int xCenter, int yCenter)
{
    int x, y;
    int nMaxWidth, nMaxHeight;

    // 设置窗口矩形为容纳最多扑克牌的矩形
    nMaxHeight = m_CARDHEIGHT;
    nMaxWidth = (MAX_OUT_CARDS_NUM - 1) * m_HSPACE + m_CARDWIDTH;

    x = xCenter - nMaxWidth / 2;
    y = yCenter - nMaxHeight / 2;

    MoveWindow(m_hWnd, x, y, nMaxWidth, nMaxHeight, TRUE);
    return TRUE;
}

// 在出牌窗口显示游戏状态信息：准备，不出，不叫分等
BOOL OutCardWnd::SetGameStatInfo(GAME_STAT_INFO info)
{
    if (m_StatInfo != info) {
        m_StatInfo = info;

        if (m_bShowPoker == TRUE) {
#ifdef PARENT_PAINT_CHILD
            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
        }
    }

    return TRUE;
}

#ifdef PARENT_PAINT_CHILD
void OutCardWnd::ParentPaintChild(HDC parentDC)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int nCardsNum = m_nCurOutCardsNum;
    int* pCardsData = m_nCurOutCards;

    if (m_bShowPrevious == TRUE) {
        nCardsNum = m_nPrevOutCardsNum;
        pCardsData = m_nPrevOutCards;
    }

    int nPokersLen = m_HSPACE * (nCardsNum - 1) + m_CARDWIDTH;

    int nStartX = 0;
    int nStartY = 0;

    nStartY = (cy - m_CARDHEIGHT) / 2;
    if (nStartY < 0) { nStartY = 0; };

    if (m_nAlignment == CARD_ALIGN_CENTER) {
        nStartX = (cx - nPokersLen) / 2;
    } else if (m_nAlignment == CARD_ALIGN_LEFT) {
        nStartX = rect.left;
    } else if (m_nAlignment == CARD_ALIGN_RIGHT) {
        nStartX = rect.right - nPokersLen;
    }

    if (nStartX < 0) { nStartX = 0; }

    POINT ptParentStart = { nStartX, nStartY };
    ClientToScreen(m_hWnd, &ptParentStart);
    ScreenToClient(m_hWndParent, &ptParentStart);

    nStartX = ptParentStart.x;
    nStartY = ptParentStart.y;

    // 画扑克牌
    HDC pokerDC = CreateCompatibleDC(parentDC);
    assert(pokerDC != NULL);

    HBITMAP hOldPokerBmp = (HBITMAP)SelectObject(pokerDC, m_hPokerBMP);

    int nCardBmpIndex;
    for (int i = 0; i < nCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(pCardsData[i]);
        } else {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
        }

        BitBlt(parentDC,
            nStartX + i * m_HSPACE,
            nStartY,
            m_CARDWIDTH,
            m_CARDHEIGHT,
            pokerDC,
            nCardBmpIndex % 13 * m_CARDWIDTH,
            nCardBmpIndex / 13 * m_CARDHEIGHT,
            SRCCOPY);
    }

    if (m_bShowPrevious != TRUE) {
        if (m_StatInfo != GSI_NONE) {
            int nImageIndex = (int)(m_StatInfo - GSI_READY);
            int xImageStart = 0;
            int yImageStart = (cy - CY_GAME_INFO_IMAGE) / 2;

            if (m_nAlignment == CARD_ALIGN_CENTER) {
                xImageStart = (cx - CX_GAME_INFO_IMAGE) / 2;
            } else if (m_nAlignment == CARD_ALIGN_LEFT) {
                xImageStart = 0;
            } else if (m_nAlignment == CARD_ALIGN_RIGHT) {
                xImageStart = cx - CX_GAME_INFO_IMAGE;
            }

            POINT ptParentImageStart = { xImageStart, yImageStart };
            ClientToScreen(m_hWnd, &ptParentImageStart);
            ScreenToClient(m_hWndParent, &ptParentImageStart);

            xImageStart = ptParentImageStart.x;
            yImageStart = ptParentImageStart.y;

            ImageList_Draw(g_himlGameInfo, nImageIndex, parentDC, xImageStart, yImageStart, ILD_NORMAL);
        }
    }

    // 还原及释放资源
    SelectObject(pokerDC, hOldPokerBmp);
    DeleteDC(pokerDC);

    //
    // The following statement will inform system not redraw this window (WM_PAINT)
    // again. For it's already been painted through this function called by it's
    // parent window.
    //
    // System sends WM_PAINT message to parent window, and then sends it to this
    // child window. There's really no need for us to process it any more. Since
    // this macro (PARENT_PAINT_CHILD) is used, and the following statement presented,
    // the window procedure will not receive WM_PAINT unless you call InvalidateRect/Rgn
    // explicitly to do something else with WM_PAINT.
    //
    //ValidateRect(m_hWnd, NULL);
}
#endif


