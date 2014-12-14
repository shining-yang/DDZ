//
// ServerLog.cpp
//
//      用于输出各种类型的日志
//

#include "stdafx.h"
#include "ServerLog.h"

extern HWND         g_hLogWnd;

BOOL                s_bServerLogStarted = FALSE;
static DWORD        s_logCurLevel = 0;

static const TCHAR*   s_logLevelString[] = {
    _T("INFO"), _T("WARNING"), _T("ERROR"), _T("DEBUG")
};
static const COLORREF s_logLevelStringColor[] = {
    RGB(0,0,0), RGB(0,0,192), RGB(192,0,0), RGB(192,192,192)
};


// 初始化并启动服务器日志功能
BOOL StartServerLog(void)
{
    if (s_bServerLogStarted == FALSE) {
        LogEnableAll(TRUE);
        s_bServerLogStarted = TRUE;
    }

    return TRUE;
}

// 停止服务日志功能
void StopServerLog(void)
{
    if (s_bServerLogStarted == TRUE) {
        LogEnableAll(FALSE);
        s_bServerLogStarted = FALSE;
    }
}

// 打开或关闭所有日志选项
void LogEnableAll(BOOL enable)
{
    if (enable == TRUE) {
        s_logCurLevel = LOG_INFO | LOG_WARN | LOG_ERROR | LOG_DEBUG;
    } else {
        s_logCurLevel = 0;
    }
}

// 打开或关闭INFO日志选项
void LogEnableInfo(BOOL enable)
{
    if (enable == TRUE) {
        s_logCurLevel |= LOG_INFO;
    } else {
        s_logCurLevel &= ~LOG_INFO;
    }
}

// 打开或关闭WARNING日志选项
void LogEnableWarn(BOOL enable)
{
    if (enable == TRUE) {
        s_logCurLevel |= LOG_WARN;
    } else {
        s_logCurLevel &= ~LOG_WARN;
    }
}

// 打开或关闭ERROR日志选项
void LogEnableError(BOOL enable)
{
    if (enable == TRUE) {
        s_logCurLevel |= LOG_ERROR;
    } else {
        s_logCurLevel &= ~LOG_ERROR;
    }
}

// 打开或关闭DEBUG日志选项
void LogEnableDebug(BOOL enable)
{
    if (enable == TRUE) {
        s_logCurLevel |= LOG_DEBUG;
    } else {
        s_logCurLevel &= ~LOG_DEBUG;
    }
}

//
// output log
//
//  Parameter:
//      level   - log type, see LOG_LEVEL
//      fmt     - log message to be printed
//  Remark:
//      The number of characters of the output message should not exceed 220.
//
void WriteLog(LOG_LEVEL level, TCHAR* fmt, ...)
{
    if ((level & s_logCurLevel) == 0) { return; }
    if (fmt == NULL) { return; }
    if (level > LOG_DEBUG) { level = LOG_DEBUG; }

    int len = 0;
    int index = 0;
    SYSTEMTIME time = { 0 };
    TCHAR szTime[32] = { 0 };
    TCHAR szLogBuf[MAX_LOG_LEN] = { 0 };

    if (level == LOG_INFO) {
        index = 0;
    } else if (level == LOG_WARN) {
        index = 1;
    } else if (level == LOG_ERROR) {
        index = 2;
    } else if (level == LOG_DEBUG) {
        index = 3;
    }

    GetLocalTime(&time);
    _stprintf_s(szTime, sizeof(szTime) / sizeof(szTime[0]), _T("%04d-%02d-%02d %02d:%02d:%02d"),
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

    // time
    len += _stprintf_s(szLogBuf + len, VALID_LOG_LEN - len, _T("["));
    len += _stprintf_s(szLogBuf + len, VALID_LOG_LEN - len, _T("%s"), szTime);
    len += _stprintf_s(szLogBuf + len, VALID_LOG_LEN - len, _T("]"));

    // log type
    len += _stprintf_s(szLogBuf + len, VALID_LOG_LEN - len,
                        _T(" %s: "), s_logLevelString[index]);

    // log message
    va_list va;
    va_start(va, fmt);

    int nStrLen = (int)_vsctprintf(fmt, va);
    if (nStrLen >= MAX_LOG_LEN - 30) {
        index = 1;
        len += _stprintf_s(szLogBuf, VALID_LOG_LEN - len,
            _T(" [WARNING]: MESSAGE TOO LONG TO BE LOGGED!\r\n"));
        va_end(va);
        goto READY_TO_WRITE_LOG;
    }

    len += _vstprintf_s(szLogBuf + len, VALID_LOG_LEN - len, fmt, va);
    va_end(va);

    len += _stprintf_s(szLogBuf + len, VALID_LOG_LEN - len, _T("\r\n"));

READY_TO_WRITE_LOG:
    GETTEXTLENGTHEX gtlex;
    gtlex.codepage = 1200;
    gtlex.flags = GTL_DEFAULT;
    
    int nCurLen;
    nCurLen = (int)SendMessage(g_hLogWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtlex, 0);

    CHARRANGE cr;
    cr.cpMin = nCurLen;
    cr.cpMax = -1;

    // 将文本追加到末尾
    SendMessage(g_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessage(g_hLogWnd, EM_REPLACESEL, (WPARAM)&cr, (LPARAM)szLogBuf);

    // 设置刚追加到末尾的文本的颜色与字符集
    SendMessage(g_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT cf;
    cf.cbSize = sizeof(CHARFORMAT);
    SendMessage(g_hLogWnd, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

    cf.dwMask = CFM_COLOR | CFM_CHARSET;
    cf.dwEffects &= ~CFE_AUTOCOLOR;
    cf.crTextColor = s_logLevelStringColor[index];
    cf.bCharSet = GB2312_CHARSET;
    SendMessage(g_hLogWnd, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

    cr.cpMax = cr.cpMin = -1;
    SendMessage(g_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    // 将滚动条移到最底端
    SendMessage(g_hLogWnd, WM_VSCROLL, (WPARAM)SB_BOTTOM, 0);
}

// 清除日志
void ClearLog(void)
{
    //CHARRANGE cr;
    //cr.cpMin = 0;
    //cr.cpMax = -1;

    //SendMessage(g_hLogWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    //SendMessage(g_hLogWnd, EM_REPLACESEL, (WPARAM)&cr, (LPARAM)_T(""));

    SendMessage(g_hLogWnd, WM_SETTEXT, 0, (LPARAM)_T(""));
}