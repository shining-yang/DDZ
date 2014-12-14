//
// CurPokerWnd.cpp
//
//  使用注意：
//      1. 扑克牌资源位图必须如下排列；单张牌尺寸保存到m_CARDWIDTH和m_CARDHEIGHT
//
//          方片：A  2  3  4  5  6  7  8  9  0  J  Q  K
//          梅花：A  2  3  4  5  6  7  8  9  0  J  Q  K
//          红桃：A  2  3  4  5  6  7  8  9  0  J  Q  K
//          黑桃：A  2  3  4  5  6  7  8  9  0  J  Q  K
//          王牌：小 大 背
//
//      2. 使用本扑克窗口类需要先调用类函数 CurPokerWndRegister() 进行窗口类注册，
//         其次，传递相应参数以调用 Create() 进行创建窗口，完成之后，即可使用其它
//         调用接口，如设置扑克牌数据，是正常显示扑克还是只显示扑克背面（禁止旁观）。
//
//      3. 父窗口需要在其 WM_SIZE 消息处理中，调用扑克牌窗口的函数
//         ParentWndSizeChanged()，以确保父窗口的背景被正确更新。
//
//  特别说明：
//      考虑到斗地主游戏客户端软件绘制背景图时不出现闪烁，很多窗口使用NULL_BRUSH，
//      见其窗口类注册函数，即窗口在重绘时，将不擦除背景图。当玩家出牌后，
//      CurPokerWnd 需重绘，其绘制的步骤是先绘制其窗口所在区域的父窗口背景，再
//      绘制剩余扑克牌位图。绘制父窗口背景时，若直接取父窗口背景，由于父窗口
//      没有擦除其背景，不能得到理想效果。解决方法是：本窗口在第一次绘制时，保存
//      本窗口在父窗口对应区域的背景图，每次本窗口需要绘制时，先绘制该保存的背景
//      图，再绘制本窗口的内容。因此，当父窗口尺寸发生变化时，父窗口需要通知本窗口，
//      而本窗口则需要重新获取并保存其对应父窗口区域的背景图。
//      
#include "stdafx.h"
#include "CurPokerWnd.h"


// 构造函数
CurPokerWnd::CurPokerWnd(void)
{
    for (int i = 0; i < PLAYER_MAX_CARDS_NUM; i++) {
        m_nCurPokerCards[i] = POKER_BACK_INDEX;
        m_bPokerCardSelected[i] = FALSE;
    }

    m_nCurPokerCardsNum = 0;

    m_hInstance = NULL;
    m_hWndParent = NULL;

    m_hWnd = NULL;
    m_hPokerBMP = NULL;
    m_hBkBitmap = NULL;
    m_bReloadBkBitmap = FALSE;

    m_bLButtonDown = FALSE;
    m_ptLButtonDown.x = INVALID_COORD;
    m_ptLButtonDown.y = INVALID_COORD;

    m_bLBtnDownAndMouseMove = FALSE;
    m_ptMouseMove.x = INVALID_COORD;
    m_ptMouseMove.y = INVALID_COORD;

    m_bShowPoker = TRUE;
    m_bHorizontal = TRUE;

    m_bIsLord = FALSE;
    m_bAllowPokerSelect = TRUE;
}

// 析构函数
CurPokerWnd::~CurPokerWnd(void)
{
}

// 注册窗口类
ATOM CurPokerWnd::CurPokerWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = CUR_POKER_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = CurPokerWndProc;
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
CurPokerWnd::CurPokerWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    CurPokerWnd* lpWnd = (CurPokerWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

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

        case WM_LBUTTONDOWN:
            lpWnd->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            lpWnd->OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MOUSEMOVE:
            lpWnd->OnMouseMove((short)(LOWORD(lParam)), (short)(HIWORD(lParam)));
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

// 创建扑克牌窗口
//
// 调用者在创建扑克窗口时，不传递其客户矩形，而是传递其矩形中心点坐标。
// 创建函数根据中心点，创建一个能容纳最多扑克牌的矩形窗口。
//
HWND CurPokerWnd::Create(DWORD dwStyle, int xCenter, int yCenter,
                         HWND hWndParent, UINT nID, HINSTANCE hInstance,
                         HBITMAP hPokerBMP, BOOL bShowPoker, BOOL bHorizontal)
{
    if ((hWndParent == NULL) || (hInstance == NULL) || (hPokerBMP == NULL)) {
        return NULL;
    }

    int x, y;
    int nMaxWidth, nMaxHeight;

    // 设置窗口矩形为容纳最多扑克牌的矩形
    if (bHorizontal == TRUE) {
        nMaxHeight = m_CARDHEIGHT + m_POPSPACE;
        nMaxWidth = (PLAYER_MAX_CARDS_NUM - 1) * m_HSPACE + m_CARDWIDTH;
    } else {
        nMaxHeight = (PLAYER_MAX_CARDS_NUM - 1) * m_VSPACE + m_CARDHEIGHT;
        nMaxWidth = m_CARDWIDTH;
    }

    x = xCenter - nMaxWidth / 2;
    y = yCenter - nMaxHeight / 2;

    m_hWnd = CreateWindowEx(0,
        CUR_POKER_WND_CLASS_NAME,
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

    m_nID = nID;
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_hPokerBMP = hPokerBMP;
    m_bShowPoker = bShowPoker;
    m_bHorizontal = bHorizontal;

    return m_hWnd;
}

//// 调整窗口的位置与尺寸。（创建窗口时指定的矩形不一定是最理想的矩形尺寸）
//void CurPokerWnd::AdjustWindowRect(void)
//{
//    RECT rect;
//    GetClientRect(m_hWnd, &rect);
//
//    int x = rect.left;
//    int y = rect.top;
//    int cx = rect.right - rect.left;
//    int cy = rect.bottom - rect.top;
//
//    int nMinHeight;
//    int nMinWidth;
//
//    if (m_bHorizontal == TRUE) {
//        // 垂直居中将窗口矩形调整至容纳扑克的最小矩形
//        nMinHeight = m_CARDHEIGHT + m_POPSPACE;
//        nMinWidth = (m_nCurPokerCardsNum - 1) * m_HSPACE + m_CARDWIDTH;
//    } else {
//        // 水平居中将窗口矩形调整至容纳扑克的最小矩形
//        nMinWidth = m_CARDWIDTH;
//        nMinHeight = (m_nCurPokerCardsNum - 1) * m_VSPACE + m_CARDHEIGHT;
//    }
//
//    x += (cx - nMinWidth) / 2;
//    y += (cy - nMinHeight) / 2;
//    cx = nMinWidth;
//    cy = nMinHeight;
//
//    POINT pt = { x, y };
//    ClientToScreen(m_hWnd, &pt);
//    ScreenToClient(m_hWndParent, &pt);
//    SetWindowPos(m_hWnd, NULL, pt.x, pt.y, cx, cy, SWP_SHOWWINDOW);
//}

// 传递扑克数据给当前扑克牌窗口
BOOL CurPokerWnd::SetCards(int poker[], int num)
{
    // 传递NUM参数为0用于清空本窗口的内容
    if ((num == 0) && (m_nCurPokerCardsNum == 0)) {
        return TRUE;
    }

    m_nCurPokerCardsNum = num;
    for (int i = 0; i < num; i++) {
        m_nCurPokerCards[i] = poker[i];
    }

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        m_bPokerCardSelected[i] = FALSE;
    }

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 获取当前扑克牌。参数 poker[] 必须足够大，推荐使用值 PLAYER_MAX_CARDS_NUM
BOOL CurPokerWnd::GetCards(int poker[], int* num)
{
    assert((poker != NULL) && (num != NULL));

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        poker[i] = m_nCurPokerCards[i];
    }

    *num = m_nCurPokerCardsNum;
    return TRUE;
}

// 获取选择的扑克牌索引序列
BOOL CurPokerWnd::GetSelectedCards(int poker[], int num, int* selnum)
{
    UNREFERENCED_PARAMETER(num);
    assert(num >= m_nCurPokerCardsNum);

    int count = 0;

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_bPokerCardSelected[i] == TRUE) {
            poker[count++] = m_nCurPokerCards[i];
        }
    }

    *selnum = count;
    return TRUE;
}

// 删除选择的扑克牌（出牌）
BOOL CurPokerWnd::RemoveSelectedCards(void)
{
    int nRemain = 0;
    int pokers[PLAYER_MAX_CARDS_NUM] = { 0 };

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_bPokerCardSelected[i] == FALSE) {
            pokers[nRemain++] = m_nCurPokerCards[i];
        }
    }

    if (nRemain == m_nCurPokerCardsNum) { // no selected pokers
        return TRUE;
    }

    for (int i = 0; i < nRemain; i++) {
        m_nCurPokerCards[i] = pokers[i];
    }

    m_nCurPokerCardsNum = nRemain;

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        m_bPokerCardSelected[i] = FALSE;
    }

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 由索引指示选择哪些牌，该接口主要用于提示功能返回的索引。注意该函数的索引参数。
BOOL CurPokerWnd::SelectCardsFromIndex(int index[], int num)
{
    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        m_bPokerCardSelected[i] = FALSE;
    }

    for (int i = 0; i < num; i++) {
        assert((index[i] >= 0) && (index[i] < m_nCurPokerCardsNum));

        m_bPokerCardSelected[index[i]] = TRUE;
    }

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 删除指定的扑克牌序列（扑克牌为升序的）
BOOL CurPokerWnd::RemoveCards(int poker[], int num)
{
    int nRemCount = 0;
    int nRemCards[PLAYER_MAX_CARDS_NUM] = { 0 };
    BOOL bRemoved[PLAYER_MAX_CARDS_NUM] = { 0 };

    if (num <= 0) {
        return TRUE;
    }

    // 先将要删除的牌置标志
    int j = 0;
    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_nCurPokerCards[i] == poker[j]) {
            bRemoved[i] = TRUE;
            if (++j >= num) {
                break;
            }
        }
    }

    // 将没删除的牌拷贝出去
    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (bRemoved[i] != TRUE) {
            nRemCards[nRemCount++] = m_nCurPokerCards[i];
        }
    }

    // 将拷贝出去的（没有删除的）牌再拷贝回来
    for (int i = 0; i < nRemCount; i++) {
        m_nCurPokerCards[i] = nRemCards[i];
    }

    m_nCurPokerCardsNum = nRemCount;

    // 出牌后，复位所有选择的牌索引
    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        m_bPokerCardSelected[i] = FALSE;
    }

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 增加扑克牌到当前扑克窗口（叫地主之后，将底牌插入地主当前牌中）
BOOL CurPokerWnd::AddCards(int poker[], int num)
{
    assert(m_nCurPokerCardsNum + num <= PLAYER_MAX_CARDS_NUM);

    for (int i = 0; i < num; i++) {
        m_nCurPokerCards[m_nCurPokerCardsNum + i] = poker[i];
    }

    m_nCurPokerCardsNum += num;

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        m_bPokerCardSelected[i] = FALSE;
    }

    CommonUtil::QuickSort(m_nCurPokerCards, 0, m_nCurPokerCardsNum - 1);

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif

    return TRUE;
}

// 保存父窗口背景。父窗口尺寸变化时需要重新保存。
void CurPokerWnd::SaveParentBackground(void)
{
    // 取父窗口DC
    HDC parentDC = GetDC(m_hWndParent);
    assert(parentDC != NULL);

    SaveParentBackground(parentDC);

    // 释放父窗口DC
    ReleaseDC(m_hWndParent, parentDC);
}

// 保存父窗口背景
void CurPokerWnd::SaveParentBackground(HDC parentDC)
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

// 绘图
void CurPokerWnd::OnPaint(HDC hdc)
{
#ifdef PARENT_PAINT_CHILD
    UNREFERENCED_PARAMETER(hdc);
    // Parent window will paint me. Do nothing...
#else
    if (m_bReloadBkBitmap == TRUE) {
        SaveParentBackground();
        m_bReloadBkBitmap = FALSE;
    }

    if (m_bHorizontal == TRUE) {
        PaintHorizontally(hdc);
    } else {
        PaintVertically(hdc);
    }
#endif
}

// 绘制水平排列的扑克序列
void CurPokerWnd::PaintHorizontally(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int nPokersLen = m_HSPACE * (m_nCurPokerCardsNum - 1) + m_CARDWIDTH;

    // 计算客户区内的坐标偏移，确保在客户区中间显示
    int nStartY = m_POPSPACE;
    int nStartX = (cx - nPokersLen) / 2;
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

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nStartY = m_bPokerCardSelected[i] ? 0 : m_POPSPACE;
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nCurPokerCards[i]);
        } else {
            nStartY = m_POPSPACE;
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

    BitBlt(hdc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);

    // 还原及释放资源
    SelectObject(tempDC, hOldTempBmp);
    SelectObject(pokerDC, hOldPokerBmp);
    SelectObject(memDC, hOldMemBmp);

    DeleteObject(hMemBmp);

    DeleteDC(tempDC);
    DeleteDC(pokerDC);
    DeleteDC(memDC);
}

// 绘制竖直排列的扑克序列
void CurPokerWnd::PaintVertically(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    // 计算客户区内的坐标偏移，确保在客户区中间显示
    int nStartX = (cx - m_CARDWIDTH) / 2;
    if (nStartX < 0) { nStartX = 0; }

    int nPokersHt = m_VSPACE * (m_nCurPokerCardsNum - 1) + m_CARDHEIGHT;
    int nStartY = (cy - nPokersHt) / 2;
    if (nStartY < 0) { nStartY = 0; }

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

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_bShowPoker == TRUE) {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nCurPokerCards[i]);
        } else {
            nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
        }

        BitBlt(memDC,
            nStartX,
            nStartY + i * m_VSPACE,
            m_CARDWIDTH,
            m_CARDHEIGHT,
            pokerDC,
            nCardBmpIndex % 13 * m_CARDWIDTH,
            nCardBmpIndex / 13 * m_CARDHEIGHT,
            SRCCOPY);
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
}


// 点击
void CurPokerWnd::ClickCard(int x, int y)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    // 将矩形调整到当前绘制扑克牌所在的矩形
    int nClientWidth = rect.right - rect.left;
    int nPokerCardsLen = (m_nCurPokerCardsNum - 1) * m_HSPACE + m_CARDWIDTH;

    if (nClientWidth > nPokerCardsLen) {
        rect.left = (nClientWidth - nPokerCardsLen) / 2;
        rect.right = (nClientWidth + nPokerCardsLen - 1) / 2;
    }

    rect.top = 0;

    if (rect.bottom > m_POPSPACE + m_CARDHEIGHT) {
        rect.bottom = m_POPSPACE + m_CARDHEIGHT;
    }

    // 点击点在扑克牌矩形上。判断点击了哪张牌
    POINT pt = {x, y};
    if (PtInRect(&rect, pt)) {
        int xCoord = pt.x - rect.left;
        int yCoord = pt.y;
        int index;

        if (xCoord > (m_nCurPokerCardsNum - 1) * m_HSPACE) {
            // 这是最右边，或者说是 Z-TopMost 的那张牌。可看到其全部内容的那张牌
            index = m_nCurPokerCardsNum - 1;
        } else {
            index = xCoord / m_HSPACE;
        }

        if (m_bPokerCardSelected[index] == TRUE) { // 已经是弹起状态
            if ((yCoord >= 0) && (yCoord <= m_CARDHEIGHT)) {
                m_bPokerCardSelected[index] = FALSE;

#ifdef PARENT_PAINT_CHILD
                CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
                InvalidateRect(m_hWnd, NULL, FALSE);
#endif
            } else {
                //
                // 进入这个分支，说明玩家点击了弹起扑克牌后的留下的小区域。
                // 因为扑克牌是层叠放置的，所以，当前鼠标点击的点可能是位
                // 于下面未弹起的牌中，应该再向前搜索几张进行判断。
                //
                int num;
                int remaider = xCoord % m_HSPACE;
                int multiple = m_CARDWIDTH / m_HSPACE;

                // 最多搜索 num 张
                if (remaider + multiple * m_HSPACE > m_CARDWIDTH) {
                    num = multiple;
                } else {
                    num = multiple + 1;
                }

                // 向前搜索num张牌，找到第一张即可。注意数组下标别underflow
                for (int i = 1; i <= num; i++) {
                    int idx = index - i;

                    if (idx < 0) { return; }

                    if (m_bPokerCardSelected[idx] == FALSE) {
                        //
                        // 这是找到的第一张，进一步确认点击是否在扑克牌里面，
                        // 如果不是，不必再往下找；如果是，设置选中项，更新界面。
                        //
                        if (idx * m_HSPACE + m_CARDWIDTH > xCoord) {
                            m_bPokerCardSelected[idx] = TRUE;

#ifdef PARENT_PAINT_CHILD
                            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
                            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
                        }

                        return;
                    }
                }
            }
        } else { // 扑克牌不是弹起状态
            if ((yCoord >= m_POPSPACE) && (yCoord <= m_POPSPACE + m_CARDHEIGHT)) {
                m_bPokerCardSelected[index] = TRUE;

#ifdef PARENT_PAINT_CHILD
                CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
                InvalidateRect(m_hWnd, NULL, FALSE);
#endif
            } else {
                int num;
                int remaider = xCoord % m_HSPACE;
                int multiple = m_CARDWIDTH / m_HSPACE;

                // 最多搜索 num 张
                if (remaider + multiple * m_HSPACE > m_CARDWIDTH) {
                    num = multiple;
                } else {
                    num = multiple + 1;
                }

                // 向前搜索num张牌，找到第一张即可。注意数组下标别underflow
                for (int i = 1; i <= num; i++) {
                    int idx = index - i;

                    if (idx < 0) { return; }

                    if (m_bPokerCardSelected[idx] == TRUE) {
                        if (idx * m_HSPACE + m_CARDWIDTH > xCoord) {
                            m_bPokerCardSelected[idx] = FALSE;

#ifdef PARENT_PAINT_CHILD
                            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
                            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
                        }

                        return;
                    }
                }
            }
        }
    }
}

// 鼠标左键单击
void CurPokerWnd::OnLButtonDown(int x, int y)
{
    if (m_bHorizontal == FALSE) { // 只有水平型的（当前玩家的牌）可以被选择
        return;
    }

    if (m_nCurPokerCardsNum <= 0) {
        return;
    }

    if (m_bAllowPokerSelect == FALSE) { // 是否允许选择扑克牌，主要用于旁观者视角
        return;
    }

    POINT pt;
    pt.x = x;
    pt.y = y;

    RECT rect;
    GetCardRangeSelectRect(&rect);

    m_ptLButtonDown.x = x;
    m_ptLButtonDown.y = y;

    m_bLButtonDown = TRUE;
    SetCapture(m_hWnd);
}

// 鼠标左键弹起
void CurPokerWnd::OnLButtonUp(int x, int y)
{
    RECT rect;
    RECT rectRangeSel;

    if (m_bLButtonDown == TRUE) {
        m_bLButtonDown = FALSE;
        ReleaseCapture();

        if (m_bLBtnDownAndMouseMove == TRUE) {
            m_bLBtnDownAndMouseMove = FALSE;

            HDC hdc = GetDC(m_hWnd);
            HPEN hpen = CreatePen(PS_SOLID|PS_INSIDEFRAME, SEL_CARDS_FRAME_WIDTH, SEL_CARDS_FRAME_CLR);
            HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, hbrush);
            int mode = SetROP2(hdc, R2_XORPEN);

            GetCardRangeSelectRect(&rectRangeSel);

            rect.left = max(0, min(m_ptMouseMove.x, m_ptLButtonDown.x));
            rect.right = max(m_ptMouseMove.x, m_ptLButtonDown.x);
            rect.top = max(0, min(m_ptMouseMove.y, m_ptLButtonDown.y));
            rect.bottom = max(m_ptMouseMove.y, m_ptLButtonDown.y);
            IntersectRect(&rect, &rect, &rectRangeSel);
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            SetROP2(hdc, mode);
            SelectObject(hdc, oldbrush);
            SelectObject(hdc, oldpen);

            DeleteObject(hpen);
            ReleaseDC(m_hWnd, hdc);

            SelectCardRange(min(m_ptMouseMove.x, m_ptLButtonDown.x),
                max(m_ptMouseMove.x, m_ptLButtonDown.x));

            m_ptMouseMove.x = INVALID_COORD;
            m_ptMouseMove.y = INVALID_COORD;
        } else {
            if (m_ptLButtonDown.x == x) {
                if (m_ptLButtonDown.y == y) {
                    ClickCard(x, y); // 单击鼠标选择一张牌
                }
            }
        }
    }
}

// 选择多张扑克牌
void CurPokerWnd::SelectCardRange(int xStart, int xEnd)
{
    RECT rectRangeSel;
    GetCardRangeSelectRect(&rectRangeSel);

    // xStart和 xEnd 必须位于 rectRangeSel 矩形内
    int xCoordStart = xStart - rectRangeSel.left;
    int xCoordEnd = xEnd - rectRangeSel.left;

    int iStart = xCoordStart / m_HSPACE;
    int iEnd = xCoordEnd / m_HSPACE;

    if (iStart < 0) { iStart = 0; }
    if (iStart > m_nCurPokerCardsNum - 1) { iStart = m_nCurPokerCardsNum - 1; }
    if (iEnd > m_nCurPokerCardsNum - 1) { iEnd = m_nCurPokerCardsNum - 1; }

    for (int i = iStart; i <= iEnd; i++) {
        m_bPokerCardSelected[i] = !m_bPokerCardSelected[i];
    }

#ifdef PARENT_PAINT_CHILD
    CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
    InvalidateRect(m_hWnd, NULL, FALSE);
#endif
}

// 获取允许多张扑克牌选择的矩形（仅可用于水平型的扑克牌窗口）
void CurPokerWnd::GetCardRangeSelectRect(LPRECT lpRect)
{
    GetClientRect(m_hWnd, lpRect);

    int nClientWidth = lpRect->right - lpRect->left;
    int nPokerCardsLen = (m_nCurPokerCardsNum - 1) * m_HSPACE + m_CARDWIDTH;

    if (nClientWidth > nPokerCardsLen) {
        lpRect->left = (nClientWidth - nPokerCardsLen) / 2;
        lpRect->right = (nClientWidth + nPokerCardsLen - 1) / 2;
    }

    lpRect->top += m_POPSPACE;
    lpRect->bottom -= m_POPSPACE;
}

// 移动鼠标（当鼠标在扑克牌上按下并移动时，XOR方式画一矩形，以示玩家选择扑克区域）
void CurPokerWnd::OnMouseMove(int x, int y)
{
    RECT rect;
    RECT rectRangeSel;

    if (m_bLButtonDown != TRUE) {
        return;
    }

    GetCardRangeSelectRect(&rectRangeSel);

    if (PtInRect(&rectRangeSel, m_ptLButtonDown)) {
        m_bLBtnDownAndMouseMove = TRUE;

        HDC hdc = GetDC(m_hWnd);
        HPEN hpen = CreatePen(PS_SOLID|PS_INSIDEFRAME, SEL_CARDS_FRAME_WIDTH, SEL_CARDS_FRAME_CLR);
        HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, hbrush);
        int mode = SetROP2(hdc, R2_XORPEN);

        if (m_ptMouseMove.x != INVALID_COORD) {
            rect.left = max(0, min(m_ptMouseMove.x, m_ptLButtonDown.x));
            rect.right = max(m_ptMouseMove.x, m_ptLButtonDown.x);
            rect.top = max(0, min(m_ptMouseMove.y, m_ptLButtonDown.y));
            rect.bottom = max(m_ptMouseMove.y, m_ptLButtonDown.y);
            IntersectRect(&rect, &rect, &rectRangeSel);
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        }

        m_ptMouseMove.x = max(0, x);
        m_ptMouseMove.y = max(0, y);

        rect.left = max(0, min(m_ptMouseMove.x, m_ptLButtonDown.x));
        rect.right = max(m_ptMouseMove.x, m_ptLButtonDown.x);
        rect.top = max(0, min(m_ptMouseMove.y, m_ptLButtonDown.y));
        rect.bottom = max(m_ptMouseMove.y, m_ptLButtonDown.y);
        IntersectRect(&rect, &rect, &rectRangeSel);
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SetROP2(hdc, mode);
        SelectObject(hdc, oldbrush);
        SelectObject(hdc, oldpen);

        DeleteObject(hpen);

        ReleaseDC(m_hWnd, hdc);
    }
}

// 父窗口的尺寸已经改变，设置标志以指示绘制窗口时，需要重新获取父窗口背景
void CurPokerWnd::ParentWndSizeChanged(void)
{
    m_bReloadBkBitmap = TRUE;
}

// 复位所有选中的扑克牌
BOOL CurPokerWnd::UnSelectAllCards(void)
{
    BOOL bNeedPaint = FALSE;

    for (int i = 0; i < m_nCurPokerCardsNum; i++) {
        if (m_bPokerCardSelected[i] == TRUE) {
            m_bPokerCardSelected[i] = FALSE;
            bNeedPaint = TRUE;
        }
    }

    if (bNeedPaint == TRUE) {
#ifdef PARENT_PAINT_CHILD
        CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
        InvalidateRect(m_hWnd, NULL, FALSE);
#endif
    }

    return TRUE;
}

// 显示扑克牌
BOOL CurPokerWnd::ShowPoker(BOOL bShow /*= TRUE*/)
{
    if (m_bShowPoker != bShow) {
        m_bShowPoker = bShow;

        if (m_nCurPokerCardsNum > 0) {
#ifdef PARENT_PAINT_CHILD
            CommonUtil::ParentPaintChild(m_hWndParent, m_hWnd, NULL, FALSE);
#else
            InvalidateRect(m_hWnd, NULL, FALSE);
#endif
        }
    }

    return TRUE;
}

// 父窗口通过设置本窗口的客户矩形中心点的位置来移动本窗口。坐标为父窗口坐标系
BOOL CurPokerWnd::SetWindowCenterPos(int xCenter, int yCenter)
{
    int x, y;
    int nMaxWidth, nMaxHeight;

    // 设置窗口矩形为容纳最多扑克牌的矩形
    if (m_bHorizontal == TRUE) {
        nMaxHeight = m_CARDHEIGHT + m_POPSPACE;
        nMaxWidth = (PLAYER_MAX_CARDS_NUM - 1) * m_HSPACE + m_CARDWIDTH;
    } else {
        nMaxHeight = (PLAYER_MAX_CARDS_NUM - 1) * m_VSPACE + m_CARDHEIGHT;
        nMaxWidth = m_CARDWIDTH;
    }

    x = xCenter - nMaxWidth / 2;
    y = yCenter - nMaxHeight / 2;

    MoveWindow(m_hWnd, x, y, nMaxWidth, nMaxHeight, TRUE);
    return TRUE;
}

#ifdef PARENT_PAINT_CHILD
void CurPokerWnd::ParentPaintChild(HDC parentDC)
{
    if (m_nCurPokerCardsNum <= 0) { return; }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    POINT ptParentStart; // 在父窗口坐标系中的画扑克牌的起始点

    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    // 将扑克牌位图选入内存DC
    HDC pokerDC = CreateCompatibleDC(parentDC);
    assert(pokerDC != NULL);
    HBITMAP hOldPokerBmp = (HBITMAP)SelectObject(pokerDC, m_hPokerBMP);

    if (m_bHorizontal == TRUE) { // 水平型的当前扑克牌窗口
        int nPokersLen = m_HSPACE * (m_nCurPokerCardsNum - 1) + m_CARDWIDTH;

        // 计算客户区内的坐标偏移，确保在客户区中间显示
        int nStartY = m_POPSPACE;
        int nStartX = (cx - nPokersLen) / 2;
        if (nStartX < 0) { nStartX = 0; }

        // 将坐标转换为父窗口坐标系
        ptParentStart.x = nStartX;
        ptParentStart.y = nStartY;
        ClientToScreen(m_hWnd, &ptParentStart);
        ScreenToClient(m_hWndParent, &ptParentStart);
        nStartX = ptParentStart.x;
        nStartY = ptParentStart.y;

        // 画扑克牌
        int nCardBmpIndex;

        for (int i = 0; i < m_nCurPokerCardsNum; i++) {
            if (m_bShowPoker == TRUE) {
                nStartY = m_bPokerCardSelected[i] ? (ptParentStart.y - m_POPSPACE) : ptParentStart.y;

                nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nCurPokerCards[i]);
            } else {
                nStartY = ptParentStart.y;

                if (m_bIsLord == TRUE) {
                    nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(LORD_POKER_BACK_INDEX);
                } else {
                    nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
                }
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
    } else { // 垂直型的当前扑克牌窗口
        // 计算客户区内的坐标偏移，确保在客户区中间显示
        int nStartX = (cx - m_CARDWIDTH) / 2;
        if (nStartX < 0) { nStartX = 0; }

        int nPokersHt = m_VSPACE * (m_nCurPokerCardsNum - 1) + m_CARDHEIGHT;
        int nStartY = (cy - nPokersHt) / 2;
        if (nStartY < 0) { nStartY = 0; }

        // 将坐标转换为父窗口坐标系
        ptParentStart.x = nStartX;
        ptParentStart.y = nStartY;
        ClientToScreen(m_hWnd, &ptParentStart);
        ScreenToClient(m_hWndParent, &ptParentStart);
        nStartX = ptParentStart.x;
        nStartY = ptParentStart.y;

        int nCardBmpIndex;

        for (int i = 0; i < m_nCurPokerCardsNum; i++) {
            if (m_bShowPoker == TRUE) {
                nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(m_nCurPokerCards[i]);
            } else {
                if (m_bIsLord == TRUE) {
                    nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(LORD_POKER_BACK_INDEX);
                } else {
                    nCardBmpIndex = CommonUtil::PokerIndexToBmpIndex(POKER_BACK_INDEX);
                }
            }

            BitBlt(parentDC,
                nStartX,
                nStartY + i * m_VSPACE,
                m_CARDWIDTH,
                m_CARDHEIGHT,
                pokerDC,
                nCardBmpIndex % 13 * m_CARDWIDTH,
                nCardBmpIndex / 13 * m_CARDHEIGHT,
                SRCCOPY);
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

// 设置是否允许当前扑克牌窗口的牌可以被选择，默认为可以选择。用于设置旁观者不可选择牌。
BOOL CurPokerWnd::AllowPokerSelection(BOOL bAllow /*= TRUE*/)
{
    if (bAllow == FALSE) {
        UnSelectAllCards();
    }

    m_bAllowPokerSelect = bAllow;
    return TRUE;
}

// 设置该玩家为地主，调用本函数不会自动刷新界面。
BOOL CurPokerWnd::SetLord(BOOL bLord /*= TRUE*/)
{
    m_bIsLord = bLord;
    return TRUE;
}
