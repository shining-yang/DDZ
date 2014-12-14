//
// Utility.h
//
//      Common utility functions
//

#pragma once

#ifdef _DEBUG
#define DEBUG_TRACE_LEN             256

extern TCHAR dbg_TraceTextBuf [];

#define TRACE(txt)     OutputDebugString (txt)

#define TRACE1(fmt,parm1)  \
{ _stprintf_s (dbg_TraceTextBuf, DEBUG_TRACE_LEN, fmt, parm1); \
    OutputDebugString (dbg_TraceTextBuf); }

#define TRACE2(fmt,parm1,parm2)  \
{ _stprintf_s (dbg_TraceTextBuf, DEBUG_TRACE_LEN, fmt, parm1, parm2); \
    OutputDebugString (dbg_TraceTextBuf); }

#define TRACE3(fmt,parm1,parm2,parm3)  \
{ _stprintf_s (dbg_TraceTextBuf, DEBUG_TRACE_LEN, fmt, parm1, parm2, parm3); \
    OutputDebugString (dbg_TraceTextBuf); }
#else
#define TRACE(txt)     ((void)0)
#define TRACE1(fmt,parm1)     ((void)0)
#define TRACE2(fmt,parm1,parm2)     ((void)0)
#define TRACE3(ftm,parm1,parm2,parm3)   ((void)0)
#endif // _DEBUG


class CommonUtil
{
public:
    static int PokerIndexToBmpIndex(int nPokerIndex);

    static void PlaySound(UINT nSndType);

public:

#ifdef _USE_CRC16
private:
    // CRC-16 = X16 + X15 + X2 + X0
    static const unsigned short cnCRC_16 = 0x8005;
    // CRC-CCITT = X16 + X12 + X5 + X0
    static const unsigned short cnCRC_CCITT = 0x1021;
    // CRC16 表 
    static unsigned long Table_CRC16[256];
    // CRC 表是否已经构建
    static BOOL bCrc16TableBuilded;

public:
    static void BuildTable16(unsigned short aPoly);
    static unsigned short CRC16(unsigned char * aData, unsigned long aSize);
#endif

#ifdef _USE_CRC32
private:
    // CRC-32 = X32 + X26 + X23 + X22 + X16 + X11 + X10 + X8 + X7 + X5 + X4 + X2 + X1 + X0
    static const unsigned long cnCRC_32 = 0x04C10DB7;
    // CRC32 表 
    static unsigned long Table_CRC32[256];
    // CRC 表是否已经构建
    static BOOL bCrc32TableBuilded;

public:
    static void BuildTable32(unsigned long aPoly);
    static unsigned long CRC32(unsigned char * aData, unsigned long aSize);
#endif

public:
#ifdef PARENT_PAINT_CHILD
    static void ParentPaintChild(HWND hWndParent, HWND hWndChild, LPRECT lpRectOfChild, BOOL bErase);
#endif

    static void ClientRectToScreen(HWND hWnd, LPRECT lpRect);
    static void ScreenRectToClient(HWND hWnd, LPRECT lpRect);

private:
    static int QuickSortPartition(int a[], int low, int high);
public:
    static void QuickSort(int a[], int low, int high); // 升序
};