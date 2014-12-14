//
// AppDef.h
//
//

#pragma once

// 自定义窗口消息
#define WM_PAGE_SEL_CHANGING                (WM_USER + 20)
#define WM_PAGE_SEL_CHANGE                  (WM_USER + 21)
#define WM_PAGE_RETURN                      (WM_USER + 22)

#define WM_SHOW_HOME_PAGE                   (WM_USER + 30)
#define WM_CONNECT_SERVER                   (WM_USER + 31)
#define WM_USER_CLICK_SEAT                  (WM_USER + 32)

// 用于处理截获的聊天消息COMBOBOX编辑框的 ESC、RETURN 按键事件
#define WM_CHATMSG_VKEY                     (WM_USER + 40)

#define WM_WRITE_LOG                        (WM_USER + 41)
#define WM_GAME_OVER                        (WM_USER + 42)

// 线程消息
#define TM_CONNECTION_LOST                  (WM_APP + 101)
#define TM_RECV_SVR_ROOM_INFO               (WM_APP + 102)
#define TM_RECV_SVR_STATUS                  (WM_APP + 103)
#define TM_RECV_PLAYER_STATE_CHANGE         (WM_APP + 104)
#define TM_GAMER_TAKE_SEAT                  (WM_APP + 105)
#define TM_LOOKON_TAKE_SEAT                 (WM_APP + 106)
#define TM_GAMER_LEAVE_SEAT                 (WM_APP + 107)
#define TM_LOOKON_LEAVE_SEAT                (WM_APP + 108)
#define TM_GAMER_READY                      (WM_APP + 109)
#define TM_DISTRIBUTE_POKER                 (WM_APP + 110)
#define TM_SVR_REQ_VOTE_LORD                (WM_APP + 111)
#define TM_SVR_NOTIFY_VOTE_LORD             (WM_APP + 112)
#define TM_VOTE_LORD_FINISH                 (WM_APP + 113)
#define TM_SVR_REQ_OUTPUT_CARD              (WM_APP + 114)
#define TM_SVR_NOTIFY_OUTPUT_CARD           (WM_APP + 115)
#define TM_SVR_NOTIFY_GAME_OVER             (WM_APP + 116)
#define TM_GAMER_CHATTING                   (WM_APP + 117)
#define TM_GAMER_DELEGATED                  (WM_APP + 118)
#define TM_GAMER_DISCONNECTED               (WM_APP + 119)

//
// 程序作为单一实例运行
// Debug版本将允许一台计算机运行多个实例；Release版本限制一台计算机运行一个实例
//
#ifndef _DEBUG
#define RUN_AS_SINGLE_INSTANCE
#endif

//
// 父窗口负责绘制子窗口
//
// 游戏窗口包括好几个子窗口，如当前牌窗口，出牌窗口，底牌窗口。考虑到窗口尺寸变化
// 时，尽量使界面绘制时不闪烁，特使用该宏。这些窗口的绘制工作将由其父窗口调用相应
// 子窗口接口完成。这样，父窗口在绘制界面时，将子窗口一齐绘制到内存DC，然后再刷到
// 显示屏幕，减少窗口尺寸变动时绘图的闪烁。
//
// 定义该宏之后，CurPokerWnd, OutCardWnd, UnderCardWnd 原先设计的绘图代码将不会被
// 调用到，当窗口需要绘制时，由其父窗口完成，因而，它们将不必SaveParentBackground。
// GamerVisualWnd 也使用此机制由父窗口进行绘制，但因其使用 PictureEx 处理图像，它
// 仍需要保存父窗口背景传递给 PictureEx，并响应 PictureEx 绘图请求。
//
// 由父窗口绘制子窗口的机制，是为了尽量避免窗口尺寸变动时闪烁而引入的。这样处理，
// 子窗口如CurPokerWnd，OutCardWnd就不必再保存父窗口背景位图。每次子窗口需要绘制
// 时，都由父窗口去完成，必然比其自己绘制执行了更多的代码，因为父窗口绘制时，除了
// 绘制该子窗口外，还绘制了其它所有子窗口。
//
// 因为玩家掉线图标被放置在各玩家当前牌窗口中央，若不使用宏PARETN_PAINT_CHILD，则
// 玩家掉线图标将被当前牌窗口遮盖住。
//
#define PARENT_PAINT_CHILD

// 服务器发送给客户端的网络消息数据由主线程处理，还是接收数据的线程自己处理
//
// 如果开启宏 MAIN_THREAD_PROCESS_NMSG，则主线程在处理消息时，不能弹出菜单或
// 模式对话框，因为，那样将阻止主线程接收并处理服务器定时发送的消息或其它消息，
// 从而导致客户端与服务器所维护的玩家信息不一致。
//
// 没有其它好的解决办法。因此，不开启该宏。网络消息将由连接线程直接处理。
//
//#define MAIN_THREAD_PROCESS_NMSG

// 使用CRC16算法
#define _USE_CRC16

// 扑克牌位图相关
#define POKER_BACK_INDEX                    -1
#define LORD_POKER_BACK_INDEX               -2

#define POKER_BMP_UNIT_WIDTH                71
#define POKER_BMP_UNIT_HEIGHT               96

#define CPW_POKER_H_SPACE                   15
#define CPW_POKER_POPUP_SPACE               18
#define CPW_POKER_V_SPACE                   17

#define OCW_POKER_H_SPACE                   CPW_POKER_H_SPACE

#define UCW_POKER_H_SPACE                   8

// 首页链接
#ifdef USE_LOCAL_FILE_AS_HOMEPAGE
#define DEFAULT_HOME_PAGE                   _T("readme.txt")
#else
#define DEFAULT_HOME_PAGE                   _T("http://minigameimg.qq.com/help/rule81.html")
#endif

#define DDZ_GAME_SEC                        _T("GAME_ENV")
#define GAME_KEY_BG_IMAGE                   _T("bg_image")
#define GAME_BG_DEF_BMP_FILE                _T("game_bg.bmp")

#define DDZ_CLIENT_INI_FILE                 _T("DdzClient.ini")
#define SERVER_LIST_INI_FILE                _T("ServerList.ini")

// 动态链接库名称
#define POIKER_LOGIC_DLL_NAME               _T("PokerLib.dll")
#define RES_IMAGE_DLL_NAME                  _T("ResImage.dll")
#define RES_POKER_DLL_NAME                  _T("ResPoker.dll")
#define RES_SOUND_DLL_NAME                  _T("ResSound.dll")
#define CWEBPAGE_DLL_NAME                   _T("CWebPage.dll")

// PageTabCtrl 的参数
#define HOME_PAGE_TAB_TEXT                  _T("游戏首页")
#define ROOM_PAGE_TAB_TEXT                  _T("游戏房间")
#define GAME_PAGE_TAB_TEXT                  _T("斗地主")

#define HOME_PAGE_TAB_PARAM                 0
#define ROOM_PAGE_TAB_PARAM                 1
#define GAME_PAGE_TAB_PARAM                 2

#define HOME_PAGE_TAB_INDEX                 0
#define ROOM_PAGE_TAB_INDEX                 1
#define GAME_PAGE_TAB_INDEX                 2

#define HOME_PAGE_TAB_IMAGE_INDEX           0
#define ROOM_PAGE_TAB_IMAGE_INDEX           1
#define GAME_PAGE_TAB_IMAGE_INDEX           2




