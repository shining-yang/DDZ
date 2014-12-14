//
// pokerhint.cpp
//
//  根据上家所出的牌，提示玩家可以拿什么牌去接。
//
//  设计思想：
//      将玩家手上的牌进行分类，依次分出单张、对子、三张、四张、顺子、连对、
//      连三、连四的类别；然后根据上家所出的牌的类型，构造出相应的提示牌型。
//
//      数据结构使用数组，这比较耗费空间，但操作简单些。
//      
//      分类表中包含各类牌索引值及其扑克牌的值，这里的索引不是全局的扑克索引，而是
//      玩家当前手中扑克牌的索引，即其取值范围为[0～19]。
//      
//      1，
//      查找出牌提示时，需要先构建扑克牌分类表。当玩家出牌后，牌的数量发生变化，
//      所以，下次查找提示时，需要重新构建扑克分类表。
//
//      2，
//      根据请求牌型，先从匹配的类别中查找，其次选择拆分以构造出与请求匹配的牌型，
//      最后提示是否有炸弹。例如，上家出牌为单张“3”，而玩家手中的牌为“4456”，
//      则优先提示“5”，其次提示“6”，最后提示“4”。
//      
//      3,
//      可能存在多个提示，而且这些提示根据其优先级应该是循环的。例如，玩家点一下
//      提示按钮，则返回第一个提示；再点一下提示按钮，则返回下一个提示，如此直到
//      最后一个提示，如果玩家再点一下提示按钮，则返回第一个提示。
//
//      当前的程序设计中，不能很快捷地实现该功能。
//      为实现这个功能，可在查找提示时，传入一个累加的计数参数，该参数由调用者控制。
//
//      例如，传入计数为1，则查找到第一个提示时并不返回，而要等到查找到第二个提示时
//      才返回，或者返回 false 表示没有更多的提示。
//
//      对调用者的要求：如果查找提示时返回为 false，调用者应检查一下传入的计数参数
//      是否为 0，（0计数表示首次进行查找），如果不是 0，则需要置计数参数为 0，重新
//      调用查找函数，以进行提示的查找。这样便可实现循环提示功能。这种方法显然不是
//      最优的，附带了一些多余的查找工作。
//
//      换一种设计算法或数据结构，可能会改进查找效率，但斗地主游戏中的牌数量并不多，
//      目前这样实现，不做更改。
//
#include "stdafx.h"
#include "error.h"
#include "mystack.h"
#include "quicksort.h"
#include "poker.h"
#include "pokertype.h"
#include "pokerhint.h"
#include "PokerLib.h"


extern POKER poker[];


// 将单张归类
static void classify_single(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    int i;
    int idx = 0;

    if (num == 1) {
        pct->one[0].idx = 0;
        pct->one[0].value = poker[vec[0]].value;
        pct->num1 = 1;
        return;
    }
 
    for (i = 0; i < num; i++) {
        if (i == 0) { // 开始处
            if (poker[vec[i]].value != poker[vec[i + 1]].value) {
                pct->one[idx].idx = i;
                pct->one[idx].value = poker[vec[i]].value;
                pct->num1++;
                idx++;
            }
        } else if (i == num - 1) { // 末尾处
            if (poker[vec[i]].value != poker[vec[i - 1]].value) {
                pct->one[idx].idx = i;
                pct->one[idx].value = poker[vec[i]].value;
                pct->num1++;
                idx++;
            }
        } else { // 中间
            if ((poker[vec[i]].value != poker[vec[i - 1]].value)
                && (poker[vec[i]].value != poker[vec[i + 1]].value)) {
                pct->one[idx].idx = i;
                pct->one[idx].value = poker[vec[i]].value;
                pct->num1++;
                idx++;
            }
        }
    }
}

// 将对子归类
static void classify_pair(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    int i;
    int idx = 0;

    if (num < 2) { return; }

    for (i = 0; i < num - 1; i++) {
        if (poker[vec[i]].value != poker[vec[i + 1]].value) {
            continue;
        }

        // 当前与下一个相等

        if (i + 1 == num - 1) { // 末尾处的对子
            pct->two[idx].idx0 = i;
            pct->two[idx].idx1 = i + 1;
            pct->two[idx].value = poker[vec[i]].value;
            pct->num2++;
            idx++;
            break;
        } else if (poker[vec[i]].value != poker[vec[i + 2]].value) { // 中间处
            pct->two[idx].idx0 = i;
            pct->two[idx].idx1 = i + 1;
            pct->two[idx].value = poker[vec[i]].value;
            pct->num2++;
            idx++;
            i++;
        } else { // TRIANGLE, not pair
            i += 2;
        }
    }
}

// 将三张归类
static void classify_triangle(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    int i;
    int idx = 0;

    if (num < 3) { return; }

    for (i = 0; i < num - 2; i++) {
        if (poker[vec[i]].value != poker[vec[i + 1]].value) {
            continue;
        }

        // 等于下一个，还得与下下一个比较
        if (poker[vec[i]].value != poker[vec[i + 2]].value) {
            i++;
            continue;
        }

        // 与下下一个也相等
        if (i + 2 == num - 1) { // 末尾处的三张
            pct->three[idx].idx0 = i;
            pct->three[idx].idx1 = i + 1;
            pct->three[idx].idx2 = i + 2;
            pct->three[idx].value = poker[vec[i]].value;
            pct->num3++;
            idx++;
            break;
        } else if (poker[vec[i]].value != poker[vec[i + 3]].value) { // 中间处
            pct->three[idx].idx0 = i;
            pct->three[idx].idx1 = i + 1;
            pct->three[idx].idx2 = i + 2;
            pct->three[idx].value = poker[vec[i]].value;
            pct->num3++;
            idx++;
            i += 2;
        } else { // 四张的炸弹
            i += 3;
        }
    }
}

// 将四张归类
static void classify_four(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    int i;
    int idx = 0;

    if (num < 4) { return; }

    for (i = 0; i < num - 3; i++) {
        if (poker[vec[i]].value == poker[vec[i + 1]].value) {
            if (poker[vec[i]].value == poker[vec[i + 2]].value) {
                if (poker[vec[i]].value == poker[vec[i + 3]].value) {
                    pct->four[idx].idx0 = i;
                    pct->four[idx].idx1 = i + 1;
                    pct->four[idx].idx2 = i + 2;
                    pct->four[idx].idx3 = i + 3;
                    pct->four[idx].value = poker[vec[i]].value;
                    pct->num4++;
                    idx++;
                    i += 3;
                } else {
                    i += 2;
                }
            } else {
                i += 1;
            }
        } else {
            i += 0;
        }
    }
}

// 记录双王炸弹
static void classify_kingbom(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    assert(vec != NULL);

    if (num < 2) { return; }
    
    if (poker[vec[num - 2]].unit == PJOKER0) {
        if (poker[vec[num - 1]].unit == PJOKER1) {
            pct->kingbomb.idx0 = num - 2;
            pct->kingbomb.idx1 = num - 1;
            pct->kingbomb.value = poker[vec[num - 1]].value;
            pct->has_king_bomb = 1;
        }
    }
}

// 查找构成顺子的下一个元素
static bool find_series_next_elem(POKER_CLASS_TABLE* pct, int value, int* index)
{
    int i;

    assert(index != NULL);

    if (value >= 14) { return false; } // A的值为14，已经到顶

    // 在单张类别中查找
    for (i = 0; i < pct->num1; i++) {
        if (pct->one[i].value == value + 1) {
            *index = pct->one[i].idx;
            return true;
        }
    }

    // 在对子类别中查找
    for (i = 0; i < pct->num2; i++) {
        if (pct->two[i].value == value + 1) {
            *index = pct->two[i].idx0;
            return true;
        }
    }

    // 在三张类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value == value + 1) {
            *index = pct->three[i].idx0;
            return true;
        }
    }

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value == value + 1) {
            *index = pct->four[i].idx0;
            return true;
        }
    }

    return false;
}

// 构造所有尽可能长的顺子
static void generate_series(POKER_CLASS_TABLE* pct)
{
    int i, n = 0;
    int index;
    int value = 0;
    int count = 0;
    bool ret = false;

    MyStack<int> s(16);

    if (pct->count < 5) { return; }

    for (value = 2; value <= 14; value++) { // 从3开始查找，直到A为止
        ret = find_series_next_elem(pct, value, &index);
        if (ret == true) {
            s.Push(index);
            count++;
        } else if ((count > 0) && (count < 5)) {
            s.ClearStack();
            count = 0;
        } else if (count >= 5) {
            assert(count <= 12);
            for (i = count - 1; i >= 0; i--) { // 确保出栈后顺子为升序
                pct->sone[n].idx[i] = s.Pop();
            }
            pct->sone[n].value = value;
            pct->sone[n].num = count;
            pct->num11++;
            n++;
            count = 0;
        }        
    }    
}

// 查找构造成连对的下一个对子
static bool find_series_pair_next_elem(POKER_CLASS_TABLE* pct, int value, int* index0, int* index1)
{
    int i;

    assert((index0 != NULL) && (index1 != NULL));
    if (value >= 14) { return false; } // A的值为14，已经到顶

    // 在对子类别中查找
    for (i = 0; i < pct->num2; i++) {
        if (pct->two[i].value == value + 1) {
            *index0 = pct->two[i].idx0;
            *index1 = pct->two[i].idx1;
            return true;
        }
    }

    // 在三张类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value == value + 1) {
            *index0 = pct->three[i].idx0;
            *index1 = pct->three[i].idx1;
            return true;
        }
    }

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value == value + 1) {
            *index0 = pct->four[i].idx0;
            *index1 = pct->four[i].idx1;
            return true;
        }
    }

    return false;
}

// 构造所有尽可能长的连对
static void generate_series_pair(POKER_CLASS_TABLE* pct)
{
    int i;
    int index0, index1;
    int value;
    int n = 0;
    int count = 0;
    bool ret;
    MyStack<int> s(32);

    if (pct->count < 6) { return; }

    for (value = 2; value <= 14; value++) {
        ret = find_series_pair_next_elem(pct, value, &index0, &index1);
        if (ret == true) {
            s.Push(index0);
            s.Push(index1);
            count += 2;
        } else if ((count > 0) && (count < 6)) {
            s.ClearStack();
            count = 0;
        } else if (count >= 6) {
            assert(count <= 20);

            for (i = count - 1; i > 0; i -= 2) {
                pct->stwo[n].idx[i] = s.Pop();
                pct->stwo[n].idx[i - 1] = s.Pop();
            }

            pct->stwo[n].value = value;
            pct->stwo[n].num = count;
            pct->num22++;
            n++;
            count = 0;
        }
    }
}

// 查找能构造成连三的下一个三张
static bool find_series_triangle_next_elem(POKER_CLASS_TABLE* pct, int value, int* index0, int* index1, int* index2)
{
    int i;

    assert((index0 != NULL) && (index1 != NULL) && (index2 != NULL));
    if (value >= 14) { return false; } // A的值为14，已经到顶

    // 在三张的类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value == value + 1) {
            *index0 = pct->three[i].idx0;
            *index1 = pct->three[i].idx1;
            *index2 = pct->three[i].idx2;
            return true;
        }
    }

    // 在四张的类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value == value + 1) {
            *index0 = pct->four[i].idx0;
            *index1 = pct->four[i].idx1;
            *index2 = pct->four[i].idx2;
            return true;
        }
    }

    return false;
}

// 构造尽可能长的连三
static void generate_series_triangle(POKER_CLASS_TABLE* pct)
{
    int i;
    int index0, index1, index2;
    int value;
    int n = 0;
    int count = 0;
    bool ret;
    MyStack<int> s(32);

    if (pct->count < 6) { return; }

    for (value = 2; value <= 14; value++) {
        ret = find_series_triangle_next_elem(pct, value, &index0, &index1, &index2);
        if (ret == true) {
            s.Push(index0);
            s.Push(index1);
            s.Push(index2);
            count += 3;
        } else if ((count > 0) && (count < 6)) {
            s.ClearStack();
            count = 0;
        } else if (count >= 6) {
            assert(count <= 18);

            for (i = count - 1; i > 1; i -= 3) {
                pct->sthree[n].idx[i] = s.Pop();
                pct->sthree[n].idx[i - 1] = s.Pop();
                pct->sthree[n].idx[i - 2] = s.Pop();
            }

            pct->sthree[n].value = value;
            pct->sthree[n].num = count;
            pct->num33++;
            n++;
            count = 0;
        }
    }
}

// 查找能构造成连四的下一个四张
static bool find_series_four_next_elem(POKER_CLASS_TABLE* pct, int value, int* index0, int* index1, int* index2, int* index3)
{
    int i;

    assert((index0 != NULL) && (index1 != NULL)
        && (index2 != NULL) && (index3 != NULL));

    if (value >= 14) { return false; } // A的值为14，已经到顶

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value == value + 1) {
            *index0 = pct->four[i].idx0;
            *index1 = pct->four[i].idx1;
            *index2 = pct->four[i].idx2;
            *index3 = pct->four[i].idx3;
            return true;
        }
    }

    return false;
}

// 构造尽可能长的连四
static void generate_series_four(POKER_CLASS_TABLE* pct)
{
    int i;
    int index0, index1, index2, index3;
    int value;
    int n = 0;
    int count = 0;
    bool ret;
    MyStack<int> s(32);

    if (pct->count < 8) { return; }

    for (value = 2; value <= 14; value++) {
        ret = find_series_four_next_elem(pct, value, &index0, &index1, &index2, &index3);
        if (ret == true) {
            s.Push(index0);
            s.Push(index1);
            s.Push(index2);
            s.Push(index3);
            count += 4;
        } else if ((count > 0) && (count < 8)) {
            s.ClearStack();
            count = 0;
        } else if (count >= 8) {
            assert(count <= 20);

            for (i = count - 1; i > 2; i -= 4) {
                pct->sfour[n].idx[i] = s.Pop();
                pct->sfour[n].idx[i - 1] = s.Pop();
                pct->sfour[n].idx[i - 2] = s.Pop();
                pct->sfour[n].idx[i - 3] = s.Pop();
            }

            pct->sfour[n].value = value;
            pct->sfour[n].num = count;
            pct->num44++;
            n++;
            count = 0;
        }
    }
}

// 复位扑克牌的分类表结构
POKER_API 
bool reset_poker_class_table(POKER_CLASS_TABLE* pct)
{
    assert(pct != NULL);

    pct->builded = 0;
    pct->count = 0;
    pct->num1 = 0;
    pct->num2 = 0;
    pct->num3 = 0;
    pct->num4 = 0;
    pct->num11 = 0;
    pct->num22 = 0;
    pct->num33 = 0;
    pct->num44 = 0;
    pct->has_king_bomb = 0;

    return true;
}

// 构造扑克牌的分类表
POKER_API 
bool build_poker_class_table(POKER_CLASS_TABLE* pct, int vec[], int num)
{
    assert(pct != NULL);

    if ((num < 0) || (num > 20)) { return false; }

    if (pct->builded == 1) {
        return true;
    }

    pct->count = num;

    classify_single(pct, vec, num);
    classify_pair(pct, vec, num);
    classify_triangle(pct, vec, num);
    classify_four(pct, vec, num);
    classify_kingbom(pct, vec, num);

    generate_series(pct);
    generate_series_pair(pct);
    generate_series_triangle(pct);
    generate_series_four(pct);

    pct->builded = 1;

    return true;
}

// 查找一个比给定值大的单张
static bool retrieve_valid_single(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int index;
    int value;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    // 在单张类别中查找
    for (i = 0; i < pct->num1; i++) {
        if (pct->one[i].value > val) {
            if (count >= times) {
                index = pct->one[i].idx;
                value = pct->one[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在对子类别中查找
    for (i = 0; i < pct->num2; i++) {
        if (pct->two[i].value > val) {
            if (count >= times) {
                index = pct->two[i].idx0;
                value = pct->two[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在三张类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value > val) {
            if (count >= times) {
                index = pct->three[i].idx0;
                value = pct->three[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                index = pct->four[i].idx0;
                value = pct->four[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 查找炸弹
    RETRIEVE_BOMB();
    return false;

found:
    out->type = SINGLE;
    out->value = value;
    out->num = 1;
    vec[0] = index;
    return true;
}

// 查找一个比给定值大的对子
static bool retrieve_valid_pair(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int value;
    int index0, index1;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    // 在对子类别中查找
    for (i = 0; i < pct->num2; i++) {
        if (pct->two[i].value > val) {
            if (count >= times) {
                index0 = pct->two[i].idx0;
                index1 = pct->two[i].idx1;
                value = pct->two[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在三张类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value > val) {
            if (count >= times) {
                index0 = pct->three[i].idx0;
                index1 = pct->three[i].idx1;
                value = pct->three[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                index0 = pct->four[i].idx0;
                index1 = pct->four[i].idx1;
                value = pct->four[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 查找炸弹
    RETRIEVE_BOMB();
    return false;

found:
    out->type = PAIR;
    out->value = value;
    out->num = 2;
    vec[0] = index0;
    vec[1] = index1;
    return true;
}

// 查找一个比给定值大的三张
static bool retrieve_valid_triangle(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int value;
    int index0, index1, index2;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    // 在三张类别中查找
    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value > val) {
            if (count >= times) {
                index0 = pct->three[i].idx0;
                index1 = pct->three[i].idx1;
                index2 = pct->three[i].idx2;
                value = pct->three[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 在四张类别中查找
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                index0 = pct->four[i].idx0;
                index1 = pct->four[i].idx1;
                index2 = pct->four[i].idx2;
                value = pct->four[i].value;
                goto found;
            } else {
                count++;
            }
        }
    }

    // 查找炸弹
    RETRIEVE_BOMB();
    return false;

found:
    out->type = TRIANGLE;
    out->value = value;
    out->num = 3;
    vec[0] = index0;
    vec[1] = index1;
    vec[2] = index2;
    return true;
}

// 查找一个更大的炸弹
static bool retrieve_valid_bomb(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int count = 0;

    assert((out != NULL) && (vec != NULL));
    
    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                out->type   = BOMB;
                out->num    = 4;
                out->value  = pct->four[i].value;
                vec[0]      = pct->four[i].idx0;
                vec[1]      = pct->four[i].idx1;
                vec[2]      = pct->four[i].idx2;
                vec[3]      = pct->four[i].idx3;

                return true;
            } else {
                count++;
            }
        }
    }

    if (pct->has_king_bomb != 0) {
        if (count >= times) {
            out->type   = BOMB;
            out->num    = 2;
            out->value  = pct->kingbomb.value;
            vec[0]      = pct->kingbomb.idx0;
            vec[1]      = pct->kingbomb.idx1;

            return true;
        } else {
            count++;
        }
    }

    return false;
}

// 查找一个比给定值更大的三带一
static bool retrieve_valid_three_plus_one(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 查找一个合适的三张，再查找一个值不等于三张的值的单张
    //

    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value > val) {
            if (count >= times) {
                out->type = THREE_PLUS_ONE;
                out->num = 4;
                out->value = pct->three[i].value;
                vec[0] = pct->three[i].idx0;
                vec[1] = pct->three[i].idx1;
                vec[2] = pct->three[i].idx2;
                goto find_three_ok;
            } else {
                count++;
            }
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                out->type = THREE_PLUS_ONE;
                out->num = 4;
                out->value = pct->four[i].value;
                vec[0] = pct->four[i].idx0;
                vec[1] = pct->four[i].idx1;
                vec[2] = pct->four[i].idx2;
                goto find_three_ok;
            } else {
                count++;
            }
        }
    }

    goto retrieve_bomb;

find_three_ok:
    for (i = 0; i < pct->num1; i++) {
        vec[3] = pct->one[i].idx;
        goto find_three_plus_one_ok;
    }

    for (i = 0; i < pct->num2; i++) {
        vec[3] = pct->two[i].idx0;
        goto find_three_plus_one_ok;
    }

    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value != out->value) {
            vec[3] = pct->three[i].idx0;
            goto find_three_plus_one_ok;
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value != out->value) {
            vec[3] = pct->four[i].idx0;
            goto find_three_plus_one_ok;
        }
    }

    if (count == 1) { count = 0; }

retrieve_bomb:
    RETRIEVE_BOMB();
    return false;

find_three_plus_one_ok:
    quick_sort(vec, 0, 3);
    return true;
}

// 查找一个比给定值更大的三带二
static bool retrieve_valid_three_plus_two(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 查找一个合适的三张，再查找一个值不等于三张的值的对子
    //

    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value > val) {
            if (count >= times) {
                out->type = THREE_PLUS_TWO;
                out->num = 5;
                out->value = pct->three[i].value;
                vec[0] = pct->three[i].idx0;
                vec[1] = pct->three[i].idx1;
                vec[2] = pct->three[i].idx2;

                goto find_pair;
            } else {
                count++;
            }
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                out->type = THREE_PLUS_TWO;
                out->num = 5;
                out->value = pct->four[i].value;
                vec[0] = pct->four[i].idx0;
                vec[1] = pct->four[i].idx1;
                vec[2] = pct->four[i].idx2;

                goto find_pair;
            } else {
                count++;
            }
        }
    }

    goto retrieve_bomb;

find_pair:
     for (i = 0; i < pct->num2; i++) {
        vec[3] = pct->two[i].idx0;
        vec[4] = pct->two[i].idx1;
        goto find_three_plus_two_ok;
    }

    for (i = 0; i < pct->num3; i++) {
        if (pct->three[i].value != out->value) {
            vec[3] = pct->three[i].idx0;
            vec[4] = pct->three[i].idx1;
            goto find_three_plus_two_ok;
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value != out->value) {
            vec[3] = pct->four[i].idx0;
            vec[4] = pct->four[i].idx1;
            goto find_three_plus_two_ok;
        }
    }

    if (count == 1) { count = 0; }

retrieve_bomb:
    RETRIEVE_BOMB();
    return false;


find_three_plus_two_ok:
    quick_sort(vec, 0, 4);
    return true;
}

// 查找一个比给定值大的四带二
static bool retrieve_valid_four_plus_two(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    // 查找一个合适的四张，再查找两个单张，即可，否则查找炸弹

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                out->type = FOUR_PLUS_TWO;
                out->num = 6;
                out->value = pct->four[i].value;
                vec[0] = pct->four[i].idx0;
                vec[1] = pct->four[i].idx1;
                vec[2] = pct->four[i].idx2;
                vec[3] = pct->four[i].idx3;

                goto find_two_singles;
            } else {
                count++;
            }
        }
    }

    goto retrieve_bomb;

find_two_singles:
    //
    // 查找两个单张，ok表示查找到的计数
    //
    int ok = 0;

    for (i = 0; i < pct->num1; i++) {
        if (ok >= 2) { goto find_four_plus_two_ok; }

        vec[4 + ok] = pct->one[i].idx;
        ok++;
    }

    for (i = 0; i < pct->num2; i++) {
        if (ok >= 2) { goto find_four_plus_two_ok; }

        if (ok == 0) {
            vec[4 + ok] = pct->two[i].idx0;
            vec[4 + ok + 1] = pct->two[i].idx1;
            ok += 2;
        } else {
            vec[4 + ok] = pct->two[i].idx0;
            ok++;
        }
    }

    for (i = 0; i < pct->num3; i++) {
        if (ok >= 2) { goto find_four_plus_two_ok; }

        if (ok == 0) {
            vec[4 + ok] = pct->three[i].idx0;
            vec[4 + ok + 1] = pct->three[i].idx1;
            ok += 2;
        } else {
            vec[4 + ok] = pct->three[i].idx0;
            ok++;
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (ok >= 2) { goto find_four_plus_two_ok; }

        if (pct->four[i].value != out->value) {
            if (ok == 0) {
                vec[4 + ok] = pct->four[i].idx0;
                vec[4 + ok + 1] = pct->four[i].idx1;
                ok += 2;
            } else {
                vec[4 + ok] = pct->four[i].idx0;
                ok++;
            }
        }
    }

    if (ok >= 2) { goto find_four_plus_two_ok; }

    if (count == 1) { count = 0; }

retrieve_bomb:
    RETRIEVE_BOMB();
    return false;

find_four_plus_two_ok:
    quick_sort(vec, 0, 5);
    return true;
}

// 查找一个比给定值大的四带四
static bool retrieve_valid_four_plus_four(int times, int val, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i;
    int count = 0;

    assert((out != NULL) && (vec != NULL));

    // 查找一个合适的四张，再查找两个对子，即可，否则查找炸弹

    for (i = 0; i < pct->num4; i++) {
        if (pct->four[i].value > val) {
            if (count >= times) {
                out->type = FOUR_PLUS_FOUR;
                out->num = 8;
                out->value = pct->four[i].value;
                vec[0] = pct->four[i].idx0;
                vec[1] = pct->four[i].idx1;
                vec[2] = pct->four[i].idx2;
                vec[3] = pct->four[i].idx3;

                goto find_two_pairs;
            } else {
                count++;
            }
        }
    }

    goto retrieve_bomb;

find_two_pairs:
    //
    // 寻找两个对子。变量ok记录找到的牌数量，如果找到4张则表示查找成功
    //
    int ok = 0;

    for (i = 0; i < pct->num2; i++) {
        if (ok >= 4) { goto find_four_plus_four_ok; }

        vec[4 + ok] = pct->two[i].idx0;
        vec[4 + ok + 1] = pct->two[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num3; i++) {
        if (ok >= 4) { goto find_four_plus_four_ok; }

        vec[4 + ok] = pct->three[i].idx0;
        vec[4 + ok + 1] = pct->three[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num4; i++) {
        if (ok >= 4) { goto find_four_plus_four_ok; }

        if (pct->four[i].value != out->value) {
            vec[4 + ok] = pct->four[i].idx0;
            vec[4 + ok + 1] = pct->four[i].idx1;
            ok += 2;
        }
    }

    if (ok >= 4) { goto find_four_plus_four_ok; }

    if (count == 1) {
        //
        // 程序如果运行到此处，说明其构造四带四失败，否则它将进入
        // “find_four_plus_four_ok”分支而不是当前的“retrieve_bomb”分支。
        // 但出牌的提示可能会有多个的，变量 count 与传入参数 times 就是为
        // 控制返回第几个提示而引入的，而在试图构造四带四时可能会修改 count 的值。
        //
        // 实际上，count 值反映的是查找提示成功的计数，但若程序运行到此处，
        // 表示查找四张成功但查找附属的两个对子失败，为使用 count 值正确反映
        // 查找提示成功的计数，在进入查找炸弹之前，应将其复位为 0
        //
        // 逻辑上应该是这样的，否则可能会引起重复提示。其它查找函数也可能存在
        // 类似情况。
        //
        // 示例：上家出牌为四带四，手上的牌为“8888AAAA”，则会重复提示。
        //
        count = 0;
    }

retrieve_bomb:
    RETRIEVE_BOMB();
    return false;

find_four_plus_four_ok:
    quick_sort(vec, 0, 7);
    return true;
}

// 查找一个比给定值大且牌数量相同的顺子
static bool retrieve_valid_series(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 牌的分类表中已经保存分析好的尽可能长的顺子，遍历这些顺子，如果找到一个，
    // 其值比请求的值大的，若其长度比请求的顺子还长，则可再构造出合适的顺子。
    //
    for (i = 0; i < pct->num11; i++) {
        value = pct->sone[i].value;
        len = pct->sone[i].num;

        if (value > val) {
            if (len == num) {
                if (count >= times) {
                    out->type = SERIES;
                    out->num = num;
                    out->value = value;
                    for (j = 0; j < num; j++) {
                        vec[j] = pct->sone[i].idx[j];
                    }
                    return true;
                } else {
                    count++;
                }
            } else if (len > num) {
                //
                // 如果现有的顺子比请求的顺子还长，根据其值差和长度差，可确定
                // 匹配的顺子的数量，其关系如下代码段所示。
                //
                if (value - val <= len - num) {
                    series_count = value - val;
                } else {
                    series_count = len - num + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        out->type = SERIES;
                        out->num = num;
                        out->value = value - k;
                        for (j = 0; j < num; j++) {
                            vec[j] = pct->sone[i].idx[len - num - k + j];
                        }
                        return true;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    RETRIEVE_BOMB();
    return false;
}


// 查找合适的连对
static bool retrieve_valid_series_pair(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 操作与查找顺子相似
    //

    for (i = 0; i < pct->num22; i++) {
        value = pct->stwo[i].value;
        len = pct->stwo[i].num;

        if (value > val) {
            if (len == num) {
                if (count >= times) {
                    out->type = SERIES_PAIR;
                    out->num = num;
                    out->value = value;
                    for (j = 0; j < num; j++) {
                        vec[j] = pct->stwo[i].idx[j];
                    }
                    return true;
                } else {
                    count++;
                }
            } else if (len > num) {
                if (value - val <= (len - num) / 2) {
                    series_count = value - val;
                } else {
                    series_count = (len - num) / 2 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        out->type = SERIES_PAIR;
                        out->num = num;
                        out->value = value - k;
                        for (j = 0; j < num; j++) {
                            vec[j] = pct->stwo[i].idx[len - num - k * 2 + j];
                        }
                        return true;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    RETRIEVE_BOMB();
    return false;
}

// 查找合适的连三
static bool retrieve_valid_series_triangle(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 操作与查找顺子相似
    //

    for (i = 0; i < pct->num33; i++) {
        value = pct->sthree[i].value;
        len = pct->sthree[i].num;

        if (value > val) {
            if (len == num) {
                if (count >= times) {
                    out->type = SERIES_TRIANGLE;
                    out->num = num;
                    out->value = value;
                    for (j = 0; j < num; j++) {
                        vec[j] = pct->sthree[i].idx[j];
                    }
                    return true;
                } else {
                    count++;
                }
            } else if (len > num) {
                if (value - val <= (len - num) / 3) {
                    series_count = value - val;
                } else {
                    series_count = (len - num) / 3 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        out->type = SERIES_TRIANGLE;
                        out->num = num;
                        out->value = value - k;
                        for (j = 0; j < num; j++) {
                            vec[j] = pct->sthree[i].idx[len - num - k * 3 + j];
                        }
                        return true;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    RETRIEVE_BOMB();
    return false;
}

// 查找合适的连三带一
static bool retrieve_valid_series_three_plus_one(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int m, n;
    int len;
    int value;
    int count = 0;
    int series_count = 0;
    int ok = 0; // 查找单张成功的计数

    assert(num % 4 == 0);
    assert((out != NULL) && (vec != NULL));

    m = num / 4; // m 个连三，即存在 m 个单张
    n = m * 3; // 连三的牌数量，3 的整数倍。（n = num - m）

    //
    // 查找一个合适的连三，再查找单张，最后选择炸弹
    //

    for (i = 0; i < pct->num33; i++) {
        value = pct->sthree[i].value;
        len = pct->sthree[i].num;

        if (value > val) {
            if (len == n) {
                if (count >= times) {
                    out->type = SERIES_THREE_PLUS_ONE;
                    out->num = num;
                    out->value = value;
                    for (j = 0; j < n; j++) {
                        vec[j] = pct->sthree[i].idx[j];
                    }
                    goto find_single;
                } else {
                    count++;
                }
            } else if (len > n) {
                if (value - val <= (len - n) / 3) {
                    series_count = value - val;
                } else {
                    series_count = (len - n) / 3 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        out->type = SERIES_THREE_PLUS_ONE;
                        out->num = num;
                        out->value = value - k;
                        for (j = 0; j < n; j++) {
                            vec[j] = pct->sthree[i].idx[len - n - k * 3 + j];
                        }
                        goto find_single;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    goto find_bomb; // 查找连三失败，寻求炸弹


find_single:
    // 查找连三完成，再查找出 m 个单张，即可完成构造
    for (i = 0; i < pct->num1; i++) { // 单张类别
        if (ok >= m) { goto find_single_finished; }

        vec[n + ok] = pct->one[i].idx;
        ok++;
    }

    for (i = 0; i < pct->num2; i++) { // 对子类别
        if (ok >= m) { goto find_single_finished; }

        vec[n + ok] = pct->two[i].idx0;
        ok++;

        if (ok < m) {
            vec[n + ok] = pct->two[i].idx1;
            ok++;
        }
    }

    for (i = 0; i < pct->num3; i++) { // 三张类别
        if (ok >= m) { goto find_single_finished; }

        if ((pct->three[i].value > out->value - n / 3)
            && (pct->three[i].value <= out->value)) {
                //
                // 该三张已经入选为连三，不能将其拆分为单张
                //
                continue;
        }

        vec[n + ok] = pct->three[i].idx0;
        ok++;

        if (ok < m - 1) {
            vec[n + ok] = pct->three[i].idx1;
            vec[n + ok + 1] = pct->three[i].idx2;
            ok += 2;
        } else if (ok < m) {
            vec[n + ok] = pct->three[i].idx1;
            ok++;
        }
    }

    for (i = 0; i < pct->num4; i++) {
        if (ok >= m) { goto find_single_finished; }

        if ((pct->four[i].value > out->value - n / 3)
            && (pct->four[i].value <= out->value)) {
                //
                // 该四张已经入选为连三，不能将其拆分为单张
                //
                continue;
        }

        vec[n + ok] = pct->four[i].idx0;
        ok++;

        // 为遵循炸弹不拆的原则，最多取其中3个牌作为单张
        if (ok < m - 1) {
            vec[n + ok] = pct->four[i].idx1;
            vec[n + ok + 1] = pct->four[i].idx2;
            ok += 2;
        } else if (ok < m) {
            vec[n + ok] = pct->four[i].idx1;
            ok++;
        }
    }

    if (ok >= m) { goto find_single_finished; }

    if (count == 1) { count = 0; }

find_bomb:
    RETRIEVE_BOMB();
    return false;

find_single_finished:
    quick_sort(vec, 0, num - 1);
    return true;
}

// 查找合适的连三带二
static bool retrieve_valid_series_three_plus_two(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int m, n;
    int len;
    int value;
    int count = 0;
    int series_count = 0;
    int ok = 0; // 查找单张成功的计数

    assert(num % 5 == 0);
    assert((out != NULL) && (vec != NULL));

    m = num / 5; // m 个连三，即存在 m 个对子
    n = m * 3; // 连三的牌数量，3 的整数倍。（n = num - m * 2）

    //
    // 查找一个合适的连三，再查找对子，最后选择炸弹
    //

    for (i = 0; i < pct->num33; i++) {
        value = pct->sthree[i].value;
        len = pct->sthree[i].num;

        if (value > val) {
            if (len == n) {
                if (count >= times) {
                    out->type = SERIES_THREE_PLUS_TWO;
                    out->num = num;
                    out->value = value;
                    for (j = 0; j < n; j++) {
                        vec[j] = pct->sthree[i].idx[j];
                    }
                    goto find_pair;
                } else {
                    count++;
                }
            } else if (len > n) {
                if (value - val <= (len - n) / 3) {
                    series_count = value - val;
                } else {
                    series_count = (len - n) / 3 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        out->type = SERIES_THREE_PLUS_TWO;
                        out->num = num;
                        out->value = value - k;
                        for (j = 0; j < n; j++) {
                            vec[j] = pct->sthree[i].idx[len - n - k * 3 + j];
                        }
                        goto find_pair;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    goto find_bomb; // 查找连三失败，寻求炸弹

find_pair:
    // 查找连三完成，再查找出 m 个对子，即可完成构造

    for (i = 0; i < pct->num2; i++) { // 对子类别
        if (ok >= m * 2) { goto find_pair_finished; }

        vec[n + ok] = pct->two[i].idx0;
        vec[n + ok + 1] = pct->two[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num3; i++) { // 三张类别
        if (ok >= m * 2) { goto find_pair_finished; }

        if ((pct->three[i].value > out->value - n / 3)
            && (pct->three[i].value <= out->value)) {
                //
                // 该三张已经入选为连三，不能将其拆分为对子
                //
                continue;
        }

        vec[n + ok] = pct->three[i].idx0;
        vec[n + ok + 1] = pct->three[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num4; i++) { // 四张类别
        if (ok >= m * 2) { goto find_pair_finished; }

        if ((pct->four[i].value > out->value - n / 3)
            && (pct->four[i].value <= out->value)) {
                //
                // 该四张已经入选为连三，不能将其拆分为对子
                //
                continue;
        }

        vec[n + ok] = pct->four[i].idx0;
        vec[n + ok + 1] = pct->four[i].idx1;
        ok += 2;
    }

    if (ok >= m * 2) { goto find_pair_finished; }

    if (count == 1) { count = 0; }

find_bomb:
    RETRIEVE_BOMB();
    return false;

find_pair_finished:
    quick_sort(vec, 0, num - 1);
    return true;
}

// 查找合适的连四
static bool retrieve_valid_series_four(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;

    assert((out != NULL) && (vec != NULL));

    //
    // 操作与查找顺子相似
    //

    for (i = 0; i < pct->num44; i++) {
        value = pct->sfour[i].value;
        len = pct->sfour[i].num;

        if (value > val) {
            if (len == num) {
                if (count >= times) {
                    for (j = 0; j < num; j++) {
                        vec[j] = pct->sfour[i].idx[j];
                    }
                    out->type = SERIES_FOUR;
                    out->num = num;
                    out->value = value;
                    return true;
                } else {
                    count++;
                }
            } else if (len > num) {
                if (value - val <= (len - num) / 4) {
                    series_count = value - val;
                } else {
                    series_count = (len - num) / 4 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        for (j = 0; j < num; j++) {
                            vec[j] = pct->sfour[i].idx[len - num - k * 4 + j];
                        }
                        out->type = SERIES_FOUR;
                        out->num = num;
                        out->value = value - k;
                        return true;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    RETRIEVE_BOMB();
    return false;
}

// 查找合适的连四带二
static bool retrieve_valid_series_four_plus_two(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{
    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;
    int m, n;
    int ok = 0; // 用于计数查找到附属的单张个数

    assert(num % 6 == 0);
    assert((out != NULL) && (vec != NULL));

    m = num / 6 * 2; // 单张的数量为 m
    n = num / 6 * 4; // 连四的牌数量为 n

    //
    // 先查找到合适的连四，再查找附属的单张，最后寻求炸弹
    //

    for (i = 0; i < pct->num44; i++) {
        value = pct->sfour[i].value;
        len = pct->sfour[i].num;

        if (value > val) {
            if (len == n) {
                if (count >= times) {
                    for (j = 0; j < n; j++) {
                        vec[j] = pct->sfour[i].idx[j];
                    }
                    out->type = SERIES_FOUR_PLUS_TWO;
                    out->num = num;
                    out->value = value;
                    goto find_two;
                } else {
                    count++;
                }
            } else if (len > n) {
                if (value - val <= (len - n) / 4) {
                    series_count = value - val;
                } else {
                    series_count = (len - n) / 4 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        for (j = 0; j < n; j++) {
                            vec[j] = pct->sfour[i].idx[len - n - k * 4 + j];
                        }
                        out->type = SERIES_FOUR_PLUS_TWO;
                        out->num = num;
                        out->value = value - k;
                        goto find_two;
                    } else {
                        count++;
                    }
                }
            }
        }
    }
    
    goto find_bomb;

find_two:
    for (i = 0; i < pct->num1; i++) { // 从单张类别中查找
        if (ok >= m) { goto find_single_finished; }

        vec[n + ok] = pct->one[i].idx;
        ok++;
    }

    for (i = 0; i < pct->num2; i++) { // 从对子类别中查找
        if (ok >= m) { goto find_single_finished; }

        if (ok < m - 1) {
            vec[n + ok] = pct->two[i].idx0;
            vec[n + ok + 1] = pct->two[i].idx1;
            ok += 2;
        } else {
            vec[n + ok] = pct->two[i].idx0;
            ok++;
        }
    }

    for (i = 0; i < pct->num3; i++) { // 从三张类别中查找
        if (ok >= m) { goto find_single_finished; }

        if (ok < m - 2) {
            vec[n + ok] = pct->three[i].idx0;
            vec[n + ok + 1] = pct->three[i].idx1;
            vec[n + ok + 2] = pct->three[i].idx2;
            ok += 3;
        } else if (ok < m - 1) {
            vec[n + ok] = pct->three[i].idx0;
            vec[n + ok + 1] = pct->three[i].idx1;
            ok += 2;
        } else {
            vec[n + ok] = pct->three[i].idx0;
            ok++;
        }
    }

    for (i = 0; i < pct->num4; i++) { // 从四张类别中查找，最多取三张相同
        if (ok >= m) { goto find_single_finished; }

        if ((pct->four[i].value > out->value - n / 4)
            && (pct->four[i].value <= out->value)) {
                continue;
        }

        if (ok < m - 2) {
            vec[n + ok] = pct->four[i].idx0;
            vec[n + ok + 1] = pct->four[i].idx1;
            vec[n + ok + 2] = pct->four[i].idx2;
            ok += 3;
        } else if (ok < m - 1) {
            vec[n + ok] = pct->four[i].idx0;
            vec[n + ok + 1] = pct->four[i].idx1;
            ok += 2;
        } else {
            vec[n + ok] = pct->four[i].idx0;
            ok++;
        }
    }

    if (ok >= m) { goto find_single_finished; }

    if (count == 1) { count = 0; }

find_bomb:
    RETRIEVE_BOMB();
    return false;

find_single_finished:
    quick_sort(vec, 0, num - 1);
    return true;
}

// 查找合适的连四带四
static bool retrieve_valid_series_four_plus_four(int times, int val, int num, POKER_CLASS_TABLE* pct, POKER_PROPERTY* out, int vec[])
{

    int i, j, k;
    int len;
    int value;
    int count = 0;
    int series_count = 0;
    int m, n;
    int ok = 0; // 用于计数查找到附属的单张个数

    assert(num % 8 == 0);
    assert((out != NULL) && (vec != NULL));

    m = num / 8 * 2; // 附带的对子数量为 m，即附属的牌数量为 (2 * m)
    n = num / 8 * 4; // 连四的牌数量为 n

    //
    // 先查找到合适的连四，再查找附属的单张，最后寻求炸弹
    //

    for (i = 0; i < pct->num44; i++) {
        value = pct->sfour[i].value;
        len = pct->sfour[i].num;

        if (value > val) {
            if (len == n) {
                if (count >= times) {
                    for (j = 0; j < n; j++) {
                        vec[j] = pct->sfour[i].idx[j];
                    }
                    out->type = SERIES_FOUR_PLUS_FOUR;
                    out->num = num;
                    out->value = value;
                    goto find_pair;
                } else {
                    count++;
                }
            } else if (len > n) {
                if (value - val <= (len - n) / 4) {
                    series_count = value - val;
                } else {
                    series_count = (len - n) / 4 + 1;
                }

                for (k = series_count - 1; k >= 0; k--) {
                    if (count >= times) {
                        for (j = 0; j < n; j++) {
                            vec[j] = pct->sfour[i].idx[len - n - k * 4 + j];
                        }
                        out->type = SERIES_FOUR_PLUS_FOUR;
                        out->num = num;
                        out->value = value - k;
                        goto find_pair;
                    } else {
                        count++;
                    }
                }
            }
        }
    }

    goto find_bomb;

find_pair:
    for (i = 0; i < pct->num2; i++) { // 从对子类别中查找
        if (ok >= m * 2) { goto find_pair_finished; }

        vec[n + ok] = pct->two[i].idx0;
        vec[n + ok + 1] = pct->two[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num3; i++) { // 从三张类别中查找
        if (ok >= m * 2) { goto find_pair_finished; }

        vec[n + ok] = pct->three[i].idx0;
        vec[n + ok + 1] = pct->three[i].idx1;
        ok += 2;
    }

    for (i = 0; i < pct->num4; i++) { // 从四张类别中查找，最多取两张相同
        if (ok >= m * 2) { goto find_pair_finished; }

        if ((pct->four[i].value > out->value - n / 4)
            && (pct->four[i].value <= out->value)) {
                continue;
        }

        vec[n + ok] = pct->four[i].idx0;
        vec[n + ok + 1] = pct->four[i].idx1;
        ok += 2;
    }

    if (ok >= m * 2) { goto find_pair_finished; }

    if (count == 1) { count = 0; }

find_bomb:
    RETRIEVE_BOMB();
    return false;

find_pair_finished:
    quick_sort(vec, 0, num - 1);
    return true;
}

//
// Function:
//      get_poker_hint
//
// Description:
//      根据上家所出的牌，从玩家手中当前牌中，提示可以出的牌。
//
// Parameter:
//      times   - 表示第几次请求提示
//      pct     - 已经构建好的扑克牌分类表
//      req     - 请求出牌的属性，即上家所打出的牌的属性
//      out     - 用于返回查找到的提示牌的属性
//      out_vec - 用于返回查找到的提示牌的索引数组，其长度可由返回的属性获得。
//
// Return:
//      TRUE if get a proper hint, else FALSE.
//
// Remark:
//      Finding PROPER pokers from [pct] according to the [req] poker property,
//      if operation succeeds, [out] poker property will contain info about the
//      pokers which were found, and the out_vec[] for corresponding indexes.
//      The [times] parameter indicates how many times the caller queried,
//      0 means first run.
// 
// ATTENTION:
//      传入的参数vec是扑克牌的全局索引数组；
//      返回的索引数组out_vec，其元素为玩家当前牌的索引。
//
POKER_API 
bool get_poker_hint(int times, POKER_CLASS_TABLE* pct, POKER_PROPERTY* req, POKER_PROPERTY* out, int out_vec[])
{
    int req_num;
    int req_type;
    int req_value;
    bool ret = false;

    assert((pct != NULL) && (req != NULL) && (out != NULL) && (out_vec != NULL));

    req_num = req->num;
    req_type = req->type;
    req_value = req->value;

    switch (req_type) {
        case SINGLE:
            ret = retrieve_valid_single(times, req_value, pct, out, out_vec);
            break;

        case PAIR:
            ret = retrieve_valid_pair(times, req_value, pct, out, out_vec);
            break;

        case TRIANGLE:
            ret = retrieve_valid_triangle(times, req_value, pct, out, out_vec);
            break;

        case THREE_PLUS_ONE:
            ret = retrieve_valid_three_plus_one(times, req_value, pct, out, out_vec);
            break;

        case THREE_PLUS_TWO:
            ret = retrieve_valid_three_plus_two(times, req_value, pct, out, out_vec);
            break;

        case BOMB:
            ret = retrieve_valid_bomb(times, req_value, pct, out, out_vec);
            break;

        case FOUR_PLUS_TWO:
            ret = retrieve_valid_four_plus_two(times, req_value, pct, out, out_vec);
            break;

        case FOUR_PLUS_FOUR:
            ret = retrieve_valid_four_plus_four(times, req_value, pct, out, out_vec);
            break;

        case SERIES:
            ret = retrieve_valid_series(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_PAIR:
            ret = retrieve_valid_series_pair(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_TRIANGLE:
            ret = retrieve_valid_series_triangle(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_THREE_PLUS_ONE:
            ret = retrieve_valid_series_three_plus_one(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_THREE_PLUS_TWO:
            ret = retrieve_valid_series_three_plus_two(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_FOUR:
            ret = retrieve_valid_series_four(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_FOUR_PLUS_TWO:
            ret = retrieve_valid_series_four_plus_two(times, req_value, req_num, pct, out, out_vec);
            break;

        case SERIES_FOUR_PLUS_FOUR:
            ret = retrieve_valid_series_four_plus_four(times, req_value, req_num, pct, out, out_vec);
            break;
    }

    return ret;
}
