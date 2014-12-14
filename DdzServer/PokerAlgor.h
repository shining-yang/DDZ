//
// PokerAlgor.h
//
// 斗地主洗牌算法
//
#pragma once

//
// 洗牌函数类型
//
// 参数：
//      poker   - [in, out] 扑克牌索引 0～53
//      num     - [in] 数组poker的长度，必须为54
//  
typedef void (*LPPOKERALGORITHM)(int poker[], int num);

typedef struct tagPokerAlogrithm {
    LPPOKERALGORITHM    algorithm;
    LPTSTR              name;
} POKER_ALOGRITHM, *LPPOKER_ALGORITHM;


void PA_Randomize1(int poker[], int num);
void PA_Randomize10(int poker[], int num);
void PA_RandRemainder(int poker[], int num);
void PA_Test1(int poker[], int num);
void PA_Test2(int poker[], int num);
void PA_Test3(int poker[], int num);


static void get_remainder(int poker[], int num, int times);
static void get_random(int poker[], int num, int times);

