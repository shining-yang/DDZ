// poker.cpp : implementation for the poker
//
//
// 扑克：斗地主（一副牌）
//
// 扑克牌使用组数索引值描述，任何操作都默认要求扑克牌序列是升序的，请注意。
//

#include "stdafx.h"
#include "poker.h"
#include "quicksort.h"
#include "PokerLib.h"

POKER_API 
POKER poker[POKER_ITEM_COUNT] = {
    { P3,   DIAMOND,    3 },
    { P3,   CLUB,       3 },
    { P3,   HEART,      3 },
    { P3,   SPADE,      3 },

    { P4,   DIAMOND,    4 },
    { P4,   CLUB,       4 },
    { P4,   HEART,      4 },
    { P4,   SPADE,      4 },

    { P5,   DIAMOND,    5 },
    { P5,   CLUB,       5 },
    { P5,   HEART,      5 },
    { P5,   SPADE,      5 },

    { P6,   DIAMOND,    6 },
    { P6,   CLUB,       6 },
    { P6,   HEART,      6 },
    { P6,   SPADE,      6 },

    { P7,   DIAMOND,    7 },
    { P7,   CLUB,       7 },
    { P7,   HEART,      7 },
    { P7,   SPADE,      7 },

    { P8,   DIAMOND,    8 },
    { P8,   CLUB,       8 },
    { P8,   HEART,      8 },
    { P8,   SPADE,      8 },

    { P9,   DIAMOND,    9 },
    { P9,   CLUB,       9 },
    { P9,   HEART,      9 },
    { P9,   SPADE,      9 },

    { P10,  DIAMOND,    10 },
    { P10,  CLUB,       10 },
    { P10,  HEART,      10 },
    { P10,  SPADE,      10 },

    { PJ,   DIAMOND,    11 },
    { PJ,   CLUB,       11 },
    { PJ,   HEART,      11 },
    { PJ,   SPADE,      11 },

    { PQ,   DIAMOND,    12 },
    { PQ,   CLUB,       12 },
    { PQ,   HEART,      12 },
    { PQ,   SPADE,      12 },

    { PK,   DIAMOND,    13 },
    { PK,   CLUB,       13 },
    { PK,   HEART,      13 },
    { PK,   SPADE,      13 },

    { PA,   DIAMOND,    14 },
    { PA,   CLUB,       14 },
    { PA,   HEART,      14 },
    { PA,   SPADE,      14 },

    { P2,   DIAMOND,    15 },
    { P2,   CLUB,       15 },
    { P2,   HEART,      15 },
    { P2,   SPADE,      15 },

    { PJOKER0,  JOKER0, 16 },

    { PJOKER1,  JOKER1, 17 }
};

POKER_API 
TCHAR poker_unit_to_char(POKER_UNIT unit)
{
    switch (unit) {
        case P3:    return _T('3');
        case P4:    return _T('4');
        case P5:    return _T('5');
        case P6:    return _T('6');
        case P7:    return _T('7');
        case P8:    return _T('8');
        case P9:    return _T('9');
        case P10:   return _T('0');
        case PJ:    return _T('J');
        case PQ:    return _T('Q');
        case PK:    return _T('K');
        case PA:    return _T('A');
        case P2:    return _T('2');
        case PJOKER0:   return 0x01;
        case PJOKER1:   return 0x02;
        default:        return _T(' ');
    }
}

POKER_API 
TCHAR poker_clr_to_char(POKER_CLR clr)
{
    switch (clr) {
        case JOKER0:    return 0x19;
        case JOKER1:    return 0x18;
        case HEART:     return 0x03;
        case DIAMOND:   return 0x04;
        case CLUB:      return 0x05;
        case SPADE:     return 0x06;
        default:        return _T(' ');
    }
}

POKER_API 
TCHAR poker_index_to_char(int index)
{
    if ((index < 0) || (index >= POKER_ITEM_COUNT)) {
        //printf("poker index out of range.\n");
        return _T(' ');
    }

    return poker_unit_to_char(poker[index].unit);
}

POKER_API 
TCHAR poker_index_to_clr_char(int index)
{
    if ((index < 0) || (index >= POKER_ITEM_COUNT)) {
        //printf("poker index out of range.\n");
        return _T(' ');
    }

    return poker_clr_to_char(poker[index].color);
}

POKER_API 
int poker_index_to_value(int index)
{
    if ((index < 0) || (index >= POKER_ITEM_COUNT)) {
        //printf("poker index out of range.\n");
        return 0;
    }

    return poker[index].value;
}

