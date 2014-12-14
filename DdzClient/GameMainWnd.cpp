//
// File: GameMainWnd.cpp
//
//  斗地主游戏主界面窗口
//
#include "stdafx.h"
#include "DdzClient.h"
#include "NetProcess.h"
#include "MyButton.h"
#include "CurPokerWnd.h"
#include "OutCardWnd.h"
#include "UnderCardWnd.h"
#include "GamerVisualWnd.h"
#include "GameMainWnd.h"


extern HWND g_hMainWnd;
extern TCHAR* g_lpszChatMessage[];

GameMainWnd::GameMainWnd(void)
{
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hInstance = NULL;

    m_bShow = TRUE;

    m_hBackBitmap = NULL;
    m_cxBackBitmap = 0;
    m_cyBackBitmap = 0;

    m_nCurUserSeat = INVALID_SEAT;

    m_bGameStarted = FALSE;
    m_bOutputCardStarted = FALSE;

    m_bNoGamerVoteLord = FALSE;
    m_nNoGamerVoteLordTimes = 0;

    m_nBombNum = 0;
    m_nHintTimes = 0;
    ZeroMemory(&m_pct, sizeof(POKER_CLASS_TABLE));

    m_bReqOutputCard = FALSE;
    ZeroMemory(&m_PokerProp, sizeof(POKER_PROPERTY));

    m_nLordSeat = INVALID_SEAT;
    m_nLordScore = 0;
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        m_bGamerConnLost[seat] = FALSE;
        m_bGamerDelegated[seat] = FALSE;
    }

    m_bGamerReadyUp = FALSE;
    m_bGamerReadyDown = FALSE;

    m_nTimeOutCur = 0;
    m_nTimeOutUp = 0;
    m_nTimeOutDown = 0;

    m_bShowClockCur = FALSE;
    m_bShowClockUp = FALSE;
    m_bShowClockDown = FALSE;
    m_nTimeOutTimes = 0;

    SetRect(&m_rcLordUp, 0, 0, 0, 0);
    SetRect(&m_rcLordCur, 0, 0, 0, 0);
    SetRect(&m_rcLordDown, 0, 0, 0, 0);
    SetRect(&m_rcConnLostUp, 0, 0, 0, 0);
    SetRect(&m_rcConnLostCur, 0, 0, 0, 0);
    SetRect(&m_rcConnLostDown, 0, 0, 0, 0);
    SetRect(&m_rcDelegatedUp, 0, 0, 0, 0);
    SetRect(&m_rcDelegatedCur, 0, 0, 0, 0);
    SetRect(&m_rcDelegatedDown, 0, 0, 0, 0);
    SetRect(&m_rcClockUp, 0, 0, 0, 0);
    SetRect(&m_rcClockCur, 0, 0, 0, 0);
    SetRect(&m_rcClockDown, 0, 0, 0, 0);
}

GameMainWnd::~GameMainWnd(void)
{

}

ATOM GameMainWnd::GameMainWndRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpszClassName  = GAME_MAIN_WND_CLASS_NAME;
    wcex.lpfnWndProc    = GameMainWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;//(HBRUSH)(COLOR_WINDOW + 1);

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK GameMainWnd::GameMainWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    UINT nCtrlId;
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    GameMainWnd* lpWnd = (GameMainWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (nMsg) {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
            break;

        case WM_PAINT:
            hdc =BeginPaint(hWnd, &ps);
            lpWnd->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            break;

        case WM_SIZE:
            lpWnd->OnSize(wParam, lParam);
            break;

        case WM_LBUTTONDBLCLK:
            lpWnd->OnLButtonDblClk(wParam, lParam);
            break;

        case WM_RBUTTONDOWN:
            lpWnd->OnRButtonDown(wParam, lParam);
            break;

        case WM_COMMAND:
            nCtrlId = LOWORD(wParam);
            lpWnd->OnCommand(nCtrlId);
            break;

        case WM_TIMER:
            lpWnd->OnTimer(wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }

    return 0;
}

HWND GameMainWnd::Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance)
{
    m_hWndParent = hWndParent;
    m_hInstance = hInstance;

    m_hWnd = CreateWindowEx(0,
        GAME_MAIN_WND_CLASS_NAME,
        _T(""),
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        hWndParent,
        NULL,
        hInstance,
        this);
    assert(m_hWnd != NULL);

    CreateChildWindows();

    SendMessage(m_hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE);
    return m_hWnd;
}

void GameMainWnd::CreateChildWindows(void)
{
    HMODULE hModule = LoadLibrary(RES_POKER_DLL_NAME);
    assert(hModule != NULL);

    m_btnStart.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_START, m_hInstance);
    m_btnStart.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_START), GMW_DEF_BTN_MASK_CLR);

    m_btnScore1.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_SCORE1, m_hInstance);
    m_btnScore2.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_SCORE2, m_hInstance);
    m_btnScore3.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_SCORE3, m_hInstance);
    m_btnScore0.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_SCORE0, m_hInstance);

    m_btnScore0.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_SCORE0), GMW_DEF_BTN_MASK_CLR);
    m_btnScore1.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_SCORE1), GMW_DEF_BTN_MASK_CLR);
    m_btnScore2.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_SCORE2), GMW_DEF_BTN_MASK_CLR);
    m_btnScore3.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_SCORE3), GMW_DEF_BTN_MASK_CLR);

    m_btnPass.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_PASS, m_hInstance);
    m_btnFollow.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_FOLLOW, m_hInstance);
    m_btnHint.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_HINT, m_hInstance);

    m_btnPass.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_PASS), GMW_DEF_BTN_MASK_CLR);
    m_btnFollow.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_FOLLOW), GMW_DEF_BTN_MASK_CLR);
    m_btnHint.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_HINT), GMW_DEF_BTN_MASK_CLR);

    m_btnOption.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_OPTION, m_hInstance);
    m_btnDelegate.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_DELEGATE, m_hInstance);
    m_btnPrevRound.Create(WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, ID_BTN_PREVROUND, m_hInstance);

    m_btnOption.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_TEMPLATE), GMW_DEF_BTN_MASK_CLR);
    m_btnDelegate.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_TEMPLATE), GMW_DEF_BTN_MASK_CLR);
    m_btnPrevRound.SetBitmap(hModule, MAKEINTRESOURCE(IDB_BTN_TEMPLATE), GMW_DEF_BTN_MASK_CLR);

    m_btnOption.SetText(_T("设置"));
    m_btnDelegate.SetText(_T("托管"));
    m_btnPrevRound.SetText(_T("上一轮"));

    m_UnderCardWnd.Create(WS_CHILD|WS_VISIBLE, 0, 0, m_hWnd, m_hInstance, g_hbmpPoker);

    m_CurPokerWndDown.Create(WS_CHILD|WS_VISIBLE, 0, 0, m_hWnd, ID_CUR_POKER_WND_DOWN,
        m_hInstance, g_hbmpPoker, FALSE, FALSE);
    m_CurPokerWndUp.Create(WS_CHILD|WS_VISIBLE, 0, 0, m_hWnd, ID_CUR_POKER_WND_UP,
        m_hInstance, g_hbmpPoker, FALSE, FALSE);
    m_CurPokerWnd.Create(WS_CHILD|WS_VISIBLE, 0, 0, m_hWnd, ID_CUR_POKER_WND,
        m_hInstance,g_hbmpPoker, FALSE, TRUE);

    m_OutCardWndDown.Create(WS_CHILD|WS_VISIBLE, 0, 0, CARD_ALIGN_RIGHT, m_hWnd, m_hInstance, g_hbmpPoker);
    m_OutCardWndUp.Create(WS_CHILD|WS_VISIBLE, 0, 0, CARD_ALIGN_LEFT, m_hWnd, m_hInstance, g_hbmpPoker);
    m_OutCardWndCur.Create(WS_CHILD|WS_VISIBLE, 0, 0, CARD_ALIGN_CENTER, m_hWnd, m_hInstance, g_hbmpPoker);

    m_GamerVisualWndUp.Create(0, 0, 0, 0, m_hWnd, m_hInstance);
    m_GamerVisualWndCur.Create(0, 0, 0, 0, m_hWnd, m_hInstance);
    m_GamerVisualWndDown.Create(0, 0, 0, 0, m_hWnd, m_hInstance);

    ShowVoteLordButtons(0, FALSE);
    ShowOutCardButtons(FALSE, FALSE);

    FreeLibrary(hModule);
}

void GameMainWnd::OnPaint(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);

    //
    // 根据窗口客户区尺寸与背景位图尺寸，从背景位图居中选择一个合适区域，
    // 拉伸的方式将所选区域的位图绘至窗口客户区。
    //
    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;

    int xBmpStart = 0;
    int yBmpStart = 0;

    int cxBmp = m_cxBackBitmap;
    int cyBmp = m_cyBackBitmap;

    if (cxBmp > cx) {
        xBmpStart += (cxBmp - cx) / 2;
        cxBmp = cx;
    }

    if (cyBmp > rect.bottom) {
        yBmpStart += (cyBmp - cy) / 2;
        cyBmp = cy;
    }

    if (cy <= 0) { return; } // avoid dividing by ZERO

    double ratio = 1.0; // 窗口客户区长宽比率
    ratio = (double)(cx) / (double)(cy);

    if (ratio >= 1.0) {
        int cyNewBmp = (int)(cxBmp / ratio);
        yBmpStart += (cyBmp - cyNewBmp) / 2;
        cyBmp = cyNewBmp;
    } else {
        int cxNewBmp = (int)(cyBmp * ratio);
        xBmpStart += (cxBmp - cxNewBmp) / 2;
        cxBmp = cxNewBmp;
    }

    if (xBmpStart < 0) { xBmpStart = 0; }
    if (yBmpStart < 0) { yBmpStart = 0; }
    if (cxBmp > m_cxBackBitmap) { cxBmp = m_cxBackBitmap; }
    if (cyBmp > m_cyBackBitmap) { cyBmp = m_cyBackBitmap; }

    HDC tempDC = CreateCompatibleDC(hdc);
    assert(tempDC != NULL);

    // background image
    HBITMAP hOldBmp = (HBITMAP)SelectObject(tempDC, m_hBackBitmap);

    HDC memDC = CreateCompatibleDC(hdc);
    assert(memDC != NULL);

    HBITMAP memBMP = CreateCompatibleBitmap(hdc, cx, cy);
    assert(memBMP != NULL);

    HBITMAP oldmemBMP = (HBITMAP)SelectObject(memDC, memBMP);

    // 绘制背景图
    StretchBlt(memDC, 0, 0, cx, cy, tempDC, xBmpStart, yBmpStart, cxBmp, cyBmp, SRCCOPY);

#ifdef PARENT_PAINT_CHILD
    // 先绘制所有按钮
    m_btnScore1.ParentPaintChild(memDC);
    m_btnScore2.ParentPaintChild(memDC);
    m_btnScore3.ParentPaintChild(memDC);
    m_btnScore0.ParentPaintChild(memDC);
    m_btnPass.ParentPaintChild(memDC);
    m_btnFollow.ParentPaintChild(memDC);
    m_btnHint.ParentPaintChild(memDC);
    m_btnOption.ParentPaintChild(memDC);
    m_btnDelegate.ParentPaintChild(memDC);
    m_btnPrevRound.ParentPaintChild(memDC);
    m_btnStart.ParentPaintChild(memDC);

    // 绘制底牌窗口
    m_UnderCardWnd.ParentPaintChild(memDC);

    // 绘制玩家当前牌窗口
    m_CurPokerWndDown.ParentPaintChild(memDC);
    m_CurPokerWndUp.ParentPaintChild(memDC);
    m_CurPokerWnd.ParentPaintChild(memDC);

    // 绘制玩家出牌窗口
    m_OutCardWndDown.ParentPaintChild(memDC);
    m_OutCardWndUp.ParentPaintChild(memDC);
    m_OutCardWndCur.ParentPaintChild(memDC);

    // 绘制玩家形象窗口
    m_GamerVisualWndDown.ParentPaintChild(memDC);
    m_GamerVisualWndUp.ParentPaintChild(memDC);
    m_GamerVisualWndCur.ParentPaintChild(memDC);
#endif

    // 地主头像
    DrawIconLord(memDC);

    // 掉线图标
    DrawIconConnLost(memDC);

    // 托管游戏图标
    DrawIconDelegated(memDC);

    // 输出底分
    DrawLordScoreString(memDC);

    // 炸弹图标
    DrawIconBomb(memDC);

    // 绘制时钟
    DrawClock(memDC);

    // 最后将绘制好的位图刷到屏幕
    BitBlt(hdc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldmemBMP);
    SelectObject(tempDC, hOldBmp);

    DeleteObject(memBMP);

    DeleteDC(memDC);
    DeleteDC(tempDC);
}

void GameMainWnd::OnSize(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    // 调整所有子窗口的尺寸与位置
    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);

    if ((cx <= 0) || (cy <= 0)) { return; }

    //
    // 通知子窗口重新保存父窗口背景
    //
    m_CurPokerWndDown.ParentWndSizeChanged();
    m_CurPokerWndUp.ParentWndSizeChanged();
    m_CurPokerWnd.ParentWndSizeChanged();

    m_OutCardWndDown.ParentWndSizeChanged();
    m_OutCardWndUp.ParentWndSizeChanged();
    m_OutCardWndCur.ParentWndSizeChanged();

    m_GamerVisualWndUp.ParentWndSizeChanged();
    m_GamerVisualWndCur.ParentWndSizeChanged();
    m_GamerVisualWndDown.ParentWndSizeChanged();

    m_UnderCardWnd.ParentWndSizeChanged();

    //
    // 调整子窗口尺寸与位置
    //
    int xCenter = cx / 2;

    // Max width of CurPokerWnd, same as OutCardWnd;
    int xMaxCPW = (PLAYER_MAX_CARDS_NUM - 1) * CPW_POKER_H_SPACE + POKER_BMP_UNIT_WIDTH;
    int xMaxOCW = xMaxCPW;

    //
    // UnderCardWnd
    //
    int xCenterUCW = xCenter;
    int yCenterUCW = 10 + POKER_BMP_UNIT_HEIGHT / 2;
    m_UnderCardWnd.SetWindowCenterPos(xCenterUCW, yCenterUCW);
    
    //
    // CurPokerWnd
    //
    int xCenterCPWDown = cx - 10 - GVW_USER_VISUAL_WND_WIDTH - 10 - POKER_BMP_UNIT_WIDTH / 2;
    int yCenterCPWDown = cy * 3 / 8; // 上、下家的当前牌窗口Y中心点为主窗口矩形高度的 3/8;
    m_CurPokerWndDown.SetWindowCenterPos(xCenterCPWDown, yCenterCPWDown);

    int xCenterCPWUp = 10 + GVW_USER_VISUAL_WND_WIDTH + 10 + POKER_BMP_UNIT_WIDTH / 2;
    int yCenterCPWUp = yCenterCPWDown;
    m_CurPokerWndUp.SetWindowCenterPos(xCenterCPWUp, yCenterCPWUp);

    int xCenterCPWCur = xCenter;
    int yCenterCPWCur = cy - 40 - (POKER_BMP_UNIT_HEIGHT + CPW_POKER_POPUP_SPACE) / 2;
    m_CurPokerWnd.SetWindowCenterPos(xCenterCPWCur, yCenterCPWCur);
    
    //
    // OutCardWnd
    //
    int xCenterOCWDown = cx - 10 - GVW_USER_VISUAL_WND_WIDTH - 10 - POKER_BMP_UNIT_WIDTH - 10 - xMaxOCW / 2;
    int yCenterOCWDown = yCenterCPWDown;
    m_OutCardWndDown.SetWindowCenterPos(xCenterOCWDown, yCenterOCWDown);

    int xCenterOCWUp = 10 + GVW_USER_VISUAL_WND_WIDTH + 10 + POKER_BMP_UNIT_WIDTH + 10 + xMaxOCW / 2;
    int yCenterOCWUp = yCenterOCWDown;
    m_OutCardWndUp.SetWindowCenterPos(xCenterOCWUp, yCenterOCWUp);

    int xCenterOCWCur = xCenter;
    int yCenterOCWCur = cy - 40 - POKER_BMP_UNIT_HEIGHT - 20 - GMW_BTN_HEIGHT - 10 - POKER_BMP_UNIT_HEIGHT / 2;
    m_OutCardWndCur.SetWindowCenterPos(xCenterOCWCur, yCenterOCWCur);

    //
    // GamerVisualWnd
    //
    int xGVWDown = cx - 10 - GVW_USER_VISUAL_WND_WIDTH;
    int yGVWDown = yCenterCPWDown - GVW_USER_VISUAL_WND_HEIGHT / 2;
    m_GamerVisualWndDown.SetWindowRect(xGVWDown, yGVWDown, GVW_USER_VISUAL_WND_WIDTH, GVW_USER_VISUAL_WND_HEIGHT);

    int xGVWUp = 10;
    int yGVWUp = yGVWDown;
    m_GamerVisualWndUp.SetWindowRect(xGVWUp, yGVWUp, GVW_USER_VISUAL_WND_WIDTH, GVW_USER_VISUAL_WND_HEIGHT);

    int xGVWCur = (cx - xMaxCPW) / 2 - GVW_USER_VISUAL_WND_WIDTH - 20;
    int yGVWCur = cy - 10 - GVW_USER_VISUAL_WND_HEIGHT;
    m_GamerVisualWndCur.SetWindowRect(xGVWCur, yGVWCur, GVW_USER_VISUAL_WND_WIDTH, GVW_USER_VISUAL_WND_HEIGHT);

    //
    // Vote Lord Buttons
    //
    int xBtnStart = (cx - (GMW_BTN_WIDTH * 4 + GMW_BTN_H_GAP * 3)) / 2;
    int yBtnStart = cy - 40 - POKER_BMP_UNIT_HEIGHT - 20 - GMW_BTN_HEIGHT;

    m_btnScore1.SetWindowRect(xBtnStart + 0 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);
    m_btnScore2.SetWindowRect(xBtnStart + 1 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);
    m_btnScore3.SetWindowRect(xBtnStart + 2 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);
    m_btnScore0.SetWindowRect(xBtnStart + 3 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);

    //
    // Out Card Buttons
    //
    xBtnStart = (cx - (GMW_BTN_WIDTH * 3 + GMW_BTN_H_GAP * 2)) / 2;

    m_btnPass.SetWindowRect(xBtnStart + 0 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);
    m_btnFollow.SetWindowRect(xBtnStart + 1 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);
    m_btnHint.SetWindowRect(xBtnStart + 2 * (GMW_BTN_WIDTH + GMW_BTN_H_GAP), yBtnStart, GMW_BTN_WIDTH, GMW_BTN_HEIGHT);

    //
    // Other Buttons
    //
    xBtnStart = cx - GMW_START_BTN_WIDTH - 4;
    yBtnStart = cy - (GMW_START_BTN_HEIGHT * 3 + GMW_BTN_V_GAP * 2) - 4;

    m_btnOption.SetWindowRect(xBtnStart, yBtnStart + 0 * (GMW_START_BTN_HEIGHT + GMW_BTN_V_GAP), GMW_START_BTN_WIDTH, GMW_START_BTN_HEIGHT);
    m_btnDelegate.SetWindowRect(xBtnStart, yBtnStart + 1 * (GMW_START_BTN_HEIGHT + GMW_BTN_V_GAP), GMW_START_BTN_WIDTH, GMW_START_BTN_HEIGHT);
    m_btnPrevRound.SetWindowRect(xBtnStart, yBtnStart + 2 * (GMW_START_BTN_HEIGHT + GMW_BTN_V_GAP), GMW_START_BTN_WIDTH, GMW_START_BTN_HEIGHT);

    m_btnStart.SetWindowRect(cx - GMW_START_BTN_WIDTH * 2 - 40, yBtnStart, GMW_START_BTN_WIDTH, GMW_START_BTN_HEIGHT);

    //
    // 设置地主头像、掉线、托管图标显示的矩形。设置其尺寸时，需参照如下所示窗口的位置。
    //
    // 地主头像图标：UP（玩家形象图下面），CUR（当前扑克牌右边），DOWN（玩家形象图下面）
    // 托管图标：UP（玩家形象图上面），CUR（当前扑克牌右边），DOWN（玩家形象图上面）
    // 掉线图标：在各玩家当前牌窗口中央。（若开启宏PARETN_PAINT_CHILD，该图标将被当前牌窗口遮盖住）
    //
    m_rcLordUp.left     = 10 + GVW_USER_VISUAL_WND_WIDTH / 2 - CX_ICON_LORD / 2;
    m_rcLordUp.top      = 20 + yCenterCPWUp + GVW_USER_VISUAL_WND_HEIGHT / 2;
    m_rcLordUp.right    = m_rcLordUp.left + CX_ICON_LORD;
    m_rcLordUp.bottom   = m_rcLordUp.top + CY_ICON_LORD;

    m_rcLordCur.left    = 20 + (cx + xMaxCPW) / 2;
    m_rcLordCur.top     = cy - 40 - CY_ICON_LORD;
    m_rcLordCur.right   = m_rcLordCur.left + CX_ICON_LORD;
    m_rcLordCur.bottom  = m_rcLordCur.top + CY_ICON_LORD;

    m_rcLordDown.left   = cx - 10 - GVW_USER_VISUAL_WND_WIDTH / 2 - CX_ICON_LORD / 2;
    m_rcLordDown.top    = 20 + yCenterCPWDown + GVW_USER_VISUAL_WND_HEIGHT / 2;
    m_rcLordDown.right  = m_rcLordDown.left + CX_ICON_LORD;
    m_rcLordDown.bottom = m_rcLordDown.top + CY_ICON_LORD;

    m_rcDelegatedUp.left    = 10 + GVW_USER_VISUAL_WND_WIDTH / 2 - CX_ICON_DELEGATE / 2;
    m_rcDelegatedUp.top     = yCenterCPWUp - GVW_USER_VISUAL_WND_HEIGHT / 2 - CY_ICON_DELEGATE - 20;
    m_rcDelegatedUp.right   = m_rcDelegatedUp.left + CX_ICON_DELEGATE;
    m_rcDelegatedUp.bottom  = m_rcDelegatedUp.top + CY_ICON_DELEGATE;

    m_rcDelegatedCur.left   = m_rcLordCur.left + (CX_ICON_LORD - CX_ICON_DELEGATE) / 2;
    m_rcDelegatedCur.top    = cy - 40 - POKER_BMP_UNIT_HEIGHT;
    m_rcDelegatedCur.right  = m_rcDelegatedCur.left + CX_ICON_DELEGATE;
    m_rcDelegatedCur.bottom = m_rcDelegatedCur.top + CX_ICON_DELEGATE;

    m_rcDelegatedDown.left  = cx - 10 - GVW_USER_VISUAL_WND_WIDTH / 2 - CX_ICON_DELEGATE / 2;
    m_rcDelegatedDown.top   = yCenterCPWDown - GVW_USER_VISUAL_WND_HEIGHT / 2 - CY_ICON_DELEGATE - 20;
    m_rcDelegatedDown.right = m_rcDelegatedDown.left + CX_ICON_DELEGATE;
    m_rcDelegatedDown.bottom= m_rcDelegatedDown.top + CY_ICON_DELEGATE;

    m_rcConnLostUp.left     = xCenterCPWUp - CX_ICON_CONNLOST / 2;
    m_rcConnLostUp.top      = yCenterCPWUp - CY_ICON_CONNLOST / 2;
    m_rcConnLostUp.right    = m_rcConnLostUp.left + CX_ICON_CONNLOST;
    m_rcConnLostUp.bottom   = m_rcConnLostUp.top + CY_ICON_CONNLOST;

    m_rcConnLostCur.left    = xCenterCPWCur - CX_ICON_CONNLOST / 2;
    m_rcConnLostCur.top     = yCenterCPWCur - CY_ICON_CONNLOST / 2;
    m_rcConnLostCur.right   = m_rcConnLostCur.left + CX_ICON_CONNLOST;
    m_rcConnLostCur.bottom  = m_rcConnLostCur.top + CY_ICON_CONNLOST;

    m_rcConnLostDown.left   = xCenterCPWDown - CX_ICON_CONNLOST / 2;
    m_rcConnLostDown.top    = yCenterCPWDown - CY_ICON_CONNLOST / 2;
    m_rcConnLostDown.right  = m_rcConnLostDown.left + CX_ICON_CONNLOST;
    m_rcConnLostDown.bottom = m_rcConnLostDown.top + CY_ICON_CONNLOST;

    m_rcClockUp.left        = xCenterCPWUp + POKER_BMP_UNIT_WIDTH / 2 + 10;
    m_rcClockUp.top         = yCenterCPWUp - POKER_BMP_UNIT_HEIGHT / 2 - CY_ICON_CLOCK - 10;
    m_rcClockUp.right       = m_rcClockUp.left + CX_ICON_CLOCK;
    m_rcClockUp.bottom      = m_rcClockUp.top + CY_ICON_CLOCK;

    m_rcClockCur.left       = xCenter - CX_ICON_CLOCK / 2;
    m_rcClockCur.top        = yCenterOCWCur - POKER_BMP_UNIT_HEIGHT / 2 - CY_ICON_CLOCK;
    m_rcClockCur.right      = m_rcClockCur.left + CX_ICON_CLOCK;
    m_rcClockCur.bottom     = m_rcClockCur.top + CY_ICON_CLOCK;

    m_rcClockDown.left      = xCenterCPWDown - POKER_BMP_UNIT_WIDTH / 2 - CX_ICON_CLOCK - 10;
    m_rcClockDown.top       = yCenterOCWDown - POKER_BMP_UNIT_HEIGHT / 2 - CY_ICON_CLOCK - 10;
    m_rcClockDown.right     = m_rcClockDown.left + CX_ICON_CLOCK;
    m_rcClockDown.bottom    = m_rcClockDown.top + CY_ICON_CLOCK;
}

void GameMainWnd::Show(BOOL bShow /*= TRUE*/)
{
    if (m_bShow != bShow) {
        m_bShow = bShow;
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
    }
}

BOOL GameMainWnd::IsVisible(void)
{
    return m_bShow;
}

BOOL GameMainWnd::SetBackBitmap(HBITMAP hBmp)
{
    if (hBmp == NULL) {
        return FALSE;
    }

    BITMAP bi = { 0 };
    GetObject(hBmp, sizeof(BITMAP), &bi);

    m_hBackBitmap = hBmp;
    m_cxBackBitmap = bi.bmWidth;
    m_cyBackBitmap = bi.bmHeight;

    return TRUE;
}

void GameMainWnd::SetWindowRect(int x, int y, int cx, int cy)
{
    MoveWindow(m_hWnd, x, y, cx, cy, TRUE);
}

void GameMainWnd::ShowVoteLordButtons(int nCurScore, BOOL bShow /*= TRUE*/)
{
    if (bShow == TRUE) { // 显示之前，根据当前叫地主的分，使能部分按钮
        switch (nCurScore) {
            case 1:
                // 当前最高叫1分
                m_btnScore1.Enable(FALSE);
                break;

            case 2:
                // 当前最高叫2分
                m_btnScore1.Enable(FALSE);
                m_btnScore2.Enable(FALSE);
                break;
        }
    } else { // 隐藏之前，全部使能
        m_btnScore0.Enable();
        m_btnScore1.Enable();
        m_btnScore2.Enable();
        m_btnScore3.Enable();
    }

    m_btnScore0.Show(bShow);
    m_btnScore1.Show(bShow);
    m_btnScore2.Show(bShow);
    m_btnScore3.Show(bShow);
}

void GameMainWnd::ShowOutCardButtons(BOOL bFirstOutput, BOOL bShow /*= TRUE*/)
{
    if (bShow == TRUE) {
        if (bFirstOutput == TRUE) { // 显示之前，判断是首次出牌还是接上家的牌
            m_btnHint.Show(FALSE);
            m_btnPass.Show(FALSE);
            m_btnFollow.Show();
        } else {
            m_btnFollow.Show();
            m_btnPass.Show();
            m_btnHint.Show();
        }
    } else { 
        m_btnFollow.Show(FALSE);
        m_btnPass.Show(FALSE);
        m_btnHint.Show(FALSE);
    }
}

void GameMainWnd::OnLButtonDblClk(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    m_CurPokerWnd.UnSelectAllCards();
}

void GameMainWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    
    On_ID_BTN_FOLLOW();
}

void GameMainWnd::OnCommand(UINT nCtrlId)
{
#define ON_CMD_BREAK(CTRL_ID) case CTRL_ID: { On_ ## CTRL_ID (); break; }

    switch (nCtrlId) {
        ON_CMD_BREAK(   ID_BTN_START);
        ON_CMD_BREAK(   ID_BTN_SCORE0);
        ON_CMD_BREAK(   ID_BTN_SCORE1);
        ON_CMD_BREAK(   ID_BTN_SCORE2);
        ON_CMD_BREAK(   ID_BTN_SCORE3);
        ON_CMD_BREAK(   ID_BTN_PASS);
        ON_CMD_BREAK(   ID_BTN_FOLLOW);
        ON_CMD_BREAK(   ID_BTN_HINT);
        ON_CMD_BREAK(   ID_BTN_OPTION);
        ON_CMD_BREAK(   ID_BTN_DELEGATE);
        ON_CMD_BREAK(   ID_BTN_PREVROUND);
    }
}

void GameMainWnd::On_ID_BTN_START(void)
{
    int table = GetLocalUserTableId();
    assert(!IS_INVALID_TABLE(table));

    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));

    SendReqGamerReady(table, seat);
}

void GameMainWnd::On_ID_BTN_SCORE0(void)
{
    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));
    
    SendAckVoteLord(seat, 0);

    // 玩家已作出响应，则复位连续超时的次数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_SCORE1(void)
{
    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));

    SendAckVoteLord(seat, 1);

    // 玩家已作出响应，则复位连续超时的次数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_SCORE2(void)
{
    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));

    SendAckVoteLord(seat, 2);

    // 玩家已作出响应，则复位连续超时的次数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_SCORE3(void)
{
    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));

    SendAckVoteLord(seat, 3);

    // 玩家已作出响应，则复位连续超时的次数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_PASS(void)
{
    POKER_PROPERTY prop;
    ZeroMemory(&prop, sizeof(POKER_PROPERTY));

    int seat = GetLocalUserSeatId();
    assert(!IS_INVALID_SEAT(seat));

    // 发送不出牌
    SendAckOutputCard(seat, &prop, NULL);

    // 取消选择所有扑克牌
    m_CurPokerWnd.UnSelectAllCards();

    // 本轮不出牌，复位服务器请求出牌的标志
    m_bReqOutputCard = FALSE;

    // 玩家已作出响应，则复位连续超时的次数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_FOLLOW(void)
{
    if (m_bReqOutputCard == FALSE) {
        return;
    }

    int num = 0;
    int poker[PLAYER_MAX_CARDS_NUM] = { 0 };

    m_CurPokerWnd.GetSelectedCards(poker, PLAYER_MAX_CARDS_NUM, &num);
    if (num <= 0) {
        return;
    }

    BOOL bCanPlay = FALSE;
    BOOL bFirstOutput = FALSE;
    POKER_PROPERTY prop;

    if ((m_PokerProp.num == 0) && (m_PokerProp.value == 0)) {
        bFirstOutput = TRUE;
    }

    if (bFirstOutput == TRUE) { // 首次出牌
        bCanPlay = can_play_poker(&prop, poker, num);
    } else { // 接牌
        bCanPlay = can_follow_poker(&prop, poker, num, &m_PokerProp);
    }

    if (bCanPlay == TRUE) {
        int seat = GetLocalUserSeatId();
        assert(!IS_INVALID_SEAT(seat));

        int nSend = SendAckOutputCard(seat, &prop, poker);
        if (nSend > 0) {
            // 出牌后，复位标志
            m_bReqOutputCard = FALSE;

            // 玩家已作出响应，则复位连续超时的次数
            m_nTimeOutTimes = 0;
        }
    } else {
        WriteLog(_T("您选择的出牌不符合规则。"), GMW_LOGCLR_INVALID_OP);
    }
}

void GameMainWnd::On_ID_BTN_HINT(void)
{
    POKER_PROPERTY prop;
    int index[PLAYER_MAX_CARDS_NUM] = { 0 };

    if (m_pct.builded == 0) {
        int num = 0;
        int poker[PLAYER_MAX_CARDS_NUM] = { 0 };

        m_CurPokerWnd.GetCards(poker, &num);
        build_poker_class_table(&m_pct, poker, num);
    }

    BOOL bGetHint = FALSE;
    
    bGetHint = get_poker_hint(m_nHintTimes, &m_pct, &m_PokerProp, &prop, index);
    if (bGetHint == FALSE) {
        if (m_nHintTimes > 0) {
            //
            // 查找提示失败，但 m_nHintTimes 大于0，表示存在多个提示，
            // 只是已经到达最后一个提示，将 m_nHintTimes 复位为0，
            // 再查找一次，这样便可实现提示的循环。
            //
            m_nHintTimes = 0;
            bGetHint = get_poker_hint(m_nHintTimes, &m_pct, &m_PokerProp, &prop, index);
        }
    }

    if (bGetHint == TRUE) {
        //
        // 查找到一个提示，则将计数累加，下次点提示时，将获得下一个提示
        //
        m_nHintTimes++;
        m_CurPokerWnd.SelectCardsFromIndex(index, prop.num);
    } else {
        //
        // 没有一个可用的提示，则通知服务器“不出”
        //
        On_ID_BTN_PASS();
    }
}

void GameMainWnd::On_ID_BTN_OPTION(void)
{
    WriteLog(_T("NOT IMPLEMENTED."));
}

void GameMainWnd::On_ID_BTN_DELEGATE(void)
{
    if (I_AM_LOOKON) {
        WriteLog(_T("您是旁观者，不能托管游戏。"), GMW_LOGCLR_SPECIAL);
        return;
    }

    if (m_bGameStarted == FALSE) {
        WriteLog(_T("游戏尚未开始，不能托管游戏。"), GMW_LOGCLR_SPECIAL);
        return;
    }

    SendReqDelegate(CUR_SEAT_NUM, !m_bGamerDelegated[CUR_SEAT_NUM]);

    // 玩家已作出响应，则复位连续超时的次数
    // 玩家主动选择托管，或因连续两次超时进入托管时，均复位连续超时计数
    m_nTimeOutTimes = 0;
}

void GameMainWnd::On_ID_BTN_PREVROUND(void)
{
    if (m_bOutputCardStarted == TRUE) {
        m_btnPrevRound.Enable(FALSE);

        m_OutCardWndUp.ShowPrevRound();
        m_OutCardWndCur.ShowPrevRound();
        m_OutCardWndDown.ShowPrevRound();

        SetTimer(m_hWnd, ID_TIMER_SHOW_PREVROUND, SHOW_PREVROUND_TIMEOUT, NULL);
    } else {
        WriteLog(_T("尚未开始出牌，没有上一轮扑克牌。"), GMW_LOGCLR_SPECIAL);
    }
}

void GameMainWnd::OnTimer(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    UINT nID = (UINT)wParam;
    switch (nID) {
        case ID_TIMER_SHOW_PREVROUND:
            m_btnPrevRound.Enable();

            m_OutCardWndUp.ShowPrevRound(FALSE);
            m_OutCardWndCur.ShowPrevRound(FALSE);
            m_OutCardWndDown.ShowPrevRound(FALSE);

            KillTimer(m_hWnd, ID_TIMER_SHOW_PREVROUND);
            break;

        case ID_TIMER_SHOW_CLOCK:
            OnTimerClock();
            break;
    }
}

void GameMainWnd::OnTimerClock(void)
{
    if (m_bShowClockUp == TRUE) { // 上家
        if (m_nTimeOutUp > 0) {
            m_nTimeOutUp--;
            InvalidateRect(m_hWnd, &m_rcClockUp, FALSE);

            if (m_nTimeOutUp == GMW_TIMEOUT_SECOND_ALARM) {
                PlayGameSound(SND_GAME_TIMEOUT);
            }
        } else if (m_nTimeOutUp == 0) {
            // Do nothing but wait for voting lord or outputting card
            // from him or the server
        }
    }

    if (m_bShowClockDown == TRUE) { // 下家
        if (m_nTimeOutDown > 0) {
            m_nTimeOutDown--;
            InvalidateRect(m_hWnd, &m_rcClockDown, FALSE);

            if (m_nTimeOutDown == GMW_TIMEOUT_SECOND_ALARM) {
                PlayGameSound(SND_GAME_TIMEOUT);
            }
        } else if (m_nTimeOutDown == 0) {
            // Do nothing but wait for voting lord or outputting card
            // from him or the server
        }
    }

    if (m_bShowClockCur == TRUE) { // 当前玩家
        if (m_nTimeOutCur > 0) {
            m_nTimeOutCur--;
        }

        if (m_nTimeOutCur > 0) {
            InvalidateRect(m_hWnd, &m_rcClockCur, FALSE);

            if (m_nTimeOutCur == GMW_TIMEOUT_SECOND_ALARM) {
                PlayGameSound(SND_GAME_TIMEOUT);
            }
        } else if (m_nTimeOutCur == 0) {
            if (I_AM_LOOKON) {
                return;
            }

            m_nTimeOutTimes++; // 累计连续超时的次数

            if (m_nTimeOutTimes < GMW_MAX_TIMEOUT_TIMES) {
                if (m_bOutputCardStarted == FALSE) {
                    //
                    // 叫地主阶段。不叫地主
                    //
                    int seat = GetLocalUserSeatId();
                    assert(!IS_INVALID_SEAT(seat));
                    SendAckVoteLord(seat, 0);
                } else {
                    //
                    // 出牌阶段。若为当前玩家首次出牌，则出一张最小的牌；
                    // 若为当前玩家接上家的牌，则不出牌。
                    //
                    POKER_PROPERTY prop;

                    int seat = GetLocalUserSeatId();
                    assert(!IS_INVALID_SEAT(seat));

                    // 服务器请求本玩家首次出牌
                    if ((m_PokerProp.num == 0) && (m_PokerProp.value == 0)) {
                        int poker[PLAYER_MAX_CARDS_NUM] = { 0 };
                        int num = 0;

                        m_CurPokerWnd.GetCards(poker, &num);
                        assert(num > 0);

                        prop.num = 1;
                        prop.type = SINGLE;
                        prop.value = poker_index_to_value(poker[0]);

                        // 出该张最小的一张牌
                        SendAckOutputCard(seat, &prop, poker);
                    } else {
                        ZeroMemory(&prop, sizeof(POKER_PROPERTY));

                        // 不出牌
                        SendAckOutputCard(seat, &prop, NULL);
                    }

                    // 复位服务器请求出牌的标志
                    m_bReqOutputCard = FALSE;

                    // 取消选择所有扑克牌
                    m_CurPokerWnd.UnSelectAllCards();
                }
            } else {
                //
                // 超过允许的连续超时次数，将本玩家置为托管
                //
                On_ID_BTN_DELEGATE();
            }
        }
    }
}

void GameMainWnd::WriteLog(LPCTSTR lpszText, COLORREF clrText /*= RGB(0,0,0)*/)
{
    SendMessage(m_hWndParent, WM_WRITE_LOG, (WPARAM)clrText, (LPARAM)lpszText);
}

void GameMainWnd::Init(BOOL bIsLookon)
{
    // 初始游戏桌相关变量
    m_nCurUserSeat = INVALID_SEAT;
    m_bGameStarted = FALSE;
    m_bOutputCardStarted = FALSE;

    m_bNoGamerVoteLord = FALSE;
    m_nNoGamerVoteLordTimes = 0;
    m_bReqOutputCard = FALSE;

    m_nBombNum = 0;
    m_nHintTimes = 0;

    m_nLordSeat = INVALID_SEAT;
    m_nLordScore = 0;
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        m_bGamerConnLost[seat] = FALSE;
        m_bGamerDelegated[seat] = FALSE;
    }

    m_bGamerReadyUp = FALSE;
    m_bGamerReadyDown = FALSE;

    m_bShowClockUp = FALSE;
    m_bShowClockCur = FALSE;
    m_bShowClockDown = FALSE;
    m_nTimeOutTimes = 0;

    m_bIsLookon = bIsLookon;

    m_GamerVisualWndUp.SetGamerId(INVALID_USER_ID);
    m_GamerVisualWndCur.SetGamerId(INVALID_USER_ID);
    m_GamerVisualWndDown.SetGamerId(INVALID_USER_ID);

    m_CurPokerWnd.SetLord(FALSE);
    m_CurPokerWndUp.SetLord(FALSE);
    m_CurPokerWndDown.SetLord(FALSE);

    m_CurPokerWnd.SetCards(NULL, 0);
    m_CurPokerWndUp.SetCards(NULL, 0);
    m_CurPokerWndDown.SetCards(NULL, 0);

    m_OutCardWndCur.SetCards(NULL, 0);
    m_OutCardWndUp.SetCards(NULL, 0);
    m_OutCardWndDown.SetCards(NULL, 0);

    m_OutCardWndCur.SetGameStatInfo(GSI_NONE);
    m_OutCardWndUp.SetGameStatInfo(GSI_NONE);
    m_OutCardWndDown.SetGameStatInfo(GSI_NONE);

    m_UnderCardWnd.SetCards(NULL, 0);

    m_CurPokerWndUp.ShowPoker(FALSE);
    m_CurPokerWndDown.ShowPoker(FALSE);
    m_UnderCardWnd.ShowPoker(FALSE);

    // 初始化牌窗口是否显示牌面
    if (bIsLookon == TRUE) {
        // 自己旁观的椅子的玩家是否允许旁观
        int mytable = GetLocalUserTableId();
        int myseat  = GetLocalUserSeatId();
        BOOL bAllow = IsGamerAllowLookon(mytable, myseat);

        m_CurPokerWnd.ShowPoker(bAllow);
        m_OutCardWndCur.ShowPoker(bAllow);
        m_OutCardWndUp.ShowPoker(bAllow);
        m_OutCardWndDown.ShowPoker(bAllow);
    } else {
        m_CurPokerWnd.ShowPoker();
        m_OutCardWndCur.ShowPoker();
        m_OutCardWndUp.ShowPoker();
        m_OutCardWndDown.ShowPoker();
    }

    // 设置自己当前扑克牌是否可被选取
    m_CurPokerWnd.AllowPokerSelection(!bIsLookon);

    // 隐藏叫分、出牌按钮
    ShowVoteLordButtons(0, FALSE);
    ShowOutCardButtons(FALSE, FALSE);

    // 显示“开始”按钮
    m_btnStart.Show(!bIsLookon);
}

void GameMainWnd::SetCurrentUserSeat(int seat)
{
    m_nCurUserSeat = seat;
}

void GameMainWnd::GamerTakeSeat(int id, int seat)
{
    // 玩家进入后，不擦除其出牌窗口的数据，这样，上局游戏结束后，其他玩家还能看到
    // 各玩家最后的出牌。该清除工作放在玩家点击“开始”后，收到GamerReady时。
    if (seat == UP_SEAT_NUM) {
        m_GamerVisualWndUp.SetGamerId(id);

        //m_OutCardWndUp.SetCards(NULL, 0);
        //m_CurPokerWndUp.SetCards(NULL, 0);
    }
    
    else if (seat == DOWN_SEAT_NUM) {
        m_GamerVisualWndDown.SetGamerId(id);

        //m_OutCardWndDown.SetCards(NULL, 0);
        //m_CurPokerWndDown.SetCards(NULL, 0);
    }
    
    else if (seat == CUR_SEAT_NUM) {
        m_GamerVisualWndCur.SetGamerId(id);

        //m_OutCardWndCur.SetCards(NULL, 0);
        //m_CurPokerWnd.SetCards(NULL, 0);
    }
}

void GameMainWnd::GamerLeaveSeat(int id, int seat)
{
    UNREFERENCED_PARAMETER(id);

    if (seat == UP_SEAT_NUM) {
        // 清除形象图
        m_GamerVisualWndUp.SetGamerId(INVALID_USER_ID);

        m_OutCardWndUp.SetGameStatInfo(GSI_NONE);

        m_bGamerReadyUp = FALSE;

        // 擦除地主头像图标
        if (seat == m_nLordSeat) {
            m_nLordSeat = INVALID_SEAT;
            m_nLordScore = 0;
            InvalidateRect(m_hWnd, &m_rcLordUp, FALSE);
        }

        // 擦除掉线图标
        if (m_bGamerConnLost[seat] == TRUE) {
            m_bGamerConnLost[seat] = FALSE;
            InvalidateRect(m_hWnd, &m_rcConnLostUp, FALSE);
        }

        // 擦除托管图标
        if (m_bGamerDelegated[seat] == TRUE) {
            m_bGamerDelegated[seat] = FALSE;
            InvalidateRect(m_hWnd, &m_rcDelegatedUp, FALSE);

            TCHAR szText[48] = { 0 };
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]),
                _T("游戏结束。[ %s ]取消托管。"),
                GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));

            WriteLog(szText, GMW_LOGCLR_DELEGATE);
        }
    } else if (seat == DOWN_SEAT_NUM) {
        // 清除形象图
        m_GamerVisualWndDown.SetGamerId(INVALID_USER_ID);

        m_OutCardWndDown.SetGameStatInfo(GSI_NONE);

        m_bGamerReadyDown = FALSE;

        // 擦除地主头像图标
        if (seat == m_nLordSeat) {
            m_nLordSeat = INVALID_SEAT;
            m_nLordScore = 0;
            InvalidateRect(m_hWnd, &m_rcLordDown, FALSE);
        }

        // 清除可能存在的掉线
        if (m_bGamerConnLost[seat] == TRUE) {
            m_bGamerConnLost[seat] = FALSE;
            InvalidateRect(m_hWnd, &m_rcConnLostDown, FALSE);
        }
        
        // 清除托管图标
        if (m_bGamerDelegated[seat] == TRUE) {
            m_bGamerDelegated[seat] = FALSE;
            InvalidateRect(m_hWnd, &m_rcDelegatedDown, FALSE);

            TCHAR szText[48] = { 0 };
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]),
                _T("游戏结束。[ %s ]取消托管。"),
                GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));

            WriteLog(szText, GMW_LOGCLR_DELEGATE);
        }
    } else if (seat == CUR_SEAT_NUM) {
        // 清除形象图
        m_GamerVisualWndCur.SetGamerId(INVALID_USER_ID);

        m_OutCardWndCur.SetGameStatInfo(GSI_NONE);

        if (I_AM_LOOKON) {
            int table = GetLocalUserTableId();
            assert(!IS_INVALID_TABLE(table));

            // 旁观者主动发送离开椅子的消息到服务器
            SendReqLookonLeaveSeat(table, seat);

            // 将自己从本地大厅信息中删除，不必等待服务器1秒定时广播的通知我离开座位，这样要快些
            int nUserId = GetLocalUserId();
            g_PlayerData[nUserId].playerInfo.table = INVALID_TABLE;
            g_PlayerData[nUserId].playerInfo.seat = INVALID_SEAT;
            g_PlayerData[nUserId].playerInfo.state = STATE_IDLE;

            //MessageBox(g_hMainWnd, _T("您旁观的玩家已经离开，请退出游戏桌并重新选择座位。"), _T("提示"), MB_OK);
            WriteLog(_T("您旁观的玩家已经离开，请退出游戏桌并重新选择座位。"), GMW_LOGCLR_INVALID_OP);
        }
    }
}

void GameMainWnd::GamerReady(int id, int seat)
{
    UNREFERENCED_PARAMETER(id);

    // 显示“准备”提示
    if (seat == UP_SEAT_NUM) {
        m_OutCardWndUp.SetGameStatInfo(GSI_READY);
        m_bGamerReadyUp = TRUE;
    }
    
    else if (seat == DOWN_SEAT_NUM) {
        m_OutCardWndDown.SetGameStatInfo(GSI_READY);
        m_bGamerReadyDown = TRUE;
    }
    
    else if (seat == CUR_SEAT_NUM) {
        m_bNoGamerVoteLord = FALSE;
        m_nNoGamerVoteLordTimes = 0;

        m_bShowClockCur = FALSE;
        m_bShowClockUp = FALSE;
        m_bShowClockDown = FALSE;
        m_nTimeOutTimes = 0;

        // 如果之前有炸弹，则清除其图标
        if (m_nBombNum > 0) {
            RECT rcBomb;
            GetClientRect(m_hWnd, &rcBomb);

            int nMargin = (40 - CY_ICON_BOMB) / 2;
            if (nMargin < 0) { nMargin = 0; }

            int xBombStart = rcBomb.right / 2;
            int yBombStart = rcBomb.bottom - nMargin - CY_ICON_BOMB;

            rcBomb.left     = xBombStart;
            rcBomb.top      = yBombStart;
            rcBomb.right    = rcBomb.left + m_nBombNum * CX_ICON_BOMB;
            rcBomb.bottom   = rcBomb.top + CY_ICON_BOMB;

            m_nBombNum = 0;
            InvalidateRect(m_hWnd, &rcBomb, FALSE);
        }

        // 清除地主头像图标
        if (m_nLordScore > 0) {
            if (m_nLordSeat == CUR_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcLordCur, FALSE);
            } else if (m_nLordSeat == UP_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcLordUp, FALSE);
            } else if (m_nLordSeat == DOWN_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcLordDown, FALSE);
            }

            m_nLordScore = 0;
            m_nLordSeat = INVALID_SEAT;

            // 擦除“底分”
            RedrawLordScoreRect();
        }

        //
        // 清空有关窗口内容
        //
        m_UnderCardWnd.SetCards(NULL, 0);

        m_CurPokerWnd.SetLord(FALSE);
        m_CurPokerWndUp.SetLord(FALSE);
        m_CurPokerWndDown.SetLord(FALSE);

        m_CurPokerWnd.SetCards(NULL, 0);
        m_CurPokerWndUp.SetCards(NULL, 0);
        m_CurPokerWndDown.SetCards(NULL, 0);

        m_OutCardWndUp.SetCards(NULL, 0);
        if (m_bGamerReadyUp == FALSE) {
            m_OutCardWndUp.SetGameStatInfo(GSI_NONE);
        }

        m_OutCardWndDown.SetCards(NULL, 0);
        if (m_bGamerReadyDown == FALSE) {
            m_OutCardWndDown.SetGameStatInfo(GSI_NONE);
        }

        m_OutCardWndCur.SetCards(NULL, 0);
        m_OutCardWndCur.SetGameStatInfo(GSI_READY);

        //
        // 设置窗口隐藏
        //
        m_CurPokerWndUp.ShowPoker(FALSE);
        m_CurPokerWndDown.ShowPoker(FALSE);
        m_UnderCardWnd.ShowPoker(FALSE);

        if (I_AM_LOOKON) {
            // 自己旁观的椅子的玩家是否允许旁观
            int mytable = GetLocalUserTableId();
            int myseat  = GetLocalUserSeatId();
            BOOL bAllow = IsGamerAllowLookon(mytable, myseat);

            m_CurPokerWnd.ShowPoker(bAllow);

            m_OutCardWndCur.ShowPoker(bAllow);
            m_OutCardWndUp.ShowPoker(bAllow);
            m_OutCardWndDown.ShowPoker(bAllow);
        } else {
            m_CurPokerWnd.ShowPoker();

            m_OutCardWndCur.ShowPoker();
            m_OutCardWndUp.ShowPoker();
            m_OutCardWndDown.ShowPoker();
        }

        // 隐藏“开始”按钮
        m_btnStart.Show(FALSE);
    }
}

void GameMainWnd::GamerCanStart(void)
{
    m_btnStart.Show();
    m_bGameStarted = FALSE;
    m_bOutputCardStarted = FALSE;

    //
    // 清除可能存在的掉线或托管图标
    // GamerOver 之后需要作该操作，如果VOTE_LORD失败，又存在掉线和托管者，
    // 服务器不会产生GameOver消息而是直接结束游戏。因此，当玩家回到SIT 状态时
    // 使能“开始”按钮，此外，还得再检查一下是否有托管或掉线的，需将其清除，
    // 因为，可能游戏根本就没有开始出牌就结束了，即不能到达GameOver。
    //
    ClearLostAndDelegatedIcon();
}

void GameMainWnd::ClearLostAndDelegatedIcon(void)
{
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        if (m_bGamerConnLost[i] == TRUE) {
            m_bGamerConnLost[i] = FALSE;

            if (i == UP_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcConnLostUp, FALSE);
            } else if (i == DOWN_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcConnLostDown, FALSE);
            } else if (i == CUR_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcConnLostCur, FALSE);
            }
        }

        if (m_bGamerDelegated[i] == TRUE) {
            m_bGamerDelegated[i] = FALSE;

            TCHAR szText[48] = { 0 };
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]),
                _T("游戏结束。[ %s ]取消托管。"),
                GetUserNameStr(GetGamerId(GetLocalUserTableId(), i)));

            WriteLog(szText, GMW_LOGCLR_DELEGATE);

            if (i == UP_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcDelegatedUp, FALSE);
            } else if (i == DOWN_SEAT_NUM) {
                InvalidateRect(m_hWnd, &m_rcDelegatedDown, FALSE);
            } else if (i == CUR_SEAT_NUM) {
                m_btnDelegate.SetText(_T("托管"));
                InvalidateRect(m_hWnd, &m_rcDelegatedCur, FALSE);
            }
        }
    }
}

void GameMainWnd::DistributePokerCards(int seat, int poker[], int num)
{
    if (seat == UP_SEAT_NUM) {
        m_OutCardWndUp.SetGameStatInfo(GSI_NONE);
        m_CurPokerWndUp.SetCards(poker, num);
        m_bGamerReadyUp = FALSE;
    }

    else if (seat == DOWN_SEAT_NUM) {
        m_OutCardWndDown.SetGameStatInfo(GSI_NONE);
        m_CurPokerWndDown.SetCards(poker, num);
        m_bGamerReadyDown = FALSE;
    }

    else if (seat == CUR_SEAT_NUM) {
        m_OutCardWndCur.SetGameStatInfo(GSI_NONE);
        m_CurPokerWnd.SetCards(poker, num);

        m_bGameStarted = TRUE;
        PlayGameSound(SND_GAME_START);

        if (m_bNoGamerVoteLord == TRUE) {
            WriteLog(_T("没有玩家叫地主，服务器重新发牌。"), GMW_LOGCLR_SPECIAL);
        }
    }
}

void GameMainWnd::SetUnderPokerCards(int poker[], int num)
{
    m_UnderCardWnd.SetCards(poker, num);
}

void GameMainWnd::GamerVoteLord(int seat, int score)
{
    TCHAR szText[32] = { 0 };
    _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("%d 椅子玩家叫 %d 分。"), seat, score);
    WriteLog(szText, GMW_LOGCLR_NORMAL);

    // 显示提示“1分”，“2分”，“3分”，“不叫”
    if (seat == UP_SEAT_NUM) {
        if (m_bShowClockUp == TRUE) {
            m_nTimeOutUp = 0;
            m_bShowClockUp = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockUp, FALSE);
        }

        switch (score) {
            case 0: m_OutCardWndUp.SetGameStatInfo(GSI_SCORE0); break;
            case 1: m_OutCardWndUp.SetGameStatInfo(GSI_SCORE1); break;
            case 2: m_OutCardWndUp.SetGameStatInfo(GSI_SCORE2); break;
            case 3: m_OutCardWndUp.SetGameStatInfo(GSI_SCORE3); break;
        }
    } else if (seat == DOWN_SEAT_NUM) {
        if (m_bShowClockDown == TRUE) {
            m_nTimeOutDown = 0;
            m_bShowClockDown = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockDown, FALSE);
        }

        switch (score) {
            case 0: m_OutCardWndDown.SetGameStatInfo(GSI_SCORE0);   break;
            case 1: m_OutCardWndDown.SetGameStatInfo(GSI_SCORE1);   break;
            case 2: m_OutCardWndDown.SetGameStatInfo(GSI_SCORE2);   break;
            case 3: m_OutCardWndDown.SetGameStatInfo(GSI_SCORE3);   break;
        }
    } else if (seat == CUR_SEAT_NUM) {
        if (m_bShowClockCur == TRUE) {
            m_nTimeOutCur = 0;
            m_bShowClockCur = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockCur, FALSE);
        }

        switch (score) {
            case 0: m_OutCardWndCur.SetGameStatInfo(GSI_SCORE0);   break;
            case 1: m_OutCardWndCur.SetGameStatInfo(GSI_SCORE1);   break;
            case 2: m_OutCardWndCur.SetGameStatInfo(GSI_SCORE2);   break;
            case 3: m_OutCardWndCur.SetGameStatInfo(GSI_SCORE3);   break;
        }

        if (!I_AM_LOOKON) {
            ShowVoteLordButtons(0, FALSE); // 服务器收到我发送叫地主的分数，则隐藏按钮
        }
    }

    // 播放声音
    int table = GetLocalUserTableId();
    PLAYER_GENDER gender = GetGamerGender(table, seat);
    if (gender == MALE) {
        switch (score) {
            case 0: PlayGameSound(SND_GAME_SCORE0_M);   break;
            case 1: PlayGameSound(SND_GAME_SCORE1_M);   break;
            case 2: PlayGameSound(SND_GAME_SCORE2_M);   break;
            case 3: PlayGameSound(SND_GAME_SCORE3_M);   break;
        }
    } else {
        switch (score) {
            case 0: PlayGameSound(SND_GAME_SCORE0_F);   break;
            case 1: PlayGameSound(SND_GAME_SCORE1_F);   break;
            case 2: PlayGameSound(SND_GAME_SCORE2_F);   break;
            case 3: PlayGameSound(SND_GAME_SCORE3_F);   break;
        }
    }

    // 更新“底分”
    if (score > m_nLordScore) {
        m_nLordScore = score;
        RedrawLordScoreRect();
    }
}

void GameMainWnd::ReqVoteLord(int seat, int score)
{
    // ToDo: 给服务器正在请求叫分的玩家椅子号开启定时钟表
    if (seat == UP_SEAT_NUM) {
        m_nTimeOutUp = GMW_TIMEOUT_SECOND;
        m_bShowClockUp = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockUp, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);
    }
    
    else if (seat == DOWN_SEAT_NUM) {
        m_nTimeOutDown = GMW_TIMEOUT_SECOND;
        m_bShowClockDown = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockDown, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);
    }
    
    else if (seat == CUR_SEAT_NUM) {
        m_nTimeOutCur = GMW_TIMEOUT_SECOND;
        m_bShowClockCur = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockCur, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);

        if (!I_AM_LOOKON) { // 服务器请求我叫地主
            ShowVoteLordButtons(score, TRUE);
            FlashInactiveWnd();
        }
    }
}

void GameMainWnd::VoteLordFinish(BOOL bVoteSucceed, int nLordSeat, int nLordScore)
{
    // 清除叫分
    m_OutCardWndCur.SetGameStatInfo(GSI_NONE);
    m_OutCardWndUp.SetGameStatInfo(GSI_NONE);
    m_OutCardWndDown.SetGameStatInfo(GSI_NONE);

    // 清除出牌窗口的内容。其实，在这之前的GAMER_READY时就清除出牌窗口数据了。
    // 这里再调用一次是为了清除出牌窗口保存的玩家上一轮的扑克牌数据。
    m_OutCardWndUp.SetCards(NULL, 0);
    m_OutCardWndCur.SetCards(NULL, 0);
    m_OutCardWndDown.SetCards(NULL, 0);

    if (bVoteSucceed == TRUE) { // 地主产生
        m_bNoGamerVoteLord = FALSE;
        m_nNoGamerVoteLordTimes = 0;

        m_nLordSeat = nLordSeat;
        m_nLordScore = nLordScore;

        // 标识开始出牌
        m_bOutputCardStarted = TRUE;

        TCHAR szMsg[32] = { 0 };
        _stprintf_s(szMsg, sizeof(szMsg) / sizeof(szMsg[0]),
            _T("%d 椅子玩家当地主。底分为 %d 分。"), nLordSeat, nLordScore);
        WriteLog(szMsg, GMW_LOGCLR_NORMAL);

        // 打开底牌
        if (I_AM_LOOKON) {
            // 自己旁观的椅子的玩家是否允许旁观
            int mytable = GetLocalUserTableId();
            int myseat  = GetLocalUserSeatId();
            BOOL bAllow = IsGamerAllowLookon(mytable, myseat);
            m_UnderCardWnd.ShowPoker(bAllow);
        } else {
            m_UnderCardWnd.ShowPoker();

            // 地主已经选出。
            // 在开始出牌前，复位玩家的扑克分类表，指示玩家的扑克分类表尚未构建。
            // 每次出牌（牌的数量变化）后，都要对其复位操作，以获得正确的出牌提示
            reset_poker_class_table(&m_pct);
        }

        // 给地主添加底牌，并显示地主头像图标
        int num = 0;
        int undercards[UNDER_CARDS_NUM] = { 0 };
        m_UnderCardWnd.GetCards(undercards, &num);

        if (nLordSeat == CUR_SEAT_NUM) {
            m_CurPokerWnd.SetLord();
            m_CurPokerWnd.AddCards(undercards, num);
            InvalidateRect(m_hWnd, &m_rcLordCur, FALSE);
        } else if (nLordSeat == UP_SEAT_NUM) {
            m_CurPokerWndUp.SetLord();
            m_CurPokerWndUp.AddCards(undercards, num);
            InvalidateRect(m_hWnd, &m_rcLordUp, FALSE);
        } else if (nLordSeat == DOWN_SEAT_NUM) {
            m_CurPokerWndDown.SetLord();
            m_CurPokerWndDown.AddCards(undercards, num);
            InvalidateRect(m_hWnd, &m_rcLordDown, FALSE);
        }
    } else {
        WriteLog(_T("本局游戏没有玩家叫地主。"), GMW_LOGCLR_NORMAL);

        m_bGameStarted = FALSE;
        m_bNoGamerVoteLord = TRUE;
        m_nNoGamerVoteLordTimes++;
        if (m_nNoGamerVoteLordTimes > SERVER_REDISTR_POKER_TIMES) {
            WriteLog(_T("连续3次没有玩家叫地主，服务器将不重新发牌。"), GMW_LOGCLR_SPECIAL);

            // 清除可能存在的掉线或托管图标
            ClearLostAndDelegatedIcon();
        }
    }
}

void GameMainWnd::ReqOutputCard(int seat, POKER_PROPERTY* pp)
{
    // ToDo: 给服务器正在请求出牌的玩家椅子开启定时钟表
    if (seat == UP_SEAT_NUM) {
        m_OutCardWndUp.Clear();

        m_nTimeOutUp = GMW_TIMEOUT_SECOND;
        m_bShowClockUp = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockUp, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);
    }
    
    else if (seat == DOWN_SEAT_NUM) {
        m_OutCardWndDown.Clear();

        m_nTimeOutDown = GMW_TIMEOUT_SECOND;
        m_bShowClockDown = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockDown, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);
    }
    
    else if (seat == CUR_SEAT_NUM) {
        m_OutCardWndCur.Clear();

        m_nTimeOutCur = GMW_TIMEOUT_SECOND;
        m_bShowClockCur = TRUE;
        InvalidateRect(m_hWnd, &m_rcClockCur, FALSE);
        SetTimer(m_hWnd, ID_TIMER_SHOW_CLOCK, SHOW_CLOCK_TIMEOUT, NULL);

        if (!I_AM_LOOKON) { // 服务器请求我出牌
            if ((pp->num == 0) && (pp->value == 0)) { // 首次出牌
                ShowOutCardButtons(TRUE, TRUE);
            } else {
                ShowOutCardButtons(FALSE, TRUE);
            }

            // 如果窗口未激活，则闪烁窗口
            FlashInactiveWnd();

            // 设置可以出牌标志，玩家选择出牌或不出之后，需复位该标志
            m_bReqOutputCard = TRUE;
            
            // 每次被请求出牌时，则复位请求提示的次数
            m_nHintTimes = 0;

            // 记录服务器请求我出牌的属性
            m_PokerProp.type = pp->type;
            m_PokerProp.value = pp->value;
            m_PokerProp.num = pp->num;
        }
    }
}

void GameMainWnd::OutputCard(int seat, int num, int poker[])
{
    if (seat == UP_SEAT_NUM) {
        if (m_bShowClockUp == TRUE) {
            m_nTimeOutUp = 0;
            m_bShowClockUp = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockUp, FALSE);
        }

        if (num <= 0) {
            m_OutCardWndUp.SetCards(NULL, 0);
            m_OutCardWndUp.SetGameStatInfo(GSI_PASS);
        } else {
            m_CurPokerWndUp.RemoveCards(poker, num);
            m_OutCardWndUp.SetCards(poker, num);
            m_OutCardWndUp.SetGameStatInfo(GSI_NONE);
        }
    } else if (seat == DOWN_SEAT_NUM) {
        if (m_bShowClockDown == TRUE) {
            m_nTimeOutDown = 0;
            m_bShowClockDown = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockDown, FALSE);
        }

        if (num <= 0) {
            m_OutCardWndDown.SetCards(NULL, 0);
            m_OutCardWndDown.SetGameStatInfo(GSI_PASS);
        } else {
            m_CurPokerWndDown.RemoveCards(poker, num);
            m_OutCardWndDown.SetCards(poker, num);
            m_OutCardWndDown.SetGameStatInfo(GSI_NONE);
        }
    } else if (seat == CUR_SEAT_NUM) {
        if (m_bShowClockCur == TRUE) {
            m_nTimeOutCur = 0;
            m_bShowClockCur = FALSE;
            KillTimer(m_hWnd, ID_TIMER_SHOW_CLOCK);
            InvalidateRect(m_hWnd, &m_rcClockCur, FALSE);
        }

        if (num <= 0) {
            m_OutCardWndCur.SetCards(NULL, 0);
            m_OutCardWndCur.SetGameStatInfo(GSI_PASS);
        } else {
            m_CurPokerWnd.RemoveCards(poker, num);
            m_OutCardWndCur.SetCards(poker, num);
            m_OutCardWndCur.SetGameStatInfo(GSI_NONE);

            // 出牌后，因牌的数量变化，需重新构建扑克分类表，以便获取正确的提示
            if (!I_AM_LOOKON) {
                reset_poker_class_table(&m_pct);
            }
        }

        // 服务器已经接收到我出的牌，隐藏出牌按钮
        ShowOutCardButtons(FALSE, FALSE);
    }

    if (num > 0) {
        POKER_PROPERTY pp;
        BOOL bValidType = can_play_poker(&pp, poker, num);

        if (bValidType == TRUE) {
            if (pp.type != BOMB) {
                PlayGameSound(SND_GAME_OUTPUT_CARD);
            } else {
                PlayGameSound(SND_GAME_BOMB);

                RECT rect;
                RECT rcNewBomb;
                GetClientRect(m_hWnd, &rect);

                int nMargin = (40 - CY_ICON_BOMB) / 2;
                if (nMargin < 0) { nMargin = 0; }

                int xBombStart = rect.right / 2 + m_nBombNum * CX_ICON_BOMB;
                int yBombStart = rect.bottom - nMargin - CY_ICON_BOMB;

                rcNewBomb.left      = xBombStart;
                rcNewBomb.top       = yBombStart;
                rcNewBomb.right     = rcNewBomb.left + CX_ICON_BOMB;
                rcNewBomb.bottom    = rcNewBomb.top + CY_ICON_BOMB;

                m_nBombNum++;
                InvalidateRect(m_hWnd, &rcNewBomb, FALSE);
            }
        }
    }
}

void GameMainWnd::GameOver(GAME_RESULT* lpResult)
{
    m_bGameStarted = FALSE;
    m_bOutputCardStarted = FALSE;

    //
    // 输出游戏结果信息
    //
    int table = GetLocalUserTableId();
    int seat = GetLocalUserSeatId();

    assert(!IS_INVALID_TABLE(table));
    assert(!IS_INVALID_SEAT(seat));

    int len = 0;
    TCHAR szText[128] = { 0 };

    len += _stprintf_s(szText + len, sizeof(szText) / sizeof(szText[0]) - len,
        _T("本局游戏结束。得分结果:\n"));

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        len += _stprintf_s(szText + len, sizeof(szText) / sizeof(szText[0]) - len,
            _T("%d 椅子玩家 [%s]: %d\n"),
            i, GetUserNameStr(GetGamerId(table, i)), lpResult->nScore[i]);
    }

    WriteLog(szText, GMW_LOGCLR_GAME_RESULT);

    if (lpResult->nScore[seat] > 0) {
        PlayGameSound(SND_GAME_WIN);
    }

    // 更新本游戏桌内玩家的分数，不必等待服务器定时广播时的通知
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        int gamerid = GetGamerId(table, i);
        assert(!IS_INVALID_USER_ID(gamerid));
        
        g_PlayerData[gamerid].playerInfo.nScore += lpResult->nScore[i];
    }

    // 通知父窗口，以更新ListCtrl
    SendMessage(m_hWndParent, WM_GAME_OVER, 0, 0);

    // 打开所有玩家手中的牌
    if (I_AM_LOOKON) {
        if (IsGamerAllowLookon(table, seat) == TRUE) {
            m_CurPokerWnd.ShowPoker();
            m_CurPokerWndUp.ShowPoker();
            m_CurPokerWndDown.ShowPoker();
        }
    } else {
        m_CurPokerWnd.ShowPoker();
        m_CurPokerWndUp.ShowPoker();
        m_CurPokerWndDown.ShowPoker();
    }

    // 清除玩家掉线、托管游戏的图标
    ClearLostAndDelegatedIcon();
}

void GameMainWnd::OutputChatText(int seat, int nMsgIndex)
{
    TCHAR szText[160];
    int table = GetLocalUserTableId();

    if (!IS_INVALID_TABLE(table)) {
        int gamerid = GetGamerId(table, seat);
        if (!IS_INVALID_USER_ID(gamerid)) {
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("%s 说: "), GetUserNameStr(gamerid));
            _tcscat_s(szText, sizeof(szText) / sizeof(szText[0]), g_lpszChatMessage[nMsgIndex]);

            WriteLog(szText, GMW_LOGCLR_CHAT);
        }
    }
}

void GameMainWnd::OutputChatText(int seat, LPCTSTR lpszChatText)
{
    TCHAR szText[160];
    int table = GetLocalUserTableId();

    if (!IS_INVALID_TABLE(table)) {
        int gamerid = GetGamerId(table, seat);
        if (!IS_INVALID_USER_ID(gamerid)) {
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("%s 说: "), GetUserNameStr(gamerid));
            _tcscat_s(szText, sizeof(szText) / sizeof(szText[0]), lpszChatText);

            WriteLog(szText, GMW_LOGCLR_CHAT);
        }
    }
}

void GameMainWnd::RedrawLordScoreRect(void)
{
    RECT rcLordScore;
    GetClientRect(m_hWnd, &rcLordScore);

    rcLordScore.right   = rcLordScore.right / 2;
    rcLordScore.bottom  = rcLordScore.bottom - 10;
    rcLordScore.top     = rcLordScore.bottom - GMW_CY_LORD_SCORE;
    rcLordScore.left    = rcLordScore.right - GMW_CX_LORD_SCORE;

    InvalidateRect(m_hWnd, &rcLordScore, FALSE);
}

void GameMainWnd::GamerDelegate(int seat, BOOL bDelegated)
{
    TCHAR szText[48] = { 0 };
    m_bGamerDelegated[seat] = bDelegated;

    if (seat == UP_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcDelegatedUp, FALSE);

        _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("玩家[ %s ]"),
            GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));
    } else if (seat == DOWN_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcDelegatedDown, FALSE);

        _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("玩家[ %s ]"),
            GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));
    } else if (seat == CUR_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcDelegatedCur, FALSE);

        if (I_AM_LOOKON) {
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("玩家[ %s ]"),
                GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));
        } else {
            _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("您"));

            if (bDelegated == TRUE) {
                m_bReqOutputCard = FALSE; // 当前玩家托管之后，复位标志

                m_btnDelegate.SetText(_T("不托管"));
            } else {
                m_btnDelegate.SetText(_T("托管"));
            }
        }
    }

    if (bDelegated == TRUE) {
        _tcscat_s(szText, sizeof(szText) / sizeof(szText[0]), _T("选择托管。"));
    } else {
        _tcscat_s(szText, sizeof(szText) / sizeof(szText[0]), _T("取消托管。"));
    }

    WriteLog(szText, GMW_LOGCLR_DELEGATE);
}

void GameMainWnd::GamerDisconnected(int seat)
{
    m_bGamerConnLost[seat] = TRUE;

    if (seat == UP_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcConnLostUp, FALSE);
    } else if (seat == DOWN_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcConnLostDown, FALSE);
    } else if (seat == CUR_SEAT_NUM) {
        InvalidateRect(m_hWnd, &m_rcConnLostCur, FALSE);
    }

    TCHAR szText[48] = { 0 };
    _stprintf_s(szText, sizeof(szText) / sizeof(szText[0]), _T("玩家[ %s ]掉线，其游戏将被托管。"),
        GetUserNameStr(GetGamerId(GetLocalUserTableId(), seat)));

    WriteLog(szText, GMW_LOGCLR_DISCONNECT);
}

void GameMainWnd::DrawIconLord(HDC hdc)
{
    if (m_nLordSeat == CUR_SEAT_NUM) {
        DrawIconEx(hdc, m_rcLordCur.left, m_rcLordCur.top, g_hiconLord,
            CX_ICON_LORD, CY_ICON_LORD, NULL, NULL, DI_NORMAL);
    } else if (m_nLordSeat == UP_SEAT_NUM) {
        DrawIconEx(hdc, m_rcLordUp.left, m_rcLordUp.top, g_hiconLord,
            CX_ICON_LORD, CY_ICON_LORD, NULL, NULL, DI_NORMAL);
    } else if (m_nLordSeat == DOWN_SEAT_NUM) {
        DrawIconEx(hdc, m_rcLordDown.left, m_rcLordDown.top, g_hiconLord,
            CX_ICON_LORD, CY_ICON_LORD, NULL, NULL, DI_NORMAL);
    }
}

void GameMainWnd::DrawIconConnLost(HDC hdc)
{
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        if (m_bGamerConnLost[i] == TRUE) {
            if (i == CUR_SEAT_NUM) {
                DrawIconEx(hdc, m_rcConnLostCur.left, m_rcConnLostCur.top,
                    g_hiconConnLost, CX_ICON_CONNLOST, CY_ICON_CONNLOST, NULL, NULL, DI_NORMAL);
            } else if (i == UP_SEAT_NUM) {
                DrawIconEx(hdc, m_rcConnLostUp.left, m_rcConnLostUp.top,
                    g_hiconConnLost, CX_ICON_CONNLOST, CY_ICON_CONNLOST, NULL, NULL, DI_NORMAL);
            } else if (i == DOWN_SEAT_NUM) {
                DrawIconEx(hdc, m_rcConnLostDown.left, m_rcConnLostDown.top,
                    g_hiconConnLost, CX_ICON_CONNLOST, CY_ICON_CONNLOST, NULL, NULL, DI_NORMAL);
            }
        }
    }
}

void GameMainWnd::DrawIconDelegated(HDC hdc)
{
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        if (m_bGamerDelegated[i] == TRUE) {
            if (i == CUR_SEAT_NUM) {
                DrawIconEx(hdc, m_rcDelegatedCur.left, m_rcDelegatedCur.top,
                    g_hiconDelegated, CX_ICON_DELEGATE, CY_ICON_DELEGATE, NULL, NULL, DI_NORMAL);
            } else if (i == UP_SEAT_NUM) {
                DrawIconEx(hdc, m_rcDelegatedUp.left, m_rcDelegatedUp.top,
                    g_hiconDelegated, CX_ICON_DELEGATE, CY_ICON_DELEGATE, NULL, NULL, DI_NORMAL);
            } else if (i == DOWN_SEAT_NUM) {
                DrawIconEx(hdc, m_rcDelegatedDown.left, m_rcDelegatedDown.top,
                    g_hiconDelegated, CX_ICON_DELEGATE, CY_ICON_DELEGATE, NULL, NULL, DI_NORMAL);
            }
        }
    }
}

void GameMainWnd::DrawIconBomb(HDC hdc)
{
    int nMargin = (40 - CY_ICON_BOMB) / 2;
    if (nMargin < 0) { nMargin = 0; }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    int xBombStart = rect.right / 2;
    int yBombStart = rect.bottom - nMargin - CY_ICON_BOMB;

    for (int i = 0; i < m_nBombNum; i++) {
        DrawIconEx(hdc, xBombStart, yBombStart, g_hiconBomb,
            CX_ICON_BOMB, CY_ICON_BOMB, NULL, NULL, DI_NORMAL);

        xBombStart += CX_ICON_BOMB;
    }
}

void GameMainWnd::DrawLordScoreString(HDC hdc)
{
    if (m_nLordScore > 0) { // 有地主叫分时才绘制，否则不绘制
        RECT rect;
        GetClientRect(m_hWnd, &rect);

        RECT rcLordScore;
        rcLordScore.right   = rect.right / 2;
        rcLordScore.bottom  = rect.bottom - 10;
        rcLordScore.top     = rcLordScore.bottom - GMW_CY_LORD_SCORE;
        rcLordScore.left    = rcLordScore.right - GMW_CX_LORD_SCORE;

        TCHAR szScoreText[16] = { 0 };
        int cchText = _stprintf_s(szScoreText, sizeof(szScoreText) / sizeof(szScoreText[0]),
            _T("底分: %d "), m_nLordScore);

        HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT oldfont = (HFONT)SelectObject(hdc, hfont);
        int mode = SetBkMode(hdc, TRANSPARENT);
        COLORREF oldtextclr = SetTextColor(hdc, GMW_TEXT_CLR);

        DrawText(hdc, szScoreText, cchText, &rcLordScore, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        SetTextColor(hdc, oldtextclr);
        SetBkMode(hdc, mode);
        SelectObject(hdc, oldfont);
    }
}

void GameMainWnd::DrawClock(HDC hdc)
{
    HDC clockDC = CreateCompatibleDC(hdc);
    HBITMAP oldClockBmp = (HBITMAP)SelectObject(clockDC, g_hbmpClockDigital);

    if (m_bShowClockUp == TRUE) {
        DrawIconEx(hdc, m_rcClockUp.left, m_rcClockUp.top, g_hiconClock, 
            CX_ICON_CLOCK, CY_ICON_CLOCK, NULL, NULL, DI_NORMAL);

        int high = (m_nTimeOutUp % 100) / 10;
        int low = m_nTimeOutUp % 10;

        BitBlt(hdc, m_rcClockUp.left + GMW_X_CLOCK_DIGITAL, m_rcClockUp.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, high * CX_CLOCK_DIGITAL, 0, SRCCOPY);

        BitBlt(hdc, m_rcClockUp.left + GMW_X_CLOCK_DIGITAL + CX_CLOCK_DIGITAL, m_rcClockUp.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, low * CX_CLOCK_DIGITAL, 0, SRCCOPY);
    }

    if (m_bShowClockCur == TRUE) {
        DrawIconEx(hdc, m_rcClockCur.left, m_rcClockCur.top, g_hiconClock, 
            CX_ICON_CLOCK, CY_ICON_CLOCK, NULL, NULL, DI_NORMAL);

        int high = (m_nTimeOutCur % 100) / 10;
        int low = m_nTimeOutCur % 10;

        BitBlt(hdc, m_rcClockCur.left + GMW_X_CLOCK_DIGITAL, m_rcClockCur.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, high * CX_CLOCK_DIGITAL, 0, SRCCOPY);

        BitBlt(hdc, m_rcClockCur.left + GMW_X_CLOCK_DIGITAL + CX_CLOCK_DIGITAL, m_rcClockCur.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, low * CX_CLOCK_DIGITAL, 0, SRCCOPY);
    }

    if (m_bShowClockDown == TRUE) {
        DrawIconEx(hdc, m_rcClockDown.left, m_rcClockDown.top, g_hiconClock, 
            CX_ICON_CLOCK, CY_ICON_CLOCK, NULL, NULL, DI_NORMAL);

        int high = (m_nTimeOutDown % 100) / 10;
        int low = m_nTimeOutDown % 10;

        BitBlt(hdc, m_rcClockDown.left + GMW_X_CLOCK_DIGITAL, m_rcClockDown.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, high * CX_CLOCK_DIGITAL, 0, SRCCOPY);

        BitBlt(hdc, m_rcClockDown.left + GMW_X_CLOCK_DIGITAL + CX_CLOCK_DIGITAL, m_rcClockDown.top + GMW_Y_CLOCK_DIGITAL,
            CX_CLOCK_DIGITAL, CY_CLOCK_DIGITAL, clockDC, low * CX_CLOCK_DIGITAL, 0, SRCCOPY);
    }

    SelectObject(clockDC, oldClockBmp);
    DeleteDC(clockDC);
}