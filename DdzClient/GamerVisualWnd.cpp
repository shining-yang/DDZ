//
// File: GamerVisualWnd.cpp
//
//  显示游戏玩家形象图
//
#include "stdafx.h"
#include "GamerVisualWnd.h"

GamerVisualWnd::GamerVisualWnd(void)
{
    m_PicVisual = NULL;
    m_nUserId = INVALID_USER_ID;

    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;

    m_hBkBitmap = NULL;
    m_bReloadBkBitmap = FALSE;
}

GamerVisualWnd::~GamerVisualWnd(void)
{
}

ATOM GamerVisualWnd::GamerVisualWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpszClassName  = GAMER_VISUAL_WND_CLASS_NAME;
    wcex.lpfnWndProc    = GamerVisualWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;//(HBRUSH)(COLOR_WINDOW + 1);

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GamerVisualWnd::GamerVisualWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    GamerVisualWnd* lpWnd = (GamerVisualWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            //lpWnd->m_PicVisual->OnDestroy();
            delete lpWnd->m_PicVisual;

            if (lpWnd->m_hBkBitmap != NULL) {
                DeleteObject(lpWnd->m_hBkBitmap);
                lpWnd->m_hBkBitmap = NULL;
            }

            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc =BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_SIZE:
            // 本窗口尺寸或位置发生变化，需要重新获取本窗口所在区域的父窗口背景
            lpWnd->m_bReloadBkBitmap = TRUE;
            break;

        case WM_LBUTTONDBLCLK:
            SendMessage(lpWnd->m_hWndParent, WM_LBUTTONDBLCLK, 0, 0);
            break;

        case WM_LBUTTONDOWN:
            break;

        case WM_RBUTTONDOWN:
            lpWnd->OnRButtonDown(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

void GamerVisualWnd::OnPaint(HDC hdc)
{
#ifdef PARENT_PAINT_CHILD
    //
    // 父窗口已经绘制完本窗口其它内容，剩下的GIF图像，由自己调用PictureEx完成绘制
    //
    if (m_nUserId != INVALID_USER_ID) {
        m_PicVisual->OnPaint();
    } else { // 该椅子没有玩家，或玩家离开，则绘制保存的父窗口背景图
        RECT rect;
        GetClientRect(m_hWnd, &rect);
        HDC memdc = CreateCompatibleDC(hdc);
        HBITMAP hbmpOld = (HBITMAP)SelectObject(memdc, m_hBkBitmap);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, hbmpOld);
        DeleteDC(memdc);
    }
#else

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    if (m_bReloadBkBitmap == TRUE) {
        m_bReloadBkBitmap = FALSE;

        SaveParentBackground();
        m_PicVisual->SetBkBitmap(m_hBkBitmap);
    }

    // 该椅子没有玩家，或玩家离开，则画保存的父窗口背景图
    if (m_nUserId == INVALID_USER_ID) {
        HDC memdc = CreateCompatibleDC(hdc);
        HBITMAP hbmpOld = (HBITMAP)SelectObject(memdc, m_hBkBitmap);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, hbmpOld);
        DeleteDC(memdc);
        return;
    }

    // PictureEx 绘制GIF图片
    m_PicVisual->OnPaint();

    // 绘制玩家昵称
    LPCTSTR lpszName = GetUserNameStr(m_nUserId);

    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldfont = (HFONT)SelectObject(hdc, hfont);
    int mode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldtextclr = SetTextColor(hdc, GVW_TEXT_CLR);

    DrawText(hdc, lpszName, _tcslen(lpszName), &rect, DT_BOTTOM | DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(hdc, oldtextclr);
    SetBkMode(hdc, mode);
    SelectObject(hdc, oldfont);
#endif
}

#ifdef PARENT_PAINT_CHILD
void GamerVisualWnd::ParentPaintChild(HDC parentDC)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    if (m_bReloadBkBitmap == TRUE) {
        m_bReloadBkBitmap = FALSE;

        SaveParentBackground(parentDC);
        m_PicVisual->SetBkBitmap(m_hBkBitmap);
    }

    // 该椅子没有玩家
    if (m_nUserId == INVALID_USER_ID) {
        return;
    }

    // 转换为父窗口坐标系
    CommonUtil::ClientRectToScreen(m_hWnd, &rect);
    CommonUtil::ScreenRectToClient(m_hWndParent, &rect);

    // CPictureEx 绘制GIF图片
    m_PicVisual->OnPaint(parentDC, &rect);

    // 绘制玩家昵称
    LPCTSTR lpszName = GetUserNameStr(m_nUserId);

    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldfont = (HFONT)SelectObject(parentDC, hfont);
    int mode = SetBkMode(parentDC, TRANSPARENT);
    COLORREF oldtextclr = SetTextColor(parentDC, GVW_TEXT_CLR);

    DrawText(parentDC, lpszName, _tcslen(lpszName), &rect, DT_BOTTOM | DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(parentDC, oldtextclr);
    SetBkMode(parentDC, mode);
    SelectObject(parentDC, oldfont);
}
#endif

HWND GamerVisualWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_hWnd = CreateWindowEx(0,
        GAMER_VISUAL_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    m_PicVisual = new CPictureEx(m_hWnd);
    assert(m_PicVisual != NULL);

    return m_hWnd;
}

// 父窗口的尺寸已经改变，设置标志以指示绘制窗口时，需要重新获取父窗口背景
void GamerVisualWnd::ParentWndSizeChanged(void)
{
    m_bReloadBkBitmap = TRUE;
}

// 保存父窗口背景。父窗口尺寸变化时需要重新保存。
void GamerVisualWnd::SaveParentBackground(void)
{
    // 取父窗口DC
    HDC parentDC = GetDC(m_hWndParent);
    assert(parentDC != NULL);
    
    SaveParentBackground(parentDC);

    // 释放父窗口DC
    ReleaseDC(m_hWndParent, parentDC);
}

// 保存父窗口背景
void GamerVisualWnd::SaveParentBackground(HDC parentDC)
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

void GamerVisualWnd::SetGamerId(int nUserId)
{
    m_nUserId = nUserId;

    if (nUserId == INVALID_USER_ID) { // 清除用户形象图
        m_PicVisual->UnLoad();

        InvalidateRect(m_hWnd, NULL, FALSE);
    } else {
        //
        // 目前，资源中仅准备男女各三张GIF形象图
        //
        int remaider = nUserId % 3;
        PLAYER_GENDER gender = GetUserGender(nUserId);
        HMODULE hModule = LoadLibrary(RES_IMAGE_DLL_NAME);

        if (hModule == NULL) {
            return;
        }

        if (gender == MALE) {
            switch (remaider) {
            case 0:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_M0), _T("GIF"));
                break;
            case 1:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_M1), _T("GIF"));
                break;
            case 2:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_M2), _T("GIF"));
                break;
            }
        } else {
            switch (remaider) {
            case 0:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_F0), _T("GIF"));
                break;
            case 1:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_F1), _T("GIF"));
                break;
            case 2:
                m_PicVisual->Load(hModule, MAKEINTRESOURCE(IDR_GAMER_VISUAL_F2), _T("GIF"));
                break;
            }
        }

        FreeLibrary(hModule);

        m_PicVisual->Draw();
    }
}

void GamerVisualWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);

    if (m_PicVisual != NULL) {
        RECT rect;
        SetRect(&rect, 0, 0,
            GVW_USER_VISUAL_WIDTH < cx ? GVW_USER_VISUAL_WIDTH : cx,
            GVW_USER_VISUAL_HEIGHT < cy ? GVW_USER_VISUAL_HEIGHT : cy);
        m_PicVisual->SetPaintRect(&rect);
    }
}

void GamerVisualWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    POINT pt;
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    //POINTSTOPOINT(pt, MAKEPOINTS(lParam));

    ClientToScreen(m_hWnd, &pt);
    
    if (!IS_INVALID_USER_ID(m_nUserId)) {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu != NULL) {
            TCHAR szInfo[80] = { 0 };

            _stprintf_s(szInfo, sizeof(szInfo) / sizeof(szInfo[0]),
                _T("玩家: [ %s ]"), GetUserNameStr(m_nUserId));
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1100, (LPCTSTR)szInfo);
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);

             _stprintf_s(szInfo, sizeof(szInfo) / sizeof(szInfo[0]),
                _T("级别: %s\t积分: %d\n"), GetUserLevelStr(m_nUserId), GetUserScore(m_nUserId));
             AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1100, (LPCTSTR)szInfo);

            _stprintf_s(szInfo, sizeof(szInfo) / sizeof(szInfo[0]),
                _T("胜: %d  负: %d\t局数: %d\n"),
                GetUserWinGames(m_nUserId),
                GetUserTotalGames(m_nUserId) - GetUserWinGames(m_nUserId),
                GetUserTotalGames(m_nUserId));
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1100, (LPCTSTR)szInfo);

            int nWinRate = 0;
            int nRunawayRate = 0;
            if (GetUserTotalGames(m_nUserId) != 0) {
                nWinRate = (int)((double)(GetUserWinGames(m_nUserId)) / (double)(GetUserTotalGames(m_nUserId)) * 100);
                nRunawayRate = (int)((double)(GetUserRunawayTimes(m_nUserId)) / (double)(GetUserTotalGames(m_nUserId)) * 100);
            }
            _stprintf_s(szInfo, sizeof(szInfo) / sizeof(szInfo[0]),
                _T("胜率: %d%%\t逃跑: %d%%\n"), nWinRate, nRunawayRate);
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1100, (LPCTSTR)szInfo);

            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1101, _T("请他离开"));
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1102, _T("加为好友"));
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);

            HMENU hMenuPop = CreatePopupMenu();
            if (hMenuPop != NULL) {
                AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hMenuPop, _T("MORE..."));

                AppendMenu(hMenuPop, MF_STRING, 1103, _T("send E-mail"));
                AppendMenu(hMenuPop, MF_STRING, 1104, _T("send files"));
                AppendMenu(hMenuPop, MF_STRING, 1105, _T("voice chat"));
                AppendMenu(hMenuPop, MF_STRING, 1106, _T("video chat"));

                TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, m_hWnd, NULL);

                DestroyMenu(hMenuPop);
            }

            DestroyMenu(hMenu);
        }
    }
}