//
// ServerLog.h
//
//  打印服务器日志
//
#pragma once

// 最多允许一次输出日志的字符数量
#define MAX_LOG_LEN                     256

// 有效的字符数量，（最大值除去回车换行和结束符）
#define VALID_LOG_LEN                   (MAX_LOG_LEN - 3)

#define LOG_OPT_NUM                     4
// Log level
typedef enum
{
    LOG_INFO    = 1UL,
    LOG_WARN    = 2UL,
    LOG_ERROR   = 4UL,
    LOG_DEBUG   = 8UL
} LOG_LEVEL;


// 初始化并启动服务器日志功能
BOOL StartServerLog(void);
// 停止服务日志功能
void StopServerLog(void);

// set log level
void LogEnableAll(BOOL enable);
void LogEnableInfo(BOOL enable);
void LogEnableWarn(BOOL enable);
void LogEnableError(BOOL enable);
void LogEnableDebug(BOOL enable);

// output log
void WriteLog(LOG_LEVEL level, TCHAR* fmt, ...);
// clear log
void ClearLog(void);
