//
// File: HomePageWnd.h
//
// shining      2009-05-10      initial creation
//
#pragma once

#define HOME_PAGE_WND_CLASS_NAME        _T("_HPW_CLASS_")

//
// 本窗口类主要目的是显示一个HTML页面，以作为斗地主游戏的帮助页面。
//
// MSDN中没有具体介绍如何在Win32项目中嵌入IE。
//
// 从网上找到的CWebPage，它支持Win32（非MFC）程序嵌入IE浏览器。将其DLL源码项目
// 加入本解决方案，与本项目一起编译。创建本窗口时将加载其DLL，窗口销毁时卸载。
//
// CWebPage好像比较神秘，我没看懂其嵌入IE的具体操作方法。只是依照其附带的示例，
// 在本窗口中使用该DLL来实现HTML页面的加载与显示。
//
// CWebPage似乎也比较诡异。程序没有使用该模块时，运行时进程内存使用量不超过4MB，
// 但加载该DLL之后，程序运行时进程占用的内存超过20MB，而且猛然增加了10多个未知
// 线程，可能IE控件本来就需要这么多的资源吧。未知的东西比较多，无法进行裁剪。
//
// 需要注意：CWebPage将IE嵌入窗口时，它会使用窗口的USER_DATA，因此，不要在本窗口
// 类中，调用如此语句： SetWindowLongPtr(hWnd, GWLP_USERDATA, pMyData)
// 
// 定义如下宏，将不使用CWebPage。
//
//#define NOT_USE_CWEBPAGE_DLL

class HomePageWnd
{
    HWND    m_hWnd;

    static int m_snInstances;
    static HMODULE m_shCWebPageDll;

    static EmbedBrowserObjectPtr        *lpEmbedBrowserObject;
    static UnEmbedBrowserObjectPtr      *lpUnEmbedBrowserObject;
    static DisplayHTMLPagePtr           *lpDisplayHTMLPage;
    static DisplayHTMLStrPtr            *lpDisplayHTMLStr;
    static ResizeBrowserPtr             *lpResizeBrowser;

public:
    HomePageWnd(void);
    ~HomePageWnd(void);

protected:
    static BOOL LoadDll(void);
    static void UnLoadDll(void);

    static void OnWndPaint(HWND hWnd, HDC hdc);
    static void OnWndSize(HWND hWnd, LPARAM lParam);

public:
    static ATOM HomePageWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK HomePageWndProc(HWND, UINT, WPARAM, LPARAM);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    void SetPageURL(LPCTSTR szURL);
    void SetWindowRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);

    operator HWND() const {
        return m_hWnd;
    }
};