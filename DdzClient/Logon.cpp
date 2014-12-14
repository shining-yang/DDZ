//
// File: Logon.cpp
//
// LOGON DIALOG
//
// IMPORTANT:
// The logon dialog used HIMAGELIST g_himlUser32x32, make sure the image list
// has already been created and initialized before call UserLogon().
//
#include "stdafx.h"
#include "DdzClient.h"
#include "AppResource.h"
#include "Logon.h"

extern HINSTANCE g_hAppInstance;

HWND g_hWndLogonTip = NULL;
BOOL g_bWndLogonTipVisible = FALSE;

// 简单自绘ComboBox
void DrawUserImgComboItem(LPDRAWITEMSTRUCT lpdis)
{
    int imgcount = ImageList_GetImageCount(g_himlUser32x32);
    if (lpdis->itemID >= (UINT)imgcount) {
        return;
    }

    IMAGEINFO imgInfo;
    if (ImageList_GetImageInfo(g_himlUser32x32, lpdis->itemID, &imgInfo) == FALSE) {
        return;
    }

    // 头像图标位图尺寸
    int cxImage = imgInfo.rcImage.right - imgInfo.rcImage.left;
    int cyImage = imgInfo.rcImage.bottom - imgInfo.rcImage.top;

    // 要绘制项的矩形尺寸
    int cxDrawItem = lpdis->rcItem.right - lpdis->rcItem.left;
    int cyDrawItem = lpdis->rcItem.bottom - lpdis->rcItem.top;

    // 创建缓冲内存DC及其绘制的位图
    HDC memDC = CreateCompatibleDC(lpdis->hDC);
    HBITMAP memBmp = CreateCompatibleBitmap(lpdis->hDC, cxDrawItem, cyDrawItem);
    HBITMAP oldmemBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // 将内存DC的位图全部刷为白色
    RECT rcFill;
    SetRect(&rcFill, 0, 0, cxDrawItem, cyDrawItem);
    FillRect(memDC, &rcFill, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // 如果该项被选中，将内存DC 从（1,1）开始，到图标尺寸＋2的矩形绘制蓝色边框
    if ((lpdis->itemState & ODS_FOCUS) || (lpdis->itemState & ODS_SELECTED)) {
        HBRUSH hbrush = CreateSolidBrush(RGB(0, 0, 255));
        SetRect(&rcFill, 1, 1, (1 + cxImage + 2), (1 + cyImage + 2));
        FrameRect(memDC, &rcFill, hbrush);
        DeleteObject(hbrush);
    }

    // 将相应图标绘制到内存DC的（2,2）开始处
    ImageList_Draw(g_himlUser32x32, lpdis->itemID, memDC, 2, 2, ILD_NORMAL);

    // 将内存DC刷到屏幕
    BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, cxDrawItem, cyDrawItem,
        memDC, 0, 0, SRCCOPY);

    // 复位原位图
    SelectObject(memDC, oldmemBmp);

    // 清理
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

// 初始化用户头像列表
void InitUserImageCombox(HWND hComboBox)
{
    int imgcount = ImageList_GetImageCount(g_himlUser32x32);

    for (int i = 0; i < imgcount; i++) {
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)i);
    }

    SendMessage(hComboBox, CB_SETCURSEL, 0, 0);
}

// 登陆对话框响应控件命令
void OnLogonDlgCommand(HWND hDlg, int nResult)
{
    int len = 0;
    TCHAR name[MAX_USER_NAME_LEN] = { 0 };
    int nImage = 0;
    BOOL bMale = TRUE;

    if (nResult == IDCANCEL) { // ESC取消或关闭窗口
        EndDialog(hDlg, nResult);
        return;
    }
    
    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_TRACK;

    if (nResult == IDOK) { // OK按钮
        len = GetDlgItemText(hDlg, IDC_LOGON_NAME, name, MAX_USER_NAME_LEN);

        // 设置昵称编辑框TIP的箭头点
        RECT rect;
        GetWindowRect(GetDlgItem(hDlg, IDC_LOGON_NAME), &rect);

        POINT pt;
        pt.x = rect.left + 4;
        pt.y = rect.bottom - 4;

        SendMessage(g_hWndLogonTip, TTM_TRACKPOSITION, 0, (LPARAM)POINTTOPOINTS(pt));

        if (len == 0) {
            ti.uId = IDC_LOGON_NAME;
            ti.lpszText = _T("请输入您的昵称(不超过31个字符)");
            SendMessage(g_hWndLogonTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            g_bWndLogonTipVisible = TRUE;
            SendMessage(g_hWndLogonTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
        } else if (name[0] == _T(' ')) {
            ti.uId = IDC_LOGON_NAME;
            ti.lpszText = _T("请不要使用空格作为昵称的开始");
            SendMessage(g_hWndLogonTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            g_bWndLogonTipVisible = TRUE;
            SendMessage(g_hWndLogonTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
        } else {
            nImage = SendMessage(GetDlgItem(hDlg, IDC_LOGON_USER_IMG), CB_GETCURSEL, 0, 0);
            bMale = BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_LOGON_MALE), BM_GETCHECK, 0, 0);

            SaveUserLogonInfo(hDlg, name, nImage, bMale);
            EndDialog(hDlg, IDOK);
        }
    } else { // 若TIP窗口已经显示，则使其隐藏
        if (g_bWndLogonTipVisible == TRUE) {
            SendMessage(g_hWndLogonTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
            g_bWndLogonTipVisible = FALSE;
        }
    }
}

// 登陆对话框窗口处理过程
INT_PTR CALLBACK LogonDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    int nResult;
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis;
    HICON hicon;

    switch (nMsg) {
    case WM_INITDIALOG:
        // 给对话框标题栏添加图标
        hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DDZCLIENT));
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hicon);

        // 保存输入的参数
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

        // 创建一个TIP提示控件
        g_hWndLogonTip = CreateWindow(TOOLTIPS_CLASS,
            NULL, TTS_NOPREFIX | TTS_BALLOON,
            0, 0, 0, 0, hDlg, NULL, g_hAppInstance, NULL);

        // 为登陆昵称编辑框添加TIP
        TOOLINFO ti;
        ZeroMemory(&ti, sizeof(TOOLINFO));
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_TRACK;
        ti.uId = IDC_LOGON_NAME;
        SendMessage(g_hWndLogonTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

        // 初始化用户头像列表
        InitUserImageCombox(GetDlgItem(hDlg, IDC_LOGON_USER_IMG));

        // 默认设置用户性别为男
        SendDlgItemMessage(hDlg, IDC_LOGON_MALE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
        return TRUE;

    case WM_COMMAND:
        nResult = LOWORD(wParam);
        OnLogonDlgCommand(hDlg, nResult);
        return TRUE;

    case WM_ACTIVATE:
    case WM_NCLBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (g_bWndLogonTipVisible == TRUE) { // 若TIP窗口已经显示，则使其隐藏
            SendMessage(g_hWndLogonTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
            g_bWndLogonTipVisible = FALSE;
        }
        return FALSE; // 返回FALSE，表示需要系统继续处理该消息

    case WM_MEASUREITEM:
        lpmis = (LPMEASUREITEMSTRUCT)lParam;

        if (lpmis->CtlType == ODT_COMBOBOX) {
            if (lpmis->CtlID == IDC_LOGON_USER_IMG) {
                lpmis->itemHeight = CY_USER_ICON_BIG + 4;
            }
        }
        return TRUE;

    case WM_DRAWITEM:
        lpdis = (LPDRAWITEMSTRUCT)lParam;

        if (lpdis->CtlType == ODT_COMBOBOX) {
            if (lpdis->CtlID == IDC_LOGON_USER_IMG) {
                DrawUserImgComboItem(lpdis);
            }
        }
        return TRUE;
    }

    return FALSE;
}

// 显示登陆对话框，进行用户登陆
BOOL UserLogon(HINSTANCE hInstance, HWND hWndParent, LPARAM lParam)
{
    int nResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_LOGON),
        hWndParent, LogonDialogProc, lParam);
    if (nResult == IDOK) {
        return TRUE;
    }

    return FALSE;
}

// 保存用户登陆信息
BOOL SaveUserLogonInfo(HWND hDlg, LPCTSTR name, int nImage, BOOL bMale)
{
    USER_LOGON_INFO* uli = (USER_LOGON_INFO*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    if (uli != NULL) {
        _tcscpy_s(uli->szName, MAX_USER_NAME_LEN, name);
        uli->nImage = nImage;
        uli->bMale = bMale;
        return TRUE;
    }

    return FALSE;
}
