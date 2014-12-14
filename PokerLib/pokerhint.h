//
// pokerhint.h
//
//      根据上家所出的牌，提示玩家可以拿什么牌去接。
//      使用链表会节省空间，考虑到操作方便，这里使用静态数组结构。
//

#ifndef __POKER_HINT_H_
#define __POKER_HINT_H_

// 以下类型定义移到 PokerLib.h 文件中

//// 单张
//typedef struct HT_SINGLE_t {
//    int     idx;
//    int     value;
//} HT_SINGLE;
//
//// 对子
//typedef struct HT_PAIR_t {
//    int     idx0;
//    int     idx1;
//    int     value;
//} HT_PAIR;
//
//// 三张
//typedef struct HT_TRIANGLE_t {
//    int     idx0;
//    int     idx1;
//    int     idx2;
//    int     value;
//} HT_TRIANGLE;
//
//// 四张
//typedef struct HT_FOUR_t {
//    int     idx0;
//    int     idx1;
//    int     idx2;
//    int     idx3;
//    int     value;
//} HT_FOUR;
//
//// 顺子
//typedef struct HT_SERIES_t {
//    int     idx[12];
//    int     num;
//    int     value;
//} HT_SERIES;
//
//// 连对
//typedef struct HT_SERIES_PAIR_t {
//    int     idx[20];
//    int     num;
//    int     value;
//} HT_SERIES_PAIR;
//
//// 三张
//typedef struct HT_SERIES_TRIANGLE_t {
//    int     idx[18];
//    int     num;
//    int     value;
//} HT_SERIES_TRIANGLE;
//
//// 连四
//typedef struct HT_SERIES_FOUR_t {
//    int     idx[20];
//    int     num;
//    int     value;
//} HT_SERIES_FOUR;
//
//// 双王炸弹
//typedef struct HT_KINGBOMB_t {
//    int     idx0;
//    int     idx1;
//    int     value;
//} HT_KINGBOMB;
//
//// 扑克牌分类表
//typedef struct POKER_CLASS_TABLE_t {
//    int             builded;
//    int             count;
//
//    HT_SINGLE       one[14];
//    int             num1;
//
//    HT_PAIR         two[10];
//    int             num2;
//
//    HT_TRIANGLE     three[6];
//    int             num3;
//
//    HT_FOUR         four[5];
//    int             num4;
//
//    HT_KINGBOMB     kingbomb;
//    int             has_king_bomb;
//
//    HT_SERIES       sone[2];
//    int             num11;
//
//    HT_SERIES_PAIR    stwo[3];
//    int                 num22;
//
//    HT_SERIES_TRIANGLE  sthree[2];
//    int                 num33;
//
//    HT_SERIES_FOUR      sfour[2];
//    int                 num44;
//} POKER_CLASS_TABLE;


// 查找一个炸弹，用于接非炸弹的任何牌
#define RETRIEVE_BOMB()\
{\
    for (i = 0; i < pct->num4; i++) {\
        if (count >= times) {\
            out->type   = BOMB;\
            out->num    = 4;\
            out->value  = pct->four[i].value;\
            vec[0]      = pct->four[i].idx0;\
            vec[1]      = pct->four[i].idx1;\
            vec[2]      = pct->four[i].idx2;\
            vec[3]      = pct->four[i].idx3;\
            return true;\
        } else {\
            count++;\
        }\
    }\
    if (pct->has_king_bomb != 0) {\
        if (count >= times) {\
            out->type   = BOMB;\
            out->num    = 2;\
            out->value  = pct->kingbomb.value;\
            vec[0]      = pct->kingbomb.idx0;\
            vec[1]      = pct->kingbomb.idx1;\
            return true;\
        } else {\
            count++;\
        }\
    }\
}

#endif
