//
// File: GameMainWnd.h
//
#pragma once

#define GAME_MAIN_WND_CLASS_NAME        _T("_GMW_CLASS_")

#define ID_BTN_SCORE0                   1000
#define ID_BTN_SCORE1                   1001
#define ID_BTN_SCORE2                   1002
#define ID_BTN_SCORE3                   1003
#define ID_BTN_START                    1004
#define ID_BTN_OPTION                   1005
#define ID_BTN_DELEGATE                 1006
#define ID_BTN_PREVROUND                1007
#define ID_BTN_PASS                     1008
#define ID_BTN_FOLLOW                   1009
#define ID_BTN_HINT                     1010

#define ID_CUR_POKER_WND                1011
#define ID_CUR_POKER_WND_UP             1012
#define ID_CUR_POKER_WND_DOWN           1013

#define GMW_BTN_WIDTH                   59
#define GMW_BTN_HEIGHT                  25

#define GMW_START_BTN_WIDTH             60
#define GMW_START_BTN_HEIGHT            32

#define GMW_BTN_H_GAP                   8
#define GMW_BTN_V_GAP                   4

#define GMW_DEF_BTN_MASK_CLR            RGB(255,255,255)
#define GMW_TEXT_CLR                    RGB(255,255,255)

#define GMW_LOGCLR_INVALID_OP           RGB(255,0,0)
#define GMW_LOGCLR_GAME_RESULT          RGB(0,255,0)
#define GMW_LOGCLR_NORMAL               RGB(255,255,255)
#define GMW_LOGCLR_SPECIAL              RGB(0,255,128)
#define GMW_LOGCLR_DELEGATE             RGB(255,192,0)
#define GMW_LOGCLR_DISCONNECT           RGB(255,0,0)
#define GMW_LOGCLR_CHAT                 RGB(0,0,0)

#define GMW_CX_LORD_SCORE               60
#define GMW_CY_LORD_SCORE               20

// 显示上一轮出牌的定时器
#define ID_TIMER_SHOW_PREVROUND         100
#define SHOW_PREVROUND_TIMEOUT          1500

// 显示时钟
#define ID_TIMER_SHOW_CLOCK             101
#define SHOW_CLOCK_TIMEOUT              1000

// 时钟定时秒数
#define GMW_TIMEOUT_SECOND              30

// 时钟报警时的秒数
#define GMW_TIMEOUT_SECOND_ALARM        10

// 允许连续超时的次数，超过该过次将托管游戏
#define GMW_MAX_TIMEOUT_TIMES           2

// 绘制时钟数字起始点（时钟ICON为64x64的，数字为13x23的）
#define GMW_X_CLOCK_DIGITAL             20
#define GMW_Y_CLOCK_DIGITAL             23


class GameMainWnd
{
    HWND        m_hWnd;
    HWND        m_hWndParent;
    HINSTANCE   m_hInstance;

    HBITMAP     m_hBackBitmap;
    int         m_cxBackBitmap;
    int         m_cyBackBitmap;

    BOOL        m_bShow;

    // 子窗口界面元素
    MyButton    m_btnStart;     // 准备就绪（开始游戏）

    MyButton    m_btnScore0;    // 不叫
    MyButton    m_btnScore1;    // 1分
    MyButton    m_btnScore2;    // 2分
    MyButton    m_btnScore3;    // 3分

    MyButton    m_btnPass;      // 不出
    MyButton    m_btnFollow;    // 出牌
    MyButton    m_btnHint;      // 提示

    MyButton    m_btnOption;    // 设置选项
    MyButton    m_btnDelegate;  // 托管游戏
    MyButton    m_btnPrevRound; // 上一轮出牌

    UnderCardWnd    m_UnderCardWnd;     // 底牌窗口

    CurPokerWnd     m_CurPokerWnd;      // 当前玩家的扑克牌窗口
    CurPokerWnd     m_CurPokerWndUp;    // 上家扑克牌窗口
    CurPokerWnd     m_CurPokerWndDown;  // 下家扑克牌窗口

    OutCardWnd      m_OutCardWndCur;    // 当前玩家的出牌窗口
    OutCardWnd      m_OutCardWndUp;     // 上家出牌窗口
    OutCardWnd      m_OutCardWndDown;   // 下家出牌窗口

    GamerVisualWnd  m_GamerVisualWndUp;     // 上家玩家大头像
    GamerVisualWnd  m_GamerVisualWndCur;    // 当前玩家大头像
    GamerVisualWnd  m_GamerVisualWndDown;   // 下家玩家大头像

    BOOL            m_bIsLookon;    // 记录自己是否为旁观者。进入游戏主窗口时的Init()设置
#define I_AM_LOOKON (m_bIsLookon == TRUE)

    int             m_nCurUserSeat; // 当前玩家或旁观者所在的椅子号
#define CUR_SEAT_NUM    m_nCurUserSeat
#define DOWN_SEAT_NUM   ((m_nCurUserSeat + 1) % GAME_SEAT_NUM_PER_TABLE)
#define UP_SEAT_NUM     ((m_nCurUserSeat + 2) % GAME_SEAT_NUM_PER_TABLE)

#define IS_CUR_SEAT(seat)       ((seat) == CUR_SEAT_NUM)
#define IS_DOWN_SEAT(seat)      ((seat) == DOWN_SEAT_NUM)
#define IS_UP_SEAT(seat)        ((seat) == UP_SEAT_NUM)

    // 标识是否已经开始游戏。
    // 定义发牌后为游戏开始，即为TRUE；GameOver或无人叫地主，或其它中途中止
    // 游戏的均视为游戏未开始，即为FALSE。
    BOOL    m_bGameStarted;

    // 标识是否已经开始出牌。
    // 定义地主产生之后即为开始出牌，即为TRUE；GameOver之后为FALSE。
    BOOL    m_bOutputCardStarted;

    BOOL    m_bNoGamerVoteLord;         // 记录本局游戏是否没有人叫地主
    int     m_nNoGamerVoteLordTimes;    // 记录这是连续第几次没有玩家叫地主

    int     m_nBombNum; // 游戏过程中出现的炸弹次数

    BOOL    m_bReqOutputCard;       // 服务器请求我出牌
    POKER_PROPERTY  m_PokerProp;    // 记录服务器请求我出牌的类型

    int     m_nHintTimes;       // 记录当前是第几次请求提示，出牌后或不出，则复位为0
    POKER_CLASS_TABLE m_pct;    // 扑克牌分类表，用于构造提示

    int     m_nLordSeat;    // 记录地主的椅子号
    int     m_nLordScore;   // 记录地主叫的分

    BOOL    m_bGamerDelegated[GAME_SEAT_NUM_PER_TABLE]; // 玩家是否托管游戏
    BOOL    m_bGamerConnLost[GAME_SEAT_NUM_PER_TABLE];  // 玩家是否掉线

    // 地主头像的矩形
    RECT    m_rcLordCur;
    RECT    m_rcLordUp;
    RECT    m_rcLordDown;
    
    // 托管图标的矩形
    RECT    m_rcDelegatedCur;
    RECT    m_rcDelegatedUp;
    RECT    m_rcDelegatedDown;
    
    // 掉线图标的矩形
    RECT    m_rcConnLostCur;
    RECT    m_rcConnLostUp;
    RECT    m_rcConnLostDown;

    // 定时钟表的矩形
    RECT    m_rcClockCur;
    RECT    m_rcClockUp;
    RECT    m_rcClockDown;

    int     m_nTimeOutCur;
    int     m_nTimeOutUp;
    int     m_nTimeOutDown;

    BOOL    m_bShowClockCur;
    BOOL    m_bShowClockUp;
    BOOL    m_bShowClockDown;

    // 当前玩家在叫地主或出牌期间，连续超时的次数
    int     m_nTimeOutTimes;

    // 判断上、下家是否已经准备就绪，用于游戏结束之后，是否擦除其出牌窗口内容
    BOOL    m_bGamerReadyUp;
    BOOL    m_bGamerReadyDown;

public:
    GameMainWnd(void);
    ~GameMainWnd(void);

    static ATOM GameMainWndRegister(HINSTANCE hInstance);
    static LRESULT CALLBACK GameMainWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void CreateChildWindows(void);

    void DrawIconLord(HDC hdc);
    void DrawIconConnLost(HDC hdc);
    void DrawIconDelegated(HDC hdc);
    void DrawIconBomb(HDC hdc);
    void DrawLordScoreString(HDC hdc);
    void DrawClock(HDC hdc);

    void OnPaint(HDC hdc);
    void OnSize(WPARAM wParam, LPARAM lParam);

    void OnTimer(WPARAM wParam, LPARAM lParam);
    void OnLButtonDblClk(WPARAM wParam, LPARAM lParam);
    void OnRButtonDown(WPARAM wParam, LPARAM lParam);

    void OnTimerClock(void);

    void OnCommand(UINT nCtrlId);
    void On_ID_BTN_START(void);
    void On_ID_BTN_SCORE0(void);
    void On_ID_BTN_SCORE1(void);
    void On_ID_BTN_SCORE2(void);
    void On_ID_BTN_SCORE3(void);
    void On_ID_BTN_PASS(void);
    void On_ID_BTN_FOLLOW(void);
    void On_ID_BTN_HINT(void);
    void On_ID_BTN_OPTION(void);
    void On_ID_BTN_DELEGATE(void);
    void On_ID_BTN_PREVROUND(void);

    void ShowVoteLordButtons(int nCurScore, BOOL bShow = TRUE);
    void ShowOutCardButtons(BOOL bFirstOutput, BOOL bShow = TRUE);

    void WriteLog(LPCTSTR lpszText, COLORREF clrText = RGB(0,0,0));
    void RedrawLordScoreRect(void);
    void ClearLostAndDelegatedIcon(void);

public:
    HWND Create(int x, int y, int cx, int cy, HWND hWndParent, HINSTANCE hInstance);
    BOOL SetBackBitmap(HBITMAP hBmp);
    void SetWindowRect(int x, int y, int cx, int cy);
    void Show(BOOL bShow = TRUE);
    BOOL IsVisible(void);

    void Init(BOOL bIsLookon);
    void SetCurrentUserSeat(int seat);

    void GamerTakeSeat(int id, int seat);
    void GamerLeaveSeat(int id, int seat);
    void GamerReady(int id, int seat);

    void GamerCanStart(void);

    void DistributePokerCards(int seat, int poker[], int num);
    void SetUnderPokerCards(int poker[], int num);

    void ReqVoteLord(int seat, int score); // 服务器请求客户端叫分
    void GamerVoteLord(int seat, int score); // 服务器通知客户端，某玩家叫几分
    void VoteLordFinish(BOOL bVoteSucceed, int nLordSeat, int nLordScore);
    void ReqOutputCard(int seat, POKER_PROPERTY* pp);
    void OutputCard(int seat, int num, int poker[]);
    void GameOver(GAME_RESULT* lpResult);

    void OutputChatText(int seat, int nMsgIndex);
    void OutputChatText(int seat, LPCTSTR lpszChatText);

    void GamerDelegate(int seat, BOOL bDelegated);
    void GamerDisconnected(int seat);
};