//
// File: GamerVisualWnd.h
//
#pragma once

#define GAMER_VISUAL_WND_CLASS_NAME             _T("_GAMER_VISUAL_WND_")

#define GVW_TEXT_CLR                            RGB(255,255,255)

#define GVW_USER_VISUAL_WIDTH                   75
#define GVW_USER_VISUAL_HEIGHT                  125

#define GVW_USER_VISUAL_WND_WIDTH               GVW_USER_VISUAL_WIDTH
#define GVW_USER_VISUAL_WND_HEIGHT              (GVW_USER_VISUAL_HEIGHT + 20)


class GamerVisualWnd
{
    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

    int         m_nUserId;

    //
    // 本窗口主要目的是显示GIF动画，目前因使用PictureEx类，但其不能实现透明效果，
    // 这里将父窗口背景保存起来，并传递给PictureEx，同时需要对PictureEx作必要
    // 修改才能实现GIF透明效果。对其所作修改仅适合本软件设计需求。
    //
    HBITMAP     m_hBkBitmap; // 保存父窗口背景位图
    BOOL        m_bReloadBkBitmap; // 当尺寸变化时，指示重新获取父窗口背景

    CPictureEx* m_PicVisual;

public:
    GamerVisualWnd(void);
    ~GamerVisualWnd(void);

public:
    static ATOM GamerVisualWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK GamerVisualWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnPaint(HDC hdc);
    void OnRButtonDown(WPARAM wParam, LPARAM lParam);
    void SaveParentBackground(void);
    void SaveParentBackground(HDC parentDC);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetGamerId(int nUserId);
    void SetWindowRect(int x, int y, int cx, int cy);

    void ParentWndSizeChanged(void);

#ifdef PARENT_PAINT_CHILD
    void ParentPaintChild(HDC parentDC);
#endif

};

