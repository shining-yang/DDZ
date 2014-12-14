#pragma once

#include "resource.h"

#define MAX_LOADSTRING                      32

#define MAIN_WND_MIN_WIDTH                  720
#define MAIN_WND_MIN_HEIGHT                 640

#define MAIN_WND_MARGIN                     4
#define MAIN_WND_MARGIN_CLR                 RGB(64,224,64)
#define CX_USER_INFO_WND                    240
#define CY_USER_INFO_WND                    60

#define CX_SERVER_LIST_WND                  CX_USER_INFO_WND

#define CY_PAGE_TAB_CTRL                    36

#define CX_USER_LIST_WND                    300


// 用户登陆基本信息
typedef struct USER_LOGON_INFO_t {
    TCHAR   szName[MAX_USER_NAME_LEN];
    int     nImage;
    BOOL    bMale;
} USER_LOGON_INFO;


// 此代码模块中包含的函数的前向声明:
ATOM				MainWndRegister(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void ExitInstance(void);
void OnPaint(HDC hdc);
void OnSize(WPARAM wParam, LPARAM lParam);
void DisconnectServer(void);
void OnConnectServer(WPARAM wParam, LPARAM lParam);
void OnPageReturn(HWND hWnd, LPARAM lParam);
void OnPageSelChanging(HWND hWnd, LPARAM lParam);
void OnPageSelChange(HWND hWnd, LPARAM lParam);
void OnUserClickSeat(int table, int seat);

BOOL IsFirstInstance(void);
BOOL RegisterWindowClass(HINSTANCE hInstance);
void FlashInactiveWnd(HWND hWnd, UINT uCount);
void FlashInactiveWnd(void);
BOOL ProcessNetMessage(LPMSG lpMsg);

void On_TM_CONNECTION_LOST(WPARAM wParam, LPARAM lParam);
void On_TM_RECV_SVR_ROOM_INFO(WPARAM wParam, LPARAM lParam);
void On_TM_RECV_SVR_STATUS(WPARAM wParam, LPARAM lParam);
void On_TM_RECV_PLAYER_STATE_CHANGE(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_TAKE_SEAT(WPARAM wParam, LPARAM lParam);
void On_TM_LOOKON_TAKE_SEAT(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_LEAVE_SEAT(WPARAM wParam, LPARAM lParam);
void On_TM_LOOKON_LEAVE_SEAT(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_READY(WPARAM wParam, LPARAM lParam);
void On_TM_DISTRIBUTE_POKER(WPARAM wParam, LPARAM lParam);
void On_TM_SVR_REQ_VOTE_LORD(WPARAM wParam, LPARAM lParam);
void On_TM_SVR_NOTIFY_VOTE_LORD(WPARAM wParam, LPARAM lParam);
void On_TM_VOTE_LORD_FINISH(WPARAM wParam, LPARAM lParam);
void On_TM_SVR_REQ_OUTPUT_CARD(WPARAM wParam, LPARAM lParam);
void On_TM_SVR_NOTIFY_OUTPUT_CARD(WPARAM wParam, LPARAM lParam);
void On_TM_SVR_NOTIFY_GAME_OVER(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_CHATTING(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_DELEGATED(WPARAM wParam, LPARAM lParam);
void On_TM_GAMER_DISCONNECTED(WPARAM wParam, LPARAM lParam);


