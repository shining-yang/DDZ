//
// MsgDef.h
//
//  斗地主游戏服务器与客户端通信消息定义，以及其它服务器与客户端相同的数据类型定义。
//  不同的消息码，其消息体的结构也不相同，具体参考相关文档。
//
#pragma once


// 大厅消息码
//#define NMSG_ACK_SVR_CONNECTION_INFO        0x00    // S->C 服务器应答当前负载情况（当前在线人数，最大连接容量）
//#define NMSG_REQ_SVR_CONNECTION_INFO        0x01    // C->S 客户端请求服务当前运行状况，是否已人满为患

#define NMSG_REQ_ROOM_INFO                  0x10    // C->S 客户端登陆后请求大厅信息（客户端携带自己的信息）
#define NMSG_ACK_ROOM_INFO                  0x11    // S->C 服务器应答大厅信息

#define NMSG_PLAYER_CHANGE_STATE            0x20    // S->C 服务器通知客户端所有状态发生变化的玩家信息（定时广播）


#define NMSG_REQ_GAMER_TAKE_SEAT            0x40    // C->S 客户端（玩家角色）请求坐入位子
#define NMSG_ACK_GAMER_TAKE_SEAT            0x41    // S->C 服务器应答（游戏桌内广播），若玩家收到该应答消息是自己所请求的，表示可以坐下
#define NMSG_REQ_LOOKON_TAKE_SEAT           0x42    // C->S 客户端（旁观者角色）请求旁观某个玩家
#define NMSG_ACK_LOOKON_TAKE_SEAT           0x43    // S->C 服务器应答（游戏桌内广播），若玩家收到该应答消息是自己所请求的，表示可以坐下
#define NMSG_REQ_GAMER_LEAVE_SEAT           0x44    // C->S 客户端（玩家角色）告诉服务器，已经离开某个座位
#define NMSG_ACK_GAMER_LEAVE_SEAT           0x45    // S->C 服务器告诉游戏桌其它所有玩家，某个游戏玩家离开某个座位
#define NMSG_REQ_LOOKON_LEAVE_SEAT          0x46    // C->S 客户端（旁观者角色）告诉服务器，已经离开某个座位
#define NMSG_ACK_LOOKON_LEAVE_SEAT          0x47    // S->C 服务器告诉游戏桌其它所有玩家，某个旁观者已经离开某个座位
#define NMSG_REQ_GAMER_READY                0x48    // C->S 客户端通知服务器，已经准备开始游戏
#define NMSG_ACK_GAMER_READY                0x49    // S->C 服务器通知游戏桌其它客户端，某玩家准备就绪

// 游戏桌消息码
#define NMSG_DISTRIBUTE_CARD                0x80    // S->C 服务器给客户端发牌
#define NMSG_REQ_VOTE_LORD                  0x81    // S->C 服务器请求客户端叫分
#define NMSG_ACK_VOTE_LORD                  0x82    // C->S 客户端应答服务器叫几分
#define NMSG_VOTE_LORD                      0x83    // S->C 服务器通知客户端，某玩家叫几分
#define NMSG_VOTE_LORD_FINISH               0x84    // S->C 服务器通知客户端，地主已经产生，叫的是几分
#define NMSG_REQ_OUTPUT_CARD                0x85    // S->C 服务器请求客户端出牌
#define NMSG_ACK_OUTPUT_CARD                0x86    // C->S 客户端应答服务器出牌
#define NMSG_OUTPUT_CARD                    0x87    // S->C 服务器通知客户端，哪个玩家出了什么牌
#define NMSG_GAME_OVER                      0x88    // S->C 服务器通知客户端游戏结束

#define NMSG_CHATTING                       0x6A    // C->S | S->C 聊天消息
#define NMSG_CONNECTION_LOST                0x6B    // S->C 服务器通知该游戏桌其它客户端，某玩家掉线
#define NMSG_REQ_DELEGATE                   0x6C    // C->S 客户端请求托管游戏
#define NMSG_ACK_DELEGATE                   0x6D    // S->C 服务器通知该游戏桌其它客户端，某玩家已托管游戏
#define NMSG_REQ_STOP_GAMING                0x6E    // C->S 客户端请求中途退出游戏
                                                    // S->C （服务器转发）服务器征求其它玩家意见，某玩家要中途退出
#define NMSG_ACK_STOP_GAMING                0x6F    // S->C 服务器应答客户端中途退出游戏的请求

#define NMSG_REQ_FORCE_STOP_GAMING          0x70    // C->S 客户端通知服务器，强行中断游戏
#define NMSG_ACK_FORCE_STOP_GAMING          0x71    // S->C 服务器通知游戏桌成员，某玩家强行中断游戏

// 游戏设置消息码
#define NMSG_ALLOW_LOOKON                   0xA0    // C->S | S->C 是否允许旁观
//#define NMSG_TABLE_PASSWORD                 0xA1    // C->S 客户端（第一个入位的玩家）要求设置游戏桌密码

// 系统消息码
#define NMSG_BROADCASTING                   0xFF    // S->C 服务器广播一些信息


// 玩家性别
typedef enum PLAYER_GENDER_t {
    MALE,
    FEMALE
} PLAYER_GENDER;

// 玩家状态
typedef enum {
    STATE_IDLE = 0,
    STATE_SIT,
    STATE_READY,
    STATE_GAMING,
    STATE_LOOKON
} PLAYER_STATE;

// 描述客户端玩家进入房间、离开房间、改变状态的事件
// 服务器定时将这些事件发送给在线客户端，这样，客户端才能获取最新的大厅的成员信息
typedef enum {
    EVT_ENTER,
    EVT_LEAVE,
    EVT_CHGSTATE
} STATE_CHANGE_EVENT;

