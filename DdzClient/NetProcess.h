//
// File: NetProc.h
//
// 处理与服务器的网络通信
//
#pragma once

#define RECV_DATA_BUF_SZ            2048


BOOL InitWinSocket(void);
void CloseWinSock(void);

SOCKET EstablishConnection(DWORD ip, USHORT port);
void CloseConnection(SOCKET sock);

void PostMessageToMain(UINT nMsg, WPARAM wParam, LPARAM lParam);
int  SendDataToServer(BYTE* buf, int len);

DWORD ConnectionReceiveDataProc(LPVOID lpData);

void OnNetMessage(BYTE* buf, int len);

void On_NMSG_ACK_ROOM_INFO(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_PLAYER_CHANGE_STATE(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_GAMER_TAKE_SEAT(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_LOOKON_TAKE_SEAT(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_GAMER_LEAVE_SEAT(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_LOOKON_LEAVE_SEAT(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_GAMER_READY(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_REQ_VOTE_LORD(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_VOTE_LORD(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_VOTE_LORD_FINISH(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_DISTRIBUTE_CARD(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_OUTPUT_CARD(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_REQ_OUTPUT_CARD(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_GAME_OVER(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_CHATTING(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_CONNECTION_LOST(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_DELEGATE(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_REQ_STOP_GAMING(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_STOP_GAMING(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ACK_FORCE_STOP_GAMING(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_ALLOW_LOOKON(BYTE* pMsgBody, int nMsgBodyLen);
void On_NMSG_BROADCASTING(BYTE* pMsgBody, int nMsgBodyLen);

int SendReqRoomInfo(void);
int SendReqGamerTakeSeat(int table, int seat);
int SendReqLookonTakeSeat(int table, int seat);
int SendReqGamerLeaveSeat(int table, int seat);
int SendReqLookonLeaveSeat(int table, int seat);
int SendReqGamerReady(int table, int seat);
int SendAckVoteLord(int seat, int score);
int SendAckOutputCard(int seat, POKER_PROPERTY* pp, int poker[]);
int SendChatMessage(int seat, LPCTSTR lpszChatText);
int SendChatMessage(int seat, int nMsgIndex);
int SendReqDelegate(int seat, BOOL bDelegate);
int SendReqStopGaming(int seat, LPCTSTR lpszTextReason);
int SendAckStopGaming(int seat, int reqSeat, BOOL bPermit, LPCTSTR lpszTextReason);
int SendReqForceStopGaming(int seat);
