//
// pokertype.cpp : 出牌的类型
//
// 检查所打出的牌的类型时，被检查的牌必须是按升序排列好的。
//
// ISSUE:
//      如果上家出牌为“333444555JQK”，下家出牌为“666777888999”为合法规则;
//      如果玩家首次出牌为“666777888999”则视为连三，而不是连三带一。
//      为解决这个问题，首次出牌与接牌分两个函数实现。
//
// Remark:
//      炸弹不拆分原则：
//      例如，上家出牌为“3334447799”，下家出牌“JJJJQQQKKK”为不合法出牌；
//      另外，如果上家出牌为“33344468”，下家出牌“77778889”也为不合法出牌。
//
//      即，成炸的四张牌都出现在连三带二或连四带四，则视为不合法的出牌；
//      但双王炸弹可以都同时出现在其它牌型的附属。
//
#include "stdafx.h"
#include "error.h"
#include "poker.h"
#include "pokertype.h"
#include "PokerLib.h"


extern POKER poker[];

//
// used for diagnostics
//
POKER_API 
TCHAR* poker_type_to_string(POKER_TYPE pt)
{
    switch (pt) {
    case SINGLE:
        return _T("SINGLE");

    case SERIES:
        return _T("SERIES");

    case PAIR:
        return _T("PAIR");

    case SERIES_PAIR:
        return _T("SERIES_PAIR");

    case TRIANGLE:
        return _T("TRIANGLE");

    case SERIES_TRIANGLE:
        return _T("SERIES_TRIANGLE");

    case THREE_PLUS_ONE:
        return _T("THREE_PLUS_ONE");

    case SERIES_THREE_PLUS_ONE:
        return _T("SERIES_THREE_PLUS_ONE");

    case THREE_PLUS_TWO:
        return _T("THREE_PLUS_TWO");

    case SERIES_THREE_PLUS_TWO:
        return _T("SERIES_THREE_PLUS_TWO");

    case SERIES_FOUR:
        return _T("SERIES_FOUR");

    case FOUR_PLUS_TWO:
        return _T("FOUR_PLUS_TWO");

    case SERIES_FOUR_PLUS_TWO:
        return _T("SERIES_FOUR_PLUS_TWO");

    case FOUR_PLUS_FOUR:
        return _T("FOUR_PLUS_FOUR");

    case SERIES_FOUR_PLUS_FOUR:
        return _T("SERIES_FOUR_PLUS_FOUR");

    case BOMB:
        return _T("BOMB");
    }

    return _T("INVALID_POKER_TYPE");
}

// 单张
bool is_single(int vec[], int num, int* val)
{
    if (num != 1) { return false; }

    *val = poker[vec[0]].value;

    return true;
}

// 对子
bool is_pair(int vec[], int num, int* val)
{
    if (num != 2) { return false; }

    if (poker[vec[0]].value == poker[vec[1]].value) {
        *val = poker[vec[1]].value;
        return true;
    }

    return false;
}

// 双王炸弹
bool is_king_bomb(int vec[], int num, int* val)
{
    if (num != 2) { return false; }

    if ((poker[vec[0]].unit == PJOKER0)
        && (poker[vec[1]].unit == PJOKER1)) {
            *val = poker[vec[1]].value;
            return true;
    }

    return false;
}

// 三张
bool is_triangle(int vec[], int num, int* val)
{
    if (num != 3) { return false; }

    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    return false;
}

// 炸弹
bool is_four_bomb(int vec[], int num, int* val)
{
    if (num != 4) { return false; }

    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)
        && (poker[vec[0]].value == poker[vec[3]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    return false;
}

// 三带一
bool is_three_plus_one(int vec[], int num, int* val)
{
    if (num != 4) { return false; }

    // "333A"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)
        && (poker[vec[0]].value != poker[vec[3]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    // "3AAA"
    if ((poker[vec[0]].value != poker[vec[1]].value)
        && (poker[vec[1]].value == poker[vec[2]].value)
        && (poker[vec[1]].value == poker[vec[3]].value)) {
            *val = poker[vec[1]].value;
            return true;
    }

    return false;
}

// 三带二
bool is_three_plus_two(int vec[], int num, int* val)
{
    if (num != 5) { return false; }

    // "33344"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)
        && (poker[vec[3]].value == poker[vec[4]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    // "33444"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[2]].value == poker[vec[3]].value)
        && (poker[vec[2]].value == poker[vec[4]].value)) {
            *val = poker[vec[2]].value;
            return true;
    }

    return false;
}

// 四带二
bool is_four_plus_two(int vec[], int num, int* val)
{
    if (num != 6) { return false; }

#if 0
    for (int i = 0; i < num - 3; i++) {
        if ((poker[vec[i]].value == poker[vec[i + 1]].value)
            && (poker[vec[i]].value == poker[vec[i + 2]].value)
            && (poker[vec[i]].value == poker[vec[i + 3]].value)) {
                *val = poker[vec[i]].value;
                return true;
        }
    }
#else
    // "333345"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)
        && (poker[vec[0]].value == poker[vec[3]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    // "344445"
    if ((poker[vec[1]].value == poker[vec[2]].value)
        && (poker[vec[1]].value == poker[vec[3]].value)
        && (poker[vec[1]].value == poker[vec[4]].value)) {
            *val = poker[vec[1]].value;
            return true;
    }

    // "345555"
    if ((poker[vec[2]].value == poker[vec[3]].value)
        && (poker[vec[2]].value == poker[vec[4]].value)
        && (poker[vec[2]].value == poker[vec[5]].value)) {
            *val = poker[vec[2]].value;
            return true;
    }
#endif

    return false;
}

// 四带四
bool is_four_plus_four(int vec[], int num, int* val)
{
    if (num != 8) { return false; }

    // "33334455"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value == poker[vec[2]].value)
        && (poker[vec[0]].value == poker[vec[3]].value)
        && (poker[vec[4]].value == poker[vec[5]].value)
        && (poker[vec[4]].value != poker[vec[6]].value)
        && (poker[vec[6]].value == poker[vec[7]].value)) {
            *val = poker[vec[0]].value;
            return true;
    }

    // "33444455"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[2]].value == poker[vec[3]].value)
        && (poker[vec[2]].value == poker[vec[4]].value)
        && (poker[vec[2]].value == poker[vec[5]].value)
        && (poker[vec[6]].value == poker[vec[7]].value)) {
            *val = poker[vec[2]].value;
            return true;
    }

    // "33445555"
    if ((poker[vec[0]].value == poker[vec[1]].value)
        && (poker[vec[0]].value != poker[vec[2]].value)
        && (poker[vec[2]].value == poker[vec[3]].value)
        && (poker[vec[4]].value == poker[vec[5]].value)
        && (poker[vec[4]].value == poker[vec[6]].value)
        && (poker[vec[4]].value == poker[vec[7]].value)) {
            *val = poker[vec[4]].value;
            return true;
    }

    return false;
}

// 顺子（连子）
bool is_series(int vec[], int num, int* val)
{
    int i;

    if ((num < 5) || (num > 12)) { return false; }

    // 最大那张牌不能为“2,王”
    if ((poker[vec[num - 1]].unit == P2)
        || (poker[vec[num - 1]].unit == PJOKER0)
        || (poker[vec[num - 1]].unit == PJOKER1)) {
            return false;
    }

    for (i = num - 1; i > 0; i--) {
        if (poker[vec[i]].value - poker[vec[i - 1]].value != 1) {
            return false;
        }
    }

    *val = poker[vec[num - 1]].value;
    return true;
}

// 连对
bool is_series_pair(int vec[], int num, int* val)
{
    int i;

    if (num % 2 != 0) { return false; }
    if ((num < 6) || (num > 20)) { return false; }

    // 最大那张牌不能为“2,王”
    if ((poker[vec[num - 1]].unit == P2)
        || (poker[vec[num - 1]].unit == PJOKER0)
        || (poker[vec[num - 1]].unit == PJOKER1)) {
            return false;
    }

    for (i = 0; i < num - 1; i += 2) {
        if (poker[vec[i]].value != poker[vec[i + 1]].value) {
            return false;
        }
    }

    for (i = num - 1; i > 2; i -= 2) {
        if (poker[vec[i]].value - poker[vec[i - 2]].value != 1) {
            return false;
        }
    }

    *val = poker[vec[num - 1]].value;
    return true;
}

// 连三
bool is_series_triangle(int vec[], int num, int* val)
{
    int i;

    if (num % 3 != 0) { return false; }
    if ((num < 6) || (num > 18)) { return false; }

    // 最大那张牌不能为“2,王”
    if ((poker[vec[num - 1]].unit == P2)
        || (poker[vec[num - 1]].unit == PJOKER0)
        || (poker[vec[num - 1]].unit == PJOKER1)) {
            return false;
    }

    for (i = 0; i < num - 2; i += 3) {
        if ((poker[vec[i]].value != poker[vec[i + 1]].value)
            || (poker[vec[i]].value != poker[vec[i + 2]].value)) {
                return false;
        }
    }

    for (i = num - 1; i > 3; i -= 3) {
        if (poker[vec[i]].value - poker[vec[i - 3]].value != 1) {
            return false;
        }
    }

    *val = poker[vec[num - 1]].value;
    return true;
}

// 连四
bool is_series_four(int vec[], int num, int* val)
{
    int i;

    if (num % 4 != 0) { return false; }
    if ((num < 8) || (num > 20)) { return false; };

    // 最大那张牌不能为“2,王”
    if ((poker[vec[num - 1]].unit == P2)
        || (poker[vec[num - 1]].unit == PJOKER0)
        || (poker[vec[num - 1]].unit == PJOKER1)) {
            return false;
    }

    for (i = 0; i < num - 3; i += 4) {
        if ((poker[vec[i]].value != poker[vec[i + 1]].value)
            || (poker[vec[i]].value != poker[vec[i + 2]].value)
            || (poker[vec[i]].value != poker[vec[i + 3]].value)) {
                return false;
        }
    }

    for (i = num - 1; i > 4; i -= 4) {
        if (poker[vec[i]].value - poker[vec[i - 4]].value != 1) {
            return false;
        }
    }

    *val = poker[vec[num - 1]].value;
    return true;
}

// 连三带一
bool is_series_three_plus_one(int vec[], int num, int* val)
{
    int i, n, v;

    if (num % 4 != 0) { return false; }
    if ((num < 8) || (num > 20)) { return false; }

    n = num / 4; // 如果是连三带一，则有n个连三张，或说明有n个单张 (1<n<6)

    // 此算法将视“3333444555666777”，“3344445555666777”，“3334445556667777”
    // 为不合法的出牌。（遵循炸弹不拆的原则）

    // 从高到低查找这n个连三带一，如“3344455566677788”被视为有效的连三带一，
    // 其值为7

    for (i = 0; i < num - 3; i++) {
        if ((poker[vec[i]].value == poker[vec[i + 1]].value)
            && (poker[vec[i]].value == poker[vec[i + 2]].value)
            && (poker[vec[i]].value == poker[vec[i + 3]].value)) {
                return false;
        }
    }

    for (i = n; i >= 0; i--) {
        if (is_series_triangle(vec + i, n * 3, &v)) {
            *val = v;
            return true;
        }
    }

    return false;
}

// 连三带二
bool is_series_three_plus_two(int vec[], int num, int* val)
{
    int i, j, n, v;

    if (num % 5 != 0) { return false; }
    if ((num < 10) || (num > 20)) { return false; }

    n = num / 5; // 如果是连三带二，则有n个连三张，或说明有n个对子 (1<n<5)

    // 在此算法中，将视“333344455566677”和“334445556667777”为不合法出牌。
    // （遵循炸弹不拆的原则）

    // 从高到低查找这n个连三带二，若存在连三，则还需判断其它牌是否都是对子。
    // 如“3344555666777888AA22”被视为有效的连三带二，其值为7

    for (i = 0; i < num - 3; i++) {
        if ((poker[vec[i]].value == poker[vec[i + 1]].value)
            && (poker[vec[i]].value == poker[vec[i + 2]].value)
            && (poker[vec[i]].value == poker[vec[i + 3]].value)) {
                return false;
        }
    }

    for (i = n * 2; i >= 0; i -= 2) {
        if (is_series_triangle(vec + i, n * 3, &v)) {
            for (j = 0; j < i - 1; j += 2) {
                if (poker[vec[j]].value != poker[vec[j + 1]].value) {
                    return false;
                }
            }
            
            for (j = i + n * 3; j < num - 1; j += 2) {
                if (poker[vec[j]].value != poker[vec[j + 1]].value) {
                    return false;
                }
            }

            *val = v;
            return true;
        }
    }

    return false;
}

// 连四带二
bool is_series_four_plus_two(int vec[], int num, int* val)
{
    int i, j, n, v;

    if (num % 6 != 0) { return false; }
    if ((num < 12) || (num > 18)) { return false; }

    n = num / 6;

    // （遵循炸弹不拆的原则）
    // 从高至低查找连四，如果找到连四，再判断两头两尾是否存在炸弹。
    // 如“4444666677778888JQ”，“34777788889999AAAA”不是合法的连四带二。
    // 但“4445666677778888JQ”，“34777788889999AAA2”是合法的连四带二。

    for (i = n * 2; i >= 0; i--) {
        if (is_series_four(vec + i, n * 4, &v)) {
            for (j = 0; j < i - 3; j++) {
                if ((poker[vec[j]].value == poker[vec[j + 1]].value)
                    && (poker[vec[j]].value == poker[vec[j + 2]].value)
                    && (poker[vec[j]].value == poker[vec[j + 3]].value)) {
                        return false;
                }
            }

            for (j = i + n * 4; j < num - 3; j++) {
                if ((poker[vec[j]].value == poker[vec[j + 1]].value)
                    && (poker[vec[j]].value == poker[vec[j + 2]].value)
                    && (poker[vec[j]].value == poker[vec[j + 3]].value)) {
                        return false;
                }
            }

            *val = v;
            return true;
        }
    }

    return false;
}

// 连四带四
bool is_series_four_plus_four(int vec[], int num, int* val)
{
    int i, j, n, v;

    if (num != 16) { return false; }

    n = 2;

    // 如果是连四带四，只能是16张牌，且只能是两个连四。

    // （遵循炸弹不拆的原则）
    // 先从高至低查找到连四，再判断两头两尾是否存在炸弹，最后判断其它牌都成对子。
    // 如“444466667777JJQQ”，“334477778888AAAA”不是合法的连四带四。

    for (i = n * 4; i >= 0; i -= 2) {
        if (is_series_four(vec + i, n * 4, &v)) {
            for (j = 0; j < i - 3; j++) {
                if ((poker[vec[j]].value == poker[vec[j + 1]].value)
                    && (poker[vec[j]].value == poker[vec[j + 2]].value)
                    && (poker[vec[j]].value == poker[vec[j + 3]].value)) {
                        return false;
                }
            }

            for (j = i + n * 4; j < num - 3; j++) {
                if ((poker[vec[j]].value == poker[vec[j + 1]].value)
                    && (poker[vec[j]].value == poker[vec[j + 2]].value)
                    && (poker[vec[j]].value == poker[vec[j + 3]].value)) {
                        return false;
                }
            }

            for (j = 0; j < i - 1; j += 2) {
                if (poker[vec[j]].value != poker[vec[j + 1]].value) {
                    return false;
                }
            }

            for (j = i + n * 4; j < num - 1; j += 2) {
                if (poker[vec[j]].value != poker[vec[j + 1]].value) {
                    return false;
                }
            }

            *val = v;
            return true;
        }
    }

    return false;
}

//
// Function:
//      get_poker_property
// Description:
//      获取出牌的相关属性
// Parameter:
//      pp  - [out] poker property if succeed
//      vec - poker vector to be analyzed. The element are indexes to GLOBAL poker.
//      num - poker num to be analyzed
// Return:
//      E_NONE if everything is OK, else the pokers are not valid.
// Remark:
//      the poker vector in array vec should be sorted in ascend.
//
POKER_RET get_poker_property(POKER_PROPERTY* pp, int vec[], int num)
{
    int value;

    assert(pp != NULL);

    if (is_single(vec, num, &value)) {
        pp->type = SINGLE;
        goto match_succeed;
    } else if (is_pair(vec, num, &value)) {
        pp->type = PAIR;
        goto match_succeed;
    } else if (is_king_bomb(vec, num, &value)) {
        pp->type = BOMB;
        goto match_succeed;
    } else if (is_triangle(vec, num, &value)) {
        pp->type = TRIANGLE;
        goto match_succeed;
    } else if (is_four_bomb(vec, num, &value)) {
        pp->type = BOMB;
        goto match_succeed;
    } else if (is_three_plus_one(vec, num, &value)) {
        pp->type = THREE_PLUS_ONE;
        goto match_succeed;
    } else if (is_three_plus_two(vec, num, &value)) {
        pp->type = THREE_PLUS_TWO;
        goto match_succeed;
    } else if (is_four_plus_two(vec, num, &value)) {
        pp->type = FOUR_PLUS_TWO;
        goto match_succeed;
    } else if (is_four_plus_four(vec, num, &value)) {
        pp->type = FOUR_PLUS_FOUR;
        goto match_succeed;
    } else if (is_series(vec, num, &value)) {
        pp->type = SERIES;
        goto match_succeed;
    } else if (is_series_pair(vec, num, &value)) {
        pp->type = SERIES_PAIR;
        goto match_succeed;
    } else if (is_series_triangle(vec, num, &value)) {
        pp->type = SERIES_TRIANGLE;
        goto match_succeed;
    } else if (is_series_three_plus_one(vec, num, &value)) {
        pp->type = SERIES_THREE_PLUS_ONE;
        goto match_succeed;
    } else if (is_series_three_plus_two(vec, num, &value)) {
        pp->type = SERIES_THREE_PLUS_TWO;
        goto match_succeed;
    } else if (is_series_four(vec, num, &value)) {
        pp->type = SERIES_FOUR;
        goto match_succeed;
    } else if (is_series_four_plus_two(vec, num, &value)) {
        pp->type = SERIES_FOUR_PLUS_TWO;
        goto match_succeed;
    } else if (is_series_four_plus_four(vec, num, &value)) {
        pp->type = SERIES_FOUR_PLUS_FOUR;
        goto match_succeed;
    } else {
        return E_INVALID;
    }

match_succeed:
    pp->value = value;
    pp->num = num;

    return E_NONE;
}

//
// Function:
//      can_play_poker
// Description:
//      检查要出的牌是否合法。如果合法，则将牌的相关属性保存在pp。（用于首次出牌）
// Parameter:
//      pp  - 用于保存扑克牌序列的属性
//      vec - 扑克牌的索引数组，其元素为指向POKER的索引，取值范围为0～53
//      num - 索引数组的长度
// Return:
//      操作成功返回true，否则返回false.
// Remark:
//      调用者需判断该函数的返回值，若返回false，表示无法判断牌的属性，该组牌
//      为不合法序列。
//      传入的参数 vec 为全局扑克牌的索引，而不是玩家当前牌的索引，否则，调用
//      该函数前需要进行转换。
//
POKER_API 
bool can_play_poker(POKER_PROPERTY* pp, int vec[], int num)
{
    assert(pp != NULL);

    return (E_NONE == get_poker_property(pp, vec, num)) ? true : false;
}

//
// Function:
//      can_follow_poker
// Description:
//      根据请求，检查要出的牌是否合法。如果合法，则将牌的属性保存在pp。（用于接牌）
// Parameter:
//      pp  - 用于保存扑克牌序列的属性
//      vec - 扑克牌的索引数组，其元素为指向POKER的索引，取值范围为0～53
//      num - 索引数组的长度
//      req - 请求出牌的属性
// Return:
//      操作成功返回true，否则返回false.
// Remark:
//      调用者需判断该函数的返回值，若返回false，表示无法判断牌的属性，该组牌
//      为不合法序列。
//      传入的参数 vec 为全局扑克牌的索引，而不是玩家当前牌的索引，否则，调用
//      该函数前需要进行转换。
//
POKER_API 
bool can_follow_poker(POKER_PROPERTY* pp, int vec[], int num, POKER_PROPERTY* req)
{
    int value;

    assert((pp != NULL) && (req != NULL));

    // 当前要出的牌是双王炸弹
    if (is_king_bomb(vec, num, &value)) {
        pp->type = BOMB;
        goto can_follow;
    }

    // 当前要出的牌是炸弹
    if (is_four_bomb(vec, num, &value)) {
        if ((req->type != BOMB) || ((req->type == BOMB) && (value > req->value))) {
            pp->type = BOMB;
            goto can_follow;
        } else {
            return false;
        }
    }

    // 当前要出的牌不是炸弹，但上家的牌是炸弹
    if (req->type == BOMB) {
        return false;
    }
    
    // 上家的牌不是炸弹，当前要打出的牌也不是炸弹
    if (num != req->num) {
        return false;
    }

    //
    // 到这里，说明上家出的牌与当前要出的牌都不是炸弹，并且数量相等，因此，
    // 只需要比较它们的类型和值即可。
    //
    switch (req->type) {
        case SINGLE:
            if (is_single(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case PAIR:
            if (is_pair(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case TRIANGLE:
            if (is_triangle(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case THREE_PLUS_ONE:
            if (is_three_plus_one(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case THREE_PLUS_TWO:
            if (is_three_plus_two(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case FOUR_PLUS_TWO:
            if (is_four_plus_two(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case FOUR_PLUS_FOUR:
            if (is_four_plus_four(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES:
            if (is_series(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_PAIR:
            if (is_series_pair(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_TRIANGLE:
            if (is_series_triangle(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_THREE_PLUS_ONE:
            if (is_series_three_plus_one(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_THREE_PLUS_TWO:
            if (is_series_three_plus_two(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_FOUR:
            if (is_series_four(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_FOUR_PLUS_TWO:
            if (is_series_four_plus_two(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        case SERIES_FOUR_PLUS_FOUR:
            if (is_series_four_plus_four(vec, num, &value)) {
                goto type_matched;
            } else {
                return false;
            }

        default:
            return false;
    }

type_matched:
    if (value <= req->value) {
        return false;
    }

    pp->type = req->type;

can_follow:
    pp->value = value;
    pp->num = num;

    return true;
}