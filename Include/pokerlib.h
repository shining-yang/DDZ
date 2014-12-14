//
// PokerLib.h
//      POKER DLL 导出的类型及函数声明
//

#pragma once

// 下列 ifdef 块是创建使从 DLL 导出更简单的宏的标准方法。
// 此 DLL 中的所有文件都是用命令行上定义的 POKERLIB_EXPORTS 符号编译的。
// 在使用此 DLL 的任何其他项目上不应定义此符号。这样，源文件中包含此文件的
// 任何其他项目都会将 POKER_API 函数视为是从 DLL 导入的，而此 DLL 则将用此
// 宏定义的符号视为是被导出的。
#ifdef POKERLIB_EXPORTS
#define POKER_API __declspec(dllexport)
#else
#define POKER_API __declspec(dllimport)
#endif

// 取消息ASSERT的使用
#ifndef _DEBUG
#ifdef assert
#undef assert
#endif
#define assert(arg)     void(0)
#endif

//
// 提供给其它模块的常量或类型定义，以及 PokerLib.DLL 导出的符号
//

// 一副扑克牌
#define  POKER_ITEM_COUNT                   54


// poker unit
typedef enum POKER_UNIT_t {
    P3,
    P4,
    P5,
    P6,
    P7,
    P8,
    P9,
    P10,
    PJ,
    PQ,
    PK,
    PA,
    P2,
    PJOKER0,
    PJOKER1
} POKER_UNIT;

// the color of poker
typedef enum POKER_CLR_t {
    SPADE,
    HEART,
    CLUB,
    DIAMOND,
    JOKER0,
    JOKER1
} POKER_CLR;

// the poker card element
typedef struct POKER_t {
    POKER_UNIT      unit;
    POKER_CLR       color;
    int             value;
} POKER;


// 出牌的类型
typedef enum {
    INVALID_TYPE = 0,
    SINGLE,                 // 单张
    SERIES,                 // 顺子
    PAIR,                   // 对子
    SERIES_PAIR,            // 连对
    TRIANGLE,               // 三张
    SERIES_TRIANGLE,        // 连三张
    THREE_PLUS_ONE,         // 三带一
    SERIES_THREE_PLUS_ONE,  // 连三带一
    THREE_PLUS_TWO,         // 三带二
    SERIES_THREE_PLUS_TWO,  // 连三带二
    SERIES_FOUR,            // 连四（两个炸弹废了）
    FOUR_PLUS_TWO,          // 四带二
    SERIES_FOUR_PLUS_TWO,   // 连四带二
    FOUR_PLUS_FOUR,         // 四带四
    SERIES_FOUR_PLUS_FOUR,  // 连四带四
    BOMB,                   // 炸弹
} POKER_TYPE;

// 出牌的详细情况
typedef struct POKER_PROPERTY_s {
    POKER_TYPE      type;       // 出牌的类型
    int             value;      // 出牌的值
    int             num;        // 出牌个数
} POKER_PROPERTY;

// 单张
typedef struct HT_SINGLE_t {
    int     idx;
    int     value;
} HT_SINGLE;

// 对子
typedef struct HT_PAIR_t {
    int     idx0;
    int     idx1;
    int     value;
} HT_PAIR;

// 三张
typedef struct HT_TRIANGLE_t {
    int     idx0;
    int     idx1;
    int     idx2;
    int     value;
} HT_TRIANGLE;

// 四张
typedef struct HT_FOUR_t {
    int     idx0;
    int     idx1;
    int     idx2;
    int     idx3;
    int     value;
} HT_FOUR;

// 顺子
typedef struct HT_SERIES_t {
    int     idx[12];
    int     num;
    int     value;
} HT_SERIES;

// 连对
typedef struct HT_SERIES_PAIR_t {
    int     idx[20];
    int     num;
    int     value;
} HT_SERIES_PAIR;

// 三张
typedef struct HT_SERIES_TRIANGLE_t {
    int     idx[18];
    int     num;
    int     value;
} HT_SERIES_TRIANGLE;

// 连四
typedef struct HT_SERIES_FOUR_t {
    int     idx[20];
    int     num;
    int     value;
} HT_SERIES_FOUR;

// 双王炸弹
typedef struct HT_KINGBOMB_t {
    int     idx0;
    int     idx1;
    int     value;
} HT_KINGBOMB;

// 扑克牌分类表
typedef struct POKER_CLASS_TABLE_t {
    int             builded;        // whether the PCT has been builded
    int             count;          // current poker cards num

    HT_SINGLE       one[14];        // single (13 singles at most)
    int             num1;           // [single] COUNT

    HT_PAIR         two[10];        // pair (10 pairs at most)
    int             num2;           // [pair] COUNT

    HT_TRIANGLE     three[6];       // triangle (6 triangles at most)
    int             num3;           // [triangle] COUNT

    HT_FOUR         four[5];        // four (5 fours at most)
    int             num4;           // [four] COUNT

    HT_KINGBOMB     kingbomb;       // king bomb
    int             has_king_bomb;  // indicate whether exists king bomb

    HT_SERIES       sone[2];        // series one (2 separate series at most)
    int             num11;          // [series one] COUNT

    HT_SERIES_PAIR      stwo[3];    // series two ( 3 separate series two at most)
    int                 num22;      // [series two] COUNT

    HT_SERIES_TRIANGLE  sthree[3];  // series three ( 3 separate series three at most)
    int                 num33;      // [series three] COUNT

    HT_SERIES_FOUR      sfour[2];   // series four ( 2 separate series four at most)
    int                 num44;      // [series four] COUNT
} POKER_CLASS_TABLE;

// 获取表示扑克牌的字符（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
TCHAR poker_unit_to_char(POKER_UNIT unit);

// 获取表示扑克牌花色的字符（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
TCHAR poker_clr_to_char(POKER_CLR clr);

// 由扑克牌索引获取表示扑克牌的字符（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
TCHAR poker_index_to_char(int index);

// 由扑克牌索引获取其花色的字符（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
TCHAR poker_index_to_clr_char(int index);

// 由扑克牌索引获取扑克牌的值（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
int poker_index_to_value(int index);

// 获取扑克牌类型对应的字符串（用于DEBUG输出，或CONSOLE界面输出）
POKER_API 
TCHAR* poker_type_to_string(POKER_TYPE pt);

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
//      调用者需判断该函数的返回值，若返回false，表示无法识别牌的属性，即该组牌
//      为不合法序列。
//      传入的参数 vec 为全局扑克牌的索引（0～53），而不是玩家当前牌的索引。
//
POKER_API 
bool can_play_poker(POKER_PROPERTY* pp, int vec[], int num);

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
//      调用者需判断该函数的返回值，若返回false，表示无法识别牌的属性，即该组牌
//      为不合法序列。
//      传入的参数 vec 为全局扑克牌的索引（0～53），而不是玩家当前牌的索引。
//
POKER_API 
bool can_follow_poker(POKER_PROPERTY* pp, int vec[], int num, POKER_PROPERTY* req);

//
// Function:
//      build_poker_class_table
// Description:
//      构建扑克牌分类表
// Parameter:
//      pct     - [out] 保存扑克牌分类结果信息
//      vec     - 玩家当前手中的扑克牌序列，其元素为全局扑克牌索引（0～53）。
//      num     - 玩家当前手中的扑克牌数量
// Return:
//      Always be TRUE
//
POKER_API 
bool build_poker_class_table(POKER_CLASS_TABLE* pct, int vec[], int num);

//
// Function:
//      reset_poker_class_table
// Description:
//      复位扑克牌分类表
// Parameter:
//      pct - 扑克牌分类表
// Return:
//      Always be TRUE
//
POKER_API 
bool reset_poker_class_table(POKER_CLASS_TABLE* pct);

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
//      out     - [out]用于返回查找到的提示牌的属性
//      out_vec - [out]用于返回查找到的提示牌的索引数组，其长度可由返回的属性获得。
//
// Return:
//      TRUE if get a proper hint, else FALSE.
//
// Remark:
//      Finding PROPER pokers from [pct] according to the [req] poker property,
//      if operation succeeds, [out] poker property will contain info about the
//      pokers which were found, and the out_vec[] for corresponding indexes.
//      The [times] parameter indicates how many times the caller had queried,
//      0 means first call.
// 
// ATTENTION:
//      传入的参数vec是扑克牌的全局索引数组，即其取值范围为［0～53］；
//      返回的索引数组out_vec，其元素为玩家当前牌的索引，即其取值范围为［0～19］；
//      查找提示时，需先构造好扑克牌分类表。若扑克牌数量发生过变化，则需要重新构造扑克分类表
//
POKER_API 
bool get_poker_hint(
    int times,
    POKER_CLASS_TABLE* pct,
    POKER_PROPERTY* req,
    POKER_PROPERTY* out,
    int out_vec[]
);

