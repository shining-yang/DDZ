//
// PokerAlgor.cpp
//
//  斗地主游戏的几种洗牌算法
//

#include "stdafx.h"
#include "PokerAlgor.h"

#define POKER_ITEM_COUNT            54

// 错位的方法洗牌
static void get_remainder(int poker[], int num, int times)
{
    int index;
    int remainder;
    int old_poker[POKER_ITEM_COUNT];

    if (num != POKER_ITEM_COUNT) {
        return;
    }

#if 0
    // 设置初始扑克牌序列为 0～53
    for (int i = 0; i < num; i++) {
        poker[i] = i;
    }
#else
    get_random(poker, num, 1);
#endif

    srand((unsigned)time(NULL));

    while (times--) { // 重复 times 次错位洗牌
        
        if (rand() % 2 == 0) {
            for (int i = 0; i < num; i++) { // 从头到尾保存保存刚洗好的牌
                old_poker[i] = poker[i];
            }
        } else {
            for (int i = 0; i < num; i++) { // 从尾到头保存保存刚洗好的牌
                old_poker[i] = poker[num - i - 1];
            }
        }

        remainder = 3 + rand() % 6; // 取一个 3～8 之间的数作为错位值

        index = 0;

        for (int i = 0; i < remainder; i++) { // 错位洗牌
            for (int j = 0; j < num; j++) {
                if (j % remainder == i) {
                    poker[index++] = old_poker[j];
                }
            }
        }
    }
}

// 随机的方法洗牌
static void get_random(int poker[], int num, int times)
{
    int random;
    int temp;

    // 设置初始扑克牌序列为 0～53
    for (int i = 0; i < num; i++) {
        poker[i] = i;
    }

    srand((unsigned)time(NULL));

    while (times--) { // 随机打乱顺序的次数
        for (int i = 0; i < num; i++) { // 随机打乱顺序
            if (i == num - 1) {
                random = 0;
            } else {
                random = rand() % (num - i);
            }

            // 将每次随机得到的牌替换到末尾
            temp = poker[num - i - 1];
            poker[num - i - 1] = poker[random];
            poker[random] = temp;
        }
    }
}

// 随机洗牌1次
void PA_Randomize1(int poker[], int num)
{
    int rd;
    int unused_cnt;
    int used[POKER_ITEM_COUNT] = { 0 };

    if (num != POKER_ITEM_COUNT) {
        return;
    }

    srand((unsigned)time(NULL));

    for (int i = 0; i < POKER_ITEM_COUNT; i++) {
        if (i == POKER_ITEM_COUNT - 1) {
            rd = 0;
        } else {
            rd = rand() % (POKER_ITEM_COUNT - i);
        }

        unused_cnt = 0;
        for (int index = 0; index < POKER_ITEM_COUNT; index++) {
            if (used[index] != 0) {
                continue;
            } else {
                if (unused_cnt == rd) {
                    used[index] = 1;
                    poker[i] = index;
                    break;
                }

                unused_cnt++;
            }
        }
    }
}

// 随机的方法洗牌
void PA_Randomize10(int poker[], int num)
{
    srand((unsigned)time(NULL));
    int times = 6 + rand() % 5;

    // 随机洗牌6～10次
    get_random(poker, num, times);
}

void PA_RandRemainder(int poker[], int num)
{
    srand((unsigned)time(NULL));
    int times = 6 + rand() % 5;

    // 随机错位洗牌，洗牌6～10次
    get_remainder(poker, num, times);
}

// 
void PA_Test1(int poker[], int num)
{
    // 初始化扑克牌序列为 0～53
    for (int i = 0; i < num; i++) {
        poker[i] = i;
    }
}

void PA_Test2(int poker[], int num)
{
    // 初始化扑克牌序列为 0～53
    for (int i = 0; i < num; i++) {
        poker[i] = i;
    }
}

void PA_Test3(int poker[], int num)
{
    // 初始化扑克牌序列为 0～53
    for (int i = 0; i < num; i++) {
        poker[i] = i;
    }
}
