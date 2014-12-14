//
// GameProc.cpp
//
//  斗地主游戏服务器逻辑
//

#include "stdafx.h"
#include "QuickSort.h"
#include "PokerAlgor.h"
#include "Crc.h"
#include "NetProc.h"
#include "GameProc.h"

extern POKER_ALOGRITHM g_PokerAlgorithm[];
extern volatile int g_nCurPokerAlgorithm;

// 分配一个TLS索引
DWORD g_dwGameTlsIndex = 0;

// 游戏工作线程句柄及ID，一张游戏桌对应一个游戏工作线程
HANDLE g_hGameThreads[GAME_TABLE_NUM] = { 0 };
DWORD g_nGameThreadIds[GAME_TABLE_NUM] = { 0 };

// 游戏数据信息，每个游戏工作线程使用其中一个
GAME_INFO g_GameInfo[GAME_TABLE_NUM] = { 0 };

#define M_GAME_STARTED(table)                   g_GameInfo[table].bStartGame
#define M_GAME_OUTPUT_CARD_STARTED(table)       g_GameInfo[table].bStartOuputCard
#define M_GAME_MULTIPLE(table)                  g_GameInfo[table].nMultiple
#define M_GAME_LORD_SEAT(table)                 g_GameInfo[table].nLordSeat
#define M_GAME_LORD_SCORE(table)                g_GameInfo[table].nLordScore
#define M_GAME_WINNER_SEAT(table)               g_GameInfo[table].nWinnerSeat
#define M_GAME_UP_NOT_FOLLOW(table)             g_GameInfo[table].bUpPlayerNotFollow
#define M_GAME_LORD_OUTPUT_TIMES(table)         g_GameInfo[table].nLordOutputTimes
#define M_GAME_VOTE_LORD_FAIL_TIMES(table)      g_GameInfo[table].nVoteLordFailTimes
#define M_GAME_ACTIVE_SEAT(table)               g_GameInfo[table].nActiveSeat
#define M_GAME_CONNECTION_LOST(table, seat)     g_GameInfo[table].bConnectionLost[seat]
#define M_GAME_READY(table, seat)               g_GameInfo[table].bReady[seat]
#define M_GAME_VOTED(table, seat)               g_GameInfo[table].bVoted[seat]
#define M_GAME_DELEGATED(table,seat)            g_GameInfo[table].bDelegated[seat]
#define M_GAME_UNDER_POKER_CARDS(table)         g_GameInfo[table].underPokerCards
#define M_GAME_PLAYER_POKER_NUM(table, seat)    g_GameInfo[table].pokerCards[seat].num
#define M_GAME_PLAYER_POKERS(table, seat)       g_GameInfo[table].pokerCards[seat].pokers
#define M_GAME_PLAYER_ID(table, seat)           g_GameInfo[table].seatInfo[seat].playerid
#define M_GAME_LOOKON_ID(table, seat)           g_GameInfo[table].seatInfo[seat].lookonid
#define M_GAME_OUTPUT_POKER_TYPE(table)         g_GameInfo[table].outputPokerProperty.type
#define M_GAME_OUTPUT_POKER_VALUE(table)        g_GameInfo[table].outputPokerProperty.value
#define M_GAME_OUTPUT_POKER_NUM(table)          g_GameInfo[table].outputPokerProperty.num
#define M_GAME_SAVE_POKER_PROPERTY(table, _t, _v, _n) \
{\
    M_GAME_OUTPUT_POKER_TYPE(table) = (POKER_TYPE)_t;\
    M_GAME_OUTPUT_POKER_VALUE(table) = _v;\
    M_GAME_OUTPUT_POKER_NUM(table) = _n;\
}


// 创建游戏处理线程
BOOL BeginGameThreads(void)
{
    // Set Low-Fragmentation Heap
    ULONG  HeapFragValue = 2;
    HeapSetInformation(GetProcessHeap(),
        HeapCompatibilityInformation,
        &HeapFragValue,
        sizeof(HeapFragValue));

    // 初始化游戏桌数据
    ZeroMemory(&g_GameInfo, sizeof(GAME_INFO) * GAME_TABLE_NUM);

    for (int table = 0; table < GAME_TABLE_NUM; table++) {
        M_GAME_LORD_SEAT(table) = INVALID_SEAT;
        M_GAME_WINNER_SEAT(table) = INVALID_SEAT;
        M_GAME_ACTIVE_SEAT(table) = INVALID_SEAT;

        for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
            M_GAME_PLAYER_ID(table, seat) = INVALID_USER_ID;

            for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
                M_GAME_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
            }
        }
    }

    // 分配一个TLS索引，用于游戏工作线程保存桌子索引号，以便线程识别自己是第几桌的工作线程
    if ((g_dwGameTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
        return FALSE;
    }

    // 创建游戏工作线程，传递参数i给线程，表示该线程是第几桌的游戏工作线程
    for (int i = 0; i < sizeof(g_nGameThreadIds) / sizeof(g_nGameThreadIds[0]); i++) {
        g_hGameThreads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GameThreadProc,
            (LPVOID)i, 0, &g_nGameThreadIds[i]);

        if (g_hGameThreads[i] == NULL) {
            for (int j = 0; j < i; j++) {
                PostThreadMessage(g_nGameThreadIds[j], TM_QUIT, 0, 0);
            }

            for (int j = 0; j < i; j++) {
                WaitForSingleObject(g_hGameThreads[j], INFINITE);
                CloseHandle(g_hGameThreads[j]);
            }

            TlsFree(g_dwGameTlsIndex);

            return FALSE;
        }
    }

    return TRUE;
}

// 结束游戏线程
BOOL EndGameThreads(void)
{
    for (int i = 0;
        i < sizeof(g_nGameThreadIds) / sizeof(g_nGameThreadIds[0]);
        i++) {
            PostThreadMessage(g_nGameThreadIds[i], TM_QUIT, 0, 0);
    }

    for (int i = 0;
        i < sizeof(g_hGameThreads) / sizeof(g_hGameThreads[0]);
        i++) {
            WaitForSingleObject(g_hGameThreads[i], INFINITE);
            CloseHandle(g_hGameThreads[i]);
    }

    TlsFree(g_dwGameTlsIndex);

    return TRUE;
}

// 游戏处理线程的入口函数
// 该线程的工作是负责游戏桌的所有逻辑和会话
DWORD GameThreadProc(LPVOID lpData)
{
    UINT nMsg;
    WPARAM wParam;
    LPARAM lParam;
    int id;
    int seat;
    int len;
    BYTE* data;

    // 创建线程时，传递的参数是游戏桌的索引号，即表示本线程是第几桌的游戏工作线程。
    // 将其保存在线程的TLS中，以便随时可以取出来使用

    int table = int(lpData);
    if (TlsSetValue(g_dwGameTlsIndex, (LPVOID)table) == 0) {
        return 1;
    }

    int ret;
    MSG msg;

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) { continue; }

        nMsg = msg.message;
        wParam = msg.wParam;
        lParam = msg.lParam;

        switch (nMsg) {
            case TM_GAMER_ENTER:
                id = (int)wParam;
                seat = (int)lParam;
                OnGamerEnter(id, table, seat);
                break;

            case TM_GAMER_LEAVE:
                id = (int)wParam;
                seat = (int)lParam;
                OnGamerLeave(id, table, seat);
                break;

            case TM_LOOKON_ENTER:
                id = (int)wParam;
                seat = (int)lParam;
                OnLookonEnter(id, table, seat);
                break;

            case TM_LOOKON_LEAVE:
                id = (int)wParam;
                seat = (int)lParam;
                OnLookonLeave(id, table, seat);
                break;

            case TM_GAMER_READY:
                id = (int)wParam;
                seat = (int)lParam;
                OnGamerReady(id, table, seat);
                break;

            case TM_CONNECTION_LOST:
                id = (int)wParam;
                seat = (int)lParam;
                OnConnectionLost(id, table, seat);
                break;

            case TM_RECEIVE_DATA:
                id = HIWORD(wParam);
                len = LOWORD(wParam);
                data = (BYTE*)lParam;
                OnReceiveData(id, len , data);
                HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
                break;

            case TM_QUIT:
                //PostQuitMessage(0);
                //break;
                return 0;

                //
                // 以下消息为本线程自己寄送给自己的
                // 这样处理是为了避免游戏过程中所有玩家都托管游戏而出现递归出牌，
                // 而无法及时处理其它消息
                //
            case TM_START_GAME:
                StartGame(table);
                break;

            case TM_REQ_VOTE_LORD:
                ReqVoteLord(table, (int)wParam, (int)lParam);
                break;

            case TM_REQ_OUTPUT_CARD:
                {
                    POKER_PROPERTY pp;
                    int req_seat = LOWORD(wParam);
                    pp.type = (POKER_TYPE)HIWORD(wParam);
                    pp.value = LOWORD(lParam);
                    pp.num = HIWORD(lParam);
                    ReqOutputCard(table, req_seat, &pp);
                }
                break;
        }
    }

    return 0;
}

// 分析并处理接收到的数据
static void OnReceiveData(int id, int len, BYTE* buf)
{
    UNREFERENCED_PARAMETER(id);

    // 程序到此，已经识别出一个合法的消息。根据消息类型，只处理消息体。
    int nMsgCode = buf[0];
    int nMsgBodyLen = len - 3;
    BYTE* pMsgBody = buf + 3;

    // 获取本线程所在的游戏桌号
    int table = (int)TlsGetValue(g_dwGameTlsIndex);

    switch (nMsgCode) {
        case NMSG_ACK_VOTE_LORD:
            OnAckVoteLord(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_ACK_OUTPUT_CARD:
            OnAckOutputCard(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_REQ_STOP_GAMING:
            OnReqStopGaming(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_ACK_STOP_GAMING:
            OnAckStopGaming(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_REQ_DELEGATE:
            OnReqDelegate(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_REQ_FORCE_STOP_GAMING:
            OnReqForceStopGaming(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_ALLOW_LOOKON:
            OnAllowLookon(table, pMsgBody, nMsgBodyLen);
            break;

        case NMSG_CHATTING:
            OnChatMessage(table, pMsgBody, nMsgBodyLen);
            break;

        default:
            break;
    }
}

// 玩家进入游戏桌
static void OnGamerEnter(int id, int table, int seat)
{
    M_GAME_PLAYER_ID(table, seat) = id;

    // 清空该座位旁观者ID，旁观者在玩家坐好之后才能进入旁观
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        M_GAME_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
    }

    M_GAME_READY(table, seat) = FALSE;
    M_GAME_VOTED(table, seat) = FALSE;
    M_GAME_DELEGATED(table, seat) = FALSE;
    M_GAME_PLAYER_POKER_NUM(table, seat) = 0;
    M_GAME_CONNECTION_LOST(table, seat) = FALSE;
    M_GAME_WINNER_SEAT(table) = INVALID_SEAT;
}

// 玩家离开游戏桌
static void OnGamerLeave(int id, int table, int seat)
{
    UNREFERENCED_PARAMETER(id);

    M_GAME_PLAYER_ID(table, seat) = INVALID_USER_ID;

    M_GAME_READY(table, seat) = FALSE;
    M_GAME_VOTED(table, seat) = FALSE;
    M_GAME_DELEGATED(table, seat) = FALSE;
    M_GAME_PLAYER_POKER_NUM(table, seat) = 0;
    M_GAME_CONNECTION_LOST(table, seat) = FALSE;
    M_GAME_WINNER_SEAT(table) = INVALID_SEAT;
}

// 旁观者进入游戏桌
static void OnLookonEnter(int id, int table, int seat)
{
    // First, distribute poker to him if game started
    if (M_GAME_STARTED(table) == TRUE) {
        
        for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) { // who's delegated.
            if (M_GAME_DELEGATED(table, i) == TRUE) {
                BYTE data[16] = { 0 };
                data[0] = NMSG_ACK_DELEGATE;
                data[1] = 0;
                data[2] = 2;

                data[3] = (BYTE)i;
                data[4] = 1;

                WORD crc = CRC16(data, 5);
                data[5] = (BYTE)(crc >> 8);
                data[6] = (BYTE)(crc >> 0);

                SendData(id, (char*)data, 7);
            }
        }

        if (M_GAME_OUTPUT_CARD_STARTED(table) == TRUE) { // who's the lord.
            BYTE data[16] = { 0 };
            data[0] = NMSG_VOTE_LORD_FINISH;
            data[1] = 0;
            data[2] = 3;

            data[3] = (BYTE)TRUE;
            data[4] = (BYTE)M_GAME_LORD_SEAT(table);
            data[5] = (BYTE)M_GAME_LORD_SCORE(table);

            WORD crc = CRC16(data, 6);
            data[6] = (BYTE)(crc >> 8);
            data[7] = (BYTE)(crc >> 0);

            SendData(id, (char*)data, 8);
        }

        // distribute poker
        int ids[1];
        ids[0] = id;
        DistributePoker(table, ids, 1);
    }

    // Second, add user id to GAME data structure so that it can receive messages
    // within the game table
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (IS_INVALID_USER_ID(M_GAME_LOOKON_ID(table, seat)[i])) {
            M_GAME_LOOKON_ID(table, seat)[i] = id;
            break;
        }
    }
}

// 旁观者离开游戏桌
static void OnLookonLeave(int id, int table, int seat)
{
    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (M_GAME_LOOKON_ID(table, seat)[i] == id) {
            M_GAME_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
            break;
        }
    }
}

// 玩家准备游戏
static void OnGamerReady(int id, int table, int seat)
{
    UNREFERENCED_PARAMETER(id);

    M_GAME_READY(table, seat) = TRUE;

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        if (M_GAME_READY(table, i) != TRUE) {
            return;
        }
    }

    // TODO:
    int playerid;
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        playerid = M_GAME_PLAYER_ID(table, i);
        M_CONNECT_STATE(playerid) = STATE_GAMING;
        AddUserStateChangeEvent(playerid, EVT_CHGSTATE, STATE_GAMING, FALSE);
    }

    PostMessage(NULL, TM_START_GAME, 0, 0);
}

// 玩家或旁观者意外掉线
// 游戏工作线程收到此消息，是因为网络掉线时，客户端正在游戏桌中。
// 若客户端是旁观者，则在游戏桌内广播通知其离开；
// 若客户端是游戏者，并且正在进行游戏，则将其托管处理，并通知游戏桌成员，该玩家掉线，
// 否则，若还没开始游戏，则仅告知其他人，本玩家已离开。
static void OnConnectionLost(int id, int table, int seat)
{
    // 旁观者掉线。则通知游戏桌成员，该旁观者离开
    if (M_CONNECT_STATE(id) == STATE_LOOKON) {
        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            if (id == M_GAME_LOOKON_ID(table, seat)[i]) {
                M_GAME_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
                break;
            }
        }

        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            if (id == M_TABLE_LOOKON_ID(table, seat)[i]) {
                M_TABLE_LOOKON_ID(table, seat)[i] = INVALID_USER_ID;
                break;
            }
        }

        BYTE data[16];
        data[0] = NMSG_ACK_LOOKON_LEAVE_SEAT;

        data[1] = 0;
        data[2] = 4;

        data[3] = (BYTE)table;
        data[4] = (BYTE)seat;
        data[5] = (BYTE)(id >> 8);
        data[6] = (BYTE)(id >> 0);

        WORD crc = CRC16(data, 7);
        data[7] = (BYTE)(crc >> 8);
        data[8] = (BYTE)(crc >> 0);
        BroadcastInTableExceptId(table, (char*)data, 9, id);

        // 离开大厅
        WriteLog(LOG_INFO, _T("ID: %d 将被回收。[旁观者掉线]"), id);

        AddUserStateChangeEvent(id, EVT_LEAVE, (PLAYER_STATE)0, FALSE);
    } else {
        if (M_GAME_STARTED(table) == FALSE) { // 玩家掉线，但游戏没有开始。告知其他成员，该玩家离开座位
            M_GAME_PLAYER_ID(table, seat) = INVALID_USER_ID;

            M_TABLE_PLAYER_ID(table, seat) = INVALID_USER_ID;

            BYTE data[8];
            data[0] = NMSG_ACK_GAMER_LEAVE_SEAT;
            data[1] = 0;
            data[2] = 2;

            data[3] = (BYTE)table;
            data[4] = (BYTE)seat;

            WORD crc = CRC16(data, 5);
            data[5] = (BYTE)(crc >> 8);
            data[6] = (BYTE)(crc >> 0);

            BroadcastInTableExceptId(table, (char*)data, 7, id);

            // 离开大厅
            WriteLog(LOG_INFO, _T("ID: %d 将被回收。[玩家掉线，并且游戏尚未开始]"), id);

            AddUserStateChangeEvent(id, EVT_LEAVE, (PLAYER_STATE)0, FALSE);
        } else {
            // 玩家掉线，并且游戏已经开始。通知其他成员，该玩家掉线
            BYTE data[8];
            data[0] = NMSG_CONNECTION_LOST;
            data[1] = 0;
            data[2] = 1;

            data[3] = (BYTE)seat;

            WORD crc = CRC16(data, 4);
            data[4] = (BYTE)(crc >> 8);
            data[5] = (BYTE)(crc >> 0);

            BroadcastInTableExceptId(table, (char*)data, 6, id);

            // 记录该玩家已经掉线，当游戏结束时，需要进行相关清理工作
            M_GAME_CONNECTION_LOST(table, seat) = TRUE;

            // 如果所有玩家都掉线，则直接结束游戏
            BOOL bAllGamerLost = TRUE;
            for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
                if (M_GAME_CONNECTION_LOST(table, i) == FALSE) {
                    bAllGamerLost = FALSE;
                    break;
                }
            }

            if (bAllGamerLost == TRUE) {
                EndGame(table);
                return;
            }

            // 没有托管，则将其托管
            if (M_GAME_DELEGATED(table, seat) == FALSE) {
                M_GAME_DELEGATED(table, seat) = TRUE;

                BYTE data[8];
                data[0] = NMSG_ACK_DELEGATE;
                data[1] = 0;
                data[2] = 2;

                data[3] = (BYTE)seat;
                data[4] = 1; // TRUE

                WORD crc = CRC16(data, 5);
                data[5] = (BYTE)(crc >> 8);
                data[6] = (BYTE)(crc >> 0);

                BroadcastInTable(table, (char*)data, 7);

                // 掉线的玩家是当前服务器等待回复消息的玩家
                if (seat == M_GAME_ACTIVE_SEAT(table)) {
                    //
                    // 服务器寄送消息给自己作出响应，否则，服务器就会这样死等
                    //
                    if (M_GAME_OUTPUT_CARD_STARTED(table) == FALSE) { // 游戏已经开始，但还没开始出牌，正叫分中
                        PostMessage(NULL, TM_REQ_VOTE_LORD, (WPARAM)seat, (LPARAM)(M_GAME_LORD_SCORE(table)));
                    } else { // 已经开始出牌
                        WPARAM wp = MAKEWPARAM(seat, M_GAME_OUTPUT_POKER_TYPE(table));
                        LPARAM lp = MAKELPARAM(M_GAME_OUTPUT_POKER_VALUE(table), M_GAME_OUTPUT_POKER_NUM(table));
                        PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
                    }
                }
            }
        }
    }
}

// 游戏结束后，复位游戏桌相关数据
static void EndGame(int table)
{
    int id;

    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        id = M_GAME_PLAYER_ID(table, seat);

        if (M_GAME_CONNECTION_LOST(table, seat) == TRUE) {
            // 若有玩家离开，则清除上局游戏赢家的椅子号
            M_GAME_WINNER_SEAT(table) = INVALID_SEAT;

            M_GAME_PLAYER_ID(table, seat) = INVALID_USER_ID;

            M_TABLE_PLAYER_ID(table, seat) = INVALID_USER_ID;

            BYTE data[8];
            data[0] = NMSG_ACK_GAMER_LEAVE_SEAT;
            data[1] = 0;
            data[2] = 2;

            data[3] = (BYTE)table;
            data[4] = (BYTE)seat;

            WORD crc = CRC16(data, 5);
            data[5] = (BYTE)(crc >> 8);
            data[6] = (BYTE)(crc >> 0);

            BroadcastInTableExceptId(table, (char*)data, 7, id);

            // 离开大厅
            WriteLog(LOG_INFO, _T("ID: %d 将被回收。[玩家掉线，并且游戏已经结束]"), id);

            AddUserStateChangeEvent(id, EVT_LEAVE, (PLAYER_STATE)0, FALSE);
        } else {
            M_CONNECT_STATE(id) = STATE_SIT;
            AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_SIT, TRUE);
        }
    }

    M_GAME_STARTED(table) = FALSE;
    M_GAME_OUTPUT_CARD_STARTED(table) = FALSE;
    M_GAME_MULTIPLE(table) = 0;
    M_GAME_ACTIVE_SEAT(table) = INVALID_SEAT;
    M_GAME_LORD_SEAT(table) = INVALID_SEAT;
    M_GAME_LORD_SCORE(table) = 0;
    M_GAME_UP_NOT_FOLLOW(table) = FALSE;
    M_GAME_SAVE_POKER_PROPERTY(table, 0, 0, 0);
    M_GAME_LORD_OUTPUT_TIMES(table) = 0;
    M_GAME_VOTE_LORD_FAIL_TIMES(table) = 0;

    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        M_GAME_READY(table, seat) = FALSE;
        M_GAME_VOTED(table, seat) = FALSE;
        M_GAME_DELEGATED(table, seat) = FALSE;
        M_GAME_CONNECTION_LOST(table, seat) = FALSE;
        M_GAME_PLAYER_POKER_NUM(table, seat) = 0;
    }
}

// 服务器决定开始游戏
static void StartGame(int table)
{
    WriteLog(LOG_DEBUG, _T("第 %d 桌开始游戏"), table);

    // 初始化玩家的扑克牌（在服务器上保存数据）
    InitPlayerPokerData(table);

    // 收集需要接收数据的客户端ID
    int count = 0;
    int ids[(1 + MAX_LOOKON_NUM_PER_SEAT) * GAME_SEAT_NUM_PER_TABLE];

    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        ids[count++] = M_GAME_PLAYER_ID(table, seat);

        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            if (M_GAME_LOOKON_ID(table, seat)[i] != INVALID_USER_ID) {
                ids[count++] = M_GAME_LOOKON_ID(table, seat)[i];
            }
        }
    }

    // 发牌（发送到客户端）
    DistributePoker(table, ids, count);

    // 请求竞选地主
    PostMessage(NULL, TM_REQ_VOTE_LORD, (WPARAM)INVALID_SEAT, (LPARAM)0);

    M_GAME_STARTED(table) = TRUE;
}

// 初始化将要发送给玩家的牌
static void InitPlayerPokerData(int table)
{
    int poker[TOTAL_CARDS_NUM];

    // 洗牌
    (*g_PokerAlgorithm[g_nCurPokerAlgorithm].algorithm)(poker, TOTAL_CARDS_NUM);

    {//Debug...
        TCHAR text[256] = { 0 };
        int len = 0;
        for (int i = 0; i < TOTAL_CARDS_NUM; i++) {
            if (i % 17 == 0) {
                len += _stprintf_s(text + len, sizeof(text) / sizeof(text[0]) - len, _T("\r\n"));
            }
            len += _stprintf_s(text + len, sizeof(text) / sizeof(text[0]) - len, _T("%d "), poker[i]); 
        }
        WriteLog(LOG_DEBUG, text);
    }

    // 随机选开头三张或末尾三张作为底牌，并随机选择先给哪个玩家分牌
    srand((unsigned)time(NULL));
    int random_underpoker = rand() % 2;
    int random_player = rand() % GAME_SEAT_NUM_PER_TABLE;

    if (random_underpoker == 0) {
        for (int i = 0; i < UNDER_CARDS_NUM; i++) { // 取开头三张作为底牌
            M_GAME_UNDER_POKER_CARDS(table)[i] = poker[i];
        }

        for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
            M_GAME_PLAYER_POKER_NUM(table, seat) = PLAYER_INIT_CARDS_NUM;

            int start = UNDER_CARDS_NUM + random_player * PLAYER_INIT_CARDS_NUM;
            for (int i = 0; i < PLAYER_INIT_CARDS_NUM; i++) {
                M_GAME_PLAYER_POKERS(table, seat)[i] = poker[start + i];
            }

            random_player = (random_player + 1) % GAME_SEAT_NUM_PER_TABLE;
        }
    } else {
        for (int i = TOTAL_CARDS_NUM - UNDER_CARDS_NUM; i < TOTAL_CARDS_NUM; i++) { // 取末尾三张作为底牌
            M_GAME_UNDER_POKER_CARDS(table)[i - (TOTAL_CARDS_NUM - UNDER_CARDS_NUM)] = poker[i];
        }

        for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
            M_GAME_PLAYER_POKER_NUM(table, seat) = PLAYER_INIT_CARDS_NUM;

            int start = random_player * PLAYER_INIT_CARDS_NUM;
            for (int i = 0; i < PLAYER_INIT_CARDS_NUM; i++) {
                M_GAME_PLAYER_POKERS(table, seat)[i] = poker[start + i];
            }

            random_player = (random_player + 1) % GAME_SEAT_NUM_PER_TABLE;
        }
    }

    // 给玩家分好牌后，对牌排序
    quick_sort(M_GAME_UNDER_POKER_CARDS(table), 0, UNDER_CARDS_NUM - 1);

    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        quick_sort(M_GAME_PLAYER_POKERS(table, seat), 0, PLAYER_INIT_CARDS_NUM - 1);
    }
}

// 将服务器保存的玩家的牌发送到客户端玩家
static void DistributePoker(int table, int ids[], int idcount)
{
    BYTE data[80];
    int offset = 0;

    data[0] = NMSG_DISTRIBUTE_CARD;

    offset = 3;

    // 填充玩家的牌
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        data[offset] = (BYTE)M_GAME_PLAYER_POKER_NUM(table, seat); // 扑克牌张数
        offset++;

        for (int i = 0; i < M_GAME_PLAYER_POKER_NUM(table, seat); i++) { // 扑克牌数据
            data[offset] = (BYTE)M_GAME_PLAYER_POKERS(table, seat)[i];
            offset++;
        }
    }

    // 填充底牌
    data[offset] = (BYTE)UNDER_CARDS_NUM;
    offset++;
    for (int i = 0; i < UNDER_CARDS_NUM; i++) {
        data[offset] = (BYTE)M_GAME_UNDER_POKER_CARDS(table)[i];
        offset++;
    }

    data[1] = (BYTE)((offset - 3) >> 8);
    data[2] = (BYTE)((offset - 3) >> 0);

    // Append CRC
    WORD crc = CRC16(data, offset);
    data[offset] = (BYTE)(crc >> 8);
    data[offset + 1] = (BYTE)(crc >> 0);
    offset += 2;

    for (int i = 0; i < idcount; i++) {
        SendData(ids[i], (char*)data, offset);
    }
}

// 请求客户端竞选地主
static void ReqVoteLord(int table, int seat, int score)
{
    int req_seat = seat;
    int req_score = score;

    if (IS_INVALID_SEAT(req_seat)) { // 非法值表示首次叫地主
        if (IS_INVALID_SEAT(M_GAME_WINNER_SEAT(table))) { // 还没开始过游戏，即没有上局游戏的赢家
            req_seat = (rand() % 10) % 3;
            //req_seat = (int)((double)(rand()) / (double)(RAND_MAX + 1) * 3);

            WriteLog(LOG_DEBUG, _T("第 %d 桌竞选地主开始。随机选择 %d 椅子先叫地主"), table, req_seat);
        } else {
            req_seat = M_GAME_WINNER_SEAT(table);

            WriteLog(LOG_DEBUG, _T("第 %d 桌竞选地主开始。由上局游戏的赢家 [%d] 椅子先叫地主"), table, req_seat);
        }

        req_score = 0;
    }

    // 记录服务器请求叫分的椅子号
    M_GAME_ACTIVE_SEAT(table) = req_seat;

    if (M_GAME_DELEGATED(table, req_seat) == TRUE) {
        VoteLordDelegated(table, req_seat);
    } else {
        BYTE data[8];
        data[0] = NMSG_REQ_VOTE_LORD;
        data[1] = 0;
        data[2] = 2;
        data[3] = (BYTE)req_seat;
        data[4] = (BYTE)req_score;

        WORD crc;
        crc = CRC16(data, 5);
        data[5] = (BYTE)(crc >> 8);
        data[6] = (BYTE)(crc >> 0);

        BroadcastInTable(table, (char*)data, 7);
    }
}

// 托管游戏的玩家响应叫地主的事件
static void VoteLordDelegated(int table, int seat)
{
    // 记录当前玩家已经叫过分
    M_GAME_VOTED(table, seat) = TRUE;

    BYTE data[8];

    data[0] = NMSG_VOTE_LORD;
    data[1] = 0;
    data[2] = 2;
    data[3] = (BYTE)seat;
    data[4] = 0; // 不叫分

    WORD crc = CRC16(data, 5);
    data[5] = (BYTE)(crc >> 8);
    data[6] = (BYTE)(crc >> 0);

    // 通知所有玩家关于该托管玩家不叫分
    BroadcastInTable(table, (char*)data, 7);

    int next_seat = (seat + 1) % GAME_SEAT_NUM_PER_TABLE;
    int cur_score = M_GAME_LORD_SCORE(table);

    WriteLog(LOG_DEBUG, _T("第 %d 桌 %d 椅子玩家已经托管游戏，不叫分。当前已经叫分数: %d 分"),
        table, seat, cur_score);

    if (M_GAME_VOTED(table, next_seat) == FALSE) { // 下一个玩家还没叫分，则通知他叫分
        PostMessage(NULL, TM_REQ_VOTE_LORD, (WPARAM)next_seat, (LPARAM)cur_score);
    } else { // 三个玩家都叫过分数
        if (cur_score == 0) { // 三个玩家都叫0分
            VoteLordFailed(table);
        } else {
            VoteLordSucceed(table, M_GAME_LORD_SEAT(table), M_GAME_LORD_SCORE(table));
        }
    }
}

// 应答客户端竞选地主
static void OnAckVoteLord(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    //
    // 客户端软件需要进行控制，玩家只能叫比上家更高的分或不叫分
    //

    if (M_GAME_STARTED(table) == FALSE) {
        return;
    }

    if (nMsgBodyLen != 2) {
        WriteLog(LOG_ERROR, _T("NMSG_ACK_VOTE_LORD 消息结构不正确"));
        return;
    }

    int seat = pMsgBody[0];
    int score = pMsgBody[1];

    // 记录当前玩家已经叫过分
    M_GAME_VOTED(table, seat) = TRUE;

    // 复位服务器等待回应的玩家椅子号
    M_GAME_ACTIVE_SEAT(table) = INVALID_SEAT;

    // 如果有玩家叫分，则更新数据，暂定该玩家为地主
    if (M_GAME_LORD_SCORE(table) < score) {
        M_GAME_LORD_SCORE(table) = score;
        M_GAME_LORD_SEAT(table) = seat;
    }

    // 通知其它玩家，该玩家叫几分
    BYTE data[8];
    data[0] = NMSG_VOTE_LORD;
    data[1] = 0;
    data[2] = 2;
    data[3] = (BYTE)seat;
    data[4] = (BYTE)score;

    WORD crc = CRC16(data, 5);
    data[5] = (BYTE)(crc >> 8);
    data[6] = (BYTE)(crc >> 0);

    BroadcastInTable(table, (char*)data, 7);

    if (score == 3) { // 某玩家叫3分，则地主已经选出
        VoteLordSucceed(table, seat, score);
        return;
    }

    int next_seat = (seat + 1) % GAME_SEAT_NUM_PER_TABLE;

    if (M_GAME_VOTED(table, next_seat) == TRUE) { // 本玩家的下家已经叫过分数，说明3个玩家都叫过分数
        if (M_GAME_LORD_SCORE(table) == 0) { // 三个玩家都叫0分
            VoteLordFailed(table);
        } else {
            VoteLordSucceed(table, M_GAME_LORD_SEAT(table), M_GAME_LORD_SCORE(table));
        }
    } else { // 下家还没叫过分，请求下一个玩家叫分
        PostMessage(NULL, TM_REQ_VOTE_LORD, (WPARAM)next_seat, (LPARAM)M_GAME_LORD_SCORE(table));
    }
}

// 地主没有产生
static void VoteLordFailed(int table)
{
    BYTE data[8];

    data[0] = NMSG_VOTE_LORD_FINISH;

    data[1] = 0;
    data[2] = 1;

    data[3] = (BYTE)FALSE;

    WORD crc = CRC16(data, 4);
    data[4] = (BYTE)(crc >> 8);
    data[5] = (BYTE)(crc >> 0);

    BroadcastInTable(table, (char*)data, 6);

    WriteLog(LOG_DEBUG, _T("第 %d 桌竞选地主结束。没有玩家叫地主"), table);

    // 重新洗牌，发牌，叫分
    {//Debug...
        Sleep(1000);
    }

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        M_GAME_VOTED(table, i) = FALSE;
    }

    // 本局没人叫地主，则随机选择先叫地主的玩家
    M_GAME_WINNER_SEAT(table) = INVALID_SEAT;

    // 检查是否有玩家掉线，如果没有，则最多发两次牌，如果还没有人叫地主，
    // 则结束游戏，将他们置为SIT状态
    BOOL bHasLost = FALSE;
    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        if (M_GAME_CONNECTION_LOST(table, i) == TRUE) {
            bHasLost = TRUE;
            break;
        }
    }

    if (bHasLost == TRUE) {
        WriteLog(LOG_DEBUG, _T("没有玩家叫地主，但有玩家掉线。结束游戏。"));
        EndGame(table);
    } else {
        M_GAME_VOTE_LORD_FAIL_TIMES(table) += 1;
        if (M_GAME_VOTE_LORD_FAIL_TIMES(table) > SERVER_REDISTR_POKER_TIMES) {
            WriteLog(LOG_DEBUG, _T("已经超过 %d 次没有玩家叫地主，服务器将不重新发牌，玩家进入就坐状态"), 
                M_GAME_VOTE_LORD_FAIL_TIMES(table));
            EndGame(table);
        } else {
            WriteLog(LOG_DEBUG, _T("第 %d 次没有玩家叫地主，服务器将重新发牌"), 
                M_GAME_VOTE_LORD_FAIL_TIMES(table));
            PostMessage(NULL, TM_START_GAME, 0, 0);
        }
    }
}

// 地主产生
static void VoteLordSucceed(int table, int seat, int score)
{
    BYTE data[8];

    data[0] = NMSG_VOTE_LORD_FINISH;

    data[1] = 0;
    data[2] = 3;

    data[3] = (BYTE)TRUE;
    data[4] = (BYTE)seat;
    data[5] = (BYTE)score;

    WORD crc = CRC16(data, 6);
    data[6] = (BYTE)(crc >> 8);
    data[7] = (BYTE)(crc >> 0);

    // 通知游戏桌所有成员，地主已经产生
    BroadcastInTable(table, (char*)data, 8);

    WriteLog(LOG_DEBUG, _T("第 %d 桌竞选地主结束。第 %d 椅子玩家叫地主: %d 分"), table, seat, score);

    // 将底牌插入地主的牌中
    for (int i = 0; i < UNDER_CARDS_NUM; i++) {
        M_GAME_PLAYER_POKERS(table, seat)[PLAYER_INIT_CARDS_NUM + i] = M_GAME_UNDER_POKER_CARDS(table)[i];
    }

    quick_sort(M_GAME_PLAYER_POKERS(table, seat), 0, PLAYER_MAX_CARDS_NUM - 1);

    M_GAME_PLAYER_POKER_NUM(table, seat) = PLAYER_MAX_CARDS_NUM;

    { //Debug...
        int len;
        TCHAR buf[128];

        len = 0;
        for (int i = 0; i < M_GAME_PLAYER_POKER_NUM(table, 0); i++) {
            len += _stprintf_s(buf + len, sizeof(buf) / sizeof(buf[0]) - len, _T("%c "),
                poker_index_to_char(M_GAME_PLAYER_POKERS(table, 0)[i]));
        }
        WriteLog(LOG_DEBUG, buf);

        len = 0;
        for (int i = 0; i < M_GAME_PLAYER_POKER_NUM(table, 1); i++) {
            len += _stprintf_s(buf + len, sizeof(buf) / sizeof(buf[0]) - len, _T("%c "),
                poker_index_to_char(M_GAME_PLAYER_POKERS(table, 1)[i]));
        }
        WriteLog(LOG_DEBUG, buf);

        len = 0;
        for (int i = 0; i < M_GAME_PLAYER_POKER_NUM(table, 2); i++) {
            len += _stprintf_s(buf + len, sizeof(buf) / sizeof(buf[0]) - len, _T("%c "),
                poker_index_to_char(M_GAME_PLAYER_POKERS(table, 2)[i]));
        }
        WriteLog(LOG_DEBUG, buf);
    }

    // 开始出牌，先请求地主首次出牌
    M_GAME_SAVE_POKER_PROPERTY(table, 0, 0, 0);

    WPARAM wp = MAKEWPARAM(M_GAME_LORD_SEAT(table), 0);
    LPARAM lp = MAKELPARAM(0, 0);
    PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);

    M_GAME_OUTPUT_CARD_STARTED(table) = TRUE;
}

// 广播消息给所有游戏桌成员
static void BroadcastInTable(int table, char* buf, int len)
{
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        SendData(M_GAME_PLAYER_ID(table, seat), buf, len);

        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            if (M_GAME_LOOKON_ID(table, seat)[i] != INVALID_USER_ID) {
                SendData(M_GAME_LOOKON_ID(table, seat)[i], buf, len);
            }
        }
    }
}

// 除某个玩家外，广播消息给该游戏桌所有其它成员
static void BroadcastInTableExceptId(int table, char* buf, int len, int id)
{
    for (int seat = 0; seat < GAME_SEAT_NUM_PER_TABLE; seat++) {
        if (id != M_GAME_PLAYER_ID(table, seat)) {
            SendData(M_GAME_PLAYER_ID(table, seat), buf, len);
        }

        for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
            if (M_GAME_LOOKON_ID(table, seat)[i] != INVALID_USER_ID) {
                if (id != M_GAME_LOOKON_ID(table, seat)[i]) {
                    SendData(M_GAME_LOOKON_ID(table, seat)[i], buf, len);
                }
            }
        }
    }
}

// 请求玩家出牌
static void ReqOutputCard(int table, int seat, POKER_PROPERTY* req)
{
    // 记录服务器请求出牌的椅子号
    M_GAME_ACTIVE_SEAT(table) = seat;

    if (M_GAME_DELEGATED(table, seat) == TRUE) {
        {//Debug...
            Sleep(100);
        }

        OutputCardDelegated(table, seat, req);
    } else {
        BYTE data[10];
        data[0] = NMSG_REQ_OUTPUT_CARD;
        data[1] = 0;
        data[2] = 4;

        data[3] = (BYTE)seat;
        data[4] = (BYTE)req->type;
        data[5] = (BYTE)req->value;
        data[6] = (BYTE)req->num;

        WORD crc = CRC16(data, 7);
        data[7] = (BYTE)(crc >> 8);
        data[8] = (BYTE)(crc >> 0);

        BroadcastInTable(table, (char*)data, 9);
    }
}

// 服务器托管游戏出牌
static void OutputCardDelegated(int table, int seat, POKER_PROPERTY* req)
{
    BYTE data[24];
    BOOL ret = FALSE;
    POKER_PROPERTY pp;
    ZeroMemory(&pp, sizeof(POKER_PROPERTY));

    int next_seat = (seat + 1) % GAME_SEAT_NUM_PER_TABLE;

    if ((req->value == 0) && (req->num == 0)) { // 首次出牌
        data[0] = NMSG_OUTPUT_CARD;
        data[1] = 0;
        data[2] = 3;

        data[3] = (BYTE)seat;
        data[4] = (BYTE)1; // 出一张最小的牌

        int poker_elem = M_GAME_PLAYER_POKERS(table, seat)[0];
        data[5] = (BYTE)(poker_elem); // 出一张最小的牌

        WORD crc = CRC16(data, 6);
        data[6] = (BYTE)(crc >> 8);
        data[7] = (BYTE)(crc >> 0);

        BroadcastInTable(table, (char*)data, 8);

        // 设置所出的牌的属性，如果下家要出牌，则需根据此牌型出牌
        pp.num = 1;
        pp.type = SINGLE;
        pp.value = poker_index_to_value(poker_elem);

        WriteLog(LOG_DEBUG, _T("[托管] 第 %d 桌 %d 椅子玩家出牌 %d 张 [%s]: %c "), table, seat,
            pp.num, poker_type_to_string(pp.type), poker_index_to_char(poker_elem));

        int indexes[1] = { 0 }; // 出了一张牌，出当前最小的那张牌，即索引为0
        RemoveOutputPokersByIndex(table, seat, indexes, 1);

        if (seat == M_GAME_LORD_SEAT(table)) { // 是否为地主出牌
            M_GAME_LORD_OUTPUT_TIMES(table) += 1;
        }

        if (M_GAME_PLAYER_POKER_NUM(table, seat) <= 0) {
            GameOver(table, seat);
        } else {
            M_GAME_UP_NOT_FOLLOW(table) = FALSE; // 本玩家出牌，则置标志位为FALSE
            M_GAME_SAVE_POKER_PROPERTY(table, pp.type, pp.value, pp.num);

            WPARAM wp = MAKEWPARAM(next_seat, pp.type);
            LPARAM lp = MAKELPARAM(pp.value, pp.num);
            PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
        }
    } else { // 接上家所出的牌
        int indexes[PLAYER_MAX_CARDS_NUM];
        POKER_CLASS_TABLE* pct;

        pct = (POKER_CLASS_TABLE*)LocalAlloc(LMEM_FIXED, sizeof(POKER_CLASS_TABLE));
        if (pct == NULL) {
            WriteLog(LOG_ERROR, _T("[托管] 申请内存失败，OutputCardDelegated: [POKER_CLASS_TABLE]"));
            ret = FALSE;
            goto FOLLOW_OR_NOT;
        }

        reset_poker_class_table(pct);

        // 利用提示，查找一个可以接得起的牌序列
        build_poker_class_table(pct, M_GAME_PLAYER_POKERS(table, seat), M_GAME_PLAYER_POKER_NUM(table, seat));
        ret = get_poker_hint(0, pct, req, &pp, indexes);

        LocalFree(pct);

FOLLOW_OR_NOT:
        if (ret == TRUE) { // 可以接上牌
            { //Debug...
                int len = 0;
                TCHAR outpokers[PLAYER_MAX_CARDS_NUM * 2 + 1] = { 0 }; // 1个字节加1个空格
                for (int i = 0; i < pp.num; i++) {
                    len += _stprintf_s(outpokers + len, sizeof(outpokers) / sizeof(outpokers[0]) - len, _T("%c "), 
                        poker_index_to_char(M_GAME_PLAYER_POKERS(table, seat)[indexes[i]]));
                }

                WriteLog(LOG_DEBUG, _T("[托管] 第 %d 桌 %d 椅子玩家出牌 %d 张 [%s]: %s"), table, seat,
                    pp.num, poker_type_to_string(pp.type), outpokers);
            }

            data[0] = NMSG_OUTPUT_CARD;
            data[1] = (BYTE)((1 + 1 + pp.num) >> 8);
            data[2] = (BYTE)((1 + 1 + pp.num) >> 0);

            data[3] = (BYTE)seat;
            data[4] = (BYTE)pp.num;

            int offset = 5;
            for (int i = 0; i < pp.num; i++) {
                data[offset] = (BYTE)(M_GAME_PLAYER_POKERS(table, seat)[indexes[i]]);
                offset++;
            }

            WORD crc = CRC16(data, offset);
            data[offset] = (BYTE)(crc >> 8);
            data[offset + 1] = (BYTE)(crc >> 0);
            offset += 2;

            // 广播游戏桌，通知其它玩家
            BroadcastInTable(table, (char*)data, offset);

            // 更新服务器上的数据
            RemoveOutputPokersByIndex(table, seat, indexes, pp.num);

            if (pp.type == BOMB) { // 出的牌是否为炸弹
                M_GAME_MULTIPLE(table)++;
            }

            if (seat == M_GAME_LORD_SEAT(table)) { // 牌是否为地主出的
                M_GAME_LORD_OUTPUT_TIMES(table) += 1;
            }

            if (M_GAME_PLAYER_POKER_NUM(table, seat) <= 0) {
                GameOver(table, seat);
            } else {
                M_GAME_UP_NOT_FOLLOW(table) = FALSE; // 本玩家出牌，则置标志位为FALSE
                M_GAME_SAVE_POKER_PROPERTY(table, pp.type, pp.value, pp.num);

                WPARAM wp = MAKEWPARAM(next_seat, pp.type);
                LPARAM lp = MAKELPARAM(pp.value, pp.num);
                PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
            }
        } else { // 不能接上牌
            WriteLog(LOG_DEBUG, _T("[托管] 第 %d 桌 %d 椅子玩家不出牌"), table, seat);

            data[0] = NMSG_OUTPUT_CARD;
            data[1] = 0;
            data[2] = 2;

            data[3] = (BYTE)seat;
            data[4] = (BYTE)0;      // 表示不出牌

            WORD crc = CRC16(data, 5);
            data[5] = (BYTE)(crc >> 8);
            data[6] = (BYTE)(crc >> 0);

            BroadcastInTable(table, (char*)data, 7);

            if (M_GAME_UP_NOT_FOLLOW(table) == TRUE) {
                // 上家接不起，本玩家也接不起牌，则下家可以重新出牌
                M_GAME_SAVE_POKER_PROPERTY(table, 0, 0, 0);

                WPARAM wp = MAKEWPARAM(next_seat, 0);
                LPARAM lp = MAKELPARAM(0, 0);
                PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
            } else {
                // 当前玩家接不起牌，要求下家接此牌
                M_GAME_UP_NOT_FOLLOW(table) = TRUE;
                M_GAME_SAVE_POKER_PROPERTY(table, req->type, req->value, req->num);

                WPARAM wp = MAKEWPARAM(next_seat, req->type);
                LPARAM lp = MAKELPARAM(req->value, req->num);
                PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
            }
        }
    }
}

// 删除索引所指示的扑克牌，该索引不是全局扑克牌的索引，而是玩家手中当前牌的索引
static void RemoveOutputPokersByIndex(int table, int seat, int indexes[], int num)
{
    // 借用一个数组，将剩余的牌拷贝出去，再拷贝回来

    int rem_poker[PLAYER_MAX_CARDS_NUM] = { 0 };
    BOOL output[PLAYER_MAX_CARDS_NUM] = { 0 };

    for (int i = 0; i < num; i++) {
        output[indexes[i]] = TRUE;
    }

    int rem_count = 0;
    for (int i = 0; i < M_GAME_PLAYER_POKER_NUM(table, seat); i++) {
        if (output[i] != TRUE) {
            rem_poker[rem_count] = M_GAME_PLAYER_POKERS(table, seat)[i];
            rem_count++;
        }
    }

    M_GAME_PLAYER_POKER_NUM(table, seat) = rem_count;

    for (int i = 0; i < rem_count; i++) {
        M_GAME_PLAYER_POKERS(table, seat)[i] = rem_poker[i];
    }
}

// 根据玩家发送到服务器的出牌消息，从服务器牌数据中删除玩家所出的牌
// 注意：pokers是BYTE型的数组，其元素值是全局扑克牌索引
static void RemoveOutputPokers(int table, int seat, BYTE pokers[], int num)
{
    int old_num = M_GAME_PLAYER_POKER_NUM(table, seat);

    BOOL output[PLAYER_MAX_CARDS_NUM] = { 0 };

    // 将所出的牌在原牌序列中置标志（扑克牌均为升序）
    int j = 0;
    for (int i = 0; i < old_num; i++) {
        if (M_GAME_PLAYER_POKERS(table, seat)[i] == (int)(pokers[j])) {
            output[i] = TRUE;
            if (++j >= num) {
                break;
            }
        }
    }

    // 将剩余的牌拷贝出去，再拷贝回来，即可实现删除所出的牌
    int rem_count = 0;
    int rem_pokers[PLAYER_MAX_CARDS_NUM];

    for (int i = 0; i < old_num; i++) {
        if (output[i] != TRUE) {
            rem_pokers[rem_count++] = M_GAME_PLAYER_POKERS(table, seat)[i];
        }
    }

    for (int i = 0; i < rem_count; i++) {
        M_GAME_PLAYER_POKERS(table, seat)[i] = rem_pokers[i];
    }

    M_GAME_PLAYER_POKER_NUM(table, seat) = rem_count;
}

// 结束游戏后，计算最后得分
static void GameOver(int table, int nWinnerSeat)
{
    BYTE data[24];

    // 记录此次游戏赢家的椅子号
    M_GAME_WINNER_SEAT(table) = nWinnerSeat;

    int nLordScore = M_GAME_LORD_SCORE(table);  //地主叫的底分
    int nLordSeat = M_GAME_LORD_SEAT(table);    //地主的座位号
    int nMultiple = M_GAME_MULTIPLE(table);     //倍数

    int nFinalLordScore = 0;        // 地主最后得分
    int nFinalFarmerScore = 0;      // 农民最后得分
    int nFinalScore[GAME_SEAT_NUM_PER_TABLE] = { 0 }; // 三个玩家最后得分（按座位号）

    if (nLordSeat == nWinnerSeat) { // 地主赢
        if (M_GAME_PLAYER_POKER_NUM(table, (nLordSeat + 1) % GAME_SEAT_NUM_PER_TABLE) == PLAYER_INIT_CARDS_NUM) {
            if (M_GAME_PLAYER_POKER_NUM(table, (nLordSeat + 2) % GAME_SEAT_NUM_PER_TABLE) == PLAYER_INIT_CARDS_NUM) {
                // 两个农民都没出牌，则加1倍
                nMultiple++;
            }
        }

        nFinalLordScore = 2 * nLordScore; // 双倍底分
        for (int i = 0; i < nMultiple; i++) {
            nFinalLordScore *= 2;
        }
        nFinalFarmerScore = -1 * nFinalLordScore / 2;
    } else { // 农民赢
        if (M_GAME_LORD_OUTPUT_TIMES(table) <= 1) {
            // 地主只出过一次牌，则加1倍
            nMultiple++;
        }

        nFinalFarmerScore = nLordScore; // 底分
        for (int i = 0; i < nMultiple; i++) {
            nFinalFarmerScore *= 2;
        }
        nFinalLordScore = -1 * nFinalFarmerScore * 2;
    }

    // 记录得分
    nFinalScore[nLordSeat] = nFinalLordScore;
    nFinalScore[(nLordSeat + 1) % GAME_SEAT_NUM_PER_TABLE] = nFinalFarmerScore;
    nFinalScore[(nLordSeat + 2) % GAME_SEAT_NUM_PER_TABLE] = nFinalFarmerScore;

    // 服务器更新玩家积分等统计
    M_CONNECT_PLAYER_TOTALGAMES(M_GAME_PLAYER_ID(table, 0)) += 1;
    M_CONNECT_PLAYER_TOTALGAMES(M_GAME_PLAYER_ID(table, 1)) += 1;
    M_CONNECT_PLAYER_TOTALGAMES(M_GAME_PLAYER_ID(table, 2)) += 1;

    if (nWinnerSeat == nLordSeat) {
        M_CONNECT_PLAYER_WINGAMES(M_GAME_PLAYER_ID(table, nLordSeat)) += 1;
    } else {
        M_CONNECT_PLAYER_WINGAMES(M_GAME_PLAYER_ID(table, (nLordSeat + 1) % 3)) += 1;
        M_CONNECT_PLAYER_WINGAMES(M_GAME_PLAYER_ID(table, (nLordSeat + 2) % 3)) += 1;
    }

    M_CONNECT_PLAYER_SCORE(M_GAME_PLAYER_ID(table, nLordSeat)) += nFinalLordScore;
    M_CONNECT_PLAYER_SCORE(M_GAME_PLAYER_ID(table, (nLordSeat + 1) % 3)) += nFinalFarmerScore;
    M_CONNECT_PLAYER_SCORE(M_GAME_PLAYER_ID(table, (nLordSeat + 2) % 3)) += nFinalFarmerScore;

    // 构造游戏结束消息
    int offset = 0;

    data[offset] = NMSG_GAME_OVER;
    offset++;

    data[offset + 0] = 0;
    data[offset + 1] = 12;
    offset += 2;

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        data[offset + 0] = (BYTE)(nFinalScore[i] >> 24);
        data[offset + 1] = (BYTE)(nFinalScore[i] >> 16);
        data[offset + 2] = (BYTE)(nFinalScore[i] >> 8);
        data[offset + 3] = (BYTE)(nFinalScore[i] >> 0);
        offset += 4;
    }

    WORD crc = CRC16(data, offset);
    data[offset + 0] = (BYTE)(crc >> 8);
    data[offset + 1] = (BYTE)(crc >> 0);
    offset += 2;

    // 通知所有玩家有关游戏结束后的得分情况
    BroadcastInTable(table, (char*)data, offset);

    WriteLog(LOG_DEBUG, _T("游戏结束。第 %d 桌 %d 椅子的玩家赢得本局游戏。玩家得分：[%d, %d, %d]"),
        table, nWinnerSeat, nFinalScore[0], nFinalScore[1], nFinalScore[2]);

    // 复位相关游戏桌数据
    EndGame(table);
}

// 响应玩家出牌
static void OnAckOutputCard(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    if (M_GAME_STARTED(table) == FALSE) {
        return;
    }

    // 复位服务器等待回应的玩家椅子号
    M_GAME_ACTIVE_SEAT(table) = INVALID_SEAT;

    BYTE data[PLAYER_MAX_CARDS_NUM + 8]; // 用于构造出牌的消息
    int seat = pMsgBody[0]; // 出牌玩家的椅子号
    POKER_TYPE type = (POKER_TYPE)pMsgBody[1]; // 出牌的类型
    int value = pMsgBody[2]; // 出牌的值
    int num = pMsgBody[3]; // 出牌的张数
    BYTE* pokers = pMsgBody + 4; // 扑克牌序列开始地址

    if ((4 + num) != nMsgBodyLen) {
        WriteLog(LOG_ERROR, _T("NMSG_ACK_OUTPUT_CARD 消息结构不正确"));
        return;
    }

    int next_seat = (seat + 1) % GAME_SEAT_NUM_PER_TABLE;

    if ((value == 0) && (num == 0)) { // 玩家不出牌
        WriteLog(LOG_DEBUG, _T("第 %d 桌 %d 椅子玩家不出牌"), table, seat);

        data[0] = NMSG_OUTPUT_CARD;
        data[1] = 0;
        data[2] = 2;

        data[3] = (BYTE)seat;
        data[4] = (BYTE)0;      // 表示不出牌

        WORD crc = CRC16(data, 5);
        data[5] = (BYTE)(crc >> 8);
        data[6] = (BYTE)(crc >> 0);

        // 广播通知客户端玩家，该椅子的玩家不出牌
        BroadcastInTable(table, (char*)data, 7);

        if (M_GAME_UP_NOT_FOLLOW(table) == TRUE) {
            // 上家接不起牌，本玩家也接不起牌，则下家可以重新出牌
            M_GAME_SAVE_POKER_PROPERTY(table, 0, 0, 0);

            WPARAM wp = MAKEWPARAM(next_seat, 0);
            LPARAM lp = MAKELPARAM(0, 0);
            PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
        } else {
            // 上家出过牌，当前玩家不出牌，要求下家接上家所出的牌
            M_GAME_UP_NOT_FOLLOW(table) = TRUE;

            WPARAM wp = MAKEWPARAM(next_seat, M_GAME_OUTPUT_POKER_TYPE(table));
            LPARAM lp = MAKELPARAM(M_GAME_OUTPUT_POKER_VALUE(table), M_GAME_OUTPUT_POKER_NUM(table));
            PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
        }
    } else { // 玩家出牌
        {//Debug...
            int len = 0;
            TCHAR outpokers[PLAYER_MAX_CARDS_NUM * 2 + 1] = { 0 }; // 1个字节加1个空格
            for (int i = 0; i < num; i++) {
                len += _stprintf_s(outpokers + len, sizeof(outpokers) / sizeof(outpokers[0]) - len,
                    _T("%c "), poker_index_to_char(pokers[i]));
            }

            WriteLog(LOG_DEBUG, _T("第 %d 桌 %d 椅子玩家出牌 %d 张 [%s]: %s"),
                table, seat, num, poker_type_to_string(type), outpokers);
        }

        data[0] = NMSG_OUTPUT_CARD;
        data[1] = (BYTE)((1 + 1 + num) >> 8);
        data[2] = (BYTE)((1 + 1 + num) >> 0);

        data[3] = (BYTE)seat;
        data[4] = (BYTE)num;

        int offset = 5;
        for (int i = 0; i < num; i++) {
            data[offset] = pokers[i];
            offset++;
        }

        WORD crc = CRC16(data, offset);
        data[offset] = (BYTE)(crc >> 8);
        data[offset + 1] = (BYTE)(crc >> 0);
        offset += 2;

        // 广播游戏桌
        BroadcastInTable(table, (char*)data, offset);

        // 更新服务器上的数据
        RemoveOutputPokers(table, seat, pokers, num);

        if (type == BOMB) { // 出的牌是否为炸弹
            M_GAME_MULTIPLE(table)++;
        }

        if (seat == M_GAME_LORD_SEAT(table)) { // 牌是否为地主出的
            M_GAME_LORD_OUTPUT_TIMES(table) += 1;
        }

        if (M_GAME_PLAYER_POKER_NUM(table, seat) <= 0) {
            GameOver(table, seat);
        } else {
            M_GAME_UP_NOT_FOLLOW(table) = FALSE; // 本玩家出牌，则置标志位为FALSE
            M_GAME_SAVE_POKER_PROPERTY(table, type, value, num);

            WPARAM wp = MAKEWPARAM(next_seat, type);
            LPARAM lp = MAKELPARAM(value, num);
            PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
        }
    }
}

// 响应玩家中途退出游戏的请求
static void OnReqStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    int seat = pMsgBody[0];

    BYTE* data = (BYTE*)LocalAlloc(LMEM_FIXED, nMsgBodyLen + 5);
    if (data == NULL) {
        WriteLog(LOG_ERROR, _T("分配内存失败，应答玩家中途退出游戏"));
        return;
    }

    data[0] = NMSG_REQ_STOP_GAMING;

    data[1] = (BYTE)(nMsgBodyLen >> 8);
    data[2] = (BYTE)(nMsgBodyLen >> 0);

    for (int i = 0; i < nMsgBodyLen; i++) {
        data[3 + i] = pMsgBody[i];
    }

    WORD crc = CRC16(data, 3 + nMsgBodyLen);
    data[3 + nMsgBodyLen + 0] = (BYTE)(crc >> 8);
    data[3 + nMsgBodyLen + 1] = (BYTE)(crc >> 0);

    int next_seat = (seat + 1) % GAME_SEAT_NUM_PER_TABLE; // 下家

    // 转发消息给下一个玩家，征求他的意见
    SendData(M_GAME_PLAYER_ID(table, next_seat), (char*)data, 3 + nMsgBodyLen + 2);

    LocalFree(data);
}

// 响应玩家是否同意某玩家中途退出游戏
static void OnAckStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    int ack_exit_seat   = pMsgBody[0];
    BOOL bPermit        = (BOOL)pMsgBody[1];
    int req_exit_seat   = pMsgBody[2];
    int nReasonTextLen  = pMsgBody[3];
    BYTE* szStopReason  = pMsgBody + 4;

    if (M_GAME_STARTED(table) == FALSE) {
        return;
    }

    if ((4 + nReasonTextLen) != nMsgBodyLen) {
        WriteLog(LOG_ERROR, _T("NMSG_ACK_STOP_GAMING 消息结构不正确"));
        return;
    }

    WORD crc;
    BYTE* data = (BYTE*)LocalAlloc(LMEM_FIXED, 5 + nMsgBodyLen);
    if (data == NULL) {
        WriteLog(LOG_ERROR, _T("分配内存失败，响应某玩家是否同意中途退出"));
        return;
    }

    if (bPermit == FALSE) { // 某玩家不同意，则回复请求退出的玩家为不同意退出
        data[0] = NMSG_ACK_STOP_GAMING;
        data[1] = 0;
        data[2] = 2;

        data[3] = (BYTE)FALSE;
        data[4] = (BYTE)ack_exit_seat;

        crc = CRC16(data, 5);
        data[5] = (BYTE)(crc >> 8);
        data[6] = (BYTE)(crc >> 0);

        SendData(M_GAME_PLAYER_ID(table, req_exit_seat), (char*)data, 7);
    } else { // 某玩家同意
        //
        // 请求退出的玩家若是应答者的下家，则说明两个玩家都反馈了同意退出的意见，
        // 否则还需征求该应答者的下家的意见
        //
        if (req_exit_seat == ((ack_exit_seat + 1) % GAME_SEAT_NUM_PER_TABLE)) { // 两个玩家都同意退出
            data[0] = NMSG_ACK_STOP_GAMING;
            data[1] = 0;
            data[2] = 1;

            data[3] = (BYTE)TRUE;

            crc = CRC16(data, 4);
            data[4] = (BYTE)(crc >> 8);
            data[5] = (BYTE)(crc >> 0);

            BroadcastInTable(table, (char*)data, 6);

            EndGame(table);

            // 更新服务器数据，及玩家状态更改
            int id = M_GAME_PLAYER_ID(table, req_exit_seat);

            M_GAME_PLAYER_ID(table, req_exit_seat) = INVALID_USER_ID;

            M_TABLE_PLAYER_ID(table, req_exit_seat) = INVALID_USER_ID;

            M_CONNECT_STATE(id) = STATE_IDLE;

            AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_IDLE, FALSE);
        } else { // 第一个玩家同意，再征求第二个玩家的意见
            int next_seat = (ack_exit_seat + 1) % GAME_SEAT_NUM_PER_TABLE;

            data[0] = NMSG_REQ_STOP_GAMING;
            data[1] = (BYTE)((nReasonTextLen + 2) >> 8);
            data[2] = (BYTE)((nReasonTextLen + 2) >> 0);

            data[3] = (BYTE)req_exit_seat;
            data[4] = (BYTE)nReasonTextLen;
            for (int i = 0; i < nReasonTextLen; i++) {
                data[5 + i] = szStopReason[i];
            }

            crc = CRC16(data, 5 + nReasonTextLen);
            data[5 + nReasonTextLen + 0] = (BYTE)(crc >> 8);
            data[5 + nReasonTextLen + 1] = (BYTE)(crc >> 0);

            SendData(M_GAME_PLAYER_ID(table, next_seat), (char*)data, 5 + nReasonTextLen + 2);
        }
    }

    LocalFree(data);
}

// 响应玩家托管游戏的请求
static void OnReqDelegate(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    if (M_GAME_STARTED(table) == FALSE) {
        return;
    }

    if (nMsgBodyLen != 2) {
        WriteLog(LOG_ERROR, _T("NMSG_REQ_DELEGATE 消息结构不正确"));
        return;
    }

    int seat = pMsgBody[0];
    BOOL bDelegate = (BOOL)pMsgBody[1];

    if (M_GAME_DELEGATED(table, seat) == bDelegate) {
        return;
    }

    M_GAME_DELEGATED(table, seat) = bDelegate;

    BYTE data[8];
    data[0] = NMSG_ACK_DELEGATE;
    data[1] = 0;
    data[2] = 2;

    data[3] = pMsgBody[0];
    data[4] = pMsgBody[1];

    WORD crc = CRC16(data, 5);
    data[5] = (BYTE)(crc >> 8);
    data[6] = (BYTE)(crc >> 0);

    BroadcastInTable(table, (char*)data, 7);

    if (M_GAME_DELEGATED(table, seat) == TRUE) {
        if (seat == M_GAME_ACTIVE_SEAT(table)) { // 请求托管游戏的玩家是服务器正等待回复消息的玩家
            if (M_GAME_OUTPUT_CARD_STARTED(table) == FALSE) { // 游戏已经开始，但还没开始出牌，正叫分中
                PostMessage(NULL, TM_REQ_VOTE_LORD, (WPARAM)seat, (LPARAM)(M_GAME_LORD_SCORE(table)));
            } else { // 已经开始出牌
                WPARAM wp = MAKEWPARAM(seat, M_GAME_OUTPUT_POKER_TYPE(table));
                LPARAM lp = MAKELPARAM(M_GAME_OUTPUT_POKER_VALUE(table), M_GAME_OUTPUT_POKER_NUM(table));
                PostMessage(NULL, TM_REQ_OUTPUT_CARD, wp, lp);
            }
        }
    }

    if (bDelegate == TRUE) {
        WriteLog(LOG_INFO, _T("第 %d 桌 %d 椅子的玩家，选择托管游戏"), table, seat);
    } else {
        WriteLog(LOG_INFO, _T("第 %d 桌 %d 椅子的玩家，取消托管游戏"), table, seat);
    }
}

// 响应玩家设置是否允许旁观的选项
// 客户端需要控制，只能是玩家发送此消息，旁观者不能发送此消息
static void OnAllowLookon(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    if (nMsgBodyLen != 2) {
        WriteLog(LOG_ERROR, _T("NMSG_ALLOW_LOOKON 消息结构不正确"));
        return;
    }

    int seat = pMsgBody[0];
    BOOL bAllow = (BOOL)pMsgBody[1];

    int id = M_GAME_PLAYER_ID(table, seat);
    M_CONNECT_ALLOW_LOOKON(id) = bAllow;

    // 通知该椅子的旁观者
    BYTE data[8];
    data[0] = NMSG_ALLOW_LOOKON;
    data[1] = 0;
    data[2] = 2;

    data[3] = (BYTE)seat;
    data[4] = (BYTE)bAllow;

    WORD crc = CRC16(data, 5);
    data[5] = (BYTE)(crc >> 8);
    data[6] = (BYTE)(crc >> 0);

    for (int i = 0; i < MAX_LOOKON_NUM_PER_SEAT; i++) {
        if (M_GAME_LOOKON_ID(table, seat)[i] != INVALID_USER_ID) {
            SendData(M_GAME_LOOKON_ID(table, seat)[i], (char*)data, 7);
        }
    }

    // 使用状态更改事件使得客户端知道该玩家的设置
    AddUserStateChangeEvent(M_GAME_PLAYER_ID(table, seat), EVT_CHGSTATE, M_CONNECT_STATE(id), FALSE);
}

// 游戏过程中，玩家强行退出游戏
static void OnReqForceStopGaming(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    if (nMsgBodyLen != 1) {
        WriteLog(LOG_ERROR, _T("NMSG_REQ_FORCE_STOP_GAMING 消息结构不正确"));
        return;
    }

    BYTE data[24];
    WORD crc;
    int seat = pMsgBody[0];
    int id = M_GAME_PLAYER_ID(table, seat);

    //
    // 通知游戏桌其他成员，某玩家强行退出
    //
    data[0] = NMSG_ACK_FORCE_STOP_GAMING;
    data[1] = 0;
    data[2] = 1;

    data[3] = (BYTE)seat;

    crc = CRC16(data, 4);
    data[4] = (BYTE)(crc >> 8);
    data[5] = (BYTE)(crc >> 0);

    BroadcastInTableExceptId(table, (char*)data, 6, id);

    //
    // 计算逃跑后得分情况
    //
    int results[GAME_SEAT_NUM_PER_TABLE];
    int multiple = M_GAME_MULTIPLE(table) + RUN_AWAY_MULTIPLE;
    int score = M_GAME_LORD_SCORE(table);

    for (int i = 0; i < multiple; i++) {
        score *= 2;
    }

    results[seat] = -1 * score;
    results[(seat + 1) % GAME_SEAT_NUM_PER_TABLE] = score / 2;
    results[(seat + 2) % GAME_SEAT_NUM_PER_TABLE] = score / 2;

    // 构造游戏结束消息
    int offset = 0;

    data[offset] = NMSG_GAME_OVER;
    offset++;

    data[offset + 0] = 0;
    data[offset + 1] = 12;
    offset += 2;

    for (int i = 0; i < GAME_SEAT_NUM_PER_TABLE; i++) {
        data[offset + 0] = (BYTE)(results[i] >> 24);
        data[offset + 1] = (BYTE)(results[i] >> 16);
        data[offset + 2] = (BYTE)(results[i] >> 8);
        data[offset + 3] = (BYTE)(results[i] >> 0);
        offset += 4;
    }

    crc = CRC16(data, offset);
    data[offset + 0] = (BYTE)(crc >> 8);
    data[offset + 1] = (BYTE)(crc >> 0);
    offset += 2;

    // 通知其它玩家最后得分情况
    BroadcastInTableExceptId(table, (char*)data, offset, id);

    // 游戏结束
    M_GAME_WINNER_SEAT(table) = INVALID_SEAT;
    EndGame(table);

    WriteLog(LOG_INFO, _T("第 %d 桌 %d 椅子的玩家强行退出游戏"), table, seat);

    // 更新服务器数据，及玩家状态更改
    M_GAME_PLAYER_ID(table, seat) = INVALID_USER_ID;

    M_TABLE_PLAYER_ID(table, seat) = INVALID_USER_ID;

    M_CONNECT_STATE(id) = STATE_IDLE;

    AddUserStateChangeEvent(id, EVT_CHGSTATE, STATE_IDLE, FALSE);
}

// 转发聊天消息（游戏桌内所有人都可见，不允许私聊）
static void OnChatMessage(int table, BYTE* pMsgBody, int nMsgBodyLen)
{
    BYTE data[140]; // 聊天消息最多127个字节

    data[0] = NMSG_CHATTING;
    data[1] = 0;
    data[2] = (BYTE)nMsgBodyLen;

    for (int i = 0; i < nMsgBodyLen; i++) {
        data[3 + i] = pMsgBody[i];
    }

    WORD crc = CRC16(data, 3 + nMsgBodyLen);
    data[3 + nMsgBodyLen + 0] = (BYTE)(crc >> 8);
    data[3 + nMsgBodyLen + 1] = (BYTE)(crc >> 0);

    BroadcastInTable(table, (char*)data, 3 + nMsgBodyLen + 2);
}
