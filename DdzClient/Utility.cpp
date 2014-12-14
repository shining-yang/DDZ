//
//
// Utility.cpp
//
//
#include "stdafx.h"
#include "Utility.h"

//
// Used for saving debug info to print out.
//
#ifdef _DEBUG
TCHAR dbg_TraceTextBuf [DEBUG_TRACE_LEN];
#endif


//
// 底层扑克牌的索引是这样设计的：
//
// 值相同的扑克牌排序为：方片 梅花 红桃 黑桃
// 扑克：3 3 3 3 4 4 4 4 5 ... ... JOKER0 JOKER1
// 索引：0 1 2 3 4 5 6 7 8 ... ... 52     53
//
//
// 目前使用的扑克位图是从网上找到的，不是自己画的，其排列如下：
//
// 方片：A  2  3  4  5  6  7  8  9  0  J  Q  K
// 梅花：A  2  3  4  5  6  7  8  9  0  J  Q  K
// 红桃：A  2  3  4  5  6  7  8  9  0  J  Q  K
// 黑桃：A  2  3  4  5  6  7  8  9  0  J  Q  K
//       小 大 背
//
//
// 为便于绘图扑克牌，需进行索引转换，使扑克牌索引正确对应到相应的位图。
//
// 说明：
//  扑克牌的位图共有5行，每行13张牌位图，其中，每5行只有3张位图，单张
//  牌位图尺寸为96x71像素。令其索引为从左到右，从上到下，依次为0～54。
//
//  绘图程序根据扑克索引获得位图索引后，除以13得到位图所在的行，取模13
//  得到位图所在的列，再根据单张牌的尺寸，即可方便绘制出扑克牌。
//
//  如果传入的扑克索引为-1，表示取扑克牌背面位图。
//
//
//  将扑克牌索引转换成如上所述的扑克牌位图索引
int CommonUtil::PokerIndexToBmpIndex(int nPokerIndex)
{
    int value; // 扑克牌的值
    int color; // 扑克牌的花色

    if (nPokerIndex == LORD_POKER_BACK_INDEX) return 55; // 地主扑克牌背面

    if ((nPokerIndex <= POKER_BACK_INDEX) || (nPokerIndex >= 54)) return 54; // 背面
    if (nPokerIndex == 52) return 52; // 小王
    if (nPokerIndex == 53) return 53; // 大王

    color = nPokerIndex % 4;
    value = nPokerIndex / 4;

    // 根据位图的排列，稍作修改
#if 0
    switch (value) {
        case 0:     value = 2;      break;
        case 1:     value = 3;      break;
        case 2:     value = 4;      break;
        case 3:     value = 5;      break;
        case 4:     value = 6;      break;
        case 5:     value = 7;      break;
        case 6:     value = 8;      break;
        case 7:     value = 9;      break;
        case 8:     value = 10;     break;
        case 9:     value = 11;     break;
        case 10:    value = 12;     break;
        case 11:    value = 0;      break;
        case 12:    value = 1;      break;
    }
#else
    value = (value + 2) % 13;
#endif

    return value + color * 13;
}

// 播放声音文件
void CommonUtil::PlaySound(UINT nSndType)
{
    switch (nSndType) {
        case SND_GAME_START:
            sndPlaySound(_T("start.wav"), SND_ASYNC);
            break;

        case SND_GAME_OUTPUT_CARD:
            sndPlaySound(_T("outcard.wav"), SND_ASYNC);
            break;

        case SND_GAME_WIN:
            sndPlaySound(_T("win.wav"), SND_ASYNC);
            break;

        case SND_GAME_BOMB:
            sndPlaySound(_T("bomb.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE0_F:
            sndPlaySound(_T("0_f.wav"), SND_ASYNC);
            break;

        case SND_GAME_RUNAWAY:
            sndPlaySound(_T("runaway.wav"), SND_ASYNC);
            break;

        case SND_GAME_TIMEOUT:
            sndPlaySound(_T("timeout.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE1_F:
            sndPlaySound(_T("1_f.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE2_F:
            sndPlaySound(_T("2_f.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE3_F:
            sndPlaySound(_T("3_f.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE0_M:
            sndPlaySound(_T("0_m.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE1_M:
            sndPlaySound(_T("1_m.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE2_M:
            sndPlaySound(_T("2_m.wav"), SND_ASYNC);
            break;

        case SND_GAME_SCORE3_M:
            sndPlaySound(_T("3_m.wav"), SND_ASYNC);
            break;
    }
}

#ifdef _USE_CRC16
// 构造 16 位 CRC 表
unsigned long CommonUtil::Table_CRC16[256] = { 0 };
BOOL CommonUtil::bCrc16TableBuilded = FALSE;

void CommonUtil::BuildTable16(unsigned short aPoly)
{
    unsigned short i, j;
    unsigned short nData;
    unsigned short nAccum;

    for (i = 0; i < 256; i++) {
        nData = (unsigned short)(i << 8);
        nAccum = 0;
        for (j = 0; j < 8; j++) {
            if ((nData ^ nAccum) & 0x8000) {
                nAccum = (nAccum << 1) ^ aPoly;
            } else {
                nAccum <<= 1;
            }
            nData <<= 1;
        }
        Table_CRC16[i] = (unsigned long)nAccum;
    }
}

// 计算 16 位 CRC 值，CRC-16 或 CRC-CCITT
unsigned short CommonUtil::CRC16(unsigned char * aData, unsigned long aSize)
{
    unsigned long i;
    unsigned short nAccum = 0;

    if (bCrc16TableBuilded == FALSE) {
        BuildTable16(cnCRC_16); // or cnCRC_CCITT
        bCrc16TableBuilded = TRUE;
    }

    for (i = 0; i < aSize; i++) {
        nAccum = (nAccum << 8) ^ (unsigned short)Table_CRC16[(nAccum >> 8) ^ *aData++];
    }

    return nAccum;
}
#endif

#ifdef _USE_CRC32
// 构造 32 位 CRC 表
unsigned long CommonUtil::Table_CRC32[256] = { 0 };
BOOL CommonUtil::bCrc32TableBuilded = FALSE;

void CommonUtil::BuildTable32(unsigned long aPoly)
{ 
    unsigned long i, j;
    unsigned long nData;
    unsigned long nAccum;

    for (i = 0; i < 256; i++) {
        nData = (unsigned long)(i << 24);
        nAccum = 0;
        for (j = 0; j < 8; j++) {
            if ((nData ^ nAccum) & 0x80000000) {
                nAccum = (nAccum << 1) ^ aPoly;
            } else {
                nAccum <<= 1;
            }
            nData <<= 1;
        }
        Table_CRC32[i] = nAccum;
    }
}

// 计算 32 位 CRC-32 值 
unsigned long CommonUtil::CRC32(unsigned char * aData, unsigned long aSize)
{
    unsigned long i;
    unsigned long nAccum = 0;

    if (bCrc32TableBuilded == FALSE) {
        BuildTable32(cnCRC_32);
        bCrc32TableBuilded = TRUE;
    }

    for (i = 0; i < aSize; i++) {
        nAccum = (nAccum << 8) ^ Table_CRC32[(nAccum >> 24) ^ *aData++];
    }

    return nAccum; 
}
#endif

#ifdef PARENT_PAINT_CHILD
void CommonUtil::ParentPaintChild(HWND hWndParent, HWND hWndChild, LPRECT lpRectOfChild, BOOL bErase)
{
    RECT rect;

    if (lpRectOfChild == NULL) {
        GetClientRect(hWndChild, &rect);
    } else {
        CopyRect(&rect, lpRectOfChild);
    }

    ClientRectToScreen(hWndChild, &rect);
    ScreenRectToClient(hWndParent, &rect);

    InvalidateRect(hWndParent, &rect, bErase);
}
#endif

void CommonUtil::ClientRectToScreen(HWND hWnd, LPRECT lpRect)
{
    assert(lpRect != NULL);

    POINT ptLT = { lpRect->left, lpRect->top };
    POINT ptRB = { lpRect->right, lpRect->bottom };

    ClientToScreen(hWnd, &ptLT);
    ClientToScreen(hWnd, &ptRB);

    SetRect(lpRect, ptLT.x, ptLT.y, ptRB.x, ptRB.y);
}

void CommonUtil::ScreenRectToClient(HWND hWnd, LPRECT lpRect)
{
    assert(lpRect != NULL);

    POINT ptLT = { lpRect->left, lpRect->top };
    POINT ptRB = { lpRect->right, lpRect->bottom };

    ScreenToClient(hWnd, &ptLT);
    ScreenToClient(hWnd, &ptRB);

    SetRect(lpRect, ptLT.x, ptLT.y, ptRB.x, ptRB.y);
}

int CommonUtil::QuickSortPartition(int a[], int low, int high)
{
    int i, j;
    int compare;

    i = low;
    j = high;
    compare = a[low];

    while (i < j) { 
        while ((i < j) && (compare <= a[j])) {
            j--;
        }

        if (i < j) {
            a[i] = a[j];
            i++;
        }

        while ((i < j) && (compare > a[i])) {
            i++;
        }

        if (i < j) {
            a[j] = a[i];
            j--;
        }
    }

    a[i] = compare;

    return i;
}

void CommonUtil::QuickSort(int a[], int low, int high)
{
    int pos;

    if (low < high) {
        pos = QuickSortPartition(a, low, high);
        QuickSort(a, low, pos - 1);
        QuickSort(a, pos + 1, high);
    }
}
