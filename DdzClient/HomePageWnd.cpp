//
// File: HomePageWnd.cpp
//
//  ÏÔÊ¾Ê×Ò³
//
#include "stdafx.h"
#include "HomePageWnd.h"

int HomePageWnd::m_snInstances = 0;
HMODULE HomePageWnd::m_shCWebPageDll = NULL;

EmbedBrowserObjectPtr*      HomePageWnd::lpEmbedBrowserObject = NULL;
UnEmbedBrowserObjectPtr*    HomePageWnd::lpUnEmbedBrowserObject = NULL;
DisplayHTMLPagePtr*         HomePageWnd::lpDisplayHTMLPage = NULL;
DisplayHTMLStrPtr*          HomePageWnd::lpDisplayHTMLStr = NULL;
ResizeBrowserPtr*           HomePageWnd::lpResizeBrowser = NULL;

HomePageWnd::HomePageWnd(void)
{
    m_hWnd = NULL;
}

HomePageWnd::~HomePageWnd(void)
{
}

BOOL HomePageWnd::LoadDll(void)
{
    m_shCWebPageDll = LoadLibrary(CWEBPAGE_DLL_NAME);
    if (m_shCWebPageDll == NULL) {
        MessageBox(NULL, _T("Load cwebpage.dll FAILED! Program will exit."),
            _T("ERROR"), MB_OK | MB_ICONERROR);
        PostQuitMessage(-1);
        return FALSE;
    }

    lpEmbedBrowserObject = (EmbedBrowserObjectPtr *)GetProcAddress(
        (HINSTANCE)m_shCWebPageDll, "EmbedBrowserObject");

    lpUnEmbedBrowserObject = (UnEmbedBrowserObjectPtr *)GetProcAddress(
        (HINSTANCE)m_shCWebPageDll, "UnEmbedBrowserObject");

    lpDisplayHTMLPage = (DisplayHTMLPagePtr *)GetProcAddress(
        (HINSTANCE)m_shCWebPageDll, "DisplayHTMLPage");

    lpDisplayHTMLStr = (DisplayHTMLStrPtr *)GetProcAddress(
        (HINSTANCE)m_shCWebPageDll, "DisplayHTMLStr");

    lpResizeBrowser = (ResizeBrowserPtr *)GetProcAddress(
        (HINSTANCE)m_shCWebPageDll, "ResizeBrowser");

    if ((lpEmbedBrowserObject == NULL) || (lpUnEmbedBrowserObject == NULL) ||
        (lpDisplayHTMLPage == NULL) || (lpDisplayHTMLStr == NULL) ||
        (lpResizeBrowser == NULL)) {
        MessageBox(NULL, _T("Load FUNCTION in cwebpage.dll FAILED! Check DLL version! Program will exit."),
            _T("ERROR"), MB_OK | MB_ICONERROR);

        UnLoadDll();
        PostQuitMessage(-1);
        return FALSE;
    }

    return TRUE;
}

void HomePageWnd::UnLoadDll(void)
{
    if (m_shCWebPageDll != NULL) {
        FreeLibrary(m_shCWebPageDll);
        m_shCWebPageDll = NULL;
    }
}

ATOM HomePageWnd::HomePageWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.lpszClassName      = HOME_PAGE_WND_CLASS_NAME;
    wcex.lpfnWndProc	    = HomePageWndProc;
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

LRESULT CALLBACK HomePageWnd::HomePageWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (nMsg) {
#ifndef NOT_USE_CWEBPAGE_DLL
        case WM_CREATE:
            if (m_snInstances == 0) {
                if (LoadDll() == FALSE) {
                    return -1;
                }
            }

            m_snInstances++;
            if ((*lpEmbedBrowserObject)(hWnd)) {
                return -1;
            }
            break;

        case WM_DESTROY:
            (*lpUnEmbedBrowserObject)(hWnd);

            m_snInstances--;
            if (m_snInstances == 0) {
                UnLoadDll();
            }
            break;
#endif

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            OnWndPaint(hWnd, hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_SIZE:
            OnWndSize(hWnd, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND HomePageWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWnd = CreateWindowEx(0,
        HOME_PAGE_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x,
        y,
        cx,
        cy,
        hWndParent,
        NULL,
        hInstance,
        NULL);

    assert(m_hWnd);
    return m_hWnd;
}

void HomePageWnd::OnWndPaint(HWND hWnd, HDC hdc)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hdc);

#ifdef NOT_USE_CWEBPAGE_DLL
    RECT rect;
    GetClientRect(hWnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
#endif
}

void HomePageWnd::OnWndSize(HWND hWnd, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lParam);

#ifndef NOT_USE_CWEBPAGE_DLL
    (*lpResizeBrowser)(hWnd, LOWORD(lParam), HIWORD(lParam));
#endif
}

void HomePageWnd::SetPageURL(LPCTSTR szURL)
{
    UNREFERENCED_PARAMETER(szURL);

#ifndef NOT_USE_CWEBPAGE_DLL
    (*lpDisplayHTMLPage)(m_hWnd, szURL);
#endif
}

void HomePageWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void HomePageWnd::Show(BOOL bShow /*= TRUE*/)
{
    ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

