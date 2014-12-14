//
// File: UserInfoWnd.cpp
//
//      显示登陆用户的昵称和头像
//
#include "stdafx.h"
#include "DdzClient.h"
#include "UserInfoWnd.h"


UserInfoWnd::UserInfoWnd(void)
{
    m_bShow = TRUE;
    m_hWnd = NULL;
    ZeroMemory(m_szUserName, sizeof(m_szUserName));

    m_nImageIndex = -1;
}

UserInfoWnd::~UserInfoWnd(void)
{

}

ATOM UserInfoWnd::UserInfoWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = USER_INFO_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = UserInfoWndProc;
    wcex.style			    = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra		    = 0;
    wcex.cbWndExtra		    = 0;
    wcex.hInstance		    = hInstance;
    wcex.hIcon			    = NULL;
    wcex.hCursor		    = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	    = NULL;//(HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	    = NULL;
    wcex.hIconSm		    = NULL;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK UserInfoWnd::UserInfoWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    UserInfoWnd* lpWnd = (UserInfoWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
    case WM_CREATE:
        lpcs = (LPCREATESTRUCT)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
        break;

    case WM_DESTROY:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        lpWnd->OnPaint(hdc);
        EndPaint(hWnd, &ps);
        break;

    default:
        return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

void UserInfoWnd::OnPaint(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    HBRUSH hbrush = CreateSolidBrush(UIW_BACK_CLR);
    FillRect(hdc, &rect, hbrush);
    DeleteObject(hbrush);

    if (m_nImageIndex < 0) {
        return;
    }

    IMAGEINFO imgInfo;
    if (ImageList_GetImageInfo(g_himlUser32x32, m_nImageIndex, &imgInfo) == FALSE) {
        return;
    }

    // 头像图标位图尺寸
    int cxImage = imgInfo.rcImage.right - imgInfo.rcImage.left;
    int cyImage = imgInfo.rcImage.bottom - imgInfo.rcImage.top;

    int xoffset = UIW_IMAGE_OFFSET;
    int yoffset = ((rect.bottom - rect.top) - cyImage + 1) / 2;

    // 在矩形中垂直居中显示头像
    ImageList_Draw(g_himlUser32x32, m_nImageIndex, hdc, xoffset, yoffset, ILD_NORMAL);

    // 给头像画一个边框
    RECT rcFrame = {
        xoffset - UIW_IMAGE_FRAME_GAP,
        yoffset - UIW_IMAGE_FRAME_GAP, 
        xoffset + cxImage + UIW_IMAGE_FRAME_GAP,
        yoffset + cyImage + UIW_IMAGE_FRAME_GAP
    };

    HPEN hpen = CreatePen(PS_SOLID, UIW_IMAGE_FRAME_WIDTH, UIW_IMAGE_FRAME_CLR);

    HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
    HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    RoundRect(hdc, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom,
        UIW_IAMGE_FRAME_ROUND_ANGEL,
        UIW_IAMGE_FRAME_ROUND_ANGEL);

    SelectObject(hdc, oldpen);
    SelectObject(hdc, oldbrush);
    DeleteObject(hpen);

    // 显示用户昵称
    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldfont = (HFONT)SelectObject(hdc, hfont);
    int mode = SetBkMode(hdc, TRANSPARENT);

    TCHAR szTemp[MAX_USER_NAME_LEN + 4] = { 0 };
    _tcscpy_s(szTemp, sizeof(szTemp) / sizeof(szTemp[0]), _T("昵称: "));
    _tcscat_s(szTemp, sizeof(szTemp) / sizeof(szTemp[0]), m_szUserName);

    rect.left = UIW_NAME_OFFSET;
    DrawText(hdc, szTemp, _tcslen(szTemp), &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS);

    SetBkMode(hdc, mode);
    SelectObject(hdc, oldfont);
}

HWND UserInfoWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWnd = CreateWindowEx(0,
        USER_INFO_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x,
        y,
        cx,
        cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    return m_hWnd;
}

void UserInfoWnd::SetUserInfo(LPCTSTR szUserName, int nImageIndex)
{
    _tcscpy_s(m_szUserName, MAX_USER_NAME_LEN, szUserName);
    m_nImageIndex = nImageIndex;

    InvalidateRect(m_hWnd, NULL, FALSE);
}

void UserInfoWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void UserInfoWnd::Show(BOOL bShow /*= TRUE*/)
{
    if (m_bShow != bShow) {
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        m_bShow = bShow;
    }
}

BOOL UserInfoWnd::IsVisible(void)
{
    return m_bShow;
}