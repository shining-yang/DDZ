//
// NetProc.h
//
//
#pragma once

// 当客户端掉线或客户端主动断开连接时，连接线程自动退出
// 连接线程发送该消息到主线程，由主线程负责关闭该连接线程的句柄
#define TM_CLIENT_DISCONNECT                (WM_USER + 301)

// 服务器定时广播客户状态的时间间隔
#define BROADCAST_TIME_INTERVAL             1000

// 每个连接都预分配一个接收数据的缓冲区，为避免报文的丢失，暂定为4K
#define CONNECTION_RECV_BUF_SZ              4096

// 服务器的INI文件
#define SERVER_CONF_FILE                _T("DdzServer.ini")
#define SVR_INI_SEC_SERVER              _T("server")
#define SVR_INI_KEY_PORT                _T("port")


// 游戏桌号是否合法，解析消息时进行判断，其它情况可以不判断
#define IF_INVALID_TABLE_RET(table) \
    if (IS_INVALID_TABLE(table)) { \
        WriteLog(LOG_ERROR, _T("游戏桌号 [%d] 不合法，不处理该消息"), (table)); \
        return; \
    }

// 游戏桌内椅子号是否合法，解析消息时进行判断，其它情况可以不判断
#define IF_INVALID_SEAT_RET(seat) \
    if (IS_INVALID_SEAT(seat)) { \
        WriteLog(LOG_ERROR, _T("游戏椅子号 [%d] 不合法，不处理该消息"), (seat)); \
        return; \
    }

#define M_CONNECT_SOCK(id)              g_ConnectionInfo[id].sock
#define M_CONNECT_LOST(id)              g_ConnectionInfo[id].bConnLost
#define M_CONNECT_TIME(id)              g_ConnectionInfo[id].time
#define M_CONNECT_STATE(id)             g_ConnectionInfo[id].state
#define M_CONNECT_ALLOW_LOOKON(id)      g_ConnectionInfo[id].bAllowLookon
#define M_CONNECT_TABLE(id)             g_ConnectionInfo[id].table
#define M_CONNECT_SEAT(id)              g_ConnectionInfo[id].seat
#define M_CONNECT_PALYER_AVAILABLE(id)  g_ConnectionInfo[id].bPlayerInfoAvail
#define M_CONNECT_PLAYER_NAME(id)       g_ConnectionInfo[id].playerInfo.name
#define M_CONNECT_PLAYER_GENDER(id)     g_ConnectionInfo[id].playerInfo.gender
#define M_CONNECT_PLAYER_IMG_IDX(id)    g_ConnectionInfo[id].playerInfo.imgIndex
#define M_CONNECT_PLAYER_SCORE(id)      g_ConnectionInfo[id].playerInfo.nScore
#define M_CONNECT_PLAYER_TOTALGAMES(id) g_ConnectionInfo[id].playerInfo.nTotalGames
#define M_CONNECT_PLAYER_WINGAMES(id)   g_ConnectionInfo[id].playerInfo.nWinGames
#define M_CONNECT_PLAYER_RUNAWAY(id)    g_ConnectionInfo[id].playerInfo.nRunawayTimes

#define M_TABLE_PLAYER_ID(table, seat)  g_GameTableInfo[table].seatInfo[seat].playerid
#define M_TABLE_LOOKON_ID(table, seat)  g_GameTableInfo[table].seatInfo[seat].lookonid

// ID 资源，用1-bit表示一个ID
typedef struct CONNECTION_ID_RES_t {
    BYTE idResource[(MAX_CONNECTION_NUM + 7) / 8];
} CONNECTION_ID_RES, *LPCONNECTION_ID_RES;


// 客户端玩家的基本信息
typedef struct PLAYER_BASIC_INFO_t {
    TCHAR               name[MAX_USER_NAME_LEN];    // 昵称
    PLAYER_GENDER       gender;                     // 性别
    int                 imgIndex;                   // 头象图片索引

    int                 nScore;         // 积分
    int                 nTotalGames;    // 总游戏局数
    int                 nWinGames;      // 赢的局数
    int                 nRunawayTimes;  // 逃跑次数
} PLAYER_BASIC_INFO, *LPPLAYER_BASIC_INFO;

// 客户端连接信息
typedef struct CONNECTION_INFO_t {
    SOCKET              sock;                   // 客户端连接的套接字
    BOOL                bConnLost;              // 服务器记录该客户端是否已经掉线
    BOOL                bPlayerInfoAvail;       // 标识该客户端玩家的信息是否可用
    SYSTEMTIME          time;                   // 连接建立时间
    PLAYER_BASIC_INFO   playerInfo;             // 游戏玩家基本信息
    PLAYER_STATE        state;                  // 玩家状态
    BOOL                bAllowLookon;           // 不允许旁观，默认为1（TRUE），表示允许旁观
    int                 table;                  // 玩家所在桌子号
    int                 seat;                   // 玩家所在座位（椅子）号
} CONNECTION_INFO, *LPCONNECTION_INFO;

// 游戏桌中的椅子信息
typedef struct GAME_SEAT_INFO_t {
    int playerid;                               // 游戏玩家的ID
    int lookonid[MAX_LOOKON_NUM_PER_SEAT];      // 该游戏玩家的旁观者ID
} GAME_SEAT_INFO, *LPGAME_SEAT_INFO;

// 游戏桌信息
typedef struct GAME_TABLE_INFO_t {
    GAME_SEAT_INFO      seatInfo[GAME_SEAT_NUM_PER_TABLE]; // 三张椅子
    //DWORD               dwThreadId;             // 对应的游戏工作线程ID
    //BOOL                bAllowLookon;           // 允许旁观
    //BOOL                bHasPassword;           // 进入时需要密码
    //TCHAR               password[6];            // 密码
} GAME_TABLE_INFO, *LPGAME_TABLE_INFO;

// 描述客户端进入或离开房间
typedef struct USER_STATE_CHANGE_t {
    int                                 id;     // 客户端的唯一ID
    STATE_CHANGE_EVENT                  evt;    // 玩家状态改变事件类型，定义放在 MsgDef.h
    struct USER_STATE_CHANGE_t*         next;   // 链表的下个结点

    PLAYER_STATE                        newState; // 改变之后的状态，仅适用于 EVT_STATE_CHG
    BOOL                                bContainStatistics; // 是否需要携带玩家统计信息，仅适用于 EVT_STATE_CHG
} USER_STATE_CHANGE, *LPUSER_STATE_CHANGE;

// 链表结构，记录玩家进入房间、离开房间、状态发生改变
typedef struct USER_STATE_CHANGE_LIST_t {
    USER_STATE_CHANGE*          lpFirstNode;    // 首结点
    USER_STATE_CHANGE*          lpLastNode;     // 尾结点
} USER_STATE_CHANGE_LIST, *LPUSER_STATE_CHANGE_LIST;

// 线游戏工作线程提供的外部全局变量声明
extern volatile int g_nCurConnectionNum;
extern CRITICAL_SECTION g_csConnectionNum;
extern CONNECTION_INFO g_ConnectionInfo[];
extern GAME_TABLE_INFO g_GameTableInfo[];


// 服务器网络功能
BOOL StartNetService(void);
void StopNetService(void);

void CloseConnectionThreadHandle(HANDLE hThread);

static void ReleaseNetServiceRes(void);
static BOOL InitNetServiceRes(void);
static BOOL InitWinSocket(void);

static BOOL InitServerIP(void);
static BOOL InitServerPort(void);
static void WriteServerPortToIni(void);

void GetHostIP(DWORD ipArray[], int num, int* count);

// IP address selection dialog procedure
INT_PTR CALLBACK IpSelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR FinishIpSel(HWND hDlg, int iItem);
INT_PTR IpListNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);
INT_PTR InitIpSelDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);

// 等待连接线程的入口函数
DWORD WaitForConnectionThreadProc(LPVOID lpData);

// 连接线程的入口函数
DWORD ConnectionThreadProc(LPVOID lpData);

// 定时广播线程的入口函数
DWORD OnTimeBroadcastThreadProc(LPVOID lpData);

// 向客户端发送数据的外部接口
int SendData(int id, char* buf, int len);

int AllocID(void);
void DeallocID(int id);

void AddUserStateChangeEvent(int id, STATE_CHANGE_EVENT evt, PLAYER_STATE newState, BOOL bContainStat);

static void SendRoomInfo(int nAllocID);
static void ExchangeInfoInTable(int myid, int table, int seat, BYTE msgcode);
static void BroadcastInfoInTable(int myid, int table, int seat, BYTE msgcode);

// 定时构造玩家状态变化的消息，并广播出去
static void OnTimeBuildMessageAndBroadcast(char* mem, int len);

// 响应接收到的消息
static void OnMessage(int id, char* msg, int len);

static void OnMsgReqRoomInfo(int id, char* msg, int len);
static void OnMsgReqGamerTakeSeat(int id, char* msg, int len);
static void OnMsgReqGamerLeaveSeat(int id, char* msg, int len);
static void OnMsgReqLookonTakeSeat(int id, char* msg, int len);
static void OnMsgReqLookonLeaveSeat(int id, char* msg, int len);
static void OnMsgReqGamerReady(int id, char* msg, int len);

// 将接收到的消息转发给游戏工作线程
static void PostMsgDataToGameThread(int id, char* buf, int len);

