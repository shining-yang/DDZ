//
// File: spec.h
//
//  斗地主游戏软件规格。服务器与客户端均须遵循这些定义的参数。
//

// 服务器默认侦听网络连接的端口号
#define DDZSERVER_PORT                      26008

// 服务器最大允许连接数量
#define MAX_CONNECTION_NUM                  300

// 服务器支持的游戏桌数量
#define GAME_TABLE_NUM                      100

// 每张游戏桌中的椅子数量
#define GAME_SEAT_NUM_PER_TABLE             3

// 每张椅子最多允许旁观者数量
#define MAX_LOOKON_NUM_PER_SEAT             2

// 服务器接受玩家名字的最大字符个数
#define MAX_USER_NAME_LEN                   32

// 扑克牌相关参数
#define TOTAL_CARDS_NUM                     54
#define PLAYER_MAX_CARDS_NUM                20
#define PLAYER_INIT_CARDS_NUM               17
#define UNDER_CARDS_NUM                     3

// 游戏开始后，若没有玩家叫地主，则服务器重新发牌。
// 若玩家都不叫地主的次数超过如下值，则服务器不重新发牌，
// 并将游戏桌玩家设置为就坐状态，玩家需重新点击开始才能进入准备状态。
// 该定义也是为了避免，当所有玩家都托管游戏时（不叫地主），服务器会不停的重新发牌。
#define SERVER_REDISTR_POKER_TIMES          2

#define INVALID_USER_ID                     -1
#define IS_INVALID_USER_ID(id)              (((id) < 0) || ((id) >= MAX_CONNECTION_NUM))

#define INVALID_TABLE                       -1
#define IS_INVALID_TABLE(table)             (((table) < 0) || ((table) >= GAME_TABLE_NUM))

#define INVALID_SEAT                        -1
#define IS_INVALID_SEAT(seat)               (((seat)  < 0) || ((seat)  >= GAME_SEAT_NUM_PER_TABLE))

