//
// File: UserData.h
//
#pragma once

// 玩家基本信息
typedef struct _USER_INFO_t {
    int             id;
    TCHAR           name[MAX_USER_NAME_LEN];
    PLAYER_GENDER   gender;
    int             imageIdx;
    PLAYER_STATE    state;
    BOOL            bAllowLookon;
    int             table;
    int             seat;

    int             nScore;         // 积分
    int             nTotalGames;    // 总游戏局数
    int             nWinGames;      // 赢的局数
    int             nRunawayTimes;  // 逃跑次数
} USER_INFO, *LPUSER_INFO, PLAYER_INFO, *LPPLAYER_INFO;

// 玩家基本信息，增加 valid 项用于指示该玩家信息是否有效
typedef struct _USER_DATA_t {
    BOOL            valid;
    PLAYER_INFO     playerInfo;
} USER_DATA, *LPUSER_DATA, PLAYER_DATA, *LPPLAYER_DATA;

// 描述状态变化的玩家信息
typedef struct _USER_CHG_DATA_t {
    STATE_CHANGE_EVENT  event;
    PLAYER_INFO     playerInfo;
    BOOL            bContainStatistics; // 是否携带玩家相关统计信息，仅适用于 EVT_CHGSTATE
} PLAYER_CHG_DATA, *LPPLAYER_CHG_DATA;

// 服务器给客户端发牌
typedef struct _DISTRIBUTE_POKER_DATA_t {
    int numSeat0;
    int pokerSeat0[PLAYER_MAX_CARDS_NUM];

    int numSeat1;
    int pokerSeat1[PLAYER_MAX_CARDS_NUM];

    int numSeat2;
    int pokerSeat2[PLAYER_MAX_CARDS_NUM];

    int underCards[UNDER_CARDS_NUM];
} DISTRIBUTE_POKER, *LPDISTRIBUTE_POKER;

// 记录出牌
typedef struct _OUTPUT_POKER_DATA_t {
    //int num;
    int poker[PLAYER_MAX_CARDS_NUM];
} OUTPUT_POKER, *LPOUTPUT_POKER;

// 记录玩家游戏最后得分
typedef struct _GAME_RESULT_t {
    int nScore[GAME_SEAT_NUM_PER_TABLE];
} GAME_RESULT, *LPGAME_RESULT;


extern PLAYER_DATA  g_PlayerData[];
extern int          g_nLocalPlayerId;


BOOL ResetPlayerDataTable(void);

int GetUserImageIndex(int nUserId);
int GetUserStatusImageIndex(int nUserId);

LPCTSTR GetUserNameStr(int nUserId);
LPCTSTR GetUserLevelStr(int nUserId);

int GetUserScore(int nUserId);
int GetUserWinGames(int nUserId);
int GetUserTotalGames(int nUserId);
int GetUserRunawayTimes(int nUserId);

int GetGamerId(int table, int seat);
int* GetLookonIdsArrayPtr(int table, int seat);

PLAYER_GENDER GetUserGender(int nUserId);
PLAYER_GENDER GetGamerGender(int table, int seat);

int GetLocalUserId(void);
PLAYER_STATE GetLocalUserState(void);
int GetLocalUserTableId(void);
int GetLocalUserSeatId(void);

BOOL IsGamerAllowLookon(int nUserId);
BOOL IsGamerAllowLookon(int table, int seat);
