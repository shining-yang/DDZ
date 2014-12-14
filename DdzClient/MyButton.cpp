//
// MyButton.cpp
//
//  Self-created image button which is not powerful while useful enough.
//
//  使用方法：
//      1.  在程序开始处注册窗口类，MyButtonRegister()
//      2.  Create()，SetBitmap()
//      3.  在父窗口中响应消息 WM_COMMAND
//
//
//  注意：按钮位图要求为5个相同大小，不同颜色的按钮排成一行。
//

#include "stdafx.h"
#include "MyButton.h"

// 构造函数
MyButton::MyButton(void)
{
    m_hInstance     = NULL;
    m_hWndParent    = NULL;

    m_nID           = 0;
    m_bShow         = TRUE;
    m_bEnable       = TRUE;
    m_bLButtonDown  = FALSE;
    m_bMouseEnter   = FALSE;
    m_hBtnBitmap    = NULL;
    m_clrMask       = RGB(255,255,255);

    m_clrText       = RGB(0,0,0);
    ZeroMemory(&m_szText, sizeof(m_szText));
}

// 析构函数
MyButton::~MyButton(void)
{
}

// 类注册函数
ATOM MyButton::MyButtonRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = MY_BUTTON_CLASS_NAME;
    wcex.lpfnWndProc	    = MyButtonProc;
    wcex.style			    = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra		    = 0;
    wcex.cbWndExtra		    = 0;
    wcex.hInstance		    = hInstance;
    wcex.hIcon			    = NULL;
    wcex.hCursor		    = LoadCursor(NULL, IDC_HAND);
    wcex.hbrBackground	    = NULL;
    wcex.lpszMenuName	    = NULL;
    wcex.hIconSm		    = NULL;

    return RegisterClassEx(&wcex);
}

// 窗口处理函数
LRESULT
CALLBACK
MyButton::MyButtonProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    MyButton* btn;
    LPCREATESTRUCT lpcs;
    
    btn = (MyButton*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
            break;

        case WM_DESTROY:
            if (btn->m_hBtnBitmap != NULL) {
                DeleteObject(btn->m_hBtnBitmap);
                btn->m_hBtnBitmap = NULL;
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            btn->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_RBUTTONDOWN:
            SendMessage(btn->m_hWndParent, nMsg, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            btn->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            btn->OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MOUSEMOVE:
            btn->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MOUSELEAVE:
            btn->OnMouseLeave(LOWORD(lParam), HIWORD(lParam));
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

// 创建按钮
HWND MyButton::Create(DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                      HWND hWndParent, UINT id, HINSTANCE hInstance)
{
    m_hWnd = CreateWindowEx(
        0,
        MY_BUTTON_CLASS_NAME,
        _T(""),
        dwStyle,
        x,
        y,
        nWidth,
        nHeight, 
        hWndParent,
        (HMENU)id,
        hInstance,
        this); // 将 this 指针传递给 WM_CREATE 保存起来

    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_nID = id;

    return m_hWnd;
}

// 设置按钮窗口的矩形
BOOL MyButton::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
    return TRUE;
}

// 设置按钮位图（来自位图资源）
BOOL MyButton::SetBitmap(UINT nResID, COLORREF clrMask)
{
    return SetBitmap(m_hInstance, MAKEINTRESOURCE(nResID), clrMask);
}

// 设置按钮位图（来自位图资源）
BOOL MyButton::SetBitmap(HMODULE hModule, LPCTSTR lpszResName, COLORREF clrMask)
{
    m_clrMask = clrMask;

    if (m_hBtnBitmap != NULL) {
        DeleteObject(m_hBtnBitmap);
        m_hBtnBitmap = NULL;
    }

    m_hBtnBitmap = (HBITMAP)LoadImage(hModule, lpszResName, IMAGE_BITMAP,
        0, 0, LR_DEFAULTCOLOR);

    if (m_bShow == TRUE) {
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    return TRUE;
}

// 设置按钮文本
BOOL MyButton::SetText(LPCTSTR lpszText, COLORREF clrText /*= RGB(0,0,0)*/)
{
    m_clrText = clrText;
    _tcscpy_s(m_szText, MY_BUTTON_TEXT_LEN, lpszText);
    m_szText[MY_BUTTON_TEXT_LEN - 1] = _T('\0');

    if (m_bShow == TRUE) {
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    return TRUE;
}

// 使能按钮
BOOL MyButton::Enable(BOOL bEnable /* = TRUE */)
{
    m_bEnable = bEnable;

    if (m_bShow == TRUE) {
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    EnableWindow(m_hWnd, bEnable);
    return TRUE;
}

// 显示或隐藏按钮
BOOL MyButton::Show(BOOL bShow /* = TRUE */)
{
    if (m_bShow != bShow) {
        m_bShow = bShow;
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
    }

    return TRUE;
}

// 按钮是否可见
BOOL MyButton::IsVisible(void)
{
    return m_bShow;
}

// 绘图
void MyButton::OnPaint(HDC hdc)
{
    if (m_hBtnBitmap == NULL) { return; }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx  = rect.right - rect.left;
    int cy  = rect.bottom - rect.top;

    // 获取保存的位图资源的尺寸
    BITMAP bi = { 0 };
    GetObject(m_hBtnBitmap, sizeof(BITMAP), &bi);
    int bmpWidth = bi.bmWidth;
    int bmpHeight = bi.bmHeight;

    int w = bmpWidth / 5;   // 单个按钮位图宽度
    int h = bmpHeight;      // 单个按钮位图高度

    int index = 0;

    // 按钮位图为5个不同颜色的按钮，确定选择哪个按钮位图
    if (m_bEnable == FALSE) {
        index = 4;
    } else if ((m_bMouseEnter == TRUE) && (m_bLButtonDown == FALSE)) {
        index = 3;
    } else if (m_bLButtonDown == FALSE) {
        index = 0;
    } else {
        index = 1;
    }

    int startx = w * index;

    // 创建临时DC，用于将按钮位图选入该DC
    HDC memTempDC = CreateCompatibleDC(hdc);
    assert(memTempDC != NULL);

    // 创建内存DC
    HDC memDC = CreateCompatibleDC(hdc);
    assert(memDC != NULL);

    // 创建掩码DC
    HDC memMaskDC = CreateCompatibleDC(hdc);
    assert(memMaskDC != NULL);

    // 创建最后准备刷向屏幕的内存DC
    HDC lastMemDC = CreateCompatibleDC(hdc);
    assert(lastMemDC != NULL);

    // 选定按钮位图
    HBITMAP hOldTempBmp = (HBITMAP)SelectObject(memTempDC, m_hBtnBitmap);

    // 内存DC的位图
    HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, bmpWidth, bmpHeight); // 5个按钮的尺寸
    assert(hMemBmp != NULL);

    HBITMAP hOldMemBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

    // 将按钮位图拷贝到内存DC的位图
    BitBlt(memDC, 0, 0, bmpWidth, bmpHeight, memTempDC, 0, 0, SRCCOPY);

    // MASK位图
    HBITMAP hMaskBmp = CreateBitmap(w, h, 1, 1, NULL); // 单个按钮的尺寸
    assert(hMaskBmp != NULL);

    HBITMAP hOldMaskBmp = (HBITMAP)SelectObject(memMaskDC, hMaskBmp);

    // 最后刷向屏幕的位图
    HBITMAP hLastMemBmp = CreateCompatibleBitmap(hdc, w, h); // 单个按钮的尺寸
    assert(hLastMemBmp != NULL);

    HBITMAP hLastOldMemBmp = (HBITMAP)SelectObject(lastMemDC, hLastMemBmp);

    {// 将父窗口背景复制过来，以便实现圆角按钮边缘处的透明
        HDC parentDC = GetDC(m_hWndParent);
        assert(parentDC != NULL);

        POINT pt = { 0, 0 };
        ClientToScreen(m_hWnd, &pt);
        ScreenToClient(m_hWndParent, &pt);
        BitBlt(lastMemDC, 0, 0, w, h, parentDC, pt.x, pt.y, SRCCOPY);
        ReleaseDC(m_hWndParent, parentDC);
    }

    //{{ Start to draw the button image in memory
    SetBkColor(memDC, m_clrMask);
    BitBlt(memMaskDC, 0, 0, w, h, memDC, startx, 0, SRCCOPY);
    BitBlt(lastMemDC, 0, 0, w, h, memDC, startx, 0, SRCINVERT);
    BitBlt(lastMemDC, 0, 0, w, h, memMaskDC, 0, 0, SRCAND);
    BitBlt(lastMemDC, 0, 0, w, h, memDC, startx, 0, SRCINVERT);
    //}} Finish drawing

    StretchBlt(hdc, 0, 0, cx, cy, lastMemDC, 0, 0, w, h, SRCCOPY);

    // 文本
    if (_tcslen(m_szText) > 0) {
        RECT rcText = { 0, 0, cx, cy };

        LOGFONT logFont = { 0 };
        logFont.lfCharSet = GB2312_CHARSET;
        logFont.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        logFont.lfWeight = FW_BOLD;
        _tcscpy_s(logFont.lfFaceName, sizeof(logFont.lfFaceName) / sizeof(logFont.lfFaceName[0]), _T("新宋体"));
        HFONT hFont = CreateFontIndirect(&logFont);

        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        COLORREF clrOldText = SetTextColor(hdc, m_clrText);
        int mode = SetBkMode(hdc, TRANSPARENT);

        DrawText(hdc, m_szText, _tcslen(m_szText), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetBkMode(hdc, mode);
        SetTextColor(hdc, clrOldText);
        SelectObject(hdc, oldFont);

        DeleteObject(hFont);
    }

    SelectObject(lastMemDC, hLastOldMemBmp);
    SelectObject(memMaskDC, hOldMaskBmp);
    SelectObject(memDC, hOldMemBmp);
    SelectObject(memTempDC, hOldTempBmp);

    DeleteObject(hLastMemBmp);
    DeleteObject(hMaskBmp);
    DeleteObject(hMemBmp);

    DeleteDC(lastMemDC);
    DeleteDC(memMaskDC);
    DeleteDC(memDC);
    DeleteDC(memTempDC);
}

// 鼠标按下
void MyButton::OnLButtonDown(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (m_bLButtonDown == FALSE) {
        SetCapture(m_hWnd);

        m_bLButtonDown = TRUE;

        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

// 鼠标弹起
void MyButton::OnLButtonUp(int x, int y)
{
    if (m_bLButtonDown == TRUE) {
        ReleaseCapture();

        //
        // 如果 LMouseUP 时，还光标还停留在按钮窗口的客户区，表示正确点击。
        //
        POINT pt;
        pt.x = x;
        pt.y = y;

        RECT rect;
        GetClientRect(m_hWnd, &rect);

        if (PtInRect(&rect, pt)) {
            SendMessage(m_hWndParent, WM_COMMAND, m_nID, 0);
        }

        m_bLButtonDown = FALSE;

        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

// 鼠标移动
void MyButton::OnMouseMove(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (m_bMouseEnter == FALSE) {
        m_bMouseEnter = TRUE;

        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        tme.dwFlags = TME_LEAVE;
        TrackMouseEvent(&tme);

        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

// 鼠标离开
void MyButton::OnMouseLeave(int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    m_bMouseEnter = FALSE;

    InvalidateRect(m_hWnd, NULL, FALSE);
}

// 父窗口在绘制时调用本函数对按钮进行绘制
#ifdef PARENT_PAINT_CHILD
void MyButton::ParentPaintChild(HDC parentDC)
{
    if (m_hBtnBitmap == NULL) { return; }

    if (m_bShow == FALSE) { return; }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int cx  = rect.right - rect.left;
    int cy  = rect.bottom - rect.top;

    // 获取保存的位图资源的尺寸
    BITMAP bi = { 0 };
    GetObject(m_hBtnBitmap, sizeof(BITMAP), &bi);
    int bmpWidth = bi.bmWidth;
    int bmpHeight = bi.bmHeight;

    int w = bmpWidth / 5;   // 单个按钮位图宽度
    int h = bmpHeight;      // 单个按钮位图高度

    int index = 0;

    // 按钮位图为5个不同颜色的按钮，确定选择哪个按钮位图
    if (m_bEnable == FALSE) {
        index = 4;
    } else if ((m_bMouseEnter == TRUE) && (m_bLButtonDown == FALSE)) {
        index = 3;
    } else if (m_bLButtonDown == FALSE) {
        index = 0;
    } else {
        index = 1;
    }

    int startx = w * index;

    // 创建临时DC，用于将按钮位图选入该DC
    HDC memTempDC = CreateCompatibleDC(parentDC);
    assert(memTempDC != NULL);

    // 创建内存DC
    HDC memDC = CreateCompatibleDC(parentDC);
    assert(memDC != NULL);

    // 创建掩码DC
    HDC memMaskDC = CreateCompatibleDC(parentDC);
    assert(memMaskDC != NULL);

    // 创建最后准备刷向屏幕的内存DC
    HDC lastMemDC = CreateCompatibleDC(parentDC);
    assert(lastMemDC != NULL);

    // 选定按钮位图
    HBITMAP hOldTempBmp = (HBITMAP)SelectObject(memTempDC, m_hBtnBitmap);

    // 内存DC的位图
    HBITMAP hMemBmp = CreateCompatibleBitmap(parentDC, bmpWidth, bmpHeight); // 5个按钮的尺寸
    assert(hMemBmp != NULL);

    HBITMAP hOldMemBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

    // 将按钮位图拷贝到内存DC的位图
    BitBlt(memDC, 0, 0, bmpWidth, bmpHeight, memTempDC, 0, 0, SRCCOPY);

    // MASK位图
    HBITMAP hMaskBmp = CreateBitmap(w, h, 1, 1, NULL); // 单个按钮的尺寸
    assert(hMaskBmp != NULL);

    HBITMAP hOldMaskBmp = (HBITMAP)SelectObject(memMaskDC, hMaskBmp);

    // 最后刷向屏幕的位图
    HBITMAP hLastMemBmp = CreateCompatibleBitmap(parentDC, w, h); // 单个按钮的尺寸
    assert(hLastMemBmp != NULL);

    HBITMAP hLastOldMemBmp = (HBITMAP)SelectObject(lastMemDC, hLastMemBmp);

    {// 将父窗口背景复制过来，以便实现圆角按钮边缘处的透明
        POINT pt = { 0, 0 };
        ClientToScreen(m_hWnd, &pt);
        ScreenToClient(m_hWndParent, &pt);
        BitBlt(lastMemDC, 0, 0, w, h, parentDC, pt.x, pt.y, SRCCOPY);
    }

    //{{ Start to draw the button image in memory
    SetBkColor(memDC, m_clrMask);
    BitBlt(memMaskDC, 0, 0, w, h, memDC, startx, 0, SRCCOPY);
    BitBlt(lastMemDC, 0, 0, w, h, memDC, startx, 0, SRCINVERT);
    BitBlt(lastMemDC, 0, 0, w, h, memMaskDC, 0, 0, SRCAND);
    BitBlt(lastMemDC, 0, 0, w, h, memDC, startx, 0, SRCINVERT);
    //}} Finish drawing

    POINT pt = { 0, 0 };
    ClientToScreen(m_hWnd, &pt);
    ScreenToClient(m_hWndParent, &pt);

    StretchBlt(parentDC, pt.x, pt.y, cx, cy, lastMemDC, 0, 0, w, h, SRCCOPY);

    // 文本
    if (_tcslen(m_szText) > 0) {
        RECT rcText = { pt.x, pt.y, pt.x + cx, pt.y + cy };

        LOGFONT logFont = { 0 };
        logFont.lfCharSet = GB2312_CHARSET;
        logFont.lfHeight = -MulDiv(10, GetDeviceCaps(parentDC, LOGPIXELSY), 72);
        logFont.lfWeight = FW_BOLD;
        _tcscpy_s(logFont.lfFaceName, sizeof(logFont.lfFaceName) / sizeof(logFont.lfFaceName[0]), _T("新宋体"));
        HFONT hFont = CreateFontIndirect(&logFont);

        HFONT oldFont = (HFONT)SelectObject(parentDC, hFont);
        COLORREF clrOldText = SetTextColor(parentDC, m_clrText);
        int mode = SetBkMode(parentDC, TRANSPARENT);

        DrawText(parentDC, m_szText, _tcslen(m_szText), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetBkMode(parentDC, mode);
        SetTextColor(parentDC, clrOldText);
        SelectObject(parentDC, oldFont);

        DeleteObject(hFont);
    }

    SelectObject(lastMemDC, hLastOldMemBmp);
    SelectObject(memMaskDC, hOldMaskBmp);
    SelectObject(memDC, hOldMemBmp);
    SelectObject(memTempDC, hOldTempBmp);

    DeleteObject(hLastMemBmp);
    DeleteObject(hMaskBmp);
    DeleteObject(hMemBmp);

    DeleteDC(lastMemDC);
    DeleteDC(memMaskDC);
    DeleteDC(memDC);
    DeleteDC(memTempDC);

    //
    // Not validate the button client area.
    // The button may expect some mouse messages for painting.
    //
    //ValidateRect(m_hWnd, NULL);
}
#endif

