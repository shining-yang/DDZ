//
// File: UserInfoWnd.h
//
//
#pragma once

#define USER_INFO_WND_CLASS_NAME            _T("_USER_INFO_WND_")


#define UIW_IMAGE_OFFSET                    16
#define UIW_NAME_OFFSET                     80
#define UIW_IMAGE_FRAME_GAP                 8
#define UIW_IAMGE_FRAME_ROUND_ANGEL         8
#define UIW_IMAGE_FRAME_WIDTH               2

#define UIW_BACK_CLR                        RGB(70,180,70)
#define UIW_IMAGE_FRAME_CLR                 RGB(192,168,128)

// 本窗口类在绘制时，使用到 g_himlUser32x32 图像列表。本窗口类不保存其复本，
// 即在销毁窗口前不应删除它。
class UserInfoWnd
{
    BOOL        m_bShow;
    HWND        m_hWnd;

    TCHAR       m_szUserName[MAX_USER_NAME_LEN];
    int         m_nImageIndex;

public:
    UserInfoWnd(void);
    ~UserInfoWnd(void);

public:
    static ATOM UserInfoWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK UserInfoWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnPaint(HDC hdc);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetUserInfo(LPCTSTR szUserName, int nImageIndex);
    void SetWindowRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);
};