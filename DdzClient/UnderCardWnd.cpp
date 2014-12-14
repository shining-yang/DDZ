//
// UnderCardWnd.cpp
//
//  用于显示斗地主的三张底牌
//
#include "stdafx.h"
#include "UnderCardWnd.h"


UnderCardWnd::UnderCardWnd(void)
{
    for (int i = 0; i < UNDER_CARDS_NUM; i++) {
        m_nUnderCards[i] = POKER_BACK_INDEX;
    }

    m_nUnderCardsNum = 0;

    m_hWndParent = NULL;
    m_hInstance = NULL;

    m_hWnd = NULL;
    m_hPokerBMP = NULL;
    m_bShowPoker = FALSE;

    m_hBkBitmap = NULL;
    m_bReloadBkBitmap = FALSE;
}

UnderCardWnd::~UnderCardWnd(void)
{

}

ATOM UnderCardWnd::UnderCardWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = UNDER_CARD_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = UnderCardWndProc;
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

LRESULT
CALLBACK
UnderCardWnd::UnderCardWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    UnderCardWnd* lpWnd = (UnderCardWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
            break;

        case WM_DESTROY:
            if (lpWnd->m_hBkBitmap != NULL) {
                DeleteObject(lpWnd->m_hBkBitmap);
                lpWnd->m_hBkBitmap = NULL;
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_SIZE:
            // 本窗口尺寸或位置发生变化，需要重新获取本窗口所在区域的父窗口背景
            lpWnd->m_bReloadBkBitmap = TRUE;
            break;

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SendMessage(lpWnd->m_hWndParent, uMsg, wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

HWND UnderCardWnd::Create(DWORD dwStyle, int xCenter, int yCenter,
                          HWND hWndParent, HINSTANCE hInstance, HBITMAP hPokerBMP)
{
    int x, y;
    int width, height;

    width = m_CARDWIDTH * UNDER_CARDS_NUM + m_HSPACE * (UNDER_CARDS_NUM - 1);
    height = m_CARDHEIGHT;

    x = xCenter - width / 2;
    y = yCenter - height / 2;

    m_hWnd = CreateWindowEx(0,
        UNDER_CARD_WND_CLASS_NAME,
        _T(""),
        dwStyle,
        x,
        y,
        width,
        height,
        hWndParent,
        NULL,
        hInstance,
        this);

    m_hPokerBMP = hPokerBMP;
    m_hInstance = hInstance;
    m_hWndParent = hWndParent;

    return m_hWnd;
}

BOOL UnderCardWnd::ShowPoker(BOOL bShow /*= TRUE*/)
{
    if (m_bShowPoker != bShow) {
        m_bShowPoker = bShow;

        if (m_nUnderCardsNum > 0) {
#ifdef PARENT_PAINT_CHILD
            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
        }
    }

    return TRUE;
}

BOOL UnderCardWnd::SetCards(int poker[], int num)
{
    assert((num == 0) || (num == UNDER_CARDS_NUM));

    // 传递NUM参数为0用于清空本窗口的内容
    if ((num == 0) && (m_nUnderCardsNum == 0)) {
        return TRUE;
    }

    for (int i = 0; i < min(UNDER_CARDS_NUM, num); i++) {
        m_nUnderCards[i] = poker[i];
    }

    m_nUnderCardsNum = min(UNDER_CARDS_NUM, num);

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

BOOL UnderCardWnd::GetCards(int poker[], int* num)
{
    assert((poker != NULL) && (num != NULL));
    for (int i = 0; i < m_nUnderCardsNum; i++) {
        poker[i] = m_nUnderCards[i];
    }

    *num = m_nUnderCardsNum;
    return TRUE;
}

// 父窗口的尺寸已经改变，设置标志以指示绘制窗口时，需要重新获取父窗口背景
void UnderCardWnd::ParentWndSizeChanged(void)
{
    m_bReloadBkBitmap = TRUE;
}

// 保存父窗口背景。父窗口尺寸变化时需要重新保存。
void UnderCardWnd::SaveParentBackground(void)
{
    // 取父窗口DC
    HDC parentDC = GetDC(m_hWndParent);
    assert(parentDC != NULL);

    SaveParentBackground(parentDC);

    // 释放父窗口DC
    ReleaseDC(m_hWndParent, parentDC);
}

// 保存父窗口背景
void UnderCardWnd::SaveParentBackground(HDC parentDC)
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

void UnderCardWnd::OnPaint(HDC hdc)
{
#ifdef PARENT_PAINT_CHILD
    UNREFERENCED_PARAMETER(hdc);
    // nothing to do ...
#else
    if (m_bReloadBkBitmap == TRUE) {
        m_bReloadBkBitmap = FALSE;
        SaveParentBackground();
    }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int nPokersLen = m_CARDWIDTH * UNDER_CARDS_NUM + m_HSPACE * (UNDER_CARDS_NUM - 1);

    int nStartX = (cx - nPokersLen) / 2;
    int nStartY = (cy - m_CARDHEIGHT) / 2;

    if (nStartX < 0) { nStartX = 0; }
    if (nStartY < 0) { nStartY = 0; };

    // 创建内存缓冲DC及其位图
    HDC memDC = CreateCompatibleDC(hdc);
    assert(memDC != NULL);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, cx, cy);
    assert(hMemBmp != NULL);
    HBITMAP hOldMemBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

    // 将父窗口背景位图拷贝到内存DC
    {
        HDC hdcTemp = CreateCompatibleDC(hdc);
        HBITMAP hOldTempBmp = (HBITMAP)SelectObject(hdcTemp, m_hBkBitmap);
        BitBlt(memDC, 0, 0, cx, cy, hdcTemp, 0, 0, SRCCOPY);
        SelectObject(hdcTemp, hOldTempBmp);
        DeleteDC(hdcTemp);
    }

    // 再画扑克牌
    HDC pokerDC = CreateCompatibleDC(hdc);
    assert(pokerDC != NULL);
    HBITMAP hOldPokerBmp = (HBITMAP)SelectObject(pokerDC, m_hPokerBMP);

    int nCardBmpIndex;
    for (int i = 0; i < m_nUnderCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nUnderCards[i]);
        } else {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
        }

        BitBlt(memDC,
            nStartX + i * (m_HSPACE + m_CARDWIDTH),
            nStartY,
            m_CARDWIDTH,
            m_CARDHEIGHT,
            pokerDC,
            nCardBmpIndex % 13 * m_CARDWIDTH,
            nCardBmpIndex / 13 * m_CARDHEIGHT,
            SRCCOPY);
    }

    BitBlt(hdc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);

    // 还原及释放资源
    SelectObject(pokerDC, hOldPokerBmp);
    SelectObject(memDC, hOldMemBmp);

    DeleteObject(hMemBmp);

    DeleteDC(pokerDC);
    DeleteDC(memDC);
#endif
}

// 父窗口通过设置本窗口的客户矩形中心点的位置来移动本窗口。坐标为父窗口坐标系
BOOL UnderCardWnd::SetWindowCenterPos(int xCenter, int yCenter)
{
    int x, y;
    int width, height;

    width = m_CARDWIDTH * UNDER_CARDS_NUM + m_HSPACE * (UNDER_CARDS_NUM - 1);
    height = m_CARDHEIGHT;

    x = xCenter - width / 2;
    y = yCenter - height / 2;

    MoveWindow(m_hWnd, x, y, width, height, TRUE);
    return TRUE;
}

#ifdef PARENT_PAINT_CHILD
void UnderCardWnd::ParentPaintChild(HDC parentDC)
{
    if (m_nUnderCardsNum <= 0) { return; }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int nPokersLen = m_CARDWIDTH * UNDER_CARDS_NUM + m_HSPACE * (UNDER_CARDS_NUM - 1);

    int nStartX = (cx - nPokersLen) / 2;
    int nStartY = (cy - m_CARDHEIGHT) / 2;

    if (nStartX < 0) { nStartX = 0; }
    if (nStartY < 0) { nStartY = 0; };

    // 将坐标转换为父窗口坐标系
    POINT ptParentStart;
    ptParentStart.x = nStartX;
    ptParentStart.y = nStartY;
    ClientToScreen(m_hWnd, &ptParentStart);
    ScreenToClient(m_hWndParent, &ptParentStart);
    nStartX = ptParentStart.x;
    nStartY = ptParentStart.y;

    // 再画扑克牌
    HDC pokerDC = CreateCompatibleDC(parentDC);
    assert(pokerDC != NULL);
    HBITMAP hOldPokerBmp = (HBITMAP)SelectObject(pokerDC, m_hPokerBMP);

    int nCardBmpIndex;
    for (int i = 0; i < m_nUnderCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nUnderCards[i]);
        } else {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
        }

        BitBlt(parentDC,
            nStartX + i * (m_HSPACE + m_CARDWIDTH),
            nStartY,
            m_CARDWIDTH,
            m_CARDHEIGHT,
            pokerDC,
            nCardBmpIndex % 13 * m_CARDWIDTH,
            nCardBmpIndex / 13 * m_CARDHEIGHT,
            SRCCOPY);
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
