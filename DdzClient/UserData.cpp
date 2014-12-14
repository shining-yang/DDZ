//
// File: UserData.cpp
//
//  在线玩家 User(Player) 的基本信息
//
#include "stdafx.h"
#include "GameSeatWnd.h"
#include "GameTableWnd.h"
#include "GameRoomWnd.h"
#include "UserData.h"

extern GameRoomWnd g_GameRoomWnd;


// 在线玩家的信息。数组的索引为玩家的ID
PLAYER_DATA     g_PlayerData[MAX_CONNECTION_NUM] = { 0 };
int             g_nLocalPlayerId = INVALID_USER_ID;


// 复位玩家信息表（数组），与服务器建立连接前调用进行初始化。
BOOL ResetPlayerDataTable(void)
{
    ZeroMemory(&g_PlayerData, sizeof(PLAYER_DATA) * MAX_CONNECTION_NUM);
    for (int i = 0; i < MAX_CONNECTION_NUM; i++) {
        g_PlayerData[i].playerInfo.bAllowLookon = TRUE;
        g_PlayerData[i].playerInfo.state = STATE_IDLE;
        g_PlayerData[i].playerInfo.table = INVALID_TABLE;
        g_PlayerData[i].playerInfo.seat = INVALID_SEAT;
    }

    g_nLocalPlayerId = INVALID_USER_ID;
    return TRUE;
}

// 用户头像
int GetUserImageIndex(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.imageIdx;
}

// 显示用户状态的图标
int GetUserStatusImageIndex(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    PLAYER_STATE state = g_PlayerData[nUserId].playerInfo.state;

    //
    // 玩家状态图标存放在小头像图标末尾
    //
    return (int)(state) - (int)(STATE_IDLE) + (IDB_USERHEAD_16_LAST - IDB_USERHEAD_16_FIRST + 1);
}

// 用户昵称
LPCTSTR GetUserNameStr(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.name;
}

// 级别
LPCTSTR GetUserLevelStr(int nUserId)
{
    UNREFERENCED_PARAMETER(nUserId);
    int score = GetUserScore(nUserId);

    if (score < 10) {
        return _T("包身工");
    } else if (score < 25) {
        return _T("短工");
    } else if (score < 40) {
        return _T("长工");
    } else if (score < 80) {
        return _T("佃户");
    } else if (score < 140) {
        return _T("贫农");
    } else if (score < 230) {
        return _T("渔夫");
    } else if (score < 365) {
        return _T("猎人");
    } else if (score < 500) {
        return _T("中农");
    } else if (score < 700) {
        return _T("富农");
    } else if (score < 1000) {
        return _T("掌柜");
    } else if (score < 1500) {
        return _T("商人");
    } else if (score < 2200) {
        return _T("衙役");
    } else if (score < 3000) {
        return _T("小财主");
    } else if (score < 4000) {
        return _T("大财主");
    } else if (score < 5500) {
        return _T("小地主");
    } else if (score < 7700) {
        return _T("大地主");
    } else if (score < 10000) {
        return _T("知县");
    } else if (score < 14000) {
        return _T("通判");
    } else if (score < 20000) {
        return _T("知府");
    } else if (score < 30000) {
        return _T("总督");
    } else if (score < 45000) {
        return _T("巡抚");
    } else if (score < 70000) {
        return _T("丞相");
    } else {
        return _T("帝王");
    }
}

// 积分
int GetUserScore(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.nScore;
}

// 赢次数
int GetUserWinGames(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.nWinGames;
}

// 游戏总局数
int GetUserTotalGames(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.nTotalGames;
}

// 逃跑次数
int GetUserRunawayTimes(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.nRunawayTimes;
}

// 判断某玩家是否允许旁观
BOOL IsGamerAllowLookon(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.bAllowLookon;
}

// 判断某椅子的玩家是否允许旁观
BOOL IsGamerAllowLookon(int table, int seat)
{
    int nUserId = GetGamerId(table, seat);
    if (IS_INVALID_USER_ID(nUserId)) {
        return TRUE; // No gamer sit yet. default is TRUE.
    }
    return IsGamerAllowLookon(nUserId);
}

// 获取游戏桌玩家的ID
int GetGamerId(int table, int seat)
{
    return g_GameRoomWnd.GetGamerId(table, seat);
}

// 获取游戏桌旁观者的ID数组指针
int* GetLookonIdsArrayPtr(int table, int seat)
{
    return g_GameRoomWnd.GetLookonIdsArrayPtr(table, seat);
}

// 本地玩家的ID
int GetLocalUserId(void)
{
    return g_nLocalPlayerId;
}

// 本地玩家状态
PLAYER_STATE GetLocalUserState(void)
{
    if (IS_INVALID_USER_ID(g_nLocalPlayerId)) {
        return STATE_IDLE;
    }

    return g_PlayerData[g_nLocalPlayerId].playerInfo.state;
}

// 本地玩家所在游戏桌号
int GetLocalUserTableId(void)
{
    if (IS_INVALID_USER_ID(g_nLocalPlayerId)) {
        return INVALID_TABLE;
    }

    return g_PlayerData[g_nLocalPlayerId].playerInfo.table;
}

// 本地玩家所在椅子号
int GetLocalUserSeatId(void)
{
    if (IS_INVALID_USER_ID(g_nLocalPlayerId)) {
        return INVALID_SEAT;
    }

    return g_PlayerData[g_nLocalPlayerId].playerInfo.seat;
}

// 用户性别
PLAYER_GENDER GetUserGender(int nUserId)
{
    assert(!IS_INVALID_USER_ID(nUserId));
    return g_PlayerData[nUserId].playerInfo.gender;
}

// 玩家性别
PLAYER_GENDER GetGamerGender(int table, int seat)
{
    assert(!IS_INVALID_TABLE(table));
    assert(!IS_INVALID_SEAT(seat));

    int nGamerId = GetGamerId(table, seat);
    return GetUserGender(nGamerId);
}