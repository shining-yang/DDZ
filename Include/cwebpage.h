/*
* This include file is meant to be included with any C source you
* write which uses the cwebpage DLL.
*/

#ifndef __CWEBPAGE_H_INCLUDED
#define __CWEBPAGE_H_INCLUDED

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

    long WINAPI EmbedBrowserObject(HWND);
#define EMBEDBROWSEROBJECT EmbedBrowserObject
    typedef long WINAPI EmbedBrowserObjectPtr(HWND);

    void WINAPI UnEmbedBrowserObject(HWND);
#define UNEMBEDBROWSEROBJECT UnEmbedBrowserObject
    typedef long WINAPI UnEmbedBrowserObjectPtr(HWND);

    long WINAPI DisplayHTMLPage(HWND, LPCTSTR);
#define DISPLAYHTMLPAGE DisplayHTMLPage
    typedef long WINAPI DisplayHTMLPagePtr(HWND, LPCTSTR);

    long WINAPI DisplayHTMLStr(HWND, LPCTSTR);
#define DISPLAYHTMLSTR DisplayHTMLStr
    typedef long WINAPI DisplayHTMLStrPtr(HWND, LPCTSTR);


    //////////////////////////////////////////////////////////////////////////
    // Shining Added this API for usage. (2009-05-10)
    //
    long WINAPI ResizeBrowser(HWND, int, int);
#define RESIZEBROWSER ResizeBrowser
    typedef long WINAPI ResizeBrowserPtr(HWND, int, int);


#ifdef __cplusplus
}
#endif

#endif /* __CWEBPAGE_H_INCLUDED */
