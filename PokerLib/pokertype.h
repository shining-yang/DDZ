//
// pokertype.h : 出牌的类型
//
#ifndef _POKER_TYPE_H_
#define _POKER_TYPE_H_


// 将以下内容移到 PokerLib.h 文件

//// 出牌的类型
//typedef enum {
//    SINGLE = 1,             // 单张
//    SERIES,                 // 顺子
//    PAIR,                   // 对子
//    SERIES_PAIR,            // 连对
//    TRIANGLE,               // 三张
//    SERIES_TRIANGLE,        // 连三张
//    THREE_PLUS_ONE,         // 三带一
//    SERIES_THREE_PLUS_ONE,  // 连三带一
//    THREE_PLUS_TWO,         // 三带二
//    SERIES_THREE_PLUS_TWO,  // 连三带二
//    SERIES_FOUR,            // 连四（两个炸弹废了）
//    FOUR_PLUS_TWO,          // 四带二
//    SERIES_FOUR_PLUS_TWO,   // 连四带二
//    FOUR_PLUS_FOUR,         // 四带四
//    SERIES_FOUR_PLUS_FOUR,  // 连四带四
//    BOMB,                   // 炸弹
//} POKER_TYPE;
//
//// 出牌的详细情况
//typedef struct POKER_PROPERTY_s {
//    POKER_TYPE      type;       // 牌的类型
//    int             value;      // 牌的值
//    int             num;        // 牌个数
//} POKER_PROPERTY;
//
//
//
//POKER_API 
//char* poker_type_to_string(POKER_TYPE pt);
//
//POKER_API 
//bool can_play_poker(POKER_PROPERTY* pp, int vec[], int num);
//
//POKER_API 
//bool can_follow_poker(POKER_PROPERTY* pp, int vec[], int num, POKER_PROPERTY* req);

#endif