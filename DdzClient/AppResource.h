//
// File: AppResource.h
//
#pragma once

#define MAX_IMG_ICON_NUM            32
#define USER_ICON_INIT_NUM          16
#define USER_ICON_GROW_NUM          8

#define CX_USER_ICON_BIG            32
#define CY_USER_ICON_BIG            32

#define CX_USER_ICON_SML            16
#define CY_USER_ICON_SML            16

#define CX_SERVER_TREE_ICON         32
#define CY_SERVER_TREE_ICON         32

#define CX_USER_VISUAL              140
#define CY_USER_VISUAL              226

#define CX_TAB_CTRL_ICON            24
#define CY_TAB_CTRL_ICON            24

#define CX_GAME_INFO_IMAGE          80
#define CY_GAME_INFO_IMAGE          40

#define CX_ICON_LORD                48
#define CY_ICON_LORD                48

#define CX_ICON_DELEGATE            32
#define CY_ICON_DELEGATE            32

#define CX_ICON_CONNLOST            48
#define CY_ICON_CONNLOST            48

#define CX_ICON_BOMB                32
#define CY_ICON_BOMB                32

#define CX_ICON_CLOCK               64
#define CY_ICON_CLOCK               64

//  ±÷” ˝◊÷ŒªÕº≥ﬂ¥Á
#define CX_CLOCK_DIGITAL            13
#define CY_CLOCK_DIGITAL            23

#define DEF_IMG_MASK_CLR            RGB(0,128,128)
#define GAME_INFO_MASK_CLR          RGB(255,255,255)


extern HIMAGELIST g_himlUser16x16;
extern HIMAGELIST g_himlUser32x32;
extern HIMAGELIST g_himlServerTree;
extern HIMAGELIST g_himlTabCtrl;
extern HIMAGELIST g_himlGameInfo;

extern HBITMAP  g_hbmpTable;
extern HBITMAP  g_hbmpTableStart;
extern HICON    g_hiconReady;

extern HICON    g_hiconLord;
extern HICON    g_hiconBomb;
extern HICON    g_hiconDelegated;
extern HICON    g_hiconConnLost;
extern HICON    g_hiconClock;

extern HBITMAP  g_hbmpGameBG;
extern HBITMAP  g_hbmpPoker;
extern HBITMAP  g_hbmpClockDigital;

// ”Œœ∑…˘“Ù
typedef enum _SND_TYPE_e {
    SND_GAME_START = 0,
    SND_GAME_OUTPUT_CARD,
    SND_GAME_BOMB,
    SND_GAME_WIN,
    SND_GAME_RUNAWAY,
    SND_GAME_TIMEOUT,
    SND_GAME_SCORE0_M,
    SND_GAME_SCORE1_M,
    SND_GAME_SCORE2_M,
    SND_GAME_SCORE3_M,
    SND_GAME_SCORE0_F,
    SND_GAME_SCORE1_F,
    SND_GAME_SCORE2_F,
    SND_GAME_SCORE3_F,
} SND_TYPE;

BOOL LoadAppResource(void);
void FreeAppResource(void);

HBITMAP LoadCommonImage(LPCTSTR szFileName);
HBITMAP LoadCommonImage(HMODULE hModule, LPCTSTR szResName, LPCTSTR szResType);

static BOOL LoadAppImageRes(void);
static void DestroyAppImageRes(void);

static BOOL LoadAppSoundRes(void);
static void FreeAppSoundRes(void);
void PlayGameSound(SND_TYPE sndType);