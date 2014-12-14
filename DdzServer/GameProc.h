//
// GameProc.h
//
//  斗地主游戏逻辑处理
//
#pragma once

// 游戏工作线程消息，提供给连接线程发送消息到游戏工作线程
#define TM_RECEIVE_DATA                     (WM_USER + 101)
#define TM_GAMER_ENTER                      (WM_USER + 102)
#define TM_LOOKON_ENTER                     (WM_USER + 103)
#define TM_GAMER_LEAVE                      (WM_USER + 104)
#define TM_LOOKON_LEAVE                     (WM_USER + 105)
#define TM_GAMER_READY                      (WM_USER + 106)
#define TM_CONNECTION_LOST                  (WM_USER + 107)
#define TM_QUIT                             (WM_USER + 108)

// 游戏工作线程自己给自己寄送消息
#define TM_START_GAME                       (WM_USER + 201)
#define TM_REQ_VOTE_LORD                    (WM_USER + 202)
#define TM_REQ_OUTPUT_CARD                  (WM_USER + 203)


// 游戏中途逃跑则加一倍扣分
#define RUN_AWAY_MULTIPLE                   1

// 玩家手中的牌
typedef struct PLAYER_POKER_INFO_t {
    int         num;
    int         pokers[PLAYER_MAX_CARDS_NUM];
} PLAYER_POKER_INFO, *LPPLAYER_POKER_INFO;

// 游戏数据信息
typedef struct GAME_INFO_t {
    BOOL    bStartGame;         //标识游戏已经开始
    BOOL    bStartOuputCard;    //标识开始出牌
    int     nLordSeat;          //地主的椅子号
    int     nWinnerSeat;        //最后赢家的椅子号
    int     nLordScore;         //地主叫的分数（游戏底分）
    int     nLordOutputTimes;   //地主出牌的次数，如果为1，则地主输时，要翻一倍输（被关）
    int     nMultiple;          //倍数
    int     nActiveSeat;        //服务器等待叫地主或出牌的椅子号
    BOOL    bUpPlayerNotFollow; //上家没接牌
    int     nVoteLordFailTimes; //没人叫地主的次数，如果大于2，则不重新发牌，置玩家到就绪状态    
    BOOL    bReady[GAME_SEAT_NUM_PER_TABLE]; //准备就绪
    BOOL    bVoted[GAME_SEAT_NUM_PER_TABLE]; //是否叫过分数
    BOOL    bDelegated[GAME_SEAT_NUM_PER_TABLE]; //玩家是否选择了托管游戏
    BOOL    bConnectionLost[GAME_SEAT_NUM_PER_TABLE]; //该椅子的玩家是否已经掉线
    int     underPokerCards[UNDER_CARDS_NUM]; // 底牌
    PLAYER_POKER_INFO   pokerCards[GAME_SEAT_NUM_PER_TABLE]; //玩家手中的牌
    GAME_SEAT_INFO      seatInfo[GAME_SEAT_NUM_PER_TABLE]; //椅子对应的玩家及其旁观者
    POKER_PROPERTY      outputPokerProperty; // 保存玩家所打出牌的属性
} GAME_INFO, *LPGAME_INFO;


// 创建游戏处理线程
BOOL BeginGameThreads(void);

// 结束游戏线程
BOOL EndGameThreads(void);

// 游戏处理线程的入口函数
DWORD GameThreadProc(LPVOID lpData);

// 处理客户端发送过来的消息数据
static void OnReceiveData(int id, int len, BYTE* buf);

static void OnGamerEnter(int id, int table, int seat);
static void OnGamerLeave(int id, int table, int seat);
static void OnLookonEnter(int id, int table, int seat);
static void OnLookonLeave(int id, int table, int seat);
static void OnGamerReady(int id, int table, int seat);
static void OnConnectionLost(int id, int table, int seat);

static void StartGame(int table);
static void EndGame(int table);

static void InitPlayerPokerData(int table);
static void DistributePoker(int table, int ids[], int idcount);

static void ReqVoteLord(int table, int seat, int score);
static void VoteLordDelegated(int table, int seat);

static void OnAckVoteLord(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void VoteLordFailed(int table);
static void VoteLordSucceed(int table, int seat, int score);

static void BroadcastInTable(int table, char* buf, int len);
static void BroadcastInTableExceptId(int table, char* buf, int len, int id);

static void ReqOutputCard(int table, int seat, POKER_PROPERTY* pp);
static void OutputCardDelegated(int table, int seat, POKER_PROPERTY* req);

static void RemoveOutputPokersByIndex(int table, int seat, int indexes[], int num);
static void RemoveOutputPokers(int table, int seat, BYTE pokers[], int num);

static void GameOver(int table, int nWinnerSeat);
static void OnAckOutputCard(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnReqStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnAckStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnReqDelegate(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnReqForceStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnAllowLookon(int table, BYTE* pMsgBody, int nMsgBodyLen);
static void OnChatMessage(int table, BYTE* pMsgBody, int nMsgBodyLen);

