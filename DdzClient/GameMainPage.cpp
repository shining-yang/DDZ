//
// File: GameMainPage.cpp
//
//  游戏主页面。本页面为一个窗口容器，包括游戏窗口、游戏桌玩家列表、LOG信息窗口。
//
#include "stdafx.h"
#include "NetProcess.h"
#include "MyButton.h"
#include "CurPokerWnd.h"
#include "OutCardWnd.h"
#include "UnderCardWnd.h"
#include "GamerVisualWnd.h"
#include "GameMainWnd.h"

#include "DdzClient.h"

#include "GameMainPage.h"

TCHAR* g_lpszChatMessage[] = {
    _T("大家好! 很高兴见到大家"),
    _T("快点吧! 我等得都不耐烦了"),
    _T("太弱了! 跟你合作是件可怕的事情"),
    _T("太强了! 你是斗地主学院毕业的吧"),
    _T("太爽了! 跟你合作真是太愉快了"),
    _T("我怒了! 我要赢死你们"),
    _T("再见了! 我会想念大家的")
};

COLORREF g_clrLogMessage[] = {
    RGB(255,255,255),   // INFO
    RGB(255,64,0),      // WARN
    RGB(255,0,0),       // ERROR
    RGB(128,128,128)    // DEBUG
};


GameMainPage::GameMainPage(void)
{
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;

    m_hUserList = NULL;
    m_hLogWnd = NULL;
    m_hChatMsgCombo = NULL;

    m_hComboEdit = NULL;
    m_lpComboEditWndProc = NULL;

    m_bShow = TRUE;
    m_bLogWndVisible = TRUE;
}

GameMainPage::~GameMainPage(void)
{

}

ATOM GameMainPage::GameMainPageRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpszClassName  = GAME_MAIN_PAGE_CLASS_NAME;
    wcex.lpfnWndProc    = GameMainPageProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GameMainPage::GameMainPageProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCT lpcs;
    GameMainPage* lpWnd = (GameMainPage*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            SetWindowLongPtr(lpWnd->m_hComboEdit, GWLP_WNDPROC, (LONG_PTR)lpWnd->m_lpComboEditWndProc);
            SetWindowLongPtr(lpWnd->m_hComboEdit, GWLP_USERDATA, NULL);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_SIZE:
            lpWnd->OnSize(wParam, lParam);
            break;

        case WM_NOTIFY:
            lpWnd->OnNotify(wParam, lParam);
            break;

        case WM_COMMAND:
            lpWnd->OnCommand(wParam, lParam);
            break;

        case WM_CHATMSG_VKEY:
            lpWnd->OnChatMsgKeyDown(wParam, lParam);
            break;

        case WM_WRITE_LOG:
            lpWnd->WriteLog(COLORREF(wParam), LPCTSTR(lParam));
            break;

        case WM_GAME_OVER:
            lpWnd->OnGameOver(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND GameMainPage::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;
    m_hWnd = CreateWindowEx(0,
        GAME_MAIN_PAGE_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    CreateChildWindows();
    return m_hWnd;
}

void GameMainPage::CreateChildWindows(void)
{
    m_GameMainWnd.Create(0, 0, 0, 0, m_hWnd, m_hInstance);
    m_GameMainWnd.SetBackBitmap(g_hbmpGameBG);

    m_hUserList = CreateWindowEx(0,
        WC_LISTVIEW,
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
        LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS,
        0, 0, 0, 0,
        m_hWnd,
        (HMENU)GMP_LISTVIEW_CTRL_ID,
        m_hInstance,
        NULL);
    assert(m_hUserList != NULL);

    InitListCtrl();

    // RichEdit
    HINSTANCE hRichED = LoadLibrary(_T("riched20.dll"));
    if (hRichED == NULL) {
        MessageBox(NULL, _T("您的系统没有 riched20.dll，或其版本太低，某些功能将不可用。"), _T("错误"), MB_OK);
    }

    m_hLogWnd = CreateWindow(RICHEDIT_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY,
        0, 0, 0, 0,
        m_hWnd,
        (HMENU)GMP_RICHEDIT_CTRL_ID,
        m_hInstance,
        NULL);
    assert(m_hLogWnd != NULL);

    InitRichEditCtrl();

    m_hChatMsgCombo = CreateWindow(WC_COMBOBOX, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN,
        0, 0, 0, 0,
        m_hWnd,
        (HMENU)GMP_COMBO_CHATMSG_ID,
        m_hInstance,
        NULL);
    assert(m_hChatMsgCombo != NULL);

    InitComboCtrl();

    //
    // Replace the Combobox-Edit window procedure with customized one, and save it.
    //
    m_hComboEdit = GetWindow(m_hChatMsgCombo, GW_CHILD);
    assert(m_hComboEdit != NULL);

    m_lpComboEditWndProc = (WNDPROC)SetWindowLongPtr(m_hComboEdit, GWLP_WNDPROC, (LONG_PTR)MyComboEditProc);
    assert(m_lpComboEditWndProc != NULL);

    SetWindowLongPtr(m_hComboEdit, GWLP_USERDATA, (LONG_PTR)this);
}

void GameMainPage::InitListCtrl(void)
{
    SendMessage(m_hUserList, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);

    ListView_SetBkColor(m_hUserList, GMP_USERLIST_BACK_CLR);
    ListView_SetTextColor(m_hUserList, GMP_USERLIST_TEXT_CLR);
    ListView_SetTextBkColor(m_hUserList, GMP_USERLIST_BACK_CLR);

    ListView_SetExtendedListViewStyle(m_hUserList, LVS_EX_FULLROWSELECT);

    ListView_SetImageList(m_hUserList, g_himlUser32x32, LVSIL_NORMAL);
    ListView_SetImageList(m_hUserList, g_himlUser16x16, LVSIL_SMALL);

    struct _lv_colum_data {
        LPTSTR  label;
        int     width;
        int     format;
    } lvcolumn[] = {
        { _T(""),       GMP_LIST_IMAGE_COLUMN_WIDTH,    LVCFMT_LEFT },
        { _T("昵称"),   GMP_LIST_NAME_COLUMN_WIDTH,     LVCFMT_LEFT },
        { _T("级别"),   GMP_LIST_LEVEL_COLUMN_WIDTH,    LVCFMT_LEFT },
        { _T("积分"),   GMP_LIST_SCORE_COLUMN_WIDTH,    LVCFMT_LEFT }
    };

    for (int i = 0; i < (sizeof(lvcolumn) / sizeof(lvcolumn[0])); i++) {
        LVCOLUMN lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
        lvc.pszText = lvcolumn[i].label;
        lvc.cchTextMax = _tcslen(lvcolumn[i].label);
        lvc.cx = lvcolumn[i].width;
        lvc.fmt = lvcolumn[i].format;

        ListView_InsertColumn(m_hUserList, i, &lvc);
    }
}

void GameMainPage::InitRichEditCtrl(void)
{
    SendMessage(m_hLogWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    SendMessage(m_hLogWnd, EM_SETBKGNDCOLOR, 0, GMP_LOGWND_BACK_CLR);
    SendMessage(m_hLogWnd, EM_SETEVENTMASK, 0, (LPARAM)(ENM_KEYEVENTS | ENM_LINK | ENM_MOUSEEVENTS));
    SendMessage(m_hLogWnd, EM_AUTOURLDETECT, TRUE, 0);
}

void GameMainPage::InitComboCtrl(void)
{
    SendMessage(m_hChatMsgCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);

    for (int i = 0; i < sizeof(g_lpszChatMessage) / sizeof(g_lpszChatMessage[0]); i++) {
        SendMessage(m_hChatMsgCombo, CB_ADDSTRING, i, (LPARAM)g_lpszChatMessage[i]);
    }

    SendMessage(m_hChatMsgCombo, CB_SETCURSEL, (WPARAM)-1, 0);
}

void GameMainPage::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void GameMainPage::Show(BOOL bShow /*= TRUE*/)
{
    if (m_bShow != bShow) {
        m_bShow = bShow;
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
    }
}

BOOL GameMainPage::IsVisible(void)
{
    return m_bShow;
}

void GameMainPage::OnSize(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);
    if ((cx <= 0) || (cy <= 0)) { return; }

    if (cx < GMP_MIN_WIDTH) {
        if (m_bLogWndVisible == TRUE) {
            m_bLogWndVisible = FALSE;

            ShowWindow(m_hUserList, SW_HIDE);
            ShowWindow(m_hLogWnd, SW_HIDE);
            ShowWindow(m_hChatMsgCombo, SW_HIDE);
        }
    } else if (cx > GMP_MIN_WIDTH + GMP_LOGWND_WIDTH / 3) {
        if (m_bLogWndVisible == FALSE) {
            m_bLogWndVisible = TRUE;

            ShowWindow(m_hUserList, SW_SHOW);
            ShowWindow(m_hLogWnd, SW_SHOW);
            ShowWindow(m_hChatMsgCombo, SW_SHOW);
        }
    }

    if (m_bLogWndVisible == FALSE) {
        m_GameMainWnd.SetWindowRect(0, 0, cx, cy);
    } else {
        m_GameMainWnd.SetWindowRect(0, 0, cx - GMP_USERLIST_WIDTH, cy);

        MoveWindow(m_hUserList, cx - GMP_USERLIST_WIDTH, 0,
            GMP_USERLIST_WIDTH, GMP_USERLIST_HEIGHT, TRUE);

        MoveWindow(m_hChatMsgCombo, cx - GMP_CHATMSG_COMBO_WITH, cy - GMP_CHATMSG_COMBO_HEIGHT,
            GMP_CHATMSG_COMBO_WITH, GMP_CHATMSG_COMBO_HEIGHT, TRUE);

        MoveWindow(m_hLogWnd, cx - GMP_LOGWND_WIDTH, GMP_USERLIST_HEIGHT,
            GMP_LOGWND_WIDTH, cy - GMP_USERLIST_HEIGHT - GMP_CHATMSG_COMBO_HEIGHT, TRUE);
    }
}

// 向列表控件添加一个玩家
void GameMainPage::ListCtrlAddUser(int nUserId)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.iImage  = I_IMAGECALLBACK;
    lvi.lParam  = (LPARAM)nUserId;

    ListView_InsertItem(m_hUserList, &lvi);
}

// 从列表控件删除一个玩家
void GameMainPage::ListCtrlRemoveUser(int nUserId)
{
    int nCount = ListView_GetItemCount(m_hUserList);

    LVITEM lvi = { 0 };

    for (int i = 0; i < nCount; i++) {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;

        if (ListView_GetItem(m_hUserList, &lvi) == TRUE) {
            if (lvi.lParam == nUserId) {
                ListView_DeleteItem(m_hUserList, i);
                break;
            }
        }
    }
}

// 将玩家列表清空
void GameMainPage::ListCtrlClear(void)
{
    ListView_DeleteAllItems(m_hUserList);
}

// 写日志
void GameMainPage::WriteLog(LPCTSTR lpszMsg)
{
    WriteLog(GMP_DEF_LOGTEXT_CLR, lpszMsg);
}

void GameMainPage::WriteLog(LOG_TYPE type, LPCTSTR lpszMsg)
{
    assert((type >= 0) && (type < sizeof(g_clrLogMessage) / sizeof(g_clrLogMessage[0])));
    COLORREF clr = g_clrLogMessage[type];
    WriteLog(clr, lpszMsg);
}

void GameMainPage::WriteLog(COLORREF clrText, LPCTSTR lpszMsg)
{
    GETTEXTLENGTHEX gtlex;
    gtlex.codepage = 1200;
    gtlex.flags = GTL_DEFAULT;

    int nCurLen;
    nCurLen = (int)SendMessage(m_hLogWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtlex, 0);

    // 将文本追加到末尾
    CHARRANGE cr;
    cr.cpMin = nCurLen;
    cr.cpMax = -1;
    SendMessage(m_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessage(m_hLogWnd, EM_REPLACESEL, (WPARAM)&cr, (LPARAM)lpszMsg);

    // 设置刚追加到末尾的文本的颜色与字符集
    SendMessage(m_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT cf;
    cf.cbSize = sizeof(CHARFORMAT);
    SendMessage(m_hLogWnd, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

    cf.dwMask = CFM_COLOR | CFM_CHARSET;
    cf.dwEffects &= ~CFE_AUTOCOLOR;
    cf.crTextColor = clrText;
    cf.bCharSet = GB2312_CHARSET;
    SendMessage(m_hLogWnd, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

    // 加一个换行符
    cr.cpMin = nCurLen + _tcslen(lpszMsg);
    cr.cpMax = -1;
    SendMessage(m_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessage(m_hLogWnd, EM_REPLACESEL, (WPARAM)&cr, (LPARAM)_T("\n"));

    // 取消选择任何文本
    cr.cpMax = cr.cpMin = -1;
    SendMessage(m_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    // 将滚动条移到最底端
    SendMessage(m_hLogWnd, WM_VSCROLL, (WPARAM)SB_BOTTOM, 0);
}

void GameMainPage::WriteLog(CHARFORMAT* charFmt, LPCTSTR lpszMsg)
{
    UNREFERENCED_PARAMETER(charFmt);
    UNREFERENCED_PARAMETER(lpszMsg);
}

// 清空日志
void GameMainPage::ClearLog(void)
{
    SendMessage(m_hLogWnd, WM_SETTEXT, 0, (LPARAM)_T(""));
}

// 响应ListView, RichEdit子窗口的通知事件
void GameMainPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    NMHDR* nmhdr = (NMHDR*)lParam;

    switch (nmhdr->idFrom) {
        case GMP_LISTVIEW_CTRL_ID:
            OnListviewNotify(wParam, lParam);
            break;

        case GMP_RICHEDIT_CTRL_ID:
            OnRicheditNotify(wParam, lParam);
            break;
    }
}

// 子窗口控件(COMBO)的通知事件或菜单消息
void GameMainPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCtrl = LOWORD(wParam);
    UINT nEvent = HIWORD(wParam);
    HWND hWnd = (HWND)lParam;

    switch (idCtrl) {
        case GMP_COMBO_CHATMSG_ID:
            OnComboNotify(hWnd, nEvent);
            break;

        case IDM_LOGWND_CLEAR:
            ClearLog();
            break;

        case IDM_LOGWND_COPY:
            OnRicheditCopy();
            break;
    }
}

// 处理列表控件的通知事件
void GameMainPage::OnListviewNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* nmhdr = (NMHDR*)lParam;
    NMLVDISPINFO* lvdi = (NMLVDISPINFO*)lParam;

    int nUserId = lvdi->item.lParam;

    switch (nmhdr->code) {
        case LVN_GETDISPINFO:
            switch (lvdi->item.iSubItem) {
                case 0:
                    lvdi->item.mask = LVIF_IMAGE;
                    lvdi->item.iImage = GetUserImageIndex(nUserId);
                    break;

                case 1:
                    lvdi->item.mask = LVIF_TEXT;
                    lvdi->item.pszText = (LPTSTR)GetUserNameStr(nUserId);
                    break;

                case 2:
                    lvdi->item.mask = LVIF_TEXT;
                    lvdi->item.pszText = (LPTSTR)GetUserLevelStr(nUserId);
                    break;

                case 3:
                    lvdi->item.mask = LVIF_TEXT;
                    _stprintf_s(lvdi->item.pszText, lvdi->item.cchTextMax, _T("%d"), GetUserScore(nUserId));
                    break;
            }
            break;
    }
}

// 处理RichEdit控件的通知事件
void GameMainPage::OnRicheditNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* nmhdr = (NMHDR*)lParam;
    MSGFILTER* msgFilter = NULL;
    ENLINK* enLink = NULL;

    switch (nmhdr->code) {
        case EN_MSGFILTER:
            msgFilter = (MSGFILTER*)lParam;
            switch (msgFilter->msg) {
                case WM_RBUTTONDOWN:
                    OnRicheditNotifyRButtonDown(msgFilter->wParam, msgFilter->lParam);
                    break;

                case WM_KEYDOWN:
                    OnRicheditNotifyKeyUp(msgFilter->wParam, msgFilter->lParam);
                    break;
            }
            break;

        case EN_LINK:
            enLink = (ENLINK*)lParam;
            switch (enLink->msg) {
                case WM_LBUTTONDOWN:
                    UNREFERENCED_PARAMETER(enLink);
                    break;
            }
            break;
    }
}

// 在RichEdit控件（LogWnd）中点击鼠标右键
void GameMainPage::OnRicheditNotifyRButtonDown(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    // Popup a menu
    HMENU hMenu = NULL;
    HMENU hMenuPopup = NULL;

    hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_LOGWND_POPUP));
    if (hMenu != NULL) {
        hMenuPopup = GetSubMenu(hMenu, 0);
        if (hMenuPopup != NULL) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(m_hLogWnd, &pt);

            CHARRANGE cr = { 0 };
            SendMessage(m_hLogWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

            //
            // If no text selection, gray COPY item. CLEAR item will always be enabled.
            //
            if (cr.cpMin == cr.cpMax) {
                EnableMenuItem(hMenuPopup, IDM_LOGWND_COPY, MF_BYCOMMAND | MF_GRAYED);
            }

            TrackPopupMenuEx(hMenuPopup, TPM_LEFTALIGN | TPM_LEFTBUTTON,
                pt.x, pt.y, m_hWnd, NULL);
        }

        DestroyMenu(hMenuPopup);
    }
}

void GameMainPage::OnRicheditNotifyKeyUp(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    UINT nVKey = (UINT)wParam;
    switch (nVKey) {
        case VK_DELETE:
            // clear all contents 
            ClearLog();
            break;
    }
}

// 处理COMBO控件的通知事件
void GameMainPage::OnComboNotify(HWND hCombo, UINT nEvent)
{
    int nCurSel = CB_ERR;

    switch (nEvent) {
        case CBN_SELCHANGE:
            nCurSel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);

            if (nCurSel != CB_ERR) {
                PLAYER_STATE state = GetLocalUserState();

                if (state == STATE_LOOKON) {
                    WriteLog(LOG_WARN, _T("你是旁观者，不能说话。"));
                } else {
                    int nCurGamerSeat = GetLocalUserSeatId();

                    if (!IS_INVALID_SEAT(nCurGamerSeat)) {
                        SendChatMessage(nCurGamerSeat, nCurSel);
                    }
                }

                SendMessage(hCombo, CB_SETCURSEL, (WPARAM)-1, 0);
            }
            break;
    }
}

// 自定义的COMBOBOX-EDIT控件窗口处理函数，用于截取必要消息。
LRESULT CALLBACK GameMainPage::MyComboEditProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    UINT nVKey;
    GameMainPage* lpWnd = (GameMainPage*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    assert(lpWnd != NULL);

    if (hWnd != lpWnd->m_hComboEdit) { // Other Combobox, not the Combobox for CHATMSG
        return CallWindowProc(lpWnd->m_lpComboEditWndProc, hWnd, nMsg, wParam, lParam);
    }

    switch (nMsg) {
        case WM_KEYDOWN:
            nVKey = (UINT)wParam;
            switch (nVKey) {
                case VK_ESCAPE:
                case VK_RETURN:
                    SendMessage(lpWnd->m_hWnd, WM_CHATMSG_VKEY, wParam, lParam);
                    return 0;

                case VK_PRIOR:
                case VK_NEXT:
                case VK_END:
                case VK_HOME:
                case VK_UP:
                case VK_DOWN:
                    return 0; // dismiss these key messages
            }
            break;

        case WM_CHAR:
        case WM_KEYUP:
            nVKey = (UINT)wParam;
            switch (nVKey) {
                case VK_PRIOR:
                case VK_NEXT:
                case VK_END:
                case VK_HOME:
                case VK_UP:
                case VK_DOWN:

                case VK_ESCAPE:
                case VK_RETURN:
                    return 0; // dismiss these key messages
            }
            break;

        case WM_MOUSEWHEEL: // dismiss the mouse wheel message
            return 0;
    }

    return CallWindowProc(lpWnd->m_lpComboEditWndProc, hWnd, nMsg, wParam, lParam);
}

// 处理ComboBox-Edit控件的按键消息
void GameMainPage::OnChatMsgKeyDown(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    UINT nVKey = (UINT)wParam;
    TCHAR szMsg[128] = { 0 };

    switch (nVKey) {
        case VK_ESCAPE:
            SetWindowText(m_hChatMsgCombo, _T(""));
            break;

        case VK_RETURN:
            GetWindowText(m_hChatMsgCombo, szMsg, sizeof(szMsg) / sizeof(szMsg[0]));

            if ((_tcslen(szMsg) > 0) && !IsBlankString(szMsg)) {
                PLAYER_STATE state = GetLocalUserState();
                if (state == STATE_LOOKON) {
                    WriteLog(LOG_WARN, _T("你是旁观者，不能说话。"));
                } else {
                    int nCurGamerSeat = GetLocalUserSeatId();
                    if (!IS_INVALID_SEAT(nCurGamerSeat)) {
                        SendChatMessage(nCurGamerSeat, szMsg);
                    }
                }
            }

            SetWindowText(m_hChatMsgCombo, _T(""));
            break;
    }
}

// 判断字符串是否全部为空白字符
BOOL GameMainPage::IsBlankString(LPCTSTR lpszText)
{
    assert(lpszText != NULL);

    TCHAR* pChar = (TCHAR*)lpszText;
    while (*pChar != _T('\0')) {
        switch (*pChar) {
            case _T(' '):
            case _T('\t'):
            case _T('\n'):
            case _T('\r'):
                break;

            default:
                return FALSE;
        }
        pChar++;
    }
    return TRUE;
}

// 响应LOGWND 菜单项清空
void GameMainPage::OnRicheditCopy(void)
{
    SendMessage(m_hLogWnd, WM_COPY, 0, 0);
}

// 进入游戏界面时，初始化游戏环境
void GameMainPage::Init(BOOL bIsLookon)
{
    m_GameMainWnd.Init(bIsLookon);
    ListCtrlClear();
    ClearLog();

    WriteLog(RGB(0,255,0), _T("开心游戏! 快乐每一天!\n"));
}

// 进入游戏界面之前，需要先设置当前玩家的椅子号，以此才能识别上家、下家所在的椅子号
void GameMainPage::SetCurentUserSeat(int seat)
{
    assert(!IS_INVALID_SEAT(seat));
    m_GameMainWnd.SetCurrentUserSeat(seat);
}

// 玩家进入游戏桌。本玩家已经进入游戏界面之后，收到别的玩家进入时，调用此函数。
void GameMainPage::GamerTakeSeat(int id, int seat)
{
    // 1.
    ListCtrlAddUser(id);

    // 2.
    TCHAR szMsg[48] = { 0 };
    LPCTSTR lpszName = GetUserNameStr(id);

    TCHAR szSeat[32] = { 0 };
    _stprintf_s(szSeat, sizeof(szSeat) / sizeof(szSeat[0]), _T(" 进入游戏桌，坐 %d 椅子。"), seat);

    _tcscpy_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), lpszName);
    _tcscat_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), szSeat);

    WriteLog(GMP_LOGCLR_INOUT, szMsg);

    // 3.
    m_GameMainWnd.GamerTakeSeat(id, seat);
}

// 玩家离开游戏桌。本玩家已经进入游戏界面之后，收到别的玩家离开时，调用此函数。
void GameMainPage::GamerLeaveSeat(int id, int seat)
{
    // 1.
    ListCtrlRemoveUser(id);

    // 2.
    TCHAR szMsg[48] = { 0 };
    LPCTSTR lpszName = GetUserNameStr(id);

    TCHAR szSeat[32] = { 0 };
    _stprintf_s(szSeat, sizeof(szSeat) / sizeof(szSeat[0]), _T(" 离开游戏桌。"), seat);

    _tcscpy_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), lpszName);
    _tcscat_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), szSeat);

    WriteLog(GMP_LOGCLR_INOUT, szMsg);

    // 3.
    m_GameMainWnd.GamerLeaveSeat(id, seat);
}

// 旁观者进入游戏桌
void GameMainPage::LookonTakeSeat(int id, int seat)
{
    // 1.
    ListCtrlAddUser(id);

    // 2.
    TCHAR szMsg[48] = { 0 };
    LPCTSTR lpszName = GetUserNameStr(id);

    TCHAR szSeat[32] = { 0 };
    _stprintf_s(szSeat, sizeof(szSeat) / sizeof(szSeat[0]), _T(" 进入游戏桌，旁观 %d 椅子。"), seat);

    _tcscpy_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), lpszName);
    _tcscat_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), szSeat);

    WriteLog(GMP_LOGCLR_INOUT, szMsg);
}

// 旁观者离开
void GameMainPage::LookonLeaveSeat(int id, int seat)
{
    // 1.
    ListCtrlRemoveUser(id);

    // 2.
    TCHAR szMsg[48] = { 0 };
    LPCTSTR lpszName = GetUserNameStr(id);

    TCHAR szSeat[32] = { 0 };
    _stprintf_s(szSeat, sizeof(szSeat) / sizeof(szSeat[0]), _T(" 离开游戏桌。"), seat);

    _tcscpy_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), lpszName);
    _tcscat_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), szSeat);

    WriteLog(GMP_LOGCLR_INOUT, szMsg);
}

// 玩家准备就绪
void GameMainPage::GamerReady(int id, int seat)
{
    m_GameMainWnd.GamerReady(id, seat);
}

// 响应子窗口 GameMainWnd 发送过来的GAMEOVER消息
void GameMainPage::OnGameOver(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    InvalidateRect(m_hUserList, NULL, FALSE);
}