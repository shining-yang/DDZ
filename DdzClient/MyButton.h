//
// MyButton.h
//
//  自绘位图按钮，功能并不丰富强大，够用而已。
//
//  仅支持位图文件。
//

#pragma once

#define MY_BUTTON_CLASS_NAME    _T("_MyButton_")

#define MY_BUTTON_TEXT_LEN         8

class MyButton
{
private:
    HINSTANCE       m_hInstance;
    HWND            m_hWndParent;
    
private:
    UINT            m_nID;
    HWND            m_hWnd;
    COLORREF        m_clrMask;
    HBITMAP         m_hBtnBitmap;
    TCHAR           m_szText[MY_BUTTON_TEXT_LEN];
    COLORREF        m_clrText;

private:
    BOOL            m_bEnable;
    BOOL            m_bShow;
    BOOL            m_bLButtonDown;
    BOOL            m_bMouseEnter;

public:
    MyButton(void);
    ~MyButton(void);

private:
    void OnPaint(HDC hdc);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseLeave(int x, int y);

public:
    static ATOM MyButtonRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK MyButtonProc(HWND hWnd, UINT nMsg,
        WPARAM wParam, LPARAM lParam);

public:
    HWND Create(DWORD dwStyle, 
        int x, 
        int y, 
        int nWidth, 
        int nHeight, 
        HWND hWndParent, 
        UINT id, 
        HINSTANCE hInstance);

    BOOL SetWindowRect(int x, int y, int cx, int cy);
    BOOL SetBitmap(UINT nResID, COLORREF clrMask);
    BOOL SetBitmap(HMODULE hModule, LPCTSTR lpszResName, COLORREF clrMask);
    BOOL SetText(LPCTSTR lpszText, COLORREF clrText = RGB(0,0,0));
    BOOL Enable(BOOL bEnable = TRUE);
    BOOL Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);

#ifdef PARENT_PAINT_CHILD
    void ParentPaintChild(HDC parentDC);
#endif
};